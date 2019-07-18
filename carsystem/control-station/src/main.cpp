#include <iostream>
#include <string>
#include <signal.h>

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

using namespace std;

bool running = true;
bool log_console = true;
bool log_file = true;
bool log_append = false;

void sigterm(int signo){
    running = false;
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
    log4cpp::Category& logger = log4cpp::Category::getRoot();
    logger.setPriority(loglevel);

    if (log_console){
        log4cpp::PatternLayout * layout1 = new log4cpp::PatternLayout();
        layout1->setConversionPattern("%d [%p] %m%n");

        log4cpp::Appender *appender1 = new log4cpp::OstreamAppender("console", &std::cout);
        appender1->setLayout(new log4cpp::BasicLayout());
        appender1->setLayout(layout1);
        logger.addAppender(appender1);
    }


    if (log_file){

        log4cpp::PatternLayout * layout2 = new log4cpp::PatternLayout();
        layout2->setConversionPattern("%d [%p] %m%n");

        //log4cpp::Appender *appender2 = new log4cpp::FileAppender("default", logfile, append);
        log4cpp::Appender *appender2 = new log4cpp::RollingFileAppender("default", logfile,5*1024*1024,10, log_append);//5M
        appender2->setLayout(new log4cpp::BasicLayout());
        appender2->setLayout(layout2);
        logger.addAppender(appender2);
    }
    logger.info("Logger initated");
    //clear the stuff
    log4cpp::Category::shutdown();    

    return 0;

}
