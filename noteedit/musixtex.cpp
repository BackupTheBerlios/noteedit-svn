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
#include <iostream>
#include <sstream>
#else
#include <iostream.h>
#include <strstream.h>
#endif
#include <stdlib.h> //  esigra: needed for mkstemp when --without-libs
#include <alloca.h>
#include <kfiledialog.h>
#include <alloca.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <ktextbrowser.h>
#include <qregexp.h>
#include <qspinbox.h>
#include <qcheckbox.h>
#include <qslider.h>
#include <qlineedit.h>
//#include <qtextedit.h> QT2 inccompatible
#include <qfile.h>
#include <qtextstream.h>
#include <unistd.h>
#include <string.h>
#include "musixtex.h"
#include "mainframewidget.h"
#include "resource.h"
#include "clef.h"
#include "keysig.h"
#include "timesig.h"
#include "sign.h"
#include "rest.h"
#include "voice.h"
#include "staff.h"
#include "uiconnect.h"
#include "chord.h"
#include "chorddiagram.h"
#include "outputbox.h"
#include "text.h"
#include "musixhint.h"
#include "multistaffinfo.h"
#include "layout.h"

#define TEX_FAC 0.08
#define MAXSTAFFSANDBEAMS 9
#define MAXFLAGS 5


NMusiXTeX::NMusiXTeX() {
	shiftes_notes_.setAutoDelete(false);
	non_shifted_notes_.setAutoDelete(false);
	pending_key_changes_.setAutoDelete(true);
	pending_time_changes_.setAutoDelete(true);
	pending_texts_.setAutoDelete(true);
}


void NMusiXTeX::exportStaffs(QString filen, QList<NStaff> *stafflist, exportFrm *form, NMainFrameWidget *mainWidget) {
	mainWidget_ = mainWidget;
	staffList_ = stafflist;
	staffCount_ = stafflist->count();
	exportDialog_ = form;
	fileName = filen;
	this->doExport();
}


void NMusiXTeX::doExport() {
	int i, j, k,maxlyrics;
	NExportError *exerr;
	NExportError *errptr;
	NPositStr *posit;
	QList<NPositStr> plist;
	char *notesString;
	bool withLyrics;
#if GCC_MAJ_VERS > 2
	ostringstream os;
#else
	char buffer[128+1];
	ostrstream os(buffer, 100);
#endif
	NStaff *staff_elem;
	NVoice *voice_elem;
	QList<QString> lyrNames;
	QString *lyrName;
	int myTime_ = 0;
	int num_positions, maxwidth, min_time = (1 << 30);
	int minlength;
	int width;
	bool only_playables, staff_placed_elements, grace_notes;
	NKeySig *ksig;
	NTimeSig *timesig;
	QString lyricslist[NUM_LYRICS];
	QString *staffname;
	int count_of_lyrics;
	int kind, z;
	bool lyrcsLineCounts[NUM_LYRICS];
	bool somethingProduced;
	bool tex_special = false;
	bool generalmeter_set;
	int voice_count;
	int staffs_to_export;
	int real_staff_nr, staff_nr;
	int dummy, multistaffnr;
	int rst_nr;
	int numOfStaffsInMultistaff;
	int idx;

	slurTiePool_ = 0x0;
	trillPool_ = 0x0;
	vaPool_ = 0x0;
	beamPool_ = 0x0;
	barNr_ = 2;
	maxTies_ = 6;
	maxSlurs_ = 6;
	spare_ = false;
	newTempo_ = -1;
	limitMeasures_ = exportDialog_->texMeasures_->isChecked();
	if (limitMeasures_) {
		maxMeasuresPerLine_ = exportDialog_->measureVal->value();
	}
	if (NResource::staffSelExport_ == 0) {
		NResource::staffSelExport_ = new bool[staffCount_];
		for (i = 0; i < (int) staffCount_; NResource::staffSelExport_[i++] = true);
		staffs_to_export = staffCount_;
	}
	else {
		staffs_to_export = 0;
		for (i = 0; i < (int) staffCount_; i++) {
			if (NResource::staffSelExport_[i]) {
				staffs_to_export++;
			}
		}
	}
	va_descr_ = (struct trill_descr_str *) alloca(staffs_to_export * sizeof(struct trill_descr_str));
	trill_descr_ = (struct trill_descr_str *) alloca(staffs_to_export * sizeof(struct trill_descr_str));
	mStaffInf_ = new NMultistaffInfo(mainWidget_, staffList_, staffCount_);
	if (staffs_to_export > (int) MAXSTAFFSANDBEAMS) {
		os << "There are more than " << MAXSTAFFSANDBEAMS << "Staffs (maximum = " << MAXSTAFFSANDBEAMS << "). Please select some !" << '\0';
#if GCC_MAJ_VERS > 2
		KMessageBox::error(0, QString(os.str().c_str()), kapp->makeStdCaption(i18n("???")));
#else
		KMessageBox::error(0, QString(os.str()), kapp->makeStdCaption(i18n("???")));
#endif
		return;
	}
	dysymDescr_ = (struct dynamics_descr_str *) alloca(staffs_to_export * sizeof(struct dynamics_descr_str));
	for (i = 0; i < staffs_to_export; i++) {
		trill_descr_[i].trill_nr = -1;
		va_descr_[i].trill_nr = -1;
		dysymDescr_[i].lastDynSym = 0;
		dysymDescr_[i].dynEndPos = 0;
	}
	pending_texts_.clear();
	if (fileName.isNull())
		return;
	out_.open(fileName);
	if (!out_) {
		os << "error opening file " << fileName << '\0';
#if GCC_MAJ_VERS > 2
		KMessageBox::sorry(0, QString(os.str().c_str()), kapp->makeStdCaption(i18n("???")));
#else
		KMessageBox::sorry(0, QString(os.str()), kapp->makeStdCaption(i18n("???")));
#endif
		return;
	}
	lastBarNr_ = 0;
	for (i = 0; i < NUM_LYRICS; i++) {
		lyrcsLineCounts[i] = false;
	}
	for (i = 0, staff_elem = staffList_->first(); staff_elem; staff_elem = staffList_->next(), i++) {
		voice_elem = staff_elem->getVoiceNr(0);
		if (voice_elem->getLastBarNr() > lastBarNr_) lastBarNr_ = voice_elem->getLastBarNr();
		maxlyrics = voice_elem->countOfLyricsLines();
		if (maxlyrics < 0 || maxlyrics > NUM_LYRICS) {
			os << "internal error: maxlyrics = " << maxlyrics << '\0';
#if GCC_MAJ_VERS > 2
			KMessageBox::error(0, QString(os.str().c_str()), kapp->makeStdCaption(i18n("???")));
#else
			KMessageBox::error(0, QString(os.str()), kapp->makeStdCaption(i18n("???")));
#endif
			return;
		}
		if (maxlyrics) lyrcsLineCounts[maxlyrics-1] = true;
	}
	badMeasures_.setAutoDelete(true);
	badMeasures_.clear();
	if (mStaffInf_->hasDisconnectedPianoBars()) {
		exerr = new NExportError(MUSIXTEX_ERR_DISCONT_PIANO, 1 /* dummy */);
		badMeasures_.append(exerr);
	}
	out_ << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
	out_ << "%                                  %" << endl;
	out_ << "% MusiXTeX output generated by     %" << endl;
	out_ << "%       \"noteedit\"                 %" << endl;
	out_ << "%                                  %" << endl;
	out_ << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl << endl;
	out_ << "\\documentclass{article}"  << endl;
	out_ << "\\usepackage{musixtex}" << endl << endl;

	switch(exportDialog_->texOutputEncoding->currentItem()) {
		case 1: out_ << "\\usepackage[utf8]{inputenc}" << endl; break;
	}
	switch(exportDialog_->texFontModule->currentItem()) {
		case 1: out_ << "\\usepackage[T2A]{fontenc}" << endl; break;
		case 2: out_ << "\\usepackage[T2B]{fontenc}" << endl; break;
		case 3: out_ << "\\usepackage[T2C]{fontenc}" << endl; break;
		case 4: out_ << "\\usepackage[OT1]{fontenc}" << endl; break;
		case 5: out_ << "\\usepackage[OT2]{fontenc}" << endl; break;
		case 6: out_ << "\\usepackage[X2]{fontenc}" << endl; break;
		case 7: out_ << "\\usepackage[LCY]{fontenc}" << endl; break;
	}
	switch(exportDialog_->texInputEncoding->currentItem()) {
		case 1: out_ << "\\usepackage[latin1]{inputenc}" << endl; break;
		case 2: out_ << "\\usepackage[cp1251]{inputenc}" << endl; break;
		case 3: out_ << "\\usepackage[koi8-u]{fontenc}" << endl; break;
		case 4: out_ << "\\usepackage[koi8-r]{fontenc}" << endl; break;
	}
	if (exportDialog_->texUcs->isChecked()) {
		out_ << "\\usepackage{ucs}" << endl;
	}
	out_ << "\\input musixper.tex" << endl;
	out_ << "\\input musixlit.tex" << endl;
	out_ << "\\input musixadd.tex" << endl;
	out_ << "\\input musixbm.tex"  << endl;

	if (mStaffInf_->DiscontOutsidePiano()) {
		exerr = new NExportError(MUSIXTEX_ERR_INDIV_BAR, 1 /* dummy */);
		badMeasures_.append(exerr);
		out_ << "\\input musixdbr.tex" << endl;
	}
	if (exportDialog_->texMLyr->isChecked()) {
		out_ << "\\input musixlyr.tex"  << endl;
	}
	if (exportDialog_->texOmitPageNumbering->isChecked()) {
	               out_ << endl << "\\pagestyle{empty}" << endl << endl;
	}
	out_ << "\\input musixgui.tex" << endl << endl;
	out_ << "\\setlength{\\parindent}{" << exportDialog_->texParindent->text() << "mm}" << endl;
	out_ << "\\setlength{\\textwidth}{" << exportDialog_->texWidth->text() << "mm}" << endl;
	out_ << "\\setlength{\\textheight}{" << exportDialog_->texHeight->text() << "mm}" << endl;
	out_ << "\\setlength{\\topmargin}{" << exportDialog_->texTop->text() << "mm}" << endl;
	out_ << "\\setlength{\\oddsidemargin}{" << exportDialog_->texLeft->text() << "mm}" << endl << endl;
	switch (exportDialog_->texSize->currentItem()) {
		case 0: out_ << "\\smallmusicsize" << endl; break;
		case 2: out_ << "\\largemusicsize" << endl; break;
		case 3: out_ << "\\Largemusicsize" << endl; break;
	}
	if (!exportDialog_->texBar->isChecked()) {
		out_ << "\\nobarnumbers" << endl;
	}

	out_ << "\\raiseguitar{20}" << endl;

	spare_ = exportDialog_->texTies->isChecked();
	out_ << "\\begin{document}" << endl << endl;

	lyrNames.setAutoDelete(true);
	k = 0;
	for (i = 0, staff_elem = staffList_->first(); exportDialog_->texMLyr->isChecked() && staff_elem; staff_elem = staffList_->next(), i++) {
		if (!NResource::staffSelExport_[i]) continue;
		voice_elem = staff_elem->getVoiceNr(0);
		voice_elem->setIdx(i);
		if (!(count_of_lyrics = voice_elem->countOfLyricsLines())) continue;
		voice_elem->collectLyrics(lyricslist);
		lyrNames.clear();
		for (j = 0; j < NUM_LYRICS; j++) {
			if (!lyricslist[j].isEmpty()) {
				lyrName = new QString();
				multistaffnr = mStaffInf_->multistaffIdxOfStaff(i, &dummy, &dummy);
				if (count_of_lyrics < 2) {
					lyrName->sprintf("lyrstaff%d", mStaffInf_->getMultiStaffCount() - multistaffnr);
				}
				else {
					lyrName->sprintf("lyrstaff%dverse%d", mStaffInf_->getMultiStaffCount() - multistaffnr, j+1);
				}
				lyrNames.append(lyrName);
				out_ << "\\setlyrics{" << (*lyrName) << "}{" << lyrics2TeX(&(lyricslist[j])) << "}" << endl;
			}
		}

		out_ << "\\assignlyrics{" << (mStaffInf_->getMultiStaffCount() - multistaffnr) << "}{";
		while(!lyrNames.isEmpty()) {
			out_ << *(lyrNames.first());
			lyrNames.remove();
			if (!lyrNames.isEmpty()) out_ << ',';
		}
		out_ << '}' << endl << endl;
	}


	if (!mainWidget_->scTitle_.isEmpty()) {
		if (!tex_special) {
			tex_special = true;
			out_ << "\\begin{center}" << endl;
		}
		out_ << "{\\Large\\bf " << lyrics2TeX(&(mainWidget_->scTitle_)) << "}\\\\" << endl;
	}
	if (!mainWidget_->scSubtitle_.isEmpty()) {
		if (!tex_special) {
			tex_special = true;
			out_ << "\\begin{center}" << endl;
		}
		out_ << "{\\Large " << lyrics2TeX(&(mainWidget_->scSubtitle_)) << "}\\\\" << endl;
	}
	if (tex_special) {
		out_ << "\\end{center}" << endl << endl;
		tex_special = false;
	}
	if (!mainWidget_->scAuthor_.isEmpty()) {
		if (!tex_special) {
			tex_special = true;
			out_ << "\\begin{flushright}" << endl;
		}
		out_ << "{\\Large\\sf " << lyrics2TeX(&(mainWidget_->scAuthor_)) << "}\\\\" << endl;
	}
	if (!mainWidget_->scLastAuthor_.isEmpty()) {
		if (!tex_special) {
			tex_special = true;
			out_ << "\\begin{flushright}" << endl;
			out_ << "{\\large\\sf " <<  lyrics2TeX(&(mainWidget_->scLastAuthor_)) << "}\\\\" << endl;
		}
	}
	if (!mainWidget_->scCopyright_.isEmpty()) {
		if (!tex_special) {
			tex_special = true;
			out_ << "\\begin{flushright}" << endl;
		}
		out_ << "{\\large\\sf " <<  lyrics2TeX(&(mainWidget_->scCopyright_)) << "}\\\\" << endl;
	}
	if (tex_special) {
		out_ << "\\end{flushright}" << endl << endl;
		tex_special = false;
	}
	out_ << "\\instrumentnumber{" << mStaffInf_->getMultiStaffCount() << "}" << endl;
	if (!mStaffInf_->ContinuedBarLines()) {
		if (mStaffInf_->DiscontOutsidePiano()) {
			out_ << "\\indivbarrules" << endl << "\\allbarrules\\sepbarrule";
			for (i = 0; i < staffCount_; i++) {
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
			out_ << "\\sepbarrules";
		}
		out_ << endl;
	}
	for (i = 1; i <= mStaffInf_->getMultiStaffCount(); i++) {
		if (mStaffInf_->getStaffCount(mStaffInf_->getMultiStaffCount()-i) > 1) {
			out_ << "\\setstaffs{" << i << "}{" << mStaffInf_->getStaffCount(mStaffInf_->getMultiStaffCount()-i) << '}' << endl;
		}
	}
	generalmeter_set = false;
	for (i = 1; i <= mStaffInf_->getMultiStaffCount(); i++) {
		out_ <<  mStaffInf_->computeTexClef(mStaffInf_->getMultiStaffCount()-i) << endl;
		if ((staff_elem = staffList_->at(mStaffInf_->getfirstStaffInMultistaff(i-1))) == 0)  {
			NResource::abort("NMusiXTeX::doExport :internal error", 1);
		}
		if (i < mStaffInf_->getMultiStaffCount() ) {
			out_ << "\\setinterinstrument{" << (mStaffInf_->getMultiStaffCount() - i) << "}{" << 
				TEX_FAC * staff_elem->underlength_ << "mm}" << endl;
		}
		else {
			out_ << "\\staffbotmarg=" << TEX_FAC * staff_elem->underlength_ << "mm%" << endl;
		}
		ksig = getKeySig(mStaffInf_->getMultiStaffCount()-i);
		if (ksig) {
			if (ksig->isRegular(&kind, &z)) {
				if (z != 0) {
					out_ << "\\setsign{" << i << "}{" << ((kind == STAT_CROSS) ? z : -z) << "}" << endl;
				}
			}
			else {
				exerr = new NExportError(MUSIXTEX_ERR_IRR_KEY, staffCount_ - i);
				badMeasures_.append(exerr);
			}
		}
		timesig = getTimeSig(mStaffInf_->getMultiStaffCount()-i);
		if (timesig) {
#define USE_GENERAL_METER
#ifdef USE_GENERAL_METER
			if (!generalmeter_set) {
				out_ << "\\generalmeter{{\\meterfrac{" << timesig->getNumerator() <<
					"}{" << timesig->getDenominator() << "}}}" << endl;
				generalmeter_set = true;
			}
#else
			out_ << "\\setmeter{" << i << "}{{\\meterfrac{" << timesig->getNumerator() <<
				"}{" << timesig->getDenominator() << "}}}" << endl;
#endif
		}
		staffname = getStaffName(mStaffInf_->getMultiStaffCount()-i);
		if (!staffname->isEmpty()) {
			out_ << "\\setname{" << i << "}{" << lyrics2TeX(staffname) << "}" << endl;
		}
	}
	mStaffInf_->writeAkkoladen(&out_, false);
	k = 0;
	for (i = 0, staff_elem = staffList_->last(); staff_elem; staff_elem = staffList_->prev(), i++) {
		if (NResource::staffSelExport_[staffCount_ - i - 1]) {
			k++;
			voice_elem = staff_elem->getVoiceNr(0);
			voice_elem->setIdx(staffs_to_export -  k);
			voice_count = staff_elem->voiceCount();
			for (j = 0; j < voice_count; j++) {
				voice_elem = staff_elem->getVoiceNr(j);
				voice_elem->startRepositioning();
			}
		}
	}
	out_ << "\\startpiece" << endl;

	while(1) {
		plist.clear();
		withLyrics = false;
		num_positions = 0;
		min_time = (1 << 30);
		for (i = 0, staff_elem = staffList_->first(); staff_elem; staff_elem = staffList_->next(), i++) {
			if (!NResource::staffSelExport_[i]) continue;
			voice_count = staff_elem->voiceCount();
			for (j = voice_count - 1; j >= 0; j--) {
				voice_elem = staff_elem->getVoiceNr(j);
				posit = voice_elem->getElementAfter(myTime_);
				if (posit) {
					plist.append(posit); 
					num_positions++;
				}
				if (posit) {
					if (posit->ev_time < min_time) min_time = posit->ev_time;
					if (posit->elem->getType() == T_CHORD) {
						for (k = 0; k < NUM_LYRICS; k++) {
							if (((NChord *) posit->elem)->getLyrics(k)) withLyrics = true;
						}
					}
			
				}
			}
		}
		if (!num_positions) {
			break;
		}
		only_playables = true;
		noteSizeIsNormal_ = true;
		grace_notes = false;
		staff_placed_elements = false;
		maxwidth = 0;
		minlength = WHOLE_LENGTH;
		for (posit = plist.first(); posit; posit = plist.next()) {
			if (posit->ev_time != min_time) continue;
			if (!(posit->ev_type & PLAYABLE)) only_playables = false;
			width = posit->elem->getBbox()->width();
			if (width > maxwidth) maxwidth = width;
			switch(posit->elem->getType()) {
			case T_CHORD:
				if (posit->ev_type & STAT_GRACE) grace_notes = true;
				break;
			case T_SIGN:
				switch (posit->elem->getSubType()) {
					case VOLUME_SIG:
					case SEGNO:
					case DAL_SEGNO_AL_FINE:
					case DAL_SEGNO_AL_CODA:
					case FINE:
					case CODA:
					case DAL_SEGNO: staff_placed_elements = true;
							break;
				}
				break;
			}
			if (posit->ev_type & PLAYABLE) {
				if (posit->elem->getMidiLength() < minlength) minlength = posit->elem->getMidiLength();
			}
		}
		if (!exportDialog_->texMLyr->isChecked() && withLyrics) {
			if (maxwidth < 12) notesString = "\\notes";
			else if (maxwidth < 14) notesString = "\\notesp";
			else if (maxwidth < 16) notesString = "\\Notes";
			else if (maxwidth < 18) notesString = "\\Notesp";
			else if (maxwidth < 20) notesString = "\\NOtes";
			else if (maxwidth < 22) notesString = "\\NOtesp";
			else if (maxwidth < 24) notesString = "\\NOTes";
			else if (maxwidth < 26) notesString = "\\NOTesp";
			else notesString = "\\NOTEs";
		}
		else {
			if (grace_notes) notesString = "\\notes";
			else if (minlength >= HALF_LENGTH*3/2) notesString = "\\NOTEs";
			else if (minlength >= HALF_LENGTH) notesString = "\\NOTes";
			else if (minlength >= QUARTER_LENGTH*3/2) notesString = "\\NOtesp";
			else if (minlength >= QUARTER_LENGTH) notesString = "\\NOtes";
			else if (minlength >= NOTE8_LENGTH*3/2) notesString = "\\Notesp";
			else if (minlength >= NOTE8_LENGTH) notesString = "\\Notes";
			else if (minlength >= NOTE16_LENGTH*3/2) notesString = "\\notesp";
			else notesString = "\\notes";
		}

		somethingProduced = false;
		staff_nr = staffs_to_export - 1;
		if (only_playables || staff_placed_elements || grace_notes) {
			writeContextChange();
			out_ << notesString;
		}
		for (posit = plist.last(); posit; posit = plist.prev()) {
			staff_elem = posit->from->getStaff();
			k = staff_elem->getVoiceNr(0)->getIdx();
			if (k != staff_nr) {
				somethingProduced = false;
				if (staff_nr < k) {
					NResource::abort("NMusiXTeX::doExport :internal error", 2);
				}
				rst_nr = real_staff_nr;
				while (staff_nr > k) {
					if (only_playables || staff_placed_elements || grace_notes) {
						if (mStaffInf_->multistaffIdxOfStaff(rst_nr, &dummy, &dummy) > 0) {
							out_ << '|';
						}
						else {
							out_ << "&";
						}
					}
					staff_nr--;
					rst_nr--;
				}
			}
			if ((real_staff_nr = staffList_->find(staff_elem)) == -1) {
					NResource::abort("NMusiXTeX::doExport :internal error", 3);
			}
			if (only_playables) {
				if (posit->ev_time == min_time)  {
					posit->from->skipTeXChord();
					generate(staff_nr, real_staff_nr, somethingProduced ? "\\bsk" : 0, posit->elem, staff_elem, posit->from);
					somethingProduced = true;
					delete posit;
				}
			}
			else {
				if (posit->ev_time == min_time && !(posit->ev_type & PLAYABLE))   {
					posit->from->skipTeXChord();
					generate(staff_nr, real_staff_nr, somethingProduced ? "\\bsk" : 0, posit->elem, staff_elem, posit->from);
					somethingProduced = true;
				}
			}
		}
		if (staff_nr < 0) {
			NResource::abort("NMusiXTeX::doExport :internal error", 4);
		}
		if (only_playables || staff_placed_elements || grace_notes ) {
			rst_nr = real_staff_nr;
			while (staff_nr > 0) {
				if (mStaffInf_->multistaffIdxOfStaff(rst_nr, &dummy, &dummy) > 0) {
					out_ << '|';
				}
				else {
					out_ << "&";
				}
				staff_nr--;
				rst_nr--;
			}
			out_ << "\\en" << endl;
		}
		if (only_playables) {
			myTime_ = min_time+1;
		}
	}

	if (lastSignWasBar_) {
		out_.seekp(lastBarPos_);
	}

	if (lastSignWasRepeatClose_) {
		out_ << "\\stoppiece" << endl << endl;
	}
	else {
		out_ << "\\Endpiece" << endl << endl;
	}
	out_ << "\\end{document}" << endl;
	out_.close();
	if (NResource::staffSelExport_) {
		delete [] NResource::staffSelExport_;
		NResource::staffSelExport_ = 0;
	}
	if (!badMeasures_.isEmpty()) {
		QString output;
		output = i18n
			("Noteedit has exported the score to MusiXTeX but there are some\n"
			 "constructs which cannot be expressed in MusiXTeX. The score will\n"
			 "work with MusiXTeX but Noteedit omitted these constructs.\n");
		output += i18n("-----------------------------------------------------\n");
		for (errptr = badMeasures_.first(); errptr; errptr = badMeasures_.next()) {
			switch (errptr->reason) {
				case MUSIXTEX_ERR_128:
				output += i18n
					("128th in measure: %2; maximum = %3 flags\n").
					arg(errptr->measure).arg(4);
					break;
				case MUSIXTEX_ERR_IRR_KEY:
				output += i18n
					("Staff %1 has irregular keysig. This cannot be expressed in MusiXTeX\n").arg(errptr->measure);
					break;
				case MUSIXTEX_ERR_TOO_MANY_TIES:
				output += i18n
					("Too many ties in measure: %2; maximum = %3\n").arg(errptr->measure).arg(maxTies_);
					break;
				case MUSIXTEX_ERR_TOO_MANY_SLURS:
				output += i18n
					("Too many slurs in measure: %1, ; maximum = %2\n").arg(errptr->measure).arg(maxSlurs_);
					break;
				case MUSIXTEX_ERR_TOO_MANY_TRILLS:
				output += i18n
					("Too many trills in measure: %1: maximum = 6\n").arg(errptr->measure);
					break;
				case MUSIXTEX_ERR_NESTED_TILLS:
				output += i18n
					("Nested va lines in measure: %2 ;\n").arg(errptr->measure);
					break;
				case MUSIXTEX_ERR_TOO_MANY_VAS:
				output += i18n
					("Too many va lines in measure: %1: maximum = 6\n").arg(errptr->measure);
					break;
				case MUSIXTEX_ERR_NESTED_VAS:
				output += i18n
					("Nested va lines in measure: %1 ;\n").arg(errptr->measure);
					break;
				case MUSIXTEX_ERR_BEAM_COUNT:
				output += i18n
					("too many beams in measure: %1; maximum: %2\n").arg(errptr->measure).arg(MAXSTAFFSANDBEAMS);
					break;
				case MUSIXTEX_ERR_INDIV_BAR:
				output += i18n
					("The score has individual bar layout (partial continued and partial discontinued)\nPlease install \"musixdbr.tex\" by  Rainer Dunker\n");
					break;
				case MUSIXTEX_ERR_DISCONT_PIANO:
				output += i18n
					("The score has piano staffs with discontinued bar rules. This cannot be expressed in MusiXTeX.\n");
					break;
			}
		}
		if (NResource::commandLine_) {
			cerr << "MusiXTeX produced. But there are some problems" << endl;
			cerr << output << endl;
		}
		else {
			NResource::exportWarning_->setOutput(i18n ("MusiXTeX produced. But there are some problems."), &output);
#if QT_VERSION >= 300
			NResource::exportWarning_->exec();
#else
			NResource::exportWarning_->show();
#endif
		}
		
	}
	// do musixtex-postprocessing if desired
  	if (exportDialog_->musixtexcmd->text().isEmpty() || exportDialog_->musixtexcmd->text().isNull()) {
	  NResource::musixScript_.truncate(0);
	}
	else {
	  externalCmd(exportDialog_->musixtexcmd->text(),fileName);
	  NResource::musixScript_ = exportDialog_->musixtexcmd->text();
  	}
	if (NResource::musixWarn_) {
		NResource::musixHint_->show();
	}
	
}

void NMusiXTeX::writeContextChange() {
	bool pendingTimeSignature;
	pending_context_change_class *pending_context_change;
	int multistaffnr;
	int i, dummy;
	bool newclef;

	pendingTimeSignature = false;
	if (!pending_time_changes_.isEmpty()) {
		out_.seekp(lastBarPos_);
		pendingTimeSignature = true;
		while (!pending_time_changes_.isEmpty()) {
			pending_context_change = pending_time_changes_.first();
			if (mStaffInf_->multistaffIdxOfStaff(pending_context_change->real_staff_nr, &multistaffnr, &dummy) == 0) {
				out_ << "\\setmeter{" << multistaffnr << "}{{\\meterfrac{" << pending_context_change->kind <<
				"}{" << pending_context_change->denom  << "}}}%" << endl;
			}
			pending_time_changes_.remove();
		}
	}
			
	newclef = false;
	for (i = 0; i < mStaffInf_->getMultiStaffCount() ;i++) {
		if (mStaffInf_->clefChanged(i, true)) {
			out_ << mStaffInf_->computeTexClef(i) << '%' << endl;
			newclef = true;
		}
	}
	if (newclef && !pendingTimeSignature) {
		out_ << "\\changeclefs%" << endl;
	}
	if (!pending_key_changes_.isEmpty()) {
		while (!pending_key_changes_.isEmpty()) {
			pending_context_change = pending_key_changes_.first();
			if (mStaffInf_->multistaffIdxOfStaff(pending_context_change->real_staff_nr, &multistaffnr, &dummy) == 0) {
				out_ << "\\setsign{" << (mStaffInf_->getMultiStaffCount()-pending_context_change->real_staff_nr) << "}{" << pending_context_change->kind << "}%" << endl;
			}
			pending_key_changes_.remove();
		}
		if (!pendingTimeSignature) {
			out_ << "\\changesignature%" << endl;
		}
	}
	if (pendingTimeSignature) {
		out_ << "\\changecontext %" << (barNr_ - 1) << endl;
	}
}

void NMusiXTeX::generate(int staff_nr, int real_staff_nr, const char *extraDelimiter, NMusElement *elem, NStaff *staff_elem, NVoice *ac_voice) {
	QList<NNote> *notelist;
	NExportError *exerr;
	QString *s;
	char *bodyString;
	int lastBodyState;
	bool first, body_changed;
	QString stemdir;
	QString *lyrics;
	NNote *note, *note2;
	NTimeSig *timesig;
	NKeySig *ksig;
	QString musi_length;
	NNote *base_note;
	NChord *chord;
	NChordDiagram *diag;
	QList<NNote> *notes_to_be_shifted, *notes_not_to_be_shifted;
	
	int countLyricsLines;
	bool shift_needed;
	bool toomany, nested, problem128;
	int num;
	int kind, z, i;
	int hline;
	QList<NNote> *acc_list;

	lastSignWasBar_ = false;
	lastSignWasRepeatClose_ = false;
	switch(elem->getType()) {
		case T_CHORD: if (extraDelimiter) out_ << extraDelimiter;
			     writeStaffTexts(real_staff_nr);
			     chord = (NChord *) elem;
			     chord->initialize_acc_pos_computation();
			     if ((chord->status_ & STAT_GRACE) && noteSizeIsNormal_) {
					out_ << "\\multnoteskip\\tinyvalue\\tinynotesize";
					noteSizeIsNormal_ = false;
			     }
			     else if (!(chord->status_ & STAT_GRACE) && !noteSizeIsNormal_) {
					out_ << "\\multnoteskip\\normalvalue\\normalnotesize";
					noteSizeIsNormal_ = true;
			     }
			     if (staff_nr == 0 && newTempo_ >= 0) {
					out_ << "\\Uptext{\\metron{\\nq}{" << newTempo_ << "}}";
					newTempo_ = -1;
			     }
			     notelist = chord->getNoteList();
			     if (chord->status_ & STAT_ARPEGG) {
			     		note = notelist->first();
					note2 = notelist->last();
					out_ << "\\hsk\\arpeggio{" << staff_elem->actualClef_.line2TexTab_[note->line+LINE_OVERFLOW-1] <<
						"}{" << (note2->line - note->line + 3) / 2 << '}';
			     }
			     if ((diag = elem->getChordChordDiagram()) != 0) {
				writeChordDiagram(diag);
			     }
			     for (z = 0, i = chord->getNumOfTexAccRows() - 1; i >= 0; i--, z++) {
				acc_list = chord->getAccTexRow(i);
				out_ << "\\roff{";
			     	for (note = acc_list->first(); note; note = acc_list->next()) {
				   switch (note->offs) {
					case  1: out_ << "\\sh " << staff_elem->actualClef_.line2TexTab_[note->line+LINE_OVERFLOW];
						      break;
					case  0: out_ << "\\na " << staff_elem->actualClef_.line2TexTab_[note->line+LINE_OVERFLOW];
						      break;
					case -1:  out_ << "\\fl " << staff_elem->actualClef_.line2TexTab_[note->line+LINE_OVERFLOW];
						      break;
					case  2:out_ << "\\dsh " << staff_elem->actualClef_.line2TexTab_[note->line+LINE_OVERFLOW];
						      break;
					case -2: out_ << "\\dfl " << staff_elem->actualClef_.line2TexTab_[note->line+LINE_OVERFLOW];
						      break;
				   }
				}
			     }
			     stemdir = ((elem->status_ & STAT_STEM_UP) ? QString("u") : QString("l"));
			     countLyricsLines = ((NChord *) elem)->countOfLyricsLines();
			     if (!exportDialog_->texMLyr->isChecked()) {
			     	for (i = countLyricsLines-1; i >= 0; i--) {
						lyrics = chord->getLyrics(i);
						if (lyrics) {
							out_ << "\\zcharnote{" << -8 - (int) ((staff_elem->staff_props_.lyricsdist - DEFAULT_LYRICSDIST ) * TEX_FAC) -i*4 << "}{"<< lyrics2TeX(lyrics) << "}";
						}
			     	}
			     }
			     s = elem->computeTeXTuplet(&(staff_elem->actualClef_));
			     if (s) {
				out_ << *s;
				delete s;
			     }
			     s = chord->computeTeXTie(&slurTiePool_, &(staff_elem->actualClef_), maxTies_, &toomany, spare_);
			     if (toomany) {
					exerr = new NExportError(MUSIXTEX_ERR_TOO_MANY_TIES, barNr_-1);
					badMeasures_.append(exerr);
			     }
			     if (s) {
				out_ << *s;
				delete s;
			     }
			     s = chord->computeTeXSlur(&slurTiePool_, &(staff_elem->actualClef_), maxSlurs_, &toomany);
			     if (toomany) {
					exerr = new NExportError(MUSIXTEX_ERR_TOO_MANY_SLURS, barNr_-1);
					badMeasures_.append(exerr);
			     }
			     if (s) {
				out_ << *s;
				delete s;
			     }
			     if (elem->status_ & STAT_BEAMED) {
				 s = chord->computeTeXBeam(MAXSTAFFSANDBEAMS, &beamPool_, &(ac_voice->beamNr_), &(ac_voice->beamCount_),
						 &(staff_elem->actualClef_), MAXFLAGS, &problem128, &toomany);
				 if (problem128) {
					exerr = new NExportError(MUSIXTEX_ERR_128, barNr_-1);
					badMeasures_.append(exerr);
				 }
				 if (toomany) {
					exerr = new NExportError(MUSIXTEX_ERR_BEAM_COUNT, barNr_-1);
					badMeasures_.append(exerr);
				 }
				 if (s) {
					out_ << *s;
					delete s;
				 }
			     }
			     if (trill_descr_[staff_nr].trill_nr >= 0 && 	
					chord->getBbox()->right() > trill_descr_[staff_nr].endpos) {
					out_ << "\\Ttrille" << trill_descr_[staff_nr].trill_nr;
					trillPool_ &= (~(1 << trill_descr_[staff_nr].trill_nr));
					trill_descr_[staff_nr].trill_nr = -1;
			     }
			     if (va_descr_[staff_nr].trill_nr >= 0 && 	
					chord->getBbox()->right() > va_descr_[staff_nr].endpos) {
					out_ << "\\Toctfin" << va_descr_[staff_nr].trill_nr;
					vaPool_ &= (~(1 << va_descr_[staff_nr].trill_nr));
					va_descr_[staff_nr].trill_nr = -1;
			     }
			     if (chord->trill_) {
				hline = ac_voice->findHighestLineInTrill(chord);
				s = chord->computeTeXTrill(hline, &trillPool_, &(staff_elem->actualClef_),
					&(trill_descr_[staff_nr]), &nested, &toomany);
				if (nested) {
					exerr = new NExportError(MUSIXTEX_ERR_NESTED_TILLS, real_staff_nr + 1);
					badMeasures_.append(exerr);
				}
				if (toomany) {
					exerr = new NExportError(MUSIXTEX_ERR_TOO_MANY_TRILLS, real_staff_nr + 1);
					badMeasures_.append(exerr);
				}
				if (s) {
					out_ << *s;
					delete s;
				}
			     }
			     if (chord->va_ != 0) {
			     	hline = ac_voice->findBorderLineInVa(chord);
				s = chord->computeTeXVa(chord->va_ < 0, hline, &vaPool_, &(staff_elem->actualClef_),
					&(va_descr_[staff_nr]), &nested, &toomany);
				if (nested) {
					exerr = new NExportError(MUSIXTEX_ERR_NESTED_VAS, real_staff_nr + 1);
					badMeasures_.append(exerr);
				}
				if (toomany) {
					exerr = new NExportError(MUSIXTEX_ERR_TOO_MANY_VAS, real_staff_nr + 1);
					badMeasures_.append(exerr);
				}
				if (s) {
					out_ << *s;
					delete s;
				}
			     }
			     if (chord->dynamic_) {
				dysymDescr_[staff_nr].lastDynSym = (char *) (chord->dynamicAlign_ ? "\\tcresc" : "\\tdecresc ");
				dysymDescr_[staff_nr].dynEndPos = chord->getDynamicEnd();
				out_ << "\\icresc ";
			     }
			     if (dysymDescr_[staff_nr].lastDynSym && 
				chord->getBbox()->right() > dysymDescr_[staff_nr].dynEndPos) {
				out_ << "\\zcharnote " << staff_elem->actualClef_.line2TexTab_[-5+LINE_OVERFLOW] << '{';
				out_ << dysymDescr_[staff_nr].lastDynSym << '}';
				dysymDescr_[staff_nr].lastDynSym = 0;
				dysymDescr_[staff_nr].dynEndPos = 0;
			     } 
			     if (chord->status2_ & STAT2_PEDAL_ON) {
			     	out_ << "\\PED";
			     }
			     if (chord->status2_ & STAT2_PEDAL_OFF) {
			     	out_ << "\\DEP";
			     }
#ifdef AAA
			     for (note = notelist->first(); note; note = notelist->next()) {
				if (note->status & STAT_FORCE) continue;
				switch (note->needed_acc) {
					case STAT_NO_ACC:break;
					case STAT_CROSS: out_ << "\\sh " << staff_elem->actualClef_.line2TexTab_[note->line+LINE_OVERFLOW];
						      break;
					case STAT_NATUR: out_ << "\\na " << staff_elem->actualClef_.line2TexTab_[note->line+LINE_OVERFLOW];
						      break;
					case STAT_FLAT:  out_ << "\\fl " << staff_elem->actualClef_.line2TexTab_[note->line+LINE_OVERFLOW];
						      break;
					case STAT_DCROSS:out_ << "\\dsh " << staff_elem->actualClef_.line2TexTab_[note->line+LINE_OVERFLOW];
						      break;
					case STAT_DFLAT: out_ << "\\dfl " << staff_elem->actualClef_.line2TexTab_[note->line+LINE_OVERFLOW];
						      break;
				}
			     }
			     for (note = notelist->first(); note; note = notelist->next()) {
				if (!(note->status & STAT_FORCE)) continue;
				switch (note->offs) {
					case  1: out_ << "\\sh " << staff_elem->actualClef_.line2TexTab_[note->line+LINE_OVERFLOW];
						      break;
					case  0: out_ << "\\na " << staff_elem->actualClef_.line2TexTab_[note->line+LINE_OVERFLOW];
						      break;
					case -1:  out_ << "\\fl " << staff_elem->actualClef_.line2TexTab_[note->line+LINE_OVERFLOW];
						      break;
					case  2:out_ << "\\dsh " << staff_elem->actualClef_.line2TexTab_[note->line+LINE_OVERFLOW];
						      break;
					case -2: out_ << "\\dfl " << staff_elem->actualClef_.line2TexTab_[note->line+LINE_OVERFLOW];
						      break;
				}
			     }
#endif
			     shiftes_notes_.clear(); non_shifted_notes_.clear(); shift_needed = false;
			     for (note = notelist->first(); note; note = notelist->next()) {
				if (note->status & STAT_SHIFTED) {
					shift_needed = true;
					shiftes_notes_.append(note);
				}
				else {
					non_shifted_notes_.append(note);
				}
			     }
			     if (shift_needed) {
			     	notes_to_be_shifted = (elem->status_ & STAT_STEM_UP) ? &shiftes_notes_ : &non_shifted_notes_;
			     	notes_not_to_be_shifted = (elem->status_ & STAT_STEM_UP) ? &non_shifted_notes_ : &shiftes_notes_;
			     }
			     else {
				notes_to_be_shifted = &shiftes_notes_;
			     	notes_not_to_be_shifted = &non_shifted_notes_;
			     }
			     base_note = notes_not_to_be_shifted->first();
			     if (shift_needed && !(elem->status_ & STAT_STEM_UP)) {
				out_ << "\\roff{";
			     }
			     if (notes_not_to_be_shifted->count() > 1) {
				lastBodyState = -1;
				for (first = true, note = notes_not_to_be_shifted->first(); note; note = notes_not_to_be_shifted->next()) {
					if (note == base_note) continue;
					body_changed = false;
					if (lastBodyState != (note->status & BODY_MASK)) {
						body_changed = true;
						switch (lastBodyState = (note->status & BODY_MASK)) {
							case STAT_BODY_CROSS: bodyString = "x"; break;
							case STAT_BODY_CROSS2: bodyString = "k"; break;
							case STAT_BODY_CIRCLE_CROSS: bodyString = "ox"; break;
							case STAT_BODY_RECT: bodyString = "ro"; break;
							case STAT_BODY_TRIA: bodyString = "tg"; break;
							default: bodyString = ""; break;
						}
					}
					if (body_changed || first) {
						if (!first) out_ << '}';
						switch (elem->getSubType()) {
							case DOUBLE_WHOLE_LENGTH: out_ << '\\' << bodyString << "zwq{"; break;
							case WHOLE_LENGTH       : out_ << '\\' << bodyString << "zw{"; break;
							case HALF_LENGTH        : out_ << '\\' << bodyString << "zh{"; break;
							default   : out_ << '\\' << bodyString << "zq{"; break;
						}
					}
					switch(elem->status_ & DOT_MASK) {
						case 1: out_ << '.'; break;
						/*case 2:  don't know what to do ? ; break;*/
					}
					out_ << staff_elem->actualClef_.line2TexTab_[note->line+LINE_OVERFLOW];
					first = false;
				}
				out_ << "}";
			     }
			     if (shift_needed) {
				out_ << ((elem->status_ & STAT_STEM_UP) ? "\\roff{" : "\\loff{"); 
				lastBodyState = -1;
				for (first = true, note = notes_to_be_shifted->first() ;note ; note = notes_to_be_shifted->next(), first = false) {
					body_changed = false;
					if (lastBodyState != (note->status & BODY_MASK)) {
						body_changed = true;
						switch (lastBodyState = (note->status & BODY_MASK)) {
							case STAT_BODY_CROSS: bodyString = "x"; break;
							case STAT_BODY_CROSS2: bodyString = "k"; break;
							case STAT_BODY_CIRCLE_CROSS: bodyString = "ox"; break;
							case STAT_BODY_RECT: bodyString = "ro"; break;
							case STAT_BODY_TRIA: bodyString = "tg"; break;
							default: bodyString = ""; break;
						}
					}
					if (body_changed || first) {
						if (!first) out_ << '}';
						switch (elem->getSubType()) {
							case DOUBLE_WHOLE_LENGTH: out_ << '\\' << bodyString << "zwq{"; break;
							case WHOLE_LENGTH       : out_ << '\\' << bodyString << "zw{"; break;
							case HALF_LENGTH        : out_ << '\\' << bodyString << "zh{"; break;
							default   : out_ << '\\' << bodyString << "zq{"; break;
						}
					}
					switch(elem->status_ & DOT_MASK) {
						case 1: out_ << '.'; break;
						/*case 2:  don't know what to do ? ; break;*/
					}
					out_ << staff_elem->actualClef_.line2TexTab_[note->line+LINE_OVERFLOW];
				}
				out_ << "}}";
			     }
			     switch (base_note->status & BODY_MASK) {
				case STAT_BODY_CROSS: bodyString = "x"; break;
				case STAT_BODY_CROSS2: bodyString = "k"; break;
				case STAT_BODY_CIRCLE_CROSS: bodyString = "ox"; break;
				case STAT_BODY_RECT: bodyString = "ro"; break;
				case STAT_BODY_TRIA: bodyString = "tg"; break;
				default: bodyString = ""; break;
			     }
			     if (elem->status_ & STAT_BEAMED && ac_voice->beamNr_ >= 0) {
				musi_length.sprintf("%sqb%d", bodyString, ac_voice->beamNr_);
			     }
			     else switch (elem->getSubType()) {
				case DOUBLE_WHOLE_LENGTH: musi_length = QString("wq"); break;
				case WHOLE_LENGTH       : musi_length = QString(bodyString) + QString("wh"); break;
				case HALF_LENGTH        : musi_length = QString(bodyString) + QString("h")+stemdir; break;
				case QUARTER_LENGTH     : musi_length = QString(bodyString) + QString("q")+stemdir; break;
				case NOTE8_LENGTH       : musi_length = QString(bodyString) + QString("c")+stemdir; break;
				case NOTE16_LENGTH      : musi_length = QString(bodyString) + QString("cc")+stemdir; break;
				/* case INTERNAL_MARKER_OF_STROKEN_GRACE: */
				case NOTE32_LENGTH      : if (elem->status_ & STAT_GRACE) {
						musi_length = QString("grcu"); break;
					    }
					    musi_length = QString(bodyString) + QString("ccc")+stemdir; break;
				case NOTE64_LENGTH      : musi_length = QString(bodyString) + QString("cccc")+stemdir; break;
					    musi_length = QString(bodyString) + QString("ccccc")+stemdir; break;
			     }
			     switch(elem->status_ & DOT_MASK) {
				case 1: out_ << "\\pt " << staff_elem->actualClef_.line2TexTab_[base_note->line + LINE_OVERFLOW]; break;
				case 2: out_ << "\\ppt " <<  staff_elem->actualClef_.line2TexTab_[base_note->line + LINE_OVERFLOW]; break;
			     }
			     if (elem->status_ & STAT_STACC || elem->status_ & STAT_SFORZ ||
			         elem->status_ & STAT_PORTA || elem->status_ & STAT_STPIZ ||
				 elem->status_ & STAT_SFZND) {
				out_ << '\\';
				out_ << ((elem->status_ & STAT_STEM_UP) ? "l" : "u");
				if (elem->status_ & STAT_STACC) out_ << "pz";
				if (elem->status_ & STAT_SFORZ) out_ << "sfz";
				if (elem->status_ & STAT_PORTA) out_ << "st";
				if (elem->status_ & STAT_STPIZ) out_ << "ppz";
				if (elem->status_ & STAT_SFZND) out_ << "sf";
				out_ << " " << staff_elem->actualClef_.line2TexTab_[base_note->line+LINE_OVERFLOW];
			     }
			     
			     if(elem->status_ & STAT_FERMT) {
			     	    note2 = (elem->status_ & STAT_STEM_UP) ? notelist->first() :  notelist->last();
			    	    out_ << "\\fermata" << ((elem->status_ & STAT_STEM_UP) ? "down" : "up")
				         << " "
				         << staff_elem->actualClef_.line2TexTab_[note2->line+LINE_OVERFLOW];
				    }
			     
			     out_ << "\\" << musi_length << ' ';
			     out_ << staff_elem->actualClef_.line2TexTab_[base_note->line+LINE_OVERFLOW];
			     if (shift_needed && !(elem->status_ & STAT_STEM_UP)) {
				out_ << '}';
			     }
			     for (i = chord->getNumOfTexAccRows() - 1; i >= 0; i--) {
				out_ << '}';
			     }
			     for (i = 0; i <= chord->getNumOfTexAccRows(); i++) {
				if (i % 2) out_ << "\\hsk";
			     }
			     break;
		case T_REST: writeStaffTexts(real_staff_nr);
			     if (elem->status_ & STAT_HIDDEN) break;
			     if (extraDelimiter) out_ << extraDelimiter;
			     if (staff_nr == 0 && newTempo_ >= 0) {
					out_ << "\\Uptext{\\metron{\\nq}{" << newTempo_ << "}}";
					newTempo_ = -1;
			     }
			     if ((diag = elem->getChordChordDiagram()) != 0) {
				writeChordDiagram(diag);
			     }
			     s = elem->computeTeXTuplet(&(staff_elem->actualClef_));
			     if (s) {
				out_ << *s;
				delete s;
			     }
			     switch(elem->status_ & DOT_MASK) {
				case 1: out_ << "\\pt " <<  staff_elem->actualClef_.line2TexTab_[4 + LINE_OVERFLOW - 2*ac_voice->yRestOffs_]; break;
				case 2: out_ << "\\ppt " <<  staff_elem->actualClef_.line2TexTab_[4 + LINE_OVERFLOW - 2*ac_voice->yRestOffs_]; break;
			     }
			     if (ac_voice->yRestOffs_) {
			     	out_ << "\\raise" << -ac_voice->yRestOffs_ << "\\Interligne"; 
			     }
			     switch (elem->getSubType()) {
				case WHOLE_LENGTH   : musi_length = QString("pause"); break;
				case HALF_LENGTH    : musi_length = QString("hpause"); break;
				case QUARTER_LENGTH : musi_length = QString("qp"); break;
				case NOTE8_LENGTH   : musi_length = QString("ds"); break;
				case NOTE16_LENGTH  : musi_length = QString("qs"); break;
				case NOTE32_LENGTH  : musi_length = QString("hs"); break;
				case NOTE64_LENGTH  : musi_length = QString("qqs"); break;
				case NOTE128_LENGTH  : musi_length = QString("qqs");
						exerr = new NExportError(MUSIXTEX_ERR_128, barNr_-1);
								badMeasures_.append(exerr);
					    break;
			     }
			     if(elem->status_ & STAT_FERMT) {
				// LVIFIX does fermataup need a parameter and, if so, what should it be ?
				// the "k" below is a random choice
				out_ << "\\fermataup k";
			     }
			     if (elem->getSubType() == MULTIREST)
				out_ << "\\Hpause32\\uptext{\\sk\\meterfont{" << ((NRest *)elem)->getMultiRestLength() << "}}\\sk\\sk\\hsk";
			     else 
			        out_ << "\\" << musi_length;
			     break;
		case T_SIGN: 
			     switch (elem->getSubType()) {
			        case END_BAR: if (staff_nr == 0) {
						if (ac_voice->testSpecialEnding(&num)) {
							out_ << "\\setvolta" << num;
						}
					      }
					      break;
				case SIMPLE_BAR: if (staff_nr == 0) {
							if (ac_voice->testSpecialEnding(&num)) {
								out_ << "\\setvolta" << num;
							}
							lastBarPos_ = out_.tellp();
							if (limitMeasures_ && ((barNr_ - 1) % maxMeasuresPerLine_) == 0 &&
								barNr_ != lastBarNr_) {
								 out_ << "\\alaligne %" << barNr_++ << endl;
							}
							else {
								out_ << "\\bar % "  << barNr_++ << endl;
							}
						 	lastSignWasBar_ = true;
						 }
						 break;
				case DOUBLE_BAR: if (staff_nr == 0) {
							if (ac_voice->testSpecialEnding(&num)) {
								out_ << "\\setvolta" << num;
							}
							lastBarPos_ = out_.tellp();
							if (limitMeasures_ && ((barNr_ - 1) % maxMeasuresPerLine_) == 0 &&
								barNr_ != lastBarNr_) {
								 out_ << "\\alaligne %" << barNr_++ << endl;
							}
							else {
								out_ << "\\doublebar % "  << barNr_++ << endl;
							}
						 	lastSignWasBar_ = true;
						 }
						 break;
				case REPEAT_OPEN: if (staff_nr == 0) {
							if (ac_voice->testSpecialEnding(&num)) {
								out_ << "\\setvolta" << num;
							}
							out_ << "\\leftrepeat % " << barNr_++ << endl;
							lastBarPos_ = out_.tellp();
						   }
						break;
				case REPEAT_CLOSE: if (staff_nr == 0) {
							if (ac_voice->testSpecialEnding(&num)) {
								out_ << "\\setvolta" << num;
							}
							if (ac_voice->isLastElem(elem)) {
								out_ << "\\setrightrepeat % " << barNr_++ << endl;
								lastSignWasRepeatClose_ = true;
							}
							else {
								out_ << "\\rightrepeat % " << barNr_++ << endl;
							}
							lastBarPos_ = out_.tellp();
						 	lastSignWasBar_ = true;
						   }
						break;
				case REPEAT_OPEN_CLOSE: if (staff_nr == 0) {
							if (ac_voice->testSpecialEnding(&num)) {
								out_ << "\\setvolta" << num;
							}
							out_ << "\\leftrightrepeat % " << barNr_++ << endl;
							lastBarPos_ = out_.tellp();
						   }
						break;
				case TEMPO_SIGNATURE: 
						   if (!exportDialog_->texTempo->isChecked()) break;
						   newTempo_ = ((NSign *) elem)->getTempo();
						break;
				case VOLUME_SIG: if (!exportDialog_->texVolume->isChecked()) break;
						out_ << "\\zchar{-7}";
							switch(((NSign *) elem)->getVolType()) {
								case V_PPPIANO : out_ << "\\ppp"; break;
								case V_PPIANO  : out_ << "\\pp"; break;
								case V_PIANO   : out_ << "\\p"; break;
								case V_MPIANO : out_ << "\\mp"; break;
								case V_FORTE   : out_ << "\\f"; break;
								case V_FFORTE  : out_ << "\\ff"; break;
								case V_FFFORTE : out_ << "\\fff"; break;
								default        : out_ << "\\mf"; break;
							}
						break;
				case SEGNO:     out_ << "\\segno " << staff_elem->actualClef_.line2TexTab_[12+LINE_OVERFLOW];
						break;
				case CODA:     out_ << "\\hsk\\coda " << staff_elem->actualClef_.line2TexTab_[12+LINE_OVERFLOW] << "\\hsk";
						break;
				case DAL_SEGNO: out_ << "\\Uptext{D.S.}\\sk\\sk";
						break;
				case DAL_SEGNO_AL_FINE: out_ << "\\Uptext{D.S. al Fine}\\sk\\sk";
						break;
				case DAL_SEGNO_AL_CODA: out_ << "\\Uptext{D.S. al Coda}\\sk\\sk";
						break;
				case FINE:      out_ << "\\Uptext{\\Large{\\textbf{Fine}}\\sk\\sk}";
						break;
				case RITARDANDO: out_ << "\\Uptext{ritard.}";
						break;
				case ACCELERANDO: out_ << "\\Uptext{accel.}";
						break;
			     }
			     break;
		case T_TIMESIG:
			     if (barNr_ <= 2) break;
			     timesig = (NTimeSig *) elem;
			     pending_time_changes_.append(new pending_context_change_class(real_staff_nr, timesig->getNumerator(), timesig->getDenominator()));
			     out_ << "\\setmeter{" << (staff_nr + 1) << "}{{\\meterfrac{" << timesig->getNumerator() <<
					"}{" << timesig->getDenominator() << "}}}%" << endl;
			     break;
		case T_KEYSIG:
			     if (barNr_ <= 2) break;
			     ksig = (NKeySig *) elem;
			     if (ksig->isRegular(&kind, &z)) {
				if (z != 0 || barNr_ > 1) {
					pending_key_changes_.append(new pending_context_change_class(real_staff_nr, (kind == STAT_CROSS) ? z : -z));
				}
			     }
			     break;
		case T_CLEF:
			     if (barNr_ <= 2) break;
			     staff_elem->actualClef_.change((NClef *) elem);
			     switch(elem->getSubType()) {
				case BASS_CLEF:
					mStaffInf_->noticeClefChange(real_staff_nr, 6);
			     		break;
				case TREBLE_CLEF:
					mStaffInf_->noticeClefChange(real_staff_nr, 0);
			     		break;
				case ALTO_CLEF:
					mStaffInf_->noticeClefChange(real_staff_nr, 3);
			     		break;
				case TENOR_CLEF:
					mStaffInf_->noticeClefChange(real_staff_nr, 4);
			     		break;
			     }
			     break;
		case T_TEXT: pending_texts_.append(new textDescr((NText *) elem, real_staff_nr));
			     break;
	}
}

void NMusiXTeX::writeStaffTexts(int real_staff_nr) {
	int pos;
	textDescr *text_tescr;
	text_tescr = pending_texts_.first();
	while (text_tescr) {
		if (text_tescr->staff_nr != real_staff_nr)  {
			text_tescr = pending_texts_.next();
			continue;
		}
		if (text_tescr->textElem->getSubType() == TEXT_UPTEXT) {
			pos = 11;
		}
		else {
			pos = -4;
		}
		out_ << "\\zcharnote{" << pos << "}{\\textbf{" << text_tescr->textElem->getText() << "}}";
		pending_texts_.remove();
		text_tescr = pending_texts_.first();
	}
}

void NMusiXTeX::writeChordDiagram(NChordDiagram *diag) {
	int i, j;
	bool isinbarre;
	QString fretvalue, chordname;
	QRegExp reg = QRegExp("#");

	chordname = diag->getChordName();
	chordname.replace (reg, "\\#");
	if (diag->showDiagram_) {
		out_ << "\\guitar {\\textbf{" << chordname << '}';
		if (diag->getFirst() > 1) {
			fretvalue.sprintf("(fr.%d)", diag->getFirst());
			out_ << fretvalue;
		}
		out_ << "}{}";
		for (i = 0; i < 6; i++) {
			switch (diag->getStrings()[i]) {
				case -1: out_ << 'x'; break;
				case  0: out_ << 'o'; break;
				default: out_ << '-'; break;
			}
		}
		for (i = 0; i < diag->getBarreCount(); i++) {
			if (diag->barree_[i][1] == 0) {
				fretvalue.sprintf("\\gbarre%d", diag->barree_[i][0]+1);
				out_ << fretvalue;
			}
		}
		for (i = 0; i < 6; i++) {
			isinbarre = false;
			for (j = 0; !isinbarre && j < diag->getBarreCount(); j++) {
				isinbarre = diag->barree_[j][1] == 0 && diag->barree_[j][0] == diag->getStrings()[i] - diag->getFirst();
			}
			if (isinbarre) continue;
			if (diag->getStrings()[i] > 0) {
				fretvalue.sprintf("\\gdot%d%d", (i+1), diag->getStrings()[i] - diag->getFirst() + 1);
				out_ << fretvalue;
			}
		}
	}
	else {
		out_ << "\\Uptext{\\textbf{" << chordname << "}}";
	}
}


NKeySig *NMusiXTeX::getKeySig(int multistaffnr) {
	NStaff *staff_elem;

	if ((staff_elem = staffList_->at(mStaffInf_->getfirstStaffInMultistaff(multistaffnr))) == 0) {
		NResource::abort("getKeySig: internal error", 2);
	}
	return staff_elem->getVoiceNr(0)->getFirstKeysig();
}

NTimeSig *NMusiXTeX::getTimeSig(int multistaffnr) {
	NStaff *staff_elem;

	if ((staff_elem = staffList_->at(mStaffInf_->getfirstStaffInMultistaff(multistaffnr))) == 0) {
		NResource::abort("getTimeSig: internal error", 2);
	}
	return staff_elem->getVoiceNr(0)->getFirstTimeSig();
}
	
QString *NMusiXTeX::getStaffName(int multistaffnr) {
	NStaff *staff_elem;

	if ((staff_elem = staffList_->at(mStaffInf_->getfirstStaffInMultistaff(multistaffnr))) == 0) {
		NResource::abort("getStaffName: internal error", 2);
	}
	return &staff_elem->staffName_;
}
	



QString NMusiXTeX::lyrics2TeX(QString *lyrics) {
	QString ret;
	QRegExp reg;

	ret = QString(*lyrics);
	reg = QRegExp("^ *[-\\*] *$");
	if (ret.find(reg) != -1) {
		return QString("");
	}
	if (exportDialog_->texOutputEncoding->currentItem() == 0) {
		NResource::germanUmlautsToTeX(&ret);
	}
	reg = QRegExp("#");
	ret.replace (reg, "\\#");
	reg = QRegExp("_");
	ret.replace (reg, "\\_");
	if (exportDialog_->texMLyr->isChecked()) {
		reg = QRegExp("<");
		ret.replace (reg, "{");
		reg = QRegExp(">");
		ret.replace (reg, "}");
	}
	else {
		reg = QRegExp("[<>]");
		ret.replace (reg, "");
	}
	if (exportDialog_->texOutputEncoding->currentItem() == 0) {
		return ret;
	}
	return ret.utf8();
}

void NMusiXTeX::externalCmd (QString command, QString filename) {
  
  // Execute external command.
  // A "%f" in command is replaced by filename
  QRegExp reg = QRegExp("%f");
  QString setUserPath;
  QString destDir;
  int slashidx;


  command.replace(reg,filename);
  // remember the user path, it is destroyed during KDE part mechanism: */
  if (!NResource::userpath_.isNull() && !NResource::userpath_.isEmpty()) {
	command = "export PATH=" + NResource::userpath_ + ';' + command;
  }
  // change directory to place ".dvi" where ".tex" is
  if ((slashidx = filename.findRev('/')) >= 0) {
  	if (slashidx < filename.length() - 1) {
		destDir = filename.left(slashidx + 1);
		command = "cd " + destDir + ';' + command;
	}
  }

  // Kdo zusammensetzen
  // abgeschrieben aus lilytest.cpp
  char tempDir[] = "/tmp/noteedit.XXXXXX";
  mkstemp( tempDir );
  command.append(" >");
  command.append(tempDir);
  command.append(" 2>&1");
  // Execute kdo
  system (command.latin1());

  // Ausgabe aus Tmp-File zusammenfriemeln
  QString output;
  QFile f(tempDir);
  if (f.open(IO_ReadOnly) ) {
    QTextStream t( &f);
    output.append( t.read());
    f.close();
    f.remove();
  }
  
  OutputBox::warning(0, i18n("Output from MusiXTeX-Cmd"), output, "MusiXTeX");

}
