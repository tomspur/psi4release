SHELL = /bin/sh

MAKE_MADNESS=1

.PHONY:	default all install install_man install_inc install_host depend clean dclean targetclean tests testsclean doc

default: all

ifeq ($(MAKE_MADNESS),1)
  subdirs = madness lib include src
else
  subdirs = lib include src
endif

top_srcdir = /Users/masaaki/psi4
srcdir = /Users/masaaki/psi4
datarootdir = ${prefix}/share
VPATH = /Users/masaaki/psi4
top_objdir = .

prefix = /usr/local/psi4
scriptdir = $(prefix)/bin
MKDIRS = $(top_srcdir)/bin/mkdirs.sh
INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL}

all:
	for dir in $(subdirs); \
          do \
            (cd $${dir}; echo Making in $${dir}; $(MAKE) all) || exit 1; \
          done

install: install_host
	for dir in $(subdirs); \
          do \
            (cd $${dir}; echo Making install in $${dir}; $(MAKE) install) || exit 1; \
          done

install_inc:
	for dir in $(subdirs); \
          do \
            (cd $${dir}; echo Making install in $${dir}; $(MAKE) install_inc) || exit 1; \
          done

install_man:
	for dir in $(subdirs); \
          do \
            (cd $${dir}; echo Making install in $${dir}; $(MAKE) install_man) || exit 1; \
          done

depend:
	for dir in $(subdirs); \
          do \
            (cd $${dir}; echo Making depend in $${dir}; $(MAKE) depend) || exit 1; \
          done

clean:
	for dir in $(subdirs); \
          do \
            (cd $${dir}; echo Making clean in $${dir}; $(MAKE) clean) || exit 1; \
          done

dclean:
	for dir in $(subdirs); \
          do \
            (cd $${dir}; echo Making dclean in $${dir}; $(MAKE) dclean) || exit 1; \
          done

targetclean:
	for dir in $(subdirs) tests doc; \
          do \
            (cd $${dir}; echo Making clean in $${dir}; $(MAKE) targetclean) || exit 1; \
          done

tests:
	(cd tests; echo Running test suite...; $(MAKE)) || exit 1;

testsclean:
	(cd tests; echo Cleaning test suite...; $(MAKE) clean) || exit 1;

doc:
	(cd doc; echo Building documentation...; $(MAKE)) || exit 1;

$(top_srcdir)/configure: $(top_srcdir)/configure.ac $(top_srcdir)/aclocal.m4
	cd $(top_srcdir) && autoconf

$(top_objdir)/config.status: $(top_srcdir)/configure
	cd $(top_objdir) && ./config.status --recheck

Makefile: $(srcdir)/Makefile.in $(top_objdir)/config.status
	cd $(top_objdir) && CONFIG_FILES=Makefile ./config.status
