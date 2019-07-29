#include "client.h"

Client::Client()
{
    //ctor
}

Client::~Client()
{
    //dtor
}

Client& Client::setIp(char *ip){
    this->ip = string(ip);
    return *this;
}

Client& Client::setConfigurator(YAML::Node *config){
    this->config = config;
    return *this;
}

string Client::getIp(){
    return ip;
}

int Client::getId(){
    return id;
}

Client& Client::setId(int id){
    this->id = id;
    return *this;
}

Client& Client::setLogger(log4cpp::Category *logger){
    this->logger = logger;
    return *this;
}

Client& Client::setRadioHandler(RadioHandler *radio){
    this->radio = radio;
    return *this;
}

Client& Client::setClientSocket(int client_socket){
    this->client_sock = client_socket;
    return *this;
}

Client& Client::setSockAddr(struct sockaddr_in client_addr){
    this->client_addr = client_addr;
    return *this;
}

Client& Client::setServer(TcpServer *server){
    this->server = server;
    return *this;
}

vector<string> & Client::split(const string &s, char delim, vector<string> &elems)
{
    stringstream ss(s+' ');
    string item;
    while(getline(ss, item, delim))
    {
        elems.push_back(item);
    }
    return elems;
}

void Client::sendRadioMessage(int nbytes, byte b0, byte b1, byte b2, byte b3, byte b4, byte b5, byte b6, byte b7){
    char msg[MESSAGE_SIZE];
    msg[0] = b0;
    msg[1] = b1;
    msg[2] = b2;
    msg[3] = b3;
    msg[4] = b4;
    msg[5] = b5;
    msg[6] = b6;
    msg[7] = b7;
    logger->debug("[%d] [Client] Sending message to radio",id);
    int n=nbytes;
    if (nbytes>MESSAGE_SIZE) n = MESSAGE_SIZE;
    radio->put_to_out_queue(msg,n);
}

void Client::sendRadioMessage(byte b0){
    sendRadioMessage(1,b0,0,0,0,0,0,0,0);
}
void Client::sendRadioMessage(byte b0,byte b1){
    sendRadioMessage(2,b0,b1,0,0,0,0,0,0);
}
void Client::sendRadioMessage(byte b0,byte b1, byte b2){
    sendRadioMessage(3,b0,b1,b2,0,0,0,0,0);
}
void Client::sendRadioMessage(byte b0,byte b1, byte b2, byte b3){
    sendRadioMessage(4,b0,b1,b2,b3,0,0,0,0);
}
void Client::sendRadioMessage(byte b0,byte b1, byte b2, byte b3, byte b4){
    sendRadioMessage(5,b0,b1,b2,b3,b4,0,0,0);
}
void Client::sendRadioMessage(byte b0,byte b1, byte b2, byte b3, byte b4, byte b5){
    sendRadioMessage(6,b0,b1,b2,b3,b4,b5,0,0);
}
void Client::sendRadioMessage(byte b0,byte b1, byte b2, byte b3, byte b4, byte b5, byte b6){
    sendRadioMessage(7,b0,b1,b2,b3,b4,b5,b6,0);
}
void Client::sendRadioMessage(byte b0,byte b1, byte b2, byte b3, byte b4, byte b5, byte b6, byte b7){
    sendRadioMessage(8,b0,b1,b2,b3,b4,b5,b6,b7);
}
void Client::sendRadioMessage(CSRD message){
    uint8_t msg[MESSAGE_SIZE];
    uint8_t len = MESSAGE_SIZE;

    len = message.getMessageBuffer(msg);
    sendRadioMessage(len, msg[0], msg[1], msg[2], msg[3], msg[4], msg[5], msg[6], msg[7]);
}