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

#ifndef KEYOFFS_H

#define KEYOFFS_H

#include <qobject.h>
#include "muselement.h" /* needed for property_type */

#define KEY_OFFS_UP_DIST 20
#define KEY_OFFS_BOTTOM_DIST 0

class NKeySig;
class QButtonGroup;
class QRadioButton;

class NKeyOffs : public QObject{
	Q_OBJECT
	public:
		NKeyOffs(const char *note_name, int bu_nr, QWidget *parent, const char *name);
		void setKeysigObj(NKeySig *keysig_obj);
		void setGeometry(int xpos, int ypos, int width, int height);
		void set(property_type kind);
	protected slots:
		void updateCross(bool on);
		void updateFlat(bool on);
		void updateNatural(bool on);
	private:
		int xpos_, ypos_;
		QButtonGroup *grp_;
		QRadioButton *crossButton_;
		QRadioButton *flatButton_;
		QRadioButton *naturButton_;
		NKeySig *keysigObj_;
		int buNr_;
};


#endif // KEYOFFS_H
