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

#ifndef TIMESIG_H

#define TIMESIG_H

#include <qpoint.h>
#include "muselement.h"

class QPixmap;

class NTimeSig : public NMusElement {
	public:
		NTimeSig(main_props_str *main_props, staff_props_str *staff_props);
		virtual NTimeSig *clone();
		void setSignature(int numerator, int denominator);
		void setSignature(NTimeSig *otherTimeSig);
		int getNumerator() {return numerator_;}
		int getDenominator() {return denominator_;}
		virtual void draw(int flags = 0);
		virtual int getType() const {return T_TIMESIG;}
		int numOf128th() const;
		virtual void calculateDimensionsAndPixmaps();
	private:
		QPoint numDrawPoint_, denomDrawPoint_;
		int numerator_, denominator_;
		QString numString_, denomString_;
		
};

#endif // TIMESIG_H
