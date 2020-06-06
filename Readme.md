This has been upgraded from chat program, so it supports voice as well as chat features.
To compile server and client, run: "make".

How to make call to another online user:
1. Run server using "./server [ip] [port]".

2. Run client using "./client [ip of server] [port of server]".

3. When connected to server, client can set his/her user name.

4. Enter "-call" command to initiate call will other online user. This command will display list of online users and client can select
   another user to call.

5. If callee accepts the call by pressing "-yes", real time voice chat will be initialised for these users.

6. Press "ctrl + c" to end call.