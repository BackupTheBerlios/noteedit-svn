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

#include "pmxexport.h"
#include <stdlib.h>
#include <qcheckbox.h>
#include <kfiledialog.h> 
#include <kmessagebox.h>
#include <klocale.h>
#include <alloca.h>
#include <qregexp.h>
#include <qdial.h>
#include <qspinbox.h>
#include "resource.h"
#include "mainframewidget.h"
#include "keysig.h"
#include "timesig.h"
#include "clef.h"
#include "rest.h"
#include "chord.h"
#include "staff.h"
#include "text.h"
#include "filehandler.h"
#include "uiconnect.h"
#include "scaleedit_impl.h"
#include "multistaffinfo.h"
#include "layout.h"

#define PMX_ERR_BAD_NOTE_COUNT    1
#define PMX_ERR_TOO_MANY_TIES     2
#define PMX_ERR_TOO_MANY_SLURS    3
#define PMX_ERR_NOT_NUM_TUMPLET   4
#define PMX_ERR_MULTIPLE_VOICES   5
#define PMX_ERR_TUPLET_ENDS_REST  6
#define PMX_WARN_MIXED_GRACES     7
#define PMX_ERR_GRACES            8
#define PMX_TUPLET_LENGTH         9
#define PMX_ERR_INDIV_BAR        10
#define PMX_ERR_DISCONT_PIANO    11
#define PMX_ERR_DRUM_STAFF       12
#define PMX_ERR_MULTIREST	 13
#define PMX_ERR_NESTED_VAS	 14
#define PMX_ERR_TOO_MANY_VAS	 15

#define MAX_LINE_CHARS 128

#define MAXTIES 9
#define MAXSLURS 9
#define OBLONG 4096
#define BUFLONG 512

#define PMX_FAC 0.5

void NPmxExport::exportStaffs(QString fname, QList<NStaff> *stafflist, exportFrm *frmWin, NMainFrameWidget *mainWidget) {
	mainWidget_ = mainWidget;
	staffList_ = stafflist;
	exportDialog_ = frmWin;
	fileName = fname;
	specialCharList_.setAutoDelete(true);
	this->doExport();
}

void NPmxExport::doExport() {
	int i, j;
	NVoice *voice_elem;
	NStaff *staff_elem;
	NTimeSig *timesig;
	NKeySig  *keysig;
	NClef *clef;
	QList<QString> lyrNames;
	QString *lyrName;
	QString lyricslist[NUM_LYRICS];
	int kind, count;
	int acr;
	bool staffsWritten, first;
	const char *endOfLine;
	int voice_count;
	int barpos;
	int count_of_lyrics;
	int idx;
	int multistaffnr;
	int numOfStaffsInMultistaff;
#if GCC_MAJ_VERS > 2
	ostringstream os;
	ostringstream *pmxout[2];
#else
	char obuffer[2][OBLONG];
	char buffer[BUFLONG];
	ostrstream os(buffer, 100);
	ostrstream *pmxout[2];
#endif
	badmeasure *bad;
	lastTone_ = 1000;
	lastLength_ = 1000;
	openSpecialEnding_ = 0;
	pendingSpecialEnd_ = 0;
	pendingEndSpecialEnd_ = false;
	tupletBase_ = 0;

	countof128th_ = 128;
	barNr_ = 1;
	tiePool_ = 0;
	slurPool_ = 0;
	vaPool_ = 0;
	pendingTimeSig_ = 0;
	pendingKeySig_ = 0;
	pendingSpecialEnd_ = 0;
	drum_problem_written_ = false;

	specialCharList_.clear();
	badlist_.clear();
	if (fileName.isNull())
		return;
	if (NResource::staffSelExport_ == 0) {
		NResource::staffSelExport_ = new bool[staffList_->count()];
		for (i = 0; i < staffList_->count(); NResource::staffSelExport_[i++] = true);
		staffsToExport_ = staffList_->count();
	}
	else {
		staffsToExport_ = 0;
		for (i = 0; i < staffList_->count(); i++) {
			if (NResource::staffSelExport_[i]) {
				staffsToExport_++;
			}
		}
	}
	out_.open(fileName);
	if (!out_) {
		os << "error opening file " << fileName << '\0';
#if GCC_MAJ_VERS > 2
		KMessageBox::sorry
		  (0, QString(os.str().c_str()), kapp->makeStdCaption(i18n("PMX export")));
#else
		KMessageBox::sorry
		  (0, QString(os.str()), kapp->makeStdCaption(i18n("PMX export")));
#endif
		return;
	}
#if GCC_MAJ_VERS > 2
	pmxout[0] = new ostringstream();
	pmxout[1] = new ostringstream();
#else
	pmxout[0] = new ostrstream(obuffer[0], OBLONG);
	pmxout[1] = new ostrstream(obuffer[1], OBLONG);
#endif
	va_descr_ = (struct trill_descr_str *) alloca(staffsToExport_ * sizeof(struct trill_descr_str));
	for (i = 0; i < staffsToExport_; i++) {
		va_descr_[i].trill_nr = -1;
	}
	LastPMXfile_ = fileName;
	voice_elem = staffList_->first()->getVoiceNr(0);
	keysig = voice_elem->getFirstKeysig();
	if (!keysig) {	
		keysig = new NKeySig(0, 0);
	}
	timesig = voice_elem->getFirstTimeSig();
	if (!timesig) {
		timesig = new NTimeSig(0, &NResource::nullprops_);
		timesig->setSignature(4, 4);
	}
	countof128th_ = timesig->numOf128th();
	mStaffInf_ = new NMultistaffInfo(mainWidget_, staffList_, staffList_->count());
	if (mStaffInf_->hasDisconnectedPianoBars()) {
		bad = new badmeasure(PMX_ERR_DISCONT_PIANO, 1 /*dummy */, 0 /* dummy */, 3 /*dummy */,  128 /*dummy */);
		badlist_.append(bad);
	}

	
	out_ << "%-----------------------------------------%" << endl;
	out_ << "%                                         %" << endl;
	out_ << "% PMX output generated by \"NoteEdit\"    %" << endl;
	out_ << "%                                         %" << endl;
	out_ << "%-----------------------------------------%" << endl;
	if (exportDialog_->pmxMLyr->isChecked()) {
		out_ << "---" << endl;
		out_ << "\\input musixtex" << endl;
		out_ << "\\input pmx" << endl;
		out_ << "\\input musixlyr" << endl << endl;
		lyrNames.setAutoDelete(true);
		for (i = 0, staff_elem = staffList_->first(); staff_elem; staff_elem = staffList_->next(), i++) {
			if (!NResource::staffSelExport_[i]) continue;
			voice_elem = staff_elem->getVoiceNr(0);
			if (!(count_of_lyrics = voice_elem->countOfLyricsLines())) continue;
			voice_elem->collectLyrics(lyricslist);
			lyrNames.clear();
			for (j = 0; j < NUM_LYRICS; j++) {
				if (!lyricslist[j].isEmpty()) {
					lyrName = new QString();
					if (count_of_lyrics < 2) {
						lyrName->sprintf("lyrstaff%d", staffsToExport_ - i);
					}
					else {
						lyrName->sprintf("lyrstaff%dverse%d", staffsToExport_ - i, j+1);
					}
					lyrNames.append(lyrName);
					out_ << "\\setlyrics{" << (*lyrName) << "}{" << lyrics2TeX(&(lyricslist[j])) << "}" << endl;
				}
			}
	
			out_ << "\\assignlyrics{" << (staffsToExport_ - i) << "}{";
			while(!lyrNames.isEmpty()) {
				out_ << *(lyrNames.first());
				lyrNames.remove();
				if (!lyrNames.isEmpty()) out_ << ',';
			}
			out_ << '}' << endl << endl;
		}
		out_ << "---" << endl;
	}
/*
	out_ << "---" << endl;
	out_ << "\\def\\mtxInterInstrument#1#2{\\setinterinstrument{#1}{#2\\Interligne}}" << endl;
	out_ << "\\def\\mtxStaffBottom#1{\\staffbotmarg #1\\Interligne}" << endl;
	out_ << "---" << endl << endl;
*/

	out_ << "% nv,noinst,mtrnuml,mtrdenl,mtrnump,mtrdenp,xmtrnum0,isig," << endl;
	out_ << "   " << staffsToExport_ << "    ";
	if (staffsToExport_ == mStaffInf_->getMultiStaffCount()) {
		out_ << staffsToExport_ << "      ";
	}
	else {
		out_ << '-' << mStaffInf_->getMultiStaffCount() << ' ';
		for (i = 0; i < mStaffInf_->getMultiStaffCount(); i++) {
			out_ << mStaffInf_->getStaffCount(mStaffInf_->getMultiStaffCount()-i-1) << ' ';
		}
	}
        out_ << timesig->getNumerator() << "      " <<  timesig->getDenominator() << "        " <<
		timesig->getNumerator() << "      " <<  timesig->getDenominator();
	if (acr = voice_elem->determineAnacrusis()) {
		out_ << "        " << ((double) acr / (128.0 / (double) timesig->getDenominator())) << "      ";
	}
	else {
		out_ << "        0      ";
	}
	if (keysig->isRegular(&kind, &count)) {
		if (kind == STAT_FLAT) count = -count;
		out_ << count << endl;
	}
	else {
		out_ << "0" << endl;
	}
	out_ << "% npages,nsyst,musicsize,fracindent" << endl;
	out_ << "    " << exportDialog_->pmxNum->getValue() << "    " << exportDialog_->pmxSystem->getValue() << "      20       0.07" << endl;
	out_ << "%" << endl;

	for (i = 0; i < mStaffInf_->getMultiStaffCount(); i++) {
		if ((staff_elem = staffList_->at(mStaffInf_->getfirstStaffInMultistaff(mStaffInf_->getMultiStaffCount()-i-1))) == 0) {
			NResource::abort("NPmxExport::doExport: internal error", 1);
		}
		if (!staff_elem->staffName_.isEmpty()) {
			out_ << staff_elem->staffName_ << endl;
		}
		else {
			out_ << endl;
		}
	}
	for (i = 0, staff_elem = staffList_->last(); staff_elem; staff_elem = staffList_->prev(), i++) {
		if (!NResource::staffSelExport_[staffList_->count() - i - 1]) continue;
		if ((voice_count = staff_elem->voiceCount()) > 2) {
			bad = new badmeasure(PMX_ERR_MULTIPLE_VOICES, i+1, 0, 3 /*dummy */,  128 /*dummy */);
			badlist_.append(bad);
		}
		voice_elem = staff_elem->getVoiceNr(0);
		clef = voice_elem->getFirstClef();
		switch (clef->getSubType()) {
			case BASS_CLEF: out_ << "b"; break;
			case SOPRANO_CLEF: out_ << "s"; break;
			case ALTO_CLEF: out_ << "a"; break;
			case TENOR_CLEF: out_ << "n"; break;
			case DRUM_CLEF: if (!drum_problem_written_) {
						drum_problem_written_ = true;
						bad = new badmeasure(PMX_ERR_DRUM_STAFF, i+1, 0, 3 /*dummy */,  128 /*dummy */);
						badlist_.append(bad);
					}
			case DRUM_BASS_CLEF: if (!drum_problem_written_) {
						drum_problem_written_ = true;
						bad = new badmeasure(PMX_ERR_DRUM_STAFF, i+1, 0, 3 /*dummy */,  128 /*dummy */);
						badlist_.append(bad);
					    }
			default: out_ << "t"; break;
		}
		for (j = 0; j < voice_count && j < 2; j++)  {
			voice_elem = staff_elem->getVoiceNr(j);
			voice_elem->prepareForWriting();
		}
	}
	out_ << endl << "./" << endl;
	mStaffInf_->writeAkkoladen(&out_, true);
	if (!mStaffInf_->ContinuedBarLines()) {
		if (mStaffInf_->DiscontOutsidePiano()) {
			bad = new badmeasure(PMX_ERR_INDIV_BAR, 1 /* dummy */, 0 /* dummy */, 3 /*dummy */,  128 /*dummy */);
			badlist_.append(bad);
			out_ << "\\\\input musixdbr.tex\\relax\\" << endl;
			out_ << "\\\\indivbarrules\\" << endl << "\\\\allbarrules\\sepbarrule";
			for (i = 0; i < staffList_->count(); i++) {
				if (mainWidget_->barCont_[i].valid) {
					for (j = mainWidget_->barCont_[i].beg; j < mainWidget_->barCont_[i].end; j++) {
						idx = mStaffInf_->multistaffIdxOfStaff(j, &multistaffnr, &numOfStaffsInMultistaff);
						if (idx == numOfStaffsInMultistaff - 1) {
							out_ << "\\conbarrule{" << (mStaffInf_->getMultiStaffCount() - multistaffnr) << '}';
						}
					}
				}
			}
		}
		else {
			out_ << "\\\\sepbarrules";
		}
		out_ << "\\" << endl;
	}
	out_ << "w" << exportDialog_->pmxWidth->text() << "m" << endl;
	out_ << "h" << exportDialog_->pmxHeight->text() << "m" << endl;
	if (!mainWidget_->scTitle_.isEmpty()) {
		out_ << "Tt" << endl << '{' << mainWidget_->scTitle_ << '}' << endl;
	}
	if (!mainWidget_->scAuthor_.isEmpty()) {
		out_ << "Tc" << endl << '{' << mainWidget_->scAuthor_ << '}' << endl;
	}
/*
	out_ << endl << "\\\\mtxStaffBottom{2}\\" << endl;
	for (i = 0; i < staffsToExport_; i++) {
		out_ << "\\\\mtxInterInstrument{" << (i+1) << "}{-1}\\" << endl;
	}
*/
	do {
		staffsWritten = false;
		first = true;
		out_ << endl << "% Measure " << barNr_ << " - " <<
			  barNr_ + exportDialog_->pmxMeasure->getValue() - 1 << endl;
		for (i = staffList_->count(), staff_elem = staffList_->last(); staff_elem; staff_elem = staffList_->prev(), i--) {
			if (!NResource::staffSelExport_[i-1]) continue;
			voice_count = staff_elem->voiceCount();
			for (j = 0; j < voice_count && j < 2; j++)  {
				voice_elem = staff_elem->getVoiceNr(j);
				endOfLine = j == 0 ? "/" : "//";
				pmxout_ = pmxout[j];
				if (j == 0) barpos = (1 << 30); /* infinity */
				lastLength_ = 1000; /* invalidate */
				lastTone_ = 1000; /* invalidate */
				if (writeTrack(voice_elem, i, j, 
					voice_count, exportDialog_->pmxMeasure->getValue(), first, endOfLine, &barpos)) {
					staffsWritten = true;
				}
			}
			first = false;
			if (staffsWritten) {
				for (j = voice_count > 1 ? 1 : 0; j >= 0; j--)  {
					lineOut(pmxout[j]);
#if GCC_MAJ_VERS > 2
					delete pmxout[j];
					pmxout[j] = new ostringstream();
#else
					pmxout[j]->seekp(0);
#endif
				}
			}
		}
		barNr_ += exportDialog_->pmxMeasure->getValue();
	}
	while (staffsWritten);
	out_.close();
	if (NResource::staffSelExport_ != 0) {
		delete [] NResource::staffSelExport_;
		NResource::staffSelExport_ = 0;
	}
	delete mStaffInf_;
	if (!badlist_.isEmpty()) {
		QString output;
		output = i18n
			("Noteedit has exported the score to PMX but there are some\n"
			 "problems which will probably prevent successful PMX output.\n");
		output += i18n("-----------------------------------------------------\n");
		for (bad = badlist_.first(); bad; bad = badlist_.next()) {
			switch (bad->kind) {
				case PMX_ERR_BAD_NOTE_COUNT :
				      output += i18n
					( "Staff %1, measure %2: %3 128th, should be: %4\n").
					arg(bad->track).arg(bad->measure).arg(bad->realcount).arg(bad->shouldbe);
				      break;
				case PMX_ERR_MULTIREST :
				      output += i18n
					( "Staff %1, measure %2: PMX cannot deal with multi rests if more than 1 staff is exported").
					arg(bad->track).arg(bad->measure);
				      break;
				case PMX_ERR_TOO_MANY_TIES:
					output += i18n
					( "measure %1: Too many opened ties\n").arg(bad->measure);
					break;
				case PMX_ERR_TOO_MANY_SLURS:
					output += i18n
					( "measure %1: Too many opened slurs\n").arg(bad->measure);
					break;
				case PMX_ERR_NOT_NUM_TUMPLET:
					output += i18n
					("Staff %1, measure %2: PMX can only deal with equal tuplet members or members in ratio 2:1\n").
					arg(bad->track).arg(bad->measure);
					break;
				case PMX_ERR_TUPLET_ENDS_REST:
					output += i18n
					("Staff %1, measure %2: In PMX tuplet cannot end with a rest\n").
					arg(bad->track).arg(bad->measure);
					break;
				case PMX_ERR_MULTIPLE_VOICES:
					output += i18n
						("PMX cannot deal with more than 2 voices per staff\n");
					break;
				case PMX_ERR_DRUM_STAFF:
					output += i18n
						("Staff %1 contains drum staff. This cannot be expressed in PMX.\n")
						.arg(bad->track);
					break;
				case PMX_WARN_MIXED_GRACES:
					output += i18n
					("Staff %1, measure %2: PMX cannot deal with mixed graces\n").
					arg(bad->track).arg(bad->measure);
					break;
				case PMX_ERR_GRACES:
					output += i18n
					("Staff %1, measure %2: grace has no main note. PMX will not work\n").
					arg(bad->track).arg(bad->measure);
					break;
				case PMX_TUPLET_LENGTH:
					output += i18n
					("Staff %1, measure %2: Tuplet must be expressible by on (ev. dotted) note. PMX will not work\n").
					arg(bad->track).arg(bad->measure);
					break;
				case PMX_ERR_INDIV_BAR:
					output += i18n
					("The score has individual bar layout (partial continued and partial discontinued)\nPlease install \"musixdbr.tex\" by  Rainer Dunker\n");
					break;
				case PMX_ERR_DISCONT_PIANO:
					output += i18n
					("The score has piano staffs with discontinued bar rules. This cannot be expressed in PMX\n");
					break;
				case PMX_ERR_NESTED_VAS:
					output += i18n
					("Nested trills in measure: %1 ;\n").arg(bad->measure);
					break;
				case PMX_ERR_TOO_MANY_VAS:
					output += i18n
					("Too many va lines in measure: %1: maximum = 6\n").arg(bad->measure);
					break;
			}
		}
		NResource::exportWarning_->setOutput(i18n ("PMX produced. But there are some problems."), &output);
		NResource::exportWarning_->show();
	}
}

bool NPmxExport::writeTrack(NVoice *voice,  int staff_nr, int voice_nr, int voice_count, int measpsystem, bool first, const char *endOfLine, int *barpos) {
	NMusElement *elem;
	QString *lyrics;
	int total = 0;
	int total_per_system = 0;
	int part, countLyricsLines;
#ifdef THIS_IS_WRONG
	bool base_shifted;
#endif
	int numMeasures = 0;
	NStaff *actual_staff;
	NChord *chord;
	NNote *note;
	NKeySig *ksig;
	NClef *clef;
	badmeasure *bad;
	int kind, count, i;
	char lastDynSym = 0;
	int dynEndPos = 0;
	int lastbarpos = *barpos;
	int elempos;
#define OUTSIDE_BEAM 0
#define IN_BEAM_UP   1
#define IN_BEAM_DOWN 2
	int beamstatus = OUTSIDE_BEAM;
	int hline;
	QString specialChar;
	QString *s;
	int grace_status;
	bool inGraceGroup = false;
	bool nested, toomany;
	QString graceString;
	NText *textElem;
	char pos;

	elem = voice->getCurrentPosition();
	if (elem == 0) return false;
	elempos = elem->getXpos();
	actual_staff = voice->getStaff();
	if (pendingTimeSig_ && first) {
		specialChar.sprintf("m%d/%d/%d/%d ", pendingTimeSig_->getNumerator(),
				pendingTimeSig_->getDenominator(),
				pendingTimeSig_->getNumerator(),
				pendingTimeSig_->getDenominator());
		countof128th_ = pendingTimeSig_->numOf128th();
		pendingTimeSig_ = 0;
		handleSpecialChar(posOfpendingTimeSig_, &specialChar, voice_count, false);
	}
	if (pendingKeySig_ && first) {
		if (pendingKeySig_->isRegular(&kind, &count)) {
			if (kind == STAT_CROSS) {
				specialChar.sprintf("K+0+%d ", count);
			}
			else {
				specialChar.sprintf("K+0-%d ", count);
			}
			handleSpecialChar(posOfpendingKeySig_, &specialChar, voice_count, false);
		}
		pendingKeySig_ = 0;
	}
	if (pendingEndSpecialEnd_ && first) {
		specialChar = QString("Vx ");
		handleSpecialChar(posOfpendingSpecialEnd_, &specialChar, voice_count, false);
		pendingEndSpecialEnd_ = false;
	}
	if (pendingSpecialEnd_ && first) {
		if (pendingSpecialEnd_ == 1) {
			specialChar.sprintf("V1 ");
		}
		else {
			specialChar.sprintf("V2x ");
		}
		handleSpecialChar(posOfpendingSpecialEnd_, &specialChar, voice_count, false);
		if (pendingSpecialEnd_ == 2) 
			openSpecialEnding_ = 1;
		pendingSpecialEnd_ = 0;
	}
	do {
		if (voice_count > 1 && voice_nr == voice_count - 1) {
			checkSpecialChar(elem->getXpos());
		}
		elempos = elem->getXpos();
		switch (elem->getType()) {
			case T_CHORD: 
				     part = elem->getSubType(); 
				     if (!(elem->status_ & STAT_GRACE)) {
					inGraceGroup = false;
					if (elem->status_ & STAT_TUPLET) {
						total += elem->getPlaytime() * part / elem->getNumNotes();
					}
					else {
						total += part;
					}
				     }
				     switch (elem->status_ & DOT_MASK) {
					case 1: total += part / 2; break;
					case 2: total += 3 * part / 4; break;
				     }
				     if (total > MULTIPLICATOR*countof128th_ && voice_nr == 0) {
					total = 0;
					bad = new badmeasure(PMX_ERR_BAD_NOTE_COUNT, staff_nr, barNr_+numMeasures, total / 3, countof128th_);
					badlist_.append(bad);
				     }
				     chord = (NChord *) elem;
				     if (va_descr_[staff_nr-1].trill_nr >= 0 && 	
					chord->getBbox()->right() > va_descr_[staff_nr-1].endpos) {
					*pmxout_ << "\\Toctfin" << va_descr_[staff_nr-1].trill_nr << "\\ ";
					vaPool_ &= (~(1 << va_descr_[staff_nr-1].trill_nr));
					va_descr_[staff_nr-1].trill_nr = -1;
				     }
				     if (chord->va_ != 0) {
					hline = voice->findBorderLineInVa(chord);
					s = chord->computeTeXVa(chord->va_ < 0, hline, &vaPool_, &(actual_staff->actualClef_),
						&(va_descr_[staff_nr-1]), &nested, &toomany);
					if (nested) {
						bad = new badmeasure(PMX_ERR_NESTED_VAS, staff_nr, barNr_+numMeasures, 3 /* dummy */, 3 /* dummy */);
						badlist_.append(bad);
					}
					if (toomany) {
						bad = new badmeasure(PMX_ERR_TOO_MANY_VAS, staff_nr, barNr_+numMeasures, 3 /* dummy */, 3 /* dummy */);
						badlist_.append(bad);
					}
					if (s) {
						*pmxout_ << *s << "\\ ";
						delete s;
					}
				      }
				     if (exportDialog_->pmxKeepBeams->isChecked()) {
					if ((chord->status_ & STAT_BEAMED) && beamstatus == OUTSIDE_BEAM) {
						*pmxout_ << '[';
						if (chord->status_ & STAT_STEM_UP) {
							beamstatus = IN_BEAM_UP;
							*pmxout_ << "u ";
						}
						else {
							beamstatus = IN_BEAM_DOWN;
							*pmxout_ << "l ";
						}
				        }
				     } 
				     if (!exportDialog_->pmxMLyr->isChecked()) {
					countLyricsLines = chord->countOfLyricsLines();
					for (i = countLyricsLines-1; i >= 0; i--) {
						lyrics = chord->getLyrics(i);
						if (lyrics) {
							*pmxout_ << " \\zcharnote{" << -8 - (int) ((actual_staff->staff_props_.lyricsdist - DEFAULT_LYRICSDIST ) * PMX_FAC) -i*4 << "}{"<< lyrics2TeX(lyrics) << "}\\relax\\ ";
						}
						else {
				     			*pmxout_ << " \\zcharnote{}\\relax\\ ";
				     		}
					}
				     }
				     if (!inGraceGroup && elem->status_ & STAT_GRACE) {
					graceString =  voice->determineGraceKind(&grace_status);
					inGraceGroup = true;
					if (grace_status == WARN_MIXED_GRACES) {
						bad = new badmeasure(PMX_WARN_MIXED_GRACES, i+1, 0, 3 /*dummy */,  128 /*dummy */);
						badlist_.append(bad);
					}
					else if (grace_status == GRACE_PMX_ERROR) {
						bad = new badmeasure(PMX_ERR_GRACES, i+1, 0, 3 /*dummy */,  128 /*dummy */);
						badlist_.append(bad);
					}
				        *pmxout_ << graceString;
				     }
				     if (beamstatus == IN_BEAM_UP) {
				     	note = elem->getNoteList()->last();
				     }
				     else {
				     	note = elem->getNoteList()->first();
				     }
				     pitchOut(&(actual_staff->actualKeysig_), note, elem->getSubType(), &(actual_staff->actualClef_), (NChord *) elem, staff_nr, barNr_+numMeasures);
				     if (!drum_problem_written_ && (note->status & BODY_MASK)) {
					drum_problem_written_ = true;
					bad = new badmeasure(PMX_ERR_DRUM_STAFF, staff_nr, barNr_+numMeasures, total / 3, countof128th_);
					badlist_.append(bad);
				     }
				     switch (elem->status_ & DOT_MASK) {
					case 1: *pmxout_ << "d"; break;
					case 2: *pmxout_ << "dd"; break;
				     }
				     if ((elem->status_ & STAT_TUPLET) && elem->getSubType() != tupletBase_)  *pmxout_ << 'D';
#ifdef THIS_IS_WRONG
				     base_shifted = note->status & STAT_SHIFTED;
#endif
				     *pmxout_ << ' ';
				     if (elem->status_ & STAT_STACC) {
					*pmxout_ << "ou ";
				     }
				     if (elem->status_ & STAT_SFORZ) {
					*pmxout_ << "o^ ";
				     }
				     if (elem->status_ & STAT_PORTA) {
					*pmxout_ << "o_ ";
				     }
				     if (elem->status_ & STAT_SFZND) {
					*pmxout_ << "o> ";
				     }
				     if (elem->status_ & STAT_FERMT) {
				        *pmxout_ << "of ";
				     }
				     if (elem->status_ & STAT_ARPEGG) {
				        *pmxout_ << "? ";
				     }
				     if (chord->trill_ < 0) {
					*pmxout_ << "oTt" << voice->findNoteCountTillTrillEnd(chord) << ' ';
				     }
				     else if (chord->trill_ > 0) {
					*pmxout_ << "oTt" << voice->findNoteCountTillTrillEnd(chord) << ' ';
				     }
				     if (chord->dynamic_) {
					*pmxout_ << 'D' << (chord->dynamicAlign_ ? '<' : '>') << ' ';
					dynEndPos = chord->getDynamicEnd();
					lastDynSym = (chord->dynamicAlign_ ?  '>' : '<');
				     }
				     else if (lastDynSym && chord->getBbox()->right() > dynEndPos) {
					*pmxout_ << 'D' << lastDynSym << ' ';
					lastDynSym = 0;
					dynEndPos = 0;
				     }
				     setTie(note, staff_nr, barNr_+numMeasures);
				     setSlur((NChord *) elem, staff_nr, barNr_+numMeasures);
	
			             if (beamstatus == IN_BEAM_UP) {
					note = elem->getNoteList()->prev();
				     }
				     else {
					note = elem->getNoteList()->next();
				      }		
			  	     while (note) {
					     *pmxout_ << "z";
					     pitchOut(&(actual_staff->actualKeysig_), note, -1, &(actual_staff->actualClef_), (NChord *) elem, staff_nr, barNr_+numMeasures);
					     if (!drum_problem_written_ && (note->status & BODY_MASK)) {
						drum_problem_written_ = true;
						bad = new badmeasure(PMX_ERR_DRUM_STAFF, staff_nr, barNr_+numMeasures, total / 3, countof128th_);
						badlist_.append(bad);
					     }
#ifdef THIS_IS_WRONG
					     if (base_shifted) {
						if (!(note->status & STAT_SHIFTED)) *pmxout_ << "r";
					     }
					     else {
				     	     	if (note->status & STAT_SHIFTED) *pmxout_ << "r";
					     }
#endif
				     	     *pmxout_ << ' ';
					     setTie(note, staff_nr, barNr_+numMeasures);
			             	     if (beamstatus == IN_BEAM_UP) {
						note = elem->getNoteList()->prev();
				     	     }
				     	     else {
						note = elem->getNoteList()->next();
				      	     }		
				     }
				     if (elem->status_ & STAT_ARPEGG) {
				     		*pmxout_ << "? ";
				     }
				     if (exportDialog_->pmxKeepBeams->isChecked()) {
					if (beamstatus != OUTSIDE_BEAM && chord->lastBeamed()) {
						beamstatus = OUTSIDE_BEAM;
						*pmxout_ << " ] ";
				        }
				     } 
				     break;
			case T_REST:
				     inGraceGroup = false;
				     part = elem->getSubType(); 
				     if (part == MULTIREST) {
						total += MULTIPLICATOR*countof128th_;
				     }
				     else {
					     if (elem->status_ & STAT_TUPLET) {
						total += elem->getPlaytime() * part / elem->getNumNotes();
					     }
					     else {
						total += part;
					     }
				     }
				     switch (elem->status_ & DOT_MASK) {
					case 1: total += part / 2; break;
					case 2: total += 3 * part / 4; break;
				     }
				     if (total > MULTIPLICATOR*countof128th_ && voice_nr == 0) {
					total = 0;
					bad = new badmeasure(PMX_ERR_BAD_NOTE_COUNT, staff_nr, barNr_+numMeasures, total / 3, countof128th_);
					badlist_.append(bad);
				     }
					
				     *pmxout_ << 'r';
				     if (elem->status_ & STAT_HIDDEN) *pmxout_ << 'b';
				     if (elem->status_ & STAT_TUPLET) {
					if (elem->isFirstInTuplet()) {
						inspectTuplet(elem, staff_nr, barNr_+numMeasures);
						*pmxout_  << computePMXTupletLength(elem->getPlaytime()*tupletBase_, staff_nr, barNr_+numMeasures);
						lastLength_ = elem->getPlaytime()*tupletBase_;
					}
				     }
				     else if (part == MULTIREST) {
					if (staffsToExport_ > 1) {
						bad = new badmeasure(PMX_ERR_MULTIREST, staff_nr, barNr_+numMeasures, total / 3, countof128th_);
						badlist_.append(bad);
					}
					lastLength_ = 1111; /* invalidate */
					*pmxout_ << 'm' << ((NRest *) elem)->getMultiRestLength();
				     }
				     else if (lastLength_ != elem->getSubType()) {
				     	*pmxout_ << computePMXLength(elem->getSubType());
					lastLength_ = elem->getSubType();
				     }
				     switch (elem->status_ & DOT_MASK) {
					case 1: *pmxout_ << "d"; break;
					case 2: *pmxout_ << "dd"; break;
				     }
				     if (elem->status_ & STAT_TUPLET) {
						if (elem->isFirstInTuplet()) *pmxout_ << "x" << (int) (elem->getNumNotes());
						if (elem->getSubType() != tupletBase_) *pmxout_ << 'D';
				     }
				     *pmxout_ << ' ';
				     break;
			case T_SIGN: inGraceGroup = false;
				     switch (elem->getSubType ()) {
				        case END_BAR:
					case SIMPLE_BAR: 
							 if (total != MULTIPLICATOR*countof128th_ && barNr_+numMeasures > 1 && voice_nr == 0 ) {
								bad = new badmeasure(PMX_ERR_BAD_NOTE_COUNT, staff_nr, barNr_+numMeasures, total / 3, countof128th_);
								badlist_.append(bad);
							 }
							 total_per_system += total;
							 total = 0;
							 numMeasures++;
							 lastTone_ = 1000;
							 if (barNr_+numMeasures > 1  && testContextChange(voice_nr, voice, first)) {
								if (first) barNr_ -= measpsystem - numMeasures;
								*pmxout_ << endOfLine; 
								*barpos = elem->getXpos();
								elem = voice->getNextPosition();
								return true;
							 }
							 if (numMeasures >= measpsystem || voice->isLast()) {
								*barpos = elem->getXpos();
								elem = voice->getNextPosition();
								*pmxout_ << endOfLine; 
								return true;
							 }
							 specialChar = QString("| ");
							 handleSpecialChar(elempos, &specialChar, voice_count, true);
							 break;
					case DOUBLE_BAR:
					case REPEAT_OPEN:
					case REPEAT_CLOSE:
					case REPEAT_OPEN_CLOSE:
							 if (total != MULTIPLICATOR*countof128th_ && barNr_+numMeasures > 1 && voice_nr == 0) {
								bad = new badmeasure(PMX_ERR_BAD_NOTE_COUNT, staff_nr, barNr_+numMeasures, total / 3, countof128th_);
								badlist_.append(bad);
							 }
							 total_per_system += total;
							 total = 0;
							 numMeasures++;
							 lastTone_ = 1000;
							 if (first) {
								specialChar = QString("R");
								switch (elem->getSubType ()) {
									case REPEAT_OPEN: specialChar += "l "; break;
									case REPEAT_CLOSE: specialChar += "r "; break;
									case REPEAT_OPEN_CLOSE: specialChar += "lr "; break;
									case DOUBLE_BAR: specialChar += "d "; break;
								}
							 	handleSpecialChar(elempos, &specialChar, voice_count, false);
							 }
							 if (barNr_+numMeasures > 1  && testContextChange(voice_nr, voice, first)) {
								*barpos = elem->getXpos();
								elem = voice->getNextPosition();
								if (first) barNr_ -= measpsystem - numMeasures;
								*pmxout_ << endOfLine; 
								return true;
							 }
							 if (!(numMeasures >= measpsystem || voice->isLast())) {
								specialChar = QString("| ");
								handleSpecialChar(elempos, &specialChar, voice_count, true);
							 }
							 if (numMeasures >= measpsystem || voice->isLast()) {
								*barpos = elem->getXpos();
								elem = voice->getNextPosition();
								*pmxout_ << endOfLine; 
								return true;
							 }
							 break;
					case SEGNO: if (staff_nr != staffsToExport_ || voice_nr != 0) break;
							*pmxout_ << "og ";
							break;
					case CODA:	*pmxout_ << "\\hsk\\coda " << actual_staff->actualClef_.line2TexTab_[12+LINE_OVERFLOW] <<"\\hsk\\ ";
							break;
					case DAL_SEGNO: *pmxout_ << "\\Uptext{D.S.}\\sk\\sk\\ ";
							break;
					case DAL_SEGNO_AL_FINE: *pmxout_ << "\\Uptext{D.S. al Fine}\\sk\\sk\\ ";
							break;
					case DAL_SEGNO_AL_CODA: *pmxout_ << "\\Uptext{D.S. al Coda}\\sk\\sk\\ ";
							break;
					case FINE: *pmxout_ << "\\Uptext{Fine}\\sk\\sk\\ ";
							break;
					case RITARDANDO: *pmxout_ << "\\Uptext{ritard.}\\ ";
							break;
					case ACCELERANDO: *pmxout_ << "\\Uptext{accel.}\\ ";
							break;
				     }
				     break;
			case T_CLEF:
				inGraceGroup = false;
				clef = (NClef *) elem;
				actual_staff->actualClef_.change(clef);
				if (barNr_ < 2) break;
				switch (clef->getSubType()) {
					case BASS_CLEF: specialChar = QString("Cb "); break;
					case SOPRANO_CLEF: specialChar = QString("Cs "); break;
					case ALTO_CLEF: specialChar = QString("Ca "); break;
					case TENOR_CLEF: specialChar = QString("Cn "); break;
					case DRUM_CLEF: if (!drum_problem_written_) {
								drum_problem_written_ = true;
								bad = new badmeasure(PMX_ERR_DRUM_STAFF, i+1, 0, 3 /*dummy */,  128 /*dummy */);
								badlist_.append(bad);
							}
					case DRUM_BASS_CLEF: if (!drum_problem_written_) {
								drum_problem_written_ = true;
								bad = new badmeasure(PMX_ERR_DRUM_STAFF, i+1, 0, 3 /*dummy */,  128 /*dummy */);
								badlist_.append(bad);
							    }							
					default: specialChar = QString("Ct "); break;
				}
				handleSpecialChar(elempos, &specialChar, voice_count, false);
				break;
			case T_KEYSIG:
				if (barNr_ < 2) break;
				ksig = (NKeySig *) elem;
				if (ksig->isRegular(&kind, &count)) {
					if (kind == STAT_CROSS) {
						specialChar.sprintf("K+0+%d ", count);
					}
					else {
						specialChar.sprintf("K+0-%d ", count);
					}
					handleSpecialChar(posOfpendingKeySig_, &specialChar, voice_count, false);
				}
				break;
			case T_TEXT:
				textElem = (NText *) elem;
				if (textElem->getSubType() == TEXT_UPTEXT) {
					pos = actual_staff->actualClef_.line2TexTab_[11+LINE_OVERFLOW];
				}
				else {
					pos = actual_staff->actualClef_.line2TexTab_[-3+LINE_OVERFLOW];
				}
				*pmxout_  << "\\zcharnote{" << pos << "}{\\bf " << textElem->getText() << "}\\relax\\ ";
				break;
		}
		elem = voice->getNextPosition();
	}
	while (elem && elem->getXpos() < lastbarpos);
	if (voice_nr == 0) {
		*barpos = (1 << 30); /* infinity */
#ifdef THIS_DOES_NOT_WORK
		append_hidden_rests(measpsystem, total_per_system);
#endif
	}
#ifdef THIS_DOES_NOT_WORK
	else {
		append_hidden_rests(measpsystem, total);
	}
#endif
	if (numMeasures < measpsystem) *pmxout_ << endOfLine;
	return true;
}

void NPmxExport::append_hidden_rests(int measpsystem, int total) {
	int len = measpsystem * MULTIPLICATOR*countof128th_ - total;
	int len2, len3, dotcount;

	if (len < MULTIPLICATOR) return;
	len2 = (MULTIPLICATOR*countof128th_) - (total % (MULTIPLICATOR*countof128th_));
	if (len2 >= MULTIPLICATOR) len -= len2;
	while (len2 >= MULTIPLICATOR) {
		len3 = NVoice::quant(len2, &dotcount, MULTIPLICATOR*countof128th_);
		*pmxout_ << "rb";
		*pmxout_ << computePMXLength(len3);
		if (dotcount) *pmxout_ << 'd';
		*pmxout_ <<  ' ';
		len2 -= dotcount ? 3 * len3 / 2 : len3;
	}
	while (len >= MULTIPLICATOR) {
		len2 = NVoice::quant(len, &dotcount, MULTIPLICATOR*countof128th_);
		*pmxout_ << "rb";
		*pmxout_ << computePMXLength(len2);
		if (dotcount) *pmxout_ << 'd';
		*pmxout_ <<  ' ';
		len -= dotcount ? MULTIPLICATOR * len2 / 2 : len2;
	}
}
	
void NPmxExport::pitchOut(NKeySig *ksig, const NNote *note, int length, NClef *ac_clef, NChord *chord, int staff_nr, int barnr) {
	int octave;
	int tone;
	bool octaveneeded;
	bool firstTuplet = false;

	*pmxout_ << ac_clef->line2PMXName(note->line, &octave);
	tone = ac_clef->line2midiTab_[note->line+LINE_OVERFLOW];
	octaveneeded = abs(lastTone_ - tone) > 5;
	if (chord->status_ & STAT_TUPLET) {
		if (chord->isFirstInTuplet() &&  length >= 0) {
			inspectTuplet(chord, staff_nr, barnr);
			firstTuplet = true;
			*pmxout_  << computePMXTupletLength(chord->getPlaytime()*tupletBase_, staff_nr, barnr);
			lastLength_ = chord->getPlaytime()*tupletBase_;
		}
	}
	else if (!(chord->status_ & STAT_GRACE) && length >= 0 && (lastLength_ != length || octaveneeded)) {
		*pmxout_  << computePMXLength(length);
		lastLength_ = length;
	}
	if (!(note->status & STAT_PART_OF_TIE)) {
		if (!(note->status & STAT_FORCE)) {
			switch (note->needed_acc) {
				case STAT_CROSS: *pmxout_ << "s"; ksig->setTempAcc(note->line, STAT_CROSS); break;
				case STAT_FLAT: *pmxout_ << "f"; ksig->setTempAcc(note->line, STAT_FLAT); break;
				case STAT_NATUR: *pmxout_ << "n"; ksig->setTempAcc(note->line, STAT_NATUR); break;
				case STAT_DCROSS: *pmxout_ << "ss"; ksig->setTempAcc(note->line, STAT_DCROSS); break;
				case STAT_DFLAT: *pmxout_ << "ff"; ksig->setTempAcc(note->line, STAT_DFLAT); break;
			}
		}
		else {
			switch (note->offs) {
				case  1: *pmxout_ << "s"; ksig->setTempAcc(note->line, STAT_CROSS); break;
				case -1: *pmxout_ << "f"; ksig->setTempAcc(note->line, STAT_FLAT); break;
				case  0: *pmxout_ << "n"; ksig->setTempAcc(note->line, STAT_NATUR); break;
				case  2: *pmxout_ << "ss"; ksig->setTempAcc(note->line, STAT_DCROSS); break;
				case -2: *pmxout_ << "ff"; ksig->setTempAcc(note->line, STAT_DFLAT); break;
			}
		}
	}
	if (octaveneeded) *pmxout_ << octave;
	if (firstTuplet) *pmxout_ << "x" << ((int) chord->getNumNotes());
	lastTone_ = tone;
}

int NPmxExport::computePMXLength(int length) {
	int len = 4;

	switch (length) {
		case DOUBLE_WHOLE_LENGTH : len = 9; break;
		case WHOLE_LENGTH        : len = 0; break;
		case HALF_LENGTH         : len = 2; break;
		case QUARTER_LENGTH      : len = 4; break;
		case NOTE8_LENGTH        : len = 8; break;
		case NOTE16_LENGTH       : len = 1; break;
		case NOTE32_LENGTH       : len = 3; break;
		case NOTE64_LENGTH       : len = 6; break;
		case NOTE128_LENGTH      : len = 6; break;
	}

	return len;
}

QString NPmxExport::computePMXTupletLength(int length, int staff, int measure) {
	badmeasure *bad;
	QString s("4");

	switch (length) {
		case DOUBLE_WHOLE_LENGTH : s = "9"; break;
		case DOUBLE_WHOLE_LENGTH*3/2 : s = "9d"; break;
		case WHOLE_LENGTH        : s = "0"; break;
		case WHOLE_LENGTH*3/2    : s = "0d"; break;
		case HALF_LENGTH         : s = "2"; break;
		case HALF_LENGTH*3/2     : s = "2d"; break;
		case QUARTER_LENGTH      : s = "4"; break;
		case QUARTER_LENGTH*3/2  : s = "4d"; break;
		case NOTE8_LENGTH        : s = "8"; break;
		case NOTE8_LENGTH*3/2    : s = "8d"; break;
		case NOTE16_LENGTH       : s = "1"; break;
		case NOTE16_LENGTH*3/2   : s = "1d"; break;
		case NOTE32_LENGTH       : s = "3"; break;
		case NOTE32_LENGTH*3/2   : s = "3d"; break;
		case NOTE64_LENGTH       : s = "6"; break;
		case NOTE64_LENGTH*3/2   : s = "6d"; break;
		case NOTE128_LENGTH      : s = "6"; break;
		case NOTE128_LENGTH*3/2  : s = "6d"; break;
		default: bad = new badmeasure(PMX_TUPLET_LENGTH, staff, measure, 3 /*dummy */,  128 /*dummy */);
			 badlist_.append(bad);
			 break;
	}
	return s;
}

void NPmxExport::handleSpecialChar(int elempos, QString *specialChar, int voice_count, bool force_output)  {
	if (voice_count < 2 || force_output) {
		*pmxout_ << (*specialChar);
	}
	if (voice_count > 1) {
		specialCharList_.append(new specialCharInfo(specialChar, elempos));
	}
}

void NPmxExport::checkSpecialChar(int newxpos) {
	specialCharInfo *specInf;

	specInf = specialCharList_.first();
	while (specInf) {
		if (specInf->xpos <= newxpos) {
			*pmxout_ << (*(specInf->specInfo));
			specialCharList_.remove();
			specInf = specialCharList_.current();
		}
		else {
			specInf = specialCharList_.next();
		}
	}
}

void NPmxExport::setTie(NNote *note, int staff_nr, int barnr) {
	NNote *tieMember;
	badmeasure *bad;
	int i;
	bool found = false;

	if ((note->status & STAT_TIED) && !(note->status & STAT_PART_OF_TIE)) { 
		for (i = 0; i < MAXTIES && !found; i++) {
			found = !((1 << i) & tiePool_);
		}
		if (found) {
			i--;
			*pmxout_ << "s" << i << ' ';
			tiePool_ |= (1 << i);
		}
		else {
			i = -1;
			bad = new badmeasure(PMX_ERR_TOO_MANY_TIES, staff_nr, barnr, 0 , 0);
                        badlist_.append(bad); 
		}
		for (tieMember = note; tieMember; tieMember = tieMember->tie_forward) {
			tieMember->TeXTieNr = i;
		}
	}
	else if ((note->status & STAT_PART_OF_TIE) && (note->status & STAT_TIED)) {
		if (note->TeXTieNr >= 0) {
			*pmxout_ << "s" << note->TeXTieNr << ' ';
			*pmxout_ << "s" << note->TeXTieNr << ' ';
		}
	}
	else if ((note->status & STAT_PART_OF_TIE) && !(note->status & STAT_TIED)) {
		if (note->TeXTieNr >= 0) {
			*pmxout_ << "s" << note->TeXTieNr << ' ';
			tiePool_ = tiePool_ & (~(1 << note->TeXTieNr));
		}
	}
}

void NPmxExport::setSlur(NChord *chord, int staff_nr, int barnr) {
	badmeasure *bad;
	int i;
	bool found = false;

	if (chord->status_ & STAT_GRACE) {
		if ((chord->status_ & STAT_SLURED)) {
			chord->setPartnerSlurNr(-1);
		}
		return;
	}
	if ((chord->status_ & STAT_SLURED) && !(chord->status_ & STAT_PART_OF_SLUR)) { 
		for (i = 0; i < MAXSLURS && !found; i++) {
			found = !((1 << i) & slurPool_);
		}
		if (found) {
			i--;
			*pmxout_ << "s" << i << ' ';
			slurPool_ |= (1 << i);
		}
		else {
			i = -1;
			bad = new badmeasure(PMX_ERR_TOO_MANY_SLURS, staff_nr, barnr, 0 , 0);
                        badlist_.append(bad); 
		}
		chord->setTeXSlurNr(i);
		chord->setPartnerSlurNr(i);
	}
	else if ((chord->status_ & STAT_PART_OF_SLUR) && (chord->status_ & STAT_SLURED)) {
		if (chord->getTeXSlurNr() >= 0) {
			*pmxout_ << "s" << chord->getTeXSlurNr() << ' ';
			*pmxout_ << "s" << chord->getTeXSlurNr() << ' ';
			chord->setPartnerSlurNr(chord->getTeXSlurNr());
		}
	}
	else if ((chord->status_ & STAT_PART_OF_SLUR) && ! (chord->status_ & STAT_SLURED)) {
		if (chord->getTeXSlurNr() >= 0) {
			*pmxout_ << "s" << chord->getTeXSlurNr() << ' ';
			slurPool_ = slurPool_ & (~(1 << chord->getTeXSlurNr()));
		}
	}
}

#if GCC_MAJ_VERS > 2
void NPmxExport::lineOut(ostringstream *outstream) {
#else
void NPmxExport::lineOut(ostrstream *outstream) {
#endif
	char  buf[256];
	const char *cptr;
	int i, n, count;

	*outstream << '\0';
#if GCC_MAJ_VERS > 2
	count = outstream->tellp();
	cptr = outstream->str().c_str();
#else
	count = outstream->pcount();
	cptr = outstream->str();
#endif
	n = 0;
	do {
		for (i = 0; n < count && i < MAX_LINE_CHARS; i++, n++, cptr++) {
			if (i > MAX_LINE_CHARS && *cptr == ' ') break;
			buf[i] = *cptr;
		}
		if (i >= MAX_LINE_CHARS) {
			do {
				i--; cptr--; n--;
			}
			while (*cptr != ' ');
		}
		buf[i] = '\0';
		out_  << buf << endl;
	}
	while (n < count);
}
	
bool NPmxExport::testContextChange(int voice_nr, NVoice *voice, bool first) {
	NMusElement *elem;
	bool found, specialfound = false;

	if (voice_nr != 0) return false;
	if (pendingEndSpecialEnd_) {
		//elem = voice->getNextPosition();
		return true;
	}
	if (openSpecialEnding_) {
		if (first) {
			if (--openSpecialEnding_ == 0) {
				pendingEndSpecialEnd_ = true;
				elem = voice->getCurrentPosition();
				if (elem) posOfpendingSpecialEnd_ = elem->getXpos();
				else posOfpendingSpecialEnd_ = voice->getLast()->getXpos();
			}
		}
		return true;
	}
	elem = voice->getNextPosition();
	if (!elem) return false;

	do {
		found = false;
		if (!elem) return false;
		switch(elem->getType()) {
			case T_TIMESIG:
				pendingTimeSig_ = (NTimeSig *) elem;
				posOfpendingTimeSig_ = elem->getXpos();
				found = true;
				specialfound = true;
				break;
			case T_KEYSIG:
				pendingKeySig_ = (NKeySig *) elem;
				posOfpendingKeySig_ = elem->getXpos();
				found = true;
				specialfound = true;
			case T_SIGN:
				switch (elem->getSubType()) {
					case SPECIAL_ENDING1: pendingSpecialEnd_ = 1;
							      posOfpendingSpecialEnd_ = elem->getXpos();
								found = true;
								specialfound = true;
								break;
					case SPECIAL_ENDING2: pendingSpecialEnd_ = 2;
							      posOfpendingSpecialEnd_ = elem->getXpos();
								found = true;
								specialfound = true;
								break;
				}
				break;
		}
		if (found) elem = voice->getNextPosition();
	}
	while (found);
	voice->getPrevPosition();
	if (specialfound) return true;
	return false;
}
		
void NPmxExport::inspectTuplet(NMusElement *elem, int staff_nr, int barnr_) {
	QList<NMusElement> *tupletlist;
	badmeasure *bad;
	int len1, len2;
	NMusElement *elem2;
	bool len2set = false;

	tupletlist = elem->getTupletList();

	if (tupletlist->count() != elem->getNumNotes()) {
		elem2 = tupletlist->first();
		len1 = len2 = elem2->getSubType();
		for (elem2 = tupletlist->next(); elem2; elem2 = tupletlist->next()) {
			if (elem2->getSubType() != len1) {
				if (len2set) {
					if (elem2->getSubType() != len2) {
						bad = new badmeasure(PMX_ERR_NOT_NUM_TUMPLET, staff_nr, barnr_, 0, 0);
						badlist_.append(bad);
					}
				}
				else {
					len2set = true;
					len2 = elem2->getSubType();
				}
			}
		}
		if (len2set) {
			if (len1 != 2*len2 && len2 != 2*len1) {
				bad = new badmeasure(PMX_ERR_NOT_NUM_TUMPLET, staff_nr, barnr_, 0, 0);
				badlist_.append(bad);
			}
			if (len1 < len2)  {
				tupletBase_ = len1;
			}
			else {
				tupletBase_ = len2;
			}
		}
		else {
			tupletBase_ = len1;
		}
						
	}
	else {
		tupletBase_ = tupletlist->getFirst()->getSubType();
	}
	if (tupletlist->getLast()->getType() == T_REST) {
		bad = new badmeasure(PMX_ERR_TUPLET_ENDS_REST, staff_nr, barnr_, 0, 0);
		badlist_.append(bad);
	}
}

QString NPmxExport::lyrics2TeX(QString *lyrics) {
	QString ret;
	QRegExp reg;

	reg = QRegExp("^ *[-\\*] *$");
	if (ret.find(reg) != -1) {
		return QString("");
	}
	ret = QString(*lyrics);
	NResource::germanUmlautsToTeX(&ret);
	reg = QRegExp("_");
	ret.replace (reg, "\\_");
	if (exportDialog_->pmxMLyr->isChecked()) {
		reg = QRegExp("<");
		ret.replace (reg, "{");
		reg = QRegExp(">");
		ret.replace (reg, "}");
	}
	else {
		reg = QRegExp("[<>]");
		ret.replace (reg, "");
	}
	return ret;
}
