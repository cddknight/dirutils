ALLDIRS= libdircmd utils

all: $(ALLDIRS)
	$(MAKE) -C libdircmd
	$(MAKE) -C utils

clean: $(ALLDIRS)
	$(MAKE) -C libdircmd clean
	$(MAKE) -C utils clean

dist: $(ALLDIRS)
	$(MAKE) -C libdircmd dist
	$(MAKE) -C utils dist

dist-bzip2: $(ALLDIRS)
	$(MAKE) -C libdircmd dist-bzip2
	$(MAKE) -C utils dist-bzip2

install: $(ALLDIRS)

installx: $(ALLDIRS)
	$(MAKE) -C libdircmd install
	$(MAKE) -C utils install

distclean: $(ALLDIRS)
	$(MAKE) -C libdircmd distclean
	$(MAKE) -C utils distclean

maintainer-clean: $(ALLDIRS)
	$(MAKE) -C libdircmd maintainer-clean
	$(MAKE) -C utils maintainer-clean
