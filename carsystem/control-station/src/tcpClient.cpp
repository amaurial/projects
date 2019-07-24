#include "tcpClient.h"
#include <stdio.h>

tcpClient::tcpClient(log4cpp::Category *logger, tcpServer *server,
                     radioHandler* radio, int client_sock, struct sockaddr_in client_addr,
                     int id, YAML::Node *config)
{
    //ctor

    this->server = server;
    this->radio = radio;
    this->client_sock = client_sock;
    this->client_addr = client_addr;
    this->logger = logger;
    this->id = id;
    this->config = config;    
    logger->debug("Client %d created", id);   

    pthread_mutex_init(&m_mutex_in_cli, NULL);
    pthread_cond_init(&m_condv_in_cli, NULL);
}

tcpClient::~tcpClient()
{
    pthread_mutex_destroy(&m_mutex_in_cli);
    pthread_cond_destroy(&m_condv_in_cli);
}

void tcpClient::start(void *param){
    running = true;
    stringstream ss;
    ss << "Hello";ss << "\n";
    sendToClient(ss.str());    
    run(nullptr);
}

void tcpClient::stop(){
    running = false;	
    usleep(1000*1000);
    close(client_sock);
}

void tcpClient::radioMessage(const CSRD msg){
    
    pthread_mutex_lock(&m_mutex_in_cli);

    //in_msgs.push(frame);

    pthread_cond_signal(&m_condv_in_cli);
    pthread_mutex_unlock(&m_mutex_in_cli);

}

void tcpClient::run(void *param){
    char msg[BUFFER_SIZE];
    int nbytes;

    pthread_t radioin;
    pthread_create(&radioin, nullptr, tcpClient::thread_processradio, this);

    while (running){
        memset(msg,0,BUFFER_SIZE);
        nbytes = recv(client_sock, msg,BUFFER_SIZE, 0);
        if (nbytes <= 0){
            logger->debug("[%d] [tcpClient] Error while receiving data from client %d",id,nbytes);			
            running = false;
        }
        else if (nbytes > 0){
            logger->notice("[%d] [tcpClient] Received from client:%s Bytes:%d",id, msg, nbytes);
            try{
                handleClientMessages(msg);
            }
            catch(const runtime_error &ex){
                logger->debug("[%d][tcpClient] Failed to process the client message\n%s",id,ex.what());
            }
        }        
    }
    logger->info("[%d] [tcpClient] Quiting client connection ip:%s id:%d.",id, ip.c_str(),id);

    usleep(2000*1000); //1sec give some time for any pending thread to finish
    try{        
        pthread_cancel(radioin);
        server->removeClient(this);
    }
    catch(...){
        logger->error("[tcpClient] Failed to stop the tcp client.");
    }

}

void tcpClient::handleClientMessages(char* msgptr){
    // TODO
}

void tcpClient::handleRadioMessages(char* msgptr){

    vector<string> msgs;
    string message (msgptr);
    const char *msgtemp;

    try{
        msgs = split(message,'\n', msgs);

        for (auto const& msg:msgs){
            logger->debug("[%d] [tcpClient] Handle message:%s",id,msg.c_str());

            if (msg.length() == 0){
                continue;
            }
            msgtemp = msg.c_str();

            // TODO            
        }
    }
    catch(const runtime_error &ex){
        logger->debug("[tcpClient] Not runtime error cought. %s",ex.what() );
        cout << "[tcpClient] Not runtime error cought" << ex.what() << endl;
    }
    catch(...){
        logger->debug("[tcpClient] Not runtime error cought");
        cout << "[tcpClient] Not runtime error cought" << endl;
    }
}

void tcpClient::processRadioQueue(void *param){
    
    char buf[100];
    logger->debug("[%d] [tcpClient] Tcp Client radio thread read cbus queue",id);
    while (running){

        pthread_mutex_lock(&m_mutex_in_cli);
        pthread_cond_wait(&m_condv_in_cli, &m_mutex_in_cli);

        if (in_msgs.empty()){
            pthread_mutex_unlock(&m_mutex_in_cli);
        }
        else{

            CSRD message = in_msgs.front();
            in_msgs.pop();
            pthread_mutex_unlock(&m_mutex_in_cli);

            try{
                memset(buf,0,sizeof(buf));                
                logger->debug("[%d] [tcpClient] Tcp Client received cbus message: %s",id, buf);
                handleRadio(buf);
            }
            catch(runtime_error &ex){
                logger->debug("[%d] [tcpClient] Failed to process the can message",id);
            }
            catch(...){
                logger->debug("[%d] [tcpClient] Failed to process the can message",id);
            }
        }
        //usleep(4000);
    }
}

void tcpClient::handleRadio(unsigned char *msg){
    
    string message;
    stringstream ss;    
    // TODO
    
}

void tcpClient::sendToClient(string msg){
    unsigned int nbytes;
    logger->notice("[%d] [tcpClient] Send to client:%s",id, msg.c_str());
    nbytes = write(client_sock,msg.c_str(),msg.length());
    if (nbytes != msg.length()){
        logger->error("[tcpClient] Fail to send message %s to ED",id, msg.c_str());
    }
}
