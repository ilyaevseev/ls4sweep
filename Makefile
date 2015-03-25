#!/usr/bin/make -f

NAME = ls4sweep
TITLE = produce listing for remove extra old daily-created backups

DESTDIR = /usr/local
ARCHIVE_DIR = .

BINDIR = /bin
DOCDIR = /doc
MANDIR = /man

bindir = $(DESTDIR)$(BINDIR)
docdir = $(DESTDIR)$(DOCDIR)/$(NAME)
mandir = $(DESTDIR)$(MANDIR)
man1dir = $(mandir)/man1

EXE =
OBJ = .o
RM  = /bin/rm -f
INSTALL = /usr/bin/install
XSL_BASE = /usr/share/xml/docbook/xsl-stylesheets/html/docbook.xsl
ARCHIVE_COMMAND = tar czf
CFLAGS = -D_FILE_OFFSET_BITS=64

ifdef WINDIR
CYGWIN = defined
endif

ifdef CYGWIN
EXE = .exe
endif

PROGRAM = $(NAME)$(EXE)
MANPAGE = $(NAME).man
WEBPAGE = $(NAME).html

SOURCES = Makefile $(NAME).spec LICENSE TODO *.c *.h *.xml *.sh
VERSION = $(shell grep '^\#define PROGRAM_VERSION' ls4sweep.c | awk '{print $$3;}' | tr -d '"')
ARCHIVE_PATH = $(ARCHIVE_DIR)/$(NAME)-$(VERSION).tar.gz

ifdef CYGWIN
all: bin
else
all: bin man doc
endif

install: install-bin install-man install-doc
install-bin: $(bindir)/$(PROGRAM)
install-doc: $(docdir)/$(WEBPAGE) $(docdir)/LICENSE
install-man: $(man1dir)/$(MANPAGE)

bin: $(PROGRAM)
doc: $(WEBPAGE) LICENSE
man: $(MANPAGE)

clean:
	$(RM) *$(OBJ)

cleanall: clean
	$(RM) $(PROGRAM) $(MANPAGE)

archive:
	$(ARCHIVE_COMMAND) $(ARCHIVE_PATH) $(SOURCES)

rpm: rpms
rpms: rpm-binary rpm-source

rpm-source:
	rpmbuild -bs --define "_sourcedir $$PWD" $(NAME).spec

rpm-binary:
	rpmbuild -bb --define "_sourcedir $$PWD" $(NAME).spec

_REPORT_C = _report.c report.h _report.h
REPORT_C  =  report.c report.h _report.h
MAIN_C    = $(NAME).c report.h

_report$(OBJ): $(_REPORT_C)
report$(OBJ) : $(REPORT_C)
$(NAME)$(OBJ): $(MAIN_C)

$(PROGRAM): $(NAME)$(OBJ) report$(OBJ) _report$(OBJ)
	$(CC) -o $@ $^

$(MANPAGE): $(PROGRAM)
	help2man -n '$(TITLE)' -N ./$(PROGRAM) > $(MANPAGE); \
	[ -s "$(MANPAGE)" ] || $(RM) "$(MANPAGE)"

$(WEBPAGE): $(NAME).xml
	xsltproc $(XSL_BASE) $^ > $@; \
	[ -s "$@" ] || $(RM) "$@"

$(bindir)/$(PROGRAM): $(PROGRAM)
	$(INSTALL) -Dm755 $^ $@

$(docdir)/$(WEBPAGE): $(WEBPAGE)
	$(INSTALL) -Dm644 $^ $@

$(docdir)/LICENSE: LICENSE
	$(INSTALL) -Dm644 $^ $@

$(man1dir)/$(MANPAGE): $(MANPAGE)
	$(INSTALL) -Dm644 $^ $@

## EOF ##
