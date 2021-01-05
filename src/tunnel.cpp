/*
 * tunnel.cpp
 *
 *  Created on: 29.12.2020
 *      Author: DI Andreas Auer
 */

#include "tunnel.h"
#include "frame.h"
#include "utils.h"

#include <Poco/ConsoleChannel.h>
#include <Poco/PatternFormatter.h>
#include <Poco/FormattingChannel.h>
#include <Poco/Util/HelpFormatter.h>
#include <Poco/Util/Option.h>
#include <Poco/NumberFormatter.h>

#include <iostream>
#include <sstream>
#include <deque>

#include <signal.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/if_tun.h>
#include <net/if.h>

using namespace Poco;
using namespace Poco::Util;
using namespace std;

const std::string Tunnel::DEVICE = "/dev/net/tun";

//------------------------------------------------------------------------------
static void signalHandler(int sig)
{
    Tunnel *tunnel = reinterpret_cast<Tunnel *>(&ServerApplication::instance());
    tunnel->terminate();
}

//------------------------------------------------------------------------------
Tunnel::Tunnel() :
    help_(false),
    dev_("/dev/ttyACM0"),
    interface_("tun0"),
    tun_fd_(-1),
    terminate_(false)
{
    // Console Channel
    AutoPtr<ColorConsoleChannel> console_channel(new ColorConsoleChannel);
    AutoPtr<PatternFormatter> pattern_formater(new PatternFormatter);
    pattern_formater->setProperty("pattern", "[%Y-%m-%d %H:%M:%S:%i] %s: %t");
    pattern_formater->setProperty("times", "local");
    AutoPtr<FormattingChannel> formatting_channel(new FormattingChannel(pattern_formater, console_channel));

    Logger::root().setChannel(formatting_channel);
    Logger::root().setLevel(Message::PRIO_DEBUG);

    logger_ = &Logger::get("Tunnel");

    signal(SIGINT, &signalHandler);
}

//------------------------------------------------------------------------------
Tunnel::~Tunnel()
{
}

//------------------------------------------------------------------------------
void Tunnel::terminate()
{
    terminate_ = true;
}

//------------------------------------------------------------------------------
void Tunnel::initialize(Poco::Util::Application &app)
{
    ServerApplication::initialized();

    if(help_)
    {
        return;
    }

    tun_fd_ = open(interface_, IFF_TUN | IFF_NO_PI);
    if(tun_fd_ < 0)
    {
        logger_->error("Failed to alloc the tunnel interface: %s", interface_);
        return;
    }

    if(serial_.open(dev_, 115200) == false)
    {
        logger_->error("Cannot open serial device: %s", dev_);
        return;
    }
}

//------------------------------------------------------------------------------
void Tunnel::uninitialize()
{
    ServerApplication::uninitialize();
    if(tun_fd_ >= 0)
    {
        close(tun_fd_);
    }

    serial_.close();
}

//------------------------------------------------------------------------------
void Tunnel::reinitialize(Poco::Util::Application &app)
{
}

//------------------------------------------------------------------------------
void Tunnel::defineOptions(Poco::Util::OptionSet &options)
{
    options.addOption(Option("help", "h", "Show this help"));
    options.addOption(Option("debug", "d", "Specify a debug level")
            .argument("<Level>", true));
    options.addOption(Option("interface", "i", "Specify the tun interface (default: tun0)")
            .argument("<Interface>", true));
    options.addOption(Option("serial", "s", "Specify the serial device (default: /dev/ttyACM0)")
            .argument("<Interface>", true));
}

//------------------------------------------------------------------------------
void Tunnel::handleOption(const std::string &name, const std::string &value)
{
    if(name == "help")
    {
        HelpFormatter help(options());
        help.format(cout);
        help_ = true;
        stopOptionsProcessing();
    }
    else if(name == "debug")
    {
        int level = Logger::parseLevel(value);
        Logger::setLevel("", level);
    }
    else if(name == "interface")
    {
        interface_ = value;
    }
    else if(name == "serial")
    {
        dev_ = value;
    }
}

//------------------------------------------------------------------------------
int Tunnel::main(const std::vector<std::string> &args)
{
    if(help_)
    {
        return EXIT_USAGE;
    }

    if(tun_fd_ < 0)
    {
        return EXIT_FAILURE;
    }

    fd_set readfds;
    struct timeval timeout;
    uint8_t buffer[2048];

    std::vector<uint8_t> serial_buf;

    int serial_fd = serial_.getFd();

    while(!terminate_)
    {
        FD_ZERO(&readfds);
        FD_SET(tun_fd_, &readfds);
        FD_SET(serial_fd, &readfds);
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        int ret = select((serial_fd > tun_fd_) ? serial_fd + 1 : tun_fd_ + 1, &readfds, NULL, NULL, &timeout);
        if(ret > 0)
        {
            if(FD_ISSET(tun_fd_, &readfds))
            {
                int len = read(tun_fd_, buffer, sizeof(buffer));
                logger_->information("%?d bytes read from tun", len);
                if(len > 0)
                {
                    vector<uint8_t> data;

                    Frame frame(Frame::CMD_SEND);
                    frame.setData(buffer, len);
                    frame.serialize(data);

                    logger_->information("%s", Utils::hexDump(deque<uint8_t>(data.begin(), data.end())));
                    serial_.send(data.data(), data.size());
                }
            }
            else if(FD_ISSET(serial_fd, &readfds))
            {
                int len = serial_.receive(buffer, sizeof(buffer));
                logger_->information("%?d bytes read from serial", len);
                if(len > 0)
                {
                    serial_buf.insert(serial_buf.end(), buffer, buffer + len);
                    logger_->information("%s", Utils::hexDump(std::deque<uint8_t>(buffer, buffer + len)));

                    Frame *f = Frame::deserialize(serial_buf.data(), serial_buf.size());
                    if(f != nullptr)
                    {
                        logger_->information("%s", f->toString());
                    }

                    if(f != nullptr && f->getCommand() == Frame::CMD_RECEIVE)
                    {
                        vector<uint8_t> d = f->getData();
                        write(tun_fd_, d.data(), d.size());
                        delete f;
                    }
                }
                else
                {
                    logger_->warning("Serial interface closed");
                    break;
                }
            }
        }
        else if(ret < 0)
        {
            logger_->error("Interface closed");
            break;
        }
    }

    return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------
int Tunnel::open(const std::string &name, int flags)
{
    int fd = ::open(DEVICE.c_str(), O_RDWR);
    if(fd == -1)
    {
        logger_->error("Cannot open %s", DEVICE);
        return -1;
    }

    struct ifreq ifr = {0};
    ifr.ifr_flags = flags;
    strncpy(ifr.ifr_name, name.c_str(), IFNAMSIZ);

    int error = ioctl(fd, TUNSETIFF, &ifr);
    if(error < 0)
    {
        logger_->error("Cannot set interface");
        close(fd);
        fd = -1;
        return -1;
    }

    return fd;
}

