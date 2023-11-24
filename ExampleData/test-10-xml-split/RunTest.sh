# test file creation manual

#!/bin/bash

xterm -ls -e "../../proxy 7655" &

read -n 1 -p "Press key when proxy has started."

xterm -ls -e "../../anyReceiver X X 127.0.0.1 7655 10" &
sleep 5

nc 127.0.0.1 7655 < nc-send.txt

