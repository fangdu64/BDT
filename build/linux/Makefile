bdt_topdir		= ../..
include $(bdt_topdir)/config/Make.rules
top_srcdir		= $(bdt_topdir)/ThirdParty/Ice/Ice-3.5.1/cpp
include $(top_srcdir)/config/Make.rules

iBS_SliceDestDir =$(BDT_Install)/bdt/slice
iBS_SliceSrcDir =$(bdt_topdir)/common/slice

iBS_iBSPyDestDir =$(BDT_Install)/bdt/bdtPy
iBS_iBSPySrcDir =$(bdt_topdir)/bdtPy

iBS_iBSShortcutCmdDestDir =$(BDT_Install)
iBS_iBSCmdDestDir =$(BDT_Install)/bdt/bdtCmds
iBS_pythonBin =$(BDT_Install)/ThirdParty/python/bin/python3.3
iBS_cmdtoolInstall =$(bdt_topdir)/bdtCmds/install-linux.py

iBS_iBSConfigDestDir =$(BDT_Install)/bdt/config
iBS_iBSConfigSrcDir =$(bdt_topdir)/common/config

iBS_iBSRDestDir =$(BDT_Install)/bdt/bdtR
iBS_iBSRSrcDir =$(bdt_topdir)/bdtR

ifeq ($(R_BinDir),)
	R_BinDir	= None
endif

SUBDIRS = bdtUtil bdtCore bigMat bdvd bigKmeans/server bigKmeans/contractor
.PHONY: $(SUBDIRS)

bdtCore: bdtUtil
bigMat: bdtCore
bdvd: bdtCore
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

install:: scripts
scripts::
	rm -rf $(iBS_SliceDestDir)
	$(call mkdir,$(iBS_SliceDestDir), -p)
	cp -rf $(iBS_SliceSrcDir)/* $(iBS_SliceDestDir)
	rm -rf $(iBS_iBSPyDestDir)
	$(call mkdir,$(iBS_iBSPyDestDir), -p)
	cp -rf $(iBS_iBSPySrcDir)/* $(iBS_iBSPyDestDir)

	@echo R_BinDir=\"$(R_BinDir)\" > $(iBS_iBSPyDestDir)/iBSConfig.py
	@echo BDT_HomeDir=\"$(BDT_Install)\" >> $(iBS_iBSPyDestDir)/iBSConfig.py

	rm -rf $(iBS_iBSShortcutCmdDestDir)/*.py
	rm -rf $(iBS_iBSCmdDestDir)/*.py

	if test ! -d $(iBS_iBSCmdDestDir) ; \
	then \
		mkdir -p $(iBS_iBSCmdDestDir) ; \
		chmod a+rx $(iBS_iBSCmdDestDir) ; \
	fi

	$(iBS_pythonBin) $(iBS_cmdtoolInstall)

	rm -rf $(iBS_iBSRDestDir)
	$(call mkdir,$(iBS_iBSRDestDir), -p)
	cp -rf $(iBS_iBSRSrcDir)/* $(iBS_iBSRDestDir)

	rm -rf $(iBS_iBSConfigDestDir)
	$(call mkdir,$(iBS_iBSConfigDestDir), -p)
	cp -rf $(iBS_iBSConfigSrcDir)/* $(iBS_iBSConfigDestDir)
