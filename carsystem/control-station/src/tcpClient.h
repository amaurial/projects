#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include "client.h"

#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <log4cpp/Category.hh>
#include <map>
#include <time.h>
#include <string>
#include <sstream>
#include <vector>
#include <regex>

#define BUFFER_SIZE 1024

#define ST  30 //ms

using namespace std;

class tcpClient : public Client
{
    public:
        tcpClient(log4cpp::Category *logger,
                  tcpServer *server,
                  radioHandler* radio,
                  int client_sock,
                  struct sockaddr_in client_addr,
                  int id,
                  YAML::Node *config);
        virtual ~tcpClient();
        void start(void *param);
        void stop();
        void radioMessage(const CSRD msg);        
    protected:
    private:
        bool running;        
        std::queue<CSRD> in_msgs;
        pthread_mutex_t m_mutex_in_cli;
        pthread_cond_t  m_condv_in_cli;

        void run(void * param);
        void handleRadio(uint8_t *msg);        
        void processRadioQueue(void *param);
        void sendToClient(string msg);        
        void handleClientMessages(char* msgptr);        
        void handleRadioMessages(char* msgptr);
        void shutdown();        
        
         static void* thread_processradio(void *classPtr){
            ((tcpClient*)classPtr)->processRadioQueue(classPtr);
            return nullptr;
        }
};

#endif // TCPCLIENT_H
