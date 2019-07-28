#include "tcp_server.h"
#include "tcp_client.h"

TcpServer::TcpServer(log4cpp::Category *logger, int port, RadioHandler* radio):MessageConsumer(logger)
{
    //ctor
    //this->setLogger(logger);
    this->setPort(port);
    this->radio = radio;
    this->counter = 0;
}

TcpServer::~TcpServer()
{
    //dtor
}

void TcpServer::setPort(int port){
    this->port = port;
}
int TcpServer::getPort(){
    return port;
}

bool TcpServer::stop(){
    removeClients();    
    running = false;
    return running;
}

// Add message to the client
bool TcpServer::putMessage(const CSRD msg){    

    if (!clients.empty()){
        std::map<int, Client*>::iterator it = clients.begin();
        while(it != clients.end())
        {
            it->second->radioMessage(msg);
            it++;
        }
    }
    return true;
}

bool TcpServer::start(){
    //Create socket    
    logger->info("[TcpServer] Starting tcp server on port %d", this->port);        
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        logger->error("[TcpServer] Could not create socket");
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
        logger->error("[TcpServer] Tcp server bind failed for port %d", port);
        return false;
    }

    logger->debug("[TcpServer] Start tcp listener");
    listen(socket_desc, 5);
    running = true;

    logger->debug("[TcpServer] Start tcp thread");

    pthread_create(&serverThread, nullptr, TcpServer::thread_entry_run, this);

    return running;
}

void TcpServer::run(void* param){
    logger->info("[TcpServer] Waiting for connections on port %d", port);

    struct sockaddr_in client_addr;
    vector<pthread_t> threads;
    char *s = NULL;
    socklen_t len = sizeof(client_addr);
    logger->info("[TcpServer] Tcp server running");

    // we can now register for messages
    this->radio->register_consumer(consumer_name, this);

    while (running){
        try{
            client_sock = accept(socket_desc, (struct sockaddr *)&client_addr, &len);

            if (client_sock < 0)
            {
                logger->debug("[TcpServer] Cannot accept connection");
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
                logger->info("[TcpServer] Creating client for ip:%s id:%d",s, counter);
                Client *client;                                
                client = new TcpClient(logger, this, radio, client_sock, client_addr, counter, config);
                client->setIp(s);
                free(s);
                tempClient = client; // to be used in run_client
                logger->debug("[TcpServer] Creating client thread %d", counter);
                pthread_t clientThread;
                pthread_create(&clientThread, nullptr, TcpServer::thread_entry_run_client, this);
                clients.insert(std::pair<int, Client*>(counter, client));
                threads.push_back(clientThread);
                counter++;
                if (counter > MAX_COUNTER_VALUE) counter = 0;
            }
        }
        catch(...){
            logger->error("[TcpServer] TCP server failed while running.");
        }
    }

    for (vector<pthread_t>::iterator itt = threads.begin();itt!=threads.end();itt++){
        pthread_cancel(*itt);
    }
    threads.clear();
}

void TcpServer::run_client(void* param){
    logger->info("[TcpServer] Starting client thread %d", counter);
    tempClient->start(nullptr);
}

void TcpServer::removeClients(){
    try{

        logger->info("[TcpServer] Stopping client connections");
        std::map<int,Client*>::iterator it = clients.begin();
        while(it != clients.end())
        {
            logger->info("[TcpServer] Stop client %d", it->second->getId());
            it->second->stop();
            it++;
        }
        //check if it dealocattes the pointers
        clients.clear();
    }
    catch(...){
        logger->error("[TcpServer] Failed to remove all clients");
    }
}

void TcpServer::removeClient(Client *client){
    try {
        if (clients.find(client->getId()) != clients.end()){
            logger->debug("[TcpServer] Removing tcp client with id: %d", client->getId());
            clients.erase(clients.find(client->getId()));
        }
        else{
            logger->debug("[TcpServer] Could not remove tcp client with id: %d", client->getId());
        }
        delete client;
    }
    catch(...){
        logger->error("[TcpServer] Failed to remove a client");
    }
}