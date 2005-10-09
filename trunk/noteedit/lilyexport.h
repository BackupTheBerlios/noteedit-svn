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

#ifndef LILYEXPORT_H

#define LILYEXPORT_H

#include "config.h"
#include "muselement.h" /* needed for property_type */
#if GCC_MAJ_VERS > 2
#include <sstream>
#include <fstream>
#else
#include <strstream.h>
#include <fstream.h>
#endif

using namespace std;

class NMusElement;
class NMainFrameWidget;
class NStaff;
class NVoice;
class exportFrm;
class NKeySig;
class NNote;
class NClef;
class badmeasure;
class layoutDef;

#define STEM_UNSET 0
#define STEM_DIR_UP 1
#define STEM_DIR_DOWN 2


struct staffdescr {
	int lyrics_count;
	bool is_selected;
};

class NLilyExport {
	public:
		NLilyExport();
		void exportStaffs(QString fname, QPtrList<NStaff> *stafflist, exportFrm *expWin, NMainFrameWidget *mainWidget);
	private:
		ofstream out_;
		void writeVoice(int staff_nr, int voice_nr,  NVoice *voi);
		void writeLyrics(int voice_nr, NVoice *voi, const QString& label);
		void writeEncoded(const QString text);
		void writeEncodedAndReplaced(const QString s);
		void pitchOut( const NNote *note, NClef *ac_clef);
		bool hasContraryStems(QPtrList<NNote> *note);
		bool chordHasMixedTies(QPtrList<NNote> *note);
		bool hasATie(QPtrList<NNote> *note);
		void writeChordName(QString chordname);
		void analyseGroup(layoutDef *group, NMainFrameWidget *mainWidget, int staffCount, bool *continuedBars, bool *discontinuedBars);
		bool continuedOutsideAGroup(NMainFrameWidget *mainWidget, int staffCount);
		const char *LilyPondKeyName(property_type kind, int count);
		int lastLine_, lastLength_, lastDotted_;
		int depth_;
		int actualStemPolicy_;
		int countof128th_;
		int currentNumerator_, currentDenominator_;
		void removeExceptsFromString(QString *str,  bool onlyDigits);
		void tabsOut();
		staffdescr *staffarray_;
		int barNr_;
		QPtrList<badmeasure> badlist_;
		exportFrm *exportDialog_;
#if GCC_MAJ_VERS > 2
		ostringstream *os_;
#else
		char buffer_[128];
		ostrstream *os_;
#endif
		bool drum_problem_written_;
		bool va_problem_written_;
		QString lastLilyPondfile_;
		static QRegExp nonAlphas_, digits_, whiteSpaces_, relSyms, starOnly;
		property_type noteBody_;
		bool noStrongPizzMsg_;
		QByteArray scoreBraceMasks;
		void buildBraceMasks(QPtrList<NStaff> *stafflist, const NMainFrameWidget *mainWidget);
		QPtrList<QString> scoreBlock;
		void buildScoreBlockAndFlush(int i, NStaff *staff_elem, const QString& label, QPtrList<NStaff> *stafflist,
			const QByteArray braceMasks, bool flush);
};

#endif // LILYEXPORT_H
