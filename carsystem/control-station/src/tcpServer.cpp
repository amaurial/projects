#include "tcpServer.h"
#include "tcpClient.h"

tcpServer::tcpServer(log4cpp::Category *logger, int port, radioHandler* radio)
{
    //ctor
    this->logger = logger;
    this->setPort(port);
    this->radio = radio;
    counter = 0;
}

tcpServer::~tcpServer()
{
    //dtor
}

void tcpServer::setConfigurator(YAML::Node *config){
    this->config = config;
}

void tcpServer::setPort(int port){
    this->port = port;
}
int tcpServer::getPort(){
    return port;
}

void tcpServer::stop(){
    removeClients();
    running = false;
}

// Add message to the client
void tcpServer::addMessage(const CSRD msg){    

    if (!clients.empty()){
        std::map<int, Client*>::iterator it = clients.begin();
        while(it != clients.end())
        {
            it->second->radioMessage(msg);
            it++;
        }
    }
}

bool tcpServer::start(){
    //Create socket
    logger->info("[tcpServer] Starting tcp server on port %d", port);
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        logger->error("[tcpServer] Could not create socket");
    }

    //Prepare the sockaddr_in structure
    int yes = 1;
    setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons( port );

    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server_addr , sizeof(server_addr)) < 0)
    {
        //print the error message
        logger->error("[tcpServer] Tcp server bind failed for port %d", port);
        return false;
    }

    logger->debug("[tcpServer] Start tcp listener");
    listen(socket_desc, 5);
    running = true;

    logger->debug("[tcpServer] Start tcp thread");

    pthread_create(&serverThread, nullptr, tcpServer::thread_entry_run, this);

    return true;
}

void tcpServer::run(void* param){
    logger->info("[tcpServer] Waiting for connections on port %d", port);

    struct sockaddr_in client_addr;
    vector<pthread_t> threads;
    char *s = NULL;
    socklen_t len = sizeof(client_addr);
    logger->info("[tcpServer] Tcp server running");

    while (running){
        try{
            client_sock = accept(socket_desc, (struct sockaddr *)&client_addr, &len);

            if (client_sock < 0)
            {
                logger->debug("[tcpServer] Cannot accept connection");
            }
            else
            {
                switch(client_addr.sin_family) {
                    case AF_INET: {
                        //struct sockaddr_in *addr_in = (struct sockaddr_in *)res;
                        s = (char*)malloc(INET_ADDRSTRLEN);
                        inet_ntop(AF_INET, &(client_addr.sin_addr), s, INET_ADDRSTRLEN);
                        break;
                    }
                    case AF_INET6: {
                        struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *)&client_addr;
                        s = (char*)malloc(INET6_ADDRSTRLEN);
                        inet_ntop(AF_INET6, &(addr_in6->sin6_addr), s, INET6_ADDRSTRLEN);
                        break;
                    }
                    default:
                        break;
                }
                logger->info("[tcpServer] Creating client for ip:%s id:%d",s, counter);
                Client *client;                                
                client = new tcpClient(logger, this, radio, client_sock, client_addr, counter, config);
                client->setIp(s);
                free(s);
                tempClient = client;
                logger->debug("[tcpServer] Creating client thread %d", counter);
                pthread_t clientThread;
                pthread_create(&clientThread, nullptr, tcpServer::thread_entry_run_client, this);
                clients.insert(std::pair<int,Client*>(counter, client));
                threads.push_back(clientThread);
                counter++;
            }
        }
        catch(...){
            logger->error("[tcpServer] TCP server failed while running.");
        }
    }

    for (vector<pthread_t>::iterator itt = threads.begin();itt!=threads.end();itt++){
        pthread_cancel(*itt);
    }
    threads.clear();
}

void tcpServer::run_client(void* param){
    logger->info("[tcpServer] Starting client thread %d", counter);
    tempClient->start(nullptr);
}

void tcpServer::removeClients(){
    try{

        logger->info("[tcpServer] Stopping client connections");
        std::map<int,Client*>::iterator it = clients.begin();
        while(it != clients.end())
        {
            logger->info("[tcpServer] Stop client %d", it->second->getId());
            it->second->stop();
            it++;
        }
        //check if it dealocattes the pointers
        clients.clear();
    }
    catch(...){
        logger->error("[tcpServer] Failed to remove all clients");
    }
}

void tcpServer::removeClient(Client *client){
    try {
        if (clients.find(client->getId()) != clients.end()){
        logger->debug("[tcpServer] Removing tcp client with id: %d", client->getId());
        clients.erase(client->getId());
        }
        else{
            logger->debug("[tcpServer] Could not remove tcp client with id: %d", client->getId());
        }
        delete client;
    }
    catch(...){
        logger->error("[tcpServer] Failed to remove a client");
    }
}

void tcpServer::postMessageToAllClients(int clientId, CSRD msg, int msize){
    //transverse the clients and send the message to all except the clientId    
    std::map<int, Client*>::iterator it = clients.begin();
    
    while(it != clients.end())
    {
        if (it->second->getId() != clientId){
            logger->info("[tcpServer] Sending msg from Client %d to Client %d", clientId, it->second->getId());
            it->second->radioMessage(msg);
        }
        it++;
    }
}

