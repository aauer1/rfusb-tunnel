/*
 * protocol.c
 *
 *  Created on: 06.01.2021
 *      Author: DI Andreas Auer
 */

#include "protocol.h"

#include <Poco/ThreadPool.h>

using namespace Poco;

//------------------------------------------------------------------------------
Protocol::Protocol(Serial *serial) :
    logger_(Logger::get("Protocol")),
    thread_(nullptr),
    serial_(serial),
    listener_(nullptr)
{
    serial->setListener(this);
}

//------------------------------------------------------------------------------
Protocol::~Protocol()
{
}

//------------------------------------------------------------------------------
void Protocol::setListener(Protocol::Listener *listener)
{
    listener_ = listener;
}

//------------------------------------------------------------------------------
void Protocol::reset()
{
    buffer_.clear();
}

//------------------------------------------------------------------------------
void Protocol::close()
{
    if(thread_ != nullptr)
    {
        tx_buffer_.put(nullptr);
        thread_ = nullptr;
    }
    serial_->close();
    logger_.information("closed");
}

//------------------------------------------------------------------------------
void Protocol::sendData(const uint8_t *data, uint32_t size)
{
    Frame *f = new Frame(Frame::CMD_SEND);
    f->setData(data, size);
    tx_buffer_.put(f);
}

//------------------------------------------------------------------------------
void Protocol::addData(const uint8_t* data, uint32_t size)
{
    buffer_.insert(buffer_.end(), data, data+size);
}

//------------------------------------------------------------------------------
Frame* Protocol::getFrame()
{
    Frame *f = Frame::deserialize(buffer_.data(), buffer_.size());
    if(f != nullptr)
    {
        if((f->getLength()+5) > buffer_.size())
        {
            logger_.warning("Frame size: %?d, Buffer size: %?d", f->getLength(), buffer_.size());
            logger_.warning(f->toString());
            buffer_.clear();
        }
        else
        {
            buffer_.erase(buffer_.begin(), buffer_.begin() + f->getLength() + 5);
        }
    }

    return f;
}

//------------------------------------------------------------------------------
void Protocol::dataReceived(uint8_t *buffer, uint32_t length)
{
    addData(buffer, length);

    while(true)
    {
        Frame *f = getFrame();
        if(f == nullptr)
        {
            break;
        }

        if(f->getFlags() & Frame::FLAG_ACK)
        {
            cond_.broadcast();
            logger_.information("Serial ACK");
        }
        else
        {
            if(listener_)
            {
                listener_->onFrameReceived(f);
            }
        }

        delete f;
    }
}

//------------------------------------------------------------------------------
void Protocol::portClosed()
{

}

//------------------------------------------------------------------------------
void Protocol::run()
{
    thread_ = Thread::current();

    ThreadPool::defaultPool().start(*serial_);

    while(true)
    {
        Frame *f = tx_buffer_.take(0);
        if(f == nullptr)
        {
            break;
        }

        try
        {
            Mutex::ScopedLock lock(mutex_);

            std::vector<uint8_t> buf;
            f->serialize(buf);
            serial_->send(buf.data(), buf.size());

            cond_.wait(mutex_, 1000);
        }
        catch(TimeoutException &te)
        {
            logger_.warning(te.message());
        }
    }
    logger_.error("Protocol closed");
}
