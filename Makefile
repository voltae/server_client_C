##
## @file Makefile for Client Server Communication
## Distribuited Systems
## Beispiel 0
##
## @author Valentin Platzgummer <ic17b096@technikum-wien.at>
## @author Lara Kammerer <ic17b001@technikum-wien.at>
## @date 2018/11/22
##
## @version $Revision: 1689 $
##
## @todo
##
## URL: $HeadURL:
##
## Last Modified: $Author:
##


# Define used Variables
CC = /usr/bin/gcc
CFLAGS = -Wall -Werror -Wextra -Wstrict-prototypes -pedantic -fno-common -O3 -g -std=gnu11
LDFLAGS = ./libsimple_message_client_commandline_handling/libsimple_message_client_commandline_handling.a
LIBOBJECT=./libsimple_message_client_commandline_handling/simple_message_client_commandline_handling.o

##
## ------------------------------------------------------------- variables --
##

%.o: %.c
	$(CC) $(CFLAGS) -c $<

##
## --------------------------------------------------------------- targets --
##

.PHONY: all
all: server client

server: server.o
	$(CC) $(CFLAGS) -oserver -L$(LDFLAGS)

client: client.o
	$(CC) $(CFLAGS) client.c $(LIBOBJECT) -oclient -L $(LDFLAGS)


.PHONY: clean
clean:
	rm -rf *.o