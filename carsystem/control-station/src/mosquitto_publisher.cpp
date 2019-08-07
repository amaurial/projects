#include "mosquitto_publisher.h"

MosquittoPublisher::MosquittoPublisher(const char *id, log4cpp::Category *logger, YAML::Node *configurator): mosquittopp(id){
    this->configurator = configurator;
    this->logger = logger;
}

MosquittoPublisher::~MosquittoPublisher(){

}

void MosquittoPublisher::on_connect(int rc){
    
}

void MosquittoPublisher::start(const char *host, int port){
    /* Connect immediately. This could also be done by calling
         * MosquittoPublisher->connect(). */
        connect(host, port, keepalive);
}