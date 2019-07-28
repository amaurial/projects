// rf69_server.cpp
//
// Example program showing how to use RH_RF69 on Raspberry Pi
// Uses the bcm2835 library to access the GPIO pins to drive the RFM69 module
// Requires bcm2835 library to be already installed
// http://www.airspayce.com/mikem/bcm2835/
// Use the Makefile in this directory:
// cd example/raspi/rf69
// make
// sudo ./rf69_server
//
// Contributed by Charles-Henri Hallard based on sample RH_NRF24 by Mike Poublon

#include <bcm2835.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string>

#include <RH_RF69.h>

// Module 1 on board RFM95 868 MHz (example)
#define CSRD1_CS_PIN  8 //RPI_V2_GPIO_P1_24 // Slave Select on CE0 so P1 connector pin #24
#define CSRD1_IRQ_PIN 25 //RPI_V2_GPIO_P1_22 // IRQ on GPIO25 so P1 connector pin #22

// Module 2 on board RFM95 433 MHz (example)
#define CSRD2_CS_PIN  7 //RPI_V2_GPIO_P1_26 // Slave Select on CE1 so P1 connector pin #26
#define CSRD2_IRQ_PIN 16 //RPI_V2_GPIO_P1_36 // IRQ on GPIO16 so P1 connector pin #36

// Module 3 on board RFM69HW (example)
#define CSRD3_CS_PIN  RPI_V2_GPIO_P1_37 // Slave Select on GPIO26  so P1 connector pin #37
#define CSRD3_IRQ_PIN RPI_V2_GPIO_P1_16 // IRQ on GPIO23 so P1 connector pin #16

// Our RFM69 Configuration 
#define RF_FREQUENCY  915.00
#define RADIO1_ID    1
#define RADIO1_GROUP_ID 50 

#define RADIO2_ID    2
#define RADIO2_GROUP_ID 50

// Create an instance of a driver
RH_RF69 *radio1 = new RH_RF69(CSRD1_CS_PIN, CSRD1_IRQ_PIN);
RH_RF69 *radio2 = new RH_RF69(CSRD2_CS_PIN, CSRD2_IRQ_PIN);

//RH_RF69 radio1(CSRD1_CS_PIN, CSRD1_IRQ_PIN);
//RH_RF69 radio2(CSRD2_CS_PIN, CSRD2_IRQ_PIN);

//RH_RF69 radios[2];

using namespace std;

//Flag for Ctrl-C
volatile sig_atomic_t force_exit = false;

bool setupRadio (RH_RF69 *radio, string radioName, uint8_t radioID, uint8_t radioGroup)
{
if (!radio->init()) {
        fprintf( stderr, "\nRF69 %s module init failed, Please verify wiring/module\n", radioName.c_str() );
        return false;
    }
    else {        
        printf( "\nRF69 %s module seen OK!\r\n", radioName.c_str());
    }

    radio->available();        
    radio->setTxPower(14);        
    radio->setModemConfig(RH_RF69::GFSK_Rb250Fd250);
    radio->setFrequency( RF_FREQUENCY );
    // set Network ID (by sync words)
    uint8_t syncwords[2];
    syncwords[0] = 0x2d;
    syncwords[1] = radioGroup;
    radio->setSyncWords(syncwords, sizeof(syncwords));
    
    printf("%s GroupID=%d, node #%d init OK @ %3.2fMHz\n", radioName.c_str(), radioGroup, radioID, RF_FREQUENCY );
    
    // Be sure to grab all node packet 
    // we're sniffing to display, it's a demo
    radio->setPromiscuous(true);  

    // We're ready to listen for incoming message
    radio->setModeRx();    
    return true;
}

bool checkMessages(RH_RF69 *radio, string radioName){
    
	uint8_t len = 0;
    if (radio->available()) { 
        //if (rf69.waitAvailableTimeout(50)) { 
        printf( "...\n" );

        // Should be a message for us now
        uint8_t buf[RH_RF69_MAX_MESSAGE_LEN];
        len  = sizeof(buf);
        uint8_t from = radio->headerFrom();
        uint8_t to   = radio->headerTo();
        uint8_t id   = radio->headerId();
        uint8_t flags= radio->headerFlags();;
        int8_t rssi  = radio->lastRssi();
          
        if (radio->recv(buf, &len)) {
            radio->setModeTx();
            radio->setModeRx();

            printf("Radio: %s Packet[%02d] #%d => #%d %ddB: ",radioName.c_str(), len, from, to, rssi);
            //rf69.printBuffer("buffer:",buf, len);
            printbuffer(buf, len);

            // Send a message to rf69_server
            /*
            printf("\nSending %02d bytes to node #%d => ", len, RF_NODE_ID);
            len = sizeof(data);
            printbuffer(data, len);
            printf("\n" );
            radio->send(data, len);
            radio->waitPacketSent();
            */
            return true;
        } 
        printf("\n");        
    }
    return false;
}

void sig_handler(int sig)
{
    printf("\n%s Break received, exiting!\n", __BASEFILE__);
    force_exit=true;
}


//Main Function
int main (int argc, const char* argv[] )
{  
    //radios[0] = radio1;
    //radios[1] = radio2;
    signal(SIGINT, sig_handler);
    printf( "%s\n", __BASEFILE__);
    
    if (!bcm2835_init()) 
    {
        fprintf( stderr, "%s bcm2835_init() Failed\n\n", __BASEFILE__ );
        return 1;
    }
  
    printf( "RADIO1 CS=GPIO%d", CSRD1_CS_PIN);
    printf( "RADIO2 CS=GPIO%d", CSRD2_CS_PIN);

    if (! setupRadio(radio1, "RADIO1", RADIO1_ID, RADIO1_GROUP_ID))
    {
        exit (1);
    }
    
    if (! setupRadio(radio2, "RADIO1", RADIO2_ID, RADIO2_GROUP_ID))
    {
        exit (1);
    }

    printf( "Listening packet...\n" );

    //Begin the main body of code
    while (!force_exit) 
    {
        if (checkMessages(radio1, "RADIO1")){
            //send message back with radio 2            
        /*    uint8_t data[] = "Hi radio2!";  
            uint8_t len = sizeof(data);     
            printf("\nRadio 2 Sending %02d bytes to node #%d => ", len, RADIO2_ID);         
            printbuffer(data, len);
            printf("\n" );
            radio2.send(data, len);
            radio2.waitPacketSent();
        */
        }
        checkMessages(radio2, "RADIO2");
      // Let OS doing other tasks
      // For timed critical appliation you can reduce or delete
      // this delay, but this will charge CPU usage, take care and monitor
      bcm2835_delay(5);
    }  

    printf( "\n%s Ending\n", __BASEFILE__ );
    bcm2835_close();
    return 0;
}

