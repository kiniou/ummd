##
 #  ummd ( Micro MultiMedia Daemon )
 ##

SUBDIRS = controls filters sources targets

noinst_LTLIBRARIES = libcore.la

libcore_la_LIBADD = \
	controls/libcontrols.la \
	filters/libfilters.la \
	sources/libsources.la \
	targets/libtargets.la

libcore_la_SOURCES = \
	main.c \
	ports.c \
	wirings.c
