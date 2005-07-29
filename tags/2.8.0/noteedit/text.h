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

#ifndef TEXT_H

#define TEXT_H
#define TEXT_UPTEXT 0
#define TEXT_DOWNTEXT 1

#include "muselement.h"

class NText : public NMusElement {
	public:
		NText(struct main_props_str *main_props, staff_props_str *staff_props, QString text, int type);
		QString getText() {return text_;}
		int destinationTime_; // used during reading;
		NMusElement * barSym_; // used during reading;
		void startTextDialog();
		virtual NText *clone();
		virtual void draw(int flags = 0);
		virtual int getSubType() const {return type_;}
		virtual int getType() const {return T_TEXT;}
		virtual void calculateDimensionsAndPixmaps();
	private:
		QString text_;
		QPoint textDrawPoint_;
		int type_;
};

#endif // TEXT_H
