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

#ifndef TUPLETDIALOG_IMPL_H
class NMainFrameWidget;
#include "tupletdialog.h"

class tupletDialogImpl : public tupletDialog {
	public: 
		tupletDialogImpl( QWidget* parent = 0, const char* name = 0 );
		virtual void show();
	public slots:
    		virtual void noteNumberChanged(int);
    		virtual void slot_canc();
    		virtual void slot_ok();
	private:
		NMainFrameWidget *mainFrameWidget_;
};

#define TUPLETDIALOG_IMPL_H

#endif /* TUPLETDIALOG_IMPL_H */
