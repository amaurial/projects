#ifndef MOSQUITTO_MSG_CONSUMER_H
#define MOSQUITTO_MSG_CONSUMER_H

#include <mosquittopp.h>
#include <log4cpp/Category.hh>
#include <yaml-cpp/yaml.h>
#include "message_consumer.h"

class MosquittoMessageConsumer:public MessageConsumer
{
        public:
            MosquittoMessageConsumer(log4cpp::Category *logger);
            ~MosquittoMessageConsumer();
            void setConfigurator(YAML::Node* config);
            void on_connect(int rc);
            int putMessage(CSRD message);
            //void on_message(const struct mosquitto_message *message);
            //void on_subscribe(int mid, int qos_count, const int *granted_qos);
        private:
            log4cpp::Category *logger;            
            YAML::Node *config;
};
#endif