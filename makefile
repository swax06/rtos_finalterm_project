CC = gcc
C+ = g++
CFLAGS = -g -Wall
OPENCV = `pkg-config opencv4 --cflags --libs`
LIBS = $(OPENCV)


all: server client

server: main.o server.o server_ext.o
	$(C+) main.o server.o server_ext.o -o server -lpthread 

main.o: main.c
	$(C+) -c main.c

server.o: server.c server.h
	$(C+) -c server.c

server_ext.o: server_ext.c server_ext.h
	$(C+) -c server_ext.c

client: client.cpp Twofish.cpp SymAlg.cpp common/includes.cpp client_sound.cpp client_sound.h
	$(C+) $(CFLAGS) client.cpp Twofish.cpp SymAlg.cpp common/includes.cpp client_sound.cpp client_sound.h -o client -lpthread -lpulse-simple -lpulse $(LIBS)

client1: client.cpp Twofish.cpp SymAlg.cpp common/includes.cpp client_sound.cpp client_sound.h
	$(C+) $(CFLAGS) client.cpp Twofish.cpp SymAlg.cpp common/includes.cpp client_sound.cpp client_sound.h -o client1 -lpthread -lpulse-simple -lpulse $(LIBS)


# client.o: client.cpp Twofish.cpp SymAlg.cpp common/includes.cpp
# 	$(C+) -c client.cpp Twofish.cpp SymAlg.cpp common/includes.cpp $(LIBS)

# client_sound.o: client_sound.cpp client_sound.h
# 	$(C+) -c client_sound.cpp

# main.o: Server/main.c
# 	$(C+) -c Server/main.c

# server.o: Server/server.c Server/server.h
# 	$(C+) -c Server/server.c

# server_ext.o: Server/server_ext.c Server/server_ext.h
# 	$(C+) -c Server/server_ext.c

# client: client.o client_sound.o
# 	$(C+) client.o client_sound.o -o client -lpthread -lpulse-simple -lpulse $(LIBS)

# client.o: Client/client.c Twofish.cpp SymAlg.cpp common/includes.cpp
# 	$(C+) -c Client/client.c Twofish.cpp SymAlg.cpp common/includes.cpp $(LIBS)

# client_sound.o: Client/client_sound.c Client/client_sound.h
# 	$(C+) -c Client/client_sound.c


# client_video.o: client_video.cpp client_video.h
# 	$(C+) -c client_video.cpp $(LIBS)

# client1: client.o client_sound.o
# 	$(C+) client.o client_sound.o -o client1 -lpthread -lpulse-simple -lpulse $(LIBS)
# git:
# 	git reset --hard origin/master
# 	git pull

clean:
	rm *.o server client