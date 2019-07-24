#include <iostream>
#include <string>
#include <signal.h>
#include <algorithm>
#include <exception>
#include <unistd.h>
// boost libraries
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
// yamls
#include <yaml-cpp/yaml.h>
#include "config.hpp"

// logger
#include "log4cpp/Portability.hh"
#ifdef LOG4CPP_HAVE_UNISTD_H
#include <unistd.h>
#endif


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

#include <bcm2835.h>
// radios
#include "radioHandler.hpp"
#include "tcpServer.h"


namespace po = boost::program_options;
namespace ba = boost::algorithm;
using namespace std;

bool running = true;
bool log_console = true;
bool log_file = true;
bool log_append = false;
string config_file = "config.yaml";
string logfile = "control-station.log";


void sigterm(int signo){
    running = false;
}

int main(int argc, char * argv[])
{
    // Register signals
    signal(SIGTERM, sigterm);
    signal(SIGHUP, sigterm);
    signal(SIGINT, sigterm);

    // Process the arguments
    try{
        po::options_description desc{"Options"};
        desc.add_options()
            ("help,h", "Help screen")
            ("config-file,c", po::value<std::string>()->default_value("control-station.yaml"), "Config file");
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help")){
            cout << desc << '\n';
            exit(0);
        }
        else if (vm.count("config-file")){
            cout << "Using config file " << vm["config-file"].as<std::string>() << '\n';
            config_file = vm["config-file"].as<std::string>();
        }
    }
    catch (const exception &ex){
        cerr << ex.what() << '\n';
    }

    // Load configuration file
    YAML::Node config = YAML::LoadFile(config_file);
    
    log4cpp::Priority::PriorityLevel loglevel = log4cpp::Priority::DEBUG;   

    string level = "notset";

    if (config[YAML_LOGGER]) {
        YAML::Node logger_config = config[YAML_LOGGER];
        if (logger_config[YAML_LEVEL]){
            level = ba::to_lower_copy(logger_config[YAML_LEVEL].as<std::string>());
            if (level == "debug")
                loglevel = log4cpp::Priority::DEBUG;
            else if (level == "info")
                loglevel = log4cpp::Priority::INFO;
            else if (level == "emerg")
                loglevel = log4cpp::Priority::EMERG;
            else if (level == "fatal")
                loglevel = log4cpp::Priority::FATAL;
            else if (level == "alert")
                loglevel = log4cpp::Priority::ALERT;
            else if (level == "crit")
                loglevel = log4cpp::Priority::CRIT;
            else if (level == "warn")
                loglevel = log4cpp::Priority::WARN;
            else if (level == "error")
                loglevel = log4cpp::Priority::ERROR;
            else if (level == "notice")
                loglevel = log4cpp::Priority::NOTICE;
            else if (level == "notset")
                loglevel = log4cpp::Priority::NOTSET;
            else loglevel = log4cpp::Priority::INFO;            
        }
    }
    

    //Logger config        
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
        log4cpp::Appender *appender2 = new log4cpp::RollingFileAppender("default", logfile, 5*1024*1024, 10, log_append);//5M
        appender2->setLayout(new log4cpp::BasicLayout());
        appender2->setLayout(layout2);
        logger.addAppender(appender2);
    }
    
    logger.info("Logger initated with level %s", level.c_str());

    // start the radio handler    
    radioHandler radio = radioHandler(&logger);
    //set the configurator
    radio.setConfigurator(&config);    

    //start the radio threads
    if (!radio.start()){
        logger.error("Failed to start radio Handler.");
        return 1;
    };

    // start the tcp tcp server
    int port = 2020;
    if (config[YAML_TCP_SERVER]){
        if (config[YAML_TCP_SERVER][YAML_PORT]){
            port = config[YAML_TCP_SERVER][YAML_PORT].as<int>();
        }
    }
    
    tcpServer server = tcpServer(&logger, port, &radio);
    server.setConfigurator(&config);
    if (!server.start()){
        logger.error("Failed to start tcp server on port %d", port);
        radio.stop();
        running = false;
    }    

    //keep looping forever
    while (running){usleep(1000000);};

    // stop the threads
    server.stop();
    radio.stop();

    //clear the stuff
    log4cpp::Category::shutdown();    

    //close the spi library
    bcm2835_close();

        //give some time for the threads to finish
    long t = 2 * 1000000;
    usleep(t);

    return 0;

}
