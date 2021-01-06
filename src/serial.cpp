/*
 * serial.cpp
 *
 *  Created on: 05.01.2021
 *      Author: DI Andreas Auer
 */

#include "serial.h"

#include <Poco/NumberFormatter.h>

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>
#include <cstdio>

using namespace Poco;

//------------------------------------------------------------------------------
Serial::Serial() :
    logger_(Logger::get("Serial")),
    fd_(-1),
    running_(false),
    thread_(nullptr),
    listener_(nullptr)
{

}

//------------------------------------------------------------------------------
Serial::~Serial()
{
    close();
}

//------------------------------------------------------------------------------
void Serial::setListener(Serial::Listener *listener)
{
    listener_ = listener;
}

//------------------------------------------------------------------------------
int Serial::getFd() const
{
    return fd_;
}

//------------------------------------------------------------------------------
bool Serial::open(const std::string &device, int baudrate)
{
    struct termios tty;
    int baud = B115200;

    fd_ = ::open(device.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if(fd_ == -1)
    {
        perror("Unable to open port");
        return false;
    }

    tcgetattr(fd_, &tty);
    tty.c_cflag = CS8 | CREAD | CLOCAL;
    tty.c_iflag = 0;
    tty.c_oflag = 0;
    tty.c_lflag = 0;
    tty.c_cc[VMIN] = 3;
    tty.c_cc[VTIME] = 5;

    switch(baudrate)
    {
        case 9600:   baud = B9600;   break;
        case 19200:  baud = B19200;  break;
        case 38400:  baud = B38400;  break;
        case 57600:  baud = B57600;  break;
        case 115200: baud = B115200; break;
        default:
            baud = B38400;
    }

    cfsetospeed(&tty, baud);
    cfsetispeed(&tty, baud);

    tcsetattr(fd_, TCSANOW, &tty);
    tcsetattr(fd_, TCSAFLUSH, &tty);

    return true;
}

//------------------------------------------------------------------------------
void Serial::close()
{
    if(thread_)
    {
        running_ = false;
        thread_ = nullptr;
        ::close(fd_);
        fd_ = -1;
    }
    logger_.information("closed");
}

//------------------------------------------------------------------------------
int32_t Serial::send(const std::vector<uint8_t> &data)
{
    return send(data.data(), data.size());
}

//------------------------------------------------------------------------------
int32_t Serial::send(const uint8_t *data, uint32_t size)
{
    return ::write(fd_, data, size);
}

//------------------------------------------------------------------------------
std::vector<uint8_t> Serial::receive()
{
    uint8_t buffer[4096];
    size_t len = read(fd_, buffer, sizeof(buffer));

    if(len > 0)
    {
        return std::vector<uint8_t>(buffer, buffer + len);
    }

    return std::vector<uint8_t>();
}

//------------------------------------------------------------------------------
uint32_t Serial::receive(uint8_t *data, uint32_t max_len)
{
    return ::read(fd_, data, max_len);
}

//------------------------------------------------------------------------------
void Serial::run()
{
    char buffer[2048];
    int len = 0;
    int ret;
    struct timeval timeout;
    fd_set read_set;

    thread_ = Thread::current();

    running_ = true;
    while(running_)
    {
        FD_ZERO(&read_set);
        FD_SET(fd_, &read_set);
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        ret = select(fd_ + 1, &read_set, 0, 0, &timeout);
        if(ret > 0)
        {
            len = read(fd_, buffer, sizeof(buffer));
            if(len > 0)
            {
                try
                {
                    if(listener_)
                    {
                        listener_->dataReceived((uint8_t *)buffer, (uint32_t)len);
                    }
                }
                catch(std::exception &e)
                {
                    logger_.error("[" + NumberFormatter::format(__LINE__) + "] " + e.what());
                }
            }
            else
            {
                logger_.error("Serial port closed");
                ::close(fd_);
                fd_ = -1;
                if(listener_)
                {
                    listener_->portClosed();
                }
                break;
            }
        }
        else if(ret < 0)
        {
            break;
        }
    }
    logger_.error("Serial closed");
}
