#ifndef PARSER_TYPES_H

#define PARSER_TYPES_H
#include "mainframewidget.h"
#include "layout.h"

/* The font sizes MUST be different. They are used as keys */
#define SC_TITLE_FONT_SIZE 20
#define SC_SUBTITLE_FONT_SIZE 12
#define SC_AUTHOR_FONT_SIZE 11
#define SC_LAST_AUTHOR_FONT_SIZE 10
#define SC_COMMENT_FONT_SIZE 6
#define SC_COPYRIGHT_FONT_SIZE 13

struct parser_param_str {
	const char *fname;
	NMainFrameWidget *mainWidget;
	QPtrList<NStaff> *newStaffs;
	QPtrList<NVoice> *newVoices;
	QString scTitle_, scSubtitle_, scAuthor_, scLastAuthor_, scCopyright_, scComment_;
	bool enableParams, with_measnum;
	int paperwidth, paperheight;
	QPtrList<layoutDef> bracketList;
	QPtrList<layoutDef> braceList;
	QPtrList<layoutDef> contList;
};

struct property_descr_str {
	int octavmodi;
	int offs, slurdist;
	property_type properties;
	unsigned int beamstatus;
};

extern parser_param_str parser_params;
extern int yylineno;
int yyparse();
void init_parser_variables();
void cleanup_parser_variables();

#define PROG_CHORUS 93
#define PROG_REVERB 91
#define PROG_VOL     7
#define PROG_PAN    10


#endif /* PARSER_TYPES_H */
