
PREFIX=/usr/local

all: aheui.o
	$(CC) aheui.o -o aheui

clean:
	rm aheui *.o

install: aheui
	cp aheui $(PREFIX)/bin/caheui

dist-clean:
	rm $(PREFIX)/bin/caheui