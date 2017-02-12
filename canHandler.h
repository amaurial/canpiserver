#ifndef CANHANDLER_H
#define CANHANDLER_H

#include <linux/can.h>
#include <net/if.h>
#include <linux/can/raw.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <unistd.h>
#include <queue>
#include <string>
#include <vector>
#include <ctime>
#include <algorithm>
#include <mutex>
#include <condition_variable>
#include "utils.h"
#include "opcodes.h"
#include "gpio.h"
#include "nodeConfigurator.h"
#include "frameCAN.h"
#include "tcpServer.h"

#define CAN_MSG_SIZE 8
#define WAIT_ENUM 200 //ms
#define NN_PB_TIME 6000 //time the button for request node number is pressed is 8 seconds
#define AENUM_PB_TIME 2000 //time the button for auto enum is pressed is 2 seconds
#define BLINK_INTERVAL 300 //time in milliseconds seconds
#define CBUS_KEEP_ALIVE  4000 //ms

using namespace std;
class tcpServer;

class canHandler
{
    public:
        canHandler(log4cpp::Category *logger,int canId);
        virtual ~canHandler();
        int start(const char* interface);
        int put_to_out_queue(char *msg,int size,CLIENT_TYPE ct);
        int put_to_out_queue(int canid,char *msg,int size,CLIENT_TYPE ct);
        int put_to_incoming_queue(int canid,char *msg,int size,CLIENT_TYPE ct);
        void stop();
        void setCanId(int id);
        int getCanId();
        void setTcpServer(tcpServer * tcpserver);
        void setPins(int pb,int gled, int yled);
        void setNodeNumber(int nn);
        int getNodeNumber(){return node_number;};
        void setConfigurator(nodeConfigurator *config);
    protected:
    private:
        int canId;
        int node_number;
        int canInterface;
        int running;
        struct ifreq ifr;
        struct sockaddr_can addr;
        log4cpp::Category *logger;
        pthread_t cbusReader;
        pthread_t queueReader;
        pthread_t queueWriter;
        tcpServer *tcpserver;
        vector<tcpServer*> servers;
        //std::queue<can_frame> in_msgs;
        //std::queue<can_frame> out_msgs;
        std::queue<frameCAN> in_msgs;
        std::queue<frameCAN> out_msgs;
        bool auto_enum_mode = false;
        bool setup_mode = false;
        bool cbus_stopped = false;
        bool pb_pressed = false;
        bool blinking = false;
        int pbpin;
        int glpin;
        int ylpin;
        gpio pb;
        gpio gl;
        gpio yl;
        vector<int> canids;
        //auto enum timers
        long double sysTimeMS_start;
        long double sysTimeMS_end;
        //node number request timer
        long double nnPressTime;
        long double nnReleaseTime;
        long double ledtime;
        pthread_mutex_t m_mutex;
        pthread_cond_t  m_condv;
        pthread_mutex_t m_mutex_in;
        pthread_cond_t  m_condv_in;
        nodeConfigurator *config;

        void send_start_event();
        void send_end_event();
        void run_in(void* param);
        void run_out(void* param);
        void run_queue_reader(void* param);
        void doSelfEnum();
        void finishSelfEnum(int id);
        //void handleCBUSEvents(struct can_frame frame);
        void handleCBUSEvents(frameCAN frame);
        void doPbLogic();
        void restart_module(SCRIPT_ACTIONS action);

        void print_frame(can_frame *frame,string message);

        static void* thread_entry_in(void *classPtr){
            ((canHandler*)classPtr)->run_in(nullptr);
            return nullptr;
        }
        static void* thread_entry_out(void *classPtr){
            ((canHandler*)classPtr)->run_out(nullptr);
            return nullptr;
        }
        static void* thread_entry_in_reader(void *classPtr){
            ((canHandler*)classPtr)->run_queue_reader(nullptr);
            return nullptr;
        }
};

#endif // CANHANDLER_H
