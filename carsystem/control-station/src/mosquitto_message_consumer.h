#ifndef MOSQUITTO_MSG_CONSUMER_H
#define MOSQUITTO_MSG_CONSUMER_H

#include <mosquittopp.h>
#include <pthread.h>
#include <log4cpp/Category.hh>
#include <yaml-cpp/yaml.h>
#include "message_consumer.h"
#include "radio_handler.hpp"
#include "mosquitto_publisher.h"
#include "utils.h"

#define MOSQUITTO_IN_TOPIC "csrd/in"
#define MOSQUITTO_OUT_TOPIC "csrd/out"

class MosquittoMessageConsumer:public MessageConsumer
{
        public:
            MosquittoMessageConsumer(log4cpp::Category *logger, YAML::Node *configurator, RadioHandler* radio);
            ~MosquittoMessageConsumer();            
            
            bool putMessage(const CSRD message);
            bool start();
            bool stop();
            bool isConnected();
            
        private:      
            RadioHandler* radio;
            MosquittoPublisher* publisher = nullptr;
            void run(void* args);      
            bool running;
            string consumer_name = "mosquitto";                    
            int retries = 10;
            int retry_interval = 5;
            bool connected;
            pthread_t mosquittoThread;

            static void* thread_entry_run(void *classPtr){
            ((MosquittoMessageConsumer*)classPtr)->run(nullptr);
            return nullptr;
        }
};
#endif