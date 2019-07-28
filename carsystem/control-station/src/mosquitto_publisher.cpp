#include "mosquitto_publisher.h"

MosquittoPublisher::MosquittoPublisher(log4cpp::Category *logger, const char *id): mosquittopp(id){

}

MosquittoPublisher::~MosquittoPublisher(){

}

void MosquittoPublisher::setConfigurator(YAML::Node* config){
    this->config = config;
}

void MosquittoPublisher::on_connect(int rc){

}

void MosquittoPublisher::start(const char *host, int port){
    /* Connect immediately. This could also be done by calling
         * MosquittoPublisher->connect(). */
        connect(host, port, keepalive);
}