
SRC=sbot.c
INC=sbot.h
OBJ=$(SRC:.c=.o)

LDFLAGS=-lpthread -L .. -lotirc
CFLAGS=-I. -I.. -g -Wall -Wextra -Wno-unused -fPIC

TARGET=sbot

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) $^ -o $@

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -f $(TARGET)
	rm -f $(OBJ)


