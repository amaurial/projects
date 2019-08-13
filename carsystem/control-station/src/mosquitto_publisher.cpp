#include "mosquitto_publisher.h"

MosquittoPublisher::MosquittoPublisher(const char *id, log4cpp::Category *logger, YAML::Node *configurator): mosquittopp(id){
    this->configurator = configurator;
    this->logger = logger;    
}

MosquittoPublisher::~MosquittoPublisher(){
}

void MosquittoPublisher::on_connect(int rc){
    logger->debug("[MosquittoPublisher] Connected with code %d.\n", rc);
	if(rc == 0){
		/* Only attempt to subscribe on a successful connect. */
		subscribe(NULL, register_topic.c_str());
	}
}

bool MosquittoPublisher::start(){
    if (!getConfiguration()){
        logger->debug("[MosquittoPublisher] Can't start mosquitto. Configuration failed.");
        return false;
    }
    running = true;
    mosqpp::lib_init();

    logger->debug("[MosquittoPublisher] Attempting connection to host [%s] port [%d] ", broker_address.c_str(), broker_port);

    int connected = connect(broker_address.c_str(), broker_port, keepalive);
    
    if (connected != 0){
        logger->debug("[MosquittoPublisher] Connection to mosquito broker failed. Reason: %s", mosqpp::strerror(connected));
        mosqpp::lib_cleanup();
        return false;
    }   
    return true;    
}

bool MosquittoPublisher::stop(){
    running= false;
    loop_stop();
    mosqpp::lib_cleanup();
    return true;
}

bool MosquittoPublisher::publishJson(string jsonString){
    int ret = publish(NULL, publish_topic.c_str(), jsonString.length(), jsonString.c_str());
    
    if (ret != MOSQ_ERR_SUCCESS){
        logger->debug("[MosquittoPublisher] Failed to send mosquito message. Reason %s", mosqpp::strerror(ret));
        return false;
    }
    return true;    
}

void MosquittoPublisher::on_subscribe(int mid, int qos_count, const int *granted_qos)
{
	logger->debug("[MosquittoPublisher] Subscription succeeded.");
}

void MosquittoPublisher::on_message(const struct mosquitto_message *message){
    logger->debug("[MosquittoPublisher] Got a mosquitto message: %s", message->payload);
}

bool MosquittoPublisher::getConfiguration(){
    if (! (*configurator)[YAML_MOSQUITTO] ){
        logger->debug("[MosquittoPublisher] Can't find mosquitto configuration.");
        return false;
    }

    YAML::Node mosquittoConfig = (*configurator)[YAML_MOSQUITTO];
    
    if (! mosquittoConfig[YAML_BROKER_ADDRESS]){
        logger->debug("[MosquittoPublisher] Can't find mosquitto broker address. ");
        return false;
    }    
    broker_address = mosquittoConfig[YAML_BROKER_ADDRESS].as<string>();
    logger->debug("[MosquittoPublisher] %s: [%s]", YAML_BROKER_ADDRESS, broker_address.c_str() );

    if (! mosquittoConfig[YAML_PORT]){
        logger->debug("[MosquittoPublisher] Can't find mosquitto broker port.");
        return false;
    }    
    broker_port = mosquittoConfig[YAML_PORT].as<int>();
    logger->debug("[MosquittoPublisher] %s: [%d]", YAML_PORT, broker_port);
    
    if (! mosquittoConfig[YAML_PUBLISH_TOPIC]){
        logger->debug("[MosquittoPublisher] Can't find mosquitto topic to publish to.");
        return false;
    }    
    publish_topic = mosquittoConfig[YAML_PUBLISH_TOPIC].as<string>();
    logger->debug("[MosquittoPublisher] %s: [%s]", YAML_PUBLISH_TOPIC, publish_topic.c_str() );

    if (! mosquittoConfig[YAML_REGISTER_TO_TOPIC]){
        logger->debug("[MosquittoPublisher] Can't find mosquitto topic to register to.");
        return false;
    }    
    register_topic = mosquittoConfig[YAML_REGISTER_TO_TOPIC].as<string>();
    logger->debug("[MosquittoPublisher] %s: [%s]", YAML_REGISTER_TO_TOPIC, register_topic.c_str() );
        
    if (! mosquittoConfig[YAML_KEEP_ALIVE]){
        logger->debug("[MosquittoPublisher] Can't find mosquitto keep alive. Using %d", keepalive);    
    }
    else{
        keepalive = mosquittoConfig[YAML_KEEP_ALIVE].as<int>();
    }   
    
    return true;

}