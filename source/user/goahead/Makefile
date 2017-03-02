.EXPORT_ALL_VARIABLES:

ifeq ("$(CONFIG_USER_MATRIXSSL)", "y")
DIRS = ../matrixssl-1.8.3 src
# cgi-src
else
DIRS = src
# cgi-src
endif

ROOT_DIRECTORY = /etc_ro/web

all:
	for i in $(DIRS) ; do make -C $$i $@ || exit $?; done

romfs:
	$(ROMFSINST) $(ROOT_DIRECTORY)
	for i in $(DIRS) ; do make -C $$i $@ ; done

clean:
	for i in $(DIRS) ; do make -C $$i clean ; done

