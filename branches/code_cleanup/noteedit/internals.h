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

#ifndef INTERNALS_H
#define INTERNALS_H

class NNumberDisplay;
class NTransPainter;
class NChord;
class NKeySig;

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
		int lastMidiTime; /* The last MIDI time of an event that happened. eg. The cycling through elements methods find their track where to start easier. */
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
		status_type noteBody;
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
		status_type needed_acc;
		char acc_TeX_pos;
		status_type status;
		QPoint tie_start_point_up, tie_start_point_down;
		QPoint tie_forward_point_up, tie_forward_point_down;
		QPoint tie_back_point_up, tie_back_point_down;
		NChord *chordref;
		short TeXTieNr;
};

#endif /* INTERNALS_H */
