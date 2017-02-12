#ifndef EDSESSION_H
#define EDSESSION_H

#include <string>
#include <log4cpp/Category.hh>
#include <time.h>

#include "utils.h"
#include "nodeConfigurator.h"

using namespace std;

class edSession
{
    public:
        edSession(log4cpp::Category *logger,int sessionuid);
        virtual ~edSession();

        void setClientId(int client_id);
        int getClientId();

        void setClientIP(int client_ip);
        int getClientIP();

        int getSessionUid();

        void setLoco(int loco);
        int getLoco();

        void setSession(byte session);
        byte getSession();

        void setSpeed(byte val);
        byte getSpeed();

        void setDirection(byte direction);
        byte getDirection();

        void setAddressType(byte val);
        byte getAddressType();

        void setCbusTime(struct timespec val);
        struct timespec getCbusTime();

        void setEDTime(struct timespec val);
        struct timespec getEDTime();

        void setFnType(int fn ,FN_TYPE state);
        FN_TYPE getFnType(int fn);

        void setFnState(int fn ,FN_STATE state);
        FN_STATE getFnState(int fn);

        void setEdName(string edname);
        string getEdName();

        void setEdHW(string hw);
        string getEdHW();

        void setLocoName(string loconame);
        string getLocoName();

        void setNodeConfigurator(nodeConfigurator *config);

        byte getDccByte(int fn);

        void configChanged();
        void getMomentaryFNs();
        void getMomentaryFNs(int loco);
        string getMomentary();

        bool isOrphan();
        void setOrphan(bool orphan);
		
		bool isSessionSet();

        void setSessionType(SESSION_TYPE stype);
        SESSION_TYPE getSessionType();

        char getCharSessionType();

    protected:
    private:
        int loco;
        char sessionid;
        long last_keep_alive;
        bool orphan;
		bool session_set;
        int client_id;  //id from the tcp client. set by the session manager
        int sessionuid; //unique number for the session set by the session manager
        long client_ip;
        byte direction;
        byte ad_type;
        struct timespec cbus_time;
        struct timespec ed_time;
        FN_STATE fns[FN_SIZE];
        FN_TYPE fnstype[FN_SIZE];
        byte speed;
        string session_name;
        string hname;
        string loconame;
        log4cpp::Category *logger;
        nodeConfigurator *config;
        SESSION_TYPE sessionType;

        byte setBit(byte val, int pos);
        byte clearBit(byte val, int pos);
        void setMomentaryFNs(string val);

        vector<string> & split(const string &s, char delim, vector<string> &elems);
};

#endif // EDSESSION_H
