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

#ifndef CONFIGUREDIALOG_H
#define CONFIGUREDIALOG_H

#include <kcolorcombo.h>
#include <kdialogbase.h>
#include <knuminput.h>

#include <qcheckbox.h>
#include <qgroupbox.h>
#include <qbuttongroup.h>

#include "mididevicelistbox.h"
#include "midimapper.h"
class NMainFrameWidget;

class ConfigureDialog : public KDialogBase {
	Q_OBJECT

public:
	ConfigureDialog(NMainFrameWidget *mainWidget);

private slots:
	void slotApply();
	void slotDefault();

private:

	NMainFrameWidget *mainWidget_;

	//  GENERAL

	QCheckBox *autosaveEnable;
	KIntNumInput *autosaveInterval;
	KIntNumInput *turnOverPoint;

	//  Startup
	QCheckBox *useMidiPedal;
	QCheckBox *startupLoadLastScore;
	QCheckBox *startupShowTip;


	//  EDITING

	//  [unnamed a]
	QCheckBox *aAllowAutoBeaming;
	QCheckBox *aAllowKeyboardInsert;
	QCheckBox *aAllowInsertEcho;
	QCheckBox *aMoveAccordingKeysig;
	QCheckBox *aAutomaticBarInsertion;


	//  COLORS

	KColorCombo *colorsBackground;
	KColorCombo *colorsSelectionBackground;
	KColorCombo *colorsStaff;
	KColorCombo *colorsSelectedStaff;
	KColorCombo *colorsBar;
	KColorCombo *colorsSelectedBar;
	KColorCombo *colorsBarNumber;
	KColorCombo *colorsSelectedBarNumber;
	KColorCombo *colorsTempoSignature;
	KColorCombo *colorsSelectedTempoSignature;
	KColorCombo *colorsVolumeSignature;
	KColorCombo *colorsSelectedVolumeSignature;
	KColorCombo *colorsProgramChange;
	KColorCombo *colorsSelectedProgramChange;
	KColorCombo *colorsSpecialEnding;
	KColorCombo *colorsSelectedSpecialEnding;
	KColorCombo *colorsStaffName;
	KColorCombo *colorsSelectedStaffName;
	KColorCombo *colorsLyric;
	KColorCombo *colorsContextBrush;


	//  SOUND

#ifdef WITH_TSE3
	//  Sequencers
	QCheckBox *sequencersALSA;
	QCheckBox *sequencersOSS;
#endif

	//  MIDI devices
	QGroupBox          *MIDIDevicesGroup;
	MIDIDeviceListBox  *MIDIDevices;

	//  CHORDNAMES
	QComboBox *aChordNames;
	QComboBox *aDom7;
	QComboBox *aAlterations;

};

#endif //  CONFIGUREDIALOG_H
