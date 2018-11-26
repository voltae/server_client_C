#@ Makefile for Client Server Applicatio
#@author: Valentin Platzgummer - ic 17b096
#@date 2018-11-26

# Define used Variables
CC = /usr/bin/gcc
CFLAGS = -Wall -Wextra -pedantic -std=gnu11
LDFLAGS =


# ------------------ rules ----
%.o: %.c
	$(CC) $(CFLAGS) -c -g $<

# ------------------- targets -

.PHONY: all
all: Server Client

Server: Server.c
	$(CC) $(CFLAGS) Server.c -oServer

Client: Client.c
	$(CC) $(CFLAGS) Client.c -oClient


.PHONY: clean
clean:
	rm -rf *.o