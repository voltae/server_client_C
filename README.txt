Readme
======
Documentation for VCS_TCPIP Project Server/Client Distributed Systems

simple_message_server:
======================

SYNOPSIS:

   This program contains the TCP/IP message bulletin board server.

USAGE:

   simple_message_server -p -v

DESCRIPTION:

   parameters:

      -p <port> : the server port number from 1 to 65535
      -v        : verbose output of server status messages

      example:

         ./simple_message_server -p 7329

The TCP/IP message bulletin board server opens a listening socket on the given port => socket(); bind(); listen();
Every incoming connection is accepted via accept() and then a child process is forked via fork() where the external business logic
is called. (execl() call to "/usr/local/bin/simple_message_server_logic")
Because all sockets are duplicated by a fork, following socket handling must happen:
The child process closes the listening socket and the parent closes the new socket from the accept call.

simple_message_client:
======================

SYNOPSIS:

   This program contains the TCP/IP message bulletin board client, for posting messages to the bulletin. Messages are composed
   as plain text and can include following html tags: "<strong>", "</strong>","<em>", "</em>", "<br/>"
   The image is provided via a valid html link.

USAGE:

   simple_message_client  -s server -p port -u user -m message –i image

   example:

      ./simple_message_client -s localhost -p7329 -u 'ic17b096' -m testrun -i <imageURL>


DESCRIPTION:

   parameters:

      -s, IP/Hostname> Hostname or IP address of the server
      -p, <port> Server Port
      -u, <name> Name that will be displayed on the message board
      -m, <text> Text that will be send to the message board
      -i, <URL> URL of an image
      -v, Acitvate verbose output
      -h, Prints Usage
