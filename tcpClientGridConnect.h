#ifndef TCPCLIENTGRID_H
#define TCPCLIENTGRID_H

#include "Client.h"
#include "canHandler.h"
#include "opcodes.h"
#include "msgdata.h"
#include "utils.h"
#include "nodeConfigurator.h"

#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <log4cpp/Category.hh>
#include <map>
#include <time.h>
#include <string>
#include <sstream>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <pthread.h>


#define BUFFER_SIZE 1024
/*
The GridConnect protocol encodes messages as an ASCII string of up to 24 characters of the form:
:ShhhhNd0d1d2d3d4d5d6d7;
The S indicates a standard CAN frame
:XhhhhhhhhNd0d1d2d3d4d5d6d7;
The X indicates an extended CAN frame hhhh is the two byte header N or R indicates a normal
or remote frame, in position 6 or 10 d0 - d7 are the (up to) 8 data bytes
*/

using namespace std;

class tcpClientGridConnect:public Client
{
    public:
        tcpClientGridConnect(log4cpp::Category *logger,
                             tcpServer *server, canHandler* can, int client_sock,
                             struct sockaddr_in client_addr, int id,nodeConfigurator *config);
        virtual ~tcpClientGridConnect();
        void start(void *param);
        void stop();
        void canMessage(int canid,const char* msg,int dlc);
    protected:
    private:
        int running;
        pthread_t queueReader;
        void run(void * param);
        void handleClientGridMessage(string msg);
        vector<byte> getBytes(string hex_chars,vector<byte> *bytes);
        std::queue<string> in_grid_msgs;
        pthread_mutex_t m_mutex_in;
        pthread_cond_t  m_condv_in;
        void run_in_grid_msgs(void* param);
        static void* thread_entry_grid_in(void *classPtr){
            ((tcpClientGridConnect*)classPtr)->run_in_grid_msgs(nullptr);
            return nullptr;
        }
		int msg_received;
		int msg_processed;
};

#endif // TCPCLIENT_H
