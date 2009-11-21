export

include Makefile.local.mk

SUBDIRS = src

.PHONY: all clean $(SUBDIRS) install uninstall

all: $(SUBDIRS)

clean: 
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir -f local.mk clean || exit $$?; \
	done

$(SUBDIRS):
	$(MAKE) $(SILENTDIR) -C $@ -f local.mk

install: $(SUBDIRS)
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir -f local.mk install || exit $$?; \
	done

uninstall:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir -f local.mk uninstall || exit $$?; \
	done

Makefile.local.mk:
	@echo " **** No Makefile.local.mk found, copying dist."
	cp Makefile.local.mk.dist Makefile.local.mk

run:
	LD_LIBRARY_PATH=$(LIBPATH) src/polspot 2> /dev/null

debug:
	LD_LIBRARY_PATH=$(LIBPATH) gdb src/polspot
