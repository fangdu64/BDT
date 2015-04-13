SUBDIRS = ../../ThirdParty bdtUtil bdtCore bigMat bigKmeans/server bigKmeans/contractor
.PHONY: $(SUBDIRS)

bdtUtil: ../../ThirdParty
bdtCore: bdtUtil
bigMat: bdtCore
bigKmeans/server: bigMat
bigKmeans/contractor: bigMat

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

