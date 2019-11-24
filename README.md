# File Transfer Protocol
This repository contains the material of the project thas was assigned during the "Distributed programming" course at Politecnico di Torino.

The project was about developing a concurrent TCP server (listening to the port specified as the first parameter of the command line, as a decimal integer) that, after having established a TCP connection with a client, accepts file transfer requests from the client and sends the requested files back to the client, following the protocol descripted in the assignment.

There are two version of the server:

*   **server1** 
  The server can manage only one connection at a time.
<br>
*   **server2**
  The server must create processes on demand (a new process for each new TCP connection).

The protocol for file transfer works as follows: to request a file the client sends to the server the three ASCII characters “GET” followed by the ASCII space character and the ASCII characters of the file name, terminated by the ASCII carriage return (CR) and line feed (LF):

<p align="center">
  <img src="./photos/protocolGET.png"></img><br>
</p>

(Note: the command includes a total of 6 ASCII characters, i.e. 6 bytes, plus the characters of the file name). The server responds by sending:

<p align="center">
  <img src="./photos/protocolOK.png"></img><br>
</p>


The client can request more files using the same TCP connection, by sending several GET commands, one after the other. 

In case of error (e.g. illegal command, non-existing file) the server always replies with:

<p align="center">
  <img src="./photos/protocolERR.png"></img><br>
</p>

(6 characters) and then it starts the procedure for gracefully closing the connection with the client.