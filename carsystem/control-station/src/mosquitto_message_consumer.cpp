#include <cstdio>
#include <cstring>

#include "mosquitto_message_consumer.h"

MosquittoMessageConsumer::MosquittoMessageConsumer(log4cpp::Category *logger):MessageConsumer()
{
        int keepalive = 60;
        this->logger = logger;
};

MosquittoMessageConsumer::~MosquittoMessageConsumer()
{

}

void MosquittoMessageConsumer::setConfigurator(YAML::Node *config){
    this->config = config;
}

void MosquittoMessageConsumer::on_connect(int rc)
{
        logger->debug("Connected with code %d.\n", rc);
        if(rc == 0){
                /* Only attempt to subscribe on a successful connect. */
                subscribe(NULL, "temperature/celsius");
        }
}

void MosquittoMessageConsumer::on_message(const struct mosquitto_message *message)
{
        double temp_celsius, temp_farenheit;
        char buf[51];

        if(!strcmp(message->topic, "temperature/celsius")){
                memset(buf, 0, 51*sizeof(char));
                /* Copy N-1 bytes to ensure always 0 terminated. */
                memcpy(buf, message->payload, 50*sizeof(char));
                temp_celsius = atof(buf);
                temp_farenheit = temp_celsius*9.0/5.0 + 32.0;
                snprintf(buf, 50, "%f", temp_farenheit);
                publish(NULL, "temperature/farenheit", strlen(buf), buf);
        }
}
/*
void MosquittoMessageConsumer::on_subscribe(int mid, int qos_count, const int *granted_qos)
{
        logger->debug("Subscription succeeded.\n");
}*/
