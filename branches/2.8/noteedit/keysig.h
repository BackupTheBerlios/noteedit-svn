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

#ifndef KEYSIG_H

#define KEYSIG_H

#include "muselement.h"

class NClef;

class NKeySig : public NMusElement {
	public:
		NKeySig(main_props_str *main_props, staff_props_str *staff_props);
		virtual ~NKeySig();
		void change(NKeySig *ksig);
		virtual NKeySig *clone();
		void reset();
		void changeHalfTone(NNote *note);
		void resetAtBar();
		void setKey(int note, int kind);
		int getState_of(int note);
		void setPreviousKeySig(NKeySig *prevKeySig);
		void setClef(NClef *ac_clef);
		void setRegular(int count, int kind);
		bool isRegular(int *kind, int *count);
		int accNeeded(int line, int offs);
		int computeOffs(int line);
		int determineDistanceUp(NNote *note);
		int getState(int note);
		int chooseOffs(int line, int offs);
		void setTempAcc(int line, int kind);
		virtual int getType () const {return T_KEYSIG;}
		void draw(int flags = 0);
		char *printKeys();
		void addSign(int kind, char pitch);
		bool isDrawable() {return !noSignes_;}
		bool isEqual(NKeySig *otherKeysig);
		virtual void calculateDimensionsAndPixmaps();
/* #define KEYSIG_DEBUG */
#ifdef KEYSIG_DEBUG
		void print();
#endif
/* ------------------------------- methods for context keysig ------------------------------------------*/
		void drawContextKeySig();
		void setClefInContextKeysig(NClef * ac_clef);
		void changeInContextKeySig(NKeySig *ksig);
		void calculateContextPixmap();
	private:
		int accCount();
		int line2Range(int line);
		bool isRegular(int *kind);
		char *noteState_;
		char *tempNoteState_;
		static int nameTab_[7];
		static int crossTab_[7];
		static int flatTab_[7];
		static char str[128];
		NClef *acClef_;
		int pixmapWidth_, resPixmapWidth_;
		bool stateChanged_;
		bool noSignes_;
		int resolvOffs_;
		QPixmap *keyPixmap_;
		QPixmap *key_redPixmap_;
		QPixmap *resolvPixmap_;
		QPixmap *resolv_redPixmap_;
		QPoint nbaseDrawPoint_;
		QPoint resolvDrawPoint_;
		NKeySig *previousKeySig_;
		NKeySig *computedPreviousKeySig_;
		static NClef defaultClef_;
};
	


#endif // KEYSIG_H
