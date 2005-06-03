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
/* Public License for more details.                                         */
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

#include <stdio.h>
#include <kapp.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmessagebox.h>

#include <qlayout.h>
#include <qradiobutton.h>
#include <qtooltip.h>
#include <qwhatsthis.h>

#include "voice.h"
#include "resource.h"
#include "mainframewidget.h"

#include "voicedialog.h"
#include "staffPropFrm.h"

VoiceBox::VoiceBox
	(QHBox *parent, VoiceDialog *voiceDialog, staffPropFrm *staffPropForm, unsigned int VoiceNumber, NVoice *voice) :
	QVBox(parent), parent_(parent), theVoice_(voice), voiceDialog_(voiceDialog), staffPropForm_(staffPropForm)  {

	this->setSpacing(KDialog::spacingHint());

	//  Stem direction selection
	stemDirection_ = new QButtonGroup(1, Horizontal, this);
	stemDirection_->setSizePolicy
		(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));

	up_ = new QRadioButton(stemDirection_);
	QToolTip::add(up_, i18n("Stems up"));

	individual_ = new QRadioButton(stemDirection_);
	QToolTip::add(individual_, i18n("Stems individual"));

	down_ = new QRadioButton(stemDirection_);
	QToolTip::add(down_, i18n("Stems down"));

	stemDirection_->setButton(voice->stemPolicy_);


	//  Rest position slider
	restPosition_ = new QSlider(-8, 8, 1, voice->yRestOffs_, QSlider::Vertical, this);
	restPosition_->setMinimumHeight(0x80);
	restPosition_->setTickmarks(QSlider::Both);
	restPosition_->setTickInterval(4);
	QToolTip::add(restPosition_, i18n("Rest position"));


	//  Delete button
	remove_ = new QPushButton(this);
	remove_->setPixmap(BarIcon("editdelete", KIcon::SizeSmall));
	remove_->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
        QToolTip::add(remove_, i18n("Delete"));
	connect(remove_, SIGNAL(clicked()), this, SLOT(destroy()));


	//  Voice number label
	voiceNumber_ = new QLabel(this);
	voiceNumber_->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
	QWhatsThis::add
		(voiceNumber_,
		 i18n("The number of the voice to which the settings above apply.")
		);

	renumber(VoiceNumber);

}

VoiceBox::~VoiceBox() {
	apply();
	delete stemDirection_;
	delete restPosition_;
	delete remove_;
}

void VoiceBox::renumber(int nr) {
	QWhatsThis::remove(up_);
	QWhatsThis::add
		(up_,
		 i18n
			("All notes in voice number %1 will have their stems pointing up when "
			 "this is selected."
			).arg(nr)
		);
	QWhatsThis::remove(individual_);
	QWhatsThis::add
		(individual_,
		 i18n
			("The direction of the stems of the notes in voice number %1 will be "
			 "chosen optimally for each note when this is selected."
			).arg(nr)
		);
	QWhatsThis::remove(down_);
	QWhatsThis::add
		(down_,
		 i18n
			("All notes in voice number %1 will have their stems pointing down "
			 "when this is selected."
			).arg(nr)
		);
	QWhatsThis::remove(restPosition_);
	QWhatsThis::add
		(restPosition_,
		 i18n("Offset in the Y dimension of rests that belong voice number %1.")
			.arg(nr)
		);
	QWhatsThis::remove(remove_);
	QWhatsThis::add
		(remove_,
		 i18n("Delete voice number %1. You will be asked for confirmation")
			.arg(nr)
		);
	voiceNumber_->setText(i18n("<center><b>%1</b></center>").arg(nr));
}


void VoiceBox::apply() {
	theVoice_->yRestOffs_ = restPosition_->value();
	if (stemDirection_->selected() == up_) {
		theVoice_->stemPolicy_ = STEM_POL_UP;
	}
	else if (stemDirection_->selected() == individual_) {
		theVoice_->stemPolicy_ = STEM_POL_INDIVIDUAL;
	}
	else if(stemDirection_->selected() == down_) {
		theVoice_->stemPolicy_ = STEM_POL_DOWN;
	}
}


void VoiceBox::destroy() {
    if(staffPropForm_)
	if (staffPropForm_->destroyVoice(this, theVoice_)) this->close(true);
    else if(voiceDialog_)
	if (voiceDialog_->destroyVoice(this, theVoice_)) this->close(true);
    else
        printf("Fatal Error: Could not destroy voice, parent widget is missing.\n");
}


VoiceDialog::VoiceDialog
	(NMainFrameWidget *mainWidget, int staffNr, QList<NStaff> *staffList) :
	KDialogBase
		(Tabbed,                               //  dialogFace
		 kapp->makeStdCaption(i18n("Voices")), //  caption
		 Help | User1 | Ok | Apply | Cancel,   //  buttonMask
		 Close,                                //  defaultButton
		 mainWidget,                           //  parent
		 "VoiceDialog",                        //  name (for internal use only)
		 true,                                 //  modal
		 true,                                 //  separator
		 i18n("C&reate voice")                 //  user1
		),
		mainWidget_(mainWidget),
		staffList_(staffList)
{
	unsigned int i = 1;
	QHBox * firstHBox = 0;
	for
		(QListIterator<NStaff> staffIterator(*staffList);
		 staffIterator.current();
		 ++staffIterator, ++i
		) {
		pageList_.append(new QList<VoiceBox>());
		QHBox *currentPage = addHBoxPage
			(staffIterator.current()->staffName_.isEmpty() ?
			 i18n("Staff %1").arg(i) :
			 staffIterator.current()->staffName_,
			 QString::null
			);
		if (firstHBox == 0) firstHBox = currentPage;
		QListIterator<NVoice> voiceIterator(staffIterator.current()->voicelist_);
		unsigned int j = 1;
		for
			(QListIterator<NVoice>
				voiceIterator(staffIterator.current()->voicelist_);
			 voiceIterator.current();
			 ++voiceIterator, ++j
			) {
			pageList_.current()->append
				(new VoiceBox(currentPage, this, 0, j, voiceIterator.current()));
		}
	}
	firstPageIdx_ = pageIndex(firstHBox); /* there seems to be a bug in activePageIndex() it does not start with 0 */
	showPage(staffNr);
}

VoiceDialog::~VoiceDialog() {
	while (pageList_.first()) {
		pageList_.current()->setAutoDelete(true);
		pageList_.current()->clear();
		pageList_.remove();
	}
}


void VoiceDialog::slotUser1() { /* create voice */
	NStaff *current_staff;
	NVoice *new_voice;
	VoiceBox *currentBox;
	QList<VoiceBox> *voice_box_list;
	QHBox *currentPage;

	int num;
	if ((current_staff = staffList_->at(myActivePageIndex ())) == 0) {
		NResource::abort("VoiceDialog::slotUser1: internal error", 1);
	}
	if ((new_voice = current_staff->addNewVoice()) == 0) return;
	if ((voice_box_list = pageList_.at(myActivePageIndex())) == 0) {
		NResource::abort("VoiceDialog::slotUser1: internal error", 2);
	}
	currentPage = voice_box_list->first()->getContainer();
	num = voice_box_list->count() + 1;
	VoiceBox *vb;
	voice_box_list->append(
		vb = new VoiceBox (currentPage, this, 0, num, new_voice)
	);
	unsigned int i = 1;
	for
		(QListIterator<VoiceBox> voiceBoxIterator(*voice_box_list);
		 voiceBoxIterator.current();
		 ++voiceBoxIterator, ++i
		) {
		voiceBoxIterator.current()->renumber(i);
	}
	vb->show();
}

void VoiceDialog::slotApply() {
	VoiceBox *voice_box;
	QList<VoiceBox> *voice_box_list;
	if ((voice_box_list = pageList_.at(myActivePageIndex ())) == 0) {
		NResource::abort("VoiceDialog::slotUser2: internal error");
	}
	for (voice_box = voice_box_list->first(); voice_box; voice_box =  voice_box_list->next()) {
		voice_box->apply();
	}
	mainWidget_->reposit();
	mainWidget_->repaint();
}

void VoiceDialog::accept() {
	slotApply();
	hide();
	while (pageList_.first()) {
		pageList_.current()->setAutoDelete(true);
		pageList_.current()->clear();
		pageList_.remove();
	}
}

bool VoiceDialog::destroyVoice(VoiceBox *rem_box, NVoice *voice) {
	VoiceBox *voice_box;
	QList<VoiceBox> *voice_box_list;
	NStaff *current_staff;
	int i;

	if ((current_staff = staffList_->at(myActivePageIndex ())) == NULL) {
		NResource::abort("VoiceDialog::destroyVoice: internal error", 1);
	}
	if (current_staff->deleteVoice(voice, this, 0) == -1) {
		return false;
	}
	if ((voice_box_list = pageList_.at(myActivePageIndex ())) == 0) {
		NResource::abort("VoiceDialog::slotUser1: internal error", 2);
	}
	if (voice_box_list->find(rem_box) == -1) {
		NResource::abort("VoiceDialog::slotUser1: internal error", 3);
	}
	voice_box_list->remove();
	for (i = 1, voice_box = voice_box_list->first(); voice_box; voice_box = voice_box_list->next(), i++) {
		voice_box->renumber(i);
	}
	return true;
}

#include "voicedialog.moc"
