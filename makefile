.PHONY:clean

BINRARY_DIR=./bin/
SRC_DIR=./src/
TEST_DIR=./test/

default:
	gcc -O2 ${SRC_DIR}sthread.c ${SRC_DIR}parser.c ${SRC_DIR}server.c -o ${BINRARY_DIR}server -lssl -lcrypto -lpthread -levent -levent_openssl -levent_pthreads -lhttp_parser
	gcc -O2 ${SRC_DIR}http_client.c -o ${BINRARY_DIR}http_client -lssl -lcrypto -lpthread -levent -levent_openssl -levent_pthreads
	gcc -O2 ${SRC_DIR}https_client.c -o ${BINRARY_DIR}https_client -lssl -lcrypto -lpthread -levent -levent_openssl -levent_pthreads
	# gcc -O2 ${TEST_DIR}test.c -o ${BINRARY_DIR}test -lhttp_parser
clean:
	rm ${BINRARY_DIR}*
