#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <log4cpp/Category.hh>
#include <yaml-cpp/yaml.h>
#include <map>
#include "client.h"
#include "radio_handler.hpp"
#include "message_consumer.h"

#define MAX_COUNTER_VALUE 1000000

class Client;
class RadioHandler;

class TcpServer: public MessageConsumer
{
    public:
        TcpServer(log4cpp::Category *logger, YAML::Node *configurator, int port, RadioHandler* radio);
        ~TcpServer();
        bool start();
        void setPort(int port);
        int getPort();
        bool stop();
        void removeClient(Client* client);
        bool putMessage(const CSRD msg);                        
    protected:
    private:
        RadioHandler *radio;
        Client *tempClient;        
        bool running;
        int port;
        int socket_desc, client_sock ,read_size;
        struct sockaddr_in server_addr;
        uint counter;     
        string consumer_name = "tcpserver";        
        std::map<int, Client*> clients;
        pthread_t serverThread;                

        void removeClients();
        void run(void* param);
        void run_client(void* param);

        static void* thread_entry_run(void *classPtr){
            ((TcpServer*)classPtr)->run(nullptr);
            return nullptr;
        }

        static void* thread_entry_run_client(void *classPtr){
            ((TcpServer*)classPtr)->run_client(classPtr);
            return nullptr;
        }
};

#endif // TCPSERVER_H
