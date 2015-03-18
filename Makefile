CFLAGS := -std=c11 -g \
	-Wall -Wextra -pedantic \
	-Wshadow -Wpointer-arith -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes \
	-D_GNU_SOURCE \
	$(CFLAGS)

PREFIX = /usr

all: accept_dad_hack

install: accept_dad_hack
	install -Dm755 accept_dad_hack $(DESTDIR)$(PREFIX)/bin/accept_dad_hack

clean:
	$(RM) accept_dad_hack *.o

.PHONY: clean install uninstall
