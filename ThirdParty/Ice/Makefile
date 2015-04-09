SUBDIRS = ThirdParty-Sources-3.5.1 Ice-3.5.1
.PHONY: $(SUBDIRS)

Ice-3.5.1: ThirdParty-Sources-3.5.1

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