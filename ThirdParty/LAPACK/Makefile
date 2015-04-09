bdt_topdir	= ../..
include $(bdt_topdir)/config/Make.rules
install_dir=$(BDT_Install)/ThirdParty
dir_lapack=lapack-3.5.0
SUBDIRS		= $(dir_lapack)
CLEAN_SUBDIRS	=$(dir_lapack)

.PHONY: $(SUBDIRS)

clean::
	cd $(dir_lapack) && $(MAKE) clean && rm -f *.a

install::$(SUBDIRS)

$(dir_lapack)::
	echo "making $@ in $(dir_lapack)";
	cd $(dir_lapack) && $(MAKE) blaslib && $(MAKE) lapacklib