/****************************************************************************************/
/*											*/
/* This program is free software; you can redistribute it and/or modify it under the	*/
/* terms of the GNU General Public License as published by the Free Software		*/
/* Foundation; either version 2 of the License, or (at your option) any later version.	*/
/*											*/
/* This program is distributed in the hope that it will be useful, but WITHOUT ANY	*/
/* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A	*/
/* PARTICULAR PURPOSE. See the GNU General Public License for more details.		*/
/*											*/
/* You should have received a copy of the GNU General Public License along with this	*/
/* program; (See "LICENSE.GPL"). If not, write to the Free Software Foundation, Inc.,	*/
/* 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.				*/
/*											*/
/*--------------------------------------------------------------------------------------*/
/*											*/
/*		Joerg Anders, TU Chemnitz, Fakultaet fuer Informatik, GERMANY		*/
/*		ja@informatik.tu-chemnitz.de						*/
/*											*/
/*											*/
/****************************************************************************************/

%{
#include <stdio.h>
#include <string.h>
#include "muselement.h"
#include "parsertypes.h"
#include "grammar.hh"
int chordname_expected = 0;
static int produce_strings = 0;
static int in_grids = 0;
static const char *special_comments[] = { "underlength", "overlength", "lyricsdist", "playtransposd",
				     "yrestoffs", "stempolicy", "irregular", 0};
static bool is_special_comment(const char *s);
%}


accelerando	accel\.
number		(\+|-)?[0-9][0-9]*
rnumber		{number}\.{number}?
pitch		c|d|e|f|g|a|b
chordname	\"_*[C|D|E|F|G|A|B|H][^\"]*\"
y_size_14	\(14\)
sdes		(([0-9][0-9]*)|x|o)
str		\"{sdes}\ {sdes}\ {sdes}\ {sdes}\ {sdes}\ {sdes}\"
keykind		{number}(#|&)
string		(\"([^\n\"]|(\\\"))*[^\\]\")|(\"\")
irregulaer	\/\/[ \t]*irregular
overlength	\/\/[ \t]*overlength
underlength	\/\/[ \t]*underlength
lyricsdist	\/\/[ \t]*lyricsdist
playtransposd	\/\/[ \t]*playtransposd
yrestoffs	\/\/[ \t]*yrestoffs
ystempolicy	\/\/[ \t]*stempolicy
fermat		\\\(ferm\)
sign		\\\(sign\)
coda		\\\(coda\)
dalsegno	D\.S\.
dalsegalcoda	D\.S\.\ *al\ *\\\(coda\)
dalsegalfine	D\.S\.\ *al\ *Fine
sm4n		\\\(sm4n\)
comment		\/\/[^\n]*\n
ritardando	ritard\.
octavbassa	\"8va\ *bassa\"

%%
1\/2		{return Y_1_2;}
\"8va\"		{return Y_8VA;}
{octavbassa}	{return Y_8VA_BASSA;}
{accelerando}	{return Y_ACCELERANDO;}
above		{return Y_ABOVE;}
all		{return Y_ALL;}
bar		{return Y_BAR;}
barstyle	{return Y_BARSTYLE;}
bcr		{return Y_CROSS;}
bcr2		{return Y_CROSS2;}
bcrc		{return Y_CIRCLE_CROSS;}
brec		{return Y_RECT;}
rehearsal	{produce_strings = 1; return Y_REHEARSAL;}
btr		{return Y_TRIA;}
below		{return Y_BELOW;}
brace 		{return Y_BRACE;}
bracket 	{return Y_BRACKET;}
bm		{return Y_BM;}
bold		{return Y_BOLD;}
boldital	{produce_strings = 1; return Y_BOLDITAL;}
channel		{return Y_CHANNEL;}
chord		{chordname_expected = 1; return Y_CHORD;}
{str}		{if (in_grids) {
			yytext[strlen(yytext)-1] = '\0';
                        YYLVAL.string = strdup(yytext+1);
			return Y_STRING_DESCR;
		 }
		 REJECT;
		}
{chordname}	{if (chordname_expected) {
			yytext[strlen(yytext)-1] = '\0';
                        YYLVAL.string = strdup(yytext+1);
			return Y_CHORDNAME;
		 }
		 REJECT;
		}
clef		{return Y_CLEF;}
clefchange	{return Y_CLEFCHANGE;}
cm		{return Y_CM;}
treble		{YYLVAL.clefkind = TREBLE_CLEF; return Y_CLEFKIND;}
bass		{YYLVAL.clefkind = BASS_CLEF; return Y_CLEFKIND;}
soprano		{YYLVAL.clefkind = SOPRANO_CLEF; return Y_CLEFKIND;}
alto		{YYLVAL.clefkind = ALTO_CLEF; return Y_CLEFKIND;}
tenor		{YYLVAL.clefkind = TENOR_CLEF; return Y_CLEFKIND;}
{coda}		{return Y_CODA;}
dblbar		{return Y_DBLBAR;}
drum		{YYLVAL.clefkind = DRUM_CLEF; return Y_CLEFKIND;}
drum_bass	{YYLVAL.clefkind = DRUM_BASS_CLEF; return Y_CLEFKIND;}
{dalsegno}	{return Y_DAL_SEGNO;}
{dalsegalcoda}	{return Y_DAL_SEGNO_AL_CODA;}
{dalsegalfine}	{return Y_DAL_SEGNO_AL_FINE;}
defoct		{return Y_DEFOCT;}
down		{return Y_DOWN;}
ebm		{return Y_EBM;}
endbar		{return Y_ENDBAR;}
endending	{return Y_ENDENDING;}
ending		{produce_strings = 1; return Y_ENDING;}
Fine		{return Y_FINE;}
{fermat}	{return Y_FERMATA;}
footer		{return Y_FOOTER;}
grace		{return Y_GRACE;}
grids		{chordname_expected = in_grids = 1; return Y_GRIDS;}
gridfret	{return Y_GRIDFRET;}
gridswhereused  {return Y_GRIDSWHEREUSED;}
header		{chordname_expected = in_grids = 0; return Y_HEADER;}
inches		{return Y_INCHES;}
{irregulaer}	{return Y_IRREGULAER_KEY;}
key		{return Y_KEY;}
keychange	{return Y_KEYCHANGE;}
{keykind}	{if (strchr(yytext, '#')) YYLVAL.key.kind = PROP_CROSS;
		 else                    YYLVAL.key.kind = PROP_FLAT;
		 sscanf(yytext, "%d", &YYLVAL.key.count);
		 return Y_KEYKIND;
		}
label		{produce_strings = 1; return Y_LABEL;}
lyrics		{produce_strings = 1; return Y_LYRICS;}
{lyricsdist}	{return Y_LYRICSDIST;}
\"lonelytrill\"	{return Y_LONELY_TRILL;}
\"~\"		{return Y_LONELY_TRILL;}
midi		{return Y_MIDI;}
measnum	{return Y_MEASNUM;}
music		{chordname_expected = in_grids = 0; return Y_MUSIC;}
mussym		{return Y_MUSSYM;}
multirest	{return Y_MULTIREST;}
{number}	{sscanf(yytext, "%d", &(YYLVAL.num)); 
		 return Y_NUMBER;
		}
{rnumber}	{sscanf(yytext, "%lf", &(YYLVAL.dnum));
		 return Y_REAL_NUMBER;
		}
octave		{return Y_OCTAVE;}
{overlength}	{return Y_OVERLENGTH;}
pagewidth	{return Y_PAGEWIDTH;}
pageheight	{return Y_PAGEHEIGHT;}
parameter	{return Y_PARAMETER;}
pedal		{return Y_PEDAL;}
pedstar		{return Y_PEDSTAR;}
pedstyle	{return Y_PEDSTYLE;}
phrase		{return Y_PHRASE;}
program		{return Y_PROGRAM;}
{pitch}		{YYLVAL.pitch = yytext[0];
		 return Y_PITCH;
		}
{playtransposd} {return Y_PLAYTRANSPOSD;}
repeatboth	{return Y_REPEATBOTH;}
repeatend	{return Y_REPEATEND;}
repeatstart	{return Y_REPEATSTART;}
{yrestoffs}	{return Y_RESTOFFS;}
{ritardando}	{return Y_RITARDANDO;}
rom		{return Y_ROM;}
{y_size_14}	{return Y_SIZE_14;}
roll		{return Y_ROLL;}
slash		{return Y_SLASH;}
score		{chordname_expected = in_grids = 0; return Y_SCORE;}
{sign}		{return Y_SIGN;}
{sm4n}		{return Y_SM4N;}
staffs		{chordname_expected = in_grids = 0; return Y_STAFFS;}
staff		{return Y_STAFF;}
{string}	{if (produce_strings) {
			produce_strings = 0;
			yytext[strlen(yytext)-1] = '\0';
			YYLVAL.string = strdup(yytext+1);
			return Y_STRING;
		 }
		 REJECT;
		}
{number}/([ \t]*:)	{sscanf(yytext, "%d", &(YYLVAL.num));
		 return Y_STAFF_NUMBER;
		}
{number}[ \t]+{number}/([ \t]*:)	{sscanf(yytext, "%d %d", &(YYLVAL.numnum.staff), &(YYLVAL.numnum.voice));
		 return Y_STAFF_VOICE_NUMBER;
		}
{ystempolicy}	{return Y_STEMPOLICY;}
stemup		{return Y_STEMUP;}
stemdown	{return Y_STEMDOWN;}
tempo		{return Y_TEMPO;}
til		{return Y_TIL;}
time		{return Y_TIME;}
title		{produce_strings = 1; return Y_TITLE;}
\"tr\"		{return Y_TRILL;}
{underlength}	{return Y_UNDERLENGTH;}
units		{return Y_UNITS;}
up		{return Y_UP;}
vscheme		{return Y_VSCHEME;}
with		{return Y_WITH;}
{comment}	{if (is_special_comment(yytext)) {
			yylineno--;
			REJECT;
		 }
		 else {
			break;
		 }
		}

[ \n\t\r]+	break;
.		{return yytext[0];}
%%

int yywrap() {return 1;}

static bool is_special_comment(const char *s) {
	const char **cptr;

	for (cptr = special_comments; *cptr; *cptr++) {
		if (strstr(s, *cptr)) return true;
	}
	return false;
}

void enable_strings() {
	produce_strings = 1;
}

#ifdef LEXDEBUG
int main(int argc, char *argv[]) {
	extern FILE *yyin;
	int t;

	if (argc != 2) {
		fprintf(stderr, "Benutzung: %s <file>\n", argv[0]);
		exit(-1);
	}

	if ((yyin = fopen(argv[1], "r")) == NULL) {
		fprintf(stderr, "Fehler bei der Eroeffnung von %s\n", argv[1]);
		exit(-1);
	}
	do {
	t = yylex();
	switch (t) {
		case Y_ABOVE: printf("Y_ABOVE\n"); break;
		case Y_ALL: printf("Y_ALL\n"); break;
		case Y_BAR: printf("Y_BAR\n"); break;
		case Y_BELOW: printf("Y_BELOW\n"); break;
		case Y_BM: printf("Y_BM\n"); break;
		case Y_BOLD: printf("Y_BOLD\n"); break;
		case Y_BOLDITAL: printf("Y_BOLDITAL\n"); break;
		case Y_CHANNEL: printf("Y_CHANNEL\n"); break;
		case Y_CLEF: printf("Y_CLEF\n"); break;
		case Y_CLEFKIND: printf("Y_CLEFKIND\n"); break;
		case Y_DEFOCT: printf("Y_DEFOCT\n"); break;
		case Y_DOWN: printf("Y_DOWN\n"); break;
		case Y_EBM: printf("Y_EBM\n"); break;
		case Y_ENDENDING: printf("Y_ENDENDING\n"); break;
		case Y_ENDING: printf("Y_ENDING\n"); break;
		case Y_FERMATA: printf("Y_FERMATA\n"); break;
		case Y_HEADER: printf("Y_HEADER\n"); break;
		case Y_IRREGULAER_KEY: printf("Y_IRREGULAER_KEY\n"); break;
		case Y_KEY: printf("Y_KEY\n"); break;
		case Y_KEYKIND: printf("Y_KEYKIND\n"); break;
		case Y_LABEL: printf("Y_LABEL\n"); break;
		case Y_LYRICS: printf("Y_LYRICS\n"); break;
		case Y_LYRICSDIST: printf("Y_LYRICSDIST\n"); break;
		case Y_MIDI: printf("Y_MIDI\n"); break;
		case Y_MUSIC: printf("Y_MUSIC\n"); break;
		case Y_NUMBER: printf("Y_NUMBER\n"); break;
		case Y_OVERLENGTH: printf("Y_OVERLENGTH\n"); break;
		case Y_PARAMETER: printf("Y_PARAMETER\n"); break;
		case Y_PROGRAM: printf("Y_PROGRAM\n"); break;
		case Y_PITCH: printf("Y_PITCH\n"); break;
		case Y_REPEATEND: printf("Y_REPEATEND\n"); break;
		case Y_REPEATSTART: printf("Y_REPEATSTART\n"); break;
		case Y_ROM: printf("Y_ROM\n"); break;
		case Y_SCORE: printf("Y_SCORE\n"); break;
		case Y_SM4N: printf("Y_SM4N\n"); break;
		case Y_STAFF: printf("Y_STAFF\n"); break;
		case Y_STAFFS: printf("Y_STAFFS\n"); break;
		case Y_STAFF_NUMBER: printf("Y_STAFF_NUMBER\n"); break;
		case Y_STRING: printf("Y_STRING\n"); break;
		case Y_TEMPO: printf("Y_TEMPO\n"); break;
		case Y_TIME: printf("Y_TIME\n"); break;
		case Y_TITLE: printf("Y_TITLE\n"); break;
		case Y_UNDERLENGTH: printf("Y_UNDERLENGTH\n"); break;
		case Y_UP: printf("Y_UP\n"); break;
		case Y_WITH: printf("Y_WITH\n"); break;
	default: printf("Zeichen: %c(0x%x)\n", t, t);
	}
	}
	while (t);
	return 0;
}
#endif

