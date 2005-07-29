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

#include "textDialog_impl.h"
#include <qpushbutton.h>

NTextDialogImpl::NTextDialogImpl(QWidget *parent, char *name) :
	textDialog(parent, name, true) {

	connect(okBu, SIGNAL(clicked()), this, SLOT(slOk()));
	connect(CanelBu, SIGNAL(clicked()), this, SLOT(slCanc()));
	text_.truncate(0);
}

QString NTextDialogImpl::getText() {
	hide();
	return text_;
}

void NTextDialogImpl::slOk() {
	hide();
	text_ = TextLine->text().stripWhiteSpace();
}

void NTextDialogImpl::slCanc() {
	text_.truncate(0);
}

