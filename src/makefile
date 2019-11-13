.PHONY:clean

default:
	gcc -O server.c -o server -lssl -lcrypto -levent -levent_openssl
	gcc -O client.c -o client -lssl -lcrypto -levent -levent_openssl

clean:
	rm server client:
