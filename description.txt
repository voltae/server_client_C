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
