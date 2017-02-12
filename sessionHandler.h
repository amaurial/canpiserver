#ifndef SESSIONHANDLER_H
#define SESSIONHANDLER_H

#include <arpa/inet.h> //inet_addr
#include <log4cpp/Category.hh>
#include <map>
#include <time.h>
#include <string>
#include <sstream>
#include <vector>
#include <utility>

#include "canHandler.h"
#include "edSession.h"
#include "nodeConfigurator.h"
#include "opcodes.h"
#include "utils.h"

class sessionHandler
{
    public:
        sessionHandler(log4cpp::Category *logger,nodeConfigurator *config, canHandler *can);
        virtual ~sessionHandler();
        unsigned int retrieveAllEDSession(int client_id, string edname, long client_ip, vector<edSession*> *edsessions);
    	edSession* createEDSession(int client_id, string edname, long client_ip);
        bool deleteEDSession(int sessionuid);
        bool deleteAllEDSessions(int client_id);
        void start();
        void stop();
    protected:
    private:
        std::vector< edSession*> sessions; //each edsession has a reference for the client
        log4cpp::Category *logger;
        nodeConfigurator *config;
        canHandler *can;
        int sessionids;
        int running;
		int timeout_orphan;
        pthread_t sessionHandlerThread;

        void run(void *param);
        void sendKeepAliveForOrphanSessions();
		void sendCbusMessage(byte b0, byte b1);
		void sendCbusMessage(byte b0, byte b1, byte b2);

        pthread_mutex_t m_mutex_in;
        pthread_cond_t  m_condv_in;

        static void* run_thread_entry(void *classPtr){
            ((sessionHandler*)classPtr)->run(classPtr);
            return nullptr;
        }

};

#endif // SESSIONHANDLER_H
