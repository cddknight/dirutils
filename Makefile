ALLDIRS= libdircmd others utils

all: $(ALLDIRS)
	$(MAKE) -C libdircmd
	$(MAKE) -C others
	$(MAKE) -C utils

clean: $(ALLDIRS)
	$(MAKE) -C libdircmd clean
	$(MAKE) -C others clean
	$(MAKE) -C utils clean

dist: $(ALLDIRS)
	$(MAKE) -C libdircmd dist
	$(MAKE) -C others dist
	$(MAKE) -C utils dist

dist-bzip2: $(ALLDIRS)
	$(MAKE) -C libdircmd dist-bzip2
	$(MAKE) -C others dist-bzip2
	$(MAKE) -C utils dist-bzip2

install: $(ALLDIRS)
	$(MAKE) -C others install

installx: $(ALLDIRS)
	$(MAKE) -C libdircmd install
	$(MAKE) -C utils install

distclean: $(ALLDIRS)
	$(MAKE) -C libdircmd distclean
	$(MAKE) -C others distclean
	$(MAKE) -C utils distclean

maintainer-clean: $(ALLDIRS)
	$(MAKE) -C libdircmd maintainer-clean
	$(MAKE) -C others maintainer-clean
	$(MAKE) -C utils maintainer-clean
