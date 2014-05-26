
PREFIX=/usr/local

all: aheui.o
	$(CC) aheui.o -o aheui

clean:
	rm aheui *.o

install: aheui
	cp aheui $(PREFIX)/bin/caheui

test: aheui
	if [ -e snippets ]; then cd snippets && git pull; else git clone https://github.com/aheui/snippets; fi
	cd snippets && AHEUI=`pwd`/../aheui bash test.sh

dist-clean:
	rm $(PREFIX)/bin/caheui