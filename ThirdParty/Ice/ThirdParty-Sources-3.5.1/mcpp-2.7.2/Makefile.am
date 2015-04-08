## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

SUBDIRS = src

if REPLACE_CPP
SUBDIRS += .
man_page = mcpp-gcc
else
man_page = mcpp
endif
man1_MANS = $(man_page).1

EXTRA_DIST = $(top_srcdir)/LICENSE $(top_srcdir)/INSTALL*

if ! REPLACE_CPP 
install-exec-hook:
	$(top_srcdir)/config/install-sh -d "$(DESTDIR)$(prefix)/share/doc/mcpp"
uninstall-hook:
	rm -rf $(DESTDIR)$(prefix)/share/doc/mcpp
endif

