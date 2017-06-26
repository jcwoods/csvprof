TARGET=csvprof
SOURCES=csvprof.c
CFLAGS=-O2 -Wall

INCLUDES=-I/opt/libcsv/include
LIBS=-lcsv
LIBPATH=-L/opt/libcsv/lib

all:	$(TARGET)

clean:	
	rm -f $(TARGET)

$(TARGET):	$(SOURCES)
	$(CC) $(CFLAGS) $(INCLUDES) $(LIBPATH) -o $(TARGET) $(SOURCES) $(LIBS)
