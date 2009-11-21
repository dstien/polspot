unexport LDFLAGS CFLAGS

POLSPOT_VERSION = 0.0.1

LD = $(CC)
INSTALL_PREFIX ?= ${DESTDIR}/usr

CFLAGS = $(MYCFLAGS) -Wall -Wextra -std=gnu99
LDFLAGS = $(MYLDFLAGS) -levent_core -levent_pthreads

ifeq ($(DEBUG), 1)
	CFLAGS += -ggdb -DDEBUG
endif

NCURSES_INCLUDE ?= /usr/include/ncursesw
CFLAGS += -I$(NCURSES_INCLUDE)

ifeq ($(shell uname -s), Darwin)
	LDFLAGS += -lncurses
else
	LDFLAGS += -lncursesw
endif

ifeq ($(AUDIO_BACKEND), libao)
	POLSPOT_OBJS += audio_libao.o
	LDFLAGS += -lao
else
	POLSPOT_OBJS += audio_null.o
endif

ifeq ($(LIBSPOTIFY), 1)
	CFLAGS += -DLIBSPOTIFY
	LDFLAGS += -lspotify
else
	LDFLAGS += -lopenspotify
endif

POLSPOT_OBJS += audio.o commands.o main.o session.o ui.o ui_footer.o ui_help.o ui_log.o ui_sidebar.o ui_splash.o ui_trackinfo.o ui_tracklist.o
KEY_H = ../key.h
UA_H = user_agent.h

.PHONY: all clean install uninstall
all: polspot

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

$(KEY_H):
	[ -f $(KEY_H) ]

$(UA_H):
	echo -e "#ifndef POLSPOT_USER_AGENT \n#define POLSPOT_USER_AGENT \"polspot/$(POLSPOT_VERSION) `uname -s`/`uname -m`\"\n#endif" > $(UA_H)

polspot: $(KEY_H) $(UA_H) $(POLSPOT_OBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $(POLSPOT_OBJS)

clean:
	rm -f polspot
	rm -f $(POLSPOT_OBJS) $(UA_H) Makefile.dep

install: polspot
	install polspot $(INSTALL_PREFIX)/bin/polspot

uninstall:
	rm -f $(INSTALL_PREFIX)/bin/polspot
