bdt_topdir	= ../../../
include $(bdt_topdir)/config/Make.rules
install_dir=$(BDT_Install)
dir_bzip2=bzip2-1.0.6
dir_db=db-5.3.21.NC
dir_mcpp=mcpp-2.7.2
SUBDIRS		= $(dir_bzip2) $(dir_mcpp) $(dir_db)

CLEAN_SUBDIRS	=$(dir_bzip2) $(dir_mcpp) $(dir_db)/build_unix

.PHONY: $(SUBDIRS)

clean::
	@for subdir in $(CLEAN_SUBDIRS); \
	do \
	    echo "making clean in $$subdir"; \
	    ( cd $$subdir && $(MAKE) clean ) || exit 1; \
	done

install::$(SUBDIRS)

$(dir_bzip2)::
	echo "making $@ in $(dir_bzip2)";
	if test ! -d $(install_dir)/bzip2 ; \
	then \
		echo "Creating $(install_dir)/bzip2..." ; \
		mkdir -p $(install_dir)/bzip2 ; \
		chmod a+rx $(install_dir)/bzip2 ; \
	fi
	cd $(dir_bzip2) && $(MAKE) install PREFIX=$(install_dir)/bzip2
	if test ! -d $(install_dir)/bzip2/lib64 ; \
	then \
	    cd $(install_dir)/bzip2 && ln -s lib lib64 ; \
	fi

$(dir_mcpp)::
	echo "making $@ in $(dir_mcpp)";
	if test ! -d $(install_dir)/mcpp ; \
	then \
		echo "Creating $(install_dir)/mcpp..." ; \
		mkdir -p $(install_dir)/mcpp ; \
		chmod a+rx $(install_dir)/mcpp ; \
	fi
	cd $(dir_mcpp) && ./configure CFLAGS=-fPIC --enable-mcpplib --disable-shared --prefix=$(install_dir)/mcpp && $(MAKE) install
	if test ! -d $(install_dir)/mcpp/lib64 ; \
	then \
	    cd $(install_dir)/mcpp && ln -s lib lib64 ; \
	fi

$(dir_db)::
	echo "making $@ in $(dir_db)";
	if test ! -d $(install_dir)/db ; \
	then \
		echo "Creating $(install_dir)/db..." ; \
		mkdir -p $(install_dir)/db ; \
		chmod a+rx $(install_dir)/db ; \
	fi
	cd $(dir_db)/build_unix && ../dist/configure CFLAGS=-fPIC --enable-cxx --prefix=$(install_dir)/db && $(MAKE) install
	if test ! -d $(install_dir)/db/lib64 ; \
	then \
	    cd $(install_dir)/db && ln -s lib lib64 ; \
	fi
