/*
 * tunnel.h
 *
 *  Created on: 29.12.2020
 *      Author: DI Andreas Auer
 */
#pragma once

#include <Poco/Util/ServerApplication.h>
#include <Poco/Logger.h>

#include <string>

class Tunnel : public Poco::Util::ServerApplication
{
    private:
        static const std::string DEVICE;

    protected:
        Poco::Logger *logger_;
        bool help_;

        std::string interface_;
        int tun_fd_;

        bool terminate_;

    public:
        Tunnel();
        virtual ~Tunnel();

        void terminate();

    protected:
        virtual void initialize(Poco::Util::Application &app);
        virtual void uninitialize();
        virtual void reinitialize(Poco::Util::Application &app);
        virtual void defineOptions(Poco::Util::OptionSet &options);
        virtual void handleOption(const std::string &name, const std::string &value);
        virtual int main(const std::vector<std::string> &args);

    private:
        int open(const std::string &name, int flags);

};
