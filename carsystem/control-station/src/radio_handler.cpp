#include "radio_handler.hpp"

RadioHandler::RadioHandler(log4cpp::Category *logger)
{
    //ctor    
    this->logger = logger;
    pthread_mutex_init(&m_mutex, NULL);
    pthread_cond_init(&m_condv, NULL);
    pthread_mutex_init(&m_mutex_in, NULL);
    pthread_cond_init(&m_condv_in, NULL);
    pthread_mutex_init(&radio1_mutex, NULL);
    pthread_cond_init(&radio1_cond_mutex, NULL);
    pthread_mutex_init(&radio2_mutex, NULL);
    pthread_cond_init(&radio2_cond_mutex, NULL);
}

RadioHandler::~RadioHandler()
{
    //dtor    
    pthread_mutex_destroy(&m_mutex);
    pthread_cond_destroy(&m_condv);
    pthread_mutex_destroy(&m_mutex_in);
    pthread_cond_destroy(&m_condv_in);
    pthread_mutex_destroy(&radio1_mutex);
    pthread_cond_destroy(&radio1_cond_mutex);
    pthread_mutex_destroy(&radio2_mutex);
    pthread_cond_destroy(&radio2_cond_mutex);
}

void RadioHandler::setConfigurator(YAML::Node* config){
    this->configurator = config;
}

bool RadioHandler::register_consumer(string name, MessageConsumer *consumer){
    logger->debug("Registering consumer with name %s", name.c_str());
    if (mapConsumer.find(name) != mapConsumer.end()){
        logger->debug("Consumer with this name is already registered.");
        return false;
    }
    logger->debug("Consumer registered %s", name.c_str());
    mapConsumer.insert(std::pair<string, MessageConsumer*>(name, consumer));
    return true;
}

bool RadioHandler::unregister_consumer(string name){
    try {
        if (mapConsumer.find(name) != mapConsumer.end()){
            logger->debug("[RadioHandler] Removing consumer with id: %s", name.c_str());
            mapConsumer.erase(name);
        }
        else{
            logger->debug("[RadioHandler] Could not find consumer with id: %s", name.c_str());
        }
    }
    catch(...){
        logger->error("[RadioHandler] Failed to remove a consumer %s", name.c_str());
        return false;
    }
    return true;
}

bool RadioHandler::start(){    	
	
    logger->debug("Initializing bcm2835.");
    if (!bcm2835_init()) 
    {
        logger->error("bcm2835_init() Failed");
        return false;
    }

    if (!startRadio(&radio1, YAML_RADIO1)){
        logger->error("Can't start radio %s. Exiting.", YAML_RADIO1);        
        return false;
    }
    radio1_activated = true;

    if (!startRadio(&radio2, YAML_RADIO2)){
        logger->error("Can't start radio %s. Exiting.", YAML_RADIO2);
        return false;
    }
    radio2_activated = true;

    running = true;    
    
	logger->debug("[RadioHandler] Starting the queue reader thread");
	pthread_create(&queueReader,nullptr,RadioHandler::thread_entry_in_reader,this);

	logger->debug("[RadioHandler] Starting the queue writer thread");
	pthread_create(&queueWriter,nullptr,RadioHandler::thread_entry_out,this);	

    logger->debug("[RadioHandler] Start the radio handler reader");
	pthread_create(&radioReader,nullptr,RadioHandler::thread_entry_in,this);

    return true;
}

bool RadioHandler::stop(){    
    running = false;
    logger->debug("[RadioHandler] Removing the registered consumers");
    mapConsumer.clear();    
    return true;
}

/*
* Get the messages from the radios and put in a queue
*/
void RadioHandler::run_in(void* param){

    logger->debug("[RadioHandler] run_in running");   
    // get the thread sleep time
    long thread_sleep = RADIO_SLEEP;
    if ((*configurator)[YAML_LIMITS]){
        if ((*configurator)[YAML_LIMITS][YAML_RADIO_SLEEP]){
            thread_sleep = (*configurator)[YAML_LIMITS][YAML_RADIO_SLEEP].as<int>();
        }                    
    }

    logger->debug("[RadioHandler] run_in thread sleep configuration is %l micro seconds", thread_sleep);   

    long t = 0;

    while (running){
        checkMessages(&radio1, YAML_RADIO1);
        //checkMessages(&radio2, YAML_RADIO2);    
        usleep(thread_sleep);    
        // TODO: Erase it after test
        if (t > 5000){
            logger->debug("[RadioHandler] run_in still checking radios. Creating broadcast register.");
            CSRD message = CSRD(logger);            
            message.createBroadcastRequestRegister(1);
            put_to_out_queue(message);
            t = 0;
        }
        else
        {
            t++;
        }        
    }
}

/*
* Get the messages from the queue and send through the radios
*/
void RadioHandler::run_out(void* param){
    logger->debug("[RadioHandler] run_out running");   

    // get the thread sleep time
    long thread_sleep = QUEUE_READER_SLEEP;
    if ((*configurator)[YAML_LIMITS]){
        if ((*configurator)[YAML_LIMITS][YAML_RADIO_OUT_QUEUE_THREAD_SLEEP]){
            thread_sleep = (*configurator)[YAML_LIMITS][YAML_RADIO_OUT_QUEUE_THREAD_SLEEP].as<int>();
        }                    
    }

    logger->debug("[RadioHandler] run_out thread sleep configuration is %l micro seconds", thread_sleep);   

    while (running){                
        if (!out_msgs.empty()){
            // send message for all registered consumers
            logger->debug("Radio queue has %d messages.", out_msgs.size());
            CSRD message = out_msgs.front();
            out_msgs.pop();
            if (radio1_activated){                
                send_message(&radio1, YAML_RADIO1, &message);
                radio1.setModeRx();
            }                                                     
            if (radio2_activated){                
                //send_message(&radio2, YAML_RADIO2, &message);
                //radio2.setModeRx();
            }                                                     
        }             
        usleep(thread_sleep);
    }
}

/*
* Consume the radio messages
*/
void RadioHandler::run_queue_reader(void* param){    
    
    logger->debug("[RadioHandler] run_queue_reader running");        

    // get the max queue size
    uint max_queue_size = RADIO_IN_QUEUE_SIZE;
    if ((*configurator)[YAML_LIMITS]){
        if ((*configurator)[YAML_LIMITS][YAML_RADIO_IN_QUEUE_SIZE]){
            max_queue_size = (*configurator)[YAML_LIMITS][YAML_RADIO_IN_QUEUE_SIZE].as<int>();
        }                    
    }

    // get the thread sleep time
    long thread_sleep = QUEUE_READER_SLEEP;
    if ((*configurator)[YAML_LIMITS]){
        if ((*configurator)[YAML_LIMITS][YAML_RADIO_IN_QUEUE_THREAD_SLEEP]){
            thread_sleep = (*configurator)[YAML_LIMITS][YAML_RADIO_IN_QUEUE_THREAD_SLEEP].as<int>();
        }                    
    }

    logger->debug("[RadioHandler] run_queue_reader thread sleep configuration is %l micro seconds", thread_sleep);   

    while (running){
        if (!in_msgs.empty()){
            // send message for all registered consumers
            if (!mapConsumer.empty()){
                CSRD message = in_msgs.front();
                in_msgs.pop();
                std::map<string, MessageConsumer*>::iterator it = mapConsumer.begin();
                while(it != mapConsumer.end())
                {
                    logger->debug("[RadioHandler] Sending message to consumer %s", it->first.c_str());
                    it->second->putMessage(message);
                    it++;
                }                                 
            }
            else{
                // check the amount of message is higher that what is configured and delete the last
                while (in_msgs.size() > max_queue_size){
                    logger->debug("[RadioHandler] Radio in queue limit reached: %d. Removing messages", in_msgs.size());
                    in_msgs.pop();
                }
            }            
        }        
        usleep(thread_sleep);       
    }
}

bool RadioHandler::checkMessages(RH_RF69 *radio, string radioName){	
    //logger->debug("Checking radio %s with timeout %d", radioName.c_str(), READ_TIMEOUT);//radio->getWaitTimeout());
        
    uint8_t len = RH_RF69_MAX_MESSAGE_LEN;
    uint8_t from;
    uint8_t to;
    uint8_t flags;
    int8_t rssi;
    bool got_message = false;  
    memset(buffer, '\0', RH_RF69_MAX_MESSAGE_LEN);

    if (radioName == YAML_RADIO1){
        pthread_mutex_lock(&radio1_mutex);        
        //pthread_cond_wait(&radio1_cond_mutex, &radio1_mutex);
    }
    else if (radioName == YAML_RADIO2){
        pthread_mutex_lock(&radio2_mutex);
        //pthread_cond_wait(&radio2_cond_mutex, &radio2_mutex);
    }
    else{
        logger->debug("Invalid radio name %s", radioName.c_str());    
        return false;
    }
    //logger->debug("[checkMessages] Got lock for radio name %s", radioName.c_str());       

    if (radio->recv(buffer, &len)) {
        if (len >0){
            // Should be a message for us now                                 
            logger->debug("%s received message: [%s]", radioName.c_str(), buffer);
            from = radio->headerFrom();
            to   = radio->headerTo();
            //uint8_t id   = radio->headerId();
            flags= radio->headerFlags();;
            rssi  = radio->lastRssi();                      
            got_message = true;
        }
    }

    if (radioName == YAML_RADIO1){
        //pthread_cond_signal(&radio1_cond_mutex);
        pthread_mutex_unlock(&radio1_mutex);
    }

    if (radioName == YAML_RADIO2){
        //pthread_cond_signal(&radio2_cond_mutex);
        pthread_mutex_unlock(&radio2_mutex);
    }

    //logger->debug("[checkMessages] Unlock for radio name %s", radioName.c_str());    

    if (got_message){    
        CSRD message = CSRD(logger, 0, buffer, len);
        message.setTo(to);
        message.setFrom(from);
        message.setRssi(rssi);
        message.setFlags(flags);
        in_msgs.push(message);  
        message.dumpBuffer(); 
        return true;                         
    }
     
    
    return false;
}

bool RadioHandler::send_message(RH_RF69 *radio, string radioName, CSRD *message){
    
    if (radioName == YAML_RADIO1 and !radio1_activated){
        logger->debug("%s is deactivated. Not sending message.", radioName.c_str());
        return false;
    }

    if (radioName == YAML_RADIO2 and !radio2_activated){
        logger->debug("%s is deactivated. Not sending message.", radioName.c_str());
        return false;
    }    

    //logger->debug("[send_message] Got lock for radio name %s", radioName.c_str());

    memset(buffer_out, '\0', MESSAGE_SIZE);
    uint8_t msize = message->getMessageBuffer(buffer_out);
    if (msize < 1){
        logger->debug("Message is empty. Not sending.");    
        return false;
    }

    printMessage(buffer_out, msize);    

    if (radioName == YAML_RADIO1){
        pthread_mutex_lock(&radio1_mutex);
        //pthread_cond_wait(&radio1_cond_mutex, &radio1_mutex);
    }
    else if (radioName == YAML_RADIO2){
        pthread_mutex_lock(&radio2_mutex);
        //pthread_cond_wait(&radio2_cond_mutex, &radio2_mutex);
    }
    else{
        logger->debug("Invalid radio name %s", radioName.c_str());    
        return false;
    }    
    
    radio->setModeRx();    
    bool message_sent = radio->send(buffer_out, msize);    
    radio->waitPacketSent();    

    if (radioName == YAML_RADIO1){
        //pthread_cond_signal(&radio1_cond_mutex);
        pthread_mutex_unlock(&radio1_mutex);
    }

    if (radioName == YAML_RADIO2){
        //pthread_cond_signal(&radio2_cond_mutex);
        pthread_mutex_unlock(&radio2_mutex);
    }

    if (message_sent){
        logger->debug("Message sent via %s.", radioName.c_str());    
    }
    else{
        logger->debug("Message not sent.");
    }
    return message_sent;
}

void RadioHandler::printMessage(uint8_t *pbuf, int len){
    char temp[3];    
    stringstream ss;

    for (int i = 0; i < len; i++){
        sprintf(temp,"%02X", pbuf[i]);
        ss << temp << " ";
    }

    logger->debug("message: %s",ss.str().c_str());
}

int RadioHandler::put_to_out_queue(char *msg, int size){
    CSRD message = CSRD(logger, 0, (uint8_t *)msg, size);    
    out_msgs.push(message);
    return 0;
}

int RadioHandler::put_to_out_queue(CSRD msg){    
    out_msgs.push(msg);
    return 0;
}

bool RadioHandler::startRadio(RH_RF69 *radio, string radioName){
    uint8_t cs;
    uint8_t irq;
    uint8_t power = 14;
    float frequency;
    uint16_t id;
    uint16_t group;
    bool promiscuos = false;
    int radio_readtimeout = READ_TIMEOUT;

    try{

        logger->debug("Getting configuration for radio %s", radioName.c_str());
        YAML::Node radioConfig = (*configurator)[radioName];
        if (!radioConfig){
            logger->error("Cant find config for radio %s", radioName.c_str());
            return false;
        }
        
        logger->debug("Getting CS pin for radio %s", radioName.c_str());
        if (!radioConfig[YAML_CSPIN]){
            logger->error("Cant find CS pin config for radio %s", radioName.c_str());
            return false;
        }
        cs = radioConfig[YAML_CSPIN].as<int>();

        logger->debug("Getting IRQ pin pin for radio %s", radioName.c_str());
        if (!radioConfig[YAML_IRQPIN]){
            logger->error("Cant find IRQ pin config for radio %s", radioName.c_str());
            return false;
        }
        irq = radioConfig[YAML_IRQPIN].as<int>();

        logger->debug("Getting frequency for radio %s", radioName.c_str());
        if (!radioConfig[YAML_FREQUENCY]){
            logger->error("Cant find frequency config for radio %s", radioName.c_str());
            return false;
        }
        frequency = radioConfig[YAML_FREQUENCY].as<float>();

        logger->debug("Getting power for radio %s", radioName.c_str());
        if (!radioConfig[YAML_POWER]){
            logger->error("Cant find power config for radio %s. Setting to %d", radioName.c_str(), power);        
        }
        else{
            power = radioConfig[YAML_POWER].as<int>();
        }    

        logger->debug("Getting ID for radio %s", radioName.c_str());
        if (!radioConfig[YAML_ID]){
            logger->error("Cant find ID config for radio %s", radioName.c_str());
            return false;
        }
        id = radioConfig[YAML_ID].as<int>();

        logger->debug("Getting GROUP for radio %s", radioName.c_str());
        if (!radioConfig[YAML_GROUP]){
            logger->error("Cant find GROUP config for radio %s", radioName.c_str());
            return false;
        }
        group = radioConfig[YAML_GROUP].as<int>();

        logger->debug("Getting promiscuous for radio %s", radioName.c_str());
        if (!radioConfig[YAML_PROMISCUOS]){
            logger->debug("Cant find promiscuos config for radio %s. Setting to false.", radioName.c_str());        
        }
        else{
            promiscuos = radioConfig[YAML_PROMISCUOS].as<bool>();
        }
        
        logger->debug("Getting read timeout for radio %s", radioName.c_str());
        if (!radioConfig[YAML_READTIMEOUT]){
            logger->debug("Cant find read timeout config for radio %s. Setting to %d ms.", radioName.c_str(), READ_TIMEOUT);        
        }
        else{            
            radio_readtimeout = radioConfig[YAML_READTIMEOUT].as<int>();
        }

        logger->info("Configuring radio: %s with id: %d group: %d frequency: %f CS: %d IRQ: %d",
                        radioName.c_str(), id, group, frequency, cs, irq);
        
        // create the radio instance
        radio = new RH_RF69((uint8_t)cs, (uint8_t)irq);

        logger->debug("Start radio");
        if (!radio->init()) {
            logger->error("Radio: %s init failed. Please verify wiring/module\n", radioName.c_str());
            return false;
        }
        else {        
            logger->debug("Radio: %s seems OK!", radioName.c_str());
        }
        
        logger->debug("Setting configuration.");
        radio->available();
        logger->debug("Setting power to %d.", power);
        radio->setTxPower((uint8_t)power);        
        logger->debug("Setting modulation to GFSK_Rb250Fd250");
        radio->setModemConfig(RH_RF69::GFSK_Rb250Fd250);
        logger->debug("Setting frequency to %d.", frequency);
        radio->setFrequency(frequency);
        logger->debug("Setting timeout to %d.", radio_readtimeout);
        radio->setWaitTimeout((uint16_t)radio_readtimeout);
        // set Network ID (by sync words)
        uint8_t syncwords[2];
        syncwords[0] = 0x10;
        syncwords[1] = 0x10;; //(uint8_t)group;   
        logger->debug("Setting sync words %d%d", syncwords[0], syncwords[1]);        
        radio->setSyncWords(syncwords, sizeof(syncwords));
        radio->setEncryptionKey(nullptr);
        radio->setPromiscuous(promiscuos);  
        radio->setModeRx();
        radio->setHeaderId(id);
        radio->setHeaderFrom(id);        
        logger->debug("Radio activated.");

    }
    catch (const exception &ex){
        cerr << ex.what() << '\n';
        return false;
    }
    return true;
}
