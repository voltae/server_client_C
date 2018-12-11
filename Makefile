##
## @file Makefile
## @brief Makefile for Client Server Communication
## Distribuited Systems
## Beispiel 1
##
## @author Valentin Platzgummer <ic17b096@technikum-wien.at>
## @author Lara Kammerer <ic17b001@technikum-wien.at>
## @date 2018/11/22
##
## @version $Revision: 1689 $
##
## URL: $HeadURL:
##
## Last Modified: $Author:
##


##
## ------------------------------------------------------------- variables --
##
CC = /usr/bin/gcc
CFLAGS = -Wall -Werror -Wextra -Wstrict-prototypes -pedantic -fno-common -O3 -g -std=gnu11
LDFLAGS = -lsimple_message_client_commandline_handling -lm
LIBOBJECT=./libsimple_message_client_commandline_handling/simple_message_client_commandline_handling.o
SERVEROBJECT=simple_message_server.o
CLIENTOBJECT=simple_message_client.o
DOXYGEN=doxygen
CD=cd
MV=mv
RM=rm
GREP=grep
EXCLUDE_PATTERN=footrulewidth


##
## ---------------------------------------------------------------- rules --
##
%.o: %.c
	$(CC) $(CFLAGS) -c $<

##
## --------------------------------------------------------------- targets --
##

.PHONY: all
all: client server

server: $(SERVEROBJECT)
	$(CC) $(CFLAGS) $(SERVEROBJECT) -osimple_message_server

client: $(CLIENTOBJECT)
	$(CC) $(CFLAGS) $(CLIENTOBJECT) -osimple_message_client  $(LDFLAGS)

debug_server: $(SERVEROBJECT)
	$(CC) $(CFLAGS) $(SERVEROBJECT) -osimple_message_server
	gdb -batch -x --args server -p7329 &

debug_client: $(CLIENTOBJECT)
	$(CC) $(CFLAGS) $(CLIENTOBJECT) -g -ossimple_message_client $(LDFLAGS)
	gdb -batch -x --args client -p7329 -u'ic17b096' -m'test' -i'localhost'


.PHONY: clean
clean:
	rm -rf *.o
	rm -f client
	rm -f server

.PHONY: distclean

distclean: clean
	$(RM) -rf doc

# create doxy documentation
doc: html pdf

.PHONY: html

# create html version of documentation
html:
	$(DOXYGEN) doxygen.dcf

# create pdf version of documentation
pdf: html
	$(CD) doc/pdf && \
    $(MV) refman.tex refman_save.tex && \
    $(GREP) -v $(EXCLUDE_PATTERN) refman_save.tex > refman.tex && \
    $(RM) refman_save.tex && \
    make && \
    $(MV) refman.pdf refman.save && \
    $(RM) *.pdf *.html *.tex *.aux *.sty *.log *.eps *.out *.ind *.idx \
    *.ilg *.toc *.tps Makefile && \
	$(MV) refman.save refman.pdf
##
## ---------------------------------------------------------- dependencies --
##

##
## =================================================================== eof ==
##