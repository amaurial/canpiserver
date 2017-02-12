CC=g++
LIB= lib/liblog4cpp.a -Wl,-Bdynamic -lpthread
CFLAGS= -c -Wall -fpermissive -lm -std=c++11 -g -I/usr/local/include -I/usr/include -Ilib/include
LDFLAGS= -L/usr/local/lib -L/usr/lib -Llib
SOURCES=main.cpp canHandler.cpp tcpServer.cpp Client.cpp tcpClientGridConnect.cpp gpio.cpp nodeConfigurator.cpp frameCAN.cpp 
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=canpiserver

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@ $(LIB)

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *.o
	rm canpiserver
before:
	test -d obj || mkdir -p obj
