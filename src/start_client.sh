#/bin/bash

for i in `seq 1 100`; do { 
    ./http_client 1>./log/http{$i}out 2>&1
    ./https_client 1>./log/https{$i}out 2>&1
} &
done