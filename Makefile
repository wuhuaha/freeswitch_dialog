LIBESL  = libesl.a
CFLAGS  = -I include/


all: server.c $(LIBESL)
	gcc $(CFLAGS) -o server server.c  $(LIBESL) -lpthread -lm
clean:
	rm -f server *.o
