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
2 bytes
grid tcp port

2 bytes 
start event
**/
#define P1_SIZE 2    //grid tcp port
#define P2_SIZE 2   //start event id

#define NVS_SIZE         P1_SIZE + P2_SIZE
//parameter index in the buffer
#define P_GRID_TCP_PORT      P1_SIZE
#define P_START_EVENT        P_GRID_TCP_PORT + P2_SIZE

using namespace std;

class nodeConfigurator
{
    public:
        nodeConfigurator(string file,log4cpp::Category *logger);
        virtual ~nodeConfigurator();

        bool loadConfig();
        string getNodeName();

        int getcanGridPort();
        bool setCanGridPort(int port);

        int getCanID(bool fresh=true);
        bool setCanID(int canid);

        int getNodeNumber(bool fresh=true);
        bool setNodeNumber(int nn);

        bool getCreateLogfile();
        bool setCreateLogfile(bool mode);

        bool setLogLevel(string val);
        string getLogLevel();

        bool setLogFile(string val);
        string getLogFile();

        bool setLogAppend(bool val);
        bool getLogAppend();

        bool setLogConsole(bool val);
        bool getLogConsole();

        bool setCanDevice(string val);
        string getCanDevice();

        string getConfigFile();
        void setConfigFile(string val);

        int getPB();
        bool setPB(int val);

        int getGreenLed();
        bool setGreenLed(int val);

        int getYellowLed();
        bool setYellowLed(int val);

        string getStringConfig(string key);
        int getIntConfig(string key);

        int getStartEventID();
        bool setStartEventID(int val);

        int getNodeMode();
        bool setNodeMode(int val);

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
        log4cpp::Category *logger = nullptr;
        string configFile;
        char NV[NVS_SIZE];
        char NODEPARAMS[NODE_PARAMS_SIZE];
        int nvs_set; //used to count how many nvs were written before saving the data do the file
        vector<string> not_quoted_config;

        void setNotQuotedConfigKeys();
        bool saveConfig();
        void loadParamsToMemory();
        void loadParamsInt2Bytes(int value,unsigned int idx);
        void loadParamsString(string value,unsigned int idx,unsigned int maxsize);
        int nvToInt(int index,int slen);
        string nvToString(int index,int slen);
        int nvToLogLevel();
        bool nvToCreateLogfile();
        vector<string> & split(const string &s, char delim, vector<string> &elems);
        map<string,string> config;
        string removeChar(string val,char c);
        string cleanString(string val);
        pair <string,string> getpair(string val);


};

#endif // NODECONFIGURATOR_H
