ALLDIRS= libdircmd utils

all: $(ALLDIRS)
	$(MAKE) -C libdircmd
	$(MAKE) -C utils
	$(MAKE) -C others

clean: $(ALLDIRS)
	$(MAKE) -C libdircmd clean
	$(MAKE) -C utils clean
	$(MAKE) -C others clean

dist: $(ALLDIRS)
	$(MAKE) -C libdircmd dist
	$(MAKE) -C utils dist
	$(MAKE) -C others dist

dist-bzip2: $(ALLDIRS)
	$(MAKE) -C libdircmd dist-bzip2
	$(MAKE) -C utils dist-bzip2
	$(MAKE) -C others dist-bzip2

install: $(ALLDIRS)

installx: $(ALLDIRS)
	$(MAKE) -C libdircmd install
	$(MAKE) -C utils install
	$(MAKE) -C others install

distclean: $(ALLDIRS)
	$(MAKE) -C libdircmd distclean
	$(MAKE) -C utils distclean
	$(MAKE) -C others distclean

maintainer-clean: $(ALLDIRS)
	$(MAKE) -C libdircmd maintainer-clean
	$(MAKE) -C utils maintainer-clean
	$(MAKE) -C others maintainer-clean
