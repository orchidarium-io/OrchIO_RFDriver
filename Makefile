#############################################################################
#
# Makefile for librf24 examples on Raspberry Pi
#
# License: GPL (General Public License)
#
# Description:
# ------------
#
prefix := /usr/local

# Detect the Raspberry Pi by the existence of the bcm_host.h file
BCMLOC=/opt/vc/include/bcm_host.h

ARCH=armv6zk
ifeq "$(shell uname -m)" "armv7l"
ARCH=armv7-a
endif

CCFLAGS=-Ofast -mfpu=vfp -mfloat-abi=hard -march=$(ARCH) -mtune=arm1176jzf-s

PROGRAMS = OrchIO_RFDriver
SOURCES = ${PROGRAMS:=.cpp}

all: ${PROGRAMS}

${PROGRAMS}: ${SOURCES}
	g++ ${CCFLAGS} -Wall -I../ -lrf24-bcm $@.cpp -o $@

clean:
	rm -rf $(PROGRAMS) *.o

install: all
	test -d $(prefix) || mkdir $(prefix)
	test -d $(prefix)/bin || mkdir $(prefix)/bin
	for prog in $(PROGRAMS); do \
	  install -m 0755 $$prog $(prefix)/bin; \
	done

.PHONY: install
