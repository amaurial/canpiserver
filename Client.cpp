#include "Client.h"

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

Client& Client::setNodeConfigurator(nodeConfigurator *config){
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

Client& Client::setCanHandler(canHandler *can){
    this->can = can;
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

Client& Client::setServer(tcpServer *server){
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

void Client::sendCbusMessage(int nbytes, byte b0, byte b1, byte b2, byte b3, byte b4, byte b5, byte b6, byte b7){
    char msg[CAN_MSG_SIZE];
    msg[0] = b0;
    msg[1] = b1;
    msg[2] = b2;
    msg[3] = b3;
    msg[4] = b4;
    msg[5] = b5;
    msg[6] = b6;
    msg[7] = b7;
    logger->debug("[%d] [Client] Sending message to CBUS",id);
    int n=nbytes;
    if (nbytes>CAN_MSG_SIZE) n = CAN_MSG_SIZE;
    can->put_to_out_queue(msg,n,clientType);
}

void Client::sendCbusMessage(byte b0){
    sendCbusMessage(1,b0,0,0,0,0,0,0,0);
}
void Client::sendCbusMessage(byte b0,byte b1){
    sendCbusMessage(2,b0,b1,0,0,0,0,0,0);
}
void Client::sendCbusMessage(byte b0,byte b1, byte b2){
    sendCbusMessage(3,b0,b1,b2,0,0,0,0,0);
}
void Client::sendCbusMessage(byte b0,byte b1, byte b2, byte b3){
    sendCbusMessage(4,b0,b1,b2,b3,0,0,0,0);
}
void Client::sendCbusMessage(byte b0,byte b1, byte b2, byte b3, byte b4){
    sendCbusMessage(5,b0,b1,b2,b3,b4,0,0,0);
}
void Client::sendCbusMessage(byte b0,byte b1, byte b2, byte b3, byte b4, byte b5){
    sendCbusMessage(6,b0,b1,b2,b3,b4,b5,0,0);
}
void Client::sendCbusMessage(byte b0,byte b1, byte b2, byte b3, byte b4, byte b5, byte b6){
    sendCbusMessage(7,b0,b1,b2,b3,b4,b5,b6,0);
}
void Client::sendCbusMessage(byte b0,byte b1, byte b2, byte b3, byte b4, byte b5, byte b6, byte b7){
    sendCbusMessage(8,b0,b1,b2,b3,b4,b5,b6,b7);
}
