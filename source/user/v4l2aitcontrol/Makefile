EXE = v4l2aitcontrol
OBJS = AitXU.o v4l2aitcontrol.o

#stdlibc++ known issue: 
#If your program  needs libstdc++, please turn off code optimization
CFLAGS += -O0 

.PHONY: libs all clean romfs

all: $(OBJS)
	$(CC) -o $(EXE) $(OBJS) $(LDFLAGS)

.o:
	$(CC) $(CFLAGS) -c $< 

romfs:
	$(ROMFSINST) /bin/$(EXE)

clean:
	@rm -f $(EXE)  *.o

