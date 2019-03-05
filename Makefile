ESLPATH = esl
LIBESL  = $(ESLPATH)/.libs/libesl.a
CFLAGS  = -I$(ESLPATH)/src/include
#include $(FS_SRC_DIR)/build/modmake.rules 


all: server.c $(LIBESL)
	gcc $(CFLAGS) -o server server.c  $(LIBESL) -lpthread -lm
clean:
	rm -f server *.o
