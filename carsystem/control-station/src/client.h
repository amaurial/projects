#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <log4cpp/Category.hh>
#include <yaml-cpp/yaml.h>
#include "config.hpp"
#include "tcpServer.h"
#include "radioHandler.hpp"
#include "csrd.h"


using namespace std;

class tcpServer;
class radioHandler;
class CSRD;


class Client
{
    public:
        Client();
        virtual ~Client();
        virtual void start(void *param)=0;
        virtual void stop()=0;
        virtual void radioMessage(const CSRD msg)=0;


        Client& setIp(char *ip);
        string getIp();

        int getId();
        Client& setId(int id);
        Client& setLogger(log4cpp::Category *logger);
        Client& setRadioHandler(radioHandler *radio);
        Client& setClientSocket(int client_socket);
        Client& setSockAddr(struct sockaddr_in client_addr);
        Client& setServer(tcpServer *server);
        Client& setConfigurator(YAML::Node *config);


    protected:
        int id;
        string ip;
        log4cpp::Category *logger;
        tcpServer *server;
        radioHandler *radio;
        int client_sock;
        struct sockaddr_in client_addr;        
        vector<string> & split(const string &s, char delim, vector<string> &elems);
        YAML::Node *config;

        void sendRadioMessage(int nbytes,byte b0, byte b1, byte b2, byte b3, byte b4, byte b5, byte b6, byte b7);
        void sendRadioMessage(byte b0);
        void sendRadioMessage(byte b0, byte b1);
        void sendRadioMessage(byte b0, byte b1, byte b2);
        void sendRadioMessage(byte b0, byte b1, byte b2, byte b3);
        void sendRadioMessage(byte b0, byte b1, byte b2, byte b3, byte b4);
        void sendRadioMessage(byte b0, byte b1, byte b2, byte b3, byte b4, byte b5);
        void sendRadioMessage(byte b0, byte b1, byte b2, byte b3, byte b4, byte b5, byte b6);
        void sendRadioMessage(byte b0, byte b1, byte b2, byte b3, byte b4, byte b5, byte b6, byte b7);
        void sendRadioMessage(CSRD message);
};

#endif // CLIENT_H
