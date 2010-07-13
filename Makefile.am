pkgdatadir = @datadir@/@PACKAGE@

dosstuff = \
	makedebug.bat \
	makerelease.bat

noinst_DATA = \
	coding-standards.html \
	devguide-CELESTIA.txt \
	MergingCelestia \
	$(dosstuff)

pkgdata_DATA = \
	celestia.cfg \
	COPYING 

EXTRA_DIST = $(pkgdata_DATA) \
	$(noinst_DATA) \
	po/extract_resource_strings.pl \
	po/translate_resources.pl \
	TRANSLATORS-CELESTIA

SUBDIRS = src po locale merge


MAINTAINERCLEANFILES = subdirs configure.in 

NONAUTO = thirdparty doc

dist-hook:
	( \
	  cd $(srcdir) && \
	  ( \
	    for dir in $(NONAUTO) ; do \
		find $$dir -depth -print | egrep -v '(~$$)|(/CVS)|(/\.svn)|(/\.#)|(/\.deps)' \
	        | cpio -pdum $(distdir) 2>/dev/null ; \
	    done \
	  ) \
	)

