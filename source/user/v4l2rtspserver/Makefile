EXE = v4l2rtspserver
OBJS = AitXU.o V4l2Capture.o V4l2Device.o V4l2MmapDevice.o DeviceSource.o H264_V4l2DeviceSource.o ServerMediaSubsession.o v4l2rtspserver.o

LIVE555_DIR = live555

LDFLAGS += -L$(LIVE555_DIR)/liveMedia -L$(LIVE555_DIR)/groupsock -L$(LIVE555_DIR)/UsageEnvironment -L$(LIVE555_DIR)/BasicUsageEnvironment
LDFLAGS += -lpthread -lliveMedia -lBasicUsageEnvironment -lUsageEnvironment -lgroupsock -lsupc++

#stdlibc++ known issue: 
#If your program  needs libstdc++, please turn off code optimization
CFLAGS += -O2 -pthread -fexceptions
CFLAGS += -I include -I $(LIVE555_DIR)/liveMedia/include -I $(LIVE555_DIR)/groupsock/include -I $(LIVE555_DIR)/UsageEnvironment/include -I $(LIVE555_DIR)/BasicUsageEnvironment/include

.PHONY: libs all clean romfs

all: libs $(OBJS)
	$(CXX) -o $(EXE) $(OBJS) $(LDFLAGS)

.cpp.o:
	$(CXX) $(CFLAGS) -c $< 

libs:
	cd $(LIVE555_DIR) ; $(MAKE)

romfs:
	$(ROMFSINST) /bin/$(EXE)

clean:
	cd $(LIVE555_DIR) ; $(MAKE) clean
	@rm -f $(EXE)  *.o

