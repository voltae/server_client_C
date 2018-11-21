#!/bin/bash

# Protocol the communication flow between server and client
#
# 1. Calculate Port und define variables
username='ic17b096' # id=2329
address='localhost'
uid=$(id -u $username)
# num=$((num1 + 2 + 3))
customPort=$((uid+5000))

# 2. Start Server
#  simple_message_server -p port [-h]
/usr/local/bin/simple_message_server --port $customPort &

# 3. Start the protocolling v... verbose, -i lo (localhost) port
/usr/sbin/tcpdump -vv -i lo port $customPort -w protocol_port_$customPort.pcap &

# 4. Start the client and send date
# simple_message_client  -s server -p port -u user [-i image URL] -m message [-v] [-h]
/usr/local/bin/simple_message_client --server $address \
    --port $customPort --user $username  -v\
    --message "<p>Hello to the Server on the other side<br>\
    Time is now: <strong>$(date)</strong>"

 # aufruf mit image
 /usr/local/bin/simple_message_client --server $address \
    --port $customPort --user $username \
    --message "Dieser Post hat den Zeitstempel: <strong>$(date)</strong>\
     und sogar ein Image" -i 'https://upload.wikimedia.org/wikipedia/en/7/78/Small_scream.png'

# 5. Stop Server
# pgrep is an acronym that stands for "Process-ID Global Regular Expressions Print"
# pgrep looks through the currently running processes and lists the process
# IDs which matches the selection criteria to stdout
pidserver=$(ps -e | pgrep simple_message_)
# 6. Stop protocolling, find pid of tcpdump
pidtcpdump=$(ps -e | pgrep tcpdump)
echo $pidserver
echo $pidtcpdump

# kill tcpdump send signal 2 - SIGINT control c
sleep 5
kill $pidserver
kill $pidtcpdump

# 7. Convert the file to something readable
/usr/sbin/tcpdump -vv -X -s0 -rprotocol_port_$customPort.pcap > protocol_port_$customPort_readable.txt
