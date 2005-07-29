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

#ifndef FILE_HANDLER_H

#define FILE_HANDLER_H
#include "config.h"
#if GCC_MAJ_VERS > 2
#include <sstream>
#include <fstream>
#else
#include <strstream.h>
#include <fstream.h>
#endif
#include <qlist.h>
#if QT_VERSION >= 300
#include <qregexp.h>
#endif
#include "muselement.h"

#define LINE_LENGTH 1024

using namespace std;

class NClef;
class NSign;
class NKeySig;
class NVoice;
class NStaff;
class NChord;
class NMainFrameWidget;
class mupWrn;
class mupFatl;
class chordDiagramName;

#define ERR_NOTE_COUNT	      1
#define ERR_LYRICS_COUNT      2
#define ERR_SLUR	      3
#define ERR_NOTE_128          4
#define ERR_IRREGULAER        5
#define ERR_NOTE_MIXED_TIES   6
#define ERR_LONELY_TRILL      7
#define ERR_BARTYPES	      8
#define ERR_TOO_MANY_VOICES   9
#define ERR_VAR_TRILLS	     10
#define ERR_DRUM_STAFF	     11
#define ERR_MULTIREST	     12
#define ERR_SLURES_IN_GRACES 13
#define ERR_BEAMS_IN_GRACES  14
#define ERR_GRACE_AFTER      15
#define ERR_CLEFCHANGE	     16
#define ERR_KEYCHANGE	     17
#define ERR_MIXED_GRIDS	     18
#define ERR_TUPLET	     19
#define ERR_SEGNOS_AT_END    20
#define ERR_PEDAL_IN_2ND     21
#define ERR_TO_MANY_BRACKETS 22
#define ERR_NESTED_BRACKETS  23
#define ERR_GRACE_SLUR       24


class badmeasure {
	public:
		badmeasure(int k, int t, int m, int r, int s) {
			kind = k; track = t; measure = m; realcount = r; shouldbe = s;
		}
		int kind;
		int track, measure;
		int realcount, shouldbe;
};


class pending_sign_information_class {
    public:
	pending_sign_information_class(double st_time, NSign *sn) {
		staff_time = st_time; pending_sign = sn;
	}	
	double staff_time;
	NSign *pending_sign;
};
class NFileHandler {
	public:
		NFileHandler();
		bool readStaffs(const char *fname, QList<NVoice> *voilist, QList<NStaff> *stafflist, NMainFrameWidget *mainWidget);
		bool writeStaffs(QString fname, QList<NStaff> *stafflist, NMainFrameWidget *mainWidget, bool showErrors);
	private:
		void pitchOut( const NNote *note, NClef *ac_clef, bool with_tie);
		int writeStaffUntilBar(int staff_nr, NVoice *voi, bool first, int multirestlength, int *measure_start_time);
		void writeVoiceElemsTill(int staff_nr, int voice_nr, NVoice *voi, int stopTime, int multirestlength, int measure_start_time);
		void writeScoreInfo(int staff_nr, NVoice *voi, bool firstcall, NMainFrameWidget *mainWidget);
		void writeTempoSig(NSign *temposig);
		void writeTempoSig(double starttime, NSign *temposig);
		void writeVolSig(double starttime, int staff_nr, NSign *volsig);
		void writeProgramChange(double starttime, int staff_nr, NSign *pChange);
		bool writeClef(NClef * clef, int voice_nr);
		void writeKeySig(NKeySig * ksig, int voice_nr, bool only_regulaer = false);
		void writeChord(int staff_nr, double starttime, NChordDiagram *diag);
		void writeStaffLayout(NMainFrameWidget *mainWidget, int staffCount);
		QString computeTripletString(int tupletsum, char numNotes, char playtime, bool *ok);
		int determineMultiRest(QList<NStaff> *stafflist);
		bool divide_multi_rest(int staff_nr, int voice_nr, int multirestlength);
		QString lyrics2MUP(QString *lyrics);
		ofstream out_;
		int repaetTime_;
		char s_[LINE_LENGTH];
		int **pending_multi_rests_;
		int lyricsLineCount_;
		QList<pending_sign_information_class> pending_program_changes_, pending_volume_sigs_, pending_tempo_sigs_;
		QList<chordDiagramName> chordDiagramList_;
#if GCC_MAJ_VERS > 2
		ostringstream *os_;
		ostringstream *ornaments_;
		ostringstream *dynamics_;
		ostringstream *chordline_;
		ostringstream *wavelines_;
		ostringstream *phrases_;
		ostringstream *trillsyms_;
		ostringstream *valines_;
		ostringstream *lyricsLine_[NUM_LYRICS];
#else
		char buffer_[128], trillsymsbuffer_[128], ornamentbuffer_[128], dynamicsbuffer_[128], chordsbuffer_[128], phrasebuffer_[128];
		char valinebuffer_[128];
		ostrstream *os_;
		ostrstream *ornaments_;
		ostrstream *trillsyms_;
		ostrstream *valines_;
		ostrstream *dynamics_;
		ostrstream *chordline_;
		ostrstream *phrases_;
		ostrstream *lyricsLine_[NUM_LYRICS];
#endif
		QString rolls_;
		QString pedals_;
		QString signs_;
		QString textsigns_above_;
		QString textsigns_below_;
		int lnr_;
		ifstream in_;
		int staffCount_;
		bool musicmode_;
		bool music_written_;
		bool hasClef_;
		int bar_nr_;
		int countof128th_;
		int curr_num_, curr_denom_;
		bool drum_problem_written_;
		bool some_notes_or_rests_written_;
		mupWrn* mupWarn_;
		mupFatl* mupFatl_;
		QList<badmeasure> badlist_;
		QList<badmeasure> fatallist_;
		QRegExp newLines_;
		
};

#endif // FILE_HANDLER_H
