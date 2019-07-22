#include "radio_handler.hpp"

radioHandler::radioHandler(log4cpp::Category *logger)
{
    //ctor    
    this->logger = logger;
    pthread_mutex_init(&m_mutex, NULL);
    pthread_cond_init(&m_condv, NULL);
    pthread_mutex_init(&m_mutex_in, NULL);
    pthread_cond_init(&m_condv_in, NULL);          
}

radioHandler::~radioHandler()
{
    //dtor    
    pthread_mutex_destroy(&m_mutex);
    pthread_cond_destroy(&m_condv);
    pthread_mutex_destroy(&m_mutex_in);
    pthread_cond_destroy(&m_condv_in);
}

void radioHandler::setConfigurator(YAML::Node* config){
    this->configurator = config;
}

bool radioHandler::start(){    	
	
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
    
	logger->debug("[radioHandler] Starting the queue reader thread");
	pthread_create(&queueReader,nullptr,radioHandler::thread_entry_in_reader,this);

	logger->debug("[radioHandler] Starting the queue writer thread");
	pthread_create(&queueWriter,nullptr,radioHandler::thread_entry_out,this);	

    logger->debug("[radioHandler] Start the radio handler reader");
	pthread_create(&radioReader,nullptr,radioHandler::thread_entry_in,this);

    return true;
}

bool radioHandler::stop(){    
    running = false;
    return true;
}

/*
* Get the messages from the radios and put in a queue
*/
void radioHandler::run_in(void* param){

    logger->debug("[radioHandler] run_in running");        
    while (running){
        checkMessages(&radio1, YAML_RADIO1);
        checkMessages(&radio2, YAML_RADIO2);
        //logger->debug("[radioHandler] run_in");
        //usleep(1000000);
    }
}

/*
* Get the messages from the queue and send through the radios
*/
void radioHandler::run_out(void* param){
    logger->debug("[radioHandler] run_out running");        
    while (running){        
        logger->debug("[radioHandler] run_out");        
        usleep(5000000);
    }
}

/*
* Used for the tcp connections
* Not sure what it will do for now
*/
void radioHandler::run_queue_reader(void* param){
    // defined in CSRD
    uint8_t local_buffer [MESSAGE_SIZE];
    uint8_t local_len = 0;

    logger->debug("[radioHandler] run_queue_reader running");        

    while (running){
        if (!in_msgs.empty()){
            // get the message and print
            CSRD message = in_msgs.front();
            in_msgs.pop();
            logger->debug("Extracting message from queue");
            local_len = message.getMessageBuffer(local_buffer);
            printMessage(local_buffer, local_len);
        }
        else{
            logger->debug("[radioHandler] run_queue_reader");
            usleep(5000000);
        } 
        //usleep(1000000);       
    }
}

bool radioHandler::checkMessages(RH_RF69 *radio, string radioName){
	
    //logger->debug("Checking radio %s with timeout %d", radioName.c_str(), READ_TIMEOUT);//radio->getWaitTimeout());                

    //if (radio->waitAvailableTimeout( READ_TIMEOUT )) {        
    if (radio->available()) {
        uint8_t len = RH_RF69_MAX_MESSAGE_LEN;         
        memset(buffer, '\0', RH_RF69_MAX_MESSAGE_LEN);
        if (radio->recv(buffer, &len)) {
            if (len >0){
                // Should be a message for us now                                 
                uint8_t from = radio->headerFrom();
                uint8_t to   = radio->headerTo();
                uint8_t id   = radio->headerId();
                uint8_t flags= radio->headerFlags();;
                int8_t rssi  = radio->lastRssi();          
                radio->setModeTx();
                radio->setModeRx();
                //logger->debug("Radio: %s len: %02d from: %d to: %d ssi: %ddB id: %d",radioName.c_str(), len, from, to, rssi, id);
                CSRD message = CSRD(logger, 0, buffer, len);
                in_msgs.push(message);            
                logger->debug("%s received message: [%s]", radioName.c_str(), buffer);            
                //printMessage(buffer, len); 
            }           
        }         
    }    
    return false;
}

void radioHandler::printMessage(uint8_t *pbuf, int len){
    stringstream ss;

    for (int i = 0; i < len; i++){
        ss << std::setw(2) << std::setfill('0') << std::hex << std::uppercase << pbuf[i];
        ss << " ";
    }

    logger->debug("message: %s",ss.str().c_str());
}

bool radioHandler::startRadio(RH_RF69 *radio, string radioName){
    uint8_t cs;
    uint8_t irq;
    uint8_t power = 14;
    float frequency;
    uint16_t id;
    uint16_t group;
    bool promiscuos = false;
    int radio_readtimeout = READ_TIMEOUT;

    try{        

        if (radio != NULL){
            logger->error("Can't config radio. Radio seems to be configured already");
            //return false;
        }

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
        syncwords[0] = 0x2d;
        syncwords[1] = (uint8_t)group;   
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