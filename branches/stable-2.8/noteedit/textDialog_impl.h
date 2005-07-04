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

#ifndef TEXTDIALOG_IMPL_H

#define TEXTDIALOG_IMPL_H
#include <qstring.h>
#include <qcombobox.h>
#include <qlineedit.h>
#include "textDialog.h"

class NTextDialogImpl : public textDialog {
	Q_OBJECT
	public:
		NTextDialogImpl(QWidget *parent = 0, char *name = 0);
		QString getText();
		bool isUpText() {return textType->currentItem() == 0;}
		void setText(QString text) {TextLine->setText(text);}
		void setTextType(int type) {textType->setCurrentItem(type);}
	private slots:
		void slOk();
		void slCanc();
	private:
		QString text_;
};


#endif /* TEXTDIALOG_IMPL_H */
