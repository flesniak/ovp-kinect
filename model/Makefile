ifeq ($(origin IMPERAS_HOME), undefined)
  $(error IMPERAS_HOME is not set. Run /opt/set_ovp_env.sh)
endif
include $(IMPERAS_HOME)/bin/Makefile.include

SRC = semihost.c
LIB = model.so
CFLAGS =  $(IMPERAS_VMIINC) -I/usr/include/libfreenect -fPIC -std=gnu99 -g -Wall -Werror -O2 -m32
LDFLAGS = $(SIM_LDFLAGS) $(IMPERAS_VMISTUBS) -shared -lfreenect -pthread

SRCOBJ = $(patsubst %.c,%.o,$(SRC))

.PHONY : all clean

all : $(LIB)

$(LIB) : $(SRCOBJ)
	gcc -o $(LIB) $^ $(LDFLAGS)

$(SRCOBJ) : $(SRC)
	gcc $(CFLAGS) -c -o $@ $(@:%.o=%.c)

clean :
	@rm -f $(SRCOBJ) $(LIB)
