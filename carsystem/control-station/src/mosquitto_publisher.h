#ifndef MOSQUITTO_PUBLISHER_H
#define MOSQUITTO_PUBLISHER_H

#include <mosquittopp.h>
#include <log4cpp/Category.hh>
#include <yaml-cpp/yaml.h>
#include <string>
#include <unistd.h>
#include "config_tokens.h"

using namespace std;

class MosquittoPublisher:public mosqpp::mosquittopp
{
        public:
            MosquittoPublisher(const char *id, log4cpp::Category *logger, YAML::Node *configurator);
            ~MosquittoPublisher();            
            void on_connect(int rc);
            void on_subscribe(int mid, int qos_count, const int *granted_qos);
            void on_message(const struct mosquitto_message *message);
            bool start();
            bool stop();
            bool publishJson(string jsonString);
            
            //void on_message(const struct mosquitto_message *message);
            //void on_subscribe(int mid, int qos_count, const int *granted_qos);
        private:
            log4cpp::Category *logger;            
            YAML::Node *configurator;
            bool getConfiguration();
            string publish_topic;
            string register_topic;
            string broker_address;
            int broker_port;
            int keepalive = 60; // default is 60 seconds            
            string id;
            bool running = false;
};
#endif