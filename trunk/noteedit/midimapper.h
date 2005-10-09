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

#ifndef MIDIMAPPER_H
#define MIDIMAPPER_H

#include <qwidget.h>
#include <qlistbox.h>
#include <qpaintdevice.h>
#include "config.h"
#define ALSA_SCHEDULER_REQUESTED (1 << 0)
#define OSS_SCHEDULER_REQUESTED (1 << 1)
#define ARTS_SCHEDULER_REQUESTED (1 << 2)
#define ALL_SCHEDULERS (ALSA_SCHEDULER_REQUESTED | OSS_SCHEDULER_REQUESTED)
#ifdef WITH_TSE3
#include <qdatetime.h> 
#include <tse3/MidiScheduler.h>
#include <tse3/Midi.h>
#include <tse3/plt/Alsa.h>
#include <tse3/plt/OSS.h>
#ifdef TSE3_HAS_ARTS
#include <tse3/plt/Arts.h>
#endif
#include "tse3/Error.h"
#if TSE3_MID_VERSION_NR > 1
#include <tse3/MidiScheduler.h>
#include <tse3/plt/Factory.h>
#endif
#elif !defined(WITH_LIBKMID)
#include "linux/soundcard.h"
#endif
#include "muselement.h"

#include "uiconnect.h"

#define MIDI_DEV 1
#define SYNTH_DEV 2

#define DEFAULT_TEMPO 100
#define TICKS_PER_QUARTER (3*128)

class NVoice;
class NClef;
class NDbufferWidget;
class listFrm;

#ifdef WITH_LIBKMID
class DeviceManager;
#endif


class NMidiEventStr {
	public:
/*
		NMidiEventStr *clone(int playTime);
*/
		int midi_cmd;
#define MNOTE_ON       1
#define MNOTE_OFF      2
#define MTIMESIG       3
#define MVOL_CONTROL   4
		int ev_time;
		int length;
		unsigned int special;
#define NO_SPECIAL 0
#define SPEC_TRILL_DOWN (1 << 0)
#define SPEC_TRILL_UP   (1 << 1)
#define SPEC_DYNAMIC	(1 << 2)
#define SPEC_DYNAMIC_START  (1 << 3)
#define SPEC_ARPEGGIO   (1 << 4)
#define TRILL_SPECS (SPEC_TRILL_DOWN | SPEC_TRILL_UP)
		int trillDist;
		int partlength;
		bool valid;
		int internalMidiTime;
		unsigned char volume;
		unsigned char midi_channel;
		char trilloffs;
		unsigned char status;
#define MIDI_STAT_PEDAL_ON	(1 << 0)
#define MIDI_STAT_PEDAL_OFF	(1 << 1)
		char arpegg_current, arpegg_total;
		char midi_prog_change;
		char volum_ctrl_change;
		int xpos;
		NMidiEventStr *next, *notehalt;
		NVoice *from;
		NMusElement *ref;
		QPtrList<NNote> *notelist;
};

struct dir_notes_str {
	int pitch, chan;
};


class NMidiMapper : public QObject{
	Q_OBJECT
	public:
		NMidiMapper();
		~NMidiMapper();
		QStringList deviceNameList;
		void playImmediately(NClef *clef, NChord *chord, int pgm, int chn, int vol, int transpose);
		void playImmediately(NClef *clef, int keyline, int offs, int pgm, int chn, int vol, int transpose);
#ifdef WITH_TSE3
		NMidiMapper(TSE3::MidiScheduler *scheduler);
		void setEchoChannel(int chn, int prog);
		QPtrList<int> *readEvents();
#elif defined (WITH_LIBKMID)
		bool isSynth(unsigned int port) const {return port >= midiDevs_;}
#else
		bool isSynth(unsigned int port) const {return port < synthDevs_;}
#endif
		void play_list(QPtrList<NMidiEventStr> *play_list, int playTime);
		void stopAllNotes(QPtrList<NMidiEventStr> *play_list);
		void changeProg(int chn, int pgm);
		void changeReverb(int chn, int rev);
		void changeChorus(int chn, int chor);
		void changePan(int chn, int pan);
		void setPaintDevice(NDbufferWidget *pd);
		void openDevice();
		int getActualDeviceNumber() {return actualDevice_nr_ < 0 ? 0 : actualDevice_nr_;}
		bool isInUse_;
	public slots:
		void changeDevice(unsigned int);
		void stopImmediateNotes();
	private:
		NDbufferWidget *pd_;
		int actualChannel_;
		int synthDevs_, midiDevs_;
		int actualDevice_nr_;
		unsigned int channelPool_;
		int lastSelectedChannel_;
		QPtrList<dir_notes_str> immList_;
#ifdef WITH_LIBKMID

		DeviceManager *devMan_;
		bool midiOk_;
#elif defined (WITH_TSE3)
#if TSE3_MID_VERSION_NR < 2
		TSE3::Plt::AlsaMidiSchedulerFactory theAlsaFactory_;
		TSE3::Plt::OSSMidiSchedulerFactory theOSSFactory_;
#ifdef TSE3_HAS_ARTS
		TSE3::Plt::ArtsMidiSchedulerFactory theARtsFactory_;
#endif
#else
		TSE3::MidiSchedulerFactory theFactory_;
#endif
	public:
		TSE3::MidiScheduler           *theScheduler_;
	private:
		int echoChannel_;
		QTime readTime_;
#else // DIRECT ACCESS VIA "soundcard.h"
	private:
		class devinfo {
			public:
				int dev_type;
				union {
					struct midi_info midi_inf;
					struct synth_info synth_inf;
				} devdata;
		} *devices_, *actualDevice_;
		void playMidiDev(QPtrList<NMidiEventStr> *play_list, int playTime) const;
		void playSynthDev(QPtrList<NMidiEventStr> *play_list, int playTime) const;
		void playNull(QPtrList<NMidiEventStr> *play_list, int playTime) const;
		void (NMidiMapper::*play_func_)(QPtrList<NMidiEventStr> *play_list, int playTime) const;
#endif
};


#endif // MIDIMAPPER_H
