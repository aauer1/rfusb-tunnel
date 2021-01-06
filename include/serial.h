/*
 * serial.h
 *
 *  Created on: 05.01.2021
 *      Author: DI Andreas Auer
 */
#pragma once

#include <Poco/Logger.h>
#include <Poco/Runnable.h>
#include <Poco/Thread.h>

#include <string>
#include <vector>

class Serial : public Poco::Runnable
{
    public:
        class Listener
        {
            public:
                virtual void dataReceived(uint8_t *buffer, uint32_t length) = 0;
                virtual void portClosed() = 0;
        };

    protected:
        Poco::Logger &logger_;
        int fd_;
        bool running_;
        Poco::Thread *thread_;

        Listener *listener_;

    public:
        Serial();
        virtual ~Serial();
        void setListener(Serial::Listener *listener);

        int getFd() const;
        bool open(const std::string &device, int baudrate);
        void close();
        int32_t send(const std::vector<uint8_t> &data);
        int32_t send(const uint8_t *data, uint32_t size);
        std::vector<uint8_t> receive();
        uint32_t receive(uint8_t *data, uint32_t max_len);

        void run();
};
