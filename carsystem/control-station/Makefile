CC = armv8-rpi3-linux-gnueabihf-g++
LIB = lib/liblog4cpp.a -Wl,-Bdynamic -lpthread
ROOTFS = /home/amauri/pi/x-tools/armv8-rpi3-linux-gnueabihf/armv8-rpi3-linux-gnueabihf/sysroot
#CFLAGS= --sysroot $(ROOTFS) -c -Wall -fpermissive -lm -std=c++11 -g -I/usr/local/include -I/usr/include -Ilib/include -I/home/amauri/pi/toolchain/rootfs/usr/include
CFLAGS = --sysroot $(ROOTFS) -c -Wall -fpermissive -Wno-deprecated -lm -std=c++11 -g -Iinclude -I$(ROOTFS)/usr/include
#LDFLAGS= -L/usr/local/lib -L/usr/lib -Llib -L/home/amauri/pi/toolchain/rootfs/lib
LDFLAGS = -Llib -L$(ROOTFS)/usr/lib
SOURCES = src/main.cpp
OBJECTS = $(SOURCES:.cpp=.o)
EXECUTABLE = control-station

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@ $(LIB)

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *.o
	rm $(EXECUTABLE)
before:
	test -d obj || mkdir -p obj
