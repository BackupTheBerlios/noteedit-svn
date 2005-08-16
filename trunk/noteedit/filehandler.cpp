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

#include <stdlib.h> //  esigra: needed for mkstemp when --without-libs
#include <stdio.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <qstring.h>
#include <qregexp.h>
#include "filehandler.h"
#include "clef.h"
#include "timesig.h"
#include "sign.h"
#include "rest.h"
#include "chord.h"
#include "keysig.h"
#include "mainframewidget.h"
#include "voice.h"
#include "outputbox.h"
#include "parsertypes.h"
#include "staff.h"
#include "text.h"
#include "uiconnect.h"
#include "chorddiagram.h"
#include "layout.h"


NFileHandler::NFileHandler() : newLines_("\n") {
	int i;
#if GCC_MAJ_VERS > 2
	os_ = new ostringstream();
	ornaments_ = new ostringstream();
	dynamics_  = new ostringstream();
	chordline_ = new ostringstream();
	phrases_ = new ostringstream();
	trillsyms_ = new ostringstream();
	valines_ = new ostringstream();
	for (i = 0; i < NUM_LYRICS; i++) {
		lyricsLine_[i] = new ostringstream();
	}
#else
	os_ = new ostrstream(buffer_, 128);
	ornaments_ = new ostrstream(ornamentbuffer_,  128);
	phrases_ = new ostrstream(phrasebuffer_,  128);
	dynamics_  = new ostrstream(dynamicsbuffer_, 128);
	chordline_ = new ostrstream(chordsbuffer_, 128);
	trillsyms_ = new ostrstream(trillsymsbuffer_, 128);
	valines_ = new ostrstream(valinebuffer_, 128);
	for (i = 0; i < NUM_LYRICS; i++) {
		lyricsLine_[i] = new ostrstream(NResource::lyricsbuffer_[i], LYRICS_LINE_LENGTH);
	}
#endif
	rolls_.truncate(0);
	pedals_.truncate(0);
	signs_.truncate(0);
	textsigns_above_.truncate(0);
	textsigns_below_.truncate(0);
	badlist_.setAutoDelete(true);
	fatallist_.setAutoDelete(true);
	pending_volume_sigs_.setAutoDelete(true);
	pending_tempo_sigs_.setAutoDelete(true);
	pending_program_changes_.setAutoDelete(true);
	chordDiagramList_.setAutoDelete(true);

	mupWarn_ = new mupWrn( 0 );
}

void NFileHandler::pitchOut( const NNote *note, NClef *ac_clef, bool with_tie) {
	int i, octave;

	out_ << ac_clef->line2Name(note->line, &octave, false, false);
	if (!(note->status & STAT_PART_OF_TIE) && with_tie) {
		if (note->status & STAT_FORCE) {
			switch (note->offs) {
				case  1: out_ << "#"; break;
				case -1: out_ << "&"; break;
				case  0: out_ << "n"; break;
				case  2: out_ << "x"; break;
				case -2: out_ << "&&"; break;
			}
		}
		else {
			switch (note->status & ACC_MASK) {
				case STAT_CROSS: out_ << "#";  break;
				case STAT_FLAT: out_ << "&";   break;
				case STAT_NATUR: out_ << "n";  break;
				case STAT_DCROSS: out_ << "x"; break;
				case STAT_DFLAT: out_ << "&&"; break;
			}
		}
	}
	if (octave > 0) {
		for (i = 0; i < octave; i++) {
			out_ << "+";
		}
	}
	else if (octave < 0) {
		for (i = 0; i > octave; i--) {
			out_ << "-";
		}
	}
	if ((note->status & STAT_TIED) && with_tie) out_ << "~";
	switch (note->status & BODY_MASK) {
		case STAT_BODY_CROSS: out_ << " bcr "; break;
		case STAT_BODY_CROSS2: out_ << " bcr2 "; break;
		case STAT_BODY_CIRCLE_CROSS: out_ << " bcrc "; break;
		case STAT_BODY_TRIA: out_ << " btr "; break;
		case STAT_BODY_RECT: out_ << " brec "; break;
	}
}


	
bool NFileHandler::writeStaffs(QString fname, QList<NStaff> *stafflist, NMainFrameWidget *mainWidget, bool showErrors) {
	NVoice *voice_elem;
	NStaff *staff_elem;
	NMusElement *last_elem;
	int i, j, written_voices;
	int multirestlength;
	int current_time;
	int voice_count;
	NTimeSig *timesig;
	int btype, bartype;
	int measure_start_time;
	bool first;
	bool ending2seen = false;
	badmeasure *bad;
	QString outstring;
	QString finger;
	bool gridsused, firstcall, gridproblem;
	chordDiagramName *diag_name;
	pending_sign_information_class *pending_sign_information;
	
	countof128th_ = 128;
	drum_problem_written_ = false;
	bar_nr_ = 1;

	badlist_.clear();
	fatallist_.clear();
	chordDiagramList_.clear();

	staffCount_ = stafflist->count();
	pending_multi_rests_ = new int*[staffCount_];
	for (i = 0, staff_elem = stafflist->first(); staff_elem; staff_elem = stafflist->next(), i++) {
		pending_multi_rests_[i] = new int[staff_elem->voiceCount()];
		for (j = 0; j < staff_elem->voiceCount(); j++)  {
			*(pending_multi_rests_[i] + j) = 0;
		}
	}
		

	if (stafflist->isEmpty()) {
		KMessageBox::sorry
		  (0,
		   i18n("nothing to write"),
		   kapp->makeStdCaption(i18n("Write Staffs"))
		  );
		return false;
	}

	out_.open(fname);
	
	if (!out_) {
		*os_ << i18n("error opening file ") << fname << '\0';
#if GCC_MAJ_VERS > 2
		KMessageBox::sorry
		  (0, os_->str().c_str(), kapp->makeStdCaption(i18n("Write Staffs")));
#else
		KMessageBox::sorry
		  (0, QString(os_->str()), kapp->makeStdCaption(i18n("Write Staffs")));
#endif
		return false;
	}

	out_ << "header" << endl;
	if (!mainWidget->scTitle_.isEmpty()) {
		outstring = mainWidget->scTitle_;
		outstring.replace('\\', "\\\\"); /* replace all backslashes with \\ two character backslashes */
		outstring.replace (newLines_, "\\n"); /* replace all newlines with \n two character symbols */
		outstring.replace('"', "\\\""); /* replace all double quotes with \" two character symbols */
		out_ << "\ttitle bold (" <<  SC_TITLE_FONT_SIZE << ") \"" << outstring.utf8() << '"' << endl;
	}
	if (!mainWidget->scSubtitle_.isEmpty()) {
		outstring = mainWidget->scSubtitle_;
		outstring.replace('\\', "\\\\"); /* replace all backslashes with \\ two character backslashes */
		outstring.replace (newLines_, "\\n"); /* replace all newlines with \n two character symbols */
		outstring.replace('"', "\\\""); /* replace all double quotes with \" two character symbols */
		out_ << "\ttitle bold (" << SC_SUBTITLE_FONT_SIZE << ") \"" << outstring.utf8() << '"' << endl;
	}
	if (!mainWidget->scAuthor_.isEmpty()) {
		outstring = mainWidget->scAuthor_;
		outstring.replace('\\', "\\\\"); /* replace all backslashes with \\ two character backslashes */
		outstring.replace (newLines_, "\\n"); /* replace all newlines with \n two character symbols */
		outstring.replace('"', "\\\""); /* replace all double quotes with \" two character symbols */
		out_ << "\ttitle bold (" << SC_AUTHOR_FONT_SIZE << ") \"" << outstring.utf8() << '"' << endl;
	}
	if (!mainWidget->scLastAuthor_.isEmpty()) {
		outstring = mainWidget->scLastAuthor_;
		outstring.replace('\\', "\\\\"); /* replace all backslashes with \\ two character backslashes */
		outstring.replace (newLines_, "\\n"); /* replace all newlines with \n two character symbols */
		outstring.replace('"', "\\\""); /* replace all double quotes with \" two character symbols */
		out_ << "\ttitle bold (" << SC_LAST_AUTHOR_FONT_SIZE << ") \"" << outstring.utf8() << '"' << endl;
	}
	if (!mainWidget->scComment_.isEmpty()) {
		outstring = mainWidget->scComment_;
		outstring.replace('\\', "\\\\"); /* replace all backslashes with \\ two character backslashes */
		outstring.replace (newLines_, "\\n"); /* replace all newlines with \n two character symbols */
		outstring.replace('"', "\\\""); /* replace all double quotes with \" two character symbols */
		out_ << "\ttitle bold (" << SC_COMMENT_FONT_SIZE << ") \"" <<  outstring.utf8() << '"' << endl;
	}
	if (!mainWidget->scCopyright_.isEmpty()) {
		out_ << "footer" << endl;
		outstring = mainWidget->scCopyright_;
		outstring.replace('\\', "\\\\"); /* replace all backslashes with \\ two character backslashes */
		outstring.replace (newLines_, "\\n"); /* replace all newlines with \n two character symbols */
		outstring.replace('"', "\\\""); /* replace all double quotes with \" two character symbols */
		out_ << "\ttitle bold (" << SC_COPYRIGHT_FONT_SIZE << ") \"" <<  outstring.utf8() << '"' << endl;
	}
	out_ << "score" << endl;
	out_ << "\tpedstyle = pedstar" << endl;
	if (mainWidget->paramsEnabled()) {
		out_ << "\tunits = cm" << endl;
		out_ << "\tpagewidth = " << ((double) mainWidget->getSaveWidth() / 10.0) << endl;
		out_ << "\tpageheight = " << ((double) mainWidget->getSaveHeight() / 10.0) << endl;
	}
	if (mainWidget->withMeasureNums()) {
		out_ << "\tmeasnum = y" << endl;
	}
	out_ << "\tstaffs = " << stafflist->count() << endl;
	voice_elem = stafflist->first()->getVoiceNr(0);
	timesig = voice_elem->getFirstTimeSig();
	if (timesig) {
		countof128th_ = timesig->numOf128th();
		out_ << "\ttime = " << timesig->getNumerator() <<
			'/' << timesig->getDenominator() << endl;
		curr_num_ = timesig->getNumerator();
		curr_denom_ = timesig->getDenominator();
	}
	else {
		countof128th_ = 128;
		curr_num_ = curr_denom_ = 4;
	}
	writeStaffLayout(mainWidget, stafflist->count());
	gridsused = false;
	firstcall = true;
	gridproblem = false;
	for (staff_elem = stafflist->first(); staff_elem; staff_elem = stafflist->next()) {
		voice_count = staff_elem->voiceCount();
		for (j = 0; j < voice_count; j++) {
			voice_elem = staff_elem->getVoiceNr(j);
			voice_elem->prepareForWriting();
			voice_elem->getChordDiagramms(&chordDiagramList_, &gridsused, firstcall, &gridproblem);
			voice_elem->inBeam_ = voice_elem->inTuplet_ = false;
			firstcall = false;
		}
	}
	if (gridproblem) {
		badmeasure *bad = new badmeasure(ERR_MIXED_GRIDS, 0 /*dummy */, 0 /* dummy */ , 3 /* dummy */, countof128th_ /* dummy */);
		badlist_.append(bad);
	}
	if (chordDiagramList_.count()) {
		if (gridsused) {
			out_ << "\tgridswhereused=y" << endl << "\tgridfret=4" << endl;
			out_ << "grids" << endl;
		}
		for (diag_name = chordDiagramList_.first(); diag_name; diag_name = chordDiagramList_.next()) {
			if (!diag_name->cdiagramm->showDiagram_) continue;
			out_ <<"\t\""; 
			for (i = 0; i < diag_name->NumOfUnderscores; i++) {
				out_ << '_';
			}
			if (diag_name->cdiagramm->getChordName().isEmpty() || diag_name->cdiagramm->getChordName().isNull()) {
				out_ << 'X';
			}
			else {
				out_ << diag_name->cdiagramm->getChordName();
			}
			out_ << "\"\t\"";
			for (i = 0; i < 6; i++) {
				switch (diag_name->cdiagramm->getStrings()[i]) {
					case -1: out_ << 'x'; break;
					case  0: out_ << 'o'; break;
					default: finger.sprintf("%d", diag_name->cdiagramm->getStrings()[i]);
						 out_ << finger; break;
				}
				if (i < 5) out_ << ' ';
			}
			out_ << '"' << endl;
		}
	}
	
	first = true;
	musicmode_ = true;
	music_written_ = false;
	do {
		written_voices = 0;
		for (i = 1, staff_elem = stafflist->first(); staff_elem; staff_elem = stafflist->next(), i++) {
			voice_elem = staff_elem->getVoiceNr(0);
			hasClef_ = false;
			writeScoreInfo(i, voice_elem, first, mainWidget);
		}
 		if (!musicmode_ || (first && !music_written_)) {
			musicmode_ = true;
			music_written_ = true;
			out_ << "music" << endl;
		}
		if (first) {
			for (i = 1, staff_elem = stafflist->first(); staff_elem; staff_elem = stafflist->next(), i++) {
				out_ << "midi " << i << " 1: 0 \"channel=" << staff_elem->getChannel()+1 << 
				"\"; 0 \"program=" << staff_elem->getVoice() << "\";" << endl;
			}
			for (i = 1, staff_elem = stafflist->first(); staff_elem; staff_elem = stafflist->next(), i++) {
				out_ << "midi " << i << " 1: 0 \"parameter=" << PROG_VOL << "," << staff_elem->getVolume() << "\";" << endl;
				out_ << "midi " << i << " 1: 0 \"parameter=" << PROG_CHORUS << "," << staff_elem->chorus_ << "\";" << endl;
				out_ << "midi " << i << " 1: 0 \"parameter=" << PROG_REVERB << "," << staff_elem->reverb_ << "\";" << endl;
				out_ << "midi " << i << " 1: 0 \"parameter=" << PROG_PAN << "," << staff_elem->pan_ << "\";" << endl;
			}
		}
		multirestlength = determineMultiRest(stafflist);
		bartype = 0;
		for (i = 1, staff_elem = stafflist->first(); staff_elem; staff_elem = stafflist->next(), i++) {
			voice_elem = staff_elem->getVoiceNr(0);
#if GCC_MAJ_VERS > 2
			delete ornaments_;
			ornaments_ = new ostringstream();
			delete valines_;
			valines_ = new ostringstream();
			delete trillsyms_;
			trillsyms_ = new ostringstream();
			delete dynamics_;
			dynamics_ = new ostringstream();
			delete chordline_;
			chordline_ = new ostringstream();
			delete phrases_;
			phrases_ = new ostringstream();
#else
			ornaments_->seekp(0);
			trillsyms_->seekp(0);
			valines_->seekp(0);
			phrases_->seekp(0);
			dynamics_->seekp(0);
			chordline_->seekp(0);
#endif
			rolls_.truncate(0);
			pedals_.truncate(0);
			signs_.truncate(0);
			textsigns_above_.truncate(0);
			textsigns_below_.truncate(0);
			lyricsLineCount_ = 0;
			pending_volume_sigs_.clear();
			pending_tempo_sigs_.clear();
			pending_program_changes_.clear();
			some_notes_or_rests_written_ = false;
			btype = writeStaffUntilBar(i, voice_elem, first, multirestlength, &measure_start_time);
			voice_count = staff_elem->voiceCount();
			last_elem = voice_elem->getCurrentPosition();
			if (last_elem) {
				current_time = last_elem->midiTime_;
			}
			else {
				current_time = (1 << 30);
			}
			for (pending_sign_information = pending_volume_sigs_.first(); pending_sign_information;
			 		pending_sign_information = pending_volume_sigs_.next()) {
				writeVolSig(pending_sign_information->staff_time, i, pending_sign_information->pending_sign);
			}
			for (pending_sign_information = pending_program_changes_.first(); pending_sign_information;
			 		pending_sign_information = pending_program_changes_.next()) {
				writeProgramChange(pending_sign_information->staff_time, i, pending_sign_information->pending_sign);
			}
			for (pending_sign_information = pending_tempo_sigs_.first(); pending_sign_information;
			 		pending_sign_information = pending_tempo_sigs_.next()) {
				writeTempoSig(pending_sign_information->staff_time, pending_sign_information->pending_sign);
			}
#if GCC_MAJ_VERS > 2
			if (ornaments_->tellp() > 1) {
				*ornaments_ << endl;
				out_ << ornaments_->str();
			}
			if (trillsyms_->tellp() > 1) {
				*trillsyms_ << endl;
				out_ << trillsyms_->str();
			}
			if (valines_->tellp() > 1) {
				*valines_ << endl;
				out_ << valines_->str();
			}
			if (dynamics_->tellp() > 1) {
				*dynamics_ << endl;
				out_ << dynamics_->str();
			}
			if (chordline_->tellp() > 1) {
				*chordline_ << endl;
				out_ << chordline_->str();
			}
			if (phrases_->tellp() > 1) {
				*phrases_ << endl;
				out_ << phrases_->str();
			}
#else
			if (ornaments_->pcount() > 1) {
				*ornaments_ << endl << '\0';
				out_ << ornaments_->str();
			}
			if (trillsyms_->pcount() > 1) {
				*trillsyms_ << endl << '\0';
				out_ << trillsyms_->str();
			}
			if (valines_->pcount() > 1) {
				*valines_ << endl << '\0';
				out_ << valines_->str();
			}
			if (phrases_->pcount() > 1) {
				*phrases_ << endl << '\0';
				out_ << phrases_->str();
			}
			if (dynamics_->pcount() > 1) {
				*dynamics_ << endl << '\0';
				out_ << dynamics_->str();
			}
			if (chordline_->pcount() > 1) {
				*chordline_ << endl << '\0';
				out_ << chordline_->str();
			}
#endif
			if (!rolls_.isEmpty()) {
				out_ << rolls_ << endl;
			}
			if (!pedals_.isEmpty()) {
				out_ << pedals_ << endl;
			}
			if (!textsigns_above_.isEmpty()) {
				out_ << textsigns_above_ << endl;
			}
			if (!textsigns_below_.isEmpty()) {
				out_ << textsigns_below_ << endl;
			}
			if (!signs_.isEmpty()) {
				if (!some_notes_or_rests_written_) {
						bad = new badmeasure(ERR_SEGNOS_AT_END, i, bar_nr_, 3 /* dummy */, countof128th_);
						fatallist_.append(bad);
				}
				else {
					out_ << signs_ << endl;
				}
			}
			for (j = 2; j <= voice_count; j++) {
				voice_elem = staff_elem->getVoiceNr(j-1);
				rolls_.truncate(0);
#if GCC_MAJ_VERS > 2
				delete phrases_;
				phrases_ = new ostringstream();
#else
				phrases_->seekp(0);
#endif
				writeVoiceElemsTill(i, j, voice_elem, current_time, multirestlength, measure_start_time);
				if (!rolls_.isEmpty()) {
					out_ << rolls_ << endl;
				}
#if GCC_MAJ_VERS > 2
				if (j == 2 && phrases_->tellp() > 1) {
					*phrases_ << endl;
					out_ << phrases_->str();
#else
				if (j == 2 && phrases_->pcount() > 1) {
					*phrases_ << endl << '\0';
					out_ << phrases_->str();
#endif
				}
			}
			if (lyricsLineCount_) {
				if (lyricsLineCount_ == 1) {
					(*lyricsLine_[0]) << '\0';
					out_ << "lyrics " << i << ": \"" << lyricsLine_[0]->str() << "\";" << endl;
				}
				else {	
					for (j = 0; j < lyricsLineCount_; j++) {
						(*lyricsLine_[j]) << '\0';
						out_ << "lyrics " << i << ":  [" << (j+1) << "]\"" << lyricsLine_[j]->str() << "\";" << endl;
					}
				}
			}
			if (btype) {
				if (bartype) {
					if (btype != bartype) {
						bad = new badmeasure(ERR_BARTYPES, i, bar_nr_, 3 /* dummy */, countof128th_);
						fatallist_.append(bad);
					}
				}
				bartype = btype;
				written_voices++;
			}
		}
		if (written_voices) {
			if (!bartype) bartype = SIMPLE_BAR;
			switch (bartype & BAR_SYMS) {
				case END_BAR: out_ << "endbar"; break;
				case SIMPLE_BAR: out_ << "bar"; break;
				case REPEAT_OPEN: out_ << "repeatstart"; break;
				case REPEAT_CLOSE: out_ << "repeatend";
							if (repaetTime_ > 2) {
								out_ << " rehearsal \"x " << repaetTime_ << '\"';
							}
						   break;
				case REPEAT_OPEN_CLOSE: out_ << "repeatboth"; break;
				case DOUBLE_BAR: out_ << "dblbar"; break;
			}
			if (ending2seen) {
				ending2seen = false;
				out_ << " endending";
			}
			switch (bartype & SPECIAL_ENDING) {
				case SPECIAL_ENDING1: out_ << " ending \"1.\""; break;
				case SPECIAL_ENDING2: out_ << " ending \"2.\""; ending2seen = true; break;
			}

			out_ << " // " << bar_nr_++ << endl;
		}
		first = false;
	}
	while (written_voices);
	out_.close();
	for (i = 0, staff_elem = stafflist->first(); staff_elem; staff_elem = stafflist->next(), i++) {
		delete pending_multi_rests_[i];
	}
	delete pending_multi_rests_;

	if (!showErrors) return true;

	if (!fatallist_.isEmpty()) {
		QString output;
		for (bad = fatallist_.first(); bad; bad = fatallist_.next()) {
			switch (bad->kind) {
				case ERR_BARTYPES:
					output += i18n
						("staff %1; measure %2: you mixed different bar types; this "
						 "cannot be expressed in MUP and causes data loss\n"
						).arg(bad->track).arg(bad->measure);
					break;
				case ERR_MULTIREST:
					output += i18n
						("Staff %1 ; measure %2 :The multirests are not aligned\n"
						).arg(bad->track).arg(bad->measure);
					break;
				case ERR_IRREGULAER:
					output += i18n
						("staff %1; measure %2: irregulaer key in the middle omitted\n"
						).arg(bad->track).arg(bad->measure);
					break;
				case ERR_TUPLET:
					output += i18n
						("staff %1; measure %2: Mup can only deal with tuplets which sum up to a single note length\n"
						).arg(bad->track).arg(bad->measure);
					break;
				case ERR_SEGNOS_AT_END:
					output += i18n
						("staff %1; measure %2: Mup cannot deal with segno staff at end. The segno staff is deleted.  Please insert a rest!\n"
						).arg(bad->track).arg(bad->measure);
					break;
				case ERR_PEDAL_IN_2ND:
					output += i18n
						("staff %1; measure %2: Mup cannot deal with pedal marks in non-1st-voice! This cannot be restored\n"
						).arg(bad->track).arg(bad->measure);
					break;
			}
		}
		OutputBox::warning(0, i18n("Possible data loss!"), output, i18n("Save"));
	}

	if (!NResource::dontShowMupWarnings_ && !badlist_.isEmpty()) {
		QString output;
		output = i18n
			("Noteedit will save your score, and it can (probably) restore the score.\n"
			 "But the file doesn't conform to MUP, so it will not work with the MUP interpreter.\n");
		output += i18n("-----------------------------------------------------\n");
		for (bad = badlist_.first(); bad; bad = badlist_.next()) {
			switch (bad->kind) {
				case ERR_NOTE_COUNT:
					output += i18n
						("staff %1; measure  %2 : %3 128th, should be: %4\n").arg(bad->track).
						arg(bad->measure).arg(bad->realcount).arg(bad->shouldbe);
					break;
				case ERR_LYRICS_COUNT:
					output += i18n(
						"staff %1; measure %2: Mup cannot deal with mixed lyrics/non-lyrics notes in the same measure\n").
						arg(bad->track).arg(bad->measure);
					break;
				case ERR_SLUR:
					output += i18n(
						"staff %1; measure %2: Mup up cannot deal with slurs over more than 1 element, besides in 1st or 2nd voice\n").
						arg(bad->track).arg(bad->measure);
					break;
				case ERR_LONELY_TRILL:
					output += i18n(
						"staff %1; measure %2: Mup cannot deal with trills without \"tr\" sign\n").
						arg(bad->track).arg(bad->measure);
					break;
				case ERR_IRREGULAER:
					output += i18n
						("Staff %1 has irregular key, this may not work with MUP.\n")
						.arg(bad->track);
					break;
				case ERR_TOO_MANY_VOICES:
					output += i18n
						("Staff %1 has too many voices! MUP accepts up to 3 voices.\n")
						.arg(bad->track);
					break;
				case ERR_DRUM_STAFF:
					output += i18n
						("Staff %1 contains drum staff. This cannot be expressed in MUP.\n")
						.arg(bad->track);
					break;
				case ERR_BEAMS_IN_GRACES:
					output += i18n(
						"staff %1; measure %2: Mup cannot deal with costum beams in graces\n").
						arg(bad->track).arg(bad->measure);
					break;
				case ERR_GRACE_AFTER:
					output += i18n(
						"staff %1; measure %2: Mup cannot deal with graces after\n").
						arg(bad->track).arg(bad->measure);
					break;
				case ERR_CLEFCHANGE:
					output += i18n
						("staff %1; measure %2: MUP cannot deal with clef changes in the middle of measure\n"
						).arg(bad->track).arg(bad->measure);
					break;
				case ERR_KEYCHANGE:
					output += i18n
						("staff %1; measure %2: MUP cannot deal with key changes in the middle of measure\n"
						).arg(bad->track).arg(bad->measure);
					break;
				case ERR_MIXED_GRIDS:
					output += i18n
						("MUP cannot deal with some chord annotations with and some without grids\n");
					break;
				case ERR_TO_MANY_BRACKETS:
					output += i18n
						("MUP cannot deal with more than one bracket per system\n");
					break;
				case ERR_NESTED_BRACKETS:
					output += i18n
						("MUP cannot deal with nested brackets in braces\n");
					break;
				case ERR_GRACE_SLUR:
					output += i18n
						("MUP cannot correctly write more than one slurred grace\n");
					break;
			}
		}
		mupWarn_->setOutput(&output);
		mupWarn_->show();
	}

	return true;
}

void NFileHandler::writeScoreInfo(int staff_nr, NVoice *voi, bool firstcall, NMainFrameWidget *mainWidget) {
	NMusElement *elem;
	NVoice *voice_elem;
	int voice_count, i;
	NClef *clef;
	NKeySig *ksig;
	NTimeSig *timesig;
	NStaff *actual_staff;
	bool staff_number_written = false;
	bool keysigWritten = false;

	actual_staff = voi->getStaff();
	elem = voi->getCurrentPosition();
	if (!elem) return;
	while(1) {
		switch(elem->getType()) {	
			case T_CLEF: 
				if (musicmode_) {
					musicmode_ = false;
					out_ << "score" << endl;
				}
				if (!staff_number_written) {
					out_ << "staff " << staff_nr << endl;
					staff_number_written = true;
					if (firstcall) {
						voice_count = actual_staff->voiceCount();
						if (voice_count > 1) {
							out_ << "\tvscheme = " << voice_count << "o" << endl;
							if (voice_count > 3) {
								badmeasure *bad = new badmeasure(ERR_TOO_MANY_VOICES, staff_nr, bar_nr_, 3 /* dummy */, countof128th_);
								badlist_.append(bad);
							}
						}
						if (!actual_staff->staffName_.isEmpty()) {
							QString staffName = actual_staff->staffName_;
							staffName.replace('\\', "\\\\"); /* replace all backslashes with \\ two character backslashes */
							staffName.replace (newLines_, "\\n"); /* replace all newlines with \n two character symbols */
							staffName.replace('"', "\\\""); /* replace all double quotes with \" two character symbols */

							out_ << "label = \"" << staffName.utf8() << '"' << endl;
						}
						out_ << "// overlength = " << actual_staff->overlength_ << endl;
						out_ << "// underlength = " << actual_staff->underlength_ << endl;
						out_ << "// lyricsdist = " << actual_staff->staff_props_.lyricsdist << endl;
						if (actual_staff->transpose_) {
							out_ << "// playtransposd = " << actual_staff->transpose_ << endl;
						}
						for (i = 0; i < voice_count; i++) {
							voice_elem = actual_staff->getVoiceNr(i);
							if (voice_elem->yRestOffs_ != 0) {
								out_ << "// yrestoffs " << (i + 1) << " = " << voice_elem->yRestOffs_ << endl;
							}
							switch(voice_elem->stemPolicy_) {
							case STEM_POL_UP: out_ << "// stempolicy " << (i + 1) << " = stemup" << endl;break;
							case STEM_POL_DOWN: out_ << "// stempolicy " << (i + 1) << " = stemdown" << endl;break;
							}
						}
					}
				}
				clef = (NClef *) elem;
				hasClef_ = writeClef(clef, staff_nr);
				actual_staff->actualClef_.change(clef);
				break;
			case T_KEYSIG:
				   if (musicmode_) {
				   musicmode_ = false;
				   out_ << "score" << endl;
				}
				if (!staff_number_written) {
					out_ << "staff " << staff_nr << endl;
					staff_number_written = true;
				}
				ksig = (NKeySig *) elem;
				writeKeySig(ksig, staff_nr);
				keysigWritten = true;
				break;
			case T_TIMESIG:
				timesig = (NTimeSig *) elem;
				if (curr_num_ == timesig->getNumerator() && curr_denom_ == timesig->getDenominator()) break;
			        if (musicmode_) {
				   musicmode_ = false;
				   out_ << "score" << endl;
				}
				out_ << "score" << endl;
				out_ << "\ttime = " << timesig->getNumerator() <<
					'/' << timesig->getDenominator() << endl;
				countof128th_ = timesig->numOf128th();
				curr_num_ = timesig->getNumerator();
				curr_denom_ = timesig->getDenominator();
				break;
			default:
				return;
		}
		elem = voi->getNextPosition();
		if (!elem) return;
	}
}

int NFileHandler::writeStaffUntilBar(int staff_nr, NVoice *voi, bool first, int multirestlength, int *measure_start_time) {
	NMusElement *elem;
	QString *lyrics;
	int total = 0;
	int part, tt;
	NNote *note, *note2;
	NChord *chord;
	NSign *sign;
	NClef *clef;
	NKeySig *ksig;
	NText *textSign;
	bool ok;
	int tupletsum;
	int subtype;
	int dist;
	int dest_measure_start_time;
	int count_of_measures;
	QString timestring;
	int i, j;
	bool loop1 = true;
	double starttime, endtime;
	badmeasure *bad;
	bool inGrace = false;
	bool beam_grace_problem_reported = false;
	NStaff *actual_staff;
	NChordDiagram *diag;
	NChord *partner;
	int numOfNonLyricsChordsInStaff[NUM_LYRICS];
	

#if GCC_MAJ_VERS > 2
	for (i = 0; i < NUM_LYRICS; i++) {
		delete lyricsLine_[i];
		lyricsLine_[i] = new ostringstream();
		numOfNonLyricsChordsInStaff[i] = 0;
	}
#else
	for (i = 0; i < NUM_LYRICS; i++) {
		lyricsLine_[i]->seekp(0);
		numOfNonLyricsChordsInStaff[i] = 0;
	}
#endif
	*measure_start_time = -1;
	actual_staff = voi->getStaff();
	elem = voi->getCurrentPosition();
	if (first) {
		while (elem && elem->getType() != T_CHORD && elem->getType() != T_REST) {
			switch (elem->getType()) {
				case T_SIGN:
		  			switch (elem->getSubType ()) {
						case REPEAT_OPEN:
								subtype = elem->getSubType();
				 				elem = voi->getNextPosition();
				 				return subtype;
								break;
						case REPEAT_CLOSE:
								subtype = elem->getSubType();
				 				elem = voi->getNextPosition();
								repaetTime_ = ((NSign *) elem)->getRepeatCount();
				 				return subtype;
								break;
						case TEMPO_SIGNATURE:
								sign = (NSign *) elem;
									pending_tempo_sigs_.append(new pending_sign_information_class(0.0, sign));
								break;
						case VOLUME_SIG: 
								sign = (NSign *) elem;
								pending_volume_sigs_.append(new pending_sign_information_class( 0.0, sign));
								break;
						case PROGRAM_CHANGE:
								sign = (NSign *) elem;
								pending_program_changes_.append(new pending_sign_information_class(
									0.0, sign));
								break;
					}
				 	elem = voi->getNextPosition();
					break;
				case T_TEXT:
					textSign = (NText *) elem;
					if (textSign->getSubType() == TEXT_UPTEXT) {
						if (textsigns_above_.isEmpty()) {
							textsigns_above_.sprintf("rom (14) above %d: ",  staff_nr);
						}
						timestring = "1 \"";
						timestring += textSign->getText() + "\";";
						textsigns_above_ += timestring;
					}
					else {
						if (textsigns_below_.isEmpty()) {
							textsigns_below_.sprintf("rom (14) below %d: ",  staff_nr);
						}
						timestring = "1 \"";
						timestring += textSign->getText() + "\";";
						textsigns_below_ += timestring;
					}
				 	elem = voi->getNextPosition();
					break;
				default: elem = voi->getNextPosition();
					break;
			}
		  }
	
		  
	}
	if (elem == 0) return 0;
	if (multirestlength) {
		if (staff_nr == 1) {
			out_ << "multirest " << multirestlength << endl;
		}
		while(elem && elem->getType() != T_SIGN && !(elem->getSubType() & BAR_SYMS))  {
			elem = voi->getNextPosition();
		}
		loop1 = false;
		total = MULTIPLICATOR*countof128th_;
	}
	if (*measure_start_time < 0) {
		*measure_start_time = elem->midiTime_;
	}
	do {
		switch (elem->getType()) {
			case T_CHORD: if (loop1) {
					loop1 = false;
					out_ << staff_nr << " 1: ";
				     }
				     some_notes_or_rests_written_ = true;
				     chord = (NChord *) elem;
				     part = elem->getSubType(); 
				     if (!(elem->status_ & STAT_GRACE)) {
					     if (elem->status_ & STAT_TUPLET) {
						total += elem->getPlaytime() * part / elem->getNumNotes();
						tupletsum += part;
					     }
					     else {
						total += part;
					     }
					     switch (elem->status_ & DOT_MASK) {
						case 1: total += part / 2; break;
						case 2: total += 3 * part / 4; break;
					     }
				     }
				     if ((elem->status_ & STAT_TUPLET) && !voi->inTuplet_) {
					voi->inTuplet_ = true;
					tupletsum = part;
					out_ << "{ ";
				     }
				     if (total > MULTIPLICATOR*countof128th_) {
					total = 0;
					bad = new badmeasure(ERR_NOTE_COUNT, staff_nr, bar_nr_, total / 3, countof128th_);
					badlist_.append(bad);
				     }
				     if (chord->status_ & STAT_SFORZ || chord->status_ & STAT_PORTA ||
				         chord->status_ & STAT_STPIZ || chord->status_ & STAT_SFZND ||
					 chord->status_ & STAT_STACC || chord->status_ & STAT_FERMT) {
				    	    out_ << "[with ";
				    	    if (chord->status_ & STAT_STACC)
						out_ << ".";
				    	    if (chord->status_ & STAT_SFORZ) 
					        out_ << "^";
					    if (chord->status_ & STAT_PORTA)
					        out_ << "-";
					    if (chord->status_ & STAT_STPIZ)
					        out_ << ",";
					    if (chord->status_ & STAT_SFZND)
					        out_ << ">";
					    if (chord->status_ & STAT_FERMT)
						out_ << "\"\\(ferm)\"";
				    	    out_ << ']';
				     }
				     if (chord->status_ & STAT_GRACE) {
						inGrace = true;
						if (!beam_grace_problem_reported && (chord->status_ & STAT_BEAMED)) {
							beam_grace_problem_reported = true;

							bad = new badmeasure(ERR_BEAMS_IN_GRACES, staff_nr, bar_nr_, total / 3, countof128th_);
							badlist_.append(bad);
						}
						out_ << "[ grace";
						if (chord->getSubType() == INTERNAL_MARKER_OF_STROKEN_GRACE) {
							out_ << "; slash 1";
						}
					    out_ << "] ";
				     }
				     else {
					inGrace = false;
					beam_grace_problem_reported = false;
				     }
				     if (!(chord->status_ & STAT_GRACE) && chord->getSubType() <= HALF_LENGTH) {
					if (((chord->status_ & STAT_STEM_UP) && 
						(chord->getNoteList()->first()->line > 3 || voi->stemPolicy_ == STEM_POL_DOWN))  ||
					    (!(chord->status_ & STAT_STEM_UP) && 
					    	(chord->getNoteList()->first()->line  < 4 || voi->stemPolicy_ == STEM_POL_UP))) {
						out_ << ((chord->status_ & STAT_STEM_UP) ? "[up]" : "[down] ");
					}
				     }
				     if (part == DOUBLE_WHOLE_LENGTH) {
					out_ << "1/2";
				     }
				     else if ((chord->status_ & STAT_GRACE) && (chord->getSubType() == INTERNAL_MARKER_OF_STROKEN_GRACE)) {
					out_ << "8";
				     }
				     else {
				     	out_ << WHOLE_LENGTH / part;
				     }
				     switch (elem->status_ & DOT_MASK) {
					case 1:	out_ << "."; break;
					case 2:	out_ << ".."; break;
				     }
			  	     for (note = elem->getNoteList()->first(); note; note = elem->getNoteList()->next()) {
					     pitchOut(note, &(actual_staff->actualClef_), true);
					     if (!drum_problem_written_ && (note->status & BODY_MASK)) {
						drum_problem_written_ = true;
						bad = new badmeasure(ERR_DRUM_STAFF, staff_nr, 0 /*dummy */, 3 /* dummy */, countof128th_);
						badlist_.append(bad);
					     }
					     /* if (loop2) { */
						if ((chord->status_ & STAT_SLURED) && (chord->status_ & STAT_GRACE)) { /* otherwise it is made by phrases */
							dist = voi->computeSlurDist(chord);
							if (dist > 1) { // possibly failes if there is a bar :-(
								bad = new badmeasure(ERR_GRACE_SLUR, staff_nr, bar_nr_, 0, 0);
								badlist_.append(bad);
								out_ << "<" << dist << ">";
							}
							else {
								partner = chord->getSlurPartner();
								note2 = partner->getNoteList()->first();
								out_ << "<" ;
					    			pitchOut( note2, &(actual_staff->actualClef_), false);
								out_ << ">" ;
							}
						}
					   /* } */
#ifdef IS_MADE_WITH_PHRASES
					     if (loop2) {
						loop2 = false;
						if (chord->status_ & STAT_SLURED) {
							dist = voi->computeSlurDist(chord);
							if (dist > 1) {
								bad = new badmeasure(ERR_SLUR, staff_nr, bar_nr_, 0, 0);
								badlist_.append(bad);
								out_ << "<" << dist << ">";
							}
							else {
								partner = chord->getSlurPartner();
								note2 = partner->getNoteList()->first();
								out_ << "<" ;
					    			pitchOut( note2, &(actual_staff->actualClef_), false);
								out_ << ">" ;
							}
						}
					    }
#endif
				     }
				     if (elem->status_ & STAT_BEAMED) {
					if (!voi->inBeam_) {
						out_ << " bm";
						voi->inBeam_ = true;
					 }
				     	 else if (chord->lastBeamed()) {
						out_ << " ebm";
						voi->inBeam_ = false;
					 }
				     }
				     out_ << "; ";
				     if (elem->status_ & STAT_LAST_TUPLET) {
					voi->inTuplet_ = false;
					out_ << " } above  " <<
						computeTripletString(tupletsum, elem->getNumNotes(), elem->getPlaytime(), &ok) << "; ";
					if (!ok) {
						bad = new badmeasure(ERR_TUPLET, staff_nr, bar_nr_, total / 3, countof128th_);
						badlist_.append(bad);
					}
				     }
				     if (elem->trill_) {
					dest_measure_start_time = *measure_start_time;
					tt = voi->findTimeOfTrillEnd((NChord *) elem, &dest_measure_start_time, &count_of_measures);
					if (elem->trill_ > 0) {
#if GCC_MAJ_VERS > 2
						if (ornaments_->tellp() < 1) {
#else
						if (ornaments_->pcount() < 1) {
#endif
							*ornaments_ << "mussym above " << staff_nr << ": ";
						}
					}
					else {
#if GCC_MAJ_VERS > 2
						if (trillsyms_->tellp() < 1) {
#else
						if (trillsyms_->pcount() < 1) {
#endif
							*trillsyms_ << "rom above " << staff_nr << ": ";
						}
					}
					starttime = ((elem->midiTime_ - *measure_start_time) / MULTIPLICATOR) / (double) (128 / curr_denom_);
					endtime   = ((tt - dest_measure_start_time) / MULTIPLICATOR) / (double) (128 / curr_denom_);
					if (count_of_measures) {
					 	timestring.sprintf("%dm + %f", count_of_measures, endtime + 1.0);
					}
					else {
						timestring.sprintf("%f", endtime + 1.0);
					}
					if (elem->trill_ > 0) {
						*ornaments_ << (starttime + 1.0) << " \"tr\" til " << timestring << ';';
					}
					else {
						*trillsyms_ << (starttime + 1.0) << " \"~\" til " << timestring << ';';
					}
				     }
				     if (elem->dynamic_) {
					dest_measure_start_time = *measure_start_time;
					tt = voi->findTimeOfDynamicEnd(chord, measure_start_time,
						 &dest_measure_start_time, &count_of_measures);
					if (tt < 0) {
						 *os_ << "damaged dynamic in staff " << staff_nr << " measure: " << bar_nr_ << '\0';
#if GCC_MAJ_VERS > 2
						KMessageBox::sorry(0, QString(os_->str().c_str()), kapp->makeStdCaption(i18n("???")));
#else
						KMessageBox::sorry(0, QString(os_->str()), kapp->makeStdCaption(i18n("???")));
#endif
					}
					else {
					   starttime = ((chord->midiTime_ - *measure_start_time) / MULTIPLICATOR) / (double) (128 / curr_denom_);
					   endtime   = ((tt - dest_measure_start_time) / MULTIPLICATOR) / (double) (128 / curr_denom_);
					   if (count_of_measures) {
					   	timestring.sprintf("%dm + %f", count_of_measures, endtime + 1.0);
					   }
					   else {
						timestring.sprintf("%f", endtime + 1.0);
					   }
					   *dynamics_ << (chord->dynamicAlign_ ? '<' : '>') << "  below " << staff_nr << ": " <<
							(starttime + 1.0) << " til " << timestring << ';';
				        }
				     }
				     if (elem->va_) {
					dest_measure_start_time = *measure_start_time;
					tt = voi->findTimeOfVaEnd((NChord *) elem, &dest_measure_start_time, &count_of_measures);
					if (elem->va_ > 0) {
#if GCC_MAJ_VERS > 2
						if (valines_->tellp() < 1) {
#else
						if (valines_->pcount() < 1) {
#endif
							*valines_ << "octave above " << staff_nr << ": ";
						}
					}
					else {
#if GCC_MAJ_VERS > 2
						if (valines_->tellp() < 1) {
#else
						if (valines_->pcount() < 1) {
#endif
							*valines_ << "octave below " << staff_nr << ": ";
						}
					}
					starttime = ((elem->midiTime_ - *measure_start_time) / MULTIPLICATOR) / (double) (128 / curr_denom_);
					endtime   = ((tt - dest_measure_start_time) / MULTIPLICATOR) / (double) (128 / curr_denom_);
					if (count_of_measures) {
					 	timestring.sprintf("%dm + %f", count_of_measures, endtime + 1.0);
					}
					else {
						timestring.sprintf("%f", endtime + 1.0);
					}
					if (elem->va_ > 0) {
						*valines_ << (starttime + 1.0) << " \"8va\" til " << timestring << ';';
					}
					else {
						*valines_ << (starttime + 1.0) << " \"8va bassa\" til " << timestring << ';';
					}
				     }
				     if ((elem->status_ & STAT_SLURED) && !(chord->status_ & STAT_GRACE)) {
					dest_measure_start_time = *measure_start_time;
					chord = ((NChord *) elem);
					tt = voi->findTimeOfSlurEnd((NChord *) elem, &dest_measure_start_time, &count_of_measures);
#if GCC_MAJ_VERS > 2
					if (phrases_->tellp() < 1) {
#else
					if (phrases_->pcount() < 1) {
#endif
						*phrases_ << "phrase  " << staff_nr << ": ";
					}
					starttime = ((elem->midiTime_ - *measure_start_time) / MULTIPLICATOR) / (double) (128 / curr_denom_);
					endtime   = ((tt - dest_measure_start_time) / MULTIPLICATOR) / (double) (128 / curr_denom_);
					if (count_of_measures) {
					 	timestring.sprintf("%dm + %f", count_of_measures, endtime + 1.0);
					}
					else {
						timestring.sprintf("%f", endtime + 1.0);
					}
					*phrases_ << (starttime + 1.0) << " til " << timestring << ';';
				     }
				     if (elem->status_ & STAT_ARPEGG) {
					   starttime = ((chord->midiTime_ - *measure_start_time) / MULTIPLICATOR) / (double) (128 / curr_denom_);
					   if (rolls_.isEmpty()) {
					   	rolls_.sprintf("roll %d 1: ",  staff_nr);
					   }
					   timestring.sprintf("%f;", starttime + 1.0);
					   rolls_ += timestring;
				     }
				     if (elem->status2_ & STAT2_PEDAL_ON) {
					   starttime = ((chord->midiTime_ - *measure_start_time) / MULTIPLICATOR) / (double) (128 / curr_denom_);
					   if (pedals_.isEmpty()) {
					   	pedals_.sprintf("pedal %d: ",  staff_nr);
					   }
					   timestring.sprintf("%f;", starttime + 1.0);
					   pedals_ += timestring;
				     }
				     if (elem->status2_ & STAT2_PEDAL_OFF) {
					   starttime = ((chord->midiTime_ - *measure_start_time) / MULTIPLICATOR) / (double) (128 / curr_denom_);
					   if (pedals_.isEmpty()) {
					   	pedals_.sprintf("pedal %d: ",  staff_nr);
					   }
					   timestring.sprintf("%f*;", starttime + 1.0);
					   pedals_ += timestring;
				     }
				     if ((diag = elem->getChordChordDiagram())) {
					starttime = ((elem->midiTime_ - *measure_start_time) / MULTIPLICATOR) / (double) (128 / curr_denom_);
					writeChord(staff_nr, starttime, diag);
				     }
				     if (!(chord->getNoteList()->first()->status & STAT_PART_OF_TIE) && !(chord->status_ & STAT_GRACE)) {
				        for (i = 0; i < NUM_LYRICS; i++) {
				     		lyrics = chord->getLyrics(i);
				     		if (lyrics) {
							for (j = 0; j < numOfNonLyricsChordsInStaff[i]; j++) {
								(*lyricsLine_[i]) << " <> ";
							}
							numOfNonLyricsChordsInStaff[i] = -1;
							(*lyricsLine_[i]) << lyrics2MUP(lyrics).utf8() << ' ';
							if (i+1 > lyricsLineCount_) lyricsLineCount_ = i+1;
				     		}
						else if (numOfNonLyricsChordsInStaff[i] == -1) {
							(*lyricsLine_[i]) << " <> ";
						}
						else {
							numOfNonLyricsChordsInStaff[i]++;
						}
					}
				     }
				     break;
			case T_REST: if (inGrace) {
					inGrace = false;
					beam_grace_problem_reported = false;
					bad = new badmeasure(ERR_GRACE_AFTER, staff_nr, bar_nr_, total / 3, countof128th_);
					badlist_.append(bad);
				     }
				     if (loop1) {
					loop1 = false;
					out_ << staff_nr << " 1: ";
				     }
				     some_notes_or_rests_written_ = true;
				     part = elem->getSubType(); 
				     if (part == MULTIREST) {
					bad = new badmeasure(ERR_MULTIREST, staff_nr, bar_nr_, total / 3, countof128th_);
					fatallist_.append(bad);
					if (divide_multi_rest(staff_nr, 1, ((NRest *) elem)->getMultiRestLength())) {
						total = MULTIPLICATOR*countof128th_;
						return SIMPLE_BAR;
					}
					break;
				     }

				     if (elem->status_ & STAT_TUPLET) {
					total += elem->getPlaytime() * part / elem->getNumNotes();
					tupletsum += part;
				     }
				     else {
					total += part;
				     }
				     switch (elem->status_ & DOT_MASK) {
					case 1: total += part / 2; break;
					case 2: total += 3 * part / 4; break;
				     }
				     if ((elem->status_ & STAT_TUPLET) && !voi->inTuplet_) {
					voi->inTuplet_ = true;
					tupletsum = part;
					out_ << "{ ";
				     }
				     if (total > MULTIPLICATOR*countof128th_) {
					total = 0;
					bad = new badmeasure(ERR_NOTE_COUNT, staff_nr, bar_nr_, total / 3, countof128th_);
					badlist_.append(bad);
				     }
				     if (elem->status_ & STAT_FERMT) {
					out_ << "[with ";
					out_ << "\"\\(ferm)\"";
					out_ << ']';
				     }
				     out_ << WHOLE_LENGTH / part;
				     switch (elem->status_ & DOT_MASK) {
					case 1:	out_ << "."; break;
					case 2:	out_ << ".."; break;
				     }
				     out_ << ((elem->status_ & STAT_HIDDEN) ? "s; " : "r; ");
				     if ((diag = elem->getChordChordDiagram())) {
					starttime = ((elem->midiTime_ - *measure_start_time) / MULTIPLICATOR) / (double) (128 / curr_denom_);
					writeChord(staff_nr, starttime, diag);
				     }
				     if (elem->status_ & STAT_LAST_TUPLET) {
					voi->inTuplet_ = false;
					out_ << " } above  " <<
						computeTripletString(tupletsum, elem->getNumNotes(), elem->getPlaytime(), &ok) << "; ";
					if (!ok) {
						bad = new badmeasure(ERR_TUPLET, staff_nr, bar_nr_, total / 3, countof128th_);
						badlist_.append(bad);
					}
				     }
				     break;
			case T_SIGN: if (inGrace) {
					inGrace = false;
					beam_grace_problem_reported = false;
					bad = new badmeasure(ERR_GRACE_AFTER, staff_nr, bar_nr_, total / 3, countof128th_);
					badlist_.append(bad);
				     }
				     switch (elem->getSubType ()) {
					case SIMPLE_BAR: 
					case END_BAR: 
					case REPEAT_OPEN:
					case REPEAT_CLOSE:
					case REPEAT_OPEN_CLOSE:
					case DOUBLE_BAR:
							 if (loop1) {
								loop1 = false;
								out_ << staff_nr << " 1: ";
							 }
							 if (total != MULTIPLICATOR*countof128th_) {
								bad = new badmeasure(ERR_NOTE_COUNT, staff_nr, bar_nr_, total / 3, countof128th_);
								badlist_.append(bad);
							 }
							 subtype = elem->getSubType();
							 if (elem->getSubType () == REPEAT_CLOSE) {
								repaetTime_ = ((NSign *) elem)->getRepeatCount();
							 }
							 elem = voi->getNextPosition();
							 if (elem && elem->getType() == T_SIGN  && (elem->getSubType() & SPECIAL_ENDING)) {
								subtype |= elem->getSubType ();
								elem = voi->getNextPosition();
							 }
							 out_ << endl;
							 return subtype;
					case TEMPO_SIGNATURE:
							sign = (NSign *) elem;
							pending_tempo_sigs_.append(new pending_sign_information_class(
								((sign->midiTime_ - *measure_start_time) / MULTIPLICATOR) / (double) (128 / curr_denom_), sign));
							break;
					case VOLUME_SIG:
							sign = (NSign *) elem;
							pending_volume_sigs_.append(new pending_sign_information_class(
								((sign->midiTime_ - *measure_start_time) / MULTIPLICATOR) / (double) (128 / curr_denom_), sign));
							break;
					case PROGRAM_CHANGE:
							sign = (NSign *) elem;
							pending_program_changes_.append(new pending_sign_information_class(
								((sign->midiTime_ - *measure_start_time) / MULTIPLICATOR) / (double) (128 / curr_denom_), sign));
							break;
					case SEGNO:	starttime = ((elem->midiTime_ - *measure_start_time) / MULTIPLICATOR) / (double) (128 / curr_denom_);
					   		if (signs_.isEmpty()) {
					   			signs_.sprintf("rom above %d: ",  staff_nr);
					   		}
					   		timestring.sprintf("%f \"\\(sign)\";", starttime + 1.0);
					   		signs_ += timestring;
							break;
					case DAL_SEGNO:	starttime = ((elem->midiTime_ - *measure_start_time) / MULTIPLICATOR) / (double) (128 / curr_denom_);
					   		if (signs_.isEmpty()) {
					   			signs_.sprintf("rom above %d: ",  staff_nr);
					   		}
					   		timestring.sprintf("%f \"D.S.\";", starttime + 1.0);
					   		signs_ += timestring;
							break;
					case DAL_SEGNO_AL_FINE:
							starttime = ((elem->midiTime_ - *measure_start_time) / MULTIPLICATOR) / (double) (128 / curr_denom_);
					   		if (signs_.isEmpty()) {
					   			signs_.sprintf("rom above %d: ",  staff_nr);
					   		}
					   		timestring.sprintf("%f \"D.S. al Fine\";", starttime + 1.0);
					   		signs_ += timestring;
							break;
					case DAL_SEGNO_AL_CODA:
							starttime = ((elem->midiTime_ - *measure_start_time) / MULTIPLICATOR) / (double) (128 / curr_denom_);
					   		if (signs_.isEmpty()) {
					   			signs_.sprintf("rom above %d: ",  staff_nr);
					   		}
					   		timestring.sprintf("%f \"D.S. al \\(coda)\";", starttime + 1.0);
					   		signs_ += timestring;
							break;
					case CODA:	starttime = ((elem->midiTime_ - *measure_start_time) / MULTIPLICATOR) / (double) (128 / curr_denom_);
					   		if (signs_.isEmpty()) {
					   			signs_.sprintf("rom above %d: ",  staff_nr);
					   		}
					   		timestring.sprintf("%f \"\\(coda)\";", starttime + 1.0);
					   		signs_ += timestring;
							break;
					case FINE:	starttime = ((elem->midiTime_ - *measure_start_time) / MULTIPLICATOR) / (double) (128 / curr_denom_);
					   		if (signs_.isEmpty()) {
					   			signs_.sprintf("rom above %d: ",  staff_nr);
					   		}
					   		timestring.sprintf("%f \"Fine\";", starttime + 1.0);
					   		signs_ += timestring;
							break;
					case RITARDANDO:
							starttime = ((elem->midiTime_ - *measure_start_time) / MULTIPLICATOR) / (double) (128 / curr_denom_);
					   		if (signs_.isEmpty()) {
					   			signs_.sprintf("rom above %d: ",  staff_nr);
					   		}
					   		timestring.sprintf("%f \"ritard.\";", starttime + 1.0);
					   		signs_ += timestring;
							break;
					case ACCELERANDO:starttime = ((elem->midiTime_ - *measure_start_time) / MULTIPLICATOR) / (double) (128 / curr_denom_);
					   		if (signs_.isEmpty()) {
					   			signs_.sprintf("rom above %d: ",  staff_nr);
					   		}
					   		timestring.sprintf("%f \"accel.\";", starttime + 1.0);
					   		signs_ += timestring;
							break;
				     }
				     break;
			case T_CLEF: 
				clef = (NClef *) elem;
				out_ << "clefchange{";
				writeClef(clef, staff_nr);
				out_ << '}';
				actual_staff->actualClef_.change(clef);
				bad = new badmeasure(ERR_CLEFCHANGE, staff_nr, bar_nr_, 3 /* dummy */, countof128th_);
				badlist_.append(bad);
				break;
			case T_KEYSIG:
				ksig = (NKeySig *) elem;
				out_ << "keychange{";
				writeKeySig(ksig, staff_nr, true);
				out_ << '}';
				bad = new badmeasure(ERR_KEYCHANGE, staff_nr, bar_nr_, 3 /* dummy */, countof128th_);
				badlist_.append(bad);
				break;
			case T_TEXT:
				if (inGrace) {
					inGrace = false;
					beam_grace_problem_reported = false;
					bad = new badmeasure(ERR_GRACE_AFTER, staff_nr, bar_nr_, total / 3, countof128th_);
					badlist_.append(bad);
				}
				starttime = ((elem->midiTime_ - *measure_start_time) / MULTIPLICATOR) / (double) (128 / curr_denom_);
				textSign = (NText *) elem;
				if (textSign->getSubType() == TEXT_UPTEXT) {
					if (textsigns_above_.isEmpty()) {
						textsigns_above_.sprintf("rom (14) above %d: ",  staff_nr);
					}
					timestring.sprintf("%f \"", starttime + 1.0);
					timestring += textSign->getText() + "\";";
					textsigns_above_ += timestring;
				}
				else {
					if (textsigns_below_.isEmpty()) {
						textsigns_below_.sprintf("rom (14) below %d: ",  staff_nr);
					}
					timestring.sprintf("%f \"", starttime + 1.0);
					timestring += textSign->getText() + "\";";
					textsigns_below_ += timestring;
				}
				break;
		}
		elem = voi->getNextPosition();
	}
	while (elem);
	out_ << endl;
	return SIMPLE_BAR;
}

void NFileHandler::writeTempoSig(double starttime, NSign *temposig) {
	out_ << "midi all: " << (starttime + 1.0) << " \"tempo=" << temposig->getTempo() <<
		 "\";" << endl;
	out_ << "rom above all: " << (starttime + 1.0) << " \"( \\(sm4n) = " << 
	temposig->getTempo() << " )\";" << endl;
}

void NFileHandler::writeChord(int staff_nr, double starttime, NChordDiagram *diag) {
	chordDiagramName *diagname;
	int i;
#if GCC_MAJ_VERS > 2
	if (chordline_->tellp() < 1) {
#else
	if (chordline_->pcount() < 1) {
#endif
		*chordline_ << "rom chord " << staff_nr << ": ";
	}
	for (diagname = chordDiagramList_.first(); diagname; diagname = chordDiagramList_.next()) {
		if (diag->isEqual(diagname->cdiagramm)) {
			*chordline_ << (starttime+1.0) << " \"";
			for (i = 0; i < diagname->NumOfUnderscores; i++) {
				*chordline_ << '_';
			}
			*chordline_ << diag->getChordName() << "\"; ";
			return;
		}
	}
	NResource::abort("writeChord");
}

void NFileHandler::writeVolSig(double starttime, int staff_nr, NSign *volsig) {
	out_ << "midi " << staff_nr << " 1: " << (starttime + 1.0) << " \"parameter=7," << volsig->getVolume() << "\";" << endl;
	out_ << "boldital above "  << staff_nr << ": " << (starttime + 1.0) << " \"";
	switch(volsig->getVolType()) {
		case V_PPPIANO : out_ << "ppp"; break;
		case V_PPIANO  : out_ << "pp"; break;
		case V_PIANO   : out_ << "p"; break;
		case V_MPIANO  : out_ << "mp"; break;
		case V_FORTE   : out_ << "f"; break;
		case V_FFORTE  : out_ << "ff"; break;
		case V_FFFORTE : out_ << "fff"; break;
		default        : out_ << "mf"; break;
	}
	out_ << "\"; // volumesign" << endl;
}

void NFileHandler::writeProgramChange(double starttime, int staff_nr, NSign *pChange) {
	out_ << "midi " << staff_nr << " 1: " << (starttime + 1.0) << " \"program=" << pChange->getProgram() << "\";" << endl;
}

void NFileHandler::writeStaffLayout(NMainFrameWidget *mainWidget, int staffCount) {
	int i, j;
	bool overlap_reported = false;
	bool symbol_written;
	badmeasure *bad;

	symbol_written = false;
	for (i = 0; i < staffCount; i++) {
		if (mainWidget->braceMatrix_[i].valid) {
			if (symbol_written) {
				out_ << ", ";
			}
			else {
				out_ << "\tbrace = ";
				symbol_written = true;
			}
			out_ << (mainWidget->braceMatrix_[i].beg+1) << '-' << (mainWidget->braceMatrix_[i].end+1);
			for (j = 0; !overlap_reported && j < staffCount; j++) {
				if (mainWidget->bracketMatrix_[j].valid) {
					if (mainWidget->bracketMatrix_[j].beg >= mainWidget->braceMatrix_[i].beg && mainWidget->bracketMatrix_[j].end <= mainWidget->braceMatrix_[i].end) {
						bad = new badmeasure(ERR_NESTED_BRACKETS, 0 /*dummy */, 0 /* dummy */ , 3 /* dummy */, countof128th_ /* dummy */);
						badlist_.append(bad);
						overlap_reported = true;
					}
				}
			}
		}
	}
	if (symbol_written) out_ << endl;
	symbol_written = false;
	for (i = 0; i < staffCount; i++) {
		if (mainWidget->bracketMatrix_[i].valid) {
			if (symbol_written) {
				out_ << ", ";
			}
			else {
				out_ << "\tbracket = ";
				symbol_written = true;
			}
			out_ << (mainWidget->bracketMatrix_[i].beg+1) << '-' << (mainWidget->bracketMatrix_[i].end+1);
		}
	}
	if (symbol_written) out_ << endl;
	symbol_written = false;
	for (i = 0; i < staffCount-1; i++) {
		if (mainWidget->barCont_[i].valid) {
			if (symbol_written) {
				out_ << ", ";
			}
			else {
				out_ << "\tbarstyle = ";
				symbol_written = true;
			}
			out_ << (mainWidget->barCont_[i].beg+1) << '-' << (mainWidget->barCont_[i].end+1);
		}
	}
	if (symbol_written) out_ << endl;

}

int NFileHandler::determineMultiRest(QList<NStaff> *stafflist) {
	NStaff *staff_elem;
	int len;

	staff_elem = stafflist->first();
	len = staff_elem->determineMultiRest();
	if (len == 0) return 0;
	for (staff_elem = stafflist->next(); staff_elem; staff_elem = stafflist->next()) {
		if (staff_elem->determineMultiRest() != len) return 0;
	}
	return len;
}

bool NFileHandler::divide_multi_rest(int staff_nr, int voice_nr, int multirestlength) {
	int part, len2, dotcount;
	int *iptr = pending_multi_rests_[staff_nr - 1] + (voice_nr - 1);

	if (*iptr == 0) {
		*iptr= multirestlength * MULTIPLICATOR * countof128th_;
	}
	if (*iptr > 0) {
		if (*iptr >= MULTIPLICATOR*countof128th_) {
			part = MULTIPLICATOR*countof128th_;
		}
		else {
			part = *iptr;
		}
		*iptr -= part;
		while (part >= MULTIPLICATOR) { 
			len2 = NVoice::quant(part, &dotcount, WHOLE_LENGTH);
			part -= dotcount ? 3 * len2 / 2 : len2;
			out_ << WHOLE_LENGTH / len2;
			if (dotcount) out_ << '.';
			out_ << "r; ";
		}
	}
	if (*iptr != 0) out_ << endl;
	return (*iptr != 0);
}


bool NFileHandler::writeClef(NClef * clef, int voice_nr) {
	switch (clef->getSubType()) {
	case TREBLE_CLEF:
		out_ << "\tclef=treble" << endl;
		switch (clef->getShift()) {
	 		case -12: out_ << "\tdefoct=3" << endl; break; 
	   		case  12: out_ << "\tdefoct=5" << endl; break; 
		}
		break;
	case DRUM_CLEF:
		out_ << "\tclef=drum" << endl;
		if (!drum_problem_written_) {
			drum_problem_written_ = true;
			badlist_.append(new badmeasure(ERR_DRUM_STAFF, voice_nr, 0 /*dummy */, 3 /* dummy */, countof128th_));
		}
		break;
	case SOPRANO_CLEF:
		out_ << "\tclef=soprano" << endl;
		switch (clef->getShift()) {
	 		case -12: out_ << "\tdefoct=3" << endl; break; 
	   		case  12: out_ << "\tdefoct=5" << endl; break; 
		}
		break;
	case ALTO_CLEF:
		out_ << "\tclef=alto" << endl;
		switch (clef->getShift()) {
	 		case -12: out_ << "\tdefoct=3" << endl; break; 
	   		case  12: out_ << "\tdefoct=5" << endl; break; 
		}
		break;
	case TENOR_CLEF:
		out_ << "\tclef=tenor" << endl;
		switch (clef->getShift()) {
	 		case -12: out_ << "\tdefoct=3" << endl; break; 
	   		case  12: out_ << "\tdefoct=5" << endl; break; 
		}
		break;
	case BASS_CLEF:
		out_ << "\tclef=bass" << endl;
		switch (clef->getShift()) {
			case -12: out_ << "\tdefoct=2" << endl; break; 
			case  12: out_ << "\tdefoct=4" << endl; break; 
		}
		break;
	case DRUM_BASS_CLEF:
		out_ << "\tclef=drum_bass" << endl;
		if (!drum_problem_written_) {
			drum_problem_written_ = true;
			badlist_.append(new badmeasure(ERR_DRUM_STAFF, voice_nr, 0 /*dummy */, 3 /* dummy */, countof128th_));
		}
		break;
	default: return false;
	}
	return true;
}

void NFileHandler::writeKeySig(NKeySig * ksig, int voice_nr, bool only_regulaer) {
	status_type kind;
	int count;
	if (ksig == 0) return; /* no clef found => no key signature available */
	if (ksig->isRegular(&kind, &count)) {
		out_ << "\tkey = " << count << ((kind == STAT_CROSS) ? "#" : "&") << endl;
	}
	else {
		if (only_regulaer) {
			badmeasure *bad = new badmeasure(ERR_IRREGULAER, voice_nr, bar_nr_, 3 /* dummy */, countof128th_);
			fatallist_.append(bad);
		}
		else {
			badmeasure *bad = new badmeasure(ERR_IRREGULAER, voice_nr, bar_nr_, 3 /* dummy */, countof128th_);
			badlist_.append(bad);
			out_ << "//\tirregular = " << ksig->toString() << endl;
		}
	}
}


void NFileHandler::writeVoiceElemsTill(int staff_nr, int voice_nr, NVoice *voi, int stopTime, int multirestlength, int measure_start_time) {
	NMusElement *elem;
	int total = 0;
	int part, tt;
	NNote *note, *note2;
	NChord *chord, *partner;
	bool something_written = false;
	int dest_measure_start_time;
	bool inGrace = false;
	bool beam_grace_problem_reported = false;
	bool ok;
	int dist;
	int count_of_measures;
	QString timestring;
	double starttime, endtime;
	int tupletsum;
	bool loop1 = true, loop2;
	badmeasure *bad;
	NStaff *actual_staff;

	actual_staff = voi->getStaff();
	elem = voi->getCurrentPosition();
	if (multirestlength) {
		while(elem && elem->midiTime_ < stopTime) 
			elem = voi->getNextPosition();
		loop1 = false;
	}
	while (elem && elem->midiTime_ < stopTime) {
		switch (elem->getType()) {
			case T_CHORD: if (loop1) {
					loop1 = false;
					out_ << staff_nr << " " << voice_nr << ": ";
				     }
				     part = elem->getSubType(); 
				     if (!(elem->status_ & STAT_GRACE)) {
					if (elem->status_ & STAT_TUPLET) {
						total += elem->getPlaytime() * part / elem->getNumNotes();
						tupletsum += part;
					}
					else {
						total += part;
					}
					switch (elem->status_ & DOT_MASK) {
						case 1: total += part / 2; break;
						case 2: total += 3 * part / 4; break;
				     	}
				     }
				     if ((elem->status_ & STAT_TUPLET) && !voi->inTuplet_) {
					voi->inTuplet_ = true;
					tupletsum = part;
					out_ << "{ ";
				     }
				     if (total > MULTIPLICATOR*countof128th_) {
					total = 0;
					bad = new badmeasure(ERR_NOTE_COUNT, staff_nr, bar_nr_, total / 3, countof128th_);
					badlist_.append(bad);
				     }
				     chord = (NChord *) elem;

				     if (chord->status_ & STAT_SFORZ || chord->status_ & STAT_PORTA ||
				         chord->status_ & STAT_STPIZ || chord->status_ & STAT_SFZND ||
					 chord->status_ & STAT_STACC || chord->status_ & STAT_FERMT) {
				    	    out_ << "[with ";
				    	    if (chord->status_ & STAT_STACC)
						out_ << ".";
				    	    if (chord->status_ & STAT_SFORZ) 
					        out_ << "^";
					    if (chord->status_ & STAT_PORTA)
					        out_ << "-";
					    if (chord->status_ & STAT_STPIZ)
					        out_ << ",";
					    if (chord->status_ & STAT_SFZND)
					        out_ << ">";
					    if (chord->status_ & STAT_FERMT)
						out_ << "\"\\(ferm)\"";
					    out_ << "] ";
				     }
				     if (chord->status_ & STAT_GRACE) {
						inGrace = true;
						if (!beam_grace_problem_reported && (chord->status_ & STAT_BEAMED)) {
							beam_grace_problem_reported = true;

							bad = new badmeasure(ERR_BEAMS_IN_GRACES, staff_nr, bar_nr_, total / 3, countof128th_);
							badlist_.append(bad);
						}
						out_ << "[ grace";
						if (chord->getSubType() == INTERNAL_MARKER_OF_STROKEN_GRACE) {
							out_ << "; slash 1";
						}
					    out_ << "] ";
				     }
				     else {
					inGrace = false;
					beam_grace_problem_reported = false;
				     }
				     if (!(chord->status_ & STAT_GRACE) && chord->getSubType() <= HALF_LENGTH) {
					if (((chord->status_ & STAT_STEM_UP) && 
						(chord->getNoteList()->first()->line > 3 || voi->stemPolicy_ == STEM_POL_DOWN))  ||
					    (!(chord->status_ & STAT_STEM_UP) && 
					    	(chord->getNoteList()->first()->line  < 4 || voi->stemPolicy_ == STEM_POL_UP))) {
						out_ << ((chord->status_ & STAT_STEM_UP) ? "[up]" : "[down] ");
					}
				     }
				     if (part == DOUBLE_WHOLE_LENGTH) {
					out_ << "1/2";
				     }
				     else if ((chord->status_ & STAT_GRACE) && (chord->getSubType() == INTERNAL_MARKER_OF_STROKEN_GRACE)) {
					out_ << "8";
				     }
				     else {
				     	out_ << WHOLE_LENGTH / part;
				     }
				     switch (elem->status_ & DOT_MASK) {
					case 1:	out_ << "."; break;
					case 2:	out_ << ".."; break;
				     }
				     loop2 = true;
			  	     for (note = elem->getNoteList()->first(); note; note = elem->getNoteList()->next()) {
					     pitchOut( note, &(actual_staff->actualClef_), true);
					     if (!drum_problem_written_  && (note->status & BODY_MASK)) {
						drum_problem_written_ = true;
						bad = new badmeasure(ERR_DRUM_STAFF, staff_nr, 0 /*dummy */, 3 /* dummy */, countof128th_);
						badlist_.append(bad);
					     }
					     if (loop2) {
						loop2 = false;
						if ((chord->status_ & STAT_SLURED) && (voice_nr > 2 || (chord->status_ & STAT_GRACE))) { /* otherwise it is made by phrases */
							dist = voi->computeSlurDist(chord);
							if (dist > 1) { /* Note! There is no bar in 2nd, 3rd ... measure. So the index diff can be used */
								if (chord->status_ & STAT_GRACE) {
									bad = new badmeasure(ERR_GRACE_SLUR, staff_nr, bar_nr_, 0, 0);
								}
								else {
									bad = new badmeasure(ERR_SLUR, staff_nr, bar_nr_, 0, 0);
								}
								badlist_.append(bad);
								out_ << "<" << dist << ">";
							}
							else {
								partner = chord->getSlurPartner();
								note2 = partner->getNoteList()->first();
								out_ << "<" ;
					    			pitchOut( note2, &(actual_staff->actualClef_), false);
								out_ << ">" ;
							}
						}
					    }
				     }
				     if (elem->status_ & STAT_BEAMED) {
					if (!voi->inBeam_) {
						out_ << " bm";
						voi->inBeam_ = true;
					 }
				     	 else if (chord->lastBeamed()) {
						out_ << " ebm";
						voi->inBeam_ = false;
					 }
				     }
				     out_ << "; ";
				     if (elem->status_ & STAT_LAST_TUPLET) {
					voi->inTuplet_ = false;
					out_ << " } above  " <<
						computeTripletString(tupletsum, elem->getNumNotes(), elem->getPlaytime(), &ok) << "; ";
					if (!ok) {
						bad = new badmeasure(ERR_TUPLET, staff_nr, bar_nr_, total / 3, countof128th_);
						badlist_.append(bad);
					}
				     }
				     if (!(chord->status_ & STAT_GRACE) && voice_nr == 2 && (elem->status_ & STAT_SLURED)) { /* slurs for voices > 2 are incompatible */
					dest_measure_start_time = measure_start_time;
					chord = ((NChord *) elem);
					tt = voi->findTimeOfSlurEnd((NChord *) elem, &dest_measure_start_time, &count_of_measures);
#if GCC_MAJ_VERS > 2
					if (phrases_->tellp() < 1) {
#else
					if (phrases_->pcount() < 1) {
#endif
						*phrases_ << "phrase  below " << staff_nr << ": ";
					}
					starttime = ((elem->midiTime_ - measure_start_time) / MULTIPLICATOR) / (double) (128 / curr_denom_);
					endtime   = ((tt - dest_measure_start_time) / MULTIPLICATOR) / (double) (128 / curr_denom_);
					if (count_of_measures) {
					 	timestring.sprintf("%dm + %f", count_of_measures, endtime + 1.0);
					}
					else {
						timestring.sprintf("%f", endtime + 1.0);
					}
					*phrases_ << (starttime + 1.0) << " til " << timestring << ';';
				     }
				     if (elem->status_ & STAT_ARPEGG) {
					   starttime = ((chord->midiTime_ - measure_start_time) / MULTIPLICATOR) / (double) (128 / curr_denom_);
					   if (rolls_.isEmpty()) {
					   	rolls_.sprintf("roll %d %d: ", staff_nr, voice_nr);
					   }
					   timestring.sprintf("%f;", starttime + 1.0);
					   rolls_ += timestring;
				     }
				     if (elem->status2_ & (STAT2_PEDAL_ON | STAT2_PEDAL_OFF)) {
					bad = new badmeasure(ERR_PEDAL_IN_2ND, staff_nr, bar_nr_, total / 3, countof128th_);
					fatallist_.append(bad);
				     }
				     something_written = true;
				     break;
			case T_REST: if (inGrace) {
					inGrace = false;
					beam_grace_problem_reported = false;
					bad = new badmeasure(ERR_GRACE_AFTER, staff_nr, bar_nr_, total / 3, countof128th_);
					badlist_.append(bad);
				     }
				     if (loop1) {
					loop1 = false;
					out_ << staff_nr << " " << voice_nr << ": ";
				     }
				     part = elem->getSubType(); 
				     if (elem->status_ & STAT_TUPLET) {
					total += elem->getPlaytime() * part / elem->getNumNotes();
					tupletsum += part;
				     }
				     else {
					total += part;
				     }
				     switch (elem->status_ & DOT_MASK) {
					case 1: total += part / 2; break;
					case 2: total += 3 * part / 4; break;
				     }
				     if ((elem->status_ & STAT_TUPLET) && !voi->inTuplet_) {
					voi->inTuplet_ = true;
					tupletsum = part;
					out_ << "{ ";
				     }
				     if (total > MULTIPLICATOR*countof128th_) {
					total = 0;
					bad = new badmeasure(ERR_NOTE_COUNT, staff_nr, bar_nr_, total / 3, countof128th_);
					badlist_.append(bad);
				     }
				     if (elem->status_ & STAT_FERMT) {
					out_ << "[with ";
					out_ << "\"\\(ferm)\"";
					out_ << ']';
				     }
				     out_ << WHOLE_LENGTH / part;
				     switch (elem->status_ & DOT_MASK) {
					case 1:	out_ << "."; break;
					case 2:	out_ << ".."; break;
				     }
				     out_ << ((elem->status_ & STAT_HIDDEN) ? "s; " : "r; ");
				     if (elem->status_ & STAT_LAST_TUPLET) {
					voi->inTuplet_ = false;
					out_ << " } above  " <<
						computeTripletString(tupletsum, elem->getNumNotes(), elem->getPlaytime(), &ok) << "; ";
					if (!ok) {
						bad = new badmeasure(ERR_TUPLET, staff_nr, bar_nr_, total / 3, countof128th_);
						badlist_.append(bad);
					}
				     }
				     something_written = true;
				     break;
		}
		elem = voi->getNextPosition();
	}
	if (something_written) out_ << endl;
}
	
bool NFileHandler::readStaffs(const char *fname, QList<NVoice> *voilist, QList<NStaff> *stafflist, NMainFrameWidget *mainWidget) {
	FILE *fp;
	int i;
	extern int YYRESTART(FILE *fp);
	extern int YYPARSE();
	extern int YYLINENO;
	int parser_return;
	NVoice *voice_elem;
	NStaff *staff_elem;
	QList<NVoice> newVoices;
	QList<NStaff> newStaffs;
	layoutDef *layoutinfo;
	int staffCount;

	parser_params.fname = fname;
	parser_params.mainWidget = mainWidget;
	parser_params.newStaffs = &newStaffs;
	parser_params.newVoices = &newVoices;

	QString strFname(fname);
	if ((fp = fopen(fname, "r")) == NULL) {
		*os_ << "error opening file " << fname << '\0';
		KMessageBox::sorry
		  (0,
#if GCC_MAJ_VERS > 2
		   QString(os_->str().c_str()),
#else
		   QString(os_->str()),
#endif
		   kapp->makeStdCaption(i18n("???")));
		return false;
	}
	YYRESTART(fp);
	YYLINENO = 1;
	init_parser_variables();
	parser_return = YYPARSE();
	fclose(fp);
	cleanup_parser_variables();
	if (parser_return != 0) {
		while (!newVoices.isEmpty()) {
			newVoices.first();
			newVoices.current()->emptyVoice();
			newVoices.remove();
		}
		return false;
	}
		
	while (!voilist->isEmpty()) {
		voilist->first();
		voilist->current()->emptyVoice();
		voilist->remove();
	}
	for (voice_elem = newVoices.first(); voice_elem; voice_elem = newVoices.next()) {
		voilist->append(voice_elem);
	}
	stafflist->clear();
	for (staff_elem = newStaffs.first(); staff_elem; staff_elem = newStaffs.next()) {
		stafflist->append(staff_elem);
	}
	mainWidget->scTitle_ = parser_params.scTitle_;
	mainWidget->scTitle_.replace("\\\"", "\""); /* replace all \" symbols with " */
	mainWidget->scTitle_.replace("\\\\", "\\"); /* replace all \\ symbols with \ */

	mainWidget->scSubtitle_ = parser_params.scSubtitle_;
	mainWidget->scSubtitle_.replace("\\\"", "\""); /* replace all \" symbols with " */
	mainWidget->scSubtitle_.replace("\\\\", "\\"); /* replace all \\ symbols with \ */

	mainWidget->scAuthor_ = parser_params.scAuthor_;
	mainWidget->scAuthor_.replace("\\\"", "\""); /* replace all \" symbols with " */
	mainWidget->scAuthor_.replace("\\\\", "\\"); /* replace all \\ symbols with \ */

	mainWidget->scLastAuthor_ = parser_params.scLastAuthor_;
	mainWidget->scLastAuthor_.replace("\\\"", "\""); /* replace all \" symbols with " */
	mainWidget->scLastAuthor_.replace("\\\\", "\\"); /* replace all \\ symbols with \ */

	mainWidget->scCopyright_ = parser_params.scCopyright_;
	mainWidget->scCopyright_.replace("\\\"", "\""); /* replace all \" symbols with " */
	mainWidget->scCopyright_.replace("\\\\", "\\"); /* replace all \\ symbols with \ */

	mainWidget->scComment_ = parser_params.scComment_;
	mainWidget->scComment_.replace("\\\"", "\""); /* replace all \" symbols with " */
	mainWidget->scComment_.replace("\\\\", "\\"); /* replace all \\ symbols with \ */

	mainWidget->setParamsEnabled(parser_params.enableParams);
	mainWidget->setSaveWidth(parser_params.paperwidth);
	mainWidget->setSaveHeight(parser_params.paperheight);
	mainWidget->setWithMeasureNums(parser_params.with_measnum);
	delete mainWidget->braceMatrix_;
	delete mainWidget->bracketMatrix_;
	delete mainWidget->barCont_;
	staffCount = stafflist->count();
	mainWidget->braceMatrix_ = new layoutDef[staffCount];
	mainWidget->bracketMatrix_ = new layoutDef[staffCount];
	mainWidget->barCont_ = new layoutDef[staffCount];
	i = 0;
	for (layoutinfo = parser_params.bracketList.first(); i < staffCount && layoutinfo; layoutinfo = parser_params.bracketList.next()) {
		mainWidget->bracketMatrix_[i++] = *layoutinfo;
	}
	i = 0;
	for (layoutinfo = parser_params.braceList.first(); i < staffCount && layoutinfo; layoutinfo = parser_params.braceList.next()) {
		mainWidget->braceMatrix_[i++] = *layoutinfo;
	}
	i = 0;
	for (layoutinfo = parser_params.contList.first(); i < staffCount && layoutinfo; layoutinfo = parser_params.contList.next()) {
		mainWidget->barCont_[i++] = *layoutinfo;
	}
	return true;
}

QString NFileHandler::lyrics2MUP(QString *lyrics) {
	QString ret;
	QRegExp reg;

	ret = QString(*lyrics);
	reg = QRegExp("\"");
	ret.replace (reg, "\\\"");
	reg = QRegExp("-");
	ret.replace (reg, "~");
	reg = QRegExp("~$");
	ret.replace (reg, "-");
	return ret;
}

QString NFileHandler::computeTripletString(int tupletsum, char numNotes, char playtime, bool *ok) {
	QString s;
	int length;
	*ok = true;
	if (numNotes == 3 && playtime == 2) return QString("3 ");
	length = tupletsum / MULTIPLICATOR * playtime / numNotes;
	switch (length) {
		case 128 : s.sprintf("%d, 1 ", numNotes); break;
		case  64 : s.sprintf("%d, 2 ", numNotes); break;
		case  32 : s.sprintf("%d, 4 ", numNotes); break;
		case  16 : s.sprintf("%d, 8 ", numNotes); break;
		case   8 : s.sprintf("%d, 16 ", numNotes); break;
		case   4 : s.sprintf("%d, 32 ", numNotes); break;
		case   2 : s.sprintf("%d, 64 ", numNotes); break;
		case   1 : s.sprintf("%d, 128 ", numNotes); break;
		case 128/2*3 : s.sprintf("%d, 1. ", numNotes); break;
		case  64/2*3 : s.sprintf("%d, 2. ", numNotes); break;
		case  32/2*3 : s.sprintf("%d, 4. ", numNotes); break;
		case  16/2*3 : s.sprintf("%d, 8. ", numNotes); break;
		case   8/2*3 : s.sprintf("%d, 16. ", numNotes); break;
		case   4/2*3 : s.sprintf("%d, 32. ", numNotes); break;
		case   2/2*3 : s.sprintf("%d, 64. ", numNotes); break;
		default  : s.sprintf("%d, x%d ", numNotes, playtime);
			    *ok = false;
			    break;
	}
	return s;
}

