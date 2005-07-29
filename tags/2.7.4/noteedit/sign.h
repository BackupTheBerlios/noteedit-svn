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

#ifndef SIGN_H

#define SIGN_H

#define V_PPPIANO 0
#define V_PPIANO  1
#define V_PIANO   2
#define V_MPIANO  3
#define V_MEZZO   4
#define V_FORTE   5
#define V_FFORTE  6
#define V_FFFORTE 7

#include "muselement.h"

class QPixmap;

class NSign : public NMusElement {
	public:
		NSign(struct main_props_str *main_props, staff_props_str *staff_props, int type);
		void setBarNr(int barNr);
		void setTempo(int tempo);
		int getTempo() {return values_.tempo;}
		int getVolume() {return values_.volume;}
		int getVolType() {return volType_;}
		void setVolume(int vol_type, int volume);
		void setProgram(int prg);
		void setRealMidiTime(int realTime) {u1_.realMidiTime = realTime;}
		int getRealMidiTime() {return u1_.realMidiTime;}
		int getProgram() {return program_;}
		int getBarNr() {return u1_.barNr;}
		void setRepeatCount(int count) {values_.repeatCount = count;}
		int getRepeatCount() {return values_.repeatCount;}

		virtual NSign *clone();
		virtual void draw(int flags = 0);
		virtual int getSubType() const {return signType_;}
		virtual int getType() const {return T_SIGN;}
		virtual void calculateDimensionsAndPixmaps();
	private:
		QPixmap *redPixmap_;
		QPixmap *blackPixmap_;
		QPoint nbaseDrawPoint_;
		QPoint nbaseDrawPoint1_;
		QPoint nbaseDrawPoint2_;
		QPoint nbaseDrawPoint3_;
		QPoint numDrawPoint_;
		QRect nbaseEllipse_;
		int signType_;
		QString valString_;
		int volType_;
		union {
			int volume;
			int tempo;
			int repeatCount;
		} values_;
		union {
			int barNr;
			int realMidiTime;
		} u1_;
		int program_;
};

#endif // SIGN_H
