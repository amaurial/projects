#include "radio_handler.hpp"

RadioHandler::RadioHandler(log4cpp::Category *logger)
{
    //ctor    
    this->logger = logger;
    pthread_mutex_init(&m_mutex, NULL);
    pthread_cond_init(&m_condv, NULL);
    pthread_mutex_init(&m_mutex_in, NULL);
    pthread_cond_init(&m_condv_in, NULL);          
}

RadioHandler::~RadioHandler()
{
    //dtor    
    pthread_mutex_destroy(&m_mutex);
    pthread_cond_destroy(&m_condv);
    pthread_mutex_destroy(&m_mutex_in);
    pthread_cond_destroy(&m_condv_in);
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

    if (!startRadio(&radio2, YAML_RADIO2)){
        logger->error("Can't start radio %s. Exiting.", YAML_RADIO2);
        return false;
    }

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

    while (running){
        checkMessages(&radio1, YAML_RADIO1);
        checkMessages(&radio2, YAML_RADIO2);    
        usleep(thread_sleep);    
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

    while (running){        
        //logger->debug("[RadioHandler] run_out");        
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
            max_queue_size = (*configurator)[YAML_LIMITS][YAML_RADIO_IN_QUEUE_THREAD_SLEEP].as<int>();
        }                    
    }

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
        // else{            
        //     usleep(thread_sleep);
        // }  
        usleep(thread_sleep);       
    }
}

bool RadioHandler::checkMessages(RH_RF69 *radio, string radioName){	
    //logger->debug("Checking radio %s with timeout %d", radioName.c_str(), READ_TIMEOUT);//radio->getWaitTimeout());
    
    if (radio->available()) {
        uint8_t len = RH_RF69_MAX_MESSAGE_LEN;         
        memset(buffer, '\0', RH_RF69_MAX_MESSAGE_LEN);
        if (radio->recv(buffer, &len)) {
            if (len >0){
                // Should be a message for us now                                 
                uint8_t from = radio->headerFrom();
                uint8_t to   = radio->headerTo();
                //uint8_t id   = radio->headerId();
                uint8_t flags= radio->headerFlags();;
                int8_t rssi  = radio->lastRssi();          
                radio->setModeTx();
                radio->setModeRx();
                //logger->debug("Radio: %s len: %02d from: %d to: %d ssi: %ddB id: %d",radioName.c_str(), len, from, to, rssi, id);
                CSRD message = CSRD(logger, 0, buffer, len);
                message.setTo(to);
                message.setFrom(from);
                message.setRssi(rssi);
                message.setFlags(flags);
                in_msgs.push(message);  
                message.dumpBuffer();          
                logger->debug("%s received message: [%s]", radioName.c_str(), buffer);                            
            }           
        }         
    }    
    return false;
}

void RadioHandler::printMessage(uint8_t *pbuf, int len){
    stringstream ss;

    for (int i = 0; i < len; i++){
        ss << std::setw(2) << std::setfill('0') << std::hex << std::uppercase << pbuf[i];
        ss << " ";
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
            logger->debug("Radio: %s seen OK!", radioName.c_str());
        }

        radio->available();        
        radio->setTxPower((uint8_t)power);        
        radio->setModemConfig(RH_RF69::GFSK_Rb250Fd250);
        radio->setFrequency(frequency);
        radio->setWaitTimeout((uint16_t)radio_readtimeout);
        // set Network ID (by sync words)
        uint8_t syncwords[2];
        syncwords[0] = 0x10;
        syncwords[1] = 0x10;; //(uint8_t)group;   
        cout << "sync words:" << syncwords[0] << ";" << syncwords[1] << endl;     
        radio->setSyncWords(syncwords, sizeof(syncwords));
        radio->setPromiscuous(promiscuos);  
        radio->setModeRx();
    }
    catch (const exception &ex){
        cerr << ex.what() << '\n';
        return false;
    }
    return true;
}
