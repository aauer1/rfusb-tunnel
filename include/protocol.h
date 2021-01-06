/*
 * protocol.h
 *
 *  Created on: 06.01.2021
 *      Author: DI Andreas Auer
 */
#pragma once

#include "frame.h"
#include "serial.h"
#include "blocking_queue.h"

#include <Poco/Logger.h>
#include <Poco/Runnable.h>

#include <vector>

class Protocol : public Poco::Runnable, public Serial::Listener
{
    public:
        class Listener
        {
            public:
                virtual void onFrameReceived(Frame *f) = 0;
        };

    protected:
        Poco::Logger &logger_;
        Poco::Thread *thread_;
        Serial *serial_;

        std::vector<uint8_t> buffer_;
        BlockingQueue<Frame *> tx_buffer_;

        Listener *listener_;

        Poco::Mutex mutex_;
        Poco::Condition cond_;

    public:
        Protocol(Serial *serial);
        virtual ~Protocol();

        void setListener(Protocol::Listener *listener);
        void reset();
        void close();

        void sendData(const uint8_t *data, uint32_t size);

        void addData(const uint8_t *data, uint32_t size);
        Frame *getFrame();

        virtual void dataReceived(uint8_t *buffer, uint32_t length);
        virtual void portClosed();
        void run();

};
