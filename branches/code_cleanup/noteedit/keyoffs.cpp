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

#include <qbuttongroup.h>
#include <qradiobutton.h>
#include <iostream.h>
#include "keyoffs.h"
#include "resource.h"
#include "keysig.h"

#define KEY_OFFS_LEFT_DIST 5
#define KEY_OFFS_BUTTONWIDTH 20

NKeyOffs::NKeyOffs(const char *note_name, int bu_nr, QWidget *parent, const char *name) {
	grp_ = new QButtonGroup(parent, name);
	grp_->setTitle(note_name);
	grp_->setAlignment(AlignCenter);
	crossButton_ = new QRadioButton("cross", parent, "cross_select");
	flatButton_ = new QRadioButton("flat", parent, "flat_select");
	naturButton_ = new QRadioButton("natural", parent, "natur_select");
	grp_->insert(crossButton_);
	grp_->insert(flatButton_);
	grp_->insert(naturButton_);
	grp_->setExclusive(true);
	buNr_ = bu_nr;
	keysigObj_ = 0;
	connect(crossButton_, SIGNAL(toggled(bool)), this, SLOT(updateCross(bool)));
	connect(flatButton_, SIGNAL(toggled(bool)), this, SLOT(updateFlat(bool)));
	connect(naturButton_, SIGNAL(toggled(bool)), this, SLOT(updateNatural(bool)));
}

void NKeyOffs::setKeysigObj(NKeySig *keysig_obj) {
	keysigObj_ = keysig_obj;
}

void NKeyOffs::set(status_type kind) {
	switch (kind) {
		case STAT_CROSS: crossButton_->setChecked(true); break;
		case STAT_FLAT: flatButton_->setChecked(true); break;
		case STAT_NATUR: naturButton_->setChecked(true); break;
		default: NResource::abort("NKeyOffs::set(): internal error"); 
	}
}


void NKeyOffs::updateCross(bool on) {
	if (!on) return;
	if (!keysigObj_) return;
	keysigObj_->setAccent(buNr_, STAT_CROSS);
}

void NKeyOffs::updateFlat(bool on) {
	if (!on) return;
	if (!keysigObj_) return;
	keysigObj_->setAccent(buNr_, STAT_FLAT);
}

void NKeyOffs::updateNatural(bool on) {
	if (!on) return;
	if (!keysigObj_) return;
	keysigObj_->setAccent(buNr_, STAT_NATUR);
}

void NKeyOffs::setGeometry(int xpos, int ypos, int width, int height) {
	int h;
	xpos_ = xpos; ypos_ = ypos;
	h = (height - KEY_OFFS_UP_DIST-KEY_OFFS_BOTTOM_DIST) / 3;
	grp_->setGeometry(xpos, ypos, width, height);
	crossButton_->setGeometry(xpos+KEY_OFFS_LEFT_DIST, ypos + KEY_OFFS_UP_DIST, width-2*KEY_OFFS_LEFT_DIST, (height - KEY_OFFS_UP_DIST - KEY_OFFS_BOTTOM_DIST) / 3);
	flatButton_->setGeometry(xpos+KEY_OFFS_LEFT_DIST, ypos + KEY_OFFS_UP_DIST + h , width-2*KEY_OFFS_LEFT_DIST, (height -  KEY_OFFS_UP_DIST - KEY_OFFS_BOTTOM_DIST) / 3);
	naturButton_->setGeometry(xpos+KEY_OFFS_LEFT_DIST, ypos + KEY_OFFS_UP_DIST + 2*h , width-2*KEY_OFFS_LEFT_DIST, (height - KEY_OFFS_UP_DIST - KEY_OFFS_BOTTOM_DIST) / 3);
}

#include "keyoffs.moc"
