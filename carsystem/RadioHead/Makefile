# Makefile
# create a radio head library for RF69 for rpi
# Caution: requires bcm2835 library to be already installed
# http://www.airspayce.com/mikem/bcm2835/

CC            = armv8-rpi3-linux-gnueabihf-g++
AR            = armv8-rpi3-linux-gnueabihf-ar
CFLAGS        = -DRASPBERRY_PI -DBCM2835_NO_DELAY_COMPATIBILITY -D__BASEFILE__=\"$*\" 
#LFLAGS        = -static
LIBS          = -L/home/amauri/pi/bcm2835-1.59/src -lbcm2835
LIBBCM_INCLUDE= /home/amauri/pi/bcm2835-1.59/src
INCLUDE       = -I./ -I$(LIBBCM_INCLUDE)

OBJ_DIR=obj
OUT_DIR=lib
SRC_DIR = ./
OUT_LIB=$(OUT_DIR)/librh_rf69.a

_OBJS = RasPi.o RH_RF69.o RHDatagram.o RHHardwareSPI.o RHSPIDriver.o  RHGenericDriver.o RHGenericSPI.o RHReliableDatagram.o
OBJS = $(patsubst %,$(OBJ_DIR)/%,$(_OBJS))

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp 
	$(CC) -c $(INCLUDE) -o $@ $< $(CFLAGS) 

$(OUT_LIB): $(OBJS) 
	$(AR) rvs $(OUT_LIB) $^

# RasPi.o: RHutil/RasPi.cpp
# 				$(CC) $(CFLAGS) -c RHutil/RasPi.cpp -I./ $(INCLUDE) -o $(OBJ_DIR)/

# RH_RF69.o: RH_RF69.cpp
# 				$(CC) $(CFLAGS) -c $(INCLUDE) -o $(OBJ_DIR)/$^

# RHDatagram.o: RHDatagram.cpp
# 				$(CC) $(CFLAGS) -c $(INCLUDE) -o $(OBJ_DIR)/$^

# RHHardwareSPI.o: RHHardwareSPI.cpp
# 				$(CC) $(CFLAGS) -c $(INCLUDE) -o $(OBJ_DIR)/$^

# RHSPIDriver.o: RHSPIDriver.cpp
# 				$(CC) $(CFLAGS) -c $(INCLUDE) -o $(OBJ_DIR)/$^

# RHGenericDriver.o: RHGenericDriver.cpp
# 				$(CC) $(CFLAGS) -c $(INCLUDE) -o $(OBJ_DIR)/$^

# RHGenericSPI.o: RHGenericSPI.cpp
# 				$(CC) $(CFLAGS) -c $(INCLUDE) -o $(OBJ_DIR)/$^

# $(OUT_FILE_NAME): RasPi.o RH_RF69.o RHHardwareSPI.o RHGenericDriver.o RHGenericSPI.o RHSPIDriver.o 
# 				@echo "Linking"								
# 				$(AR) rcs $(OUT_DIR)/$@ $(OBJ_DIR)/$^

all: dirmake $(OUT_LIB)

dirmake:
	@mkdir -p $(OUT_DIR)
	@mkdir -p $(OBJ_DIR)
	
clean:
	rm -f $(OBJ_DIR)/*.o $(OUT_DIR)/$(OUT_FILE_NAME) Makefile.bak

rebuild: clean build
