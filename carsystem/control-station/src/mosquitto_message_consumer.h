#ifndef MOSQUITTO_MSG_CONSUMER_H
#define MOSQUITTO_MSG_CONSUMER_H

#include <mosquittopp.h>
#include <log4cpp/Category.hh>
#include <yaml-cpp/yaml.h>
#include "message_consumer.h"

#define MOSQUITTO_IN_TOPIC "csrd/in"
#define MOSQUITTO_OUT_TOPIC "csrd/out"

class MosquittoMessageConsumer:public MessageConsumer
{
        public:
            MosquittoMessageConsumer(log4cpp::Category *logger);
            ~MosquittoMessageConsumer();
            
            void on_connect(int rc);
            bool putMessage(const CSRD message);
            //void on_message(const struct mosquitto_message *message);
            //void on_subscribe(int mid, int qos_count, const int *granted_qos);
        private:            
};
#endif