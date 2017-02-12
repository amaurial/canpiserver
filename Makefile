CC=g++
LIB= lib/liblog4cpp.a -Wl,-Bdynamic -lpthread
CFLAGS= -c -Wall -fpermissive -lm -std=c++11 -g -I/usr/local/include -I/usr/include -Ilib/include
LDFLAGS= -L/usr/local/lib -L/usr/lib -Llib
SOURCES=main.cpp canHandler.cpp tcpServer.cpp tcpClient.cpp edSession.cpp Client.cpp tcpClientGridConnect.cpp Turnout.cpp gpio.cpp nodeConfigurator.cpp frameCAN.cpp sessionHandler.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=canpi

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@ $(LIB)

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *.o
	rm canpi
before:
	test -d obj || mkdir -p obj
