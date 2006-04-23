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

#include <kapp.h>
#include <kconfig.h>
#include <klocale.h>

#include <qlabel.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#if QT_VERSION >= 300
#include <qgrid.h>
#endif

#include "scoreinfo.h"

ScoreInfoDialog::ScoreInfoDialog(NMainFrameWidget *parent) :
	KDialogBase
		(Tabbed,                             //  dialogFace
		 kapp->makeStdCaption(i18n("Score information")),
		 Help | User1 | User2 | Ok | Apply | Cancel,
		 Ok,                                 //  defaultButton
		 parent,                             //  parent
		 "ScoreInfoDialog",                  //  name (for internal use only)
		 true,                               //  modal
		 true,                               //  separator
		 i18n("&Revert"),                    //  User1
		 i18n("Cl&ear all")                  //  User1
		),
	mainWidget(parent) {

	kapp->config()->setGroup("ScoreInfo");


	//  GENERAL

	QFrame *pageGeneral = addPage(i18n("&General"));
	QGridLayout *layoutGeneral = new QGridLayout(pageGeneral, 5, 2);
	layoutGeneral->setSpacing(KDialog::spacingHint());
	layoutGeneral->setColStretch(1 /* the right column */, 1 /*factor*/);

	//  Title
	title = new KHistoryCombo(pageGeneral);
	title->setHistoryItems(kapp->config()->readListEntry("TitleHistory"), true);
	title->setEditText(mainWidget->scTitle_);
	layoutGeneral->addWidget(title, 0, 1);
	QLabel *titleLabel = new QLabel(title, i18n("&Title:"), pageGeneral);
	layoutGeneral->addWidget(titleLabel, 0, 0);

	//  Subject
	subject = new KHistoryCombo(pageGeneral);
	subject->setHistoryItems
		(kapp->config()->readListEntry("SubjectHistory"), true);
	subject->setEditText(mainWidget->scSubtitle_);
	layoutGeneral->addWidget(subject, 1, 1);
	QLabel *subjectLabel = new QLabel(subject, i18n("&Subject:"), pageGeneral);
	layoutGeneral->addWidget(subjectLabel, 1, 0);

	//  Composer
	author = new KHistoryCombo(pageGeneral);
	author->setHistoryItems
		(kapp->config()->readListEntry("AuthorHistory"), true);
	author->setEditText(mainWidget->scAuthor_);
	layoutGeneral->addWidget(author, 2, 1);
	QLabel *authorLabel = new QLabel(author, i18n("Compo&ser:"), pageGeneral);
	layoutGeneral->addWidget(authorLabel, 2, 0);

	//  Arranger
	lastAuthor = new KHistoryCombo(pageGeneral);
	lastAuthor->setHistoryItems
		(kapp->config()->readListEntry("LastAuthorHistory"), true);
	lastAuthor->setEditText(mainWidget->scLastAuthor_);
	layoutGeneral->addWidget(lastAuthor, 3, 1);
	QLabel *lastAuthorLabel = new QLabel
		(lastAuthor, i18n("&Arranger:"), pageGeneral);
	layoutGeneral->addWidget(lastAuthorLabel, 3, 0);


	// Copyright
	copyright = new KHistoryCombo(pageGeneral);
	copyright->setHistoryItems
		(kapp->config()->readListEntry("Copyright"), true);
	copyright->setEditText(mainWidget->scCopyright_);
	layoutGeneral->addWidget(copyright, 4, 1);
	QLabel *copyrightLabel = new QLabel
		(lastAuthor, i18n("&Copyright:"), pageGeneral);
	layoutGeneral->addWidget(copyrightLabel, 4, 0);



	//  COMMENTS
	QGrid *pageComments = addGridPage(1, QGrid::Horizontal, i18n("Co&mments"));
	comments = new QMultiLineEdit(pageComments);
	comments->setText(mainWidget->scComment_);
	comments->setSizePolicy
		(QSizePolicy
			(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding)
		);

	connect(this, SIGNAL(finished()), this, SLOT(saveComboData()));
	connect(this, SIGNAL(user2Clicked()), title,      SLOT(clearEdit()));
	connect(this, SIGNAL(user2Clicked()), subject,    SLOT(clearEdit()));
	connect(this, SIGNAL(user2Clicked()), author,     SLOT(clearEdit()));
	connect(this, SIGNAL(user2Clicked()), lastAuthor, SLOT(clearEdit()));
	connect(this, SIGNAL(user2Clicked()), copyright, SLOT(clearEdit()));
	connect(this, SIGNAL(user2Clicked()), comments,   SLOT(clear()));
	connect(this, SIGNAL(okClicked()), this, SLOT(slotApply()));
	connect(this, SIGNAL(okClicked()), this, SLOT(hide()));

}

void ScoreInfoDialog::saveComboData() {
	//  if this dialog is ever made modeless, don't forget to add a call to
	//  kapp->config()->setGroup("ScoreInfo") here.
	kapp->config()->writeEntry("TitleHistory",      title     ->historyItems());
	kapp->config()->writeEntry("SubjectHistory",    subject   ->historyItems());
	kapp->config()->writeEntry("AuthorHistory",     author    ->historyItems());
	kapp->config()->writeEntry("LastAuthorHistory", lastAuthor->historyItems());
	kapp->config()->writeEntry("Copyright",         copyright->historyItems());
}

void ScoreInfoDialog::slotApply() {

	if ((mainWidget->scTitle_.length() > 0 || title->currentText().length() > 0)
	    && mainWidget->scTitle_ != title->currentText()) {
		mainWidget->scTitle_ = title->currentText();
		mainWidget->setEdited();
	}
	title->addToHistory(title->currentText());

	if ((mainWidget->scSubtitle_.length() > 0
	    || subject->currentText().length() > 0)
	    && mainWidget->scSubtitle_ != subject->currentText()) {
		mainWidget->scSubtitle_ = subject->currentText();
		mainWidget->setEdited();
	}
	subject->addToHistory(subject->currentText());

	if ((mainWidget->scAuthor_.length() > 0
	    || author->currentText().length() > 0)
	    && mainWidget->scAuthor_ != author->currentText()) {
		mainWidget->scAuthor_ = author->currentText();
		mainWidget->setEdited();
	}
	author->addToHistory(author->currentText());

	if ((mainWidget->scLastAuthor_.length() > 0
	    || lastAuthor->currentText().length() > 0)
	    && mainWidget->scLastAuthor_ != lastAuthor->currentText()) {
		mainWidget->scLastAuthor_ = lastAuthor->currentText();
		mainWidget->setEdited();
	}
	lastAuthor->addToHistory(lastAuthor->currentText());

	if ((mainWidget->scCopyright_.length() > 0
	    || copyright->currentText().length() > 0)
	    && mainWidget->scCopyright_ != copyright->currentText()) {
		mainWidget->scCopyright_ = copyright->currentText();
		mainWidget->setEdited();
	}
	copyright->addToHistory(copyright->currentText());

	if ((mainWidget->scComment_.length() > 0 || comments->text().length() > 0)
	    && mainWidget->scComment_ != comments->text()) {
		mainWidget->scComment_ = comments->text();
		mainWidget->setEdited();
	}
}

void ScoreInfoDialog::slotUser1() { //  Revert
	title     ->setEditText(mainWidget->scTitle_);
	subject   ->setEditText(mainWidget->scSubtitle_);
	author    ->setEditText(mainWidget->scAuthor_);
	lastAuthor->setEditText(mainWidget->scLastAuthor_);
	copyright ->setEditText(mainWidget->scCopyright_);
	comments  ->setText    (mainWidget->scComment_);
}

#include "scoreinfo.moc"
