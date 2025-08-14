PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
UDEVDIR ?= $(PREFIX)/etc/udev/rules.d
SYSTEMDDIR ?= $(PREFIX)/lib/systemd/system
MANDIR ?= $(PREFIX)/share/man/man8

all: aulaf87-rgb

aulaf87-rgb:
	$(CC) aulaf87-rgb.c -o aulaf87-rgb

install: aulaf87-rgb
	install -Dm755 aulaf87-rgb $(BINDIR)/aulaf87-rgb
	install -Dm644 80-aulaf87-rgb.rules $(UDEVDIR)/80-aulaf87-rgb.rules
	install -Dm644 aulaf87-rgb@.service $(SYSTEMDDIR)/aulaf87-rgb@.service
	echo "ExecStart=$(BINDIR)/aulaf87-rgb /dev/%I" >> $(SYSTEMDDIR)/aulaf87-rgb@.service
	install -Dm644 aulaf87-rgb.8 $(MANDIR)/aulaf87-rgb.8

clean:
	rm aulaf87-rgb
