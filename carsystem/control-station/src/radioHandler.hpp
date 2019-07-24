#ifndef RADIOHANDLER_H
#define RADIOHANDLER_H

#include <string.h>
#include <pthread.h>
#include <queue>
#include <string>
#include <vector>
#include <unistd.h>
#include <ctime>
#include <algorithm>
#include <mutex>
#include <condition_variable>
#include <iomanip>
#include <exception>

#include <log4cpp/Category.hh>
#include <yaml-cpp/yaml.h>
#include <bcm2835.h>
#include <RH_RF69.h>
#include "csrd.h"
#include "config.hpp"

#define READ_TIMEOUT 5

using namespace std;

class radioHandler
{
    public:
        radioHandler(log4cpp::Category *logger);
        virtual ~radioHandler();
        bool start();
        bool stop();
        int put_to_out_queue(char *msg, int size);        
        int put_to_incoming_queue(char *msg, int size);
        void setConfigurator(YAML::Node* config);


    private:
        bool running = false;
        log4cpp::Category *logger;
        YAML::Node* configurator;
        RH_RF69 radio1;
        RH_RF69 radio2;        
        uint8_t buffer[RH_RF69_MAX_MESSAGE_LEN];

        pthread_t radioReader;
        pthread_t queueReader;
        pthread_t queueWriter;        
        std::queue<CSRD> in_msgs;
        std::queue<CSRD> out_msgs;
        pthread_mutex_t m_mutex;
        pthread_cond_t  m_condv;
        pthread_mutex_t m_mutex_in;
        pthread_cond_t  m_condv_in;
        
        void run_in(void* param);
        void run_out(void* param);
        void run_queue_reader(void* param);

        static void* thread_entry_in(void *classPtr){
            ((radioHandler*)classPtr)->run_in(nullptr);
            return nullptr;
        }
        static void* thread_entry_out(void *classPtr){
            ((radioHandler*)classPtr)->run_out(nullptr);
            return nullptr;
        }
        static void* thread_entry_in_reader(void *classPtr){
            ((radioHandler*)classPtr)->run_queue_reader(nullptr);
            return nullptr;
        }

        bool startRadio(RH_RF69 *radio, string radioName);
        bool checkMessages(RH_RF69 *radio, string radioName);
        void printMessage(uint8_t *pbuf, int len);
        
};


#endif // RADIOHANDLER_H