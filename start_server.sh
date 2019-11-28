#/bin/bash

CERTIFICATE_DIR="./config/cacert.pem"
PRIVATE_KEY_DIR="./config/private_key.pem"
EXE_DIR="./bin/"
${EXE_DIR}server $CERTIFICATE_DIR $PRIVATE_KEY_DIR
