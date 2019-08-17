CC = armv8-rpi3-linux-gnueabihf-g++
SRCDIR = src/
OBJDIR = obj/
BINDIR = bin/

BCM2835 = /home/amauri/pi/bcm2835-1.59
LIBPTHREAD = -Wl,-Bdynamic -lpthread
LIBLOGCPP = -Wl,-Bstatic -llog4cpp
LIBYAML = -Wl,-Bstatic -lyaml-cpp
LIBRF69 = -Wl,-Bstatic -lrh_rf69
LIBCM = -Wl,-Bdynamic -lbcm2835
LIBBOOST = -Wl,-Bdynamic -lboost_program_options
LIBMOSQ = -Wl,-Bdynamic -lmosquittopp -lmosquitto
LIB = $(LIBPTHREAD) $(LIBLOGCPP) $(LIBYAML) $(LIBRF69) $(LIBCM) $(LIBBOOST) $(LIBMOSQ)

ROOTFS = /home/amauri/pi/x-tools/armv8-rpi3-linux-gnueabihf/armv8-rpi3-linux-gnueabihf/sysroot
NOERRORFLAGS = -Wall -fpermissive -Wno-deprecated -Wno-psabi -Wno-deprecated-declarations
INCLUDES = -Iinclude -I$(ROOTFS)/usr/include -I$(ROOTFS)/include -I$(BCM2835)/src
CFLAGS = -std=c++11 --sysroot $(ROOTFS) -c $(NOERRORFLAGS) -lm -g $(INCLUDES)
LDFLAGS = --sysroot $(ROOTFS) -Llib -L$(ROOTFS)/usr/lib -L$(ROOTFS)/lib -L$(BCM2835)/src

#OBJECTS = $(SOURCES:.cpp=.o)
_BIN = control-station
BIN = $(addprefix $(BINDIR), $(_BIN))

SRC = $(wildcard $(SRCDIR)*.cpp)
_OBJS = $(patsubst $(SRCDIR)%.cpp, %.o, $(SRC))
OBJS = $(addprefix $(OBJDIR), $(_OBJS))

#.PHONY: all
all: $(BINDIR) $(OBJDIR) $(BIN)

$(BIN): $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $(BIN) $(LIB)

$(BINDIR):
	mkdir -p $(BINDIR)

$(OBJDIR)%.o : $(SRCDIR)%.cpp
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

before:
	test -d obj || mkdir -p obj

#.PHONY: clean
clean:
	rm -rf $(BINDIR) $(OBJDIR)

# debugging make
print-% :
	@echo $* = $($*)