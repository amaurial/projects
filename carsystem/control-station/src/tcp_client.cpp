#include "tcp_client.h"
#include <stdio.h>

TcpClient::TcpClient(log4cpp::Category *logger,
                     TcpServer *server,
                     RadioHandler* radio,
                     int client_sock,
                     struct sockaddr_in client_addr,
                     int id,
                     YAML::Node *config)
{
    //ctor

    this->server = server;
    this->radio = radio;
    this->client_sock = client_sock;
    this->client_addr = client_addr;
    this->logger = logger;
    this->id = id;
    this->configurator = config;
    
    YAML::Node node = (*configurator)[YAML_TCP_SERVER];

    logger->debug("[TcpClient] Getting configuration");
    if (!node){
        logger->debug("[TcpClient] Oops. No configuration for tcpserver");
    }
    else{            
        if (node[YAML_OUTPUT_FORMAT]){            
            string output_format = node[YAML_OUTPUT_FORMAT].as<string>();            
            logger->debug("[TcpClient] Found output format to json %s", output_format.c_str());
            if (output_format == "json"){
                logger->debug("[TcpClient] Setting output format to json");
                this->json_output = true;
            }
            else{
                logger->debug("[TcpClient] Setting output format to hexa string");
            }
        }
        else{
            logger->debug("[TcpClient] Missing output format. Setting output format to hexa string");
        }
        
    }
    
    logger->debug("[TcpClient] Client %d created", id);   

    pthread_mutex_init(&m_mutex_in_cli, NULL);
    pthread_cond_init(&m_condv_in_cli, NULL);
}

TcpClient::~TcpClient()
{
    pthread_mutex_destroy(&m_mutex_in_cli);
    pthread_cond_destroy(&m_condv_in_cli);
}

void TcpClient::start(void *param){
    running = true;    
    run(nullptr);
}

void TcpClient::stop(){
    running = false;	
    usleep(1000*1000);
    close(client_sock);
}

void TcpClient::radioMessage(const CSRD msg){
    
    //pthread_mutex_lock(&m_mutex_in_cli);
    in_msgs.push(msg);
    //pthread_cond_signal(&m_condv_in_cli);
    //pthread_mutex_unlock(&m_mutex_in_cli);

}
/*
* This thread handles the incoming messages from tcp clients
*/
void TcpClient::run(void *param){
    char msg[BUFFER_SIZE];
    int nbytes;

    pthread_t radioin;
    pthread_create(&radioin, nullptr, TcpClient::thread_processradio, this);

    while (running){
        memset(msg,0,BUFFER_SIZE);
        nbytes = recv(client_sock, msg,BUFFER_SIZE, 0);
        if (nbytes <= 0){
            logger->debug("[%d] [TcpClient] Error while receiving data from client %d", id,nbytes);			
            running = false;
        }
        else if (nbytes > 0){
            logger->notice("[%d] [TcpClient] Received from client:%s Bytes:%d", id, msg, nbytes);
            try{
                handleClientMessages(msg);
            }
            catch(const runtime_error &ex){
                logger->debug("[%d][TcpClient] Failed to process the client message\n%s", id,ex.what());
            }
        }        
    }
    logger->info("[%d] [TcpClient] Quiting client connection ip:%s id:%d.", id, ip.c_str(),id);

    usleep(2000*1000); //1sec give some time for any pending thread to finish
    try{        
        running = false;
        pthread_cancel(radioin);
        server->removeClient(this);        
    }
    catch(...){
        logger->error("[TcpClient] Failed to stop the tcp client.");
    }
}

void TcpClient::handleClientMessages(char* msgptr){
    // TODO
    // Check if the format is HEXA
    // 01 00 03 E7 00 00 00 00

    vector<string> tokens;
    string token;
    stringstream ss(msgptr);   
    while (getline(ss, token, '\n'))
    {
        tokens.push_back(token);
    }
    
    for (auto token:tokens){        
        CSRD message = CSRD(logger); 
        //if (hexaToCSRD(&message, token, logger) == false){
        if (message.setMessageFromHexaString(0, token) == 0){
            // try json
            if (jsonToCSRD(&message, token, logger)){
                // put in the radio queue
                radio->put_to_out_queue(message);
            }
            else{
                logger->debug("Message is not in json nor in hexa format. Skipping message. [%s]", token);
            }
        }
        else{
            // put in the radio queue
            radio->put_to_out_queue(message);
        }
    }    
}

/*
* This thread handles the incoming messages from radios
*/
void TcpClient::processRadioQueue(void *param){    
    
    logger->debug("[%d] [TcpClient] Tcp Client thread read radio queue", id);
    while (running){

        //pthread_mutex_lock(&m_mutex_in_cli);
        //pthread_cond_wait(&m_condv_in_cli, &m_mutex_in_cli);

        if (in_msgs.empty()){
            //pthread_mutex_unlock(&m_mutex_in_cli);
        }
        else{

            CSRD message = in_msgs.front();
            in_msgs.pop();
            //pthread_mutex_unlock(&m_mutex_in_cli);

            try{                
                handleRadio(message);
            }
            catch(runtime_error &ex){
                logger->debug("[%d] [TcpClient] Failed to process the radio message", id);
                logger->debug("%s", ex.what());
            }
            catch(const exception &ex){
                logger->debug("[%d] [TcpClient] Failed to process the radio message", id);
                logger->debug("%s", ex.what());
            }
        }
        usleep(1000);
    }
}

void TcpClient::handleRadio(CSRD message){   
    try{  
        if (json_output){        
            string json_message = csrdToJson(&message);        
            logger->debug("[%d] [TcpClient] Tcp Client received radio json message: %s", id, json_message.c_str());  
            sendToClient(json_message);
            sendToClient("\n");
        }
        else{
            logger->debug("[%d] [TcpClient] Tcp Client received radio hexa message: %s", id, message.bufferToHexString().c_str());  
            sendToClient(message.bufferToHexString());    
            sendToClient("\n");
        }
    }   
    catch(const exception &ex){
        logger->error("Failed to generate json message. %s", ex.what());               
    }  
}

void TcpClient::sendToClient(string msg){
    unsigned int nbytes;
    logger->notice("[%d] [TcpClient] Send to client:%s", id, msg.c_str());
    nbytes = write(client_sock, msg.c_str(), msg.length());
    if (nbytes != msg.length()){
        logger->error("[TcpClient] Fail to send message %s", id, msg.c_str());
    }
}
