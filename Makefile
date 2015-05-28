
ifeq ($(prefix),)
	prefix	= ~/bdtHome
endif

SUBDIRS = build/linux
.PHONY: $(SUBDIRS)

build/linux: config

all install:: $(SUBDIRS)

$(SUBDIRS):
	@echo "making install in $@"
	@$(MAKE) install --directory=$@

clean::
	@for subdir in $(SUBDIRS); \
        do \
            if test -d $$subdir ; \
            then \
                echo "making $@ in $$subdir"; \
                ( cd $$subdir && $(MAKE) $@ ) || exit 1; \
            fi; \
        done

config::
	rm -f config/Make.rules
	@echo BDT_Install=\"$(prefix)\" > config/Make.rules
	