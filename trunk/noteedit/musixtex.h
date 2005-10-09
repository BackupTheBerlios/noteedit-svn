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

#ifndef MUSIXTEX_H

#define MUSIXTEX_H

#include <fstream>
#include <qstring.h>
#include "muselement.h"
// Wird eigentlich nur in musixtex.cpp gebraucht. Aber der Pointer
// muss Objektweit bekannt sein und musixtex.h wird auch in
// mainframewidget.cpp eingebunden...
// #include <qprocess.h> QT2 incompatible

using namespace std;

class NMusElement;
class NVoice;
class NStaff;
class exportFrm;
class NTimeSig;
class NKeySig;
struct trill_descr_str;
struct dynamics_descr_str {
	char *lastDynSym;
	int dynEndPos;
};

class NExportError {
	public:
		NExportError(int r, int m) {reason = r; measure = m;}
		int reason;
#define MUSIXTEX_ERR_128             1
#define MUSIXTEX_ERR_IRR_KEY         2
#define MUSIXTEX_ERR_TOO_MANY_TIES   3
#define MUSIXTEX_ERR_TOO_MANY_SLURS  4
#define MUSIXTEX_ERR_NESTED_TILLS    5
#define MUSIXTEX_ERR_TOO_MANY_TRILLS 6
#define MUSIXTEX_ERR_BEAM_COUNT      7
#define MUSIXTEX_ERR_INDIV_BAR       8
#define MUSIXTEX_ERR_DISCONT_PIANO   9
#define MUSIXTEX_ERR_TOO_MANY_VAS   10
#define MUSIXTEX_ERR_NESTED_VAS	    11
		int measure;
};

class pending_context_change_class {
	public:
		pending_context_change_class(int re_st_nr, int k) {
			real_staff_nr = re_st_nr;
			kind = k;
		}
		pending_context_change_class(int re_st_nr, int k, int d) {
			real_staff_nr = re_st_nr;
			kind = k;
			denom = d;
		}
		int real_staff_nr;
		int kind;
		int denom;
};

class NText;

class textDescr {
	public:
		textDescr(NText *telem, int st_nr) {
			textElem = telem;
			staff_nr = st_nr;
		}
		NText * textElem;
		int staff_nr;
};

class NMultistaffInfo;

class NMusiXTeX {
	public:
		NMusiXTeX();
		void exportStaffs(QString filen, QPtrList<NStaff> *stafflist, exportFrm *form, NMainFrameWidget *mainWidget);
		void doExport();
	private:
		void generate(int staff_nr, int real_staff_nr, const char *extraDelimiter,
			 NMusElement *elem, NStaff *staff_elem, NVoice *ac_voice);
		void writeContextChange();
		void writeChordDiagram(NChordDiagram *diag);
		NKeySig *getKeySig(int multistaffnr);
		NTimeSig *getTimeSig(int multistaffnr);
		QString *getStaffName(int multistaffnr);
		void writeStaffTexts(int real_staff_nr);
		NMultistaffInfo *mStaffInf_;
		ofstream out_;
		unsigned int staffCount_;
		bool lastSignWasBar_;
		bool lastSignWasRepeatClose_;
		QString lyrics2TeX(QString *lyrics);
		int *beamNrs_;
		bool elemsWritten_;
		unsigned int slurTiePool_;
		int barNr_;
		int maxTies_;
		int maxSlurs_;
		int lastBarNr_;
		bool spare_;
		bool limitMeasures_;
		bool noteSizeIsNormal_;
		int maxMeasuresPerLine_;
		unsigned int trillPool_;
		unsigned int vaPool_;
		unsigned int beamPool_;
		struct trill_descr_str *trill_descr_;
		struct trill_descr_str *va_descr_;
		QPtrList<NExportError> badMeasures_;
		exportFrm *exportDialog_;
		QPtrList<NStaff> *staffList_;
		QPtrList<NNote> shiftes_notes_, non_shifted_notes_;
		long lastBarPos_;
		int newTempo_;
		struct dynamics_descr_str *dysymDescr_;
		QString fileName;
		QPtrList<pending_context_change_class> pending_key_changes_, pending_time_changes_;
		QPtrList<textDescr> pending_texts_;
		NMainFrameWidget *mainWidget_;
		void externalCmd (QString command, QString filename);
};

#endif // MUSIXTEX_H
