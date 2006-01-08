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

// #include "config.h"
#ifdef WITH_TSE3
#if GCC_MAJ_VERS > 2
#include <fstream>
#else
#include <fstream.h>
#endif
#include <stdio.h>
#include "tse3handler.h"
#include <tse3/TSE3.h>
#include <tse3/MidiFile.h>
#include <tse3/TempoTrack.h>
#include <tse3/TimeSigTrack.h>
#include <tse3/KeySigTrack.h>
#include <tse3/TSE3MDL.h>
#include <tse3/PhraseList.h>
#include <tse3/util/Phrase.h>
#include "mainframewidget.h"
#include "timesig.h"
#include "midimapper.h"
#include "voice.h"
#include "staff.h"
#include "uiconnect.h"
#include "scaleedit_impl.h"
#include "tempotrack.h"
#include <qtextview.h>
#include <qslider.h>
#include <kprogress.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <qcheckbox.h>
#include <qlistview.h>
#include "miditimescale.h"

using namespace std;

#define DRUM_CHANNEL (10 - 1)

/* #define TRIPLET_RECOGNITION */
#ifdef TRIPLET_RECOGNITION
#define OWN_SNAP
#endif
#ifdef OWN_SNAP
static double SnapVal_, SnapVal_2_;
static int SnapInt_;
static int tripletSum_ = 0;
static double TripletSnapVal_, TripletSnapVal_2_;
static int TripletSnapInt_;
static int TripletPart1_, TripletPart2_;
static bool snapToTriplet_ = false;
static QPtrList<NMusElement> *tripletList_ = 0;
#endif


#ifdef WITH_TSE3

NTSE3Handler::NTSE3Handler(NMainFrameWidget *mainWidget) : QObject() {
	cout << "TSE3 Copyright information :" << TSE3::TSE3_Copyright() << " Version: "  << TSE3::TSE3_Version() << endl;
	mainWidget_ = mainWidget;
	theSong_ = 0;
	recTrack_ = 0;
	recNum_ = 0;
	numCounter_ = 0;
	info_ = new tse3InfoFrm( mainWidget );
	multiStaff_ =  new staffFrm( mainWidget );
	filterDialog_ = new filterFrm( mainWidget, false );
	metronomeDialog_ = new metronomFrm( mainWidget, this, false );
	connect(&recordtimer_, SIGNAL(timeout()), this, SLOT(TSE3recordNext()));
}

NTSE3Handler::~NTSE3Handler() {
	delete multiStaff_;
	delete filterDialog_;
	delete metronomeDialog_;
	delete info_;
}

#define MYTIME2TSE3TIME(t) ((int) (double) (TSE3::Clock::PPQN) * (double) (t) / (double) (QUARTER_LENGTH))
#define TSE3TIME2MYMIDITIME(t) ((int) ((double) (t) * (double) QUARTER_LENGTH / (double) (TSE3::Clock::PPQN)))

void NTSE3Handler::createTSE3(QPtrList<NVoice> *voices) {
	NVoice *voice;
	NSign *sign;
	int i;
	NTempoTrack SortedTempoSigs;
	if (theSong_) delete theSong_;
	theSong_ = new TSE3::Song(0);
	TSE3::Tempo *tt = new TSE3::Tempo(DEFAULT_TEMPO);
        TSE3::Event<TSE3::Tempo> *tempoEvent = new TSE3::Event<TSE3::Tempo>(*tt, 0);
	theSong_->tempoTrack()->insert(*tempoEvent);

	for (voice = voices->first(); voice; voice = voices->next()) {
		voice->getTempoSigs(&SortedTempoSigs, 0);
	}
	SortedTempoSigs.resolveRitardandoAndAccelerando();
	for (sign = SortedTempoSigs.first(); sign; sign = SortedTempoSigs.next()) {
		tt = new TSE3::Tempo(sign->getTempo());
		tempoEvent = new TSE3::Event<TSE3::Tempo>(*tt, MYTIME2TSE3TIME(sign->getRealMidiTime()));
		theSong_->tempoTrack()->insert(*tempoEvent);
	}
	SortedTempoSigs.clear();
	for (i = 0, voice = voices->first(); voice; voice = voices->next(), i++) {
		theSong_->insert(createTSE3Track(voice, i, theSong_));
	}
}

TSE3::Track *NTSE3Handler::createTSE3Track(NVoice *voice, int nr, TSE3::Song *song) {
	TSE3::Track  *myTrack = new TSE3::Track;
	TSE3::PhraseEdit thePhraseEditor;
	TSE3::TimeSig *tse3timesig;
	NNote *note;
	NVoice *firstVoice;
	NMidiEventStr *m_evt, *m_evt2, *m_events;
	TSE3::Event<TSE3::TimeSig> *tse3timesigEvent;
	TSE3::KeySig *tse3keysig;
	TSE3::Event<TSE3::KeySig> *tse3keysigEvent;
	NTimeSig *timesig;
	NStaff *actual_staff;
	int i;
	char trackName[20];
	unsigned int myTime, noteoff;
	int deviceNumber = NResource::mapper_->getActualDeviceNumber();
	QPtrList<NMidiEventStr> stopList;
	stopList.setAutoDelete(false);

	thePhraseEditor.reset();
	int sign = 0;
	property_type kind;

	voice->getStaff()->startPlaying();  /* necessary to reset some member variables in all staffs (EndIdx_) */
	actual_staff = voice->getStaff();
	if (actual_staff->actualKeysig_.isRegular(&kind, &sign)) {
		if (kind == PROP_FLAT) {
			sign = -sign;
		}
	}
	if (sign) {
		tse3keysig = new TSE3::KeySig(sign);
		tse3keysigEvent = new TSE3::Event<TSE3::KeySig>(*tse3keysig, TSE3::Clock(0));
		theSong_->keySigTrack()->insert(*tse3keysigEvent);
	}
	myTime = 0;
	thePhraseEditor.insert(TSE3::MidiEvent(TSE3::MidiCommand(TSE3::MidiCommand_ProgramChange, actual_staff->getChannel(), 
		 			   deviceNumber, actual_staff->getVoice()), 0)
	);
	thePhraseEditor.insert(TSE3::MidiEvent(TSE3::MidiCommand(TSE3::MidiCommand_ControlChange, actual_staff->getChannel(),
		 			   deviceNumber, TSE3::MidiControl_ReverbDepth, actual_staff->reverb_), 0)
	);
	thePhraseEditor.insert(TSE3::MidiEvent(TSE3::MidiCommand(TSE3::MidiCommand_ControlChange, actual_staff->getChannel(),
		 			   deviceNumber, TSE3::MidiControl_PanMSB, actual_staff->pan_), 0)
	);
	thePhraseEditor.insert(TSE3::MidiEvent(TSE3::MidiCommand(TSE3::MidiCommand_ControlChange, actual_staff->getChannel(),
		 			   deviceNumber, TSE3::MidiControl_ChorusDepth, actual_staff->chorus_), 0)
	);
	if (!voice->isFirstVoice()) {
		firstVoice = voice->getStaff()->getVoiceNr(0);
	}
	do {
		m_evt = voice->getNextMidiEvent(myTime, true);
		if (m_evt && !voice->isFirstVoice() && m_evt->special != SPEC_ARPEGGIO) {
			do { /* for handling of repeat */
				m_evt2 = firstVoice->getNextMidiEvent(myTime, false);
				if (m_evt2 && m_evt2->ev_time < m_evt->ev_time) firstVoice->skipAndInvalidate(); /* avoid actualMidiEvent_ handling */
			}
			while (m_evt2 && m_evt2->ev_time < m_evt->ev_time);
			voice->skipAndInvalidate(false);
			m_evt = voice->getNextMidiEvent(myTime, true); /* can be different because of left shift (repeat) */
		}
		if (m_evt) {
			m_events = stopList.first();
			while (m_events) {
				if (m_events->ev_time <= m_evt->ev_time) {
					noteoff = MYTIME2TSE3TIME(m_events->ev_time) - 1;
					for (note = m_events->notelist->first(); note; note = m_events->notelist->next()) {
					  if (!(note->properties & PROP_TIED) || (m_evt->special & TRILL_SPECS)) {
					    thePhraseEditor.insert(
					        TSE3::MidiEvent( TSE3::MidiCommand(TSE3::MidiCommand_NoteOff,
					           m_events->midi_channel, deviceNumber, note->midiPitch+m_events->trilloffs, 0), noteoff)
					    );
					  }
						
					}
					stopList.remove();
					m_events = stopList.current();
				}
				else {
					m_events = stopList.next();
				}
			}
			switch(m_evt->midi_cmd) {
			case  MNOTE_ON:
				if (m_evt->midi_prog_change >= 0)  {
					thePhraseEditor.insert(
				        TSE3::MidiEvent(TSE3::MidiCommand(TSE3::MidiCommand_ProgramChange, actual_staff->getChannel(),
					 deviceNumber, m_evt->midi_prog_change), MYTIME2TSE3TIME(m_evt->ev_time))
					);
				}
				if (m_evt->volum_ctrl_change >= 0) {
					thePhraseEditor.insert(
				        TSE3::MidiEvent(TSE3::MidiCommand(TSE3::MidiCommand_ControlChange, actual_staff->getChannel(),
					 deviceNumber, TSE3::MidiControl_ChannelVolumeMSB, m_evt->volum_ctrl_change), MYTIME2TSE3TIME(m_evt->ev_time))
					);
				}
				if (NResource::useMidiPedal_) {
					if (m_evt->status & MIDI_STAT_PEDAL_ON) {
						thePhraseEditor.insert(
				        	TSE3::MidiEvent(TSE3::MidiCommand(TSE3::MidiCommand_ControlChange, actual_staff->getChannel(),
					 	deviceNumber, TSE3::MidiControl_SustainPedal, 100), MYTIME2TSE3TIME(m_evt->ev_time))
						);
					}
					if (m_evt->status & MIDI_STAT_PEDAL_OFF) {
						thePhraseEditor.insert(
				        	TSE3::MidiEvent(TSE3::MidiCommand(TSE3::MidiCommand_ControlChange, actual_staff->getChannel(),
					 	deviceNumber, TSE3::MidiControl_SustainPedal, 1), MYTIME2TSE3TIME(m_evt->ev_time))
						);
					}
				}
				for (i = 0, note = m_evt->notelist->first(); note; note = m_evt->notelist->next(), i++) {
			   		if ((note->properties & PROP_PART_OF_TIE) && !(m_evt->special & TRILL_SPECS)) {
						note->midiPitch = note->tie_backward->midiPitch; /* for note off */
					}
			   		else if (m_evt->special != SPEC_ARPEGGIO || i == m_evt->arpegg_current) {
					    thePhraseEditor.insert(
					        TSE3::MidiEvent(TSE3::MidiCommand(TSE3::MidiCommand_NoteOn,
					           m_evt->midi_channel, deviceNumber, note->midiPitch+m_evt->trilloffs, m_evt->volume), MYTIME2TSE3TIME(m_evt->ev_time))
					   );
					}
				}
				if (m_evt->notehalt->valid) {
					m_evt->notehalt->ev_time = m_evt->ev_time + m_evt->length;
					stopList.append(m_evt->notehalt);
				}
				myTime = MYTIME2TSE3TIME(m_evt->ev_time + 1);
				voice->skipChord();
				break;
			case MTIMESIG:
				timesig = (NTimeSig *) m_evt->ref;
				tse3timesig = new TSE3::TimeSig(timesig->getNumerator(), timesig->getDenominator());
				tse3timesigEvent = new TSE3::Event<TSE3::TimeSig>(*tse3timesig, TSE3::Clock(MYTIME2TSE3TIME(m_evt->ev_time)));
				theSong_->timeSigTrack()->insert(*tse3timesigEvent);
				break;
		        case MVOL_CONTROL:
				thePhraseEditor.insert(
				   TSE3::MidiEvent(TSE3::MidiCommand(TSE3::MidiCommand_ControlChange, actual_staff->getChannel(),
				 deviceNumber, TSE3::MidiControl_ChannelVolumeMSB, m_evt->volume), MYTIME2TSE3TIME(m_evt->ev_time))
				);
				if (m_evt->notehalt->valid) {
					m_evt->notehalt->ev_time = m_evt->ev_time + m_evt->length;
					stopList.append(m_evt->notehalt);
				}
				voice->skipChord();
				break;
			}
		}
	}
	while (m_evt);
	m_events = stopList.first();
	while (m_events) {
		noteoff = MYTIME2TSE3TIME(m_events->ev_time) - 1;
		for (note = m_events->notelist->first(); note; note = m_events->notelist->next()) {
			if (!(note->properties & PROP_TIED)) {
				thePhraseEditor.insert(
				        TSE3::MidiEvent( TSE3::MidiCommand(TSE3::MidiCommand_NoteOff,
					           m_events->midi_channel, deviceNumber, note->midiPitch+m_events->trilloffs, 0), noteoff)
				    );
			}
		}
		stopList.remove();
		m_events = stopList.current();
	}
	voice->stopPlaying();
	thePhraseEditor.tidy();
	sprintf(trackName, "Track%d", nr); // PJG
	TSE3::Phrase *phr = thePhraseEditor.createPhrase(song->phraseList(), song->phraseList()->newPhraseTitle(trackName)); // PJG
	//phr->setTitle();
	//song->phraseList()->insert(phr);

	TSE3::Part *myPart = new TSE3::Part;
	myPart->setPhrase(phr);
	myPart->setStart(0);
	myPart->setEnd(myTime);

	myTrack->insert(myPart);
	return myTrack;
}

void NTSE3Handler::playSong() {
	if (!theSong_) {
		KMessageBox::sorry
			(0,
			 i18n("First create a TSE3 song structure!"),
			 kapp->makeStdCaption(i18n("Play TSE3 song"))
			);
		return;
	}

    	TSE3::Transport myTransport(new TSE3::Metronome(), NResource::mapper_->theScheduler_);
	
    	myTransport.play(theSong_, TSE3::Clock (0));
	
    	while (myTransport.status() != TSE3::Transport::Resting) {
        	// We have to call poll frequently to produce output
        	myTransport.poll();
    	}
	
    	if (myTransport.status() != TSE3::Transport::Resting) {
        	cout << "Manually stopping the transport since it's "
             	<< NResource::mapper_->theScheduler_->clock() << " and we're still going.\n";
        	myTransport.stop();
    	}
}

bool NTSE3Handler::writeTSE3(const char *fname) {
	if (theSong_ == 0) {
		KMessageBox::sorry
			(0,
			 i18n("First create a TSE3 song structure!"),
			 kapp->makeStdCaption(i18n("Write TSE3 song"))
			);
		return false;
	}
	TSE3::TSE3MDL tse3mdl("noteedit");
	tse3mdl.save(fname, theSong_);
	return true;
}

bool NTSE3Handler::readTSE3(const char *fname) {
	TSE3::Song *newsong;
	TSE3::TSE3MDL tse3mdl("noteedit");
	if (!(newsong = tse3mdl.load(fname))) {
		return false;
	}
	if (theSong_) delete theSong_;
	theSong_ = newsong;
	return true;
}

bool NTSE3Handler::TSE3MidiOut(const char *fname) {
	if (theSong_ == 0) {
		KMessageBox::sorry
			(0,
			 i18n("First create a TSE3 song structure!"),
			 kapp->makeStdCaption(i18n("TSE3 MIDI out"))
			);
		return false;
	}
	TSE3::MidiFileExport midiexport;
	try {
		midiexport.save(fname, theSong_);
	}
	catch (TSE3::MidiFileExportError e) {
		return false;
	}
	return true;
}

bool NTSE3Handler::TSE3MidiIn(const char *fname) {
	try {
		TSE3::MidiFileImport midiimport(fname);
		theSong_ = midiimport.load();
	}
	catch (TSE3::MidiFileImportError) {
		return false;
	}
	return true;
}

void NTSE3Handler::TSE3recordNext() {
	TSE3::Part *myPart;
	QString s;
	TSE3::Clock stoptime;

	recTransport_->poll();
	if (mainWidget_->stillRecording()) {
		recordtimer_.start(10, true);
		return;
	}
	myPart = new TSE3::Part();
	recTrack_ = new TSE3::Track();
	recTransport_->stop();
	stoptime = NResource::mapper_->theScheduler_->clock();
	recPhraseEdit_->tidy();
	s.sprintf("rec%d", recNum_++); // PJG
	recPhrase_ = recPhraseEdit_->createPhrase(theSong_->phraseList(), s.ascii()); // PJG
	//recPhrase_->setTitle(s.ascii()); // PJG
	myPart->setPhrase(recPhrase_);
	myPart->setStart(TSE3::Clock (0));
	myPart->setEnd(NResource::mapper_->theScheduler_->clock());
	recTrack_->insert(myPart);
	delete recTransport_;
}

void NTSE3Handler::TSE3PhraseEditToStaff(TSE3::PhraseEdit *phraseEdit, NStaff *staff) {
	TSE3::Part *myPart;
	QString s;
	TSE3::PhraseList myPhraseList; /*dummy */
	NVoice *voice;
	int noteCount;

	voice = staff->getVoiceNr(0);
	voice->emptyVoice();
	myPart = new TSE3::Part();
	recTrack_ = new TSE3::Track();
	phraseEdit->tidy();
	s.sprintf("rec%d", recNum_++);
	recPhrase_ = phraseEdit->createPhrase(&myPhraseList, s.ascii());
	myPart->setPhrase(recPhrase_);
	myPart->setStart(TSE3::Clock (0));
	myPart->setEnd(TSE3::Clock(100000));
	recTrack_->insert(myPart);
	if (!TSE3TrackLimits(recTrack_->iterator(TSE3::Clock (0)), &noteCount)) {
		KMessageBox::error
			(0,
			 i18n("unable to determine a clef"),
			 kapp->makeStdCaption(i18n("???")) //  FIXME: What should it say here?
			);
		emit endRecorded(false);
		return;
	}
	if (!noteCount) {
		KMessageBox::sorry
			(0,
			 i18n("No notes recorded"),
			 kapp->makeStdCaption(i18n("???")) //  FIXME: What should it say here?
			);
		emit endRecorded(false);
		return;
	}
	if (staffsPerTrack_ != 1) {
		KMessageBox::error
			(0,
			 i18n("This cannot be imported to ONE staff"),
			 kapp->makeStdCaption(i18n("???")) //  FIXME: What should it say here?
			);
		emit endRecorded(false);
		return;
	}
	TSE3Track2Staff(0, staff, recTrack_, false);
	emit endRecorded(true);
}

bool  NTSE3Handler::TSE3record(int channel, QPtrList<NVoice> *voices) {
	if (theSong_ == 0) {
#ifdef IS_NOT_SO_GOOG
		KMessageBox::sorry
			(0,
			 i18n("First create a TSE3 song structure!"),
			 kapp->makeStdCaption(i18n("Record AAAAAAAAAAAAAA")) //  FIXME: What should it say here?
			);
		return false;
#endif
		createTSE3(voices);
	}
	targetChannel_ = channel;
	filterDialog_->show();
	metronomeDialog_->metDev->setCurrentItem( NResource::defMidiPort_ );
	metronomeDialog_->show();
	metronomeDialog_->reactivate();
	return true;
}

void  NTSE3Handler::doRecord() {
	if (theSong_ == 0) {
		KMessageBox::sorry
			(0,
			 i18n("First create a TSE3 song structure!"),
			 kapp->makeStdCaption(i18n("Record BBBBBBBBBBB")) //  FIXME: What should it say here?
			);
		return;
	}
	NResource::mapper_->theScheduler_->setTempo(metronomeDialog_->metTempo->getValue(), TSE3::Clock(0));
	theSong_->tempoTrack()->insert(TSE3::Event<TSE3::Tempo>(TSE3::Tempo(metronomeDialog_->metTempo->getValue()), TSE3::Clock(0)));
	recTransport_ = new TSE3::Transport(&recMetronome_, NResource::mapper_->theScheduler_);
	recMetronome_.setBeatNote(metronomeDialog_->metBeat->getValue());
	recMetronome_.setBarNote(metronomeDialog_->metBar->getValue());
#if TSE3_MID_VERSION_NR < 2
	recMetronome_.setPort(metronomeDialog_->metDev->currentItem());
	recTransport_->filter()->setPort(metronomeDialog_->metDev->currentItem());
	recTransport_->midiEcho()->setPort(metronomeDialog_->metDev->currentItem()); 
	recTransport_->midiEcho()->setChannel(targetChannel_); 
#else
	recMetronome_.setPort(NResource::mapper_->theScheduler_->portNumber(metronomeDialog_->metDev->currentItem()));
	recTransport_->filter()->setPort(NResource::mapper_->theScheduler_->portNumber(metronomeDialog_->metDev->currentItem()));
	recTransport_->midiEcho()->filter()->setPort(NResource::mapper_->theScheduler_->portNumber(metronomeDialog_->metDev->currentItem())); 
	recTransport_->midiEcho()->filter()->setChannel(targetChannel_); 
#endif
	recPhraseEdit_ = new TSE3::PhraseEdit();
	
	recTransport_->record(theSong_, TSE3::Clock(0), recPhraseEdit_, 0);
	recordtimer_.start(100, true);
} 

void NTSE3Handler::printSongInformation() {
	QTextView *textview;
	TSE3::Track *tr;
	int noteCount, i, j;
	if (theSong_ == 0) {
		KMessageBox::sorry
			(0,
			 i18n("First create a TSE3 song structure!"),
			 kapp->makeStdCaption(i18n("Song information"))
			);
		return;
	}
	info_->table->clear();
	items_ = new QListViewItem*[4*theSong_->size()];
	int l = 0;
	items_[l] = new QCheckListItem( (QListView *) info_->table, "" );
	for (i = 0; i < theSong_->size(); ++i) {
		tr = (*theSong_)[i];
		TSE3TrackLimits(tr->iterator(TSE3::Clock(0)), &noteCount);
		items_[l]->setText(0, i18n("Track %1%2").arg(i < 9 ? " " : "").arg( i + 1 ));
		items_[l]->setText(1, i18n("%1 Notes").arg(noteCount));
		items_[l]->setText( 2, QString( "%1" ).arg( staffsPerTrack_ ) );
		for (j = 0; j < staffsPerTrack_; ++j) {
			items_[l]->setText(3, i18n("Staff %1%2").arg(j < 9 ? " " : "").arg(j + 1));
			items_[l]->setText(4, i18n(NResource::instrTab[TrackVoice_]));
			items_[l]->setText(5, i18n("Channel %1").arg(staffInfo_[j].channel));
			items_[l]->setText(6, i18n("Volume %1").arg( staffInfo_[j].volume));
			items_[l]->setText(7, staffInfo_[j].clefType == BASS_CLEF ? i18n("bass") : i18n("treble"));
			items_[l++]->setText(8, i18n("Shift %1").arg(staffInfo_[j].clefShift));
			items_[l] = new QCheckListItem( (QListView *) info_->table, "" );			
		}
	}
	info_->show();
	delete [] items_;
}

void NTSE3Handler::initFiltering() {
#if QT_VERSION >= 300
	filterDialog_->exec();
#else
	filterDialog_->show();
#endif
}

				

void NTSE3Handler::TSE3Merge() {
	if (theSong_ == 0) {
		KMessageBox::sorry
			(0,
			 i18n("First create a TSE3 song structure!"),
			 kapp->makeStdCaption(i18n("Merge"))
			);
		return;
	}

	if (NResource::staffSelMerge_) delete [] NResource::staffSelMerge_;
	NResource::staffSelMerge_ = 0;
	
	multiStaff_->boot(0, STAFF_ID_MERGE, theSong_->size() );
	int i, z;
	TSE3::Track *tr;
	if( multiStaff_->abort_ ) return;
	if (!NResource::staffSelMerge_) return;
	TSE3::Track *newTrack = new TSE3::Track();
	TSE3::Phrase *newPhrase;
	TSE3::Clock maxtime, endtime;
	TSE3::Part *newPart = new TSE3::Part(), *part;
	QPtrList<TSE3::Track> removeableTracks;
	QString phraseTitle;

	for (z = 0, i = 0; i < theSong_->size(); ++i) {
		if (NResource::staffSelMerge_[i]) z++;
	}
	if (z < 2) return;
	std::vector<TSE3::Playable*> mergevec;
	for (i = 0; i < theSong_->size(); ++i) {
		if (NResource::staffSelMerge_[i]) {
			mergevec.push_back((*theSong_)[i]);
			tr =  (*theSong_)[i];
			removeableTracks.append(tr);
			part = (*tr)[0];
			endtime = part->end();
			if (endtime > maxtime) maxtime = endtime;
		}

	}
	phraseTitle.sprintf("Merged Phrase %d", numCounter_++); // PJG
    TSE3::PhraseEdit pe; // PJG
    TSE3::Util::Phrase_Merge(mergevec, &pe); // PJG
	//newPhrase = TSE3::Util::Phrase_Merge(mergevec); PJG
	//newPhrase->setTitle(phraseTitle.ascii()); PJG
    newPhrase = pe.createPhrase(theSong_->phraseList(), phraseTitle.ascii()); // PJG
	//theSong_->phraseList()->insert(newPhrase); PJG
	newPart->setPhrase(newPhrase);
	newPart->setEnd(maxtime);
	newTrack->insert(newPart);
	theSong_->insert(newTrack);
	for (tr = removeableTracks.first(); tr; tr = removeableTracks.next()) {
		theSong_->remove(tr);
	}
}
		

void NTSE3Handler::TSE3toScore(QPtrList<NStaff> *staffs, QPtrList<NVoice> *voices, bool keep) {
	oldvoiceList_ = voices;
	oldstaffList_ = staffs;
	if (theSong_ == 0) {
		emit endTSE3toScore(false);
		return;
	}

	keep_ = keep;
	if (keep_) {
		multiStaff_->boot( 0, STAFF_ID_TRACK, theSong_->size() );
		if( multiStaff_->abort_ ) return;
		bool f = false;
		for( int i = 0; i < theSong_->size(); ++i )
		    f += NResource::staffSelTrack_[i];
		if( !f ) {
			KMessageBox::sorry
				(0,
				 i18n("No staff selected for import."),
				 kapp->makeStdCaption(i18n("Import TSE3")));
			return;
		}
	}
	else {
		NResource::staffSelTrack_ = new bool[theSong_->size()];  // dummy
	}
	TSE3::Track *tr;
	TSE3::PlayableIterator *ttritr;
	TSE3::PhraseEdit phraseEdit;
	NStaff *staff;
	NVoice *voice;
	QPtrList<NVoice> newVoices;
	QPtrList<NStaff> newStaffs;
	NSign *sign;
	unsigned int i, j, k, l;
	int noteCount;
	int oldstaffCount, newStaffCount;
	NStaff *newStaffElem, *oldStaffElem;
	double partTime = (double) theSong_->size() / ( 100.0 * FIRST_PART_TIME ) + 1.e-20;
	NResource::progress_->setValue(0);
	NResource::progress_->show();

	for (i = 0; i < theSong_->size(); ++i) {
		tr = (*theSong_)[i];
		if (filterDialog_->filAvVol->isChecked()) {
			tr->filter()->setVelocityScale(filterDialog_->filVelSca->getValue());
			tr->filter()->setMaxVelocity(filterDialog_->filVal2->getValue());
			if (filterDialog_->filVal1->getValue() <= filterDialog_->filVal2->getValue()) {
				tr->filter()->setMinVelocity(filterDialog_->filVal1->getValue());
			}
			else {
				tr->filter()->setMinVelocity(filterDialog_->filVal2->getValue());
			}
		}
		else {
			tr->filter()->setVelocityScale(100); tr->filter()->setMaxVelocity(127); tr->filter()->setMinVelocity(0);
		}
	}

	if ((trackInfoArray_ = (int *) malloc(4 * theSong_->size() * sizeof(int))) == NULL) {
		NResource::abort("NTSE3Handler::TSE3toScore");
	}
	l = k = 0;
	for (i = 0; i < theSong_->size(); ++i) {
		NResource::progress_->setValue((int) ((double) (i+1) / partTime));
		tr = (*theSong_)[i];
		if (!TSE3TrackLimits(tr->iterator(TSE3::Clock (0)), &noteCount)) {
			if (!noteCount) continue;
			NResource::progress_->hide();
			emit endTSE3toScore(false);
			delete [] NResource::staffSelTrack_;
			NResource::staffSelTrack_ = 0;
			return;
		}
	
		++l;
		for (j = 0; j < staffsPerTrack_; ++j) {
			staff = new NStaff(k*(NResource::overlength_+STAFF_HIGHT+NResource::underlength_)+NResource::overlength_, l, 0, mainWidget_);
			trackInfoArray_[k] = i;
			if (!keep_ || NResource::staffSelTrack_[trackInfoArray_[k]]) {
				if (!TSE3Track2Staff(j, staff, tr)) {
					NResource::progress_->hide();
					emit endTSE3toScore(false);
					delete [] NResource::staffSelTrack_;
					NResource::staffSelTrack_ = 0;
					return;
				}
			}
			++k;
			newStaffs.append(staff);
			for (voice = staff->voicelist_.first(); voice; voice = staff->voicelist_.next()) {
				newVoices.append(voice);
			}
		}
	}
	if (newVoices.count() < 1) {
		delete [] NResource::staffSelTrack_;
		NResource::staffSelTrack_ = 0;
		NResource::progress_->hide();
		emit endTSE3toScore(false);
		return;
	}

	ttritr = theSong_->tempoTrack()->iterator(TSE3::Clock(0));

	voice = newVoices.first();
	staff = voice->getStaff();

	while (ttritr->more()) {
		sign = new NSign(voice->getMainPropsAddr(), staff->getStaffPropsAddr(), TEMPO_SIGNATURE);
		sign->setTempo((*ttritr)->data.data2);
		voice->insertAtTime(TSE3TIME2MYMIDITIME((*ttritr)->time.pulses), sign);
		++(*ttritr);
	}

	delete ttritr;

	newStaffCount = newStaffs.count();
	if (!keep) {
		while (oldStaffElem = oldstaffList_->first()) {
			oldStaffElem->updateVoiceList(oldvoiceList_);
			delete oldStaffElem;
			oldstaffList_->remove();
		}
		for (i = 0; i < newStaffCount; i++) {
			newStaffElem = newStaffs.at(i);
			oldstaffList_->append(newStaffElem);
			j = newStaffElem->voiceCount();
			for (k = 0; k < j; k++) {
				oldvoiceList_->append(newStaffElem->getVoiceNr(k));
			}
		}
		emit endTSE3toScore(true);
		return;
	}
	oldstaffCount = oldstaffList_->count();
	for (i = 0; i < newStaffCount; i++) {
		newStaffElem = newStaffs.at(i);
		if (i < oldstaffCount) {
			if (NResource::staffSelTrack_[trackInfoArray_[i]]) {
				oldStaffElem = oldstaffList_->at(i);
				oldStaffElem->updateVoiceList(oldvoiceList_);
				oldstaffList_->remove();
				oldstaffList_->insert(i, newStaffElem);
				j = newStaffElem->voiceCount();
				for (k = 0; k < j; k++) {
					oldvoiceList_->append(newStaffElem->getVoiceNr(k));
				}
			}
		}
		else {
			oldstaffList_->append(newStaffElem);
			j = newStaffElem->voiceCount();
			for (k = 0; k < j; k++) {
				oldvoiceList_->append(newStaffElem->getVoiceNr(k));
			}
		}
	}
	for (i = newStaffCount; i < oldstaffCount; i++) {
		oldStaffElem = oldstaffList_->at(i);
		if (!oldStaffElem) continue;
		oldStaffElem->updateVoiceList(oldvoiceList_);
		oldstaffList_->remove();
	}
	emit endTSE3toScore(true);
}

void NTSE3Handler::insertTimeAndKeySigs(QPtrList<NStaff> *staffs) {
	//TSE3::TimeSigTrackIterator *tmtritr;
	TSE3::PlayableIterator *tmtritr;
	//TSE3::KeySigTrackIterator *kstritr;
	TSE3::PlayableIterator *kstritr;
	NVoice *voice;
	NStaff *staff;
	NTimeSig *timesig;
	NKeySig *keysig;
	int i, j;
	double partTime, readyPart, numOfSigs;

	//kstritr = new TSE3::KeySigTrackIterator(theSong_->keySigTrack(), TSE3::Clock (0)); 
	kstritr = theSong_->keySigTrack()->iterator(TSE3::Clock (0)); 

	readyPart = 100.0 * (FIRST_PART_TIME + SECOND_PART_TIME);
	numOfSigs = (double) theSong_->keySigTrack()->size();
	partTime = numOfSigs  / (100.0 * THIRD_PART_TIME) + 1.e-20;
	for (j = 0; kstritr->more(); ++j) {
		NResource::progress_->setValue((int) (readyPart + (double) (j+1) / partTime));
		for (i = 0, staff = staffs->first(); staff; staff = staffs->next(), i++) {
			voice = staff->getVoiceNr(0);
			if (!keep_ || NResource::staffSelTrack_[trackInfoArray_[i]] && staff->getChannel() != DRUM_CHANNEL) {
				keysig = new NKeySig(voice->getMainPropsAddr(), voice->getStaff()->getStaffPropsAddr());
				if ((*kstritr)->data.data2 & 0xf) {
					keysig->setRegular(((*kstritr)->data.data2 & 0xf), PROP_FLAT);
				}
				else if ((*kstritr)->data.data2 & 0xf0) {
					keysig->setRegular(((((*kstritr)->data.data2) >> 4)) & 0xf, PROP_CROSS);
				}
				else continue;
				voice->insertAtTime(TSE3TIME2MYMIDITIME((*kstritr)->time.pulses), keysig, true);
			}
		}
		++(*kstritr);
	}
	delete kstritr;

	//tmtritr = new TSE3::TimeSigTrackIterator(theSong_->timeSigTrack(), TSE3::Clock (0)); 
	tmtritr = theSong_->timeSigTrack()->iterator(TSE3::Clock(0)); 

	readyPart = 100.0 * (FIRST_PART_TIME + SECOND_PART_TIME + THIRD_PART_TIME);
	numOfSigs = (double) theSong_->timeSigTrack()->size();
	partTime = numOfSigs / (100.0 * FOURTH_PART_TIME) + 1.e-20;
	for (j = 0; tmtritr->more(); ++j) {
		NResource::progress_->setValue((int) (readyPart + (double) (j+1) / partTime));
		for (i = 0, staff = staffs->first(); staff; staff = staffs->next(), i++) {
			voice = staff->getVoiceNr(0);
			if (!keep_ || NResource::staffSelTrack_[trackInfoArray_[i]]) {
				timesig = new NTimeSig(voice->getMainPropsAddr(), voice->getStaff()->getStaffPropsAddr());
				timesig->setSignature((0xf & ((*tmtritr)->data.data2) >> 4), (0xf & ((*tmtritr)->data.data2)));
				voice->insertAtTime(TSE3TIME2MYMIDITIME((*tmtritr)->time.pulses), timesig, true);
				voice->setHalfsAccordingKeySig(false);
			}
		}
		++(*tmtritr);
	}
	delete tmtritr;
	delete [] NResource::staffSelTrack_;
	delete trackInfoArray_;
	NResource::staffSelTrack_ = 0;
}


#ifdef OWN_SNAP
static int SNAP(int x) {
	bool triplet_ = true;
	int tripletsnap, regularsnap;
	regularsnap = ((int) ((x + SnapVal_2_) / SnapVal_)) * SnapInt_;
	if (triplet_) {
		tripletsnap = ((int) ((x + TripletSnapVal_2_) / TripletSnapVal_)) * TripletSnapInt_;
		if (abs(tripletsnap - x) < abs(regularsnap - x)) {
			snapToTriplet_ = true;
			return tripletsnap;
		}
	}
	snapToTriplet_ = false;
	return regularsnap;
}
#else
#define SNAP(x) (x)
#endif

#ifdef TRIPLET_RECOGNITION
static int findTripletPart(int l) {
	int exponent;
	int testlength;

	testlength = (0x2 << 7);
	for (exponent = 7; exponent > 0; exponent--) {
		if (testlength <= (unsigned int) l) {
			if ((abs(l - testlength) % SnapInt_) == 0 && (tripletSum_ + testlength) % SnapInt_ == 0 && testlength != l) {
				return (0x3 << exponent);
			}
		}
		testlength >>= 1;
	}
	return (3 * l / 2);
}

static int extractTripletPart(int l) {
	int exponent;
	int testlength;

	testlength = 0x2;
	for (exponent = 0; exponent < 7; ++exponent) {
		if (testlength < (unsigned int) l) {
			printf("Teste: l = %d, testlength = %d, (l - testlength) %% SnapInt_ = %d\n",
				l, testlength, (l - testlength) % SnapInt_);
			if ((l - testlength) % SnapInt_  == 0) {
				printf("gefunden\n");
				return 3 * testlength / 2;
			}
		}
		testlength <<= 1;
	}
	return l;
}
#endif


bool NTSE3Handler::TSE3TrackLimits(TSE3::PlayableIterator *tri, int *noteCount) {
	unsigned int minMidi = 1000, maxMidi = 0;
	bool first = true, firstVoice = false;
	unsigned int channel = 0, volume = 90;
	int i;

	if (!tri) {
		return false;
	}

	*noteCount = 0;
	TrackVoice_ = 0;
	

	averageVolume_ = 0.0;
	while (tri->more()) {
		switch ((*tri)->data.status) {
			case TSE3::MidiCommand_NoteOn:
				++(*noteCount);
				if ((*tri)->data.data1 < minMidi) minMidi = (*tri)->data.data1;
				if ((*tri)->data.data1 > maxMidi) maxMidi = (*tri)->data.data1;
				averageVolume_ += (*tri)->data.data2;
				if (first) {
					first = false;
					volume = (*tri)->data.data2;
					channel = (*tri)->data.channel;
				}
				break;
			case TSE3::MidiCommand_ProgramChange:
				if (firstVoice) break;
				firstVoice = true;
				TrackVoice_ = (*tri)->data.data1;
				break;
		}
		++(*tri);
	}
	delete tri;

	if ((*noteCount) == 0) {
		staffsPerTrack_ = 1;
		minMidi = maxMidi = 60;
		averageVolume_ = 64.0;
		volume = 64; channel = 0;
	}
	else {
		averageVolume_ /= *noteCount;
	}
	if ((staffsPerTrack_ = NClef::chooseClefType(staffInfo_, minMidi, maxMidi, channel == DRUM_CHANNEL)) == 0) {
		KMessageBox::error
			(0, i18n("Can't determine clef"), kapp->makeStdCaption(i18n("???")));
		return false;
	}
	for (i = 0; i < staffsPerTrack_; ++i) {
		staffInfo_[i].volume = volume;
		staffInfo_[i].channel = channel;
	}
	return true;
}


/*---------------------------- internal functions ----------------------------*/

int NTSE3Handler::threwCase( int item ) {

    int val = TSE3::Clock::PPQN;

        switch (item) {
                case  0: val = 4*TSE3::Clock::PPQN; break;
                case  1: val = 2*TSE3::Clock::PPQN / 2 * 3; break;
                case  2: val = 2*TSE3::Clock::PPQN; break;
                case  3: val = TSE3::Clock::PPQN / 2 * 3; break;
                case  4: val = TSE3::Clock::PPQN; break;
                case  5: val = TSE3::Clock::PPQN/2 / 2 * 3; break;
                case  6: val = TSE3::Clock::PPQN/2; break;
                case  7: val = TSE3::Clock::PPQN/4 / 2 * 3; break;
                case  8: val = TSE3::Clock::PPQN/4; break;
                case  9: val = TSE3::Clock::PPQN/8 / 2 * 3; break;
                case 10: val = TSE3::Clock::PPQN/8; break;
                case 11: val = TSE3::Clock::PPQN/16; break;
        }

    return val;

}


void NTSE3Handler::TSE3Rec2Staff(NStaff *staff, QPtrList<NVoice> *voice_list) {
	int noteCount;
	int n, i, nr_voices;
	NVoice *voice;
	QPtrList<NVoice> oldvoices;

	if (recNum_ == 0) {
		KMessageBox::sorry
			(0,
			 i18n("Nothing to import"),
			 kapp->makeStdCaption(i18n("Import recording"))
			);
		return;
	}
	recTrack_->filter()->setVelocityScale(filterDialog_->filVelSca->getValue());
	voice = staff->getVoiceNr(0);
	voice->emptyVoice();
	recTrack_->filter()->setMaxVelocity(filterDialog_->filVal2->getValue());
	if (filterDialog_->filVal1->getValue() <= filterDialog_->filVal2->getValue()) {
		recTrack_->filter()->setMinVelocity(filterDialog_->filVal1->getValue());
	}
	else {
		recTrack_->filter()->setMinVelocity(filterDialog_->filVal2->getValue());
	}
	if (!TSE3TrackLimits(recTrack_->iterator(TSE3::Clock (0)), &noteCount)) {
		KMessageBox::error
			(0,
			 i18n("unable to determine a clef"),
			 kapp->makeStdCaption(i18n("???"))
			);
		emit endRecorded(false);
		return;
	}
	if (!noteCount) {
		KMessageBox::sorry
			(0, i18n("No notes recorded"), kapp->makeStdCaption(i18n("???")));
		emit endRecorded(false);
		return;
	}
	if (staffsPerTrack_ != 1) {
		KMessageBox::error
			(0,
			 i18n("This cannot be imported to ONE staff"),
			 kapp->makeStdCaption(i18n("???"))
			);
		emit endRecorded(false);
		return;
	}
	nr_voices = staff->voiceCount();
	for (i = 1; i < nr_voices; i++) {
		oldvoices.append(staff->getVoiceNr(i));
	}
	TSE3Track2Staff(0, staff, recTrack_, false);
	while (voice = oldvoices.first()) {
		if (voice_list->find(voice) == -1) {
			NResource::abort("NTSE3Handler::TSE3Track2Staff");
		}
		voice_list->remove();
		oldvoices.remove();
	}
	nr_voices = staff->voiceCount();
	for (i = 1; i < nr_voices; i++) {
		voice_list->append(staff->getVoiceNr(i));
	}
	emit endRecorded(true);
}


bool NTSE3Handler::TSE3Track2Staff(int nr, NStaff *staff, TSE3::Track *tr, bool assignChannel) {
	TSE3::PlayableIterator *tri;
	NClef *clef;
	unsigned int actualVolume;

	tri = tr->iterator(TSE3::Clock (0));
	if (!tri) {
		return false;
	}

	staffTime_ = 0;
	
	staff->actualClef_ = NClef(&(mainWidget_->main_props_), staff->getStaffPropsAddr(), staffInfo_[nr].clefType);
	staff->actualClef_.setShift(staffInfo_[nr].clefShift);
	staff->actualClef_.setStaffProps(staff->getStaffPropsAddr());
	staff->actualKeysig_.setActual(false);
	staff->actualKeysig_.setStaffProps(staff->getStaffPropsAddr());
	staff->actualKeysig_.setClef(&(staff->actualClef_));
	staff->getVoiceNr(0)->appendElem(clef = new NClef(&(mainWidget_->main_props_), staff->getStaffPropsAddr()));
	clef->change(&(staff->actualClef_));
	if (assignChannel) staff->setChannel(staffInfo_[nr].channel);
	if (filterDialog_->filAvVol->isChecked()) {
		staff->setVolume(actualVolume = (int) averageVolume_);
	}
	else {
		staff->setVolume(actualVolume = staffInfo_[nr].volume);
	}
	return CreateTSE3StaffFromIterator(nr, staff, tri, actualVolume, assignChannel);
}

int NTSE3Handler::minimalNote(int nr)  {
	switch (nr) {
		case  1: return WHOLE_LENGTH;
		case  2: return HALF_LENGTH * 3 / 2;
		case  3: return HALF_LENGTH;
		case  4: return QUARTER_LENGTH * 3 / 2;
		case  5: return QUARTER_LENGTH;
		case  6: return NOTE8_LENGTH * 3 / 2;
		case  7: return NOTE8_LENGTH;
		case  8: return NOTE16_LENGTH * 3 / 2;
		case  9: return NOTE16_LENGTH;
		case 10: return NOTE32_LENGTH * 3 / 2;
		case 11: return NOTE32_LENGTH;
		case 12: return NOTE64_LENGTH;
	}
	return -1;
}

int NTSE3Handler::minimalTripletNote(int nr)  {
	switch (nr) {
		case 0: return -2;
		case 1: return -1;
		case 2: return HALF_LENGTH;
		case 3: return QUARTER_LENGTH;
		case 4: return NOTE8_LENGTH;
		case 5: return NOTE16_LENGTH;
	}
	return -1;
}
	
bool NTSE3Handler::CreateTSE3StaffFromIterator(int nr, NStaff *staff,  TSE3::PlayableIterator *tri, unsigned int actualVolume, bool assignChannel) {
	TSE3::MidiEvent m_evt;
	TSE3::PlayableIterator *tmtritr;
	NMidiTimeScale mTimeScale(minimalNote(filterDialog_->filSnapDist->currentItem()), minimalTripletNote(filterDialog_->filSmTripletNote->currentItem()));

	tmtritr = theSong_->timeSigTrack()->iterator(TSE3::Clock(0));
	while (tmtritr->more()) {
		mTimeScale.insertTimeOfTimesig(TSE3TIME2MYMIDITIME((*tmtritr)->time.pulses));
		++(*tmtritr);
	}
	delete tmtritr;
	for (;tri->more(); ++(*tri)) {
		m_evt = **tri;
		mTimeScale.insertMidiEvent(&m_evt, staffInfo_[nr].minMidi, staffInfo_[nr].maxMidi);
	}
	delete tri;
	mTimeScale.createStaff(staff, staffInfo_[nr].channel == DRUM_CHANNEL, filterDialog_->filVolDist->getValue(), filterDialog_->filAvVol->isChecked(), actualVolume, averageVolume_, filterDialog_->filVal1->getValue());
	if (assignChannel) staff->changeVoice(mTimeScale.getMidiProgram());
	return true;
}
#endif
#include "tse3handler.moc"
#endif /* WITH_TSE3 */
