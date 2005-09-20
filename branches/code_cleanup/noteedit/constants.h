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
#define property_type unsigned long long


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


/* is single-dotted */
/* NOTE! PROP_SINGLE_DOT *must* have value 1 to correctly work with dotcount and must be within 32bits not to crash as dotcount is still int and not property_type */
#define PROP_SINGLE_DOT   ((property_type)1 << 0)
/* is double-dotted */
/* NOTE! PROP_DOUBLE_DOT *must* have value 2 to correctly work with dotcount and must be within 32bits not to crash as dotcount is still int and not property_type */
#define PROP_DOUBLE_DOT   ((property_type)1 << 1)
/* has dot */
#define DOT_MASK     (PROP_SINGLE_DOT | PROP_DOUBLE_DOT)
/* is hidden rest */
#define PROP_HIDDEN   ((property_type)1 << 2)
/* has accidentals: sharp */
#define PROP_CROSS   ((property_type)1 << 3)
/* has accidentals: flat */
#define PROP_FLAT    ((property_type)1 << 4)
/* has accidentals: double-sharp */
#define PROP_DCROSS  ((property_type)1 << 5)
/* has accidentals: double-flat */
#define PROP_DFLAT   ((property_type)1 << 6)
/* has accidentals: natural */
#define PROP_NATUR   ((property_type)1 << 7)
/* doesn't have any accidentals defined at all */
#define PROP_NO_ACC  0
/* always show accidentals */
#define PROP_FORCE   ((property_type)1 << 8)
/* has any accidentals */
#define ACC_MASK     (PROP_CROSS | PROP_FLAT | PROP_DCROSS | PROP_DFLAT | PROP_NATUR)
/* is beamed */
#define PROP_BEAMED  ((property_type)1 << 9)
/* has slur */
#define PROP_SLURED  ((property_type)1 << 10)
/* has part of the slur */
#define PROP_PART_OF_SLUR  ((property_type)1 << 11)
/* has tuplet (triola) */
#define PROP_TUPLET   ((property_type)1 << 12)
/* is the end of tuplet */
#define PROP_LAST_TUPLET ((property_type)1 << 13)
/* stem direction: 1-up, 0-down */
#define PROP_STEM_UP ((property_type)1 << 14)
/* an exception with stem direction before beam */
#define PROP_STEM_UP_BEFORE_BEAM ((property_type)1 << 15)
/* has tie */
#define PROP_TIED    ((property_type)1 << 16)
/* the last part of tie */
#define PROP_PART_OF_TIE ((property_type)1 << 17)

#define PROP_SHIFTED ((property_type)1 << 18)
#define PROP_VIRTUAL ((property_type)1 << 19)
// Note: If you want to change the values of sforzato - sforzando, study function setAccent(*) at first!
/* has staccato accent */
#define PROP_STACC   ((property_type)1 << 20)
/* has sforzato accent */
#define PROP_SFORZ   ((property_type)1 << 21)
/* has portato accent */
#define PROP_PORTA   ((property_type)1 << 22)
/* has strong pizzicato accent */
#define PROP_STPIZ   ((property_type)1 << 23)
/* has sforzando accent */
#define PROP_SFZND   ((property_type)1 << 24)
/* has fermata */
#define PROP_FERMT   ((property_type)1 << 25)
/* has arpeggio */
#define PROP_ARPEGG ((property_type)1 << 26)
/* is grace note */
#define PROP_GRACE ((property_type)1 << 27)
/* body is cross */
#define PROP_BODY_CROSS ((property_type)1 << 28)
/* body is alternative cross */
#define PROP_BODY_CROSS2 ((property_type)1 << 29)
/* body is cross with circle */
#define PROP_BODY_CIRCLE_CROSS ((property_type)1 << 30)
/* body is rectangle */
#define PROP_BODY_RECT ((property_type)1 << 31)
/* body is triangle */
#define PROP_BODY_TRIA ((property_type)1 << 32)
/* pedal status values */
#define PROP_PEDAL_ON ((property_type)1 << 33)
#define PROP_PEDAL_OFF ((property_type)1 << 34)
#define PROP_AUTO_TRIPLET ((property_type)1 << 35)


#define CHORD_PROP_PART ( PROP_SINGLE_DOT | PROP_DOUBLE_DOT | PROP_BEAMED | PROP_SLURED | \
	 PROP_PART_OF_SLUR | PROP_TUPLET | PROP_LAST_TUPLET | PROP_STEM_UP | PROP_STACC | \
	 PROP_SFORZ | PROP_PORTA | PROP_STPIZ | PROP_SFZND | PROP_FERMT | PROP_GRACE | PROP_ARPEGG)

#define GRACE_PROP_PART ( PROP_BEAMED | PROP_SLURED | PROP_PART_OF_SLUR | PROP_STEM_UP | PROP_GRACE )

#define NOTE_PROP_PART (~CHORD_PROP_PART)

#define BODY_MASK ( PROP_BODY_CROSS | PROP_BODY_CROSS2 | PROP_BODY_CIRCLE_CROSS |\
			PROP_BODY_RECT | PROP_BODY_TRIA )

#define SET_NOTE_PROPERTY(condition, stat_var, stat_bit) if (condition) {stat_var |= stat_bit;} else {stat_var &= (~stat_bit);}


#endif /* CONSTANTS_H */
