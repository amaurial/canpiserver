#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <unistd.h>
#include <signal.h>
#include <string>
#include <sys/stat.h>

//logger
#include "log4cpp/Portability.hh"
#ifdef LOG4CPP_HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <iostream>
#include "log4cpp/Category.hh"
#include "log4cpp/Appender.hh"
#include "log4cpp/FileAppender.hh"
#include "log4cpp/RollingFileAppender.hh"
#include "log4cpp/OstreamAppender.hh"
#ifdef LOG4CPP_HAVE_SYSLOG
#include "log4cpp/SyslogAppender.hh"
#endif
#include "log4cpp/Layout.hh"
#include "log4cpp/BasicLayout.hh"
#include "log4cpp/PatternLayout.hh"
#include "log4cpp/SimpleLayout.hh"
#include "log4cpp/Priority.hh"
#include "log4cpp/NDC.hh"

//project classes
#include "utils.h"
#include "canHandler.h"
#include "Turnout.h"
#include "nodeConfigurator.h"
#include "sessionHandler.h"

using namespace std;
//using namespace libconfig;
int running = 1;

void sigterm(int signo)
{
   running = 0;
}

inline bool file_exists (const std::string& name) {
  struct stat buffer;
  return (stat (name.c_str(), &buffer) == 0);
}

int main()
{
    signal(SIGTERM, sigterm);
    signal(SIGHUP, sigterm);
    signal(SIGINT, sigterm);

    //****************
    //default config
    //***************
    log4cpp::Priority::PriorityLevel loglevel = log4cpp::Priority::DEBUG;
    string logfile = "canpi.log";
    string configfile = "canpi.cfg";
    string turnoutfile = "turnout.txt";
    int port = 4444;
    string candevice = "can0";
    bool append = false;
    bool start_grid_server = false;
    int gridport = 5555;
    int canid = 100;
    int pb_pin=4;
    int gled_pin=18;
    int yled_pin=27;
    int node_number=4321;
    log4cpp::Category& logger = log4cpp::Category::getRoot();
    nodeConfigurator *config = new nodeConfigurator(configfile,&logger);

    //get configuration
    string debugLevel = config->getLogLevel();

    if (!debugLevel.empty()){
        if (debugLevel.compare(TAG_INFO)== 0){
           loglevel = log4cpp::Priority::INFO;
        }
        if (debugLevel.compare(TAG_WARN)== 0){
           loglevel = log4cpp::Priority::WARN;
        }
    }

    logfile = config->getLogFile();
    candevice = config->getCanDevice();
    port = config->getTcpPort();
    canid = config->getCanID();
    gridport = config->getcanGridPort();
    append = config->getLogAppend();
    start_grid_server = config->isCanGridEnabled();
    turnoutfile = config->getTurnoutFile();
    pb_pin = config->getPB();
    gled_pin = config->getGreenLed();
    yled_pin = config->getYellowLed();
    node_number = config->getNodeNumber();

    //config the logger
    logger.setPriority(loglevel);

    if (config->getLogConsole()){
        log4cpp::PatternLayout * layout1 = new log4cpp::PatternLayout();
        layout1->setConversionPattern("%d [%p] %m%n");

        log4cpp::Appender *appender1 = new log4cpp::OstreamAppender("console", &std::cout);
        appender1->setLayout(new log4cpp::BasicLayout());
        appender1->setLayout(layout1);
        logger.addAppender(appender1);
    }


    if (config->getCreateLogfile()){

        log4cpp::PatternLayout * layout2 = new log4cpp::PatternLayout();
        layout2->setConversionPattern("%d [%p] %m%n");

        //log4cpp::Appender *appender2 = new log4cpp::FileAppender("default", logfile, append);
        log4cpp::Appender *appender2 = new log4cpp::RollingFileAppender("default", logfile,5*1024*1024,10, append);//5M
        appender2->setLayout(new log4cpp::BasicLayout());
        appender2->setLayout(layout2);
        logger.addAppender(appender2);
    }
    logger.info("Logger initated");

    config->printMemoryNVs();

    //start the CAN
    canHandler can = canHandler(&logger,canid);
    //set the configurator
    can.setConfigurator(config);
    //set gpio pins
    can.setPins(pb_pin,gled_pin,yled_pin);
    can.setNodeNumber(node_number);

    //start the CAN threads
    if (can.start(candevice.c_str()) == -1 ){
        logger.error("Failed to start can Handler.");
        return 1;
    };

    //load the turnout file
    Turnout turnouts=Turnout(&logger);
    if (file_exists(turnoutfile)){
        turnouts.load(turnoutfile);
    }

    //start the session handler
    sessionHandler session_handler = sessionHandler(&logger, config, &can);
	session_handler.start();
    //start the tcp server
    tcpServer tcpserver = tcpServer(&logger, port, &can, &session_handler, CLIENT_TYPE::ED);
    tcpserver.setTurnout(&turnouts);
    tcpserver.setNodeConfigurator(config);
    tcpserver.start();
    can.setTcpServer(&tcpserver);

    //start the grid tcp server
    tcpServer *gridserver;
    if (start_grid_server){
        tcpServer tcpserverGrid = tcpServer(&logger, gridport, &can, nullptr, CLIENT_TYPE::GRID);
        tcpserverGrid.setNodeConfigurator(config);
        tcpserverGrid.start();
        can.setTcpServer(&tcpserverGrid);
        gridserver = &tcpserverGrid;
    }

    //keep looping forever
    while (running == 1){usleep(1000000);};

    //finishes
    logger.info("Stopping the tcp server");
	session_handler.stop();
    tcpserver.stop();
    gridserver->stop();

    logger.info("Stopping CBUS reader");
    can.stop();

    //give some time for the threads to finish
    long t = 2 * 1000000;
    usleep(t);

    //clear the stuff
    log4cpp::Category::shutdown();
    delete config;

    return 0;
}
