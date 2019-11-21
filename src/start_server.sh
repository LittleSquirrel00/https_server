#/bin/bash

CERTIFICATE_DIR="./cacert.pem"
PRIVATE_KEY_DIR="./private_key.pem"
./server $CERTIFICATE_DIR $PRIVATE_KEY_DIR
