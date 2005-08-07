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

#ifndef ABCEXPORT_H

#define ABCEXPORT_H

#include <qlist.h>
#include <qstring.h>
#include "config.h"
#if GCC_MAJ_VERS > 2
#include <sstream>
#include <fstream>
#else
#include <strstream.h>
#include <fstream.h>
#endif

using namespace std;

class NKeySig;
class NStaff;
class NVoice;
class NClef;
class NText;
class chordDiagramName;


class NABCExport  {
	public:
		NABCExport();
		void exportStaffs(QString fname, QList<NStaff> *stafflist, int count_of_voices, exportFrm *expWin, NMainFrameWidget *mainWidget);
	private:
		bool writeFirstVoice(NVoice *voice_elem, QString staffName, int staff_nr, int voice_count, int measure_count, bool lastStaff);
		bool writeOtherVoicesTill(int staff_nr, int voice_nr, QString staffName, NVoice *voice_elem, NStaff *staff_elem, int stopTime);
		void outputNote(NNote *note, NClef *actualClef, bool inInChord);
		void outputLength(int len, status_type status, bool inChord, bool drumNote);
		void outputKeySig(NKeySig *key, bool inHeader);
		void outputMidi(QList<NStaff> *stafflist);
		void outputStaffAndVoiceDescription(QList<NStaff> *stafflist, NMainFrameWidget *mainWidget);
		void outputVoiceParams(NVoice *voice, QString staffName);
		QString lyrics2ABC(QString *lyrics);
		void outputMeter(NTimeSig *timesig, bool inHeader);
		void outputBarSym(NSign *sign, int volta, bool isLast);
		void outputTupletStart(int staff_nr, NMusElement *elem);
		bool outputClefInfo(NClef *clef);
		void outputPedalGlyphs();
		void outputDrumDefinitions();
		void writeChord(NChordDiagram *diag);
		void outputGrid(chordDiagramName *diagNam);
		void outputGuitarPostscript();
		void handleSpecialElements(NStaff *staff_elem, NMusElement *elem);
		void writePendingSigns(int idx);
		void appendContextChangeList(NVoice *voice_elem);
		NClef *lastClef_;
		QList<chordDiagramName> chordDiagramList_;
#if GCC_MAJ_VERS > 2
		ostringstream *lyricsLine_[NUM_LYRICS];
#else
		ostrstream *lyricsLine_[NUM_LYRICS];
#endif
		int *countOfLyricsLines_;
		QString createVoiceName(QString staffName,  int staff_nr, int voice_nr);
		ofstream out_;
		class badinfo {
			public:
				badinfo(int type, int staffnr, int barnr) {
					type_ = type;
					staffnr_ = staffnr;
					barnr_ = barnr;
				}
				int type_, staffnr_, barnr_;
		};
		QList<badinfo> badlist_;
		class voice_stat_str {
			public:
				voice_stat_str() {
					slurDepth = 0;
					pendingVolumes = 0;
					pendingSegnos = 0;
					pendingSegnos2 = 0;
					pendingRitAccel = 0;
					pendingText = 0;
					lastBarSym = 0;
					trillendpos = 0;
					dynEndPos = 0;
					lastDynSym = 0;
				}
				int slurDepth;
				int trillendpos;
				int dynEndPos;
				char *lastDynSym;
				NSign *lastBarSym;
				NSign *pendingVolumes;
				NSign *pendingSegnos;
				NSign *pendingSegnos2;
				NSign *pendingRitAccel;
				NText *pendingText;
		};
		voice_stat_str *voiceStatList_;
		int staffCount_;
		int lastMeasureNum_;
#if GCC_MAJ_VERS > 2
		ostringstream *os_;
#else
		char buffer_[128];
		ostrstream *os_;
#endif
};

#endif /* ABCEXPORT_H */

