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
#include <stdlib.h>
#include "parsertypes.h"
/* #define YYDEBUG 1 */
#ifndef YACCDEBUG
#include <qptrlist.h>
#include "muselement.h"
#include "resource.h"
#include "staff.h"
#include "rest.h"
#include "sign.h"
#include "chord.h"
#include "chorddiagram.h"
extern int yylex(), yyparse();
#endif
parser_param_str parser_params;
static char Str[128];
static NStaff *current_staff;
static NVoice *current_voice;
static int i, voiceNumber, staffCount, voiceCount;
static int currentStaffNr;
static int current_modificators;
static int pending_volume = -1;
static int num = 4, denom = 4, countof128 = 128;
static int braces = 0;
static int miditime;
static int newtimesig = 0;
static int before_first_music = 1;
static QString newLine("\n");
static QString newLineLiterally("\\n");
static QPtrList<chordDiagramName> chordDiagramList;
extern int chordname_expected;
extern void enable_strings();
struct slur_stack_str {
	NChord *slurstart_chord;
	int slurdist;
};
static struct pendings {
	int newkeysig, newclef;
	NMusElement *beamstart_chord;
	NMusElement *lastBarSym;
	QPtrList<slur_stack_str> *slur_stack;
	int inbeam;
	NVoice *theVoice;
} *pending_elements = 0;
#define NO_BEAM     0
#define BEAM_START (1 << 0)
#define BEAM_END   (1 << 1)
#define INTERIM_STOKE (1 << 21)
static void insert_new_clefs_timesigs_and_keys();
static int select_voice_and_staff(unsigned int staff_nr, unsigned int voice_nr);
static void append_barsym_in_all_voices_and_reset_keysig(int bartype, int repcount);
static int insert_volume_marker(int volTime, char *volstring, int volume);
static void set_score_information(int key, char *value);
static void remove_newlines(QString *s);
static void insert_multirest(int multirestlength);
static void setStaffPropsMeasureLength(int mlength);
static void MUP2lyrics(char *s);
static void  insertChordDiagramm(unsigned int at, char *chordname, NMusElement *last_bar_sym);
static void insert_segno_accel_and_ritard(unsigned int at, int type, NMusElement *last_bar_sym);
static void update_voice_stack(int voice_nr, int incr);
void cleanup_parser_variables();
void init_parser_variables();
static int current_string_placement;
void yyerror(const char *s);
%}


%union {
	struct {
		int count;
		char kind;
	} key;
	struct {
		int length;
		unsigned int properties;
	} length;
	struct {
		int line;
		int offs;
		unsigned int properties;
		unsigned int beamstatus;
		int slurdist;
	} noteparam;
	struct property_descr_str propertydescr;
	struct {
		unsigned int beamstatus;
		unsigned int slurdist;
		union {
			NMusElement *muselem;
			NChord *chord;
		} element;
	} muselemdescr;
	struct {
		NMusElement *first, *last;
	} tupletdescr;
	struct {
		int staff, voice;
	} numnum;
	struct {
		int type;
#define INCORRECT_TRIPLET 1
#define CORRECT_TRIPLET 2
#define CORRECT_TRIPLET_WITH_DOT 3
		int numNotes, length, playtime;
	} tupletdescr2;
	unsigned int modidescr;
	struct {
		unsigned int beats;
		unsigned int measures;
	} shifted_miditime;
	char pitch;
	char *string;
	int num;
	double dnum;
	int clefkind;
	int symbolkind;
	bool boolval;
}

%token <num> Y_1_2
%token Y_8VA
%token Y_8VA_BASSA
%token Y_ACCELERANDO
%token Y_ABOVE
%token Y_ALL
%token Y_BAR
%token Y_BARSTYLE
%token Y_BELOW
%token Y_BM
%token Y_BOLD
%token Y_BOLDITAL
%token Y_BRACE
%token Y_BRACKET
%token Y_CHANNEL
%token Y_CHORD
%token Y_CODA
%token <string> Y_CHORDNAME
%token Y_CIRCLE_CROSS
%token Y_CLEF
%token Y_CLEFCHANGE
%token Y_CM
%token <clefkind> Y_CLEFKIND
%token Y_CROSS
%token Y_CROSS2
%token Y_DAL_SEGNO
%token Y_DAL_SEGNO_AL_FINE
%token Y_DAL_SEGNO_AL_CODA
%token Y_DBLBAR
%token Y_DEFOCT
%token Y_DOWN
%token Y_EBM
%token Y_ENDBAR
%token Y_ENDENDING
%token Y_ENDING
%token Y_FERMATA
%token Y_FINE
%token Y_FOOTER
%token Y_GRACE
%token Y_GRIDFRET
%token Y_GRIDSWHEREUSED
%token Y_GRIDS
%token Y_HEADER
%token Y_INCHES
%token Y_IRREGULAER_KEY
%token Y_KEY
%token Y_KEYCHANGE
%token <key> Y_KEYKIND
%token Y_LABEL
%token Y_LYRICS
%token Y_LYRICSDIST
%token Y_LONELY_TRILL
%token Y_MIDI
%token Y_MEASNUM
%token Y_MULTIREST
%token Y_MUSIC
%token Y_MUSSYM
%token <num> Y_NUMBER
%token Y_OCTAVE
%token Y_OVERLENGTH
%token Y_PAGEWIDTH
%token Y_PAGEHEIGHT
%token Y_PARAMETER
%token Y_PEDAL
%token Y_PEDSTAR
%token Y_PEDSTYLE
%token Y_PHRASE
%token Y_PLAYTRANSPOSD
%token Y_PROGRAM
%token <pitch> Y_PITCH
%token <dnum> Y_REAL_NUMBER
%token Y_RECT
%token Y_REHEARSAL
%token Y_REPEATBOTH
%token Y_REPEATEND
%token Y_REPEATSTART
%token Y_RITARDANDO
%token Y_RESTOFFS
%token Y_ROM
%token Y_ROLL
%token Y_SIGN
%token Y_SIZE_14
%token Y_SLASH
%token Y_SCORE
%token Y_SM4N
%token Y_STAFF
%token Y_STAFFS
%token <num> Y_STAFF_NUMBER
%token <numnum> Y_STAFF_VOICE_NUMBER
%token Y_STEMPOLICY
%token Y_STEMUP
%token Y_STEMDOWN
%token <string> Y_STRING
%token <string> Y_STRING_DESCR
%token Y_TEMPO
%token Y_TIL
%token Y_TIME
%token Y_TITLE
%token Y_TRIA
%token Y_TRILL
%token Y_UNDERLENGTH
%token Y_UNITS
%token Y_UP
%token Y_VSCHEME
%token Y_WITH

%type <length> length
%type <noteparam> pitch
%type <muselemdescr> note chord element rest
%type <propertydescr> pitchsuffixes pitchsuffix slur_info bodyinfo
%type <tupletdescr> elmentdescriptions elmentdescription clefchange keychange
%type <tupletdescr2> tupletdescr
%type <modidescr> modificators modificator modi_detail ornament
%type <shifted_miditime> timeposition pedaltimeposition rolltimeposition
%type <symbolkind> trillsym dynamicsym octaviationsym
%type <num> anno_detail stempol size repcount placement
%type <numnum> staffnumber
%type <boolval> yesno

%start musicsheet

%%

musicsheet: header footer parts
		{for (int i = 0; i < voiceCount; i++) {
			if (pending_elements[i].inbeam) {
				yyerror("open beam at end");
				YYERROR;
			}
			while (pending_elements[i].slur_stack->count()) {
				pending_elements[i].slur_stack->first()->slurstart_chord->resetSlurForward();
				 pending_elements[i].slur_stack->remove();
			}
		 }
		}

	  ;

parts: parts  part

     |

     ;

part: scorepart

    | musicpart

    | gridpart

    ;

header: Y_HEADER titles
 	    /* {yydebug = 1;} */

      ;

footer : Y_FOOTER titles

       | 

       ;



titles : titles title

       |	  

       ;


title : Y_TITLE font size Y_STRING
		{set_score_information($3, $4); free($4);}
      ;

font : Y_BOLD

     | Y_ROM

     |

     ;

size : '(' Y_NUMBER ')' 
	{$$ = $2;}
     |

	{$$ = 0;}
     ;

scorepart: Y_SCORE staffproperties

	 ;


staffproperties: staffproperties staffproperty

	       |

	       ;

staffproperty : overallproperty

	      | Y_STAFF Y_NUMBER 
			{if (!select_voice_and_staff($2, 1)) YYERROR; currentStaffNr = $2;}
		singlestaffproperies
	      ;

overallproperty : staffs | timesignature | paperdimension | unitspecification | gridspecification | pedalspecification | bracketinfo | braceinfo | barstyleinfo

		;

unitspecification : Y_UNITS '=' unit

		  ;

pedalspecification : Y_PEDSTYLE '=' Y_PEDSTAR

		   ;

unit : Y_CM | Y_INCHES ;


bracketinfo :  Y_BRACKET '=' brinfos

	    ;

brinfos : brinfo

        | brinfos ',' brinfo

	;

brinfo	: Y_NUMBER '-' Y_NUMBER
		{parser_params.bracketList.append(new layoutDef($1-1, $3-1));}
	| Y_NUMBER Y_NUMBER /* '-' sign attached to number */
		{parser_params.bracketList.append(new layoutDef($1-1, (-$2)-1));}
	;

braceinfo :  Y_BRACE '=' bracinfos

	  ;

bracinfos : bracinfo

	  | bracinfos ',' bracinfo

	  ;

bracinfo : Y_NUMBER '-' Y_NUMBER
		{parser_params.braceList.append(new layoutDef($1-1, $3-1));}
	 | Y_NUMBER Y_NUMBER /* '-' sign attached to number */
		{parser_params.braceList.append(new layoutDef($1-1, (-$2)-1));}
         ;

barstyleinfo : Y_BARSTYLE '=' bstyleinfos

	     ;

bstyleinfos : bstyleinfo

	    | bstyleinfos ',' bstyleinfo

	    |

	    ;

bstyleinfo : Y_NUMBER '-' Y_NUMBER
		{parser_params.contList.append(new layoutDef($1-1, $3-1));}
	   |
	     Y_NUMBER Y_NUMBER /* '-' sign attached to number */
		{parser_params.contList.append(new layoutDef($1-1, (-$2)-1));}
	   ;


paperdimension : Y_PAGEWIDTH '=' Y_NUMBER
			{parser_params.paperwidth = $3 * 10; parser_params.enableParams = true;}

	       | Y_PAGEWIDTH '=' Y_REAL_NUMBER
			{parser_params.paperwidth = (int) ($3 * 10); parser_params.enableParams = true;}

	       | Y_PAGEHEIGHT '=' Y_NUMBER
			{parser_params.paperheight = $3 * 10; parser_params.enableParams = true;}

	       | Y_PAGEHEIGHT '=' Y_REAL_NUMBER
			{parser_params.paperheight = (int) ($3 * 10); parser_params.enableParams = true;}

	       | Y_MEASNUM '=' yesno
			{parser_params.with_measnum = $3;}

	       ;

gridspecification : Y_GRIDSWHEREUSED '=' yesno

		  | Y_GRIDFRET '=' Y_NUMBER

		  ;


yesno : 'y'
		{$$ = true;}
      | 'n'
		{$$ = false;}
      ;
singlestaffproperies : singlestaffproperies singlestaffproperty

		     | singlestaffproperty

		     ;

singlestaffproperty: label | overlength | underlength | lyricsdist | playtransposd | yrestoffs | stempolicy | clef | key | defoct | vscheme

	           ;

label : Y_LABEL '=' Y_STRING
		{current_staff->staffName_ = QString::fromUtf8($3);
		 current_staff->staffName_.replace("\\\"", "\""); /* replace all \" symbols with " */
		 current_staff->staffName_.replace("\\\\", "\\"); /* replace all \\ symbols with \ */
		 free($3);
		}


      ;

gridpart : Y_GRIDS griddescriptions

	 ;


griddescriptions : griddescriptions griddescription

		 |

		 ;

griddescription : Y_CHORDNAME Y_STRING_DESCR
			{chordDiagramList.append(new chordDiagramName($1, $2, true));}
		;

staffs : Y_STAFFS '=' Y_NUMBER
			{if ($3 < 1 || $3 > 200) {
				sprintf(Str, "bad staff count: %d", $3);
				yyerror(Str);
				YYERROR;
			 }
			 else if (staffCount > 0) {
				if (staffCount != $3) {
					sprintf(Str, "contradictory staff count: %d <--> %d", staffCount, $3);
					yyerror(Str);
					YYERROR;
				}
			 }
			 else if (staffCount <= 0) {
				staffCount = $3;
				for (i = 0; i < staffCount; i++) {
                                	parser_params.newStaffs->append(current_staff = new NStaff(i*(NResource::overlength_+STAFF_HIGHT+NResource::underlength_)+NResource::overlength_, i, 0, parser_params.mainWidget));
                                	parser_params.newVoices->append(current_staff->getVoiceNr(0));
				}
			 }
			}

       ;

timesignature : Y_TIME '=' Y_NUMBER '/' Y_NUMBER
			{countof128 = $3 * 128 / $5;  num = $3; denom = $5;newtimesig=1;
			 setStaffPropsMeasureLength(MULTIPLICATOR*countof128);
			}

	      ;

overlength : Y_OVERLENGTH '=' Y_NUMBER
			{if ($3 < 1 || $3 > 200) {
				sprintf(Str, "bad overlength: %d", $3);
				yyerror(Str);
				YYERROR;
			 }
			 else {
				current_staff->overlength_ = $3;
			 }
			}
	   ;




playtransposd : Y_PLAYTRANSPOSD '=' Y_NUMBER
			{if ($3 < -12 || $3 > 12) {
				sprintf(Str, "bad tanspose parameter: %d", $3);
				yyerror(Str);
				YYERROR;
			 }
			 else {
				current_staff->transpose_ = $3;
			 }
			}
	   ;

yrestoffs  : Y_RESTOFFS Y_NUMBER '=' Y_NUMBER
			{NVoice *voice_elem;
			 if ($2 < 1 || $2 > current_staff->voiceCount()) {
				sprintf(Str, "bad voice number %d", $2);
				yyerror(Str);
				YYERROR;
			 }
			 voice_elem = current_staff->getVoiceNr($2-1);
			 voice_elem->yRestOffs_ = $4;
			}

	   ;

stempolicy : Y_STEMPOLICY Y_NUMBER '=' stempol
			{NVoice *voice_elem;
			 if ($2 < 1 || $2 > current_staff->voiceCount()) {
				sprintf(Str, "bad voice number %d", $2);
				yyerror(Str);
				YYERROR;
			 }
			 voice_elem = current_staff->getVoiceNr($2-1);
			 voice_elem->stemPolicy_ = $4;
			}

	   ;

stempol   : Y_STEMUP 
		{$$ = STEM_POL_UP;}

	   | Y_STEMDOWN
		{$$ = STEM_POL_DOWN;}

	   ;
	
			 

underlength : Y_UNDERLENGTH '=' Y_NUMBER
			{if ($3 < 1 || $3 > 200) {
				sprintf(Str, "bad underlength: %d", $3);
				yyerror(Str);
				YYERROR;
			 }
			 else {
				current_staff->underlength_ = $3;
			 }
			}

	   ;

lyricsdist : Y_LYRICSDIST '=' Y_NUMBER
			{if ($3 < 1 || $3 > 200) {
				sprintf(Str, "bad lyricsdist: %d", $3);
				yyerror(Str);
				YYERROR;
			 }
			 else {
				current_staff->staff_props_.lyricsdist = $3;
			 }
			}

	   ;

defoct : Y_DEFOCT '=' Y_NUMBER
		{current_voice->octave_ = $3;}

       ;

clef : Y_CLEF '=' Y_CLEFKIND
		{
		 current_staff->actualClef_.changeKind($3);
		 if (!before_first_music) {
		 	pending_elements[voiceNumber].newclef = 1;
			update_voice_stack(voiceNumber, -1);
		 }
		}

     ;

vscheme : Y_VSCHEME '=' Y_NUMBER 'o'
		{int i;
		 current_staff->addVoices($3 -1);
		 for (i = 1; i < $3; i++) {
		 	parser_params.newVoices->append(current_staff->getVoiceNr(i));
		 }
		}

     ;

key : Y_KEY '=' Y_KEYKIND
		{if ($3.count < 0 || $3.count > 7) {
			sprintf(Str, "bad keysig: %d", $3.count);
			yyerror(Str);
			YYERROR;
		 }
		 else {
			current_staff->actualKeysig_.setRegular($3.count, $3.kind);
			current_staff->actualKeysig_.setClef(&(current_staff->actualClef_));
			if (!before_first_music) {
				pending_elements[voiceNumber].newkeysig = 1;
				update_voice_stack(voiceNumber, -1);
			 }
		 }
		}

    | irregular_key

    ;

irregular_key : Y_IRREGULAER_KEY '=' 
			{current_staff->actualKeysig_.reset();}
		  keydescriptions
			{if (!before_first_music) {
				pending_elements[voiceNumber].newkeysig = 1;
			 }
			}

	      ;

keydescriptions : keydescriptions keydescription

		| keydescription

		;

keydescription : Y_PITCH '#'
			{ current_staff->actualKeysig_.setAccentByNoteName($1, PROP_CROSS);}

	       | Y_PITCH '&'
			{current_staff->actualKeysig_.setAccentByNoteName($1, PROP_FLAT);}
	       ;


musicpart : Y_MUSIC
	    {int i;
		if (before_first_music) {
			voiceCount = parser_params.newVoices->count();
			if ((pending_elements = (struct pendings *) calloc(voiceCount, sizeof(struct pendings))) == NULL) {
				NResource::abort("YACC: internal error");
			}
			for (i = 0; i < voiceCount; i++) {
				pending_elements[i].theVoice = parser_params.newVoices->at(i);
				if (pending_elements[i].theVoice->isFirstVoice()) {
					pending_elements[i].newkeysig = pending_elements[i].newclef = 1;
				}
				else {
					pending_elements[i].newkeysig = pending_elements[i].newclef = 0;
				}
				pending_elements[i].beamstart_chord = 0;
				pending_elements[i].lastBarSym = 0;
				pending_elements[i].slur_stack = new QPtrList<slur_stack_str>();
				pending_elements[i].slur_stack->setAutoDelete(true);
				pending_elements[i].inbeam = 0;
			}
	    		insert_new_clefs_timesigs_and_keys(); /* important: before_first_music == 0 !!! */
			before_first_music = 0;
		}
		else {
	    		insert_new_clefs_timesigs_and_keys(); /* important: before_first_music != 0 !!! */
		}
	    }

	    musicdescriptions

	  ;

musicdescriptions : musicdescriptions musicdescription

		  |

		  ;

musicdescription : mididescription

		 | scoredescription

		 | multirest

		 | annotation 

		 | phrases

		 | lyricsline

		 | volume_marker

		 | musicsym

		 | dynamic

		 | barsym possible_clef_or_keychange

		 | roll

		 | pedal

		 | octaviation

		 | arbitrary_text


		 ;

possible_clef_or_keychange : 
			{/* bring Bison to work */}

		  | clefchange
			{/* bring Bison to work */}

                  | keychange
			{/* bring Bison to work */}

		  ;

mididescription : Y_MIDI Y_STAFF_VOICE_NUMBER ':'
			{if (!select_voice_and_staff($2.staff, 1)) YYERROR;}
		   midiparameters

		| Y_MIDI Y_ALL ':' midiparameters

		;

midiparameters: midiparameters midiparameter

	      |

	      ;

midiparameter : paramdescription ';'
		{$<num>$ = 0; /* to bring bison to work */}

	      ;

paramdescription : timeposition '"' Y_PARAMETER '=' Y_NUMBER ',' Y_NUMBER '"'
			{if ($7 < 0 || $7 > 127) {
				sprintf(Str, "bad papameter: %d", $7);
				yyerror(Str);
				YYERROR;
			 }
			 else {
			    switch($5) {
				case PROG_CHORUS: current_voice->getStaff()->chorus_ = $7; break;
				case PROG_REVERB: current_voice->getStaff()->reverb_ = $7; break;
				case PROG_PAN: current_voice->getStaff()->pan_ = $7; break;
				case PROG_VOL: 
					if (current_voice->firstVolume_) {
						current_voice->getStaff()->setVolume($7);
						current_voice->firstVolume_ = false;
					}
					else {	
						pending_volume = $7;
					}
					break;
			    }
			 }
			}

		 | timeposition '"' Y_CHANNEL '=' Y_NUMBER '"'
			{if ($5 < 1 || $5 > 16) {
				sprintf(Str, "bad channel: %d", $5);
				yyerror(Str);
				YYERROR;
			 }
			 else {
				current_staff->setChannel($5-1);
			 }
			}

		 | timeposition '"' Y_TEMPO  '=' Y_NUMBER '"'
			{if ($5 < 10 || $5 > 300) {
				sprintf(Str, "bad tempo: %d", $5);
				yyerror(Str);
				YYERROR;
			 }
			 else {
				NSign *sign;
				sign = new NSign(current_voice->getMainPropsAddr(), current_staff->getStaffPropsAddr(), TEMPO_SIGNATURE);
				sign->setTempo($5);
				if (!current_voice->insertElemAtTime(($1.beats * 128 / denom) >> 7, sign, pending_elements[voiceNumber].lastBarSym)) {
					yyerror("error inserting time signature");
					YYERROR;
				}
				update_voice_stack(voiceNumber, -1);
			 }
			}

		 | timeposition '"' Y_PROGRAM '=' Y_NUMBER '"'
			{if ($5 < 0 || $5 > 127) {
				sprintf(Str, "bad voice: %d", $5);
				yyerror(Str);
				YYERROR;
			 }
			 else {
				if (current_voice->voiceSet_) {
					NSign *sign;
					sign = new NSign(current_voice->getMainPropsAddr(), current_staff->getStaffPropsAddr(), PROGRAM_CHANGE);
					sign->setProgram($5);
					if (!current_voice->insertElemAtTime(($1.beats * 128 / denom) >> 7, sign, pending_elements[voiceNumber].lastBarSym)) {
						yyerror("bad tempo sig time");
						YYERROR;
					}
					update_voice_stack(voiceNumber, -1);
				}
				else {
					current_staff->changeVoice($5);
					current_voice->voiceSet_ = true;
				}
			 }
			}
		 ;

multirest : Y_MULTIREST Y_NUMBER
		{insert_multirest($2);}
	  ;


annotation : Y_ROM placement Y_STAFF_NUMBER  ':' 
		{if (!select_voice_and_staff($3, 1)) YYERROR;}
		anno_descriptions

	   | Y_ROM placement Y_ALL ':' anno_descriptions

	   | Y_ROM Y_CHORD Y_STAFF_NUMBER  ':' chord_annotations
			{chordname_expected = 0;}
	   ;

anno_descriptions : anno_descriptions anno_description

		  |

		  ;

volume_marker : Y_BOLDITAL placement Y_STAFF_NUMBER  ':' timeposition Y_STRING ';'
				{if (pending_volume >= 0) {
					if (!insert_volume_marker(($5.beats * 128 / denom) >> 7, $6, pending_volume)) {
						sprintf(Str, "bad volume marker: %s", $6);
						yyerror(Str);
						YYERROR;
					}
					pending_volume = -1;
				 }
				 free($6);
				}

	      ;

placement: Y_ABOVE
		{$$ = 0;}

	 | Y_BELOW
	 	{$$ = 1;}

	 ;

chord_annotations : chord_annotations chord_annotation

		  |

		  ;

chord_annotation : timeposition Y_CHORDNAME ';'
			{insertChordDiagramm(($1.beats * 128 / denom) >> 7, $2, pending_elements[voiceNumber].lastBarSym);}
		 ;


scoredescription: staffnumber
			{if (!select_voice_and_staff($1.staff, $1.voice)) YYERROR;}
		   elmentdescriptions

		| staffnumber
			{if (!select_voice_and_staff($1.staff, $1.voice)) YYERROR;}
                ;

staffnumber : Y_STAFF_NUMBER ':'
		{$$.staff = $1, $$.voice = 1;}

	    | Y_STAFF_VOICE_NUMBER ':'

	    ;

lyricsline: Y_LYRICS Y_STAFF_NUMBER ':' 
		{if (!select_voice_and_staff($2, 1)) YYERROR;}
	    Y_STRING ';'
		{MUP2lyrics($5);
		 current_voice->addLyrics($5, 0);
		 free($5);
		}

	  | Y_LYRICS Y_STAFF_NUMBER ':' '[' Y_NUMBER ']'
		{if (!select_voice_and_staff($2, 1)) YYERROR;}
	    Y_STRING ';'
		{if ($5 < 1 || $5 > 5) {
			sprintf(Str, "bad verse number: %d", $5);
			yyerror(Str);
			YYERROR;
		 }
		 else {
		        MUP2lyrics($8);
			current_voice->addLyrics($8, $5 - 1);
		 }
		 free($8);
		}
	  ;

barsym : Y_BAR
		{append_barsym_in_all_voices_and_reset_keysig(SIMPLE_BAR, 0 /* dummy */);}

       | Y_BAR Y_ENDING Y_STRING
		{append_barsym_in_all_voices_and_reset_keysig(SIMPLE_BAR, 0 /* dummy */);
		 if (strchr($3, '1')) {
			append_barsym_in_all_voices_and_reset_keysig(SPECIAL_ENDING1, 0 /* dummy */);
		 }
		 else {
			append_barsym_in_all_voices_and_reset_keysig(SPECIAL_ENDING2, 0 /* dummy */);
		 }
		 free($3);
		}

       | Y_REPEATSTART
		{append_barsym_in_all_voices_and_reset_keysig(REPEAT_OPEN, 0 /* dummy */);}

       | Y_REPEATBOTH
		{append_barsym_in_all_voices_and_reset_keysig(REPEAT_OPEN_CLOSE, 0 /* dummy */);}

       | Y_REPEATEND repcount Y_ENDING Y_STRING
		{append_barsym_in_all_voices_and_reset_keysig(REPEAT_CLOSE, $2);
		 if (strchr($4, '1')) {
			append_barsym_in_all_voices_and_reset_keysig(SPECIAL_ENDING1, 0 /* dummy */);
		 }
	 	 else {
			append_barsym_in_all_voices_and_reset_keysig(SPECIAL_ENDING2, 0 /* dummy */);
	 	 }
	 	 free($4);
		}

       | Y_REPEATEND repcount
		{append_barsym_in_all_voices_and_reset_keysig(REPEAT_CLOSE, $2);}

       | Y_REPEATEND repcount Y_ENDENDING
		{append_barsym_in_all_voices_and_reset_keysig(REPEAT_CLOSE, $2);}

       | Y_BAR Y_ENDENDING
		{append_barsym_in_all_voices_and_reset_keysig(SIMPLE_BAR, 0 /* dummy */);}

       | Y_REPEATSTART Y_ENDENDING
		{append_barsym_in_all_voices_and_reset_keysig(REPEAT_OPEN, 0 /* dummy */);}
       | Y_DBLBAR
       		{append_barsym_in_all_voices_and_reset_keysig(DOUBLE_BAR, 0 /* dummy */);}
       | Y_DBLBAR Y_ENDENDING
       		{append_barsym_in_all_voices_and_reset_keysig(DOUBLE_BAR, 0 /* dummy */);}
       | Y_ENDBAR
       		{append_barsym_in_all_voices_and_reset_keysig(END_BAR, 0 /* dummy */);}
       | Y_ENDBAR Y_ENDENDING
       		{append_barsym_in_all_voices_and_reset_keysig(END_BAR, 0 /* dummy */);}

       ;


repcount : Y_REHEARSAL Y_STRING
		{int k; sscanf($2, "x %d", &k); free($2); $$ = k;}
	 | {$$ = 2;}

	 ;

anno_description :  timeposition anno_detail ';'
			{insert_segno_accel_and_ritard((($1.beats * 128 / denom) >> 7), $2, pending_elements[voiceNumber].lastBarSym); }
	         | trill ';'

		 ;

anno_detail : '"' '(' Y_SM4N  '=' Y_NUMBER ')' '"'

		{$$ = 0;}

	    | '"' Y_SIGN '"'
	    	{$$ = SEGNO;}

	    | '"' Y_DAL_SEGNO '"'
	    	{$$ = DAL_SEGNO;}

	    | '"' Y_DAL_SEGNO_AL_CODA '"'
	    	{$$ = DAL_SEGNO_AL_CODA;}

	    | '"' Y_DAL_SEGNO_AL_FINE '"'
	    	{$$ = DAL_SEGNO_AL_FINE;}

	    | '"' Y_FINE '"'
	    	{$$ = FINE;}

	    | '"' Y_CODA '"'
	    	{$$ = CODA;}

	    | '"' Y_RITARDANDO '"'
	    	{$$ = RITARDANDO;}

	    | '"' Y_ACCELERANDO '"'
	    	{$$ = ACCELERANDO;}

	    ;

musicsym : Y_MUSSYM placement Y_STAFF_NUMBER ':'
		{if (!select_voice_and_staff($3, 1)) YYERROR;}
	   trills

	 ;

trills : trills trill ';'

       |

       ;


trill : timeposition trillsym Y_TIL timeposition 
		{if (!current_voice->setProvisionalTrill($2, ($1.beats * 128 / denom) >> 7, $4.measures, ($4.beats * 128 / denom) >> 7,
			pending_elements[voiceNumber].lastBarSym))  {
				yyerror("bad placed trills");
				YYERROR;
			}
		}
						

      ; 

trillsym : Y_TRILL

		{$$ = NORMAL_TRILL;}

	 | Y_LONELY_TRILL

		{$$ = LONELY_TRILL;}

	 ;

phrases : Y_PHRASE  Y_STAFF_NUMBER ':'
		{if (!select_voice_and_staff($2, 1)) YYERROR;}
	  phrasesymbols

	| Y_PHRASE Y_BELOW Y_STAFF_NUMBER ':'
		 {if (!select_voice_and_staff($3, 2)) YYERROR;}
	  phrasesymbols

	;

phrasesymbols : phrasesymbols phrase ';'

	|

	;

phrase  : timeposition Y_TIL timeposition
		{ if (!current_voice->setProvisionalSlur(($1.beats * 128 / denom) >> 7,  $3.measures, ($3.beats * 128 / denom) >> 7,
				pending_elements[voiceNumber].lastBarSym))  {
				yyerror("bad placed phrase");
				YYERROR;
			}
		}
	;

roll    : Y_ROLL staffnumber 
		{if (!select_voice_and_staff($2.staff, $2.voice)) YYERROR;}
	  rolltimepositiones

	;

rolltimepositiones : rolltimepositiones rolltimeposition ';'
		{if (!current_voice->setReadArpeggio(($2.beats * 128 / denom) >> 7,
			pending_elements[voiceNumber].lastBarSym))  {
				yyerror("bad placed arpeggio");
				YYERROR;
			}
		}

	       |

	       ;

rolltimeposition : timeposition

		 ;

octaviation : Y_OCTAVE placement Y_STAFF_NUMBER ':'
		{if (!select_voice_and_staff($3, 1)) YYERROR;}
	   oct_descrs
	 ;

oct_descrs : oct_descrs oct_descr

	   | oct_descr

	   ;

oct_descr : timeposition octaviationsym Y_TIL timeposition ';'
		{if (!current_voice->setProvisionalOctaviation($2, ($1.beats * 128 / denom) >> 7, $4.measures, ($4.beats * 128 / denom) >> 7,
			pending_elements[voiceNumber].lastBarSym))  {
				yyerror("bad placed octaviation");
				YYERROR;
			}
		}
      ; 

octaviationsym : Y_8VA
			{$$ = OCTAVIATION1P;}
	       |
		 Y_8VA_BASSA
		 	{$$ = OCTAVIATION1M;}
	       ;

pedal    : Y_PEDAL staffnumber 
		{if (!select_voice_and_staff($2.staff, $2.voice)) YYERROR;}
	  pedaltimepositiones

	;

pedaltimepositiones : pedaltimepositiones pedaltimeposition ';'
		{if (!current_voice->setReadPedalOn(($2.beats * 128 / denom) >> 7,
			pending_elements[voiceNumber].lastBarSym))  {
				yyerror("bad placed arpeggio");
				YYERROR;
			}
		}

	       | pedaltimepositiones pedaltimeposition '*' ';'
		{if (!current_voice->setReadPedalOff(($2.beats * 128 / denom) >> 7,
			pending_elements[voiceNumber].lastBarSym))  {
				yyerror("bad placed arpeggio");
				YYERROR;
			}
		}

	       |

	       ;

pedaltimeposition : timeposition

		 ;


dynamic : dynamicsym placement Y_STAFF_NUMBER ':'
		{if (!select_voice_and_staff($3, 1)) YYERROR;}
	  timeposition Y_TIL timeposition ';'
		{if (!current_voice->setProvisionalDynamic($1, ($6.beats * 128 / denom) >> 7, $8.measures, ($8.beats * 128 / denom) >> 7,
			pending_elements[voiceNumber].lastBarSym))  {
				yyerror("bad placed dynamics");
				YYERROR;
			}
		}
						

      ; 

dynamicsym : '>'

		{$$ = DYN_CRESCENDO;}

	   | '<'

		{$$ = DYN_DECRESCENDO;}

	   ;

arbitrary_text: Y_ROM Y_SIZE_14 placement Y_STAFF_NUMBER ':'
			{if (!select_voice_and_staff($4, 1)) YYERROR; current_string_placement = $3; }
		string_infos

	     ;

string_infos : string_info

	     | string_infos string_info

	     ;

string_info : {enable_strings();} timeposition Y_STRING ';'

		{current_voice->setProvisionalString(QString::fromUtf8($3) ,current_string_placement ,($2.beats * 128 / denom) >> 7,
			pending_elements[voiceNumber].lastBarSym);
		}
	     ;

timeposition : Y_NUMBER
		{$$.beats = (($1 - 1) << 7); $$.measures = 0;}

	     | Y_REAL_NUMBER
		{$$.beats = (int) (($1 - 1.0) * ((double) (1 << 7)) + 0.5); $$.measures = 0;}


	     | Y_NUMBER 'm' '+' Y_NUMBER 

		{$$.beats = (($4 - 1) << 7); $$.measures = $1;}

	     | Y_NUMBER 'm' '+' Y_REAL_NUMBER 

		{$$.beats = (int) (($4 - 1.0) * ((double) (1 << 7)) + 0.5); $$.measures = $1;}
	     ;


elmentdescriptions: elmentdescriptions elmentdescription

			{$$.last = $2.first;}

		  | elmentdescription


		  ;

elmentdescription : modificators
			{current_modificators = $1;}
		    element ';'
			{current_voice->appendElem($3.element.muselem);
			 miditime += $3.element.muselem->getMidiLength();
			 update_voice_stack(voiceNumber, -1);
			 if (!pending_elements[voiceNumber].slur_stack->isEmpty()) {
				if (pending_elements[voiceNumber].slur_stack->getFirst()->slurdist < 0) {
					yyerror("bad placed slures(1)");
					YYERROR;
				}
				if (pending_elements[voiceNumber].slur_stack->getFirst()->slurdist == 0) {
					if ($3.element.muselem->getType() != T_CHORD) {
						fprintf(stderr, "Type = 0x%x\n", $3.element.muselem->getType());
						yyerror("bad placed slures(2)");
						YYERROR;
					}
					NChord *chord = (NChord *) $3.element.muselem;
					pending_elements[voiceNumber].slur_stack->first()->slurstart_chord->setSlured(true, chord);
					pending_elements[voiceNumber].slur_stack->remove();
				}
			 }
			 if ($3.slurdist) {
		                slur_stack_str *sl_str;
			 	if ($3.element.muselem->getType() != T_CHORD) {
					fprintf(stderr, "Type = 0x%x\n", $3.element.muselem->getType());
					yyerror("bad placed slures(5)");
					YYERROR;
				}
				pending_elements[voiceNumber].slur_stack->insert(0, sl_str = new slur_stack_str);
				sl_str->slurdist = $3.slurdist;
				sl_str->slurstart_chord = $3.element.chord;
			 }
			 $$.first = $3.element.muselem;
			 $$.last = 0;
			 switch($3.beamstatus) {
				case NO_BEAM : break;
				case BEAM_END: if (pending_elements[voiceNumber].beamstart_chord == 0) {
							yyerror("bad beam placement(1)");
							YYERROR;
					       }
					       if (!current_voice->buildBeam(pending_elements[voiceNumber].beamstart_chord, $3.element.chord))  {
							yyerror("bad beam placement(2)");
							YYERROR;
					       }
					       pending_elements[voiceNumber].beamstart_chord = 0;
					       break;
				case BEAM_START: if (pending_elements[voiceNumber].beamstart_chord != 0) {
							yyerror("bad beam placement(3)");
							YYERROR;
					       }
					       pending_elements[voiceNumber].beamstart_chord = $3.element.chord;
					       break;
					       yyerror("bad beam placement(4)");
					       YYERROR;
					       break;
			 }
			  current_voice->setActualStemDir(STEM_DIR_AUTO);
			}

		  | '{' 
			{if (braces++) {
				yyerror("nested tuplets");
				YYERROR;
			 }
			}
		     elmentdescriptions '}' Y_ABOVE tupletdescr
			{braces--; 
			 if (!$3.first || !$3.last) {
				yyerror("bad tuplet placment");
				YYERROR;
			 }
			 else {
				switch($6.type) {
					case CORRECT_TRIPLET: 
					case CORRECT_TRIPLET_WITH_DOT: 
						if (!current_voice->buildTuplet2($3.first, $3.last, $6.numNotes, $6.length, $6.type == CORRECT_TRIPLET_WITH_DOT)) {
							yyerror("bad tuplet placment");
							YYERROR;
						}
						break;
					default:
						if (!current_voice->buildTuplet($3.first, $3.last, $6.numNotes, $6.playtime)) {
							yyerror("bad tuplet placment");
							YYERROR;
						}
						break;
				}
			}
		       }

		    | clefchange

		    | keychange

		    ;


clefchange : Y_CLEFCHANGE '{' Y_CLEF '=' Y_CLEFKIND '}'

		{NClef *clef;
		 current_staff->actualClef_.changeKind($5);
		 current_voice->appendElem(clef = new NClef(current_voice->getMainPropsAddr(), current_staff->getStaffPropsAddr()));
		 clef->change(&(current_staff->actualClef_));
		 current_staff->actualKeysig_.setClef(&(current_staff->actualClef_));
		 update_voice_stack(voiceNumber, -1);
		}
	    ;
			 

keychange: Y_KEYCHANGE '{'  Y_KEY '=' Y_KEYKIND '}'

		{NKeySig *ksig;
		 if ($5.count < 0 || $5.count > 7) {
			sprintf(Str, "bad keysig: %d", $5.count);
			yyerror(Str);
			YYERROR;
		 }
		 else {
			current_staff->actualKeysig_.setRegular($5.count, $5.kind);
			current_staff->actualKeysig_.setClef(&(current_staff->actualClef_));
			ksig = new NKeySig(current_voice->getMainPropsAddr(), current_staff->getStaffPropsAddr());
			ksig->change(&(current_staff->actualKeysig_));
			current_voice->appendElem(ksig);
			update_voice_stack(voiceNumber, -1);
		 }
	 	}
	  ;

tupletdescr: Y_NUMBER ';'
		{ if ($1 != 3) {
			$$.type = INCORRECT_TRIPLET;
			$$.numNotes = 3;  $$.playtime = 2;
			sprintf(Str, "bad tuplet number (%d)", $1);
			yyerror(Str);
			YYERROR;
		  }
		  $$.type = INCORRECT_TRIPLET;
		  $$.numNotes = 3;  $$.playtime = 2;
		}

	    | Y_NUMBER ',' Y_NUMBER ';'
		{ $$.type =  CORRECT_TRIPLET;
		  $$.numNotes = $1; $$.length = $3;
		}

	    | Y_NUMBER ',' Y_NUMBER '.' ';'
		{ $$.type =  CORRECT_TRIPLET_WITH_DOT;
		  $$.numNotes = $1; $$.length = $3;
		}

	    | Y_NUMBER ',' 'x' Y_NUMBER ';'
		{$$.type = INCORRECT_TRIPLET;
		 $$.numNotes = $1; $$.playtime = $4;
		}
	    ;
			

modificators : modificators modificator
		{$$ = $1 | $2;}

	     |
		{$$ = 0;}

	     ;

modificator : '[' modi_detail']'
		{$$ = $2;}

	    ;

modi_detail: Y_WITH ornament

		{$$ = $2;}

	   | Y_UP
		{$$ = 0; current_voice->setActualStemDir(STEM_DIR_UP);}

	   | Y_DOWN
		{$$ = 0; current_voice->setActualStemDir(STEM_DIR_DOWN);}

	   | Y_GRACE
		{$$ = PROP_GRACE;}

	   | Y_GRACE ';' Y_SLASH Y_NUMBER
		{$$ = (PROP_GRACE | INTERIM_STOKE);}

	   ;


ornament : '.' 
		{$$ = PROP_STACC;}
	 |
	   '^' 
		{$$ = PROP_SFORZ;}
	 |
	   '-' 
		{$$ = PROP_PORTA;}
	 |
	   ',' 
		{$$ = PROP_STPIZ;}
	 |
	   '>' 
		{$$ = PROP_SFZND;}
	 |
	   '"' Y_FERMATA '"'
		{$$ = PROP_FERMT;}
	 ;

element : note
		{$$.element.muselem = $1.element.chord;
		 $$.slurdist = $1.slurdist;
		}

	| chord
		{$$.element.muselem = $1.element.chord;
		 $$.slurdist = $1.slurdist;
		}

	| rest
	       {$$.slurdist = 0;}

	;

chord: note pitch

	{$1.element.chord->insertNewNote($2.line, $2.offs, STEM_POL_INDIVIDUAL, $2.properties);
	 $$.beamstatus = $2.beamstatus;
	 $$.slurdist = $1.slurdist;
	 }
	 

     | chord pitch

	{$1.element.chord->insertNewNote($2.line, $2.offs, STEM_POL_INDIVIDUAL, $2.properties);
	 $$.beamstatus = $2.beamstatus;
	 $$.slurdist = $1.slurdist;
	}

     ;


note : length pitch
	 {if ((current_modificators & PROP_GRACE) && (current_modificators & INTERIM_STOKE)) {
		$1.length = INTERNAL_MARKER_OF_STROKEN_GRACE;
		current_modificators &= (~INTERIM_STOKE);
	 }
	 $$.element.chord = new NChord(&(parser_params.mainWidget->main_props_), &(current_staff->staff_props_),
		 current_voice, $2.line, $2.offs, $1.length, current_voice->stemPolicy_, $1.properties | $2.properties | current_modificators);
	 $$.beamstatus = $2.beamstatus;
	 $$.slurdist = 0;
	 if ($2.properties &  PROP_SLURED) {
		if ($2.slurdist < 1 || $2.slurdist > 1000) {
			sprintf(Str, "bad slur distance: %d", $2.slurdist);
			yyerror(Str);
			YYERROR;
		}
		else {
			$$.slurdist = $2.slurdist;
		}
	 }
	}
     ;

rest : length 'r'
	{int stat = 0;
	 if (current_modificators & PROP_FERMT) stat = PROP_FERMT;
	 $$.element.muselem = new NRest(&(parser_params.mainWidget->main_props_), &(current_staff->staff_props_), &(current_voice->yRestOffs_), $1.length, $1.properties | stat);
	 $$.beamstatus = 0;
	}
     | length 's'
	{$$.element.muselem = new NRest(&(parser_params.mainWidget->main_props_), &(current_staff->staff_props_), &(current_voice->yRestOffs_), $1.length, $1.properties | PROP_HIDDEN);
	 $$.beamstatus = 0;
	}

     ;

length : Y_NUMBER
		{$$.length = WHOLE_LENGTH / $1; $$.properties = 0;} 

       | Y_NUMBER '.'
		{$$.length = WHOLE_LENGTH / $1; $$.properties = PROP_SINGLE_DOT;} 

       | Y_NUMBER '.' '.'
		{$$.length = WHOLE_LENGTH / $1; $$.properties = PROP_DOUBLE_DOT;} 

       | Y_REAL_NUMBER /* "4." or so */
       		{$$.length = WHOLE_LENGTH / ((int) ($1+0.4)); $$.properties = PROP_SINGLE_DOT;} 

       | Y_REAL_NUMBER '.' /* "4.." or so */
       		{$$.length = WHOLE_LENGTH / ((int) ($1+0.4)); $$.properties = PROP_DOUBLE_DOT;} 

       | Y_1_2
		{$$.length = DOUBLE_WHOLE_LENGTH / $1; $$.properties = 0;} 

       | Y_1_2 '.'
		{$$.length = DOUBLE_WHOLE_LENGTH / $1; $$.properties = PROP_SINGLE_DOT;} 

       | Y_1_2 '.' '.'
		{$$.length = DOUBLE_WHOLE_LENGTH / $1; $$.properties = PROP_DOUBLE_DOT;} 

       ;

pitch : Y_PITCH pitchsuffixes
		{$$.line = current_staff->actualClef_.name2Line($1);
		 if ($2.properties & PROP_FORCE) {
			$$.offs = $2.offs;
		 }
		 else {
		 	$$.offs = UNDEFINED_OFFS;
		 }
		 $$.line += $2.octavmodi;
		 $$.properties = $2.properties;
		 $$.beamstatus = $2.beamstatus;
		 $$.slurdist = $2.slurdist;
		}
      ;


pitchsuffixes : pitchsuffixes pitchsuffix
			{$$.octavmodi = $1.octavmodi + $2.octavmodi; 
			 $$.properties = $1.properties | $2.properties;
			 $$.offs = $1.offs + $2.offs;
			 $$.beamstatus = $1.beamstatus | $2.beamstatus;
			 $$.slurdist = $1.slurdist + $2.slurdist;
			}

		  |
			{memset(&($$), 0, sizeof(struct property_descr_str));}

		  ;

pitchsuffix : '+'
		{memset(&($$), 0, sizeof(struct property_descr_str)); $$.octavmodi = 7;}

	    | '-'
		{memset(&($$), 0, sizeof(struct property_descr_str)); $$.octavmodi = -7;}

	    | '#'
		{memset(&($$), 0, sizeof(struct property_descr_str));$$.offs = 1; $$.properties = PROP_FORCE;}

	    | '&'
		{memset(&($$), 0, sizeof(struct property_descr_str));$$.offs -= 1; $$.properties = PROP_FORCE;}

	    | 'x'
		{memset(&($$), 0, sizeof(struct property_descr_str));$$.offs = 2;  $$.properties = PROP_FORCE;}

	    | '~'
		{memset(&($$), 0, sizeof(struct property_descr_str)); $$.properties = PROP_TIED; }

	    | 'n'
		{memset(&($$), 0, sizeof(struct property_descr_str)); $$.properties = PROP_FORCE;}

    	    | slur_info

	    | Y_BM
		{if (pending_elements[voiceNumber].inbeam) {	
			yyerror("nested beams");
			YYERROR;
		 }
		 pending_elements[voiceNumber].inbeam = 1;
		 memset(&($$), 0, sizeof(struct property_descr_str));$$.beamstatus = BEAM_START;
		}

	    | Y_EBM
		{if (!pending_elements[voiceNumber].inbeam) {
			yyerror("missing beam start");
			YYERROR;
		 }
		 pending_elements[voiceNumber].inbeam = 0;
		 memset(&($$), 0, sizeof(struct property_descr_str));$$.beamstatus = BEAM_END;
		}

	    | bodyinfo

	    ; 

bodyinfo : Y_CROSS
		{memset(&($$), 0, sizeof(struct property_descr_str)); $$.properties = PROP_BODY_CROSS;}
	  | Y_CROSS2
		{memset(&($$), 0, sizeof(struct property_descr_str)); $$.properties = PROP_BODY_CROSS2;}
	  | Y_CIRCLE_CROSS
		{memset(&($$), 0, sizeof(struct property_descr_str)); $$.properties = PROP_BODY_CIRCLE_CROSS;}
	  | Y_RECT
		{memset(&($$), 0, sizeof(struct property_descr_str)); $$.properties = PROP_BODY_RECT;}
	  | Y_TRIA
		{memset(&($$), 0, sizeof(struct property_descr_str)); $$.properties = PROP_BODY_TRIA;}
	  ;


slur_info : '<' Y_NUMBER '>'
		{memset(&($$), 0, sizeof(struct property_descr_str)); $$.properties = PROP_SLURED, $$.slurdist = $2;}
	  | '<' pitch '>'
		{memset(&($$), 0, sizeof(struct property_descr_str)); $$.properties = PROP_SLURED, $$.slurdist = 1;}
	
	 ;

%%

void yyerror(const char *s) {
	extern int YYLINENO;
	QString Str;
#ifdef YACCDEBUG
	fprintf(stderr, "In %s line %d: %s\n", parser_params.fname, YYLINENO, s);
#else
	Str.sprintf("%s line %d: %s", parser_params.fname, YYLINENO, s);
	NResource::printWarning(Str);
#endif
}


void init_parser_variables() {
	chordDiagramName *diagname;

	while (!chordDiagramList.isEmpty()) {
		diagname = chordDiagramList.first();
		delete diagname->cdiagramm;
		chordDiagramList.remove();
	}
	current_staff = 0;
	current_voice = 0;
	staffCount = 0;
	voiceCount = 0;
	braces = 0;
	newtimesig = 0;
	num = denom = 4;
	pending_volume = -1;
	miditime = 0;
	before_first_music = 1;
	parser_params.scTitle_.truncate(0);
	parser_params.scSubtitle_.truncate(0);
	parser_params.scAuthor_.truncate(0);
	parser_params.scLastAuthor_.truncate(0);
	parser_params.scComment_.truncate(0);
	parser_params.enableParams = false;
	parser_params.paperwidth = 213;
	parser_params.paperheight = 275;
	parser_params.with_measnum = false;
	pending_elements = 0;
	parser_params.bracketList.setAutoDelete(true);
	parser_params.bracketList.clear();
	parser_params.braceList.setAutoDelete(true);
	parser_params.braceList.clear();
	parser_params.contList.setAutoDelete(true);
	parser_params.contList.clear();
}

void cleanup_parser_variables() {
	int i;
	chordDiagramName *diagname;
	if (pending_elements != 0) {
		for (i = 0; i < voiceCount; i++) {
			pending_elements[i].slur_stack->clear();
			delete pending_elements[i].slur_stack;
		}
		free(pending_elements);
		pending_elements = 0;
	}
	while (!chordDiagramList.isEmpty()) {
		diagname = chordDiagramList.first();
		delete diagname->cdiagramm;
		chordDiagramList.remove();
	}
}
static int insert_volume_marker(int volTime, char *volstring, int volume) {
	NSign *sign;
	int voltype;
	if (!strncmp(volstring, "ppp", strlen("ppp"))) {
		voltype = V_PPPIANO;
	}
	else if (!strncmp(volstring, "pp", strlen("pp"))) {
		voltype = V_PPIANO;
	}
	else if (!strncmp(volstring, "p", strlen("p"))) {
		voltype = V_PIANO;
	}
	else if (!strncmp(volstring, "mp", strlen("mp"))) {
		voltype = V_MPIANO;
	}
	else if (!strncmp(volstring, "fff", strlen("fff"))) {
		voltype = V_FFFORTE;
	}
	else if (!strncmp(volstring, "ff", strlen("ff"))) {
		voltype = V_FFORTE;
	}
	else if (!strncmp(volstring, "f", strlen("f"))) {
		voltype = V_FORTE;
	}
	else if (!strncmp(volstring, "mf", strlen("mf"))) {
		voltype = V_MEZZO;
	}
	else {
		return 0;
	}
	sign = new NSign(current_voice->getMainPropsAddr(), current_voice->getStaff()->getStaffPropsAddr(), VOLUME_SIG);
	sign->setVolume(voltype, volume);
	if (!current_voice->insertElemAtTime(volTime, sign, pending_elements[voiceNumber].lastBarSym)) {
		return 0;
	}
	update_voice_stack(voiceNumber, -1);
	return 1;
}

static void append_barsym_in_all_voices_and_reset_keysig(int bartype, int repcount) {
	NVoice *voice_elem;
	NStaff *staff_elem;
	NSign *sign;
	int i;
	for (i = 0, voice_elem = parser_params.newVoices->first(); voice_elem; voice_elem = parser_params.newVoices->next(), i++) {
		if (!voice_elem->isFirstVoice()) continue;
		update_voice_stack(i, -1);
		staff_elem = voice_elem->getStaff();
		staff_elem->actualKeysig_.deleteTempAccents();
		sign = new NSign(&(parser_params.mainWidget->main_props_), &(staff_elem->staff_props_), bartype);
		voice_elem->appendElem(pending_elements[i].lastBarSym = sign);
		if (repcount > 2) {
			sign->setRepeatCount(repcount);
		}
	}
}

static void insert_multirest(int multirestlength) {
	NVoice *voice_elem;
	NStaff *staff_elem;
	for (voice_elem = parser_params.newVoices->first(); voice_elem; voice_elem = parser_params.newVoices->next()) {
		update_voice_stack(voiceNumber, -1);
		staff_elem = voice_elem->getStaff();
		NRest *rest = new NRest(&(parser_params.mainWidget->main_props_), &(staff_elem->staff_props_), &(voice_elem->yRestOffs_),
			 MULTIREST, multirestlength);
		voice_elem->appendElem(rest);
	}
}

static void setStaffPropsMeasureLength(int mlength) {
	NStaff *staff_elem;
	for (staff_elem = parser_params.newStaffs->first(); staff_elem; staff_elem = parser_params.newStaffs->next()) {
		staff_elem->staff_props_.measureLength = mlength;
	}
}

static int select_voice_and_staff(unsigned int staff_nr, unsigned int voice_nr) {
	bool found;

	staff_nr--; voice_nr--;
	if ((current_staff = parser_params.newStaffs->at(staff_nr)) == NULL) {
		sprintf(Str, "bad staff number: %d\n", staff_nr+1);
		yyerror(Str);
		return 0;
	}
	if (voice_nr >= current_staff->voiceCount()) {
		sprintf(Str, "bad voice number: %d\n", voice_nr+1);
		yyerror(Str);
		return 0;
	}
	current_voice = current_staff->getVoiceNr(voice_nr);
	if (!voiceCount) return 1;
	found = false; voiceNumber = 0;
	
	while (!found && voiceNumber < voiceCount) {
		if (!(found = (pending_elements[voiceNumber].theVoice == current_voice))) voiceNumber++;
	}
	if (!found) {
		NResource::abort("Parser: internal error");
	}
	return 1;
} 

static void insert_new_clefs_timesigs_and_keys() {
	NClef *c;
	NKeySig *ksig;
	property_type kind;
	int count;
	for (i = 0, current_voice = parser_params.newVoices->first(); current_voice; current_voice = parser_params.newVoices->next(), i++) {
		if (!current_voice->isFirstVoice()) continue;
		current_staff = current_voice->getStaff();
		current_staff->actualClef_.setShift(current_voice->octave_);
		if (pending_elements[i].newclef) {
			current_voice->appendElem(c = new NClef(current_voice->getMainPropsAddr(), current_staff->getStaffPropsAddr()));
			c->change(&(current_staff->actualClef_));
			current_staff->actualKeysig_.setClef(&(current_staff->actualClef_));
			pending_elements[i].newclef = 0;
		}
		if (pending_elements[i].newkeysig) {
			ksig = new NKeySig(current_voice->getMainPropsAddr(), current_staff->getStaffPropsAddr());
			ksig->change(&(current_staff->actualKeysig_));
			if (before_first_music) {
				if (ksig->isRegular(&kind, &count)) {
					if (count == 0) {
						delete ksig;
						continue;
					}
				}
			}
			current_voice->appendElem(ksig);
			pending_elements[i].newkeysig = 0;
		}
	}
	if (newtimesig) {
		for (current_voice = parser_params.newVoices->first(); current_voice; current_voice = parser_params.newVoices->next()) {
			if (!current_voice->isFirstVoice()) continue;
			current_voice->appendElem(T_TIMESIG, num, denom);
		}
		newtimesig = 0;
	}
}

static void remove_newlines(QString *s) {
	int idx;

	while ((idx = s->find(newLineLiterally, 0)) != -1) {
		s->replace(idx, 2, newLine);
	}
}


static void set_score_information(int key, char *value) {
	switch (key) {
		case SC_TITLE_FONT_SIZE: parser_params.scTitle_ = QString::fromUtf8(value);
					remove_newlines(&(parser_params.scTitle_)); break;
		case SC_SUBTITLE_FONT_SIZE:parser_params.scSubtitle_ = QString::fromUtf8(value);
					remove_newlines(&(parser_params.scSubtitle_)); break;
		case SC_AUTHOR_FONT_SIZE:parser_params.scAuthor_ = QString::fromUtf8(value); 
					remove_newlines(&(parser_params.scAuthor_)); break;
		case SC_LAST_AUTHOR_FONT_SIZE:parser_params.scLastAuthor_ = QString::fromUtf8(value);
					remove_newlines(&(parser_params.scLastAuthor_)); break;
		case SC_COMMENT_FONT_SIZE:parser_params.scComment_ = QString::fromUtf8(value);
					remove_newlines(&(parser_params.scComment_)); break;
		case SC_COPYRIGHT_FONT_SIZE:parser_params.scCopyright_ = QString::fromUtf8(value);
					remove_newlines(&(parser_params.scComment_)); break;
	}
}

static void MUP2lyrics(char *s) {
	char *cptr1, *cptr2, *cptr3;

	cptr1 = s; 
	while (*cptr1) {
		switch (*cptr1) {
			case '~': *cptr1 = '-'; cptr1++; break;
			case '\\':
				for (cptr2 = cptr1,  cptr3 = cptr1+1; *cptr2; cptr2++, cptr3++) {
					*cptr2 = *cptr3;
				}
				break;
			default: cptr1++;
				break;
		}
	}
}

static void insertChordDiagramm(unsigned int at, char *chordname, NMusElement *last_bar_sym) {
	chordDiagramName *diag_name;
	for (diag_name = chordDiagramList.first(); diag_name; diag_name = chordDiagramList.next()) {
		if (diag_name->cdiagramm->getChordName() == chordname) {
			if (!current_voice->insertChordDiagrammAt(at, diag_name->cdiagramm->clone(), last_bar_sym)) {
				NResource::abort("insertChord", 1);
			}
			return;
		}
	}
	if (!current_voice->insertChordDiagrammAt(at, new NChordDiagram(chordname), last_bar_sym))  {
		NResource::abort("insertChord", 2);
	}
}

static void insert_segno_accel_and_ritard(unsigned int at, int type, NMusElement *last_bar_sym) {
	if (!type) return; /* tempo */
	if (!current_voice->insertSegnoRitardAndAccelAt(at, type, last_bar_sym))  {
		NResource::abort("insert_segno_accel_and_ritard");
	}
}

static void update_voice_stack(int voice_nr, int incr) {
	struct slur_stack_str *slur_stack_elem;
	int i;
	if (!pending_elements[voice_nr].slur_stack->isEmpty()) {
		for (i = 0, slur_stack_elem = pending_elements[voice_nr].slur_stack->first(); slur_stack_elem;
			slur_stack_elem = pending_elements[voice_nr].slur_stack->next(), i++) {
			slur_stack_elem->slurdist += incr;
		}
	}
}

			
#ifdef YACCDEBUG
int main (int argc, char *argv[]) {
	extern FILE *yyin;
	extern int yydebug;
	if (argc != 2) {
		fprintf(stderr, "Benutzung: %s <file>\n", argv[0]);
		exit(-1);
	}

	if ((yyin = fopen(argv[1], "r")) == NULL) {
		fprintf(stderr, "Fehler bei der Eroeffnung von %s\n", argv[1]);
		exit(-1);
	}

	yydebug = 0;
	if (yyparse() == 0) {
		printf("Syntax OK\n");
	}
	return 0;
}
#endif

