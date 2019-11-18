.PHONY:clean

default:
	gcc -O server.c -o server -lssl -lcrypto -levent -levent_openssl -levent_pthreads
	gcc -O client.c -o client -lssl -lcrypto -levent -levent_openssl -levent_pthreads

clean:
	rm server client:
