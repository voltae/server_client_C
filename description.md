# server_client_C

Server-Client Application in C

## Aufgabe 1: Protokoll Analyse

Untersuchen Sie das Zusammenspiel der 3 Programme. Benutzen Sie tcpdump(8) um die Kommunikation zwischen Client und Server nachzuvollziehen und erstellen Sie eine Protokoll Analyse. Verwenden Sie dazu die vorbereitete OpenOffice Vorlage VCS_TCPIP_Protokoll_Analyse_Template.ott (MS Word: VCS_TCPIP_Protokoll_Analyse_Template.doc) und speichern Sie die fertige Protokoll Analyse als VCS_TCPIP_Protokoll_Analyse_.odt (bzw. VCS_TCPIP_Protokoll_Analyse.doc) ab und konvertieren Sie es in ein PDF File mit Namen VCS_TCPIP_Protokoll_Analyse_.pdf. Die Abgabe der fertigen Protokoll Analyse (PDF File) erfolgt per E-Mail an Ihren Betreuer am Tag vor der zweiten Studieneinheit (Deadline 23:59:59 CET (das kennen Sie ja schon von Betriebsysteme ;-))).
## Aufgabe 2: Implementierung Client

Ersetzen Sie den Client simple_message_client(1) durch Ihre eigene Implementierung und speichern Sie diese unter simple_message_client.c ab.

Um Ihnen das Parsen der Kommandozeilenargumente zu erleichtern steht Ihnen am annuminas die Library libsimple_message_client_commandline_handling.a in /usr/local/lib zur Verfügung. - Diese Library bietet die Funktion smc_parsecommandline(3), welche genau auf das Parsen der Kommendozeile des Clients ausgerichtet ist. Das Linken mit dieser Library erfolgt durch Angabe von -lsimple_message_client_commandline_handling in der Kommandozeile zum Linken des Client Executables.

Die Sourcen zu dieser Library liegen am annuminas in /usr/local/src/libsimple_message_client_commandline_handling.tar.gz bzw. /usr/local/src/libsimple_message_client_commandline_handling.zip.

Ihr Client soll genauso wie die Musterimplementierung (simple_message_client(1)) sowohl mit IPV4 als auch mit IPV6 funktionieren.
## Aufgabe 3: Implementierung Server

Ersetzen Sie den Server simple_message_server(1) durch Ihre eigene Implementierung und speichern Sie diese unter simple_message_server.c ab.

Bei Ihrem Server ist es ausreichend wenn er, genauso wie die Musterimplementierung (simple_message_server(1)), ausschließlich mit IPV4 funktioniert.

Die Businesslogic des Servers brauchen Sie nicht zu implementieren. - Verwenden Sie das fertige Executable simple_message_server_logic(1) im Sinne edmenes spawning servers.

### Server Funktionen
NAME
       simple_message_server - The VCS TCP/IP message bulletin board server.

SYNOPSIS
       simple_message_server -p port [-h]

DESCRIPTION
       simple_message_server - The VCS TCP/IP message bulletin board server.

       simple_message_server (VCS TCP/IP message bulletin board server) is the
       daemon program for the simple_message_client(1) client  program.   sim-
       ple_message_server  listens  on  TCP port port for incoming connections
       from  simple_message_client(1)   applications.    simple_message_server
       forks  a  new  daemon for each incoming connection, redirects stdin and
       stdout of the forked child daemons to the connected socket and executes
       the  simple_message_server_logic(1) executable in the forked child dae-
       mons.

       Providing the -h commandline option causes a usage message to be  writ-
       ten to stdout.

OPTIONS
       The following options are supported:

       -p, --port port
              Use  port  (which  is  either a port number or a service name as
              listed in /etc/services) as the TCP port to  listen  on.  -  The
              range for port numbers is limited to [0..65535].

       -h, --help
              Write usage information to stdout.

SEE ALSO
       simple_message_client(1), simple_message_server_logic(1)


### Client Funktionen
NAME
       simple_message_client - The VCS TCP/IP message bulletin board client.

SYNOPSIS
       simple_message_client  -s server -p port -u user [-i image URL] -m mes-
       sage [-v] [-h]

DESCRIPTION
       simple_message_client - Send a message to the VCS TCP/IP  message  bul-
       letin board server simple_message_server(1).

       The  message  may contain the following HTML tags: <strong>, </strong>,
       <em>, </em>, <br/>.

       For the server either the IPV4 or IPV6 address or a  hostname  must  be
       provided via the server option. The TCP port to connect to is given via
       the port option, which can either be a port number of a service name as
       listed in /etc/services.

       The  user  name  of the message submitter must be provided via the user
       option. Optionally the URL of an image for the user can be provided via
       the image URL option.

       In  case the -v commandline option is provided, execution trace message
       are written to stdout.

       Providing the -h commandline option causes a usage message to be  writ-
       ten to stdout.

OPTIONS
       The following options are supported:

       -s, --server  server
              Use server (which is either an IPV4 address, an IPV6 address, or
              a hostname) as the server to connect to.

       -p, --port port
              Use port (which is either a port number or  a  service  name  as
              listed  in  /etc/services)  as the TCP port to connect to. - The
              range for port numbers is limited to [0..65535].

       -u, --user user
              Use user as user name for the message submission.

       -i, --image image URL
              Use image URL as URL of an image for the submitting user.

       -m, --message message
              Use message as message to submit to the bulletin board.

       -v, --verbose
              Write execution trace information to stdout

       -h, --help
              Write usage information to stdout.