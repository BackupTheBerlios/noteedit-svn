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

#ifndef CLEF_H

#define CLEF_H

#include "muselement.h"

class QPixmap;

class staffInfoClass {
	public:
		int clefType, clefShift;
		unsigned int minMidi, maxMidi;
		unsigned int volume, channel;
};
	

class NClef : public NMusElement {
	public:
		NClef(main_props_str *main_props, staff_props_str *staff_props, int kind = TREBLE_CLEF, int shift = 0);
		void change(NClef *clef);
		void changeKind(int kind);
		void changeShift(int shift);
		virtual NClef *clone();
		virtual void draw(int flags = 0);
		void drawContextClef();
		virtual int getSubType() const {return clefKind_;}
		virtual int getType() const {return T_CLEF;}
		const unsigned int *line2midiTab_;
		const char *line2TexTab_;
		int getAccPos(int kind, int nr);
		int noteNumber2Line(int note_number) const ;
		int lineOfC4();
		char line2PMXName(int line, int *octave) const;
		char line2Name(int line, int *octave, bool lilyexport, bool abcexport) const;
		int name2Line(char name) const;
		int line2note(int line) const;
		virtual void calculateDimensionsAndPixmaps();
		int getOctave();
		int getShift() {return shift_;}
		void setShift(int octave);
		void midi2Line(unsigned int midival,int *line, int *offs, NKeySig *ksig = 0);
		static int chooseClefType(staffInfoClass *staffInfos, int minMidi, int MaxMidi, bool drumchannel);
	private:
		QPixmap *redPixmap_;
		QPixmap *blackPixmap_;
		QPoint nbaseDrawPoint_;
		int clefKind_;
		int shift_;
		const char *crossPosTab_, *flatPosTab_;
		static const unsigned int line2midiTreble_[MAXLINE-MINLINE+1];
		static const unsigned int line2midiBass_[MAXLINE-MINLINE+1];
		static const unsigned int line2midiAlto_[MAXLINE-MINLINE+1];
		static const unsigned int line2midiTenor_[MAXLINE-MINLINE+1];
		static const char line2musixtexTreble_[MAXLINE-MINLINE+1];
		static const char line2musixtexBass_[MAXLINE-MINLINE+1];
		static const char line2musixtexAlto_[MAXLINE-MINLINE+1];
		static const char line2musixtexTenor_[MAXLINE-MINLINE+1];
		static const char bassCrossPos_[7];
		static const char bassFlatPos_[7];
		static const char altoCrossPos_[7];
		static const char altoFlatPos_[7];
		static const char voiCrossPos_[7];
		static const char voiFlatPos_[7];
		static const char tenorCrossPos_[7];
		static const char tenorFlatPos_[7];
		
};

#endif // CLEF_H
