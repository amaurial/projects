# Makefile
# Sample for RH_RF69 (client and server) on Raspberry Pi
# Caution: requires bcm2835 library to be already installed
# http://www.airspayce.com/mikem/bcm2835/

CC            = armv8-rpi3-linux-gnueabihf-g++
CFLAGS        = -DRASPBERRY_PI -DBCM2835_NO_DELAY_COMPATIBILITY -D__BASEFILE__=\"$*\" 
#LFLAGS        = -static
LIBS          = -L/home/amauri/pi/bcm2835-1.59/src -lbcm2835
RADIOHEADBASE = ../../..
LIBBCM_INCLUDE= /home/amauri/pi/bcm2835-1.59/src
INCLUDE       = -I$(RADIOHEADBASE) -I$(LIBBCM_INCLUDE)

all: rf69_client rf69_server rf69_dual_server

RasPi.o: $(RADIOHEADBASE)/RHutil/RasPi.cpp
				$(CC) $(CFLAGS) -c $(RADIOHEADBASE)/RHutil/RasPi.cpp $(INCLUDE)

rf69_dual_server.o: rf69_dual_server.cpp
				$(CC) $(CFLAGS) -c $(INCLUDE) $<

rf69_client.o: rf69_client.cpp
				$(CC) $(CFLAGS) -c $(INCLUDE) $<

rf69_server.o: rf69_server.cpp
				$(CC) $(CFLAGS) -c $(INCLUDE) $<

RH_RF69.o: $(RADIOHEADBASE)/RH_RF69.cpp
				$(CC) $(CFLAGS) -c $(INCLUDE) $<

RHDatagram.o: $(RADIOHEADBASE)/RHDatagram.cpp
				$(CC) $(CFLAGS) -c $(INCLUDE) $<

RHHardwareSPI.o: $(RADIOHEADBASE)/RHHardwareSPI.cpp
				$(CC) $(CFLAGS) -c $(INCLUDE) $<

RHSPIDriver.o: $(RADIOHEADBASE)/RHSPIDriver.cpp
				$(CC) $(CFLAGS) -c $(INCLUDE) $<

RHGenericDriver.o: $(RADIOHEADBASE)/RHGenericDriver.cpp
				$(CC) $(CFLAGS) -c $(INCLUDE) $<

RHGenericSPI.o: $(RADIOHEADBASE)/RHGenericSPI.cpp
				$(CC) $(CFLAGS) -c $(INCLUDE) $<

rf69_client: rf69_client.o RH_RF69.o RasPi.o RHHardwareSPI.o RHGenericDriver.o RHGenericSPI.o RHSPIDriver.o 
				$(CC) $^ $(LIBS) $(LFLAGS) -o rf69_client

rf69_server: rf69_server.o RH_RF69.o RasPi.o RHHardwareSPI.o RHGenericDriver.o RHGenericSPI.o RHSPIDriver.o
				$(CC) $^ $(LIBS) $(LFLAGS) -o rf69_server

rf69_dual_server: rf69_dual_server.o RH_RF69.o RasPi.o RHHardwareSPI.o RHGenericDriver.o RHGenericSPI.o RHSPIDriver.o
				$(CC) $^ $(LIBS) $(LFLAGS) -o rf69_dual_server

clean:
				rm -rf *.o rf69_client rf69_server rf69_dual_server
