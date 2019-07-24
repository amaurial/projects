#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <log4cpp/Category.hh>
#include <yaml-cpp/yaml.h>
#include <map>
#include "client.h"
#include "radioHandler.hpp"
#include "config.hpp"

class Client;
class radioHandler;

class tcpServer
{
    public:
        tcpServer(log4cpp::Category *logger, int port, radioHandler* radio);
        virtual ~tcpServer();
        bool start();
        void setPort(int port);
        int getPort();
        void stop();
        void removeClient(Client* client);
        void addMessage(const CSRD msg);        
        void postMessageToAllClients(int clientId, CSRD msg, int msize);
        void setConfigurator(YAML::Node* config);
    protected:
    private:
        radioHandler *radio;
        Client *tempClient;        
        bool running;
        int port;
        int socket_desc, client_sock ,read_size;
        struct sockaddr_in server_addr;
        int counter;        
        log4cpp::Category *logger;
        std::map<int, Client*> clients;
        pthread_t serverThread;        
        YAML::Node *config;

        void removeClients();
        void run(void* param);
        void run_client(void* param);

        static void* thread_entry_run(void *classPtr){
            ((tcpServer*)classPtr)->run(nullptr);
            return nullptr;
        }

        static void* thread_entry_run_client(void *classPtr){
            ((tcpServer*)classPtr)->run_client(classPtr);
            return nullptr;
        }
};

#endif // TCPSERVER_H
