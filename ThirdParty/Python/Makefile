bdt_topdir	= ../..
include $(bdt_topdir)/config/Make.rules
install_dir=$(BDT_Install)/ThirdParty
dir_python=Python-3.3.3
SUBDIRS		= $(dir_python)
CLEAN_SUBDIRS	=$(dir_python)

.PHONY: $(SUBDIRS)

clean::
	@for subdir in $(CLEAN_SUBDIRS); \
	do \
	    echo "making clean in $$subdir"; \
	    ( cd $$subdir && $(MAKE) clean ) || exit 1; \
	done

install::$(SUBDIRS)

UNAME := $(shell uname)
ifeq ($(UNAME),Darwin)
$(dir_python)::
	echo "making $@ in $(dir_python)";
	if test ! -d $(install_dir)/python ; \
	then \
		echo "Creating $(install_dir)/python..." ; \
		mkdir -p $(install_dir)/python ; \
		chmod a+rx $(install_dir)/python ; \
	fi
	cd $(dir_python) && ./configure CFLAGS="-arch x86_64 -fPIC" LDFLAGS="-arch x86_64" --with-universal-archs=64-bit --enable-shared --prefix=$(install_dir)/python && $(MAKE) install
	rm -f $(install_dir)/python/lib/python3.3/site-packages/Ice.pth
	echo "$(install_dir)/Ice-3.5.1/python" > "$(install_dir)/python/lib/python3.3/site-packages/Ice.pth"
	rm -f $(install_dir)/python/lib/python3.3/site-packages/bdtPy.pth
	echo "$(BDT_Install)/bdt/bdtPy" > "$(install_dir)/python/lib/python3.3/site-packages/bdtPy.pth"
else
$(dir_python)::
	echo "making $@ in $(dir_python)";
	if test ! -d $(install_dir)/python ; \
	then \
		echo "Creating $(install_dir)/python..." ; \
		mkdir -p $(install_dir)/python ; \
		chmod a+rx $(install_dir)/python ; \
	fi
	cd $(dir_python) && ./configure CFLAGS=-fPIC --prefix=$(install_dir)/python && $(MAKE) install
	rm -f $(install_dir)/python/lib/python3.3/site-packages/Ice.pth
	echo "$(install_dir)/Ice-3.5.1/python" > "$(install_dir)/python/lib/python3.3/site-packages/Ice.pth"
	rm -f $(install_dir)/python/lib/python3.3/site-packages/bdtPy.pth
	echo "$(BDT_Install)/bdt/bdtPy" > "$(install_dir)/python/lib/python3.3/site-packages/bdtPy.pth"
endif


