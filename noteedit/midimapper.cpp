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

#include "config.h"
#if GCC_MAJ_VERS > 2
#ifndef WITH_TSE3
#include <iostream>
#else
#include <istream>
#endif
#else
#ifndef WITH_TSE3
#include <iostream.h>
#else
#include <istream.h>
#endif
#endif
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <qtimer.h>
#include <qpushbutton.h>
#include <qdatetime.h>
#include "resource.h"
#include "midimapper.h"
#include "voice.h"
#include "chord.h"

#define STOP_ALL_NOTES 0x7b
#ifdef WITH_LIBKMID
#include <libkmid/libkmid.h>
#include <libkmid/deviceman.h>
#endif
#include "dbufferwidget.h"
#include "clef.h"

using namespace std;

/*
NMidiEventStr *NMidiEventStr::clone(int playTime) {
	NMidiEventStr *c_evt = new NMidiEventStr;
	*c_evt = *this;
	c_evt->midi_cmd = MNOTE_OFF;
	c_evt->ev_time = playTime+c_evt->length;
	c_evt->midi_prog_change = -1;
	return c_evt;
}
*/

#ifdef WITH_LIBKMID

NMidiMapper::NMidiMapper() : QObject() {
	devMan_ = new DeviceManager();
	isInUse_ = false;
	channelPool_ = 0x0;
	lastSelectedChannel_ = 0;

	if (devMan_->initManager() < 0) {
		cerr << "error opening Midi Device --> music cannot be played" << endl;
		actualDevice_nr_ = -1;
	}
	else {
		midiDevs_ = devMan_->midiPorts();
		synthDevs_ = devMan_->synthDevices();

		if (midiDevs_+synthDevs_ < 1) {
			actualDevice_nr_ = -1;
		}
		else {
			actualDevice_nr_ = 0;
			for (int i = 0; i < midiDevs_ + synthDevs_; ++i) {
				deviceNameList += devMan_->name(i);
			}
			if (NResource::midiPortSet_) {
				if (NResource::defMidiPort_ >= midiDevs_+synthDevs_  ||  NResource::defMidiPort_ < 0) {
					cerr << "There is no MIDI port " << NResource::defMidiPort_ << ". I try 0!" << endl;
				}
				else {
					actualDevice_nr_ = NResource::defMidiPort_;
				}
			}
			devMan_->openDev();
			devMan_->setDefaultDevice(NResource::defMidiPort_);
		}
	}
	immList_.setAutoDelete(true);
}

void NMidiMapper::openDevice() {
	if (actualDevice_nr_ < 0) return;
	//devMan_->initDev();
#define NEWLIBKMID
#ifdef NEWLIBKMID
	devMan_->tmrStart(0);
#else
	devMan_->tmrStart();
#endif
}

NMidiMapper::~NMidiMapper() {
/*
	if (devMan_) {
		delete devMan_;
		devMan_ = 0;
	}
	*/
}


void NMidiMapper::stopAllNotes(QList<NMidiEventStr> *play_list) {
	NMidiEventStr *m_evt;
	NNote *note;
	int i;
	if (actualDevice_nr_ < 0) return;
	for (m_evt = play_list->first(); m_evt; m_evt = play_list->next()) {
		for (note = m_evt->notelist->first(); note; note = m_evt->notelist->next()) {
			devMan_->noteOff(m_evt->midi_channel, note->midiPitch, 0);
		}
	}
	/* To be very shure: */
	for (i = 0; i < 16; i++) {
		devMan_->chnController(i, STOP_ALL_NOTES, 0);
		if (NResource::useMidiPedal_) {
			/* close an eventually open pedal: */
    			devMan_->chnController(i, 0x40, 1);
		}
	}
	devMan_->sync();
}

void NMidiMapper::play_list(QList<NMidiEventStr> *play_list, int playTime) {
	NMidiEventStr *m_evt;
	int i;
	NNote *note;
	for (m_evt = play_list->first(); m_evt; m_evt = play_list->next()) {
		if (m_evt->ev_time != playTime) continue;
		if (m_evt->special == SPEC_TRILL_UP) continue;
		if (!m_evt->valid) continue;
		switch(m_evt->midi_cmd) {
			case MNOTE_ON: m_evt->ref->draw(DRAW_DIRECT_RED); break;
			case MNOTE_OFF: m_evt->ref->draw(DRAW_DIRECT_BLACK); break;
		}
	}
	if (actualDevice_nr_ < 0) return;
	for (m_evt = play_list->first(); m_evt; m_evt = play_list->next()) {
		if (m_evt->ev_time != playTime || m_evt->midi_cmd != MNOTE_OFF) continue;
		if (!m_evt->valid) continue;
		for (note = m_evt->notelist->first(); note; note = m_evt->notelist->next()) {
			if (!(note->status & STAT_TIED) || (m_evt->special & TRILL_SPECS)) {
				devMan_->noteOff(m_evt->midi_channel, note->midiPitch+m_evt->trilloffs, 0);
			}
		}
/*
		delete m_evt;
*/
	}
	for (m_evt = play_list->first(); m_evt; m_evt = play_list->next()) {
		if (m_evt->ev_time != playTime) continue;
		switch(m_evt->midi_cmd) {
		   case MNOTE_ON:
			if (m_evt->ev_time ==  playTime && m_evt->midi_prog_change >= 0) {
				devMan_->chnPatchChange ( m_evt->midi_channel, m_evt->midi_prog_change);
			}
			if (m_evt->volum_ctrl_change >= 0) {
				devMan_->chnController(m_evt->midi_channel, 0x07, m_evt->volum_ctrl_change);
			}
			if (NResource::useMidiPedal_) {
				if (m_evt->status & MIDI_STAT_PEDAL_ON) {
    					devMan_->chnController(m_evt->midi_channel, 0x40, 100);
				}
				if (m_evt->status & MIDI_STAT_PEDAL_OFF) {
    					devMan_->chnController(m_evt->midi_channel, 0x40, 1);
				}
			}
			for (i = 0, note = m_evt->notelist->first(); note; note = m_evt->notelist->next(), i++) {
			   if ((note->status & STAT_PART_OF_TIE) && !(m_evt->special & TRILL_SPECS)) {
					note->midiPitch = note->tie_backward->midiPitch; /* for note off */
				}
			   	else if (m_evt->special != SPEC_ARPEGGIO || i == m_evt->arpegg_current) {
					devMan_->noteOn(m_evt->midi_channel, note->midiPitch+m_evt->trilloffs, m_evt->volume);
				}
			}
			break;
		   case MVOL_CONTROL:
			devMan_->chnController(m_evt->midi_channel, 0x07, m_evt->volume);
			break;
		}
	}
	devMan_->sync();

}

void NMidiMapper::changeProg(int chn, int pgm) {
	if (actualDevice_nr_ < 0) return;
	devMan_->chnPatchChange(chn, pgm);
}

void NMidiMapper::changeReverb(int chn, int rev) {
	if (actualDevice_nr_ < 0) return;
	devMan_->chnController(chn, 0x5b, rev);
}

void NMidiMapper::changeChorus(int chn, int chor) {
	if (actualDevice_nr_ < 0) return;
	devMan_->chnController(chn, 0x5d, chor);
}

void NMidiMapper::changePan(int chn, int pan) {
	if (actualDevice_nr_ < 0) return;
	devMan_->chnController(chn, 0x0a, pan);
}

void NMidiMapper::changeDevice(unsigned int index) {
	if (actualDevice_nr_ < 0) return;
	devMan_->closeDev();
	devMan_->setDefaultDevice(index);
	devMan_->openDev();
	NResource::defMidiPort_ = index;
}

void NMidiMapper::playImmediately(NClef *clef, NChord *chord, int pgm, int chn, int vol, int transpose) {
	if (actualDevice_nr_ < 0) return;
	if (isInUse_) return;
	QList<NNote> *notelist;
	NNote *note;
	struct dir_notes_str *immListElem;
	if (!immList_.isEmpty())  {
		stopImmediateNotes();
	}
	notelist = chord->getNoteList();
	devMan_->chnPatchChange(chn, pgm);
	for (note = notelist->first(); note; note = notelist->next()) {
		immListElem = new dir_notes_str;
		immListElem->pitch = clef->line2midiTab_[note->line+LINE_OVERFLOW]+note->offs+clef->getShift()+transpose;
		immListElem->chan = chn;
		immList_.append(immListElem);
		devMan_->noteOn(chn, immListElem->pitch, vol);
	}
	devMan_->sync();
	QTimer::singleShot(200, this, SLOT(stopImmediateNotes()));
}

void NMidiMapper::playImmediately(NClef *clef, int keyline, int offs, int pgm, int chn, int vol, int transpose) {
	if (actualDevice_nr_ < 0) return;
	if (isInUse_) return;
	struct dir_notes_str *immListElem;
	if (!immList_.isEmpty())  {
		stopImmediateNotes();
	}
	devMan_->chnPatchChange(chn, pgm);
	immListElem = new dir_notes_str;
	immListElem->pitch = clef->line2midiTab_[keyline+LINE_OVERFLOW]+offs+clef->getShift()+transpose;
	immListElem->chan = chn;
	immList_.append(immListElem);
	devMan_->noteOn(chn, immListElem->pitch, vol);
	devMan_->sync();
	QTimer::singleShot(200, this, SLOT(stopImmediateNotes()));
}

void NMidiMapper::stopImmediateNotes() {
	struct dir_notes_str *immListElem;
	while (!immList_.isEmpty()) {
		immListElem = immList_.first();
		devMan_->noteOff(immListElem->chan, immListElem->pitch, 0);
		immList_.remove();
		immList_.first();
	}
	devMan_->sync();
}

#elif defined (WITH_TSE3)

NMidiMapper::NMidiMapper() : QObject() {
	theScheduler_ = 0;
	echoChannel_ = 0;
	theScheduler_ = 0;
	channelPool_ = 0x0;
	lastSelectedChannel_ = 0;
#ifdef TSE3_HAS_ARTS
	if (NResource::schedulerRequest_ & ARTS_SCHEDULER_REQUESTED) {
		try {
#if TSE3_MID_VERSION_NR < 2
			theScheduler_ = theARtsFactory_.createScheduler();
#else
			TSE3::Plt::UnixMidiSchedulerFactory::setPreferredPlatform(TSE3::Plt::UnixMidiSchedulerFactory::UnixPlatform_Arts);
			theScheduler_ = theFactory_.createScheduler();
#endif
			cout << "TSE3 aRts MIDI scheduler created" << endl;
		}
		catch (TSE3::MidiSchedulerError e) {
			cerr << "cannot create an aRts MIDI Scheduler" << endl;
		}
	}
#endif
	if (!theScheduler_ && (NResource::schedulerRequest_ & ALSA_SCHEDULER_REQUESTED)) {
		try {
#if TSE3_MID_VERSION_NR < 2
			theScheduler_ = theAlsaFactory_.createScheduler();
#else
			TSE3::Plt::UnixMidiSchedulerFactory::setPreferredPlatform(TSE3::Plt::UnixMidiSchedulerFactory::UnixPlatform_Alsa);
			theScheduler_ = theFactory_.createScheduler();
#endif
			cout << "TSE3 ALSA MIDI scheduler created" << endl;
		}
		catch (TSE3::MidiSchedulerError e) {
			cerr << "cannot create an ALSA MIDI Scheduler" << endl;
		}
	}
	if (!theScheduler_ && (NResource::schedulerRequest_ & OSS_SCHEDULER_REQUESTED)) {
		try {
#if TSE3_MID_VERSION_NR < 2
			theScheduler_ = theOSSFactory_.createScheduler();
#else
			TSE3::Plt::UnixMidiSchedulerFactory::setPreferredPlatform(TSE3::Plt::UnixMidiSchedulerFactory::UnixPlatform_OSS);
			theScheduler_ = theFactory_.createScheduler();
#endif
			cout << "TSE3 OSS MIDI scheduler created" << endl;
		}
		catch (TSE3::MidiSchedulerError e) {
			cerr << "cannot create an OSS MIDI Scheduler" << endl;
		}
	}
		
	isInUse_ = false;

	if (!theScheduler_) {
		actualDevice_nr_ = -1;
		cerr << "error opening Midi Device --> music cannot be played" << endl;
	}
	else {
#if TSE3_MID_VERSION_NR < 2
		for (int i = 0; i < theScheduler_->ports(); ++i) {
			deviceNameList += theScheduler_->portName(i);
		}
		actualDevice_nr_ = 0;
#else
		for (int i = 0; i < theScheduler_->numPorts(); ++i) {
			deviceNameList += theScheduler_->portName(
				theScheduler_->portNumber(i));
		}
		actualDevice_nr_ = theScheduler_->portNumber(0);
#endif
		if (NResource::midiPortSet_) {
#if TSE3_MID_VERSION_NR < 2
			if (NResource::defMidiPort_ >= theScheduler_->ports()  ||  NResource::defMidiPort_ < 0) {
#else
			if (NResource::defMidiPort_ >= theScheduler_->numPorts()  ||  NResource::defMidiPort_ < 0) {
#endif
				cerr << "There is no MIDI port " << NResource::defMidiPort_ << ". I try 0!" << endl;
			}
			else {
#if TSE3_MID_VERSION_NR < 2
				actualDevice_nr_ = NResource::defMidiPort_;
#else
				actualDevice_nr_ = theScheduler_->portNumber(NResource::defMidiPort_);
#endif
			}
		}
	}
	immList_.setAutoDelete(true);
}

NMidiMapper::~NMidiMapper() {
	if (theScheduler_) {
		delete theScheduler_;
		theScheduler_ = 0;
	}
}

NMidiMapper::NMidiMapper(TSE3::MidiScheduler *scheduler) : QObject() {
	theScheduler_ = scheduler;
	echoChannel_ = 0;
	isInUse_ = false;
	channelPool_ = 0x0;
	lastSelectedChannel_ = 0;

	if (!theScheduler_) {
		actualDevice_nr_ = -1;
		cerr << "error opening Midi Device --> music cannot be played" << endl;
	}
	else {
#if TSE3_MID_VERSION_NR < 2
		for (int i = 0; i < theScheduler_->ports(); ++i) {
			deviceNameList += theScheduler_->portName(i);
#else
		for (int i = 0; i < theScheduler_->numPorts(); ++i) {
			deviceNameList += theScheduler_->portName(
				theScheduler_->portNumber(i));
#endif
		}
		actualDevice_nr_ = 0;
		if (NResource::midiPortSet_) {
#if TSE3_MID_VERSION_NR < 2
			if (NResource::defMidiPort_ >= theScheduler_->ports()  ||  NResource::defMidiPort_ < 0) {
#else
			if (NResource::defMidiPort_ >= theScheduler_->numPorts()  ||  NResource::defMidiPort_ < 0) {
#endif
				cerr << "There is no MIDI port " << NResource::defMidiPort_ << ". I try 0!" << endl;
			}
			else {
#if TSE3_MID_VERSION_NR < 2
				actualDevice_nr_ = NResource::defMidiPort_;
#else
				actualDevice_nr_ = theScheduler_->portNumber(NResource::defMidiPort_);
#endif
			}
		}
	}
	immList_.setAutoDelete(true);
}

void NMidiMapper::openDevice() {
}


void NMidiMapper::stopAllNotes(QList<NMidiEventStr> *play_list) {
	NMidiEventStr *m_evt;
	NNote *note;
	int i;
	if (actualDevice_nr_ < 0) return;
	for (m_evt = play_list->first(); m_evt; m_evt = play_list->next()) {
		if (m_evt->midi_cmd != MNOTE_OFF) continue;
		for (note = m_evt->notelist->first(); note; note = m_evt->notelist->next()) {
			theScheduler_->tx(TSE3::MidiCommand(TSE3::MidiCommand_NoteOff,
					   m_evt->midi_channel, actualDevice_nr_, note->midiPitch, 0));
		}
	}
	/* To be very shure: */
	for (i = 0; i < 16; i++) {
    		theScheduler_->tx(TSE3::MidiCommand(TSE3::MidiCommand_ControlChange, i, actualDevice_nr_, TSE3::MidiControl_AllNotesOff, 0));
		if (NResource::useMidiPedal_) {
			/* close an eventually open pedal: */
			theScheduler_->tx(TSE3::MidiCommand(TSE3::MidiCommand_ControlChange, i, actualDevice_nr_, TSE3::MidiControl_SustainPedal, 0));
		}
	}
}

void NMidiMapper::play_list(QList<NMidiEventStr> *play_list, int playTime) {
	NMidiEventStr *m_evt;
	NNote *note;
	int i;
	for (m_evt = play_list->first(); m_evt; m_evt = play_list->next()) {
		if (m_evt->ev_time != playTime) continue;
		if (m_evt->special == SPEC_TRILL_UP) continue;
		if (!m_evt->valid) continue;
		switch(m_evt->midi_cmd) {
			case MNOTE_ON: m_evt->ref->draw(DRAW_DIRECT_RED); break;
			case MNOTE_OFF: m_evt->ref->draw(DRAW_DIRECT_BLACK); break;
		}
	}
	if (actualDevice_nr_ < 0) return;
	for (m_evt = play_list->first(); m_evt; m_evt = play_list->next()) {
		if (m_evt->ev_time != playTime || m_evt->midi_cmd != MNOTE_OFF) continue;
		if (!m_evt->valid) continue;
		for (note = m_evt->notelist->first(); note; note = m_evt->notelist->next()) {
			if (!(note->status & STAT_TIED) || (m_evt->special & TRILL_SPECS)) {
				
				theScheduler_->tx(TSE3::MidiCommand(TSE3::MidiCommand_NoteOff,
						   m_evt->midi_channel, actualDevice_nr_, note->midiPitch+m_evt->trilloffs, 0));
			}

		}
	}
	for (m_evt = play_list->first(); m_evt; m_evt = play_list->next()) {
		if (m_evt->ev_time != playTime) continue;
		switch(m_evt->midi_cmd) {
		   case MNOTE_ON:
			if (m_evt->midi_prog_change >= 0) {
				theScheduler_->tx(TSE3::MidiCommand(TSE3::MidiCommand_ProgramChange,
					m_evt->midi_channel, actualDevice_nr_, m_evt->midi_prog_change));
			}
			if (m_evt->volum_ctrl_change >= 0) {
				theScheduler_->tx(TSE3::MidiCommand(TSE3::MidiCommand_ControlChange, m_evt->midi_channel, actualDevice_nr_,
				TSE3::MidiControl_ChannelVolumeMSB, m_evt->volum_ctrl_change));
			}
			if (NResource::useMidiPedal_) {
				if (m_evt->status & MIDI_STAT_PEDAL_ON) {
    					theScheduler_->tx(TSE3::MidiCommand(TSE3::MidiCommand_ControlChange, m_evt->midi_channel, actualDevice_nr_,
						TSE3::MidiControl_SustainPedal, 127));
				}
				if (m_evt->status & MIDI_STAT_PEDAL_OFF) {
    					theScheduler_->tx(TSE3::MidiCommand(TSE3::MidiCommand_ControlChange, m_evt->midi_channel, actualDevice_nr_,
						TSE3::MidiControl_SustainPedal, 0));
				}
			}
			for (i = 0, note = m_evt->notelist->first(); note; note = m_evt->notelist->next(), i++) {
			   if ((note->status & STAT_PART_OF_TIE) && !(m_evt->special & TRILL_SPECS)) {
			        note->midiPitch = note->tie_backward->midiPitch; /* for note off */
			   }
			   else if (m_evt->special != SPEC_ARPEGGIO || i == m_evt->arpegg_current) {
				theScheduler_->tx(TSE3::MidiCommand(TSE3::MidiCommand_NoteOn, m_evt->midi_channel, actualDevice_nr_, note->midiPitch+ m_evt->trilloffs, m_evt->volume));
			   }
			}
			break;
		   case MVOL_CONTROL:
			theScheduler_->tx(TSE3::MidiCommand(TSE3::MidiCommand_ControlChange, m_evt->midi_channel, actualDevice_nr_,
				TSE3::MidiControl_ChannelVolumeMSB, m_evt->volume));

			break;
		}
	}
	/* devMan_->sync(); */
}
void NMidiMapper::changeProg(int chn, int pgm) {
	if (actualDevice_nr_ < 0) return;
    	theScheduler_->tx(TSE3::MidiCommand(TSE3::MidiCommand_ProgramChange, chn, actualDevice_nr_, pgm));
}


void NMidiMapper::changeReverb(int chn, int rev) {
	if (actualDevice_nr_ < 0) return;
    	theScheduler_->tx(TSE3::MidiCommand(TSE3::MidiCommand_ControlChange, chn, actualDevice_nr_, TSE3::MidiControl_ReverbDepth, rev));
}

void NMidiMapper::changePan(int chn, int pan) {
	if (actualDevice_nr_ < 0) return;
    	theScheduler_->tx(TSE3::MidiCommand(TSE3::MidiCommand_ControlChange, chn, actualDevice_nr_, TSE3::MidiControl_PanMSB , pan));
}

void NMidiMapper::changeChorus(int chn, int chor) {
	if (actualDevice_nr_ < 0) return;
    	theScheduler_->tx(TSE3::MidiCommand(TSE3::MidiCommand_ControlChange, chn, actualDevice_nr_, TSE3::MidiControl_ChorusDepth, chor));
}

void NMidiMapper::setEchoChannel(int chn, int prog) {
	if (actualDevice_nr_ < 0) return;
	theScheduler_->tx(TSE3::MidiCommand(TSE3::MidiCommand_ProgramChange, chn, actualDevice_nr_, prog));
	echoChannel_ = chn;
}

void NMidiMapper::changeDevice(unsigned int index) {
	if (actualDevice_nr_ < 0) return;
	NResource::defMidiPort_ = index;
	actualDevice_nr_ =
#if TSE3_MID_VERSION_NR < 2
		index;
#else
		theScheduler_->portNumber(index);
#endif
}

void NMidiMapper::playImmediately(NClef *clef, NChord *chord, int pgm, int chn, int vol, int transpose) {
	if (actualDevice_nr_ < 0) return;
	if (isInUse_) return;
	QList<NNote> *notelist;
	NNote *note;
	struct dir_notes_str *immListElem;
	if (!immList_.isEmpty())  {
		stopImmediateNotes();
	}
	notelist = chord->getNoteList();
	theScheduler_->tx(TSE3::MidiCommand(TSE3::MidiCommand_ProgramChange, chn, actualDevice_nr_, pgm));
	for (note = notelist->first(); note; note = notelist->next()) {
		immListElem = new dir_notes_str;
		immListElem->pitch = clef->line2midiTab_[note->line+LINE_OVERFLOW]+note->offs+clef->getShift()+transpose;
		immListElem->chan = chn;
		immList_.append(immListElem);
		theScheduler_->tx(TSE3::MidiCommand(TSE3::MidiCommand_NoteOn, chn, actualDevice_nr_,
				immListElem->pitch, vol));
	}
	QTimer::singleShot(200, this, SLOT(stopImmediateNotes()));
}

void NMidiMapper::playImmediately(NClef *clef, int keyline, int offs, int pgm, int chn, int vol, int transpose) {
	if (actualDevice_nr_ < 0) return;
	struct dir_notes_str *immListElem;
	if (isInUse_) return;
	if (!immList_.isEmpty())  {
		stopImmediateNotes();
	}
	theScheduler_->tx(TSE3::MidiCommand(TSE3::MidiCommand_ProgramChange, chn, actualDevice_nr_, pgm));
	immListElem = new dir_notes_str;
	immListElem->pitch = clef->line2midiTab_[keyline+LINE_OVERFLOW]+offs+clef->getShift()+transpose;
	immListElem->chan = chn;
	immList_.append(immListElem);
	theScheduler_->tx(TSE3::MidiCommand(TSE3::MidiCommand_NoteOn, chn, actualDevice_nr_,
		immListElem->pitch, vol));
	QTimer::singleShot(200, this, SLOT(stopImmediateNotes()));
}

void NMidiMapper::stopImmediateNotes() {
	struct dir_notes_str *immListElem;
	while (!immList_.isEmpty()) {
		immListElem = immList_.first();
		theScheduler_->tx(TSE3::MidiCommand(TSE3::MidiCommand_NoteOff,
                                 immListElem->chan, actualDevice_nr_, immListElem->pitch, 0));
		immList_.remove();
		immList_.first();
	}
}
	

	



#else // DIRECT ACCESS VIA "soundcard.h"

#define SEQUENCER "/dev/sequencer"
#define TRACKS 16
SEQ_DEFINEBUF(128);

#ifndef SEQ_PGM_CHANGE
#   define SEQ_PGM_CHANGE(dev, chn, patch) \
                _CHN_COMMON(dev, MIDI_PGM_CHANGE, chn, patch, 0, 0)
#endif

static int seqfd;
void seqbuf_dump() {
        if (_seqbufptr)
                if (write(seqfd, _seqbuf, _seqbufptr)  < -1) {
                        perror("write device");
                        exit(10);
                }
        _seqbufptr = 0;
}


NMidiMapper::NMidiMapper() : QObject() {
	int i;

	isInUse_ = false;
	channelPool_ = 0x0;
	lastSelectedChannel_ = 0;

	if ((seqfd = open(SEQUENCER, O_WRONLY)) < 0) {
		cerr << "error opening sequencer" << endl;
		actualDevice_nr_ = -1;
	}
	else {
		ioctl(seqfd, SNDCTL_SEQ_NRSYNTHS, &synthDevs_);
		ioctl(seqfd, SNDCTL_SEQ_NRMIDIS, &midiDevs_);

		if (synthDevs_ || midiDevs_) {
			if ((devices_ = new devinfo[synthDevs_+midiDevs_]) == NULL) {
				cerr << "memory allocation error" << endl;
				exit(10);
			}
			actualDevice_nr_ = -1;
			for (i = 0; i < synthDevs_; ++i) {
				devices_[i].devdata.synth_inf.device = i;
				ioctl(seqfd, SNDCTL_SYNTH_INFO, &(devices_[i].devdata.synth_inf));
				devices_[i].dev_type = SYNTH_DEV;
				ioctl(seqfd, SNDCTL_SEQ_RESET, devices_[i].devdata.synth_inf.device);
				actualDevice_ = &(devices_[i]);
				actualDevice_nr_ = devices_[i].devdata.synth_inf.device;
				play_func_ = &NMidiMapper::playSynthDev;
				deviceNameList += devices_[i].devdata.midi_inf.name;
			}

			for (; i < synthDevs_ + midiDevs_; ++i) {
				devices_[i].devdata.synth_inf.device = i - synthDevs_;
				ioctl(seqfd, SNDCTL_MIDI_INFO, &(devices_[i].devdata.midi_inf));
				devices_[i].dev_type = MIDI_DEV;
				if (actualDevice_nr_ < 0) {
					actualDevice_ = &(devices_[0]);
					actualDevice_nr_ = devices_[i].devdata.midi_inf.device;
					play_func_ = &NMidiMapper::playMidiDev;
				}
				deviceNameList += devices_[i].devdata.midi_inf.name;
				for (int j = 0; j < TRACKS; ++j) {
					SEQ_MIDIOUT(devices_[i].devdata.midi_inf.device, MIDI_PGM_CHANGE | j);
					SEQ_MIDIOUT(devices_[i].devdata.midi_inf.device, 0);
				}
			}
		}
		if (actualDevice_nr_ >= 0 && NResource::midiPortSet_) {
			if (NResource::defMidiPort_ >= midiDevs_+synthDevs_  ||  NResource::defMidiPort_ < 0) {
				cerr << "There is no MIDI port " << NResource::defMidiPort_ << ". I try 0!" << endl;
				actualDevice_nr_ = 0;
			}
			else {
				actualDevice_nr_ = NResource::defMidiPort_;
			}
		}
	}
	if (actualDevice_nr_ == -1) {
		cerr << "no devices found --> music cannot be played" << endl;
		play_func_ = &NMidiMapper::playNull;
	}
	else {
             actualDevice_ = &(devices_[actualDevice_nr_]);
		switch (actualDevice_ ->dev_type) {
			case MIDI_DEV:
				actualDevice_nr_ = actualDevice_->devdata.midi_inf.device;
				play_func_ = &NMidiMapper::playMidiDev;
				break;
			case SYNTH_DEV:
				actualDevice_nr_ = actualDevice_->devdata.synth_inf.device;
				play_func_ =  &NMidiMapper::playSynthDev;
				break;
		}
	}
	immList_.setAutoDelete(true);
}


void NMidiMapper::stopAllNotes(QList<NMidiEventStr> * /* dummy */) {
	int i;
	for ( i = 0; i < synthDevs_ + midiDevs_; i++ ) {
		ioctl(seqfd, SNDCTL_SEQ_RESET, i);
	}
}

void NMidiMapper::play_list(QList<NMidiEventStr> *play_list, int playTime) {
	(this->*play_func_)(play_list, playTime);
}

void NMidiMapper::changeProg(int chn, int pgm) {
	if (actualDevice_nr_ < 0) return;
	switch (devices_[actualDevice_nr_].dev_type) {
	case SYNTH_DEV: SEQ_PGM_CHANGE(actualDevice_nr_, chn, pgm);
			break;
	case MIDI_DEV:  SEQ_MIDIOUT(actualDevice_nr_, MIDI_PGM_CHANGE|chn);
			SEQ_MIDIOUT(actualDevice_nr_, pgm);
			break;
	}
}

void NMidiMapper::changeReverb(int chn, int rev) {
	if (actualDevice_nr_ < 0) return;
	switch (devices_[actualDevice_nr_].dev_type) {
		case SYNTH_DEV: SEQ_CONTROL(actualDevice_nr_, chn, 0x5b, rev); break;
		case MIDI_DEV : SEQ_MIDIOUT(actualDevice_nr_, MIDI_CTL_CHANGE|chn);
			        SEQ_MIDIOUT(actualDevice_nr_, 0x5b);
			        SEQ_MIDIOUT(actualDevice_nr_, rev);
				break;
	}
}

void NMidiMapper::changeChorus(int chn, int chor) {
	if (actualDevice_nr_ < 0) return;
	switch (devices_[actualDevice_nr_].dev_type) {
		case SYNTH_DEV: SEQ_CONTROL(actualDevice_nr_, chn, CTL_CHORUS_DEPTH, chor); break;
		case MIDI_DEV : SEQ_MIDIOUT(actualDevice_nr_, MIDI_CTL_CHANGE|chn);
			        SEQ_MIDIOUT(actualDevice_nr_, CTL_CHORUS_DEPTH);
			        SEQ_MIDIOUT(actualDevice_nr_, chor);
				break;
	}
}

void NMidiMapper::changePan(int chn, int pan) {
	if (actualDevice_nr_ < 0) return;
	switch (devices_[actualDevice_nr_].dev_type) {
		case SYNTH_DEV: SEQ_CONTROL(actualDevice_nr_, chn, CTL_PAN, pan); break;
		case MIDI_DEV : SEQ_MIDIOUT(actualDevice_nr_, MIDI_CTL_CHANGE|chn);
			        SEQ_MIDIOUT(actualDevice_nr_, CTL_PAN);
			        SEQ_MIDIOUT(actualDevice_nr_, pan);
				break;
	}
}

void NMidiMapper::playMidiDev(QList<NMidiEventStr> *play_list, int playTime) const{
	int i;
	NMidiEventStr *m_evt;
	NNote *note;
	for (m_evt = play_list->first(); m_evt; m_evt = play_list->next()) {
		for (m_evt = play_list->first(); m_evt; m_evt = play_list->next()) {
			if (m_evt->ev_time != playTime) continue;
			if (m_evt->special == SPEC_TRILL_UP) m_evt->trillDist;
			if (!m_evt->valid) continue;
			switch(m_evt->midi_cmd) {
				case MNOTE_ON: m_evt->ref->draw(DRAW_DIRECT_RED); break;
				case MNOTE_OFF: m_evt->ref->draw(DRAW_DIRECT_BLACK); break;
			}
		}
	}
	for (m_evt = play_list->first(); m_evt; m_evt = play_list->next()) {
		if (m_evt->ev_time != playTime || m_evt->midi_cmd != MNOTE_OFF) continue;
		if (!m_evt->valid) continue;
		for (note = m_evt->notelist->first(); note; note = m_evt->notelist->next()) {
			if (!(note->status & STAT_TIED) || (m_evt->special & TRILL_SPECS)) {
				SEQ_MIDIOUT(actualDevice_nr_, MIDI_NOTEOFF|m_evt->midi_channel);
				SEQ_MIDIOUT(actualDevice_nr_, note->midiPitch+m_evt->trilloffs);
				SEQ_MIDIOUT(actualDevice_nr_, 0);
			}
		}
/*
		delete m_evt;
*/
	}
	for (m_evt = play_list->first(); m_evt; m_evt = play_list->next()) {
		if (m_evt->ev_time != playTime) continue;
		switch(m_evt->midi_cmd) {
		case MNOTE_ON:
			if (m_evt->midi_prog_change >= 0) {
				SEQ_MIDIOUT(actualDevice_nr_, MIDI_PGM_CHANGE|m_evt->midi_channel);
				SEQ_MIDIOUT(actualDevice_nr_, m_evt->midi_prog_change);
			}
			if (m_evt->volum_ctrl_change >= 0) {
				SEQ_MIDIOUT(actualDevice_nr_, MIDI_CTL_CHANGE|m_evt->midi_channel);
				SEQ_MIDIOUT(actualDevice_nr_, CTL_MAIN_VOLUME);
				SEQ_MIDIOUT(actualDevice_nr_, m_evt->volum_ctrl_change);
			}
			if (NResource::useMidiPedal_) {
				if (m_evt->status & MIDI_STAT_PEDAL_ON) {
					SEQ_MIDIOUT(actualDevice_nr_, MIDI_CTL_CHANGE|m_evt->midi_channel);
					SEQ_MIDIOUT(actualDevice_nr_, CTL_SUSTAIN);
					SEQ_MIDIOUT(actualDevice_nr_, 100);
				}
				if (m_evt->status & MIDI_STAT_PEDAL_OFF) {
					SEQ_MIDIOUT(actualDevice_nr_, MIDI_CTL_CHANGE|m_evt->midi_channel);
					SEQ_MIDIOUT(actualDevice_nr_, CTL_SUSTAIN);
					SEQ_MIDIOUT(actualDevice_nr_, 1);
				}
			}
			for (i = 0, note = m_evt->notelist->first(); note; note = m_evt->notelist->next(), i++) {
				if ((note->status & STAT_PART_OF_TIE) && !(m_evt->special & TRILL_SPECS)) {
					note->midiPitch = note->tie_backward->midiPitch; /* for note off */
				}
			   	else if (m_evt->special != SPEC_ARPEGGIO || i == m_evt->arpegg_current) {
					SEQ_MIDIOUT(actualDevice_nr_, MIDI_NOTEON|m_evt->midi_channel);
					SEQ_MIDIOUT(actualDevice_nr_, note->midiPitch+m_evt->trilloffs);
					SEQ_MIDIOUT(actualDevice_nr_, m_evt->volume);
				}
			}
			break;
		   case MVOL_CONTROL:
			SEQ_MIDIOUT(actualDevice_nr_, MIDI_CTL_CHANGE|m_evt->midi_channel);
			SEQ_MIDIOUT(actualDevice_nr_, CTL_MAIN_VOLUME);
			SEQ_MIDIOUT(actualDevice_nr_, m_evt->volume);
			break;
		}
	}
	//SEQ_WAIT_TIME(m_evt->length);
	SEQ_DUMPBUF();
}

void NMidiMapper::playSynthDev(QList<NMidiEventStr> *play_list, int playTime) const{
	NNote *note;
	NMidiEventStr *m_evt;
	int i;

	SEQ_START_TIMER();
	for (m_evt = play_list->first(); m_evt; m_evt = play_list->next()) {
		if (m_evt->ev_time != playTime) continue;
		if (m_evt->special == SPEC_TRILL_UP) continue;
		if (!m_evt->valid) continue;
		switch(m_evt->midi_cmd) {
			case MNOTE_ON: m_evt->ref->draw(DRAW_DIRECT_RED); break;
			case MNOTE_OFF: m_evt->ref->draw(DRAW_DIRECT_BLACK); break;
		}
	}
	for (m_evt = play_list->first(); m_evt; m_evt = play_list->next()) {
		if (m_evt->ev_time != playTime || m_evt->midi_cmd != MNOTE_OFF) continue;
		if (!m_evt->valid) continue;
		for (note = m_evt->notelist->first(); note; note = m_evt->notelist->next()) {
			if (!(note->status & STAT_TIED) || (m_evt->special & TRILL_SPECS)) {
				SEQ_STOP_NOTE(actualDevice_nr_, m_evt->midi_channel, note->midiPitch+m_evt->trilloffs, 0);
			}
		}
/*
		delete m_evt;
*/
	}
	for (m_evt = play_list->first(); m_evt; m_evt = play_list->next()) {
		if (m_evt->ev_time != playTime) continue;
		switch(m_evt->midi_cmd) {
		   case MNOTE_ON:
			if (m_evt->midi_prog_change >= 0) {
				SEQ_PGM_CHANGE(actualDevice_nr_, m_evt->midi_channel, m_evt->midi_prog_change);
			}
			if (m_evt->volum_ctrl_change >= 0) {
				SEQ_CONTROL(actualDevice_nr_, m_evt->midi_channel, CTL_MAIN_VOLUME, m_evt->volum_ctrl_change);
			}
			for (i = 0, note = m_evt->notelist->first(); note; note = m_evt->notelist->next(), i++) {
				if ((note->status & STAT_PART_OF_TIE) && !(m_evt->special & TRILL_SPECS)) {
					note->midiPitch = note->tie_backward->midiPitch; /* for note off */
				}
			   	else if (m_evt->special != SPEC_ARPEGGIO || i == m_evt->arpegg_current) {
					SEQ_START_NOTE(actualDevice_nr_, m_evt->midi_channel, note->midiPitch+m_evt->trilloffs, m_evt->volume);
				}
			}
			break;
		   case MVOL_CONTROL:
			SEQ_CONTROL(actualDevice_nr_, m_evt->midi_channel, CTL_MAIN_VOLUME, m_evt->volume);
			break;
		}
	}
	//SEQ_DELTA_TIME(l);
	SEQ_DUMPBUF();
}

void NMidiMapper::playNull(QList<NMidiEventStr> *play_list, int playTime) const{
	NMidiEventStr *m_evt;

	for (m_evt = play_list->first(); m_evt; m_evt = play_list->next()) {
		if (m_evt->ev_time != playTime) continue;
		if (m_evt->special == SPEC_TRILL_UP) continue;
		if (!m_evt->valid) continue;
		m_evt->ref->draw(m_evt->midi_cmd == MNOTE_ON ? DRAW_DIRECT_RED : DRAW_DIRECT_BLACK);
		/* if (m_evt->midi_cmd == MNOTE_OFF) delete m_evt;  */
	}

}

void NMidiMapper::openDevice() {
}

NMidiMapper::~NMidiMapper() {
	close(seqfd);
}


void NMidiMapper::changeDevice(unsigned int index) {
	if (actualDevice_nr_ < 0) return;
	actualDevice_ = &(devices_[index]);
	switch (actualDevice_ ->dev_type) {
		case MIDI_DEV:
			actualDevice_nr_ = actualDevice_->devdata.midi_inf.device;
			play_func_ = &NMidiMapper::playMidiDev;
			break;
		case SYNTH_DEV:
			actualDevice_nr_ = actualDevice_->devdata.synth_inf.device;
			play_func_ =  &NMidiMapper::playSynthDev;
			break;
	}
	NResource::defMidiPort_ = index;
}

void NMidiMapper::playImmediately(NClef *clef, NChord *chord, int pgm, int chn, int vol, int transpose) {
	if (actualDevice_nr_ < 0) return;
	if (isInUse_) return;
	QList<NNote> *notelist;
	NNote *note;
	struct dir_notes_str *immListElem;
	if (!immList_.isEmpty())  {
		stopImmediateNotes();
	}
	notelist = chord->getNoteList();
	switch (devices_[actualDevice_nr_].dev_type) {
		case SYNTH_DEV: SEQ_PGM_CHANGE(actualDevice_nr_, chn, pgm); break;
		case MIDI_DEV : SEQ_MIDIOUT(actualDevice_nr_, MIDI_PGM_CHANGE|chn);
				SEQ_MIDIOUT(actualDevice_nr_, pgm);
				break;
	}
	for (note = notelist->first(); note; note = notelist->next()) {
		immListElem = new dir_notes_str;
		immListElem->pitch = clef->line2midiTab_[note->line+LINE_OVERFLOW]+note->offs+clef->getShift()+transpose;
		immListElem->chan = chn;
		immList_.append(immListElem);
		switch (devices_[actualDevice_nr_].dev_type) {
			case SYNTH_DEV: SEQ_START_NOTE(actualDevice_nr_, chn, immListElem->pitch, vol); break;
			case MIDI_DEV : SEQ_MIDIOUT(actualDevice_nr_, MIDI_NOTEON|chn);
					SEQ_MIDIOUT(actualDevice_nr_, immListElem->pitch);
					SEQ_MIDIOUT(actualDevice_nr_, vol);
					break;
		}
	}
	SEQ_DUMPBUF();
	QTimer::singleShot(200, this, SLOT(stopImmediateNotes()));
}

void NMidiMapper::playImmediately(NClef *clef, int keyline, int offs, int pgm, int chn, int vol, int transpose) {
	if (actualDevice_nr_ < 0) return;
	if (isInUse_) return;
	struct dir_notes_str *immListElem;
	if (!immList_.isEmpty())  {
		stopImmediateNotes();
	}
	switch (devices_[actualDevice_nr_].dev_type) {
		case SYNTH_DEV: SEQ_PGM_CHANGE(actualDevice_nr_, chn, pgm); break;
		case MIDI_DEV : SEQ_MIDIOUT(actualDevice_nr_, MIDI_PGM_CHANGE|chn);
				SEQ_MIDIOUT(actualDevice_nr_, pgm);
				break;
	}
	immListElem = new dir_notes_str;
	immListElem->pitch = clef->line2midiTab_[keyline+LINE_OVERFLOW]+offs+clef->getShift()+transpose;
	immListElem->chan = chn;
	immList_.append(immListElem);
	switch (devices_[actualDevice_nr_].dev_type) {
		case SYNTH_DEV: SEQ_START_NOTE(actualDevice_nr_, chn, immListElem->pitch, vol); break;
		case MIDI_DEV : SEQ_MIDIOUT(actualDevice_nr_, MIDI_NOTEON|chn);
				SEQ_MIDIOUT(actualDevice_nr_, immListElem->pitch);
				SEQ_MIDIOUT(actualDevice_nr_, vol);
				break;
	}
	SEQ_DUMPBUF();
	QTimer::singleShot(200, this, SLOT(stopImmediateNotes()));
}

void NMidiMapper::stopImmediateNotes() {
	struct dir_notes_str *immListElem;
	while (!immList_.isEmpty()) {
		immListElem = immList_.first();
		switch (devices_[actualDevice_nr_].dev_type) {
			case SYNTH_DEV: SEQ_STOP_NOTE(actualDevice_nr_, immListElem->chan, immListElem->pitch, 0); break;
			case MIDI_DEV : SEQ_MIDIOUT(actualDevice_nr_, MIDI_NOTEOFF|immListElem->chan);
					SEQ_MIDIOUT(actualDevice_nr_, immListElem->pitch);
					SEQ_MIDIOUT(actualDevice_nr_, 0);
					break;
		}
		immList_.remove();
		immList_.first();
	}
	SEQ_DUMPBUF();
}
#endif // WITH_LIBKMID


void NMidiMapper::setPaintDevice(NDbufferWidget *pd) {
	pd_ = pd;
}

#ifdef WITH_TSE3
QList<int> *NMidiMapper::readEvents() {
	TSE3::MidiEvent evt;
	TSE3::MidiCommand *midiOnCommand;
	QList<int> *pitches = 0;
	bool first = true;
	if (!theScheduler_) return 0;
	while (1) {
		evt = theScheduler_->rx();
		midiOnCommand = &(evt.data);
		switch (midiOnCommand->status) {
			case TSE3::MidiCommand_NoteOn:
				if (!midiOnCommand->data2) {
					theScheduler_->tx(TSE3::MidiCommand(TSE3::MidiCommand_NoteOff, echoChannel_, actualDevice_nr_,
                                	midiOnCommand->data1, midiOnCommand->data2));
					break;
				}
				if (first) {
					first = false;
					pitches = new QList<int>();
					pitches->setAutoDelete(true);
					readTime_.start();
				}
				theScheduler_->tx(TSE3::MidiCommand(TSE3::MidiCommand_NoteOn, echoChannel_, actualDevice_nr_,
				midiOnCommand->data1, midiOnCommand->data2));
				pitches->append(new int(midiOnCommand->data1));
				break;
			case TSE3::MidiCommand_NoteOff:
				theScheduler_->tx(TSE3::MidiCommand(TSE3::MidiCommand_NoteOff, echoChannel_, actualDevice_nr_,
				midiOnCommand->data1, midiOnCommand->data2));
				break;
			default: if (first) return 0; 
				 if (readTime_.elapsed () > 10) return pitches;
		}
	}
}
#endif

#include "midimapper.moc"
