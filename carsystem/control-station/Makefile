CC = armv8-rpi3-linux-gnueabihf-g++
BCM2835=/home/amauri/pi/bcm2835-1.59
LIB = -Wl,-Bdynamic -lpthread -Wl,-Bstatic -llog4cpp -Wl,-Bstatic -lyaml-cpp -Wl,-Bstatic -lrh_rf69 -Wl,-Bdynamic -lbcm2835 -Wl,-Bdynamic -lboost_program_options
ROOTFS = /home/amauri/pi/x-tools/armv8-rpi3-linux-gnueabihf/armv8-rpi3-linux-gnueabihf/sysroot
#CFLAGS= --sysroot $(ROOTFS) -c -Wall -fpermissive -lm -std=c++11 -g -I/usr/local/include -I/usr/include -Ilib/include -I/home/amauri/pi/toolchain/rootfs/usr/include
CFLAGS = -std=c++11 --sysroot $(ROOTFS) -c -Wall -fpermissive -Wno-deprecated -lm -std=c++11 -g -Iinclude -I$(ROOTFS)/usr/include -I$(ROOTFS)/include -I/$(BCM2835)/src
#LDFLAGS= -L/usr/local/lib -L/usr/lib -Llib -L/home/amauri/pi/toolchain/rootfs/lib
LDFLAGS = --sysroot $(ROOTFS) -Llib -L$(ROOTFS)/usr/lib -L$(ROOTFS)/lib -L$(BCM2835)/src
SOURCES = src/main.cpp src/radio_handler.cpp src/csrd.cpp
OBJECTS = $(SOURCES:.cpp=.o)
EXECUTABLE = control-station

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@ $(LIB)

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f ./src/*.o
	rm $(EXECUTABLE)
before:
	test -d obj || mkdir -p obj
