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

#include "tupletdialog_impl.h"
#include "mainframewidget.h"
#include <qslider.h>
#include <qlcdnumber.h>
#include <stdio.h>

tupletDialogImpl::tupletDialogImpl( QWidget* parent, const char* name ) : tupletDialog(parent, name) {
	mainFrameWidget_ = (NMainFrameWidget *)  parent;
	numNotes->setRange (2, 10);
	playTime->setRange (1, 9);
	numNotes->setValue(3);
	playTime->setValue(2);
	numNotesDisplay->display(3);
	playLengthDisplay->display(2);
}

void tupletDialogImpl::noteNumberChanged(int nr) {
	if (nr == 2) {
		playTime->setRange ( 1, 3);
		playTime->setValue(3);
		return;
	}
	playTime->setRange ( 1, nr - 1);
	playTime->setValue(nr - 1);
}

void tupletDialogImpl::slot_canc() {
	hide();
}

void tupletDialogImpl::slot_ok() {
	mainFrameWidget_->createTuplet(numNotes->value(), playTime->value());
	hide();
}

void tupletDialogImpl::show() {
	numNotes->setRange (2, 10);
	playTime->setRange (1, 9);
	numNotes->setValue(3);
	playTime->setValue(2);
	numNotesDisplay->display(3);
	playLengthDisplay->display(2);
	tupletDialog::show();
}
	
