#include <kdebug.h>/****************************************************************************/
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

#include <kiconloader.h>
#include <klocale.h>

#include "mididevicelistbox.h"
#include "resource.h"

#ifdef WITH_TSE3
//  We would like to use different icons for these different devices if we
//  could get some appropriate icons for them.
const char * portTypeNameToIconName(const char * portTypeName) {
	if (strcmp(portTypeName, "Adlib")   == 0 ||
	    strcmp(portTypeName, "FM")      == 0 ||
	    strcmp(portTypeName, "MPU 401") == 0 ||
	    strcmp(portTypeName, "GUS")     == 0 ||
	    strcmp(portTypeName, "Unknown") == 0)                 return "kcmpci";
	else if (strcmp(portTypeName, "External MIDI port") == 0) return "midikeyboard";
	else                                                      return "";
}

#include <tse3/MidiScheduler.h>
#include <tse3/Midi.h>
#include <tse3/plt/Alsa.h>
#include <tse3/plt/OSS.h>
#include "tse3/Error.h"

MIDIDeviceListBox::MIDIDeviceListBox(QWidget *Parent)
	: KListBox(Parent) {
	if (NResource::mapper_->theScheduler_) {
#if TSE3_MID_VERSION_NR < 2
		for (unsigned int i = 0; i < NResource::mapper_->theScheduler_->ports(); ++i)
#else
		for (unsigned int i = 0; i < NResource::mapper_->theScheduler_->numPorts(); ++i)
#endif
			new QListBoxPixmap
				(this, kapp->iconLoader()->loadIcon
				 (portTypeNameToIconName(NResource::mapper_->theScheduler_->portType(i)),
				  KIcon::Small, 32),
#if TSE3_MID_VERSION_NR < 2
				 NResource::mapper_->theScheduler_->portName(i)
#else
				 NResource::mapper_->theScheduler_->portName(
					NResource::mapper_->theScheduler_->portNumber(i))
#endif
				);
	setCurrentItem(NResource::defMidiPort_);
	}
	else {
		new QListBoxPixmap
			(this, kapp->iconLoader()->loadIcon("stop", KIcon::Small, 32),
			 i18n("[no MIDI device created]"));
		Parent->setEnabled(false);
	}
}

#else
MIDIDeviceListBox::MIDIDeviceListBox(QWidget *Parent)
	: KListBox(Parent) {
	for (unsigned int i = 0; i < NResource::mapper_->deviceNameList.count(); ++i)
		new QListBoxPixmap
			(this, kapp->iconLoader()->loadIcon
			 (NResource::mapper_->isSynth(i) ? "kcmpci" : "midikeyboard", KIcon::Small, 32),
			 NResource::mapper_->deviceNameList[i]);
	if (count() == 0) {
	new QListBoxPixmap
			(this, kapp->iconLoader()->loadIcon("stop", KIcon::Small, 32),
			 i18n("[no MIDI device created]"));
		Parent->setEnabled(false);
	}
}
#endif

#include "mididevicelistbox.moc"
