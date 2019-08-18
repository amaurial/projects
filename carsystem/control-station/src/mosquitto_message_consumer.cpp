#include <cstdio>
#include <cstring>
#include "mosquitto_message_consumer.h"

MosquittoMessageConsumer::MosquittoMessageConsumer(log4cpp::Category *logger,
                                                   YAML::Node *configurator,
                                                   RadioHandler* radio):MessageConsumer(logger, configurator)
{        
        this->radio = radio;  
};

MosquittoMessageConsumer::~MosquittoMessageConsumer()
{
    if (publisher != nullptr){
        delete publisher;
    }
}

bool MosquittoMessageConsumer::putMessage(const CSRD message){

    try{ 
        string jsonMessage = csrdToJson(&message);
        if (connected){
            publisher->publishJson(jsonMessage);
        }
    }
    catch(const exception &ex){
        logger->error("Failed to generate json message. %s", ex.what());  
        return false;      
    } 
    return true;
}

bool MosquittoMessageConsumer::isConnected(){
    return connected;
}

bool MosquittoMessageConsumer::start(){    
    logger->debug("[MosquittoMessageConsumer] Starting mosquitto thread.");

    // create the mosquitto connection
    string id = "csrd";
    YAML::Node mosquittoConfig = (*configurator)[YAML_MOSQUITTO];
    if (! mosquittoConfig[YAML_ID]){
        logger->debug("Can't find mosquitto id.Using csrd.");        
    }
    else{
        id = mosquittoConfig[YAML_ID].as<string>();
    }

    if (! mosquittoConfig[YAML_RETRIES]){
        logger->debug("Can't find mosquitto connection retries. Using %d", retries);    
    }
    else{
        retries = mosquittoConfig[YAML_RETRIES].as<int>();
    }

    if (! mosquittoConfig[YAML_RETRY_INTERVAL]){
        logger->debug("Can't find mosquitto connection retry interval. Using %d", retry_interval);    
    }
    else{
        retry_interval = mosquittoConfig[YAML_RETRY_INTERVAL].as<int>();
    }

    logger->debug("[MosquittoMessageConsumer] Starting mosquitto publisher.");
    publisher = new MosquittoPublisher(id.c_str(), logger, configurator);

    running = true;
    pthread_create(&mosquittoThread, nullptr, MosquittoMessageConsumer::thread_entry_run, this);
    return running;
}

bool MosquittoMessageConsumer::stop(){
    running = false;
    logger->debug("[MosquittoMessageConsumer] Stopping the publisher");
    publisher->stop();   
    radio->unregister_consumer(consumer_name);     
    usleep(1000*1000);
    return true;
}

void MosquittoMessageConsumer::run(void* args){
    int rc;
    int retry = 0;
    radio->register_consumer(consumer_name, this);
    connected = publisher->start();    

    while (running){
        while (!connected && retry < retries && running){
            logger->debug("[MosquittoMessageConsumer] Connect failed. Retrying %d from %d", retry, retries);
            usleep(retry_interval * 1000 * 1000); //seconds
            connected = publisher->start();
            retry++;
        }

        if (!connected){
            logger->debug("[MosquittoMessageConsumer] Mosquitto connection failed. Finishing.");
            running = false;            
        }
        
        rc = publisher->loop();
		if(rc){
            logger->debug("[MosquittoMessageConsumer] Reconnecting.");
			publisher->reconnect();
		}
        //usleep(1000);
    }
    logger->debug("[MosquittoMessageConsumer] Stopping mosquitto consumer.");
}
