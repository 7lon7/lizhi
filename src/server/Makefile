LDFLAGS+= -lpthread

all:server

server:server.o thr_channel.o thr_list.o tbf.o medialib.o
	$(CC) $^ -o $@ $(LDFLAGS) -Wall

clean:
	rm *.o server
