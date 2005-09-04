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

#ifndef CONSTANTS_H
#define CONSTANTS_H


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

/* 64bit integer for musElement flags. Redefine it here, if compiling under non-gcc environment. */
#define status_type unsigned long long


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


// definition of status bits in NMusElement::status_
// TBD and also in NNote::status ?

/* is single-dotted */
/* NOTE! STAT_SINGLE_DOT *must* have value 1 to correctly work with dotcount and must be within 32bits not to crash as dotcount is still int and not status_type */
#define STAT_SINGLE_DOT   ((status_type)1 << 0)
/* is double-dotted */
/* NOTE! STAT_DOUBLE_DOT *must* have value 2 to correctly work with dotcount and must be within 32bits not to crash as dotcount is still int and not status_type */
#define STAT_DOUBLE_DOT   ((status_type)1 << 1)
/* has dot */
#define DOT_MASK     (STAT_SINGLE_DOT | STAT_DOUBLE_DOT)
/* is hidden rest */
#define STAT_HIDDEN   ((status_type)1 << 2)
/* has accidentals: sharp */
#define STAT_CROSS   ((status_type)1 << 3)
/* has accidentals: flat */
#define STAT_FLAT    ((status_type)1 << 4)
/* has accidentals: double-sharp */
#define STAT_DCROSS  ((status_type)1 << 5)
/* has accidentals: double-flat */
#define STAT_DFLAT   ((status_type)1 << 6)
/* has accidentals: natural */
#define STAT_NATUR   ((status_type)1 << 7)
/* doesn't have any accidentals defined at all */
#define STAT_NO_ACC  0
/* always show accidentals */
#define STAT_FORCE   ((status_type)1 << 8)
/* has any accidentals */
#define ACC_MASK     (STAT_CROSS | STAT_FLAT | STAT_DCROSS | STAT_DFLAT | STAT_NATUR)
/* is beamed */
#define STAT_BEAMED  ((status_type)1 << 9)
/* has slur */
#define STAT_SLURED  ((status_type)1 << 10)
/* has part of the slur */
#define STAT_PART_OF_SLUR  ((status_type)1 << 11)
/* has tuplet (triola) */
#define STAT_TUPLET   ((status_type)1 << 12)
/* is the end of tuplet */
#define STAT_LAST_TUPLET ((status_type)1 << 13)
/* stem direction: 1-up, 0-down */
#define STAT_STEM_UP ((status_type)1 << 14)
/* an exception with stem direction before beam */
#define STAT_STEM_UP_BEFORE_BEAM ((status_type)1 << 15)
/* has tie */
#define STAT_TIED    ((status_type)1 << 16)
/* the last part of tie */
#define STAT_PART_OF_TIE ((status_type)1 << 17)

#define STAT_SHIFTED ((status_type)1 << 18)
#define STAT_VIRTUAL ((status_type)1 << 19)
// Note: If you want to change the values of sforzato - sforzando, study function setAccent(*) at first!
/* has staccato accent */
#define STAT_STACC   ((status_type)1 << 20)
/* has sforzato accent */
#define STAT_SFORZ   ((status_type)1 << 21)
/* has portato accent */
#define STAT_PORTA   ((status_type)1 << 22)
/* has strong pizzicato accent */
#define STAT_STPIZ   ((status_type)1 << 23)
/* has sforzando accent */
#define STAT_SFZND   ((status_type)1 << 24)
/* has fermata */
#define STAT_FERMT   ((status_type)1 << 25)
/* has arpeggio */
#define STAT_ARPEGG ((status_type)1 << 26)
/* is grace note */
#define STAT_GRACE ((status_type)1 << 27)
/* body is cross */
#define STAT_BODY_CROSS ((status_type)1 << 28)
/* body is alternative cross */
#define STAT_BODY_CROSS2 ((status_type)1 << 29)
/* body is cross with circle */
#define STAT_BODY_CIRCLE_CROSS ((status_type)1 << 30)
/* body is rectangle */
#define STAT_BODY_RECT ((status_type)1 << 31)
/* body is triangle */
#define STAT_BODY_TRIA ((status_type)1 << 32)
/* pedal status values */
#define STAT_PEDAL_ON ((status_type)1 << 33)
#define STAT_PEDAL_OFF ((status_type)1 << 34)
#define STAT_AUTO_TRIPLET ((status_type)1 << 35)


#define CHORD_STAT_PART ( STAT_SINGLE_DOT | STAT_DOUBLE_DOT | STAT_BEAMED | STAT_SLURED | \
	 STAT_PART_OF_SLUR | STAT_TUPLET | STAT_LAST_TUPLET | STAT_STEM_UP | STAT_STACC | \
	 STAT_SFORZ | STAT_PORTA | STAT_STPIZ | STAT_SFZND | STAT_FERMT | STAT_GRACE | STAT_ARPEGG)

#define GRACE_STAT_PART ( STAT_BEAMED | STAT_SLURED | STAT_PART_OF_SLUR | STAT_STEM_UP | STAT_GRACE )

#define NOTE_STAT_PART (~CHORD_STAT_PART)

#define BODY_MASK ( STAT_BODY_CROSS | STAT_BODY_CROSS2 | STAT_BODY_CIRCLE_CROSS |\
			STAT_BODY_RECT | STAT_BODY_TRIA )

#define SET_STATUS(condition, stat_var, stat_bit) if (condition) {stat_var |= stat_bit;} else {stat_var &= (~stat_bit);}


#endif /* CONSTANTS_H */
