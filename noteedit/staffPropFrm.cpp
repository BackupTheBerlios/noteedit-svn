#include <klocale.h>
#include <qlistbox.h>
#include <qspinbox.h>
#include <qlineedit.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qtabwidget.h>

#include "staff.h"
#include "staffElem.h"
#include "scaleedit_impl.h"
#include "voicedialog.h"
#include "staffPropFrm.h"
#include "mainframewidget.h"

#define PROPERTYNUM 10

/*------------------------------------- staffPropsForm ------------------------------------------*/

staffPropFrm::staffPropFrm(QWidget *parent)  : staffPropForm(parent, 0, true)
{
	mbApply = true;
	for (int i = 0; i < 128; ++i)
		staffVoice->insertItem(i18n("%1. %2").arg(i).arg(i18n(NResource::instrTab[i])));
	for (int i = 0; i < 16; ++i)
		staffChannel->insertItem(i18n("Channel %1").arg(i + 1 ));

	this->staffVol->setAll(0, 127, 80);
	this->staffOver->setAll(1, 200, 60);
	this->staffUnder->setAll(1, 200, 60);
	this->staffLyrics->setAll(1, 200, 60);
	this->staffStereo->setAll(0, 127, 80);
	this->staffReverb->setAll(0, 127, 0);
	this->staffChorus->setAll(0, 127, 0);
	this->staffPlay->setAll(-12, 17, 0);

	mw_ = (NMainFrameWidget *) parent;
	this->staffOk->setFocus();
	
	// I wish this could be done in another way..
	// For every change with the NScaleEdit widgets, go to the slotPropertyChanged slot
    connect( staffPlay, SIGNAL( returnPressed() ), this, SLOT( slotPropertyChanged() ) );
    connect( staffChorus, SIGNAL( returnPressed() ), this, SLOT( slotPropertyChanged() ) );
    connect( staffReverb, SIGNAL( returnPressed() ), this, SLOT( slotPropertyChanged() ) );
    connect( staffStereo, SIGNAL( returnPressed() ), this, SLOT( slotPropertyChanged() ) );
    connect( staffLyrics, SIGNAL( returnPressed() ), this, SLOT( slotPropertyChanged() ) );
    connect( staffUnder, SIGNAL( returnPressed() ), this, SLOT( slotPropertyChanged() ) );
    connect( staffOver, SIGNAL( returnPressed() ), this, SLOT( slotPropertyChanged() ) );
    connect( staffVol, SIGNAL( returnPressed() ), this, SLOT( slotPropertyChanged() ) );
    connect( staffPlay, SIGNAL( valueChanged(int) ), this, SLOT( slotPropertyChanged() ) );
    connect( staffChorus, SIGNAL( valueChanged(int) ), this, SLOT( slotPropertyChanged() ) );
    connect( staffReverb, SIGNAL( valueChanged(int) ), this, SLOT( slotPropertyChanged() ) );
    connect( staffStereo, SIGNAL( valueChanged(int) ), this, SLOT( slotPropertyChanged() ) );
    connect( staffLyrics, SIGNAL( valueChanged(int) ), this, SLOT( slotPropertyChanged() ) );
    connect( staffUnder, SIGNAL( valueChanged(int) ), this, SLOT( slotPropertyChanged() ) );
    connect( staffOver, SIGNAL( valueChanged(int) ), this, SLOT( slotPropertyChanged() ) );
    connect( staffVol, SIGNAL( valueChanged(int) ), this, SLOT( slotPropertyChanged() ) );
    mbApply = false;
}

staffPropFrm::~staffPropFrm()
{
    VoiceBox *voice_box;        // Voice box
    int i = 0;                  // Counter
    
    // Remove Voices from staff
    for (i = 1, voice_box = voiceList_.first(); voice_box; voice_box = voiceList_.next(), ++i) 
        delete voice_box;
    voiceList_.clear();
}

void staffPropFrm::boot( int staffNr, QList<NStaff> *staffList ) {

    // Prevent apply from happen
    mbApply = true;
    QListIterator<NStaff> staffIterator(*staffList); // List of staffs
    VoiceBox *voice_box;     // Voice Box
    int i = 0;               // Counter

    // Remove Voices from staff
    for (i = 1, voice_box = voiceList_.first(); voice_box; voice_box = voiceList_.next(), ++i) 
      delete voice_box;
    // Clear list of voice boxes
    voiceList_.clear();
    voiceBoxFrame_->update();
    i = 0;
        
    // Temporary memory for changed staff property values
    values_     = new int[ PROPERTYNUM * staffIterator.count() ];
    staffNames_ = new QString[ staffIterator.count() ];
    
    // Clear old list
    staffName->clear();    
    
    // Add Staff names to the Combobox
    for
         (;
	  staffIterator.current();
	  ++staffIterator, ++i
	 ) {
      // If staff is empty, insert default Staff name
      this->staffName->insertItem
		(staffIterator.current()->staffName_.isEmpty() ?
		 ("") :
		 staffIterator.current()->staffName_
                );
      // Is this the current staff ?
      if( i == staffNr ) // Save current staff
        actualStaff_ = staffIterator.current();
      // Save values for current staff
      values_[i*PROPERTYNUM + 0] = staffIterator.current()->getVolume();
      values_[i*PROPERTYNUM + 1] = staffIterator.current()->overlength_;
      values_[i*PROPERTYNUM + 2] = staffIterator.current()->underlength_;
      values_[i*PROPERTYNUM + 3] = staffIterator.current()->staff_props_.lyricsdist;
      values_[i*PROPERTYNUM + 4] = staffIterator.current()->pan_;
      values_[i*PROPERTYNUM + 5] = staffIterator.current()->reverb_;
      values_[i*PROPERTYNUM + 6] = staffIterator.current()->chorus_;
      values_[i*PROPERTYNUM + 7] = staffIterator.current()->getChannel();
      values_[i*PROPERTYNUM + 8] = staffIterator.current()->getVoice();
      values_[i*PROPERTYNUM + 9] = staffIterator.current()->transpose_;
    }
    // Save current staffList
    staffList_ = staffList;
     
    // Do not show the "Test" button
    apply->hide();
       
    setValuesFromActualStaff(staffNr);
    
    currentStaffNr_ = staffNr;   
    mbApply = false;
 }

void staffPropFrm::setValuesFromActualStaff(int staffNr)
{
    // Set default values from current staff
    this->staffVol->setStartVal( actualStaff_->getVolume() );
    this->staffOver->setStartVal( actualStaff_->overlength_ );
    this->staffUnder->setStartVal( actualStaff_->underlength_ );
    this->staffLyrics->setStartVal( actualStaff_->staff_props_.lyricsdist );
    this->staffStereo->setStartVal( actualStaff_->pan_ );
    this->staffReverb->setStartVal( actualStaff_->reverb_ );
    this->staffChorus->setStartVal( actualStaff_->chorus_ );
    this->staffChannel->setCurrentItem( actualStaff_->getChannel() );
    // Change staff name to the current selected Staff
    this->staffName->setCurrentItem( staffNr );
    this->staffVoice->setCurrentItem( actualStaff_->getVoice() );
    this->staffPlay->setStartVal( actualStaff_->transpose_ );

    // Read voices from voice list    
    QListIterator<NVoice> voiceIterator(actualStaff_->voicelist_);
    unsigned int j = 1;
    // Add voices to voice tab
    for
    (QListIterator<NVoice>
        voiceIterator(actualStaff_->voicelist_);
        voiceIterator.current();
        ++voiceIterator, ++j
    ) {
        // Create new voice box
	voiceList_.append(
	(new VoiceBox(voiceBoxFrame_, 0, this, j, voiceIterator.current())));
    }
    
    this->show();
}

void staffPropFrm::slotPropertyChanged()
{
    // Apply only, if we do not change within this class
    if( mbApply == false )
    {
	// Apply the changes to the staff
	actualStaff_->setVolume( this->staffVol->getValue() );
	actualStaff_->overlength_ = this->staffOver->getValue();
	actualStaff_->underlength_ = this->staffUnder->getValue();
	actualStaff_->staff_props_.lyricsdist = this->staffLyrics->getValue();
	actualStaff_->pan_ = this->staffStereo->getValue();
	actualStaff_->reverb_ = this->staffReverb->getValue();
	actualStaff_->chorus_ = this->staffChorus->getValue();
	actualStaff_->setChannel( this->staffChannel->currentItem() );
	actualStaff_->staffName_ = QString( this->staffName->currentText() );
	actualStaff_->changeVoice( this->staffVoice->currentItem() );
	actualStaff_->transpose_ = this->staffPlay->getValue();
	mw_->arrangeStaffs(true);
    }
}

void staffPropFrm::slotCreateVoice()
{
	NVoice *new_voice;

	int num;
	if (actualStaff_ == 0) {
		NResource::abort("VoiceDialog::slotUser1: internal error", 1);
	}
	if ((new_voice = actualStaff_->addNewVoice()) == 0) return;
	num = voiceList_.count() + 1;
	VoiceBox *vb;
	voiceList_.append(
		vb = new VoiceBox (voiceBoxFrame_, 0, this, num, new_voice)
	);
	unsigned int i = 1;
	for
		(QListIterator<VoiceBox> voiceBoxIterator(voiceList_);
		 voiceBoxIterator.current();
		 ++voiceBoxIterator, ++i
		) {
		voiceBoxIterator.current()->renumber(i);
	}
	vb->show();
}

void staffPropFrm::slotStaffNameActivated(int staffNr)
{
    mbApply = true;
    // Counter, Position for current Staff in value Buffer
    int i = 0;
    VoiceBox *voice_box;

    // Remove Voices from staff
    for (i = 1, voice_box = voiceList_.first(); voice_box; voice_box = voiceList_.next(), ++i) 
      delete voice_box;
    voiceList_.clear();
    voiceBoxFrame_->update();      
    i=0;
    
    //staffNames_[currentStaffNr_] = this->staffName->currentText();
    
    // Search Staff list for the selected staff
    for
         (QListIterator<NStaff> staffIterator(*staffList_);
	  staffIterator.current();
	  ++staffIterator, ++i
	 ) {
      // Is this the current staff ?
      if( i == staffNr ) // Save current staff
        actualStaff_ = staffIterator.current();
    }
    setValuesFromActualStaff(staffNr);
    currentStaffNr_ = staffNr;
    mbApply = false;
}

void staffPropFrm::slotStaffCancel() {
    QListIterator<NStaff> staffIterator(*staffList_); // List of staffs
    int i = 0; // Counter
    
    // Add Staff names to the Combobox
    for
         (;
	  staffIterator.current();
	  ++staffIterator, ++i
	 ) {
    this->close();
    actualStaff_ = staffIterator.current();
    actualStaff_->setVolume(                values_[i*PROPERTYNUM + 0] );
    actualStaff_->overlength_ =             values_[i*PROPERTYNUM + 1];
    actualStaff_->underlength_ =            values_[i*PROPERTYNUM + 2];
    actualStaff_->staff_props_.lyricsdist = values_[i*PROPERTYNUM + 3];
    actualStaff_->pan_ =                    values_[i*PROPERTYNUM + 4];
    actualStaff_->reverb_ =                 values_[i*PROPERTYNUM + 5];
    actualStaff_->chorus_ =                 values_[i*PROPERTYNUM + 6];
    actualStaff_->setChannel(               values_[i*PROPERTYNUM + 7] );
    actualStaff_->changeVoice(              values_[i*PROPERTYNUM + 8] );
    actualStaff_->transpose_ =              values_[i*PROPERTYNUM + 9];
    }
    mw_->arrangeStaffs(true);
  
  // Free temporary memory
  delete [] values_;
  delete [] staffNames_;
}

void staffPropFrm::slotStaffOk() {

    refresh();
    close();
    // Free temporary memory
    delete [] values_;
    delete [] staffNames_;    
}
    
void staffPropFrm::refresh() {
    
    actualStaff_->setVolume( this->staffVol->getValue() );
    actualStaff_->overlength_ = this->staffOver->getValue();
    actualStaff_->underlength_ = this->staffUnder->getValue();
    actualStaff_->staff_props_.lyricsdist = this->staffLyrics->getValue();
    actualStaff_->pan_ = this->staffStereo->getValue();
    actualStaff_->reverb_ = this->staffReverb->getValue();
    actualStaff_->chorus_ = this->staffChorus->getValue();
    actualStaff_->setChannel( this->staffChannel->currentItem() );
    actualStaff_->staffName_ = QString( this->staffName->currentText() );
    actualStaff_->changeVoice( this->staffVoice->currentItem() );
    actualStaff_->transpose_ = this->staffPlay->getValue();
    mw_->arrangeStaffs(true);
}

bool staffPropFrm::destroyVoice(VoiceBox *rem_box, NVoice *voice) {
	VoiceBox *voice_box;
	int i;

	if (actualStaff_ == NULL) {
		NResource::abort("VoiceDialog::destroyVoice: internal error", 1);
	}
	if (actualStaff_->deleteVoice(voice, 0, this) == -1) {
		return false;
	}
	if (voiceList_.find(rem_box) == -1) {
		NResource::abort("VoiceDialog::slotUser1: internal error", 3);
	}
	voiceList_.remove();
	for (i = 1, voice_box = voiceList_.first(); voice_box; voice_box = voiceList_.next(), i++) {
		voice_box->renumber(i);
	}
	return true;
}
