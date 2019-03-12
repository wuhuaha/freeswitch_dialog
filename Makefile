ESLPATH = esl
LIBESL  = $(ESLPATH)/.libs/libesl.a
CFLAGS  = -I $(ESLPATH)/src/include
#include $(FS_SRC_DIR)/build/modmake.rules 


all: uniu_esl.c uniu_esl_tst.c  $(LIBESL)
	gcc $(CFLAGS) -o uniu_esl uniu_esl.c uniu_list.c  uniu_esl_tst.c  $(LIBESL) -lpthread -lm
clean:
	rm -f uniu_esl *.o
