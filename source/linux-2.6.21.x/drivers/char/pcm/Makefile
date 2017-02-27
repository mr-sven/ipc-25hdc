EXTRA_CFLAGS += -Idrivers/char/pcm/proslic_api/inc -Idrivers/char/pcm/proslic_api/custom
EXTRA_CFLAGS += -Idrivers/char/pcm/proslic_api/example_drivers/ralink/proslic_ppt_mb
EXTRA_CFLAGS += -Idrivers/char/pcm/proslic_api/example_drivers/ralink/timer

SRC_DIR = ./proslic_api/src
SRC_OBJ = $(SRC_DIR)/si_voice_version.o $(SRC_DIR)/vdaa.o \
			$(SRC_DIR)/proslic.o $(SRC_DIR)/si_voice.o

RALINK_OBJ = $(RALINK_DIR)/proslic_spi_api.o
TIMER_DIR = ./proslic_api/example_drivers/ralink/timer
RALINK_OBJ += $(TIMER_DIR)/proslic_timer_intf_linux.o ./proslic_api/slic_init.o
