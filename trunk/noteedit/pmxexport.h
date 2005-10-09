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

#ifndef PMXEXPORT_H

#define PMXEXPORT_H


#include "config.h"
#if GCC_MAJ_VERS > 2
#include <sstream>
#include <fstream>
#else
#include <fstream.h>
#include <strstream.h>
#endif
#include <qptrlist.h>
#include <qstring.h>

using namespace std;

class NVoice;
class NStaff;
class NKeySig;
class NNote;
class NClef;
class NChord;
class NTimeSig;
class NKeySig_;
class exportFrm;
class badmeasure;
class exportFrm;
class NMusElement;
class NPlayable;
class NMainFrameWidget;

class specialCharInfo {
	public:
		specialCharInfo(QString *info, int pos) {
			specInfo = new QString(*info);
			xpos = pos;
		}
		~specialCharInfo() {
			delete specInfo;
		}
		QString *specInfo;
		int xpos;
};
class NMultistaffInfo;
struct trill_descr_str;

class NPmxExport {
	public:
		void exportStaffs(QString fname, QPtrList<NStaff> *stafflist, exportFrm *frmWin, NMainFrameWidget *mainWidget);
		void doExport();
	private:
		QPtrList<badmeasure> badlist_;
		QString lyrics2TeX(QString *lyrics);
		ofstream out_;
#if GCC_MAJ_VERS > 2
		ostringstream *pmxout_;
#else
		ostrstream *pmxout_;
#endif
		int countof128th_;
		exportFrm *exportDialog_;
		int staffsToExport_;
		QString fileName;
		QPtrList<NStaff> *staffList_;
		void inspectTuplet(NPlayable *elem, int staff_nr, int barnr_);
		bool writeTrack(NVoice *voice, int staff_nr, int voice_nr,
				int voice_count, int measpsystem, bool first, const char *endOfLine, int *barpos);
		void handleSpecialChar(int elempos, QString *specialChar, int voice_count, bool force_output);
		void append_hidden_rests(int measpsystem, int total);
		NMultistaffInfo *mStaffInf_;
		QString computePMXTupletLength(int len);
		QString computePMXTupletLength(int length, int staff, int measure);
		void checkSpecialChar(int newxpos);
		int computePMXLength(int length);
		void pitchOut(NKeySig *ksig, const NNote *note, int length, NClef *ac_clef, NChord *chord, int staff_nr, int barnr);
		void setTie(NNote *note, int staff_nr, int barnr);
		void setSlur(NChord *chord, int staff_nr, int barnr);
		bool testContextChange(int voice_nr, NVoice *voice, bool first);
#if GCC_MAJ_VERS > 2
		void lineOut(ostringstream *outstream);
#else
		void lineOut(ostrstream *outstream);
#endif
		int barNr_;
		int tupletBase_;
		NTimeSig *pendingTimeSig_;
		int posOfpendingTimeSig_;
		NKeySig *pendingKeySig_;
		int posOfpendingKeySig_;
		int pendingSpecialEnd_;
		int posOfpendingSpecialEnd_;
		int lastTone_, lastLength_;
		int openSpecialEnding_;
		bool pendingEndSpecialEnd_;
		bool drum_problem_written_;
		unsigned int tiePool_, slurPool_, vaPool_;
		QString LastPMXfile_;
		QPtrList<specialCharInfo> specialCharList_;
		struct trill_descr_str *va_descr_;
		NMainFrameWidget *mainWidget_;
};

#endif //
