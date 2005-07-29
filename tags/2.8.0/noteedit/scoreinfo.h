/****************************************************************************/
/*                                                                          */
/* This program is free software; you can redistribute it and/or modify it  */
/* under the terms of the GNU General Public License as published by the    */
/* Free Software Foundation; either version 2 of the License, or (at your   */
/* option) any later version.                                               */
/*                                                                          */
/* This program is distributed in the hope that it will be useful, but      */
/* WITHOUT ANY WARRANTY; without even the implied warranty of               */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General */
/* Public License for more details.	                                        */
/*                                                                          */
/* You should have received a copy of the GNU General Public License along  */
/* with this program; (See "LICENSE.GPL"). If not, write to the Free        */
/* Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA       */
/* 02111-1307, USA.                                                         */
/*                                                                          */
/*--------------------------------------------------------------------------*/
/*                                                                          */
/*    Erik Sigra, SWEDEN                                                    */
/*    sigra@home.se                                                         */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

#ifndef SCOREINFO_H
#define SCOREINFO_H

#include <kcombobox.h>
#include <kdialogbase.h>

#include <qmultilineedit.h>

#include "mainframewidget.h"

class ScoreInfoDialog : public KDialogBase {
	Q_OBJECT

public:
	ScoreInfoDialog(NMainFrameWidget *parent);

private slots:
	void slotApply();
	void slotUser1();
	void saveComboData();

private:

	NMainFrameWidget *mainWidget;

	//  General
	KHistoryCombo *title;
	KHistoryCombo *subject;
	KHistoryCombo *author;
	KHistoryCombo *lastAuthor;
	KHistoryCombo *copyright;

	//  Comments
	QMultiLineEdit *comments;

};

#endif //  SCOREINFO_H
