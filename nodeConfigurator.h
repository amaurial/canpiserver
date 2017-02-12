#ifndef NODECONFIGURATOR_H
#define NODECONFIGURATOR_H

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <vector>
#include <log4cpp/Category.hh>
#include <map>
#include <fstream>
#include <algorithm>
#include "utils.h"
#include "opcodes.h"


/**
1 byte
apmode bit 1, enable can grid bit 2, log level bit 3,4

2 bytes
tcp port

2 bytes
grid tcp port

1 byte
wifi channel

8 bytes
ssid

8 bytes
ssid password

12 bytes
router ssid

12 bytes
router password

8 bytes
service name

11 bytes
turnout file name
**/
#define P1_SIZE 1    //apmode bit 0, enable can grid bit 1, log level bit 2,3, ap no password bit 4, no log file bit 5
#define P2_SIZE 2    //tcp port
#define P3_SIZE 2    //grid tcp port
#define P4_SIZE 1    //wifi channel
#define P5_SIZE 8    //ssid
#define P6_SIZE 8    //ssid password
#define P7_SIZE 12   //router ssid
#define P8_SIZE 12   //router ssid password
#define P9_SIZE 8    //service name
#define P10_SIZE 11  //turnout file name
#define P11_SIZE 4   //momentary FNs
#define P12_SIZE 2   //start event id

#define NVS_SIZE         P1_SIZE + P2_SIZE + P3_SIZE + P4_SIZE + P5_SIZE + P6_SIZE + P7_SIZE + P8_SIZE + P9_SIZE + P10_SIZE + P11_SIZE + P12_SIZE
//parameter index in the buffer
#define PARAM1 0 //apmode bit 1, enable can grid bit 2, log level bit 3,4
#define P_TCP_PORT           PARAM1 + P1_SIZE
#define P_GRID_TCP_PORT      P_TCP_PORT + P2_SIZE
#define P_WIFI_CHANNEL       P_GRID_TCP_PORT + P3_SIZE
#define P_SSID               P_WIFI_CHANNEL + P4_SIZE
#define P_PASSWD             P_SSID + P5_SIZE
#define P_ROUTER_SSID        P_PASSWD + P6_SIZE
#define P_ROUTER_PASSWD      P_ROUTER_SSID + P7_SIZE
#define P_SERVICE_NAME       P_ROUTER_PASSWD + P8_SIZE
#define P_TURNOUT_FILE       P_SERVICE_NAME + P9_SIZE
#define P_MOMENTARY_FNS      P_TURNOUT_FILE + P10_SIZE
#define P_START_EVENT        P_MOMENTARY_FNS + P11_SIZE

#define FN_SIZE 29

using namespace std;

class nodeConfigurator
{
    public:
        nodeConfigurator(string file,log4cpp::Category *logger);
        virtual ~nodeConfigurator();

        bool loadConfig();
        string getNodeName();

        int getTcpPort();
        bool setTcpPort(int port);

        int getcanGridPort();
        bool setCanGridPort(int port);

        int getCanID(bool fresh=true);
        bool setCanID(int canid);

        int getNodeNumber(bool fresh=true);
        bool setNodeNumber(int nn);

        bool getAPMode();
        bool setAPMode(bool apmode);

        bool getAPNoPassword();
        bool setAPNoPassword(bool mode);

        bool getCreateLogfile();
        bool setCreateLogfile(bool mode);

        bool isCanGridEnabled();
        bool enableCanGrid(bool grid);

        bool setSSID(string val);
        string getSSID();

        bool setPassword(string val);
        string getPassword();

        bool setRouterSSID(string val);
        string getRouterSSID();

        bool setRouterPassword(string val);
        string getRouterPassword();

        bool setLogLevel(string val);
        string getLogLevel();

        bool setLogFile(string val);
        string getLogFile();

        bool setLogAppend(bool val);
        bool getLogAppend();

        bool setLogConsole(bool val);
        bool getLogConsole();

        bool setTurnoutFile(string val);
        string getTurnoutFile(bool fresh=true);

        bool setCanDevice(string val);
        string getCanDevice();

        bool setServiceName(string val);
        string getServiceName();

        int getApChannel();
        bool setApChannel(int channel);

        string getConfigFile();
        void setConfigFile(string val);

        int getPB();
        bool setPB(int val);

        int getGreenLed();
        bool setGreenLed(int val);

        int getYellowLed();
        bool setYellowLed(int val);

        string getMomentaryFn(bool fresh=true);
        bool setMomentaryFn(string val);

        string getStringConfig(string key);
        int getIntConfig(string key);

        int getStartEventID();
        bool setStartEventID(int val);

        int getNodeMode();
        bool setNodeMode(int val);

        int getOrphanTimeout();
        bool setOrphanTimeout(int val);

        /*ASON code for remote shutdown*/
        int getShutdownCode();

        void printMemoryNVs();
        byte getNumberOfNVs(){return NVS_SIZE;};
        byte getNV(int idx);
        byte setNV(int idx,byte val);
        void setNodeParams(byte p1,byte p2, byte p3,byte p4,byte p5, byte p6, byte p7, byte p8, byte p9, byte p10);
        byte getNodeParameter(byte idx);
        /*
        * these functions are designed to be used dynamically,
        * it means that a manual change in the file won't take imediate effect
        * a loadConfig is required
        */
        bool setNewPair(string key,string value,bool quoted); //creates a new one or update an existing one
        string getPairValue(string key);
        bool existConfigEntry(string key);

        void restart_module();
    protected:
    private:
        string nvToMomentary();
        log4cpp::Category *logger = nullptr;
        string configFile;
        char NV[NVS_SIZE];
        char NODEPARAMS[NODE_PARAMS_SIZE];
        int nvs_set; //used to count how many nvs were written before saving the data do the file
        vector<string> not_quoted_config;

        void setNotQuotedConfigKeys();
        bool saveConfig();
        void loadParamsToMemory();
        void loadParam1();
        void loadParamsInt2Bytes(int value,unsigned int idx);
        void loadParamsString(string value,unsigned int idx,unsigned int maxsize);
        int nvToInt(int index,int slen);
        string nvToString(int index,int slen);
        bool nvToApMode();
        bool nvToCanGrid();
        int nvToLogLevel();
        bool nvToApNoPassword();
        bool nvToCreateLogfile();
        void momentaryFnsToNVs();
        vector<string> & split(const string &s, char delim, vector<string> &elems);
        map<string,string> config;
        string removeChar(string val,char c);
        string cleanString(string val);
        pair <string,string> getpair(string val);


};

#endif // NODECONFIGURATOR_H
