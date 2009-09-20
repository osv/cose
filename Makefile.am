pkgdatadir = @datadir@/@PACKAGE@

dosstuff = \
	celestia.mak \
	celestia.iss \
	celestia.sln \
	celestia.vcproj \
	makedebug.bat \
	makerelease.bat

noinst_DATA = \
	CelestiaKeyAssignments.txt \
	coding-standards.html \
	devguide.txt \
	KbdMouseJoyControls.txt \
	MergingCelestia \
	$(dosstuff)

pkgdata_DATA = \
	celestia.cfg \
	controls.txt \
	COPYING 

EXTRA_DIST = $(pkgdata_DATA) \
	$(noinst_DATA) \
	splash.png \
	po/extract_resource_strings.pl \
	po/translate_resources.pl \
	TRANSLATORS-CELESTIA

splashdir = $(pkgdatadir)/splash
splash_DATA = splash.png

SUBDIRS = src po locale merge


MAINTAINERCLEANFILES = subdirs configure.in 

NONAUTO = thirdparty

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

