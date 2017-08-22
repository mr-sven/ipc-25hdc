EXE = v4l2rtspserver
OBJS = DeviceSource.o H264_V4l2DeviceSource.o ServerMediaSubsession.o v4l2rtspserver.o

LIVE555_DIR = live555
V4L2WRAPPER_DIR = v4l2wrapper

LDFLAGS += -L$(V4L2WRAPPER_DIR) -L$(LIVE555_DIR)/liveMedia -L$(LIVE555_DIR)/groupsock -L$(LIVE555_DIR)/UsageEnvironment -L$(LIVE555_DIR)/BasicUsageEnvironment
LDFLAGS += -lpthread -lV4L2Wrapper -lliveMedia -lBasicUsageEnvironment -lUsageEnvironment -lgroupsock -lsupc++

#stdlibc++ known issue: 
#If your program  needs libstdc++, please turn off code optimization
CFLAGS += -O2 -I $(V4L2WRAPPER_DIR)/include -I $(LIVE555_DIR)/liveMedia/include -I $(LIVE555_DIR)/groupsock/include -I $(LIVE555_DIR)/UsageEnvironment/include -I $(LIVE555_DIR)/BasicUsageEnvironment/include -pthread

.PHONY: libs all clean romfs

all: libs $(OBJS)
	$(CXX) -o $(EXE) $(OBJS) $(LDFLAGS)

.cpp.o:
	$(CXX) $(CFLAGS) -c $< 

libs:
	cd $(LIVE555_DIR) ; $(MAKE)
	cd $(V4L2WRAPPER_DIR) ; $(MAKE)

romfs:
	$(ROMFSINST) /bin/$(EXE)

clean:
	cd $(LIVE555_DIR) ; $(MAKE) clean
	cd $(V4L2WRAPPER_DIR) ; $(MAKE) clean
	@rm -f $(EXE)  *.o

