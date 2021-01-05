/*
 * utils.h
 *
 *  Created on: Jan 7, 2015
 *      Author: andreas
 */

#ifndef INCLUDE_UTILS_H_
#define INCLUDE_UTILS_H_

#include <stdint.h>
#include <string>
#include <deque>
#include <vector>

class Utils
{
    public:
        static void replace(std::string &s, const std::string &search, const std::string &replace);

        static std::string hexDump(const std::deque<uint8_t> &data);

        static std::string toHexString(const std::vector<uint8_t> &data);
};

#endif /* INCLUDE_UTILS_H_ */
