#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include "Client.h"
#include "edSession.h"
#include "opcodes.h"
#include "msgdata.h"
#include "Turnout.h"
#include "sessionHandler.h"

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

#define ED_KEEP_ALIVE  9000 //ms
#define ST  30 //ms

using namespace std;

class tcpClient : public Client
{
    public:
        tcpClient(log4cpp::Category *logger, tcpServer *server, canHandler* can,
                  int client_sock, struct sockaddr_in client_addr, int id,
                  nodeConfigurator *config, sessionHandler *session_handler);
        virtual ~tcpClient();
        void start(void *param);
        void stop();
        void canMessage(int canid,const char* msg, int dlc);
        void setTurnout(Turnout *turnouts);
    protected:
    private:
        int running;
        edSession* edsession;
        std::map<int,edSession*> sessions; //the loco number is the key
        sessionHandler *session_handler;
        Turnout *turnouts;
        std::queue<can_frame> in_msgs;
        pthread_mutex_t m_mutex_in_cli;
        pthread_cond_t  m_condv_in_cli;

        void run(void * param);
        void handleCBUS(unsigned char* msg);
        void handleLearnEvents(const char* msg);
        void sendKeepAlive(void *param);
        void processCbusQueue(void *param);
        void sendToEd(string msg);
        string generateFunctionsLabel(int loco,char stype,char adtype);
        void handleEDMessages(char* msgptr);
        int handleCreateSession(string message);
        void handleReleaseSession(string message);
        void handleDirection(string message);
        void handleSpeed(string message);
        void handleIdle(string message);
        void handleQueryDirection(string message);
        void handleQuerySpeed(string message);
        void handleSetFunction(string message);
        void handleTurnout(string message);
        void handleTurnoutGeneric(string message);
        void sendFnMessages(edSession* session, int fn, string message);
        bool programmingFn(int fn, int loco,int onoff);
        int getLoco(string msg);

		void releaseAllSessions();
		void releaseActualSession();
		void deleteUnsetSessions();
		void retrieveRemainingSessions();
		void ackEDSessionCreated(edSession *ed, bool sendSpeedMode);
		bool sessions_retrieved;

        void setStartSessionTime();
        void shutdown();

        regex re_speed;
        regex re_session;
        regex re_rel_session;
        regex re_dir;
        regex re_qry_speed;
        regex re_qry_direction;
        regex re_func;
        regex re_turnout;
        regex re_turnout_generic;
        regex re_idle;

        static void* thread_keepalive(void *classPtr){
            ((tcpClient*)classPtr)->sendKeepAlive(classPtr);
            return nullptr;
        }
         static void* thread_processcbus(void *classPtr){
            ((tcpClient*)classPtr)->processCbusQueue(classPtr);
            return nullptr;
        }
};

#endif // TCPCLIENT_H
