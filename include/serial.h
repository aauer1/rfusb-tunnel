/*
 * serial.h
 *
 *  Created on: 05.01.2021
 *      Author: DI Andreas Auer
 */
#pragma once

#include <string>
#include <vector>

class Serial
{
    protected:
        int fd_;

    public:
        Serial();
        virtual ~Serial();

        int getFd() const;
        bool open(const std::string &device, int baudrate);
        void close();
        int32_t send(const std::vector<uint8_t> &data);
        int32_t send(const uint8_t *data, uint32_t size);
        std::vector<uint8_t> receive();
        uint32_t receive(uint8_t *data, uint32_t max_len);
};
