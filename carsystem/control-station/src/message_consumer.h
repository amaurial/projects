#ifndef MESSAGE_CONSUMER_H
#define MESSAGE_CONSUMER_H

#include <string.h>
#include <queue>
#include <unistd.h>
#include <exception>

#include <log4cpp/Category.hh>
#include <yaml-cpp/yaml.h>
#include "csrd.h"
#include "config.hpp"

using namespace std;

class MessageConsumer
{
    public:
        MessageConsumer(log4cpp::Category *logger);
        virtual ~MessageConsumer();
        void setLogger(log4cpp::Category *logger);
        void setConfigurator(YAML::Node* configurator);        
        //virtual bool start();
        //virtual bool stop();
        virtual bool putMessage(const CSRD message);
    protected:
        log4cpp::Category *logger;
        YAML::Node* configurator;
};

#endif