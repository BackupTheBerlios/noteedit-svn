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

#include <kapp.h>
#include <kconfig.h>
#include <kiconloader.h>
#include <klocale.h>
#include <knuminput.h>

#if KDE_VERSION >= 220
#include <ktip.h>
#endif

#if QT_VERSION >= 300
#include <qpen.h>
#endif

#include <qlabel.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qvbox.h>
#include <qwhatsthis.h>

#include "configuredialog.h"
#include "configuredefaultvalues.h"
#include "mainframewidget.h"

#include "resource.h"

ConfigureDialog::ConfigureDialog(NMainFrameWidget *mainWidget) :
	KDialogBase
		(IconList,                                 //  dialogFace
		 kapp->makeStdCaption(i18n("Configure")),  //  caption
		 Help | Default | Ok | Apply | Cancel,     //  buttonMask
		 Ok,                                       //  defaultButton
		 mainWidget,                               //  parent
		 "ConfigureDialog",                        //  name (for internal use only)
		 true,                                     //  modal
		 true                                      //  separator
		)
	{
	mainWidget_ = mainWidget;

	//  GENERAL
	QVBox *pageGeneral = addVBoxPage
		(i18n("General"), QString::null, BarIcon("misc", KIcon::SizeLarge));

	//  Autosave
	QGroupBox *autosaveGroup = new QGroupBox
		(1, Horizontal, i18n("Autosave"), pageGeneral);
	autosaveEnable = new QCheckBox
		(i18n("&Enable autosave"), autosaveGroup);
	autosaveEnable->setChecked(NResource::autosaveEnable_);
	QToolTip::add
		(autosaveEnable,
		 i18n("The current score will be saved regularly if this is enabled.")
		);
	QWhatsThis::add
		(autosaveEnable,
		 i18n
			("The current score will be saved regularly if this is enabled. It "
			 "will be saved to <filename>.sav, where <filename> is the name of the "
			 "file to which the current score was last saved, or if it was not "
			 "saved, the name of the file from which it was loaded. If the current "
			 "score is new, it will be saved to unnamed<number>.not.sav, where "
			 "<number> is a positive integer with the purpouse of making the name "
			 "unique.")
		);
	autosaveInterval = new KIntNumInput
		(NResource::autosaveInterval_, autosaveGroup);
	autosaveInterval->setRange
		(AUTOSAVE_INTERVAL_MIN, AUTOSAVE_INTERVAL_MAX, 1, true);
	autosaveInterval->setSuffix(i18n(" minutes"));
	autosaveInterval->setLabel(i18n("Autosave &interval:"));
	QToolTip::add
		(autosaveInterval,
		 i18n
			("The time interval between the occations when the current score is "
			 "saved if there are unsaved changes.")
		);
/* is done during NResource::NResource
	connect //  The parameters don't match exactly but it works.
		(autosaveEnable, SIGNAL(stateChanged(int)),
		 autosaveInterval, SLOT(setEnabled(bool))
		);
*/

	//  Startup
	QGroupBox *startupGroup = new QGroupBox
		(2, Horizontal, i18n("Startup"), pageGeneral);
	musixWarn = new QCheckBox(i18n("&MusixWarn"), startupGroup);
	musixWarn->setChecked(NResource::musixWarn_);
	QToolTip::add (musixWarn, i18n("enable MusiXTeX warning."));

	useMidiPedal = new QCheckBox(i18n("useMidi&Pedal"), startupGroup);
	useMidiPedal->setChecked(NResource::useMidiPedal_);
	QToolTip::add (useMidiPedal, i18n("enable MIDI pedal instruction (does not work on all sound cards; especially TiMidity++)"));
	
	startupShowTip = new QCheckBox(i18n("&Tip of the day"), startupGroup);
	kapp->config()->setGroup("TipOfDay");
	startupShowTip->setChecked
		(kapp->config()->readBoolEntry(QString("RunOnStart"), true));
	QToolTip::add
		(startupShowTip,
		 i18n("A tip is shown when the application has started if this is enabled.")
		);
	startupLoadLastScore = new QCheckBox
		(i18n("Load la&st score"), startupGroup);
	startupLoadLastScore->setChecked(NResource::startupLoadLastScore_);
	QToolTip::add
		(startupLoadLastScore,
		 i18n
			("The last score that was open the last time the appliaction was used "
			 "will be loaded if this is enabled.")
		);
	//  turn over
	QGroupBox *turnOverGroup = new QGroupBox
		(1, Horizontal, i18n("Turn Over"), pageGeneral);
	turnOverPoint = new KIntNumInput
		(NResource::turnOverPoint_, turnOverGroup);
	turnOverPoint->setRange
		(0, TURN_OVER_MAX, 1, true);
/*
	turnOverPoint->setSuffix(i18n(" pixel"));
*/
	turnOverPoint->setLabel(i18n("page turn point:"));
	QToolTip::add
		(turnOverPoint,
		 i18n
			("Determine the offset at which the turn over takes place.")
		);


	//  EDITING
	QVBox *pageEditing = addVBoxPage
		(i18n("Editing"), QString::null, BarIcon("edit", KIcon::SizeLarge));

	//  [unnamed a]
	QGroupBox *aGroup = new QGroupBox
		(2, Horizontal, QString::null, pageEditing);
	aGroup->setFrameStyle(QFrame::NoFrame);
	aAllowAutoBeaming = new QCheckBox
		(i18n("auto &beam insertion"), aGroup);
	aAllowAutoBeaming->setChecked(NResource::autoBeamInsertion_);
	aAllowInsertEcho = new QCheckBox
		(i18n("insert &echo"), aGroup);
	aAllowInsertEcho->setChecked(NResource::allowInsertEcho_);
	aMoveAccordingKeysig = new QCheckBox
		(i18n("Move according ke&ysig"), aGroup);
	aMoveAccordingKeysig->setChecked(NResource::moveAccKeysig_);
	aAutomaticBarInsertion = new QCheckBox
		(i18n("autom. bar insertion"), aGroup);
	aAutomaticBarInsertion->setChecked(NResource::automaticBarInsertion_);


	//  COLORS
	QFrame *pageColors = addPage
		(i18n("Colors"), QString::null, BarIcon("colors", KIcon::SizeLarge));

	QGridLayout *colorsLayout = new QGridLayout(pageColors, 6, 4);
	colorsLayout->setSpacing(KDialog::spacingHint());
	colorsLayout->setColStretch(1, 1 /*factor*/);
	colorsLayout->setColStretch(3, 1 /*factor*/);

	colorsBackground = new KColorCombo(pageColors);
	colorsBackground->setColor(NResource::backgroundBrush_.color());
	colorsLayout->addWidget(colorsBackground, 0, 1);
	QLabel *colorsBackgroundLabel = new QLabel
		(colorsBackground, i18n("&Backgrounds:"), pageColors);
	colorsLayout->addWidget(colorsBackgroundLabel, 0, 0);

	colorsSelectionBackground = new KColorCombo(pageColors);
	colorsSelectionBackground->setColor
		(NResource::selectionBackgroundBrush_.color());
	colorsLayout->addWidget(colorsSelectionBackground, 0, 3);
	QLabel *colorsSelectionBackgroundLabel = new QLabel
		(colorsSelectionBackground, i18n("Selection bac&kgrounds:"), pageColors);
	colorsLayout->addWidget(colorsSelectionBackgroundLabel, 0, 2);

	colorsStaff = new KColorCombo(pageColors);
	colorsStaff->setColor(NResource::staffPen_.color());
	colorsLayout->addWidget(colorsStaff, 1, 1);
	QLabel *colorsStaffLabel = new QLabel
		(colorsStaff, i18n("&Staffs:"), pageColors);
	colorsLayout->addWidget(colorsStaffLabel, 1, 0);

	colorsSelectedStaff = new KColorCombo(pageColors);
	colorsSelectedStaff->setColor(NResource::selectedStaffPen_.color());
	colorsLayout->addWidget(colorsSelectedStaff, 1, 3);
	QLabel *colorsSelectedStaffLabel = new QLabel
		(colorsSelectedStaff, i18n("Selected sta&ffs:"), pageColors);
	colorsLayout->addWidget(colorsSelectedStaffLabel, 1, 2);

	colorsBar = new KColorCombo(pageColors);
	colorsBar->setColor(NResource::barPen_.color());
	colorsLayout->addWidget(colorsBar, 2, 1);
	QLabel *colorsBarLabel = new QLabel(colorsBar, i18n("Ba&rs:"), pageColors);
	colorsLayout->addWidget(colorsBarLabel, 2, 0);

	colorsSelectedBar = new KColorCombo(pageColors);
	colorsSelectedBar->setColor(NResource::selectedBarPen_.color());
	colorsLayout->addWidget(colorsSelectedBar, 2, 3);
	QLabel *colorsSelectedBarLabel = new QLabel
		(colorsSelectedBar, i18n("Sele&cted bars:"), pageColors);
	colorsLayout->addWidget(colorsSelectedBarLabel, 2, 2);

	colorsBarNumber = new KColorCombo(pageColors);
	colorsBarNumber->setColor(NResource::barNumberPen_.color());
	colorsLayout->addWidget(colorsBarNumber, 3, 1);
	QLabel *colorsBarNumberLabel = new QLabel
		(colorsBarNumber, i18n("Bar n&umbers:"), pageColors);
	colorsLayout->addWidget(colorsBarNumberLabel, 3, 0);

	colorsSelectedBarNumber = new KColorCombo(pageColors);
	colorsSelectedBarNumber->setColor(NResource::selectedBarNumberPen_.color());
	colorsLayout->addWidget(colorsSelectedBarNumber, 3, 3);
	QLabel *colorsSelectedBarNumberLabel = new QLabel
		(colorsSelectedBarNumber, i18n("Selected bar nu&mbers:"), pageColors);
	colorsLayout->addWidget(colorsSelectedBarNumberLabel, 3, 2);

	colorsTempoSignature = new KColorCombo(pageColors);
	colorsTempoSignature->setColor(NResource::tempoSignaturePen_.color());
	colorsLayout->addWidget(colorsTempoSignature, 4, 1);
	QLabel *colorsTempoSignatureLabel = new QLabel
		(colorsTempoSignature, i18n("&Tempo signatures:"), pageColors);
	colorsLayout->addWidget(colorsTempoSignatureLabel, 4, 0);

	colorsSelectedTempoSignature = new KColorCombo(pageColors);
	colorsSelectedTempoSignature->setColor
		(NResource::selectedTempoSignaturePen_.color());
	colorsLayout->addWidget(colorsSelectedTempoSignature, 4, 3);
	QLabel *colorsSelectedTempoSignatureLabel = new QLabel
		(colorsSelectedTempoSignature, i18n("Selected temp&o signatures:"), pageColors);
	colorsLayout->addWidget(colorsSelectedTempoSignatureLabel, 4, 2);

	colorsVolumeSignature = new KColorCombo(pageColors);
	colorsVolumeSignature->setColor(NResource::volumeSignaturePen_.color());
	colorsLayout->addWidget(colorsVolumeSignature, 5, 1);
	QLabel *colorsVolumeSignatureLabel = new QLabel
		(colorsVolumeSignature, i18n("&Volume signatures:"), pageColors);
	colorsLayout->addWidget(colorsVolumeSignatureLabel, 5, 0);

	colorsSelectedVolumeSignature = new KColorCombo(pageColors);
	colorsSelectedVolumeSignature->setColor
		(NResource::selectedVolumeSignaturePen_.color());
	colorsLayout->addWidget(colorsSelectedVolumeSignature, 5, 3);
	QLabel *colorsSelectedVolumeSignatureLabel = new QLabel
		(colorsSelectedVolumeSignature,
		 i18n("Selected volume si&gnatures:"),
		 pageColors
		);
	colorsLayout->addWidget(colorsSelectedVolumeSignatureLabel, 5, 2);

	colorsProgramChange = new KColorCombo(pageColors);
	colorsProgramChange->setColor(NResource::programChangePen_.color());
	colorsLayout->addWidget(colorsProgramChange, 6, 1);
	QLabel *colorsProgramChangeLabel = new QLabel
	(colorsProgramChange, i18n("&Program changes:"), pageColors);
	colorsLayout->addWidget(colorsProgramChangeLabel, 6, 0);

	colorsSelectedProgramChange = new KColorCombo(pageColors);
	colorsSelectedProgramChange->setColor
		(NResource::selectedProgramChangePen_.color());
	colorsLayout->addWidget(colorsSelectedProgramChange, 6, 3);
	QLabel *colorsSelectedProgramChangeLabel = new QLabel
		(colorsSelectedProgramChange,
		 i18n("Selected program c&hanges:"),
		 pageColors
		);
	colorsLayout->addWidget(colorsSelectedProgramChangeLabel, 6, 2);

	colorsSpecialEnding = new KColorCombo(pageColors);
	colorsSpecialEnding->setColor(NResource::specialEndingPen_.color());
	colorsLayout->addWidget(colorsSpecialEnding, 7, 1);
	QLabel *colorsSpecialEndingLabel = new QLabel
		(colorsSpecialEnding, i18n("Special &endings:"), pageColors);
	colorsLayout->addWidget(colorsSpecialEndingLabel, 7, 0);

	colorsSelectedSpecialEnding = new KColorCombo(pageColors);
	colorsSelectedSpecialEnding->setColor
		(NResource::selectedSpecialEndingPen_.color());
	colorsLayout->addWidget(colorsSelectedSpecialEnding, 7, 3);
	QLabel *colorsSelectedSpecialEndingLabel = new QLabel
		(colorsSelectedSpecialEnding, i18n("Selected spec&ial endings:"), pageColors);
	colorsLayout->addWidget(colorsSelectedSpecialEndingLabel, 7, 2);

	colorsStaffName = new KColorCombo(pageColors);
	colorsStaffName->setColor(NResource::staffNamePen_.color());
	colorsLayout->addWidget(colorsStaffName, 8, 1);
	QLabel *colorsStaffNameLabel = new QLabel
		(colorsStaffName, i18n("Staff &names:"), pageColors);
	colorsLayout->addWidget(colorsStaffNameLabel, 8, 0);

	colorsSelectedStaffName = new KColorCombo(pageColors);
	colorsSelectedStaffName->setColor(NResource::selectedStaffNamePen_.color());
	colorsLayout->addWidget(colorsSelectedStaffName, 8, 3);
	QLabel *colorsSelectedStaffNameLabel = new QLabel
		(colorsSelectedStaffName, i18n("Se&lected staff names:"), pageColors);
	colorsLayout->addWidget(colorsSelectedStaffNameLabel, 8, 2);

	colorsLyric = new KColorCombo(pageColors);
	colorsLyric->setColor(NResource::lyricPen_.color());
	colorsLayout->addWidget(colorsLyric, 9, 1);
	QLabel *colorsLyricLabel = new QLabel
		(colorsLyric, i18n("L&yrics:"), pageColors);
	colorsLayout->addWidget(colorsLyricLabel, 9, 0);

	colorsContextBrush = new KColorCombo(pageColors);
	colorsContextBrush->setColor
		(NResource::contextBrush_.color());
	colorsLayout->addWidget(colorsContextBrush, 9, 3);
	QLabel *colorsContextBrushLabel = new QLabel
		(colorsContextBrush, i18n("staff con&text:"), pageColors);
	colorsLayout->addWidget(colorsContextBrushLabel, 9, 2);


	//  SOUND
	QVBox *pageSound = addVBoxPage
		(i18n("Sound"), QString::null, BarIcon("sound", KIcon::SizeLarge));

#ifdef WITH_TSE3
	//  Sequencers
	QGroupBox *sequencersGroup = new QGroupBox
		(2, Horizontal, i18n("Sequencers"), pageSound);
	sequencersALSA = new QCheckBox(i18n("A&LSA"), sequencersGroup);
	sequencersALSA->setChecked
		(NResource::schedulerRequest_ & ALSA_SCHEDULER_REQUESTED);
	QToolTip::add
		(sequencersALSA,
		 i18n("Attempt to create an ALSA MIDI scheduler at startup.")
		);
	QWhatsThis::add
		(sequencersALSA,
		 i18n
		 	("The TSE3 sequencer library will attempt to create an ALSA (Advanced "
			 "Linux Sound Architecture) MIDI scheduler when the application starts "
			 "if this is enabled. Enable this if you use ALSA to acces your "
			 "soundcard."
			)
		);
	sequencersOSS = new QCheckBox(i18n("O&SS"), sequencersGroup);
	sequencersOSS->setChecked
		(NResource::schedulerRequest_ & OSS_SCHEDULER_REQUESTED);
	QToolTip::add
		(sequencersOSS,
		 i18n("Attempt to create an OSS MIDI scheduler at startup.")
		);
	QWhatsThis::add
		(sequencersOSS,
		 i18n
			("The TSE3 sequencer library will attempt to create an OSS (Open Sound "
			 "System) MIDI scheduler when the application starts if this is "
			 "enabled. Enable this if you use OSS to acces your soundcard."
			)
		);
#endif

	//  MIDI devices
	MIDIDevicesGroup = new QGroupBox
		(1, Horizontal, i18n("&MIDI devices"), pageSound);
	MIDIDevices = new MIDIDeviceListBox(MIDIDevicesGroup);

	//  CHORD NAMES
	QVBox *chordConfig = addVBoxPage
		(i18n("Chords"), QString::null, BarIcon("chordnames", KIcon::SizeLarge));

	QGroupBox *chordNameGroup = new QGroupBox
		(1, Horizontal, i18n("Chordnames"), chordConfig);
	aChordNames = new QComboBox (i18n("Chordnames"), chordNameGroup);
	aChordNames->insertItem( i18n("American, sharps") );
	aChordNames->insertItem( i18n("American, flats") );
	aChordNames->insertItem( i18n("American, mixed") );
	aChordNames->insertItem( i18n("European, sharps") );
	aChordNames->insertItem( i18n("European, flats") );
	aChordNames->insertItem( i18n("European, mixed") );
	aChordNames->insertItem( i18n("Jazz, sharps") );
	aChordNames->insertItem( i18n("Jazz, flats") );
	aChordNames->insertItem( i18n("Jazz, mixed") );
	aChordNames->setCurrentItem(NResource::globalNoteNames_);


	QGroupBox *domGroup = new QGroupBox
		(1, Horizontal, i18n("Dominant 7"), chordConfig);
	aDom7 = new QComboBox (i18n("Dominant 7"), domGroup);
	aDom7->insertItem( "7M" );
	aDom7->insertItem( "maj7" );
	aDom7->insertItem( "dom7" );
	aDom7->setCurrentItem(NResource::globalMaj7_);

	QGroupBox *altGroup = new QGroupBox
		(1, Horizontal, i18n("Alterations"), chordConfig);
	aAlterations = new QComboBox (i18n("Alterations"), altGroup);
	aAlterations->insertItem( "+/- symbols" );
	aAlterations->insertItem( "#/b symbols" );
	aAlterations->setCurrentItem(NResource::globalFlatPlus_);

	connect(this, SIGNAL(tryClicked()), this, SLOT(slotApply()));
	connect(this, SIGNAL(okClicked()), this, SLOT(slotApply()));
	connect(this, SIGNAL(okClicked()), this, SLOT(hide()));

}

void ConfigureDialog::slotApply() {

	//  GENERAL

	//  Autosave
	NResource::setAutosave(autosaveEnable->isChecked(), autosaveInterval->value());
	// turn over
	NResource::turnOverPoint_	= turnOverPoint->value();

	//  Startup
	NResource::musixWarn_            = musixWarn           ->isChecked();
	NResource::useMidiPedal_	 = useMidiPedal         ->isChecked();
	NResource::startupLoadLastScore_ = startupLoadLastScore->isChecked();

	//  Setting both setShowOnStart and TipOfDay/RunOnStart is apparently not
	//  enough. They can still be made out of sync.
#if KDE_VERSION >= 220
	KTipDialog::setShowOnStart(startupShowTip->isChecked());
#endif
	kapp->config()->setGroup("TipOfDay");
	kapp->config()->writeEntry("RunOnStart", startupShowTip->isChecked());


	//  EDITING

	//  [unnamed a]
	NResource::autoBeamInsertion_    = aAllowAutoBeaming   ->isChecked();
	NResource::allowInsertEcho_     = aAllowInsertEcho    ->isChecked();
	NResource::moveAccKeysig_       = aMoveAccordingKeysig->isChecked();
	NResource::automaticBarInsertion_= aAutomaticBarInsertion->isChecked();


	//  COLORS

	NResource::backgroundBrush_.setColor(colorsBackground->color());
	NResource::selectionBackgroundBrush_.setColor
		(colorsSelectionBackground->color());

	NResource::staffPen_.setColor(colorsStaff->color());
	NResource::selectedStaffPen_.setColor(colorsSelectedStaff->color());

	NResource::barPen_.setColor(colorsBar->color());
	NResource::selectedBarPen_.setColor(colorsSelectedBar->color());

	NResource::barNumberPen_.setColor(colorsBarNumber->color());
	NResource::selectedBarNumberPen_.setColor(colorsSelectedBarNumber->color());

	NResource::tempoSignaturePen_.setColor(colorsTempoSignature->color());
	NResource::selectedTempoSignaturePen_.setColor
		(colorsSelectedTempoSignature->color());
	NResource::tempoSignatureBrush_.setColor(colorsTempoSignature->color());
	NResource::selectedTempoSignatureBrush_.setColor
		(colorsSelectedTempoSignature->color());

	NResource::volumeSignaturePen_.setColor(colorsVolumeSignature->color());
	NResource::selectedVolumeSignaturePen_.setColor
		(colorsSelectedVolumeSignature->color());

	NResource::programChangePen_.setColor(colorsProgramChange->color());
	NResource::selectedProgramChangePen_.setColor
		(colorsSelectedProgramChange->color());

	NResource::specialEndingPen_.setColor(colorsSpecialEnding->color());
	NResource::selectedSpecialEndingPen_.setColor
		(colorsSelectedSpecialEnding->color());

	NResource::contextBrush_.setColor(colorsContextBrush->color());

	NResource::staffNamePen_.setColor(colorsStaffName->color());
	NResource::selectedStaffNamePen_.setColor(colorsSelectedStaffName->color());

	NResource::lyricPen_.setColor(colorsLyric->color());


	//  SOUND

#ifdef WITH_TSE3
	//  Sequencers
	NResource::schedulerRequest_ =
		(sequencersALSA->isChecked() ? ALSA_SCHEDULER_REQUESTED : 0) |
		(sequencersOSS->isChecked()  ?  OSS_SCHEDULER_REQUESTED : 0);
#endif

	// MIDI devices
	if (MIDIDevicesGroup->isEnabled())
		NResource::mapper_->changeDevice(MIDIDevices->currentItem());


	// Chord names

	NResource::globalNoteNames_ = aChordNames->currentItem(); 
	NResource::globalMaj7_ = aDom7->currentItem(); 
	NResource::globalFlatPlus_ = aAlterations->currentItem(); 

	mainWidget_->updateChordnames();
	mainWidget_->repaint(); //  So that color changes become visibled immediately.
}

void ConfigureDialog::slotDefault() {

	//  GENERAL

	//  Autosave
	autosaveEnable->setChecked(AUTOSAVE_ENABLE);
	autosaveInterval->setValue(AUTOSAVE_INTERVAL);
	turnOverPoint->setValue(TURN_OVER_MIN);

	//  Startup
	musixWarn           ->setChecked(MUSIX_WARN);
	useMidiPedal        ->setChecked(MIDI_PEDAL);
	startupLoadLastScore->setChecked(STARTUP_TIP);
	startupShowTip      ->setChecked(STARTUP_LOAD_LAST_SCORE);


	//  EDITING

	//  [unnamed a]
	aAllowAutoBeaming   ->setChecked(EDITING_ALLOW_AUTO_BEAMING);
	aAllowInsertEcho    ->setChecked(EDITING_INSERT_ECHO);
	aMoveAccordingKeysig->setChecked(EDITING_MOVE_ACCORDING_KEYSIG);
	aAutomaticBarInsertion->setChecked(EDITING_AUTOMATIC_BAR_INSERTION);


	//  COLORS

	colorsBackground             ->setColor(COLORS_BACKGROUND);
	colorsSelectionBackground    ->setColor(COLORS_SELECTION_BACKGROUND);
	colorsContextBrush	     ->setColor(COLORS_CONTEXT_BRUSH);
	colorsStaff                  ->setColor(COLORS_STAFF);
	colorsSelectedStaff          ->setColor(COLORS_SELECTED_STAFF);
	colorsBar                    ->setColor(COLORS_BAR);
	colorsSelectedBar            ->setColor(COLORS_SELECTED_BAR);
	colorsBarNumber              ->setColor(COLORS_BAR_NUMBER);
	colorsSelectedBarNumber      ->setColor(COLORS_SELECTED_BAR_NUMBER);
	colorsTempoSignature         ->setColor(COLORS_TEMPO_SIGNATURE);
	colorsSelectedTempoSignature ->setColor(COLORS_SELECTED_TEMPO_SIGNATURE);
	colorsVolumeSignature        ->setColor(COLORS_VOLUME_SIGNATURE);
	colorsSelectedVolumeSignature->setColor(COLORS_SELECTED_VOLUME_SIGNATURE);
	colorsProgramChange          ->setColor(COLORS_PROGRAM_CHANGE);
	colorsSelectedProgramChange  ->setColor(COLORS_SELECTED_PROGRAM_CHANGE);
	colorsSpecialEnding          ->setColor(COLORS_SPECIAL_ENDING);
	colorsSelectedSpecialEnding  ->setColor(COLORS_SELECTED_SPECIAL_ENDING);
	colorsStaffName              ->setColor(COLORS_STAFF_NAME);
	colorsSelectedStaffName      ->setColor(COLORS_SELECTED_STAFF_NAME);
	colorsLyric                  ->setColor(COLORS_LYRIC);


	//  SOUND

#ifdef WITH_TSE3
	//  Sequencers
	sequencersALSA->setChecked(SEQUENCERS_ALSA);
	sequencersOSS ->setChecked(SEQUENCERS_OSS);
#endif

	//  MIDI devices
	if (MIDIDevicesGroup->isEnabled())
		MIDIDevices->setCurrentItem(DEFAULT_MIDI_PORT);


	// CHORD NAMES

	aChordNames->setCurrentItem(DEFAULT_CHORD_NAME_SET);
	aDom7->setCurrentItem(DEFAULT_DOM7_ID);
	aAlterations->setCurrentItem(DEFAULT_ALTERATION_SIGN);

}

#include "configuredialog.moc"
