ver = debug
VPATH = ./src:./obj
CC = gcc
ifeq ($(ver), debug)
CCFLAGS = -g -lssl -lcrypto -lpthread -levent -levent_openssl -levent_pthreads -lhttp_parser -DDEBUG
else
CCFLAGS = -O3 -lssl -lcrypto -lpthread -levent -levent_openssl -levent_pthreads -lhttp_parser
endif

ALL : bin/server bin/http_client bin/https_client
.PHONY : ALL

bin/server : obj/server.o obj/sthread.o obj/parser.o obj/html.o
	$(CC) -o $@ $^ $(CCFLAGS)

bin/http_client : obj/http_client.o
	$(CC) -o $@ $^ $(CCFLAGS)

bin/https_client : obj/https_client.o
	$(CC) -o $@ $^ $(CCFLAGS)

obj/%.o: src/%.c
	$(CC) -c -o $@ $< $(CCFLAGS)

.PHONY : clean

clean:
	rm ./bin/* ./obj/*.o
