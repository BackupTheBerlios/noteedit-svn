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
/****************************************************************************************/
/*											*/
/*		Leon Vinken, The Netherlands						*/
/*		leon.vinken@hetnet.nl							*/
/*											*/
/****************************************************************************************/

#ifndef MUSICXMLEXPORT_H

#define MUSICXMLEXPORT_H

#ifndef WITH_SCONS
#include "config.h"
#endif

#include <qptrlist.h>
#include <qstring.h>
#include "muselement.h" /* needed for property_type */
#if GCC_MAJ_VERS > 2
#include <sstream>
#include <fstream>
#else
#include <strstream.h>
#include <fstream.h>
#endif
#include "resource.h"

using namespace std;

struct musicxml_options
{
};

class NKeySig;
class NStaff;
class NVoice;
class NChord;
class NClef;
class exportFrm;
class NMainFrameWidget;
class NNote;
class NTimeSig;
class NSign;
class NMusElem;
class NRest;
class NChordDiagram;

class NMusicXMLExport  {
	public:
		NMusicXMLExport();
		void exportStaffs(QString fname, QPtrList<NStaff> *stafflist, int count_of_voices, exportFrm *expWin, NMainFrameWidget *mainWidget);
	private:
		void debugDumpElem(NMusElement * elem);
		void debugDumpVoice(NVoice * voice_elem);
		void debugDumpStaff(NStaff * staff_elem);
		void debugDump(QPtrList<NStaff> *stafflist, NMainFrameWidget *mainWidget);
		bool writeFirstVoice(NVoice *voice_elem, int staff_nr);
		bool writeOtherVoicesTill(int staff_nr, int voice_nr, NVoice *voice_elem, int stopTime);
		void outputNote(NNote *note, NVoice *voice_elem, NClef *actualClef, int va, int staff_nr, int voice_nr, int note_nr);
		void outputVoiceNr(int voice_nr);
		int calcDuration(int len, property_type status);
		void calcLength(NMusElement *elem, int& dur, QString& type);
		void calcDivisions(QPtrList<NStaff> *stafflist);
		NChord * findDynEndChord(NStaff * staff_elem, NChord * start_chord);
		NChord * findVaEndChord(NStaff * staff_elem, NChord * start_chord);
		void outputKeySig(NKeySig *key);
		void outputStaffAndVoiceDescription(QPtrList<NStaff> *stafflist);
		void outputMeter(NTimeSig *timesig);
		void outputClefInfo(NClef *clef, int staff_nr);
		void outputDegree(int val, int alt, QString typ);
		void outputDiagram(NChordDiagram *diag);
		void outputDirection(QString direction, QString placement);
		void outputDots(NMusElement *elem);
		void outputFrame(NChordDiagram *diag);
		void outputTimeMod(NMusElement *elem);
		void writePendingSigns(int staff_nr);
		void writePendingSignsAtEnd();
		NClef *lastClef_;
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
		QPtrList<badinfo> badlist_;
		class voice_stat_str {
			public:
				voice_stat_str() {
					slurDepth = 0;
					pendingVolumes = 0;
					pendingSegnos = 0;
					pendingSegnos2 = 0;
					pendingRitAccel = 0;
					lastBarSym = 0;
					trillendpos = 0;
					vaEndPos = 0;
					dynEndPos = 0;
					lastDynSym = 0;
					pendingClef = 0;
					pendingTimeSig = 0;
					pendingKeySig = 0;
					pendingBarSym = 0;
					pendingBarSymAtEnd = 0;
					pendingEnding = 0;
					pendingEndingAtEnd = 0;
					pendingTempo = 0;
					pendingMultiRest = 0;
				}
				int slurDepth;
				int trillendpos;
				int vaEndPos;
				int dynEndPos;
				char *lastDynSym;
				NSign *lastBarSym;
				NSign *pendingVolumes;
				NSign *pendingSegnos;
				NSign *pendingSegnos2;
				NSign *pendingRitAccel;
				NClef *pendingClef;
				NTimeSig *pendingTimeSig;
				NKeySig *pendingKeySig;
				NSign *pendingBarSym;
				NSign *pendingBarSymAtEnd;
				NSign *pendingEnding;
				NSign *pendingEndingAtEnd;
				NSign *pendingTempo;
				NRest *pendingMultiRest;
		};
		voice_stat_str *voiceStatList_;
		int staffCount_;
		bool drum_problem_written_;
		int lastMeasureNum_;
		bool divisions_written_;
		bool keysig_written_;
#if GCC_MAJ_VERS > 2
		ostringstream *os_;
#else
		char buffer_[128];
		ostrstream *os_;
#endif
		int curTime_;
		int divisions_;
		NChord * dynEndChord;
		NChord * vaEndChord;
};


#endif /* MUSICXMLEXPORT_H */

