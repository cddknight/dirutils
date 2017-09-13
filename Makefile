ALLDIRS= lib utils

all: $(ALLDIRS)
	$(MAKE) -C lib
	$(MAKE) -C utils

clean: $(ALLDIRS)
	$(MAKE) -C lib clean
	$(MAKE) -C utils clean

dist: $(ALLDIRS)
	$(MAKE) -C lib dist
	$(MAKE) -C utils dist

dist-bzip2: $(ALLDIRS)
	$(MAKE) -C lib dist-bzip2
	$(MAKE) -C utils dist-bzip2

install: $(ALLDIRS)

installx: $(ALLDIRS)
	$(MAKE) -C lib install
	$(MAKE) -C utils install

distclean: $(ALLDIRS)
	$(MAKE) -C lib distclean
	$(MAKE) -C utils distclean

maintainer-clean: $(ALLDIRS)
	$(MAKE) -C lib maintainer-clean
	$(MAKE) -C utils maintainer-clean
