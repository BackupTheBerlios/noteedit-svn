# create a static library with libtool
noinst_LTLIBRARIES = libkguitar_excerpt.la

METASOURCES = AUTO

libkguitar_excerpt_la_LDFLAGS = $(all_libraries)

INCLUDES = $(all_includes) $(TSE3INCLUDE)

libkguitar_excerpt_la_SOURCES = chord.cpp chordlist.cpp chordlistitem.cpp fingerlist.cpp \
                                fingers.cpp tabcolumn.cpp tabtrack.cpp

