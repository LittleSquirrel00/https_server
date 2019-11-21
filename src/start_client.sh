#/bin/bash

for i in `seq 1 10`; do { 
    ./http_client 1>out.log 2>&1
    ./https_client 1>out.log 2>&1
} &
done