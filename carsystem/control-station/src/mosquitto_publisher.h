#ifndef MOSQUITTO_PUBLISHER_H
#define MOSQUITTO_PUBLISHER_H

#include <mosquittopp.h>
#include <log4cpp/Category.hh>
#include <yaml-cpp/yaml.h>

class MosquittoPublisher:public mosqpp::mosquittopp
{
        public:
            MosquittoPublisher(log4cpp::Category *logger, const char *id, const char *host, int port);
            ~MosquittoPublisher();
            void setConfigurator(YAML::Node* config);
            void on_connect(int rc);
            void start(const char *host, int port);
            
            //void on_message(const struct mosquitto_message *message);
            //void on_subscribe(int mid, int qos_count, const int *granted_qos);
        private:
            log4cpp::Category *logger;            
            YAML::Node *config;
            int keepalive = 60; // default is 60 seconds
};
#endif