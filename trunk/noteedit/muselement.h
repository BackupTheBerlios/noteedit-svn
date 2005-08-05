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

#ifndef MUSELEMENT_H

#define MUSELEMENT_H
#include <qpoint.h>
#include <qrect.h>
#include "resource.h"

#define LINE_DIST 21
#define LINE_OVERFLOW 12
#define MINLINE (-LINE_OVERFLOW)
#define MAXLINE (8 + LINE_OVERFLOW)
#define STAFF_HIGHT (4*LINE_DIST)

#define T_CHORD (1 << 0)
#define T_REST (1 << 1)
#define T_SIGN (1 << 2)
#define T_CLEF (1 << 3)
#define T_KEYSIG (1 << 4)
#define T_TIMESIG (1 << 5)
#define T_TEXT (1 << 6)

#define PLAYABLE (T_CHORD | T_REST)

// please study uiconnect before you make any clef changes!
#define TREBLE_CLEF (1 << 0)
#define BASS_CLEF (1 << 1)
#define SOPRANO_CLEF (1 << 2)
#define ALTO_CLEF (1 << 3)
#define TENOR_CLEF (1 << 4)
#define DRUM_CLEF (1 << 5)
#define DRUM_BASS_CLEF (1 << 6)

// signs:

#define TEMPO_SIGNATURE 3
#define VOLUME_SIG      5
#define PROGRAM_CHANGE  6
#define TRILL		7
#define LNTRILL		9
#define DYNAMIC		10	
#define VA8		11
#define VA8_BASSA	12
#define CDIAGRAM	13
#define SEGNO		14
#define DAL_SEGNO	15
#define DAL_SEGNO_AL_FINE 17
#define DAL_SEGNO_AL_CODA 18
#define FINE		19
#define CODA		20
#define RITARDANDO	21
#define ACCELERANDO	22
#define SIMPLE_BAR      (1 << 8)
#define REPEAT_OPEN     (1 << 9)
#define REPEAT_CLOSE    (1 << 10)
#define REPEAT_OPEN_CLOSE (1 << 11)
#define DOUBLE_BAR      (1 << 12)
#define SPECIAL_ENDING1 (1 << 13)
#define SPECIAL_ENDING2 (1 << 14)
#define END_BAR		(1 << 15)

// special rest
#define MULTIREST	23

// for internal use / control

#define PAGE_TURN_OVER  22

#define SPECIAL_ENDING (SPECIAL_ENDING1 | SPECIAL_ENDING2)
#define BAR_SYMS (END_BAR | REPEAT_OPEN | REPEAT_CLOSE | SIMPLE_BAR | REPEAT_OPEN_CLOSE | DOUBLE_BAR)

#define TUPLET_HEIGHT 15
#define TUPLET_DGIT_DIST 39

#define DRAW_INDIRECT 0
#define DRAW_DIRECT_BLACK (1 << 0)
#define DRAW_DIRECT_RED (1 << 1)
#define DRAW_INDIRECT_GREY (1 << 2)
#define DRAW_NO_HIDDEN_REST (1 << 3)

#define STEM_POL_UP 0
#define STEM_POL_INDIVIDUAL 1
#define STEM_POL_DOWN 2


#define CPOINT_X_OFFS -10
#define CPOINT_Y_OFFS -130

class muselement;
class NClef;
class NKeySig;
class NChord;
class NTransPainter;
class NNumberDisplay;
class NChordDiagram;

class staff_props_str {
	public:
		int base;
		int lyricsdist;
		bool is_actual;
		int measureLength;
		NKeySig *actual_keysig;
};

class main_props_str {
	public:
		main_props_str();
		~main_props_str();
		int dotcount;
		bool hidden;
		bool tied;
		bool triplet;
		bool staccato;
		bool sforzato;
		bool portato;
		bool strong_pizzicato;
		bool sforzando;
		bool fermate;
		bool arpeggio;
		bool grace;
		bool pedal_on;
		bool pedal_off;
		unsigned int noteBody;
		int actualLength;
		int actualStemDir;
		NTransPainter *p;
		NTransPainter *tp;
		NTransPainter *directPainter;
		NNumberDisplay *voiceDisplay;
		QFont scaledText_;
		QFont scaledItalic_;
		QFont scaledMiniItalic_;
		QFont scaledBoldItalic_;
		QFont scaledBold_;
		QFont scaledBold2_;
		QFontMetrics scaledBoldItalicMetrics_;
		float zoom; /* zoom value in scaler */
		int left_page_border;
		int context_clef_xpos;
		int context_keysig_xpos;
};

#define MULTIPLICATOR       (1*2*3*4*5*6*7) /* enable x-tuplets with x in {3,4,5,6,7,8,9,10} */

#define DOUBLE_WHOLE_LENGTH (256*MULTIPLICATOR)
#define WHOLE_LENGTH        (128*MULTIPLICATOR)
#define HALF_LENGTH         ( 64*MULTIPLICATOR)
#define QUARTER_LENGTH      ( 32*MULTIPLICATOR)
#define NOTE8_LENGTH        ( 16*MULTIPLICATOR)
#define NOTE16_LENGTH       (  8*MULTIPLICATOR)
#define NOTE32_LENGTH       (  4*MULTIPLICATOR)
#define NOTE64_LENGTH       (  2*MULTIPLICATOR)
#define NOTE128_LENGTH      (  1*MULTIPLICATOR)

#define INTERNAL_GRACE_MIDI_LENGTH (NOTE64_LENGTH)
#define INTERNAL_MARKER_OF_STROKEN_GRACE (NOTE32_LENGTH)

class NNote {
	public:
		QPixmap *bodyPixmap;
		QPixmap *redBodyPixmap;
		QPixmap *greyBodyPixmap;
		Q_INT8 line;
		Q_INT8 offs;
		short midiPitch; /* during replay */
		QPoint nbase_draw_point;
		QPoint acc_draw_point;
		QRect point_pos1;
		QRect point_pos2;
		NNote *tie_forward, *tie_backward;
		char acc_offs;
		char needed_acc;
		char acc_TeX_pos;
		unsigned int status;

// definition of status bits in NMusElement::status_
// TBD and also in NNote::status ?

#define STAT_SINGLE_DOT   (1 << 0)
#define STAT_DOUBLE_DOT   (1 << 1)
#define DOT_MASK     (STAT_SINGLE_DOT | STAT_DOUBLE_DOT)

#define STAT_HIDDEN   (1 << 2) /* conflicts with "STAT_CROSS", but can only occur in rests anyway. Cannot add more than 32 bits for status */

#define STAT_CROSS   (1 << 2)
#define STAT_FLAT    (1 << 3)
#define STAT_DCROSS  (1 << 4)
#define STAT_DFLAT   (1 << 5)
#define STAT_NATUR   (1 << 6)
#define STAT_NO_ACC  0

#define ACC_MASK     (STAT_CROSS | STAT_FLAT | STAT_DCROSS | STAT_DFLAT | STAT_NATUR)

#define STAT_BEAMED  (1 << 7)
#define STAT_SLURED  (1 << 8)
#define STAT_PART_OF_SLUR  (1 << 9)
#define STAT_TUPLET   (1 << 10)
#define STAT_LAST_TUPLET (1 << 11)
#define STAT_STEM_UP (1 << 12)
#define STAT_STEM_UP_BEFORE_BEAM (1 << 13)
#define STAT_STACC   (1 << 14)
#define STAT_TIED    (1 << 15)
#define STAT_PART_OF_TIE (1 << 16)
#define STAT_FORCE   (1 << 17)
#define STAT_SHIFTED (1 << 18)
#define STAT_VIRTUAL (1 << 19)
// Note: If you want to change the values of sforzato - sforzando, study function setAccent(*) at first!
// sforzato
#define STAT_SFORZ   (1 << 20)
// portato
#define STAT_PORTA   (1 << 21)
// strong pizzicato
#define STAT_STPIZ   (1 << 22)
// sforzando
#define STAT_SFZND   (1 << 23)
// fermate
#define STAT_FERMT   (1 << 24)
// body as cross
#define STAT_BODY_CROSS (1 << 25)
// body as alternative cross
#define STAT_BODY_CROSS2 (1 << 26)
// body as cross with circle
#define STAT_BODY_CIRCLE_CROSS (1 << 27)
// body as rectangle
#define STAT_BODY_RECT (1 << 28)
// body as triangle
#define STAT_BODY_TRIA (1 << 29)
// grace note
#define STAT_GRACE (1 << 30)
// with arpeggio
#define STAT_ARPEGG (1 << 31)




#define CHORD_STAT_PART ( STAT_SINGLE_DOT | STAT_DOUBLE_DOT | STAT_BEAMED | STAT_SLURED | \
	 STAT_PART_OF_SLUR | STAT_TUPLET | STAT_LAST_TUPLET | STAT_STEM_UP | STAT_STACC | \
	 STAT_SFORZ | STAT_PORTA | STAT_STPIZ | STAT_SFZND | STAT_FERMT | STAT_GRACE | STAT_ARPEGG)

#define GRACE_STAT_PART (  STAT_BEAMED | STAT_SLURED | STAT_PART_OF_SLUR | STAT_STEM_UP | STAT_GRACE )

#define NOTE_STAT_PART (~CHORD_STAT_PART)

#define BODY_MASK ( STAT_BODY_CROSS | STAT_BODY_CROSS2 | STAT_BODY_CIRCLE_CROSS |\
			STAT_BODY_RECT | STAT_BODY_TRIA )

#define SET_STATUS(condition, stat_var, stat_bit) if (condition) {stat_var |= stat_bit;} else {stat_var &= (~stat_bit);}
		QPoint tie_start_point_up, tie_start_point_down;
		QPoint tie_forward_point_up, tie_forward_point_down;
		QPoint tie_back_point_up, tie_back_point_down;
		NChord *chordref;
		short TeXTieNr;
};

class NMusElement {
	public :
		NMusElement(main_props_str *main_props, staff_props_str *staff_props);
		void change(NMusElement *elem);
		unsigned int status_;
		unsigned int status2_;
// definition of status bits in NMusElement::status2_
#define STAT2_PEDAL_ON (1 << 0)
#define STAT2_PEDAL_OFF (1 << 1)
#define STAT2_AUTO_TRIPLET (1 << 2)
		int trill_;
		int dynamic_;
		int va_;
		bool dynamicAlign_;
		int slurY_;
		virtual NMusElement *clone() = 0;
		virtual ~NMusElement();
		virtual void draw(int flags = 0) = 0;
		virtual void setStaffProps(staff_props_str *staff_props) {staff_props_ = staff_props;}
		virtual void setMainProps(main_props_str *main_props) {main_props_ = main_props;}
		virtual void moveUp(int, int, NKeySig *) {};
		virtual void moveDown(int, int, NKeySig *) {};
		virtual void moveSemiToneUp(int, NClef *, NKeySig *) {}
		virtual void moveSemiToneDown(int, NClef *, NKeySig *) {}
		virtual void changeLength(int) {};
		virtual void changeBody(unsigned int) {};
		virtual void changeOffs(int, NKeySig *) {}
		virtual void setActualTied(bool) {};
		virtual int getSubType() const {return 0;}
		virtual QRect *getBbox () {return &bbox_;} 
		virtual QList<NNote> *getNoteList() {return 0;}
		virtual bool deleteNoteAtLine(int, int) {return false;}
		virtual void deletePart(NNote *) {};
		virtual NNote *searchLine(int , int) { return 0;}
		virtual NNote *insertNewNote(int , int , int , unsigned int ) {return 0;}
		virtual void insertNewNote(NNote *) {};
		virtual NChordDiagram *getChordChordDiagram() {return 0;}
		void reposit(int xpos, int sequNr_);
		virtual int getXposDecorated() {return xpos_;}
		int getXpos() {return xpos_;}
		int getSequNr() {return sequNr_;}
		void setActual(bool ac) {actual_ = ac;}
		virtual int getType () const  = 0;
		int intersects(const QPoint p) const;
		virtual int intersects_horizontally(const QPoint p) const {return intersects(p);} /* for "normal" elements except NChords */
		virtual void setDotted(int) {}
		virtual int getMidiLength (bool = false) const {return 0;}
		virtual bool lastBeamed() {return false;}
		void breakTuplet();
		virtual void breakBeames() {}
		virtual QPoint *getTopY() {return 0;}
		virtual int getTopY2() {return 0;}
		virtual int getTopX2() {return 0;}
		virtual double getBotY() {return 0.0;}
		virtual void addChordDiagram(NChordDiagram *) {}
		virtual void removeChordDiagram() {}
		virtual char getNumNotes() {return 3;}
		virtual char getPlaytime() {return 2;}
		void computeTuplet();
		static void computeTuplet(QList<NMusElement> *tupletlList, char numNotes, char playtime);
		virtual void setTupletParams(QList<NMusElement> *, bool, double, double, double, int, int, char, char) {}
		void changeTupletList(QList<NMusElement> *tList) {tupletList_ = tList;}
		void unsetTuplet();
		QList<NMusElement> *getTupletList() {return tupletList_;}
		void resetTupletFlag() { status_ &= (~((STAT_TUPLET | STAT_LAST_TUPLET))); tupletList_ = 0; midiLength_ = computeMidiLength();}
		int midiTime_;
		virtual void calculateDimensionsAndPixmaps() = 0;
		QString *computeTeXTuplet(NClef *clef);
		bool isFirstInTuplet() {return tupletList_->first() == this;}
		virtual int computeMidiLength() const {return 0;}
	protected:
		bool actual_;
		int pixmapHeight_;
		int pixmapWidth_;
		QRect bbox_;
		staff_props_str *staff_props_;
		main_props_str *main_props_;
		int xpos_;
		int sequNr_;
		int midiLength_;
		double tupm_; double tupn_;
		double tupTeXn_;
		int xstart_, xend_;
		QList<NMusElement> *tupletList_;	// all elements in this element's tuplet
};

#endif // MUSELEMENT_H
