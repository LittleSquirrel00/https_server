VPATH = ./src:./obj
CC = gcc
CFLAGS = -lssl -lcrypto -lpthread -levent -levent_openssl -levent_pthreads -lhttp_parser -DDEBUG

ALL : bin/server bin/http_client bin/https_client
.PHONY : ALL

bin/server : obj/server.o obj/sthread.o obj/parser.o
	$(CC) -o $@ $^ $(CFLAGS)

bin/http_client : obj/http_client.o
	$(CC) -o $@ $^ $(CFLAGS)

bin/https_client : obj/https_client.o
	$(CC) -o $@ $^ $(CFLAGS)

obj/%.o: src/%.c
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY : clean

clean:
	rm ./bin/* ./obj/*.o
