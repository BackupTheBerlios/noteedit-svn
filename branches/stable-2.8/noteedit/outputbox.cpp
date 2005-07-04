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
/* Public License for more details.	                                    */
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

#undef KDE_NO_COMPAT
#include <kapp.h>
#include <kdialogbase.h>
#include <klocale.h>

#include <qlabel.h>
#include <qlayout.h>
#include <qmessagebox.h>
#include <qmultilineedit.h>
#include <qvbox.h>
#if QT_VERSION >= 300
#include <qstyle.h>
#endif

#include "outputbox.h"

void OutputBox::warning
	(QWidget *parent, const QString & message,
	 const QString & output, const QString & caption
	)
{

	KDialogBase *dialog = new KDialogBase   //  Arguments:
		(parent,                              //  parent
		 "OutputBox",                         //  name
		 true,                                //  modal
		 kapp->makeStdCaption(i18n(caption)), //  caption
		 KDialogBase::Ok                      //  buttonMask
		);

	QVBox *contents = dialog->makeVBoxMainWidget();
	contents->setSpacing(KDialog::spacingHint());
	contents->setMargin(KDialog::marginHint());

	QWidget *topContents = new QWidget(contents);
	QHBoxLayout * topLayout = new QHBoxLayout(topContents);
	topLayout->setSpacing(KDialog::spacingHint());

	topLayout->addStretch(1);
	QLabel *Image = new QLabel(topContents);
	Image->setPixmap
		(QMessageBox::standardIcon
			(QMessageBox::Warning        //  Can be NoIcon, Information, Warning or Critical.
#if QT_VERSION < 300
			 ,kapp->style().guiStyle()
#endif
			)
		);
	topLayout->add(Image);
	QLabel *Text = new QLabel(message, topContents);
	Text->setMinimumSize(Text->sizeHint());
	topLayout->add(Text);
	topLayout->addStretch(1);

#if QT_VERSION >= 300
	QTextEdit *OutputTarget = new QTextEdit(contents);
#else
	QMultiLineEdit *OutputTarget = new QMultiLineEdit(contents);
#endif
	OutputTarget->setText(output);
	OutputTarget->setReadOnly(true);
	OutputTarget->setMinimumSize(648, 243);

	dialog->exec();
	delete dialog;
}
