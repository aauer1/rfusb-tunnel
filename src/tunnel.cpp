/*
 * tunnel.cpp
 *
 *  Created on: 29.12.2020
 *      Author: DI Andreas Auer
 */

#include "tunnel.h"

#include <Poco/ConsoleChannel.h>
#include <Poco/PatternFormatter.h>
#include <Poco/FormattingChannel.h>
#include <Poco/Util/HelpFormatter.h>
#include <Poco/Util/Option.h>
#include <Poco/NumberFormatter.h>

#include <iostream>
#include <sstream>

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

    tun_fd_ = open(interface_, IFF_TUN | IFF_NO_PI);
    if(tun_fd_ < 0)
    {
        logger_->error("Failed to alloc the tunnel interface: %s", interface_);
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

    while(!terminate_)
    {
        FD_ZERO(&readfds);
        FD_SET(tun_fd_, &readfds);
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        int ret = select(tun_fd_ + 1, &readfds, NULL, NULL, &timeout);
        if(ret > 0)
        {
            int len = read(tun_fd_, buffer, sizeof(buffer));
            logger_->information("Read %?d bytes", len);
            if(len > 0)
            {
                stringstream ss;
                for(int i=0; i<len; i++)
                {
                    if(i % 16 == 0)
                    {
                        ss << endl;
                    }
                    ss << NumberFormatter::formatHex(buffer[i], 2, false) << " ";
                }
                logger_->information(ss.str());
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
