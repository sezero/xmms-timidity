libdir = $(XMMS_INPUT_PLUGIN_DIR)

lib_LTLIBRARIES = libtimidity.la

libtimidity_la_SOURCES = \
	callbacks.c \
	callbacks.h \
	interface.c \
	interface.h \
	xmms-timidity.c \
	xmms-timidity.h

INCLUDES = \
	   -I$(top_srcdir) \
	   $(XMMS_CFLAGS) \
	   $(LIBTIMIDITY_CFLAGS)

libtimidity_la_LIBADD = $(XMMS_LIBS) $(LIBTIMIDITY_LIBS)
libtimidity_la_LDFLAGS = -module -avoid-version \
		    -export-symbols-regex "get_.plugin_info"
