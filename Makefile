CC = mipsel-linux-gcc
CFLAGS = -W -Wall -O2
OBJS = tvout.o cpu.o

all: tvout
tvout: $(OBJS)
clean:
	rm -f $(OBJS) tvout
