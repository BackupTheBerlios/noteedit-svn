# this has all of the subdirectories that make will recurse into.  if
# there are none, comment this out
SUBDIRS = resources icons

lib_LTLIBRARIES = libnoteedit.la


# this is the program that gets installed.  it's name is used for all
# of the other Makefile.am variables
bin_PROGRAMS = noteedit

libnoteedit_la_SOURCES = abcexport.cpp abcexportform.ui \
	mididevicelistbox.cpp chord.cpp clef.cpp \
	configuredialog.cpp chorddiagram.cpp \
	dbufferwidget.cpp exports.ui expWarn.ui \
	filehandler.cpp filter.ui \
	generProps.ui grammar.yy \
	keyoffs.cpp keysig.cpp layout.cpp \
	lexik.ll lilyexport.cpp lilypondexportform.ui \
	lilytest.cpp lines.ui listSel.ui lyrics.ui \
	mainframewidget2.cpp mainframewidget.cpp metronom.ui \
	midiexport.cpp midiexportform.ui midimapper.cpp \
	midi.ui mupWarning.ui muselement.cpp musicxmlexportform.ui \
	internals.cpp multistaffinfo.cpp \
	musicxmlcommon.cpp musicxmlimport.cpp musicxmlexport.cpp \
	musixtex.cpp musixtexexportform.ui miditimescale.cpp \
	noteedit_part.cpp notesel.cpp numberdisplay.cpp \
	outputbox.cpp pmxexportform.ui \
	pmxexport.cpp saveparametersform.ui \
	scaleedit.ui resource.cpp rest.cpp \
	scaleEd.ui ScoreEditor.cpp scoreinfo.cpp sign.cpp \
	smRest.ui staff.cpp staffElem.ui staffProps.ui staffSel.ui \
	staffPropFrm.cpp text.cpp textDialog.ui textDialog_impl.cpp \
	tempotrack.cpp timesig.cpp timesigDia.ui tip.ui \
	transpainter.cpp tse3handler.cpp tse3Info.ui tupletdialog.ui \
	tupletdialog_impl.cpp scaleedit_impl.cpp uiconnect.cpp \
	voice.cpp voicedialog.cpp voiceDia.ui volume.ui \
	zoomselection.cpp 

libnoteedit_la_LDFLAGS = $(all_libraries) -version-info 1:0:1 -module

# the libraries to link against.
libnoteedit_la_LIBADD   = ../kguitar_excerpt/libkguitar_excerpt.la $(LIB_KFILE) $(TSE3LIBS) $(KMIDLIBS) -lkparts -lkdeprint

# set the include path for X, qt and KDE
INCLUDES = $(all_includes) $(TSE3INCLUDE) $(KMIDINCLUDE) $(ANTHEMINCLUDE)

# we want exceptions
# KDE_CXXFLAGS = $(USE_EXCEPTIONS)
# no we won't use exceptions except "midimapper.cpp"

noteedit_LDFLAGS = $(KDE_RPATH)
noteedit_SOURCES = main.cpp
noteedit_LDADD   = libnoteedit.la 
noteedit_YFLAGS = -p$(LEX_PREFIX)

LEX_PREFIX=@LEX_PREFIX@
LEX_RESTART=@LEX_RESTART@
LEX_LINENO=@LEX_LINENO@
YACC_PARSE=@YACC_PARSE@
YACC_LVAL=@YACC_LVAL@
AM_YFLAGS = -p$(LEX_PREFIX)

PREFIX_DEF=-DYYLINENO=$(LEX_LINENO) -DYYRESTART=$(LEX_RESTART) -DYYPARSE=$(YACC_PARSE) -DYYLVAL=$(YACC_LVAL)
KDE_CXXFLAGS = @SPECIAL_FLAGS_FOR_FINAL@

noinst_HEADERS = mididevicelistbox.h \
	abcexportform.h aboutinfo.h \
	chord.h clef.h configuredefaultvalues.h configuredialog.h \
	dbufferwidget.h \
	exports.h expWarn.h \
	filehandler.h filter.h \
	generProps.h grammar.h \
	keyoffs.h keysig.h lilypondexportform.h \
	lilyexport.h lilytest.h lines.h listSel.h lyrics.h \
	mainframewidget.h metronom.h midiexport.h midi.h midimapper.h \
	mupWarning.h muselement.h internals.h musicxmlexportform.h \
	musixtexexportform.h musixtex.h miditimescale.h \
	noteedit_part.h notesel.h numberdisplay.h \
	outputbox.h midiexportform.h \
	parsertypes.h pmxexport.h pmxexportform.h \
	text.h resource.h rest.h saveparametersform.h \
	scaleEd.h ScoreEditor.h scoreinfo.h sign.h smRest.h \
	staffElem.h staff.h staffProps.h staffSel.h \
	textDialog_impl.h timesigDia.h timesig.h tip.h transpainter.h tse3handler.h tse3Info.h \
	uiconnect.h \
	voiceDia.h voicedialog.h voice.h volume.h \
	zoomselection.h

tipsdir = $(kde_datadir)/noteedit
tips_DATA = tips
# let automoc handle all of the meta source files (moc)
METASOURCES = AUTO

messages: rc.cpp
	$(PREPARETIPS) > tips.cpp
	$(XGETTEXT) *.cpp ../kguitar_excerpt/*.cpp -o $(podir)/noteedit.pot
	rm -f tips.cpp

# this is where the desktop file will go
desktopdir   = $(kde_appsdir)/Utilities
desktop_DATA = noteedit.desktop

# this is where the XML-UI file will go
xmluidir = $(kde_datadir)/noteedit
xmlui_DATA = noteeditui.rc noteedit_part.rc


.yy.cc:
	$(YACC) $(AM_YFLAGS) $(YFLAGS) $<
	sed '/extern *char *\*getenv *( *) *;/d' y.tab.c > y.tab1.c
	sed '/__attribute__ *( *( *__unused__ *) *)/d' y.tab1.c > $@
	rm y.tab.c y.tab1.c
	if test -f y.tab.h; then \
	if cmp -s y.tab.h $*.h; then rm -f y.tab.h; else mv y.tab.h $*.h; fi; \
	else :; fi

.ll.cc:
	$(LEX) -P$(LEX_PREFIX) -olex.yy.c $<
	rm -f $@ lex.yy1.c
	sed '/extern *int *isatty/d' lex.yy.c > lex.yy1.c
	sed '/yy_state_buf.*= *yyalloc/s/yyalloc/(yy_state_type*) yyalloc/' lex.yy1.c > $@
	rm lex.yy.c lex.yy1.c

filehandler.lo: filehandler.cpp
	$(LTCXXCOMPILE) $(PREFIX_DEF) -c $<

grammar.h: grammar.yy

grammar.lo: grammar.cc muselement.h resource.h staff.h rest.h sign.h chord.h chorddiagram.h grammar.h
	$(LTCXXCOMPILE) $(PREFIX_DEF) -c $<

lexik.lo: lexik.cc grammar.cc  # because with "grammar.cc" "grammar.h" is built
	$(LTCXXCOMPILE) $(PREFIX_DEF) -c $<

