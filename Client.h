#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <log4cpp/Category.hh>
#include "utils.h"
#include "tcpServer.h"
#include "canHandler.h"
#include "nodeConfigurator.h"

using namespace std;

class tcpServer;
class canHandler;


class Client
{
    public:
        Client();
        virtual ~Client();
        virtual void start(void *param)=0;
        virtual void stop()=0;
        virtual void canMessage(int canid,const char* msg,int dlc)=0;


        Client& setIp(char *ip);
        string getIp();

        int getId();
        Client& setId(int id);

        Client& setLogger(log4cpp::Category *logger);
        Client& setCanHandler(canHandler *can);
        Client& setClientSocket(int client_socket);
        Client& setSockAddr(struct sockaddr_in client_addr);
        Client& setServer(tcpServer *server);
        Client& setNodeConfigurator(nodeConfigurator *config);


    protected:
        int id;
        string ip;
        log4cpp::Category *logger;
        tcpServer *server;
        canHandler *can;
        int client_sock;
        struct sockaddr_in client_addr;
        CLIENT_TYPE clientType;
        vector<string> & split(const string &s, char delim, vector<string> &elems);
        nodeConfigurator *config;

        void sendCbusMessage(int nbytes,byte b0, byte b1, byte b2, byte b3, byte b4, byte b5, byte b6, byte b7);
        void sendCbusMessage(byte b0);
        void sendCbusMessage(byte b0, byte b1);
        void sendCbusMessage(byte b0, byte b1, byte b2);
        void sendCbusMessage(byte b0, byte b1, byte b2, byte b3);
        void sendCbusMessage(byte b0, byte b1, byte b2, byte b3, byte b4);
        void sendCbusMessage(byte b0, byte b1, byte b2, byte b3, byte b4, byte b5);
        void sendCbusMessage(byte b0, byte b1, byte b2, byte b3, byte b4, byte b5, byte b6);
        void sendCbusMessage(byte b0, byte b1, byte b2, byte b3, byte b4, byte b5, byte b6, byte b7);
};

#endif // CLIENT_H
