
SRC=libotirc.c
INC=libotirc.h
OBJ=$(SRC:.c=.o)

LDFLAGS=-lpthread
CFLAGS=-I. -g -Wall -Wextra -fPIC

TARGET=libotirc.so

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -shared $(LDFLAGS) $^ -o $@

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

install:
	cp $(INC) $(DESTDIR)/usr/include/
	cp $(TARGET) $(DESTDIR)/usr/lib/

uninstall:
	rm $(DESTDIR)/usr/include/$(INC)
	rm $(DESTDIR)/usr/lib/$(TARGET)

clean:
	rm -f $(TARGET)
	rm -f $(OBJ)


