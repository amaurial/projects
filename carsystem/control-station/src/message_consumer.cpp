#include "message_consumer.h"

MessageConsumer::MessageConsumer(log4cpp::Category *logger, YAML::Node *configurator){
    this->logger = logger;
    this->configurator = configurator;
}

MessageConsumer::~MessageConsumer(){

}

/*
void MessageConsumer::setLogger(log4cpp::Category *logger){
    this->logger = logger;
}

void MessageConsumer::setConfigurator(YAML::Node* configurator){
    this->configurator = configurator;
}
*/

bool MessageConsumer::start(){
    return false;
}

bool MessageConsumer::stop(){
    return false;
}

bool MessageConsumer::putMessage(const CSRD message){
    return true;
}
