CC:=$(CROSS_COMPILE)gcc

INCLUDE:=-I$(PROJECT_PATH)/linux-2.6.20-lf1000/include
CFLAGS=-std=gnu99 -Wall -pedantic -DDEBUG_PRINT
LIBS=-lrt

all: monitord rl

monitord: monitord.c socket.c
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LIBS)

test_monitord: test_monitord.c socket.c
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LIBS)

monitor-ctl: monitor-ctl.c socket.c
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LIBS)

rl: rl.c
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LIBS)

clean:
	rm -f *.o monitord test_monitord monitor-ctl rl
