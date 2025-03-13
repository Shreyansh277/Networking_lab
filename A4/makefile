LIBNAME = libksocket.a
LIBS = -L. -lksocket
CC = gcc
AR = ar
CFLAGS = -c -Wall
RM = rm -f

all: $(LIBNAME) initksocket user1 user2

apps: user1 user2

$(LIBNAME): ksocket.o
	$(AR) rcs $(LIBNAME) ksocket.o

ksocket.o: ksocket.c ksocket.h
	$(CC) $(CFLAGS) -o ksocket.o ksocket.c

initksocket: initksocket.o $(LIBNAME)
	$(CC) -o initksocket initksocket.o $(LIBS)

initksocket.o: initksocket.c ksocket.h
	$(CC) $(CFLAGS) -o initksocket.o initksocket.c

user1: user1.o $(LIBNAME)
	$(CC) -o user1 user1.o $(LIBS)

user1.o: user1.c ksocket.h
	$(CC) $(CFLAGS) -o user1.o user1.c

user2: user2.o $(LIBNAME)
	$(CC) -o user2 user2.o $(LIBS)

user2.o: user2.c ksocket.h
	$(CC) $(CFLAGS) -o user2.o user2.c

clean:
	$(RM) *.o $(LIBNAME) initksocket user1 user2 new*.txt