#include "message_consumer.h"

MessageConsumer::MessageConsumer(log4cpp::Category *logger){
    setLogger(logger);
}

MessageConsumer::~MessageConsumer(){

}

void MessageConsumer::setLogger(log4cpp::Category *logger){
    this->logger = logger;
}

void MessageConsumer::setConfigurator(YAML::Node* configurator){
    this->configurator = configurator;
}
/*
bool MessageConsumer::start(){
    return true;
}

bool MessageConsumer::stop(){
    return true;
}
*/
bool MessageConsumer::putMessage(const CSRD message){
    return true;
}
