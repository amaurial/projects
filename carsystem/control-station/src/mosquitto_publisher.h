#ifndef MOSQUITTO_PUBLISHER_H
#define MOSQUITTO_PUBLISHER_H

#include <mosquittopp.h>
#include <log4cpp/Category.hh>
#include <yaml-cpp/yaml.h>
#include <string>

using namespace std;

class MosquittoPublisher:public mosqpp::mosquittopp
{
        public:
            MosquittoPublisher(const char *id, log4cpp::Category *logger, YAML::Node *configurator);
            ~MosquittoPublisher();            
            void on_connect(int rc);
            void on_subscribe(int mid, int qos_count, const int *granted_qos);
            void on_message(const struct mosquitto_message *message);
            void start();
            
            //void on_message(const struct mosquitto_message *message);
            //void on_subscribe(int mid, int qos_count, const int *granted_qos);
        private:
            log4cpp::Category *logger;            
            YAML::Node *configurator;
            string publish_topic;
            string register_topic;
            string broker_address;
            int keepalive = 60; // default is 60 seconds
};
#endif