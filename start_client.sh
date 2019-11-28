#/bin/bash

EXE_DIR="./bin/"
LOG_DIR="./log/"

for i in `seq 1 100`; do { 
    ${EXE_DIR}http_client 1>${LOG_DIR}http${i}.out 2>&1
    ${EXE_DIR}https_client 1>${LOG_DIR}https${i}.out 2>&1
} &
done