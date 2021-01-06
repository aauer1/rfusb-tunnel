/*
 * frame.h
 *
 *  Created on: 05.01.2021
 *      Author: DI Andreas Auer
 */
#pragma once

#include <vector>
#include <sstream>

#include <stdint.h>

class Frame
{
    public:
        enum Command
        {
            CMD_INVALID = 0,
            CMD_GET_VERSION,
            CMD_SEND,
            CMD_RECEIVE,
            CMD_RF_FAILURE,
            CMD_SET_ADDRESS,
            CMD_RESET,
            CMD_END
        };

        enum Flags
        {
            FLAG_NONE = 0x0,
            FLAG_ACK  = 0x1,
            FLAG_NAK  = 0x2
        };

    protected:
        Command  command;
        Flags    flags;
        uint16_t length;
        uint8_t  crc;
        std::vector<uint8_t> data;

    public:
        Frame(Command cmd) :
            command(cmd),
            flags(FLAG_NONE),
            length(0),
            crc(0)
        {
        }

        virtual ~Frame()
        {
        }

        void setCommand(Command cmd)
        {
            command = cmd;
        }

        Command getCommand() const
        {
            return command;
        }

        void setFlags(Flags flag)
        {
            flags = flag;
        }

        Flags getFlags() const
        {
            return flags;
        }

        uint32_t getLength() const
        {
            return length;
        }

        uint8_t getChecksum() const
        {
            return crc;
        }

        void setData(const uint8_t *buffer, uint32_t len)
        {
            data.assign(buffer, buffer + len);
            length = data.size();
        }

        std::vector<uint8_t> getData() const
        {
            return data;
        }

        void serialize(std::vector<uint8_t> &buffer)
        {
            buffer.push_back(uint8_t(command));
            buffer.push_back(uint8_t(flags));
            buffer.push_back(length & 0xFF);
            buffer.push_back((length >> 8) & 0xFF);
            buffer.push_back(0);
            buffer.insert(buffer.end(), data.begin(), data.end());

            uint8_t crc = 0;
            for(std::vector<uint8_t>::const_iterator it = buffer.begin(); it != buffer.end(); it++)
            {
                crc ^= *it;
            }

            buffer[4] = crc;
        }

        std::string toString() const
        {
            std::stringstream ss;
            ss << "Command: " << command << ", Flags: " << flags << ", Length: " << length;
            return ss.str();
        }

        static Frame *deserialize(const uint8_t *data, uint32_t size)
        {
            if(size < 4)
            {
                return nullptr;
            }

            uint32_t len = data[2] | (data[3] << 8);
            if(size < (len + 5))
            {
                return nullptr;
            }

            Frame *frame = new Frame(Command(data[0]));
            frame->flags = Flags(data[1]);
            frame->length = len;
            frame->crc = data[4];
            frame->data.assign(data + 5, data + len + 5);

            return frame;
        }
};
