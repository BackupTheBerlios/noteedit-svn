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
#include <sstream>
#else
#include <strstream.h>
#endif
#include <qlist.h>
#include <kmessagebox.h>
#include <klocale.h>
#include "midiexport.h"
#include "resource.h"
#include "keysig.h"
#include "timesig.h"
#include "voice.h"
#include "staff.h"
#include "midimapper.h"
#include "chord.h"
#include "tempotrack.h"


using namespace std;

#define META_TEXT    0x01
#define META_TIMESIG 0x58
#define META_KEYSIG  0x59
#define META_TEMPO   0x51

#define MIDI_CTL_REVERB  0x5b
#define MIDI_CTL_CHORUS  0x5d
#define MIDI_CTL_PAN     0x0a
#define MIDI_CTL_VOLUME  0x07
#define MIDI_CTL_SUSTAIN 0x40

#define MY2MIDITIME(t) ((unsigned int) ((((double) t) * (double) (TICKS_PER_QUARTER)) / ((double) (QUARTER_LENGTH))))


static unsigned char trackend[] = {0x00, 0xff , 0x2f, 0x00};

void NMidiExport::writeByte(unsigned char b) {
	putc(0xff & b, midiout_);
}

void NMidiExport::writeWord(unsigned int w) {
	putc((0xff & (w >> 8)), midiout_);
	putc(0xff & w, midiout_);
}

void NMidiExport::writeDWord(unsigned int dw) {
	putc((0xff & (dw >> 24)), midiout_);
	putc((0xff & (dw >> 16)), midiout_);
	putc((0xff & (dw >> 8)), midiout_);
	putc(0xff & dw, midiout_);
}

void NMidiExport::writeString(char *s) {
	while (*s) {
		putc(0xff & (*s++), midiout_);
	}
}

void NMidiExport::writeTime(int time) {
	unsigned char b;
	bool byteswritten = false;

	b = (time >> 3*7) & 0x7f;
	if (b) {
		writeByte(0x80 | b);
		byteswritten = true;
	}
	b = (time >> 2*7) & 0x7f;
	if (b || byteswritten) {
		writeByte(0x80 | b);
		byteswritten = true;
	}
	b = (time >> 7) & 0x7f;
	if (b || byteswritten) {
		writeByte(0x80 | b);
		byteswritten = true;
	}
	b = time & 0x7f;
	writeByte(b);
}

void NMidiExport::writeNoteOn(int time, unsigned char ch, unsigned char ptch, unsigned char vel) {
	writeTime(time);
	writeByte(0x90 | ch);
	writeByte(ptch);
	writeByte(vel);
}

void NMidiExport::writeNoteOff(int time, unsigned char ch, unsigned char ptch, unsigned char vel) {
	writeTime(time);
	writeByte(0x80 | ch);
	writeByte(ptch);
	writeByte(vel);
}

void NMidiExport::writePgmChange(int time, unsigned char chn, unsigned char pgm) {
	writeTime(time);
	writeByte(0xc0 | chn);
	writeByte(pgm);
}


void NMidiExport::writeCtlChange(int time, unsigned char chn, unsigned char ctl, unsigned char pgm) {
	writeTime(time);
	writeByte(0xb0 | chn);
	writeByte(ctl);
	writeByte(pgm);
}

void NMidiExport::exportMidi(const char *fname, QList<NVoice> *voilist, char *miditext) {
	NVoice *voice_elem;
	NTimeSig *timesig;
#if GCC_MAJ_VERS > 2
	ostringstream os;
#else
	char buffer[128+1];
	ostrstream os(buffer, 128);
#endif
	if ((midiout_ = fopen(fname, "wb")) == NULL) {
		os << "error opening file " << fname << '\0';
#if GCC_MAJ_VERS > 2
		KMessageBox::error
			(0, QString(os.str().c_str()), kapp->makeStdCaption(i18n("???")));
#else
		KMessageBox::error
		(0, QString(os.str()), kapp->makeStdCaption(i18n("???")));
#endif
		return ;
	}

	
	writeString("MThd");
	writeDWord(6);
	writeWord(1);
	writeWord(voilist->count() + 1);
	writeWord(TICKS_PER_QUARTER);
	voice_elem = voilist->first();
	timesig = voice_elem->getFirstTimeSig();
	writeCtrlTrack(voilist, "Music generated by \"NoteEdit\"", miditext, timesig, 0/* , 60000000 / tempo*/);

	for (voice_elem = voilist->first(); voice_elem; voice_elem = voilist->next()) {
		writeTrack(voice_elem, timesig);
	}
	fclose(midiout_);
}

void NMidiExport::writeText(int time, const char *s) {
	const char *cptr = s;
	writeTime(time);
	writeByte(0xff); writeByte(META_TEXT); writeByte(strlen(s));
	while (*cptr) {
		putc(0xff & (*cptr++), midiout_);
	}
}

void NMidiExport::writeTimeSig(int time, int num, int denom) {
	writeTime(time);
	writeByte(0xff); writeByte(META_TIMESIG); writeByte(0x04);
	writeByte(num);
	if (denom <= 4) {
		writeByte(2);
	}
	else if (denom <= 8) {
		writeByte(3);
	}
	else if (denom <= 16) {
		writeByte(4);
	}
	else if (denom <= 32) {
		writeByte(5);
	}
	else if (denom <= 64) {
		writeByte(6);
	}
	else {
		writeByte(7);
	}
	writeByte(0x1); writeByte(0x8);
}

void NMidiExport::writeKeySig(int time, int sig) {
	writeTime(time);
	writeByte(0xff); writeByte(META_KEYSIG);  writeByte(0x02);
	if (sig < 0) {
		writeByte(0x00); writeByte(-sig);
	}
	else {
		writeByte(sig); writeByte(0x00);
	}
}

void NMidiExport::writeTempo(int time, unsigned int tempo) {
	writeTime(time);
	writeByte(0xff); writeByte(META_TEMPO); writeByte(0x03);
	writeByte(0xff & (tempo >> 16));
	writeByte(0xff & (tempo >> 8));
	writeByte(0xff & tempo);
}


void NMidiExport::writeCtrlTrack(QList<NVoice> *voilist, char *Title, char *miditext, NTimeSig *firstTsig, int keysig) {
	long pos0, pos1;
	unsigned int length;
	int lastEventTime = 0;
	NVoice *voice_elem;
	NTempoTrack SortedTempoSigs;
	NSign *sign;
	for (voice_elem = voilist->first(); voice_elem; voice_elem = voilist->next()) {
		voice_elem->getTempoSigs(&SortedTempoSigs, 0);
	}
	SortedTempoSigs.resolveRitardandoAndAccelerando();
	writeString("MTrk");
	pos0 = ftell(midiout_);
	writeDWord(0);
	writeText(0, Title);
	if (miditext != 0) {
		writeText(0, miditext);
	}
	if (firstTsig) {
		writeTimeSig(0, firstTsig->getNumerator(), firstTsig->getDenominator());
	}
	else {
		writeTimeSig(0, 4, 4);
	}
	writeKeySig(0, keysig);
	writeTempo(0, 60000000 / DEFAULT_TEMPO);
	for (sign = SortedTempoSigs.first(); sign; sign = SortedTempoSigs.next()) {
		writeTempo(MY2MIDITIME(sign->getRealMidiTime()) - lastEventTime, 60000000 / sign->getTempo());
		lastEventTime = MY2MIDITIME(sign->getRealMidiTime());
	}
	SortedTempoSigs.clear();
	fwrite(trackend, sizeof(trackend), 1, midiout_);
	length = (pos1 = ftell(midiout_)) - pos0 - 4;
	fseek(midiout_, pos0, SEEK_SET);
	writeDWord(length);
	fseek(midiout_, pos1, SEEK_SET);
}

void NMidiExport::writeTrack(NVoice *voice, NTimeSig *firstTsig) {
	long pos0, pos1;
	int lyricsLines;
	unsigned int length;
	int i;
	NNote *note;
	NVoice *firstVoice;
	NMidiEventStr *m_evt, *m_evt2, *m_events;
	int myTime, lastEventTime = 0, noteoff;
	int sign = 0;
	status_type kind;
	NTimeSig *timesig;
	QString *lyrics;
	QList<NMidiEventStr> stopList;
	stopList.setAutoDelete(false);

	timesig = voice->getFirstTimeSig();

	voice->getStaff()->startPlaying();  /* necessary to reset some member variables in all staffs (EndIdx_) */
	if (voice->getStaff()->actualKeysig_.isRegular(&kind, &sign)) {
		if (kind == STAT_FLAT) {
			sign = -sign;
		}
	}
	myTime = 0;
	
	writeString("MTrk");
	pos0 = ftell(midiout_);
	writeDWord(0);
	if (timesig) {
		writeTimeSig(0, timesig->getNumerator(), timesig->getDenominator());
	}
	else {
		writeTimeSig(0, 4, 4);
	}
	writeKeySig(0, sign);
	writePgmChange(0, voice->getStaff()->getChannel(), voice->getStaff()->getVoice());
	writeCtlChange(0, voice->getStaff()->getChannel(), MIDI_CTL_REVERB, voice->getStaff()->reverb_);
	writeCtlChange(0, voice->getStaff()->getChannel(), MIDI_CTL_CHORUS, voice->getStaff()->chorus_);
	writeCtlChange(0, voice->getStaff()->getChannel(), MIDI_CTL_PAN, voice->getStaff()->pan_);

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
		if (!m_evt && !voice->isFirstVoice()) { // re-activate 2nd voice by jump (left shift) caused by first voice repeat
			do {
				m_evt2 = firstVoice->getNextMidiEvent(myTime, false);
				if (m_evt2) {
					firstVoice->skipAndInvalidate();
				}
				m_evt = voice->getNextMidiEvent(myTime, true);
			}
			while (m_evt2 && !m_evt);
		}
		if (m_evt) {
			m_events = stopList.first();
			while (m_events) {
				if (m_events->ev_time <= m_evt->ev_time) {
					noteoff = MY2MIDITIME(m_events->ev_time) - 3;
					for (note = m_events->notelist->first(); note; note = m_events->notelist->next()) {
						if (!(note->status & STAT_TIED) || (m_evt->special & TRILL_SPECS)) {
							writeNoteOff(noteoff - lastEventTime, m_events->midi_channel, note->midiPitch+m_events->trilloffs, m_events->volume);
							lastEventTime = noteoff;
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
				if (m_evt->midi_prog_change >= 0) {
					writePgmChange(MY2MIDITIME(m_evt->ev_time) - lastEventTime, voice->getStaff()->getChannel(), m_evt->midi_prog_change);
					lastEventTime = MY2MIDITIME(m_evt->ev_time);
				}
				if (m_evt->volum_ctrl_change >= 0) {
					writeCtlChange(MY2MIDITIME(m_evt->ev_time) - lastEventTime,
						voice->getStaff()->getChannel(), 
						MIDI_CTL_VOLUME, m_evt->volum_ctrl_change);
					lastEventTime = MY2MIDITIME(m_evt->ev_time);
				}
				if (NResource::useMidiPedal_) {
					if (m_evt->status & MIDI_STAT_PEDAL_ON) {
						writeCtlChange(MY2MIDITIME(m_evt->ev_time) - lastEventTime,
							voice->getStaff()->getChannel(), 
							MIDI_CTL_SUSTAIN, 100);
						lastEventTime = MY2MIDITIME(m_evt->ev_time);
					}
					if (m_evt->status & MIDI_STAT_PEDAL_OFF) {
						writeCtlChange(MY2MIDITIME(m_evt->ev_time) - lastEventTime,
							voice->getStaff()->getChannel(), 
							MIDI_CTL_SUSTAIN, 1);
						lastEventTime = MY2MIDITIME(m_evt->ev_time);
					}
				}
				for (i = 0, note = m_evt->notelist->first(); note; note = m_evt->notelist->next(), i++) {
			   		if ((note->status & STAT_PART_OF_TIE) && !(m_evt->special & TRILL_SPECS)) {
						note->midiPitch = note->tie_backward->midiPitch; /* for note off */
					}
			   		else if (m_evt->special != SPEC_ARPEGGIO || i == m_evt->arpegg_current) {
						writeNoteOn(MY2MIDITIME(m_evt->ev_time) - lastEventTime, m_evt->midi_channel,
							note->midiPitch+m_evt->trilloffs, m_evt->volume);
						lastEventTime = MY2MIDITIME(m_evt->ev_time);
					}
				}
				lyricsLines = ((NChord *) m_evt->ref)->countOfLyricsLines();
				if (lyricsLines) {
					lyrics = ((NChord *) m_evt->ref)->getLyrics((lyricsLines > 1 && m_evt->from->inRepeat()) ? 1 : 0);
					if (lyrics) {
						writeText(0, lyrics->ascii());
					}
				}
				if (m_evt->notehalt->valid) {
					m_evt->notehalt->ev_time = m_evt->ev_time + m_evt->length;
					stopList.append(m_evt->notehalt);
				}
				myTime = m_evt->ev_time + 1;
				voice->skipChord();
				break;
			case MTIMESIG:
				timesig = (NTimeSig *) m_evt->ref;
				writeTimeSig(MY2MIDITIME(m_evt->ev_time) - lastEventTime, timesig->getNumerator(), timesig->getDenominator());
				lastEventTime = MY2MIDITIME(m_evt->ev_time);
				break;
			case MVOL_CONTROL:
				writeCtlChange(MY2MIDITIME(m_evt->ev_time) - lastEventTime,
					voice->getStaff()->getChannel(), 
					MIDI_CTL_VOLUME, m_evt->volume);
				if (m_evt->notehalt->valid) {
					m_evt->notehalt->ev_time = m_evt->ev_time + m_evt->length;
					stopList.append(m_evt->notehalt);
				}
				lastEventTime = MY2MIDITIME(m_evt->ev_time);
				voice->skipChord();
				break;
			}
		}
	}
	while (m_evt);
	m_events = stopList.first();
	while (m_events) {
		noteoff = MY2MIDITIME(m_events->ev_time) - 3;
		for (note = m_events->notelist->first(); note; note = m_events->notelist->next()) {
			if (!(note->status & STAT_TIED) || (m_events->special & TRILL_SPECS)) {
				writeNoteOff(noteoff - lastEventTime, m_events->midi_channel, note->midiPitch, m_events->volume);
				lastEventTime = noteoff;
			}
		}
		stopList.remove();
		m_events = stopList.current();
	}
	voice->stopPlaying();

	fwrite(trackend, sizeof(trackend), 1, midiout_);
	length = (pos1 = ftell(midiout_)) - pos0 - 4;
	fseek(midiout_, pos0, SEEK_SET);
	writeDWord(length);
	fseek(midiout_, pos1, SEEK_SET);
}
