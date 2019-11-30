#/bin/bash

HOME_DIR="./home/"

for i in `seq 1 10`; do { 
    echo $i > $HOME_DIR$i.txt
}
done
