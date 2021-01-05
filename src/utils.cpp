/*
 * utils.cpp
 *
 *  Created on: Jan 7, 2015
 *      Author: andreas
 */

#include "utils.h"

#include <Poco/NumberFormatter.h>

#include <iostream>
#include <sstream>

using namespace std;
using namespace Poco;

//------------------------------------------------------------------------------
void Utils::replace(std::string &s, const std::string &search, const std::string &replace)
{
    for(size_t pos = 0; ; pos += replace.length())
    {
        pos = s.find(search, pos);
        if(pos == string::npos)
            break;

        s.erase(pos, search.length());
        s.insert(pos, replace);
    }
}

//------------------------------------------------------------------------------
std::string Utils::hexDump(const std::deque<uint8_t> &data)
{
    int i=0;
    stringstream ss;
    for(deque<uint8_t>::const_iterator it = data.begin();
        it != data.end();
        it++, i++)
    {
        if((i % 16) == 0)
        {
            ss << endl;
        }

        ss << NumberFormatter::formatHex(*it, 2, false) << " ";
    }
    ss << endl;
    return ss.str();
}

//------------------------------------------------------------------------------
std::string Utils::toHexString(const std::vector<uint8_t> &data)
{
    stringstream ss;

    for(vector<uint8_t>::const_iterator it = data.begin(); it != data.end(); it++)
    {
        ss << NumberFormatter::formatHex(*it, 2, false);
    }

    return ss.str();
}
