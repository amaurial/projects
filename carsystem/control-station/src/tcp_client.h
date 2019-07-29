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
#include <queue>
#include "utils.h"

#define BUFFER_SIZE 1024

#define ST  30 //ms

using namespace std;

class TcpClient : public Client
{
    public:
        TcpClient(log4cpp::Category *logger,
                  TcpServer *server,
                  RadioHandler* radio,
                  int client_sock,
                  struct sockaddr_in client_addr,
                  int id,
                  YAML::Node *config);
        virtual ~TcpClient();
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
        void handleRadio(CSRD message);        
        void processRadioQueue(void *param);
        void sendToClient(string msg);        
        void handleClientMessages(char* msgptr);                
        void shutdown();        
        
         static void* thread_processradio(void *classPtr){
            ((TcpClient*)classPtr)->processRadioQueue(classPtr);
            return nullptr;
        }
};

#endif // TCPCLIENT_H
