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

#ifndef TSE3HANDLER_H

#define TSE3HANDLER_H
#ifndef WITH_SCONS
#include "config.h"
#endif
#ifdef WITH_TSE3
#include <qobject.h>
#include <tse3/MidiScheduler.h>
#include <tse3/Song.h>
#include <tse3/Track.h>
#include <tse3/Phrase.h>
#include <tse3/PhraseEdit.h>
#include <tse3/Metronome.h>
#include <tse3/Part.h>
#include <tse3/Midi.h>
#include <tse3/Transport.h>
#include <tse3/plt/OSS.h>
#include <tse3/MidiFile.h>
#include <tse3/util/PowerQuantise.h>
#include <qptrlist.h>
#include <qtimer.h>
#include "clef.h"

class NMidiMapper;
class NMainFrameWidget;
class NStaff;
class filterFrm;
class metronomFrm;
class NVoice;
class staffFrm;
class tse3InfoFrm;
class QListViewItem;

class NTSE3Handler : public QObject {
	Q_OBJECT
	public:
		NTSE3Handler(NMainFrameWidget *mainWidget);
		~NTSE3Handler();
		void createTSE3(QPtrList<NVoice> *voices);
		void playSong();
		bool writeTSE3(const char *fname);
		bool readTSE3(const char *fname);
		bool TSE3MidiOut(const char *fname);
		bool TSE3MidiIn(const char *fname);
		bool TSE3record(int channel, QPtrList<NVoice> *voices);
		void TSE3toScore(QPtrList<NStaff> *staffs, QPtrList<NVoice> *voices, bool keep);
		void TSE3PhraseEditToStaff(TSE3::PhraseEdit *phraseEdit, NStaff *staff);
		void TSE3Rec2Staff(NStaff *staff, QPtrList<NVoice> *voice_list);
		void insertTimeAndKeySigs(QPtrList<NStaff> *staffs);
		void initFiltering();
		void doRecord();
		static int threwCase( int item );
	signals:
		void endRecorded(bool);
		void endTSE3toScore(bool);
	public slots:
		void TSE3Merge();
		void printSongInformation();
		void TSE3recordNext();
	private:
		bool CreateTSE3StaffFromIterator(int nr, NStaff *staff, TSE3::PlayableIterator *tri, unsigned int actualVolume, bool assignChannel);
		bool TSE3Track2Staff(int nr, NStaff *staff, TSE3::Track *tr, bool assignChannel = true);
		staffFrm *multiStaff_;//*MergeSelect_, *TrackSelect_;
		filterFrm *filterDialog_;
		int staffTime_;
		int recNum_;
		void createChordFromMidiEventList(QPtrList<TSE3::MidiEvent> *eventList, NStaff *staff, NVoice *voice, int *midiOnTime, int *minMidiOffTime);
		bool TSE3TrackLimits(TSE3::PlayableIterator *tri, int *noteCount);
		int minimalNote(int nr);
		int minimalTripletNote(int nr);
		TSE3::Track *createTSE3Track(NVoice *voice, int nr, TSE3::Song *song);
		TSE3::Track *recTrack_;
		TSE3::Phrase *recPhrase_;
		TSE3::Song *theSong_;
		TSE3::PhraseEdit *recPhraseEdit_;
		TSE3::Transport *recTransport_;
		TSE3::Metronome recMetronome_;
		int staffsPerTrack_;
		staffInfoClass staffInfo_[4];
		int *trackInfoArray_;
		QTimer recordtimer_;
		NMainFrameWidget *mainWidget_;
		metronomFrm *metronomeDialog_;
		int TrackVoice_;
		tse3InfoFrm *info_;
		QListViewItem **items_;
		int targetChannel_;
		int numCounter_;
		bool keep_;
		double averageVolume_;
		QPtrList<NVoice> *oldvoiceList_;
		QPtrList<NStaff> *oldstaffList_;
};

#endif

#endif /* WITH_TSE3 */
