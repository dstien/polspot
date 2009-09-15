unexport LDFLAGS CFLAGS

NCURSES_INCLUDE ?= /usr/include/ncursesw

CFLAGS += -I$(NCURSES_INCLUDE)
ifeq ($(shell uname -s),Darwin)
    LDFLAGS += -lncurses
else
    LDFLAGS += -lncursesw
endif

POLSPOT_OBJS = commands.o event.o main.o session.o ui.o ui_footer.o ui_help.o ui_log.o ui_sidebar.o ui_splash.o ui_tracklist.o

.PHONY: all clean install uninstall
all: polspot

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

polspot: $(POLSPOT_OBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $(POLSPOT_OBJS)

clean:
	rm -f polspot
	rm -f $(POLSPOT_OBJS) Makefile.dep

install: polspot
	install polspot $(INSTALL_PREFIX)/bin/polspot

uninstall:
	rm -f $(INSTALL_PREFIX)/bin/polspot
