UNAME := $(shell uname)

ifeq ($(UNAME),Darwin)
	SUBDIRS = Python Ice BamTools
else
	SUBDIRS = Python Ice LAPACK BamTools
endif

.PHONY: $(SUBDIRS)

Ice: Python

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