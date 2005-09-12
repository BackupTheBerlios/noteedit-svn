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

// Export to MusicXML

// LVIFIX: add "forward" to multiple voice support
// including forward to end-of-measure if necessary ?
// currently <forward> is generated for hidden rests only, which may be insufficient
// as the second voice in the last measure may incomplete
// furthermore, NoteEdit happily inserts hidden rests in the second voice that cross
// the measure boundary

// LVIFIX: check handling "D.S. al Coda" (and friends). The exporter assumes
// it logically attaches to the end of the measure and forces that location,
// while NoteEdit inserts D.S. al Coda at any location in the measure.
// See: example1.not

// LVIFIX: when a trill extends beyond the last chord in a part,
// the trill stop gets lost.

// LVIFIX: end of trill / wavy line sometimes off by one

// LVIFIX: key changes fail for the second voice
// LVI20040328: fixed, but check all remaining key handling code

/*

Status overview of features implemented

Accidentals	supported: sharp, flat and natural, including double sharp and flat
Accents		supported: staccato, sforzato, portato, strong pizzicato, sforzando, fermate,
			   arpeggio and pedal on/off
Arbitrary text	supported, including above/below position
Bar separators	supported: simple, double, end
Beams		supported
Clefs		supported: treble, bass, soprano, alto and tenor clef, not supported: drum and drum_bass clef
Clef change	supported
Chords		supported
Chord diagrams	supported, but excluding bass and degree
Dots		supported, including double dot
Grace notes	supported
Jumps		supported: coda, dal segno, dal segno al coda, dal segno al fine, fine, segno
Key signature	supported, including changing the signature
Lyrics		supported
Multiple voices	supported
Notes		supported: 2, 1, 1/2, 1/4, 1/8, 1/16, 1/32, 1/64, 1/128
Repeats		supported: open, close, open_close, special_ending1, special_ending2
Rests		supported: 2, 1, 1/2, 1/4, 1/8, 1/16, 1/32, 1/64, 1/128,
			  (including hidden and multi-measure rests)
Score info	supported: title, composer and copyright, not supported: subject, last author and comment
Slurs		supported
Stem dir	supported: up, down
Tempo changes	supported: accelerando, ritardando and tempo signature
Ties		supported
Time signature	supported, including changing the signature
Trills		supported
Tuplets		supported
Volume changes	supported: ppp .. fff, crescendo and diminuendo

Not supported:
Program change
Staff properties / options
Layout (brace and bracket)
Multistaff (e.g. for piano music NoteEdit should generate one part with two staves)
Note shapes: body as cross, alternative cross, cross with circle, rectangle, triangle

*/


// LVIFIX future expansion:
// add <sound> element (a.o. volume changes, coda, segno)


#include "musicxmlcommon.h"
#include "musicxmlexport.h"
#if GCC_MAJ_VERS > 2
#include <iostream>
#else
#include <iostream.h>
#endif
#include <qstring.h>
#include <qregexp.h>
#include <qspinbox.h>
#include <qcheckbox.h>
#include <qvaluelist.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <ctype.h>
#include "staff.h"
#include "uiconnect.h"
#include "keysig.h"
#include "timesig.h"
#include "mainframewidget.h"
#include "rest.h"
#include "chord.h"
#include "chorddiagram.h"
#include "text.h"
#include "layout.h"
#include "uiconnect.h"
#include "../kguitar_excerpt/global.h"

#define MUSICXML_ERRIRREGULAER   1
#define MUSICXML_ERRDRUM_CLEF    2
#define MUSICXML_ERRDRUM_STAFF   3
#define MUSICXML_ERRVOICE_CHANGE 4
#define MUSICXML_ERR8VA_NOEND    5
#define MUSICXML_ERR8VA_SAMENOTE 6
#define MUSICXML_ERRDYN_NOEND    7

static int ntsign = 0;

void NMusicXMLExport::debugDumpElem(NMusElement *elem) {
	if (elem == 0) return;
	NChord *chord;
	NClef *clef;
	int count;
	NKeySig *key;
	status_type kind;
	NNote *note;
	NSign *sign;
	NTimeSig *timesig;
	out_
		<< "miditime=" << elem->midiTime_
		<< " midilen=" << elem->getMidiLength()
		<< " xpos=" << elem->getXpos()
//		<< " bbox-l,r=" << elem->getBbox()->left()
//		<< "," << elem->getBbox()->right()
		<< hex
		<< " status=" <<  ( elem->playable() ? elem->playable()->status_ : 0 )
		<< dec << " ";
	if (elem->chord() && elem->chord()->va_) {
		out_ << "va=" << elem->chord()->va_ << " ";
	}
	switch (elem->getType()) {
		case T_CHORD:
			chord = (NChord *) elem;
			if (chord->va_) {
				out_ << "vaend=" << chord->getVaEnd() << " ";
			}
			out_ << "T_CHORD";
			out_ << " typ=" << elem->getSubType();
			for (note = chord->getNoteList()->first();
					note;
					note = chord->getNoteList()->next()) {
				out_ << endl << "  note"
					<< hex
					<< " status=" << note->status
					<< dec
					<< " line=" << (int) note->line
					<< " offs=" << (int) note->offs;
			}
			break;
		case T_REST:
			out_ << "T_REST";
			if (elem->getSubType() == MULTIREST) {
				out_ << " multilen=" << ((NRest *) elem)->getMultiRestLength();
			} else {
				out_ << " len=" << elem->getSubType();
			}
			break;
		case T_SIGN:
			out_ << "T_SIGN";
			ntsign++;
			sign = (NSign *) elem;
			out_ << " type=" << sign->getSubType();
			switch (sign->getSubType()) {
			case SIMPLE_BAR:
			     out_ << " SIMPLE_BAR ntsign=" << ntsign; break;
			case REPEAT_OPEN: out_ << " REPEAT_OPEN"; break;
			case REPEAT_CLOSE: out_ << " REPEAT_CLOSE"; break;
			case REPEAT_OPEN_CLOSE: out_ << " REPEAT_OPEN_CLOSE"; break;
			case DOUBLE_BAR: out_ << " DOUBLE_BAR"; break;
			case SPECIAL_ENDING1: out_ << " SPECIAL_ENDING1"; break;
			case SPECIAL_ENDING2: out_ << " SPECIAL_ENDING2"; break;
			case END_BAR: out_ << " END_BAR"; break;
			default: out_ << "???"; break;
			}
			break;
		case T_CLEF:
			out_ << "T_CLEF";
			clef = (NClef *) elem;
			out_ << " kind=" << clef->getSubType()
				<< " shift=" << clef->getShift()
				<< " lineOfC4=" << clef->lineOfC4();
			break;
		case T_TIMESIG: 
			out_ << "T_TIMESIG";
			timesig = (NTimeSig *) elem;
			out_ << " " << timesig->getNumerator()
				<< "/" << timesig->getDenominator();
			break;
		case T_KEYSIG:
			out_ << "T_KEYSIG";
			key = (NKeySig *) elem;
			if (key->isRegular(&kind, &count)) {
				out_ << " fifths=";
				if (kind == STAT_FLAT) {
					out_ << "-";
				}
				out_ << count;
			}
			break;
		default:
			out_ << "default";
	}
}


void NMusicXMLExport::debugDumpVoice(NVoice * voice_elem) {
	if (voice_elem == 0) return;
	voice_elem->prepareForWriting();
	ntsign = 0;
	out_ << "isFirstVoice=" << voice_elem->isFirstVoice()
		<< " octave=" << voice_elem->octave_
		<< endl;
	NMusElement *elem;
	elem = voice_elem->getCurrentPosition();
	if (elem == 0) return;
	do {
//		out_ << "elem @" << elem;
		debugDumpElem(elem);
		out_ << endl;
		elem = voice_elem->getNextPosition();
	} while (elem);
}


void NMusicXMLExport::debugDumpStaff(NStaff * staff_elem) {
	if (staff_elem == 0) return;
	for (int i = 0; i < staff_elem->voiceCount(); i++) {
		out_ << "*** Voice " << i << endl;
		debugDumpVoice(staff_elem->getVoiceNr(i));
	}
}


void NMusicXMLExport::debugDump(QList<NStaff> *stafflist, NMainFrameWidget *mainWidget) {
	if (!mainWidget->scTitle_.isEmpty()) {
		out_ << "scTitle=" << mainWidget->scTitle_ << endl;
	}
	if (!mainWidget->scSubtitle_.isEmpty()) {
		out_ << "scSubtitle=" << mainWidget->scSubtitle_ << endl;
	}
	if (!mainWidget->scAuthor_.isEmpty()) {
		out_ << "scAuthor=" << mainWidget->scAuthor_ << endl;
	}
	if (!mainWidget->scLastAuthor_.isEmpty()) {
		out_ << "scLastAuthor=" << mainWidget->scLastAuthor_ << endl;
	}
	if (!mainWidget->scCopyright_.isEmpty()) {
		out_ << "scCopyright=" << mainWidget->scCopyright_ << endl;
	}
	if (!mainWidget->scComment_.isEmpty()) {
		out_ << "scComment=" << mainWidget->scComment_ << endl;
	}
	// staff list
	NStaff *staff_elem;
//	int staffcount;
	int i;
	for (i = 0, staff_elem = stafflist->first(); staff_elem; i++, staff_elem = stafflist->next()) {
		out_ << "*** Staff " << i;
		if (!staff_elem->staffName_.isEmpty()) {
			out_ << " staffName=" << staff_elem->staffName_;
		}
		out_ << " #voices="   << staff_elem->voiceCount()
			<< " midi-chn="  << staff_elem->getChannel()
			<< " midi-pgm="  << staff_elem->getVoice()
			<< endl;
		debugDumpStaff(staff_elem);
	}
}

NMusicXMLExport::NMusicXMLExport() {
#if GCC_MAJ_VERS > 2
	os_ = new ostringstream();
#else
	os_ = new ostrstream(buffer_, 128);
#endif
}

// export all staffs

void NMusicXMLExport::exportStaffs(QString fname, QList<NStaff> *stafflist, int count_of_voices, exportFrm * /* expWin */, NMainFrameWidget *mainWidget) {
	NStaff *staff_elem;
	NVoice *voice_elem;
	NClef *firstClef;
	badinfo *bad;
	int i, j, k;
	int voice_count;
	int current_time;
	bool something_written;
	NMusElement *last_elem;
	QString s;

	out_.open(fname);
	if (!out_) {
		*os_ << "error opening file " << fname << '\0';
#if GCC_MAJ_VERS > 2
		KMessageBox::sorry
			(0, QString(os_->str().c_str()), kapp->makeStdCaption(i18n("???")));
#else
		KMessageBox::sorry
			(0, QString(os_->str()), kapp->makeStdCaption(i18n("???")));
#endif
		return;
	}
	out_.setf(ios::showpoint);
	staffCount_ = stafflist->count();
	badlist_.setAutoDelete(true);
	badlist_.clear();
	drum_problem_written_ = false;
	voiceStatList_ = new voice_stat_str;


	// debug: if filename ends in .dmp dump data structures
	QString strFname(fname);
	if (strFname.right(8).lower() == ".dmp.xml") {
		debugDump(stafflist, mainWidget);
		out_.close();
		return;
	}

	// header
	out_ << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>"
	   << endl;
	out_ << "<!DOCTYPE score-partwise PUBLIC" << endl;
	out_ << "    \"-//Recordare//DTD MusicXML 1.0 Partwise//EN\"" << endl;
	out_ << "    \"http://www.musicxml.org/dtds/partwise.dtd\">" << endl;
	out_ << endl;
	out_ << "<score-partwise>\n";
	out_ << "\t<work>\n";
	if (!mainWidget->scTitle_.isEmpty()) {
		out_ << "\t\t<work-title>" << mainWidget->scTitle_.utf8()
			<< "</work-title>\n";
	}
	out_ << "\t</work>\n";

	// identification
	out_ << "\n";
	out_ << "\t<identification>\n";
	if (!mainWidget->scAuthor_.isEmpty()) {
		out_ << "\t\t<creator type=\"composer\">" << mainWidget->scAuthor_.utf8()
			<< "</creator>\n";
	}
	if (!mainWidget->scCopyright_.isEmpty()) {
		out_ << "\t\t<rights>" << mainWidget->scCopyright_.utf8() << "</rights>\n";
	}
	out_ << "\t\t<encoding>\n";
	// LVIFIX	out_ << "\t\t\t<encoder>" << TBD << "</encoder>\n";
	out_ << "\t\t\t<software>NoteEdit</software>\n";
	out_ << "\t\t</encoding>\n";
	out_ << "\t</identification>\n";

	outputStaffAndVoiceDescription(stafflist);

	// before writing the parts, calculate a divisions value that is OK for all parts
	calcDivisions(stafflist);

	firstClef = stafflist->first()->getVoiceNr(0)->getFirstClef();
	lastClef_ = new NClef(&(mainWidget->main_props_), &(stafflist->first()->staff_props_));
	k = 0;
	for (i = 0, staff_elem = stafflist->first(); staff_elem; staff_elem = stafflist->next(), i++) {
		voice_count = staff_elem->voiceCount();
		for (j = 0; j < voice_count; j++) {
			voice_elem = staff_elem->getVoiceNr(j);
			voice_elem->prepareForWriting();
			if (k >= count_of_voices) {
				NResource::abort("NMusicXMLExport::exportStaffs");
			}
		}
		staff_elem->actualClef_.change(staff_elem->getVoiceNr(0)->getFirstClef());
	}

	for (i = 0, staff_elem = stafflist->first(); staff_elem; i++, staff_elem = stafflist->next()) {
		lastMeasureNum_ = 1;
		divisions_written_ = false;
		keysig_written_ = false;
		out_ << "\t<part id=\"P" << i+1 << "\">\n";
		voice_count = staff_elem->voiceCount();
		dynEndChord = NULL;
		vaEndChord = NULL;

		do {
			something_written = false;

			voice_elem = staff_elem->getVoiceNr(0);
			lastClef_->change(&(staff_elem->actualClef_));

			out_ << "\t\t<measure number=\"" << lastMeasureNum_ << "\">" << endl;

			curTime_ = 0;
			if (writeFirstVoice(voice_elem, i+1)) {
				something_written = true;
			}
			last_elem = voice_elem->getCurrentPosition();
			if (last_elem) {
				current_time = last_elem->midiTime_;
			}
			else {
				current_time = (1 << 30);
			}
			if (voice_count > 1) {
				staff_elem->actualClef_.change(lastClef_);
				staff_elem->mark();
			}
			for (j = 2; j <= voice_count; j++) {
				if (curTime_ > 0) {
					out_ << "\t\t\t<backup>\n";
					out_ << "\t\t\t\t<duration>"
						<< curTime_
						<< "</duration>\n";
					out_ << "\t\t\t</backup>\n";
					curTime_ = 0;
				}
				voice_elem = staff_elem->getVoiceNr(j-1);
				if (writeOtherVoicesTill(i+1, j, voice_elem, current_time)) {
					something_written = true;
				}
			}
			if (voice_count > 1) {
				staff_elem->gotoMarkedPosition();
			}
			// After the last writeFirstVoice(), getCurrentPosition() returns 0,
			// but mark() / gotoMarkedPosition() moves the current position
			// back to the last element. This causes export of an empty last
			// measure in multi-voice staffs. The fix is to explicitly check
			// for the null pointer here
			if (!last_elem) {
				something_written = false;
			}
			writePendingSignsAtEnd();
			out_ << "\t\t</measure>" << endl;
			lastMeasureNum_++;
		}
		while (something_written);
		out_ << "\t</part>\n";
	} // for (i = 0, staff_elem = ...
	out_ << "</score-partwise>\n";
	out_.close();
	delete voiceStatList_;
	if (!badlist_.isEmpty()) {
		QString output;
		output = i18n
			("Noteedit has exported the score to MusicXML\n"
			 "but there are some problems.\n");
		output += i18n("-----------------------------------------------------\n");
		for (bad = badlist_.first(); bad; bad = badlist_.next()) {
			switch (bad->type_) {
				case MUSICXML_ERRIRREGULAER:
					output += i18n
						("Staff %1 has irregular keysig. This is currently not supported.\n")
						.arg(bad->staffnr_);
					break;
				case MUSICXML_ERRDRUM_CLEF:
					output += i18n
						("Staff %1 contains drum clef. This is currently not supported.\n")
						.arg(bad->staffnr_);
					break;
				case MUSICXML_ERRDRUM_STAFF:
					output += i18n
						("Staff %1 contains drum staff. This is currently not supported.\n")
						.arg(bad->staffnr_);
					break;
				case MUSICXML_ERRVOICE_CHANGE:
					output += i18n
						("Staff %1 contains voice change. This is currently not supported.\n")
						.arg(bad->staffnr_);
					break;
				case MUSICXML_ERR8VA_NOEND:
					output += i18n
						("Staff %1 contains 8va start without 8va stop.\n")
						.arg(bad->staffnr_);
					break;
				case MUSICXML_ERR8VA_SAMENOTE:
					output += i18n
						("Staff %1 contains 8va over a single note. This is currently not supported.\n")
						.arg(bad->staffnr_);
					break;
				case MUSICXML_ERRDYN_NOEND:
					output += i18n
						("Staff %1 contains dynamic start without dynamic stop.\n")
						.arg(bad->staffnr_);
					break;
			}
		}
		NResource::exportWarning_->setOutput(i18n ("MusicXML produced. But there are some problems."), &output);
		NResource::exportWarning_->show();
	}
}

// write one measure of the first voice of staff staff_nr starting at voice_elem

bool NMusicXMLExport::writeFirstVoice(NVoice *voice_elem, int staff_nr) {
	badinfo *bad = 0;
	NMusElement *elem;
	NStaff *actual_staff;
	NChord *chord = 0;
	NRest *rest;
	NNote *note;
	NClef *clef;
	NSign *sign;
	NChordDiagram *diag;
	int len;
	int duration;
	QString noteType;
	int measure_count = 1;

//	cout << "NMusicXMLExport::writeFirstVoice()" << endl;

	elem = voice_elem->getCurrentPosition();
	if (!elem) return false;

	actual_staff = voice_elem->getStaff();
	do {
//		out_ << "writeFirstVoice";
//		out_ << " elem=" << elem << " ";
	        int n = 0;
		int va = 0;
		switch (elem->getType()) {
			case T_CHORD:
//				      out_ << "T_CHORD";
				      writePendingSigns(staff_nr);
				      voiceStatList_->lastBarSym = 0;
				      chord = (NChord *) elem;
//				      out_ << ", chord=" << hex << chord << dec << endl;

				      if ((diag = chord->getChordChordDiagram()) != 0) {
					outputDiagram(diag);
				      }

				      // In MusicXML pedal on/off are expressed as directions
				      // containing direction-type pedal before the chord
				      // they apply to.
				      // In NoteEdit it is a status bit in the chord and cannot
				      // occur in a rest. Sometimes both pedal on and off are found,
				      // in which case pedal on has priority.
				      // LVIFIX: may need to add relative-x,y and offset for proper positioning
				      if (chord->status_ & STAT_PEDAL_ON) {
					outputDirection("\t\t\t\t\t<pedal type=\"start\"/>\n",
							"below");
				      } else if (chord->status_ & STAT_PEDAL_OFF) {
					outputDirection("\t\t\t\t\t<pedal type=\"stop\"/>\n",
							"below");
				      }
				      if (chord->dynamic_) {
				        // start of crescendo/diminuendo
					dynEndChord = findDynEndChord(actual_staff, chord);
					if (!dynEndChord) {
					  bad = new badinfo(MUSICXML_ERRDYN_NOEND, staff_nr, 0);
					  badlist_.append(bad);
//					} else if (chord == vaEndChord) {
//					  bad = new badinfo(MUSICXML_ERR8VA_SAMENOTE, staff_nr, 0);
//					  badlist_.append(bad);
					} else {
					  // LVIFIX: may need to add relative-x,y for proper positioning
					  QString wedgeType = chord->dynamicAlign_ ?
						  "type=\"crescendo\" spread=\"0\""
						: "type=\"diminuendo\" spread=\"15\"";
					  QString direction = "\t\t\t\t\t<wedge ";
					  direction += wedgeType;
					  direction += "/>\n";
					  outputDirection(direction, "below");
					  voiceStatList_->dynEndPos = chord->getDynamicEnd();
					  voiceStatList_->lastDynSym = (char *) (chord->dynamicAlign_ ?
						  "type=\"stop\" spread=\"15\""
						: "type=\"stop\" spread=\"0\"");
				        }
				      }
				      if (chord->va_) {
				        // start of octava
					vaEndChord = findVaEndChord(actual_staff, chord);
					if (!vaEndChord) {
					  bad = new badinfo(MUSICXML_ERR8VA_NOEND, staff_nr, 0);
					  badlist_.append(bad);
					} else if (chord == vaEndChord) {
					  bad = new badinfo(MUSICXML_ERR8VA_SAMENOTE, staff_nr, 0);
					  badlist_.append(bad);
					} else {
					  // LVIFIX: may need to add relative-x,y for proper positioning
					  QString vaType = (chord->va_ > 0) ?
						  "type=\"down\""
						: "type=\"up\"";
					  QString direction = "\t\t\t\t\t<octave-shift ";
					  direction += vaType;
					  direction += " size=\"8\"/>\n";
					  // LVIFIX: add <staff>
					  outputDirection(direction, "");
					  voiceStatList_->vaEndPos = chord->getVaEnd();
					}
				      }
				      voice_elem->setCorrectClefAccordingTime(elem->midiTime_);
//				      cout << " midiTime_=" << elem->midiTime_
//				           << " actualClef_:"
//				           << " kind=" << actual_staff->actualClef_.getSubType()
//				           << " shift=" << actual_staff->actualClef_.getShift()
//				           << " lineOfC4=" << actual_staff->actualClef_.lineOfC4()
//					   << endl;
				      va = voice_elem->getVaAtXpos(chord->getXpos());
				      n = 0;
				      for (note = chord->getNoteList()->first(); note; note = chord->getNoteList()->next()) {
				     	  outputNote(note, voice_elem, &(actual_staff->actualClef_), va, staff_nr, 1, n);
					  n++;
				      }
				      curTime_ += calcDuration(chord->getSubType(), chord->status_);
				      if (chord == dynEndChord) {
				        // dynamic ends after this chord
					// LVIFIX: may need to add relative-x,y for proper positioning
					QString wedgeType = voiceStatList_->lastDynSym;
					QString direction = "\t\t\t\t\t<wedge ";
					direction += wedgeType;
					direction += "/>\n";
					outputDirection(direction, "below");
					voiceStatList_->lastDynSym = 0;
					voiceStatList_->dynEndPos = 0;
					dynEndChord = NULL;
				      }
				      if (chord == vaEndChord) {
				        // octava ends after this chord
					// LVIFIX: may need to add relative-x,y for proper positioning
					QString direction = "\t\t\t\t\t<octave-shift ";
					direction += "type=\"stop\" size=\"8\"/>\n";
					outputDirection(direction, "");
					voiceStatList_->vaEndPos = 0;
					vaEndChord = NULL;
				      }
				      break;
			case T_REST:
//				     out_ << "T_REST" << endl;
					 rest = (NRest *) elem;
				     if (rest->getSubType() == MULTIREST) {
					voiceStatList_->pendingMultiRest = rest;
				     }
				     writePendingSigns(staff_nr);
				     voiceStatList_->lastBarSym = 0;
				     if (rest->getSubType() == MULTIREST) {
					/* already handled */ ;
				     }
				     else if (rest->status_ & STAT_HIDDEN) {
					len = rest->getSubType();
					calcLength(rest, duration, noteType);
					out_ << "\t\t\t<forward>\n";
					out_ << "\t\t\t\t<duration>"
						<< duration
						<< "</duration>\n";
					out_ << "\t\t\t</forward>\n";
					curTime_ += duration;
				     }
				     else {
				        // real, visible rest
					out_ << "\t\t\t<note>\n";
					out_ << "\t\t\t\t<rest/>\n";
					len = rest->getSubType();
					calcLength(rest, duration, noteType);
					out_ << "\t\t\t\t<duration>" << duration << "</duration>\n";
					curTime_ += duration;
					outputVoiceNr(1);
					out_ << "\t\t\t\t<type>" << noteType << "</type>\n";
					outputDots(rest);
					outputTimeMod(rest);
					bool needNotations = false;
					bool fermata = false;
					if (rest->status_ & STAT_FERMT) {
						fermata = true;
						needNotations = true;
					}
					bool tupletStart = false;
					if ((rest->status_ & STAT_TUPLET)
					     && rest->isFirstInTuplet()) {
						tupletStart = true;
						needNotations = true;
					}
					bool tupletStop = false;
					if (rest->status_ & STAT_LAST_TUPLET) {
						tupletStop = true;
						needNotations = true;
					}
					if (needNotations) {
						out_ << "\t\t\t\t<notations>\n";
						if (fermata) {
							// LVIFIX: only upright fermata's are generated.
							out_ << "\t\t\t\t\t<fermata type=\"upright\"/>\n";
						}
						if (tupletStop) {
							out_ << "\t\t\t\t\t<tuplet type=\"stop\"/>\n";
						}
						if (tupletStart) {
							out_ << "\t\t\t\t\t<tuplet type=\"start\"/>\n";
						}
						out_ << "\t\t\t\t</notations>\n";
					}
					out_ << "\t\t\t</note>\n";
				     }
				     break;
			case T_SIGN:
//				     out_ << "T_SIGN" << endl;
				     sign = (NSign *) elem;
				     if (sign->getSubType() & BAR_SYMS) {
					measure_count--;
					switch (elem->getSubType()) {
			             	  case REPEAT_OPEN:
					  	voiceStatList_->pendingBarSym = sign;
						break;
					  case REPEAT_CLOSE:
					  	voiceStatList_->pendingBarSymAtEnd = sign;
						break;
					  case REPEAT_OPEN_CLOSE:
					  	voiceStatList_->pendingBarSym = sign;
					  	voiceStatList_->pendingBarSymAtEnd = sign;
						break;
					  case DOUBLE_BAR:
					  	voiceStatList_->pendingBarSymAtEnd = sign;
						break;
				          case END_BAR:
					  	voiceStatList_->pendingBarSymAtEnd = sign;
						break;
					  default: /* do nothing */ break;
					}
					break;
				     }

				     switch (sign->getSubType()) {
					case SPECIAL_ENDING1:
					case SPECIAL_ENDING2:
						voiceStatList_->pendingEnding = sign;
						voiceStatList_->pendingEndingAtEnd = sign;
						break;
				     	case VOLUME_SIG: voiceStatList_->pendingVolumes = sign; break;
					case PROGRAM_CHANGE:
						bad = new badinfo(MUSICXML_ERRVOICE_CHANGE, staff_nr, 0);
						badlist_.append(bad);
						break;
					case SEGNO:
					case CODA: voiceStatList_->pendingSegnos = sign; break;
					case DAL_SEGNO:
					case DAL_SEGNO_AL_FINE:
					case DAL_SEGNO_AL_CODA:
					case FINE: voiceStatList_->pendingSegnos2 = sign; break;
					case RITARDANDO:
					case ACCELERANDO: voiceStatList_->pendingRitAccel = sign; break;
					case TEMPO_SIGNATURE: voiceStatList_->pendingTempo = sign; break;
				     }
				     break;
			case T_CLEF:
//				     out_ << "T_CLEF" << endl;
				     voiceStatList_->pendingClef = (NClef *) elem;
				     voiceStatList_->lastBarSym = 0;
				     clef = (NClef *) elem;
				     actual_staff->actualClef_.change(clef);
				     break;
			case T_TIMESIG: 
//				     out_ << "T_TIMESIG" << endl;
				     voiceStatList_->pendingTimeSig = (NTimeSig *) elem;
				     voiceStatList_->lastBarSym = 0;
				     break;
			case T_KEYSIG:
//				     out_ << "T_KEYSIG" << endl;
				     voiceStatList_->pendingKeySig = (NKeySig *) elem;
				     voiceStatList_->lastBarSym = 0;
				     break;
			case T_TEXT:
					out_ << "\t\t\t<direction placement=\"" << ((elem->getSubType() == TEXT_DOWNTEXT) ? "below" : "above") << "\">\n";
					out_ << "\t\t\t\t<direction-type>\n";
					out_ << "\t\t\t\t\t<words>" << ((NText*)(elem))->getText() << "</words>\n";
					out_ << "\t\t\t\t</direction-type>\n";
					out_ << "\t\t\t</direction>\n";
					break;
			default:
//				     out_ << "default" << endl;
				     voiceStatList_->lastBarSym = 0;
		}
		elem = voice_elem->getNextPosition();
	}
	while (elem && measure_count > 0);
//	out_ << endl;
	return true;
}

// write one measure of voice voice_nr of staff staff_nr starting at voice_elem ending at stopTime

bool NMusicXMLExport::writeOtherVoicesTill(int staff_nr, int voice_nr, NVoice *voice_elem, int stopTime) {
	NMusElement *elem;
	NStaff *actual_staff;
	NChord *chord;
	NRest *rest;
	NNote *note;
	int len;
	int duration;
	QString noteType;

//	cout << "NMusicXMLExport::writeOtherVoicesTill()" << endl;

	actual_staff = voice_elem->getStaff();
	elem = voice_elem->getCurrentPosition();


	if (!elem || elem->midiTime_ >= stopTime) return false;
	actual_staff->resetSpecialElement();
	actual_staff->syncSpecialElement(elem->getXpos());

	while (elem && elem->midiTime_ < stopTime) {
	        int n = 0;
		int va = 0;
		switch (elem->getType()) {
			case T_CHORD:
//				      out_ << "writeOtherVoicesTill T_CHORD";
				      chord = (NChord *) elem;
//				      out_ << ", chord=" << hex << chord << dec << endl;
				      voice_elem->getStaff()->getVoiceNr(0)->setCorrectClefAccordingTime(elem->midiTime_);
//				      cout << " midiTime_=" << elem->midiTime_
//				           << " actualClef_:"
//				           << " kind=" << actual_staff->actualClef_.getSubType()
//				           << " shift=" << actual_staff->actualClef_.getShift()
//				           << " lineOfC4=" << actual_staff->actualClef_.lineOfC4()
//					   << endl;
				      va = voice_elem->getStaff()->getVoiceNr(0)->getVaAtXpos(chord->getXpos());
				      n = 0;
				      for (note = chord->getNoteList()->first(); note; note = chord->getNoteList()->next()) {
				     	  outputNote(note, voice_elem, &(actual_staff->actualClef_), va, staff_nr, voice_nr, n);
					  n++;
				      }
				      curTime_ += calcDuration(chord->getSubType(), chord->status_);
#if 0
// LVIFIX reenable when octava end is allowed in other voices than the first voice
				      if (chord == vaEndChord) {
				        // octava ends after this chord
					// LVIFIX: may need to add relative-x,y for proper positioning
					QString direction = "\t\t\t\t\t<octave-shift ";
					direction += "type=\"stop\" size=\"8\"/>\n";
					outputDirection(direction, "");
					voiceStatList_->vaEndPos = 0;
					vaEndChord = NULL;
				      }
#endif
				      break;
			case T_REST:
//				     out_ << "writeOtherVoicesTill T_REST" << endl;
				     // LVIFIX: repair code duplication between this and previous "case T_REST:"
				     rest = (NRest *) elem;
				     if (rest->getSubType() == MULTIREST) {
				        // LVIFIX: this should be reported
					out_ << "<!-- multi rest (not supported in this voice) -->" << endl;
					len = rest->getMultiRestLength() * QUARTER_LENGTH;
				     }
				     else if (rest->status_ & STAT_HIDDEN) {
					len = rest->getSubType();
					calcLength(rest, duration, noteType);
					out_ << "\t\t\t<forward>\n";
					out_ << "\t\t\t\t<duration>"
						<< duration
						<< "</duration>\n";
					out_ << "\t\t\t</forward>\n";
				     }
				     else {
				        // real, visible rest
					out_ << "\t\t\t<note>\n";
					out_ << "\t\t\t\t<rest/>\n";
					len = rest->getSubType();
					calcLength(rest, duration, noteType);
					out_ << "\t\t\t\t<duration>" << duration << "</duration>\n";
					curTime_ += duration;
					outputVoiceNr(voice_nr);
					out_ << "\t\t\t\t<type>" << noteType << "</type>\n";
					outputDots(rest);
					outputTimeMod(rest);
					bool needNotations = false;
					bool fermata = false;
					if (rest->status_ & STAT_FERMT) {
						fermata = true;
						needNotations = true;
					}
					bool tupletStart = false;
					if ((rest->status_ & STAT_TUPLET)
					     && rest->isFirstInTuplet()) {
						tupletStart = true;
						needNotations = true;
					}
					bool tupletStop = false;
					if (rest->status_ & STAT_LAST_TUPLET) {
						tupletStop = true;
						needNotations = true;
					}
					if (needNotations) {
						out_ << "\t\t\t\t<notations>\n";
						if (fermata) {
							// LVIFIX: only upright fermata's are generated.
							out_ << "\t\t\t\t\t<fermata type=\"upright\"/>\n";
						}
						if (tupletStop) {
							out_ << "\t\t\t\t\t<tuplet type=\"stop\"/>\n";
						}
						if (tupletStart) {
							out_ << "\t\t\t\t\t<tuplet type=\"start\"/>\n";
						}
						out_ << "\t\t\t\t</notations>\n";
					}
					out_ << "\t\t\t</note>\n";
				     }
				     break;
			default:
				     /* nothing */ ;
//				     out_ << "writeOtherVoicesTill default" << endl;
		}
		elem = voice_elem->getNextPosition();
	}
	return true;
}

// write out the end of a measure: barline, coda, d.s. al coda/fine

void NMusicXMLExport::writePendingSignsAtEnd() {
	NSign *sign = 0;
	if ((sign = voiceStatList_->pendingSegnos)) {
		voiceStatList_->pendingSegnos = 0;
		QString segnoType = "";
		switch(sign->getSubType()) {
			case SEGNO     : segnoType = "segno"; break;
			case CODA      : segnoType = "coda"; break;
		}
		QString direction = "\t\t\t\t\t\t<";
		// LVIFIX: may need to add relative-x,y for proper positioning
		direction += segnoType;
		direction += "/>\n";
		outputDirection(direction, "above");
	}
	if ((sign = voiceStatList_->pendingSegnos2)) {
		voiceStatList_->pendingSegnos2 = 0;
		QString segnoType = "";
		switch(sign->getSubType()) {
			case DAL_SEGNO : segnoType = "D.S."; break;
			case FINE      : segnoType = "Fine"; break;
			case DAL_SEGNO_AL_FINE: segnoType = "D.S. al Fine"; break;
			case DAL_SEGNO_AL_CODA: segnoType = "D.S. al Coda"; break;
		}
		QString direction = "\t\t\t\t\t\t<words font-style=\"italic\">";
		// LVIFIX: may need to add relative-x,y for proper positioning
		direction += segnoType;
		direction += "</words>\n";
		outputDirection(direction, "above");
	}

	// MusicXML DTD: if location is right, the barline should be the last
	// element in the measure
	if (voiceStatList_->pendingBarSymAtEnd
	    || voiceStatList_->pendingEndingAtEnd) {
		out_ << "\t\t\t<barline location=\"right\">\n";
		if (voiceStatList_->pendingBarSymAtEnd) {
			switch (voiceStatList_->pendingBarSymAtEnd->getSubType()) {
			  case REPEAT_CLOSE:
		  		out_ << "\t\t\t\t<bar-style>light-heavy</bar-style>\n";
				out_ << "\t\t\t\t<repeat direction=\"backward\"/>\n";
				voiceStatList_->pendingBarSymAtEnd = 0;
				break;
			  case REPEAT_OPEN_CLOSE:
			  	out_ << "\t\t\t\t<bar-style>light-heavy</bar-style>\n";
				out_ << "\t\t\t\t<repeat direction=\"backward\"/>\n";
				voiceStatList_->pendingBarSymAtEnd = 0;
				break;
			  case DOUBLE_BAR:
			  	out_ << "\t\t\t\t<bar-style>light-light</bar-style>\n";
				voiceStatList_->pendingBarSymAtEnd = 0;
				break;
		          case END_BAR:
			  	out_ << "\t\t\t\t<bar-style>light-heavy</bar-style>\n";
				voiceStatList_->pendingBarSymAtEnd = 0;
				break;
			  default: /* do nothing */ break;
			}
		}
		if (voiceStatList_->pendingEndingAtEnd) {
			int n = 1;
			if (voiceStatList_->pendingEndingAtEnd->getSubType() == SPECIAL_ENDING2) {
				n = 2;
			}
			out_ << "\t\t\t\t<ending type=\"discontinue\" number=\"" << n << "\"/>\n";
			voiceStatList_->pendingEndingAtEnd = 0;
		}
		out_ << "\t\t\t</barline>\n";
	}
}

// write out the start of a measure: attribute, barline, coda, dynamics, segno, tempo

void NMusicXMLExport::writePendingSigns(int staff_nr) {

	// MusicXML DTD: if location is left, the barline should be the first
	// element in the measure
	if (voiceStatList_->pendingBarSym
	    || voiceStatList_->pendingEnding) {
		out_ << "\t\t\t<barline location=\"left\">\n";
		if (voiceStatList_->pendingBarSym) {
			switch (voiceStatList_->pendingBarSym->getSubType()) {
             		  case REPEAT_OPEN:
			  case REPEAT_OPEN_CLOSE:
			  	out_ << "\t\t\t\t<bar-style>heavy-light</bar-style>\n";
				out_ << "\t\t\t\t<repeat direction=\"forward\"/>\n";
				voiceStatList_->pendingBarSym = 0;
				break;
		          case END_BAR:
			  	out_ << "\t\t\t\t<bar-style>light-heavy</bar-style>\n";
				voiceStatList_->pendingBarSym = 0;
				break;
			  default: /* do nothing */ break;
			}
		}
		if (voiceStatList_->pendingEnding) {
			int n = 1;
			if (voiceStatList_->pendingEnding->getSubType() == SPECIAL_ENDING2) {
				n = 2;
			}
			out_ << "\t\t\t\t<ending type=\"start\" number=\"" << n << "\"/>\n";
			voiceStatList_->pendingEnding = 0;
		}
		out_ << "\t\t\t</barline>\n";
	}

	NSign *sign = 0;
	if (voiceStatList_->pendingMultiRest
	    || voiceStatList_->pendingClef
	    || voiceStatList_->pendingTimeSig
	    || voiceStatList_->pendingKeySig) {
		// write measure attributes
		out_ << "\t\t\t<attributes>\n";
		if (!divisions_written_) {
			out_ << "\t\t\t\t<divisions>" << divisions_ << "</divisions>\n";
			divisions_written_ = true;
		}
		if (voiceStatList_->pendingKeySig) {
			outputKeySig(voiceStatList_->pendingKeySig);
			voiceStatList_->pendingKeySig = 0;
			keysig_written_ = true;
		}
		if (!keysig_written_) {
			// LVIFIX: find out how to get the actual keysig, if not explicitly written
			// following code dumps core
			// Then replace "out_" by "outputKeySig"
			/*
			NKeySig keysig(0, 0);
			outputKeySig(&keysig);
			*/
			out_ << "\t\t\t\t<key>\n";
			out_ << "\t\t\t\t\t<fifths>0</fifths>\n";
			out_ << "\t\t\t\t</key>\n";
			keysig_written_ = true;
		}
		if (voiceStatList_->pendingTimeSig) {
			outputMeter(voiceStatList_->pendingTimeSig);
			voiceStatList_->pendingTimeSig = 0;
		}
		if (voiceStatList_->pendingClef) {
			outputClefInfo(voiceStatList_->pendingClef, staff_nr);
			voiceStatList_->pendingClef = 0;
		}
		if (voiceStatList_->pendingMultiRest) {
			// LVINOTE: if multirests can have fermatas, add support here
			out_ << "\t\t\t\t<measure-style>\n";
			out_ << "\t\t\t\t\t<multiple-rest>"
				<< voiceStatList_->pendingMultiRest->getMultiRestLength()
				<< "</multiple-rest>\n";
			out_ << "\t\t\t\t</measure-style>\n";
			voiceStatList_->pendingMultiRest = 0;
		}
		out_ << "\t\t\t</attributes>\n";
	}

	if ((sign = voiceStatList_->pendingVolumes)) {
		voiceStatList_->pendingVolumes = 0;
		QString volType = "";
		switch (sign->getVolType()) {
			case V_PPPIANO : volType = "ppp"; break;
			case V_PPIANO  : volType = "pp"; break;
			case V_PIANO   : volType = "p"; break;
			case V_MPIANO  : volType = "mp"; break;
			case V_MEZZO   : volType = "mf"; break;
			case V_FORTE   : volType = "f"; break;
			case V_FFORTE  : volType = "ff"; break;
			case V_FFFORTE : volType = "fff"; break;
			default        : volType = "mf"; break;
		}
		QString direction = "\t\t\t\t\t<dynamics>\n";
		direction += "\t\t\t\t\t\t<";
		direction += volType;
		direction += "/>\n";
		direction += "\t\t\t\t\t</dynamics>\n";
		outputDirection(direction, "above");
	}
	if ((sign = voiceStatList_->pendingSegnos)) {
		voiceStatList_->pendingSegnos = 0;
		QString segnoType = "";
		switch(sign->getSubType()) {
			case SEGNO     : segnoType = "segno"; break;
			case CODA      : segnoType = "coda"; break;
		}
		QString direction = "\t\t\t\t\t\t<";
		// LVIFIX: may need to add relative-x,y for proper positioning
		direction += segnoType;
		direction += "/>\n";
		outputDirection(direction, "above");
	}
	if ((sign = voiceStatList_->pendingRitAccel)) {
		voiceStatList_->pendingRitAccel = 0;
		QString tempoType = "";
		switch(sign->getSubType()) {
			case ACCELERANDO: tempoType = "accel."; break;
			case RITARDANDO:  tempoType = "ritard."; break;
		}
		QString direction = "\t\t\t\t\t\t<words>";
		// LVIFIX: may need to add relative-x,y for proper positioning
		direction += tempoType;
		direction += "</words>\n";
		outputDirection(direction, "above");
	}
	if ((sign = voiceStatList_->pendingTempo)) {
		voiceStatList_->pendingTempo = 0;
		QString tempoType;
		tempoType.setNum(sign->getTempo());
		QString direction = "\t\t\t\t\t<metronome>\n";
		// LVIFIX: may need to add relative-x,y for proper positioning
		direction += "\t\t\t\t\t\t<beat-unit>quarter</beat-unit>\n";
		direction += "\t\t\t\t\t\t<per-minute>";
		direction += tempoType;
		direction += "</per-minute>\n";
		direction += "\t\t\t\t\t</metronome>\n";
		outputDirection(direction, "above");
	}
}
	
// determine the number of flags for a given note

static int beamLevel(NMusElement *elem) {
	int len = elem->getSubType();
	int lvl = 0;
	switch (len) {
		case NOTE8_LENGTH:        lvl = 1; break;
		case NOTE16_LENGTH:       lvl = 2; break;
		case NOTE32_LENGTH:       lvl = 3; break;
		case NOTE64_LENGTH:       lvl = 4; break;
		case NOTE128_LENGTH:      lvl = 5; break;
		default:	          lvl = 0; break;
	}
	return lvl;
}

// write a single note

void NMusicXMLExport::outputNote(NNote *note, NVoice *voice_elem, NClef *actualClef, int va, int staff_nr, int voice_nr, int note_nr) {
	badinfo *bad = 0;
	NChord *chord = note->chordref;
	int duration = 0;
	char notename = ' ';
	QString noteType = "";
	int octave = 0;
	if ((note->status & BODY_MASK) && !drum_problem_written_)  {
		drum_problem_written_ = true;
		bad = new badinfo(MUSICXML_ERRDRUM_STAFF, staff_nr, lastMeasureNum_);
		badlist_.append(bad);
	}
	calcLength(chord, duration, noteType);
	notename = actualClef->line2Name(note->line, &octave, false, true);
	if (actualClef->getSubType() == BASS_CLEF
	    || actualClef->getSubType() == DRUM_BASS_CLEF
	    || actualClef->getSubType() == TENOR_CLEF) {
		octave--;
	}
	switch (actualClef->getShift()) {
 		case -12: octave--; break; 
   		case  12: octave++; break; 
	}
	octave += va;
	out_ << "\t\t\t<note>\n";
	if (note_nr) {
		out_ << "\t\t\t\t<chord/>\n";
	}
	if (chord->status_ & STAT_GRACE) {
		if (chord->getSubType() != INTERNAL_MARKER_OF_STROKEN_GRACE) {
			// normal grace
			out_ << "\t\t\t\t<grace/>\n";
		} else {
			// slashed eighth grace, which NoteEdit stores as 1/32
			out_ << "\t\t\t\t<grace slash=\"yes\"/>\n";
			noteType = "eighth";
		}
	}
	out_ << "\t\t\t\t<pitch>\n";
	out_ << "\t\t\t\t\t<step>" << (char) toupper(notename) << "</step>\n";
	if (note->offs) {
		out_ << "\t\t\t\t\t<alter>" << (int) note->offs << "</alter>\n";
	}
	out_ << "\t\t\t\t\t<octave>" << octave + 4 << "</octave>\n";
	out_ << "\t\t\t\t</pitch>\n";
	// grace notes have a <type> but no <duration> element
	if (!(chord->status_ & STAT_GRACE)) {
		out_ << "\t\t\t\t<duration>" << duration << "</duration>\n";
	}
	// accidental handling (borrowed from filehandler.cpp, NFileHandler::pitchOut)
	if (!(note->status & STAT_PART_OF_TIE)) {
		QString accid = "";
		if (note->status & STAT_FORCE) {
			switch (note->offs) {
				case  1: accid = "sharp"; break;
				case -1: accid = "flat"; break;
				case  0: accid = "natural"; break;
				case  2: accid = "double-sharp"; break;
				case -2: accid = "flat-flat"; break;
			}
		}
		else {
			switch (note->status & ACC_MASK) {
				case STAT_CROSS:  accid = "sharp"; break;
				case STAT_FLAT:   accid = "flat"; break;
				case STAT_NATUR:  accid = "natural"; break;
				case STAT_DCROSS: accid = "double-sharp"; break;
				case STAT_DFLAT:  accid = "flat-flat"; break;
			}
		}
		if (accid != "") {
			out_ << "\t\t\t\t<accidental>" << accid << "</accidental>\n";
		}
	}
	// notations
	bool needNotations = false;
        bool tieStart = false;
        bool tieStop  = false;
	if (note->status & STAT_PART_OF_TIE) {
		tieStop = true;
		needNotations = true;
		out_ << "\t\t\t\t<tie type=\"stop\"/>\n";
	}
	if (note->status & STAT_TIED) {
		tieStart = true;
		needNotations = true;
		out_ << "\t\t\t\t<tie type=\"start\"/>\n";
	}
        bool slurStart = false;
	if (!note_nr &&  chord->getSlurPartner()) {
		slurStart = true;
		needNotations = true;
	}
        bool slurStop = false;
	if (!note_nr && voiceStatList_->slurDepth > 0 && chord->getSlurStart()) {
		slurStop = true;
		needNotations = true;
	}
	bool needArti = false;
	if (!note_nr && (chord->status_ & (STAT_STACC | STAT_SFORZ | STAT_PORTA | STAT_STPIZ | STAT_SFZND))) {
		needArti = true;
		needNotations = true;
	}
	bool fermata = false;
	if (!note_nr && chord->status_ & STAT_FERMT) {
		fermata = true;
		needNotations = true;
	}
	if (chord->status_ & STAT_ARPEGG) {
		needNotations = true;
	}
	bool needTrill = false;
	if (!note_nr && (voiceStatList_->trillendpos || chord->trill_)) {
		needTrill = true;
		needNotations = true;
	}
	outputVoiceNr(voice_nr);
	out_ << "\t\t\t\t<type>" << noteType << "</type>\n";
	outputDots(chord);
	bool tupletStart = false;
	if (!note_nr && (chord->status_ & STAT_TUPLET) && chord->isFirstInTuplet()) {
		tupletStart = true;
		needNotations = true;
	}
	bool tupletStop = false;
	if (!note_nr && (chord->status_ & STAT_LAST_TUPLET)) {
		tupletStop = true;
		needNotations = true;
	}
	outputTimeMod(chord);
	if (chord->getSubType() <= HALF_LENGTH) {
		QString stemType = "";
		if (chord->status_ & STAT_STEM_UP) {
			stemType = "up";
		} else {
			stemType = "down";
		}
		out_ << "\t\t\t\t<stem>" << stemType << "</stem>\n";
	}
	// LVIFIX: add <staff>
	if (!note_nr && chord->status_ & STAT_BEAMED) {
		QList<NChord> *beamlist = 0;		// the group of beamed notes
		int currlvl = 0;			// beam level current note
		int nextlvl = 0;			// beam level next note
		int prevlvl = 0;			// beam level previous note
		NChord *next = 0;			// next note
		NChord *prev = 0;			// previous note
		beamlist = chord->getBeamList();
		beamlist->findRef(chord);
		prev = beamlist->prev();
		if (prev) {
			prevlvl = beamLevel(prev);
		}
		currlvl = beamLevel(chord);
		beamlist->findRef(chord);
		next = beamlist->next();
		if (next) {
			nextlvl = beamLevel(next);
		}
		for (int i=1; i<=currlvl; i++) {
			bool bmnext = (nextlvl >= i);	// beam on next note at level i
			bool bmprev = (prevlvl >= i);	// beam on previous note at level i
			QString bmtype = "";
			if (bmprev && bmnext) {
				bmtype = "continue";
			}
			if (bmprev && !bmnext) {
				bmtype = "end";
			}
			if (!bmprev && bmnext) {
				bmtype = "begin";
			}
			if (!bmprev && !bmnext) {
				if (next) {
					bmtype = "forward hook";
				} else {
					bmtype = "backward hook";
				}
			}
			out_ << "\t\t\t\t<beam number=\"" << i << "\">" << bmtype << "</beam>\n";
		}
	}
	// write <notations> if:
	// - chord starts/ends slur (only at first note of chord)
	// - note is part of tie
	// - chord starts/ends tuplet (only at first note of chord)
	// - and even more ...
	if (needNotations) {
		out_ << "\t\t\t\t<notations>\n";
		if (tupletStop) {
			out_ << "\t\t\t\t\t<tuplet type=\"stop\"/>\n";
		}
		if (tupletStart) {
			out_ << "\t\t\t\t\t<tuplet type=\"start\"/>\n";
		}
		if (slurStop) {
			out_ << "\t\t\t\t\t<slur type=\"stop\" number=\""
				<< voiceStatList_->slurDepth << "\"/>\n";
			voiceStatList_->slurDepth--;
		}
		if (slurStart) {
			voiceStatList_->slurDepth++;
			out_ << "\t\t\t\t\t<slur type=\"start\" number=\""
				<< voiceStatList_->slurDepth << "\"/>\n";
		}
		if (tieStop) {
			out_ << "\t\t\t\t\t<tied type=\"stop\"/>\n";
		}
		if (tieStart) {
			out_ << "\t\t\t\t\t<tied type=\"start\"/>\n";
		}
		if (fermata) {
			// LVIFIX: fermata upright/inverted seems to depends on accent position,
			// which is not easily accessible from here. Therefore only upright
			// fermata's are generated.
			out_ << "\t\t\t\t\t<fermata type=\"upright\"/>\n";
		}
		if (chord->status_ & STAT_ARPEGG) {
			out_ << "\t\t\t\t\t<arpeggiate/>\n";
		}
		// articulations, which are mutually exclusive in NoteEdit
		// LVIFIX: determine placement, see chord.cpp
		// <... placement="above"/>
		// <... placement="below"/>
		if (needArti) {
			out_ << "\t\t\t\t\t<articulations>\n";
			if (chord->status_ & STAT_STACC) {
				out_ << "\t\t\t\t\t\t<staccato/>\n";
			}
			if (chord->status_ & STAT_SFORZ) {
				out_ << "\t\t\t\t\t\t<strong-accent/>\n";
			}
			if (chord->status_ & STAT_PORTA) {
				out_ << "\t\t\t\t\t\t<tenuto/>\n";
			}
			if (chord->status_ & STAT_STPIZ) {
				out_ << "\t\t\t\t\t\t<staccatissimo/>\n";
			}
			if (chord->status_ & STAT_SFZND) {
				out_ << "\t\t\t\t\t\t<accent/>\n";
			}
			out_ << "\t\t\t\t\t</articulations>\n";
		}
		// ornaments, for the time being only trill
		// LVIFIX: sometimes (very short wavy line ?) an empty <ornaments></ornaments> pair
		// is generated
		if (needTrill) {
			out_ << "\t\t\t\t\t<ornaments>\n";
			if (voiceStatList_->trillendpos != 0) {
				if (chord->getBbox()->right() > voiceStatList_->trillendpos) {
					voiceStatList_->trillendpos = 0;
					out_ << "\t\t\t\t\t\t<wavy-line type=\"stop\"/>\n";
				} else {
					out_ << "\t\t\t\t\t\t<wavy-line type=\"continue\"/>\n";
				}
			}
			if (chord->trill_ != 0) {
				if (chord->trill_ > 0) {
					out_ << "\t\t\t\t\t\t<trill-mark/>\n";
				}
				if (voice_elem->findNoteCountTillTrillEnd(chord)) {
					voiceStatList_->trillendpos = chord->getTrillEnd();
					out_ << "\t\t\t\t\t\t<wavy-line type=\"start\"/>\n";
				}
			}
			out_ << "\t\t\t\t\t</ornaments>\n";
		}
		out_ << "\t\t\t\t</notations>\n";
	}
	QString *lyrics;
	for (int i = 0; i < NUM_LYRICS; i++) {
		lyrics = note->chordref->getLyrics(i);
		if (lyrics) {
			out_ << "\t\t\t\t<lyric number=\"" << i+1 << "\">\n";
			out_ << "\t\t\t\t\t<text>" << (*lyrics).utf8() << "</text>\n";
			out_ << "\t\t\t\t</lyric>\n";
		}
	}
	out_ << "\t\t\t</note>\n";
}

// write the voice number

void NMusicXMLExport::outputVoiceNr(int voice_nr) {
	out_ << "\t\t\t\t<voice>" << voice_nr << "</voice>\n";
}

int NMusicXMLExport::calcDuration(int len, status_type status) {
	int dur = len * divisions_ / QUARTER_LENGTH;
	switch (status & DOT_MASK) {
		case STAT_DOUBLE_DOT:
			dur = dur * 7 / 4;
			break;
		case STAT_SINGLE_DOT:
			dur = dur * 3 / 2;
			break;
	}
	return dur;
}

// calculate length expressed in divisions
// be careful not to introduce rounding errors: do division by QUARTER_LENGTH last

void NMusicXMLExport::calcLength(NMusElement *elem, int& dur, QString& type) {
	int len = elem->getSubType();
	status_type status = ( elem->playable() ? elem->playable()->status_ : 0 );
	dur = len * divisions_;
	switch (status & DOT_MASK) {
		case STAT_DOUBLE_DOT:
			dur = dur * 7 / 4;
			break;
		case STAT_SINGLE_DOT:
			dur = dur * 3 / 2;
			break;
	}
	if (status & STAT_TUPLET && (elem->getType() & PLAYABLE) ) {
		dur = dur * elem->playable()->getPlaytime() / elem->playable()->getNumNotes();
	}
	dur /= QUARTER_LENGTH;
	switch (len) {
		case DOUBLE_WHOLE_LENGTH: type = "breve"; break;
		case WHOLE_LENGTH:        type = "whole"; break;
		case HALF_LENGTH:         type = "half"; break;
		case QUARTER_LENGTH:      type = "quarter"; break;
		case NOTE8_LENGTH:        type = "eighth"; break;
		case NOTE16_LENGTH:       type = "16th"; break;
		case NOTE32_LENGTH:       type = "32nd"; break;
		case NOTE64_LENGTH:       type = "64th"; break;
		case NOTE128_LENGTH:      type = "128th"; break;
		default:	          type = ""; /* LVIFIX: error msg ? */ break;
	}
}

// LVIFIX: factor out common code in all length calculations

static int calcLengthForCalcDivisions(NMusElement *elem) {
	int len = elem->getSubType();
	status_type status = ( elem->playable() ? elem->playable()->status_ : 0 );
	switch (status & DOT_MASK) {
		case STAT_DOUBLE_DOT:
			len = len * 7 / 4;
			break;
		case STAT_SINGLE_DOT:
			len = len * 3 / 2;
			break;
	}
	if (status & STAT_TUPLET && (elem->getType() & PLAYABLE) ) {
		len = len * elem->playable()->getPlaytime() / elem->playable()->getNumNotes();
	}
	return len;
}

// helpers for NMusicXMLExport::calcDivisions

typedef QValueList<int> IntVector;
static IntVector integers;
static IntVector primes;

// check if all integers can be divided by div

static bool canDivideBy(int div) {
  bool res = true;
  for (unsigned int i = 0; i < integers.count(); i++) {
    if ((integers[i] <= 1) || ((integers[i] % div) != 0)) {
      res = false;
    }
  }
  return res;
}

// divide all integers by div

static void divideBy(int div) {
  for (unsigned int i = 0; i < integers.count(); i++) {
    integers[i] /= div;
  }
}

// Loop over all voices in all staffs and determine a suitable value for divisions.

// Length of time in MusicXML is expressed in "units", which should allow expressing all time values
// as an integral number of units. Divisions contains the number of units in a quarter note.
// The way NoteEdit stores note length meets this requirement, but sets divisions to a very
// large number: QUARTER_LENGTH = 161280. Solution is to collect all time values required,
// and divide them by the highest common denominator, which is implemented as a series of
// divisions by prime factors. Initialize the list with QUARTER_LENGTH to make sure a quarter
// note can always be written as an integral number of units.

void NMusicXMLExport::calcDivisions(QList<NStaff> *stafflist) {
	NStaff *staff_elem;
	NVoice *voice_elem;
	NMusElement *elem;
	int voice_count;
	int staffcount;
	int i;
	int len;
	IntVector::Iterator it;

	// init
	integers.clear();
	primes.clear();
	integers.append(QUARTER_LENGTH);
	primes.append(2);
	primes.append(3);
	primes.append(5);
	primes.append(7);

	// loop over all elements in all voices in all staffs,
	// calculate and store length
	staffcount = stafflist->count();
	for (i = 0, staff_elem = stafflist->first(); staff_elem; i++, staff_elem = stafflist->next()) {
		voice_count = staff_elem->voiceCount();
		for (int j = 0; j < voice_count; j++) {
			voice_elem = staff_elem->getVoiceNr(j);
			voice_elem->prepareForWriting();
			elem = voice_elem->getCurrentPosition();
			while (elem) {
				switch (elem->getType()) {
					case T_CHORD:
						/* no break */
					case T_REST:
						if (elem->getSubType() == MULTIREST) {
							// ignore multirests
							break;
						}
						len = calcLengthForCalcDivisions(elem);
						it = integers.find(len);
						if (it == integers.end()) {
							integers.append(len);
						}
						break;
					default:
						/* nothing */ ;
				}
				elem = voice_elem->getNextPosition();
			} // end while (elem)
		}
	}

	// do it: divide by all primes as often as possible
	for (unsigned int u = 0; u < primes.count(); u++) {
		while (canDivideBy(primes[u])) {
			divideBy(primes[u]);
		}
	}

	divisions_= integers[0];
}

// find the chord after which dynamic ends

NChord * NMusicXMLExport::findDynEndChord(NStaff * staff_elem, NChord * start_chord)
{
	NChord * res = NULL;
	int x1 = start_chord->getXpos();
	int x2 = start_chord->getDynamicEnd();
	NVoice * v = staff_elem->getVoiceNr(0);
	res = v->findLastChordBetweenXpos(x1, x2);
	return res;
}

// find the chord after which va ends

NChord * NMusicXMLExport::findVaEndChord(NStaff * staff_elem, NChord * start_chord)
{
	NChord * res = NULL;
#if 0

// The "MusicMXL-correct" algorithm to find the chord where va ends:
// step 1: find max tstart of notes under va
// step 2: fine min of tend of notes under va for which tend > max tstart
// note: findLastChordBetweenXpos is expensive and done twice, may need to
// cache result

	int maxtstart = -1;
	int mintend   = -1;
	int x1 = start_chord->getXpos();
	int x2 = start_chord->getVaEnd();
	for (int j = 0; j < staff_elem->voiceCount(); j++) {
		NVoice * v = staff_elem->getVoiceNr(j);
		if (v) {
			NChord * c = v->findLastChordBetweenXpos(x1, x2);
			if (c) {
				// determine max tstart
				if (c->midiTime_ > maxtstart) {
					maxtstart = c->midiTime_;
				}
			}
		}
	}
	for (int j = 0; j < staff_elem->voiceCount(); j++) {
		NVoice * v = staff_elem->getVoiceNr(j);
		if (v) {
			NChord * c = v->findLastChordBetweenXpos(x1, x2);
			if (c) {
				int tend = c->midiTime_ + c->getMidiLength();
				// determine min tend
				if (((mintend == -1) || (tend < mintend)) && (tend > maxtstart)) {
					mintend = tend;
					res = c;
				}
			}
		}
	}
#else

// Note: behaviour described above creates valid and correct MusicXML, but NoteEdit requires
// both va start and va end in the first voice. Thus the generated MusicXML may fail on import.
// Solution: emulate NoteEdit's behaviour: end the va after the last note in the first voice
// that starts under the va line

	int x1 = start_chord->getXpos();
	int x2 = start_chord->getVaEnd();
	NVoice * v = staff_elem->getVoiceNr(0);
	res = v->findLastChordBetweenXpos(x1, x2);

#endif
	return res;
}

// write the part list

void NMusicXMLExport::outputStaffAndVoiceDescription(QList<NStaff> *stafflist) {
	NStaff *staff_elem;
	int staffcount;
	int i;

	staffcount = stafflist->count();
	out_ << endl;
	out_ << "\t<part-list>\n";
	for (i = 0, staff_elem = stafflist->first(); staff_elem; i++, staff_elem = stafflist->next()) {
		out_ << "\t\t<score-part id=\"P" << i+1 << "\">\n";
		if (!staff_elem->staffName_.isEmpty()) {
			out_ << "\t\t\t<part-name>" << staff_elem->staffName_.utf8() << "</part-name>\n";
		} else {
			out_ << "\t\t\t<part-name>" << "Staff " << i+1 << "</part-name>\n";
		}
		out_ << "\t\t\t<score-instrument id=\"P" << i+1 << "-I" << i+1 << "\">\n";
		out_ << "\t\t\t\t<instrument-name>" << i18n( NResource::instrTab[staff_elem->getVoice()] )
			<< "</instrument-name>\n";
		out_ << "\t\t\t</score-instrument>\n";
		out_ << "\t\t\t<midi-instrument id=\"P" << i+1 << "-I" << i+1 << "\">\n";
		out_ << "\t\t\t\t<midi-channel>" << staff_elem->getChannel()+1 << "</midi-channel>\n";
		out_ << "\t\t\t\t<midi-program>" << staff_elem->getVoice()+1 << "</midi-program>\n";
		out_ << "\t\t\t</midi-instrument>\n";
		out_ << "\t\t</score-part>\n";
	}
	out_ << "\t</part-list>\n";
	out_ << endl;
}

// write the clef description

void NMusicXMLExport::outputClefInfo(NClef *clef, int staff_nr) {
	badinfo *bad;
	QString sign = "";
	int line = 0;
	switch (clef->getSubType()) {
		case TREBLE_CLEF:    sign="G"; line=2; break;
		case BASS_CLEF:      sign="F"; line=4; break;
		case SOPRANO_CLEF:   sign="C"; line=1; break;
		case ALTO_CLEF:      sign="C"; line=3; break;
		case TENOR_CLEF:     sign="C"; line=4; break;
		case DRUM_CLEF:	     sign="TBD"; line=2; // LVIFIX
				     bad = new badinfo(MUSICXML_ERRDRUM_CLEF, staff_nr, 0);
				     badlist_.append(bad);
				     break;
		case DRUM_BASS_CLEF: sign="TBD"; line=2; // LVIFIX
				     bad = new badinfo(MUSICXML_ERRDRUM_CLEF, staff_nr, 0);
				     badlist_.append(bad);
				     break;
		default:             NResource::abort("NMusicXMLExport::outputClefInfo");
	}
	// LVIFIX: add number= to clef when using multiple (combined) staffs as in piano music
	out_ << "\t\t\t\t<clef>\n";
	out_ << "\t\t\t\t\t<sign>" << sign << "</sign>\n";
	out_ << "\t\t\t\t\t<line>" << line << "</line>\n";
	switch (clef->getShift()) {
 		case -12: out_ << "\t\t\t\t\t<clef-octave-change>-1</clef-octave-change>\n"; break; 
   		case  12: out_ << "\t\t\t\t\t<clef-octave-change>1</clef-octave-change>\n"; break; 
	}
	out_ << "\t\t\t\t</clef>\n";
}

// helper to create a QString containing chord name and frame

static QString chordNameFrame(NChordDiagram *diag)
{
	QString res = "name=";
	res += diag->getChordName();
	res += ", frame=";
	for (int i = 0; i < 6; i++) {
		if (i != 0) {
			res += " ";
		}
		switch (diag->getStrings()[i]) {
			case -1: res += 'x'; break;
			case  0: res += 'o'; break;
			default: QString s;
			         s.setNum((int) diag->getStrings()[i]);
			         res += s; break;
		}
	}
	return res;
}

// helper to compare a single step
// - if step is present in chord table but differs in chord: no match
//   returns false in that case
// - if step is present in chord but not in chord table: add degree

static bool matchStep(int chordTabStep, int step, bool& addStep)
{
	if ((chordTabStep != -1) && (chordTabStep != step)) { return false; }
	if ((step != -1) && (chordTabStep == -1)) { addStep = true; }
	return true;
}

// write a degree element with degree-value val, -alter alt and -type typ

void NMusicXMLExport::outputDegree(int val, int alt, QString typ)
{
	out_ << "\t\t\t\t<degree>\n";
	out_ << "\t\t\t\t\t<degree-value>" << val << "</degree-value>\n";
	out_ << "\t\t\t\t\t<degree-alter>" << alt << "</degree-alter>\n";
	out_ << "\t\t\t\t\t<degree-type>"  << typ << "</degree-type>\n";
	out_ << "\t\t\t\t</degree>\n";
}

// write a chord diagram
// both chord name and diagram are required to uniquely identify the chord

// Note: NMusicXMLExport::outputDiagram depends on MxmlChordTab[] to be sorted
// on descending number of notes (implements "greedy" match)

void NMusicXMLExport::outputDiagram(NChordDiagram *diag)
{
//	cout << "NMusicXMLExport::outputDiagram ";
	// check for valid frame (i.e. at least one string defined)
	bool hasValidFrame = false;
	for (int i = 0; i < 6; i++) {
		if (diag->getStrings()[i] != -1) {
			hasValidFrame = true;
		}
	}
	if (!hasValidFrame) {
		cout << "chord " << diag->getChordName()
		     << "without frame" << endl;
		return;
	}

	// match chord and chord diagram
	QString stp;
	int alt, s3, s5, s7, s9, s11, s13;
	if (!identifyChord(diag->getChordName(), diag->getStrings(),
			stp, alt, s3, s5, s7, s9, s11, s13)) {
		cout << "could not identify chord ("
		     << chordNameFrame(diag)
		     << ")" << endl;
		return;
	}

	// identifyChord returns steps modulo 12, compensate for that
	if ((s9 != -1) && (s9  < 12)) { s9  += 12; }
	if ((s11!= -1) && (s11 < 12)) { s11 += 12; }
	if ((s13!= -1) && (s13 < 12)) { s13 += 12; }
/*
	cout
		<< " stp=" << stp
		<< " alt=" << alt
		<< " s3=" << s3
		<< " s5=" << s5
		<< " s7=" << s7
		<< " s9=" << s9
		<< " s11=" << s11
		<< " s13=" << s13;
*/
	// handle missing steps
	bool no3  = false;
	bool no5  = false;
	bool no7  = false;
	bool no9  = false;
	bool no11 = false;
	if ((s11 == -1) && (s13 != -1)) { no11 = true; s11 = 17; }
	if ((s9  == -1) && (s11 != -1)) { no9  = true; s9  = 14; }
	if ((s7  == -1) && (s9  != -1)) { no7  = true; s7  = 10; }
	if ((s5  == -1)               ) { no5  = true; s5  =  7; }
	if ((s3  == -1)               ) { no3  = true; s3  =  4; }

	// convert step values to MusicXML kind
	// add3 and add5 don't exist as at this point all chords have s3 != -1
	// and s5 != -1: see last two if statements above
	bool dummy;
	bool add7  = false;
	bool add9  = false;
	bool add11 = false;
	bool add13 = false;
	int ind = -1;		// index of this chord in MxmlChordTab
	for (int i = 0; MxmlChordTab[i].kind != 0; i++) {
		if (   (matchStep(MxmlChordTab[i].s3,  s3,  dummy))
		    && (matchStep(MxmlChordTab[i].s5,  s5,  dummy))
		    && (matchStep(MxmlChordTab[i].s7,  s7,  add7) )
		    && (matchStep(MxmlChordTab[i].s9,  s9,  add9) )
		    && (matchStep(MxmlChordTab[i].s11, s11, add11))
		    && (matchStep(MxmlChordTab[i].s13, s13, add13))) {
			// all steps in MxmlChordTab match exactly,
			// add7..13 are set
			ind = i;
			break;
		}
	}
	if (ind == -1) {
		cout << " chord not supported ("
		     << chordNameFrame(diag)
		     << ")" << endl;
		return;
	}
/*
	else {
		cout << " kind=" << MxmlChordTab[ind].kind;
		if   (no3) { cout <<   " no3"; }
		if   (no5) { cout <<   " no5"; }
		if   (no7) { cout <<   " no7"; }
		if   (no9) { cout <<   " no9"; }
		if  (no11) { cout <<  " no11"; }
		if  (add7) { cout <<  " add7(" <<  s7 << ")"; }
		if  (add9) { cout <<  " add9(" <<  s9 << ")"; }
		if (add11) { cout << " add11(" << s11 << ")"; }
		if (add13) { cout << " add13(" << s13 << ")"; }
		cout << endl;
	}
*/
	// write result
	out_ << "\t\t\t<harmony>\n";
	out_ << "\t\t\t\t<root>\n";
	out_ << "\t\t\t\t\t<root-step>" << stp << "</root-step>\n";
	if (alt) {
		out_ << "\t\t\t\t\t<root-alter>" << alt << "</root-alter>\n";
	}
	out_ << "\t\t\t\t</root>\n";
	out_ << "\t\t\t\t<kind>" << MxmlChordTab[ind].kind << "</kind>\n";
	if   (no3) { outputDegree( 3,      0, "subtract"); }
	if   (no5) { outputDegree( 5,      0, "subtract"); }
	if   (no7) { outputDegree( 7,      0, "subtract"); }
	if   (no9) { outputDegree( 9,      0, "subtract"); }
	if  (no11) { outputDegree(11,      0, "subtract"); }
	if  (add7) { outputDegree( 7,  s7-10,      "add"); }
	if  (add9) { outputDegree( 9,  s9-14,      "add"); }
	if (add11) { outputDegree(11, s11-17,      "add"); }
	if (add13) { outputDegree(13, s13-21,      "add"); }
	outputFrame(diag);
	out_ << "\t\t\t</harmony>\n";
}

// write a direction

void NMusicXMLExport::outputDirection(QString direction, QString placement = "")
{
	out_ << "\t\t\t<direction";
	if (placement != "") {
		out_ << " placement=\"" << placement << "\"";
	}
	out_ <<">\n";
	out_ << "\t\t\t\t<direction-type>\n";
	out_ << direction;
	out_ << "\t\t\t\t</direction-type>\n";
	out_ << "\t\t\t</direction>\n";
}


// write dots

void NMusicXMLExport::outputDots(NMusElement *elem)
{
	if( elem->playable() ) {
		switch (elem->playable()->status_ & DOT_MASK) {
			case STAT_DOUBLE_DOT:
				out_ << "\t\t\t\t<dot/>\n";
				out_ << "\t\t\t\t<dot/>\n";
				break;
			case STAT_SINGLE_DOT:
				out_ << "\t\t\t\t<dot/>\n";
				break;
			default:
				// ignored
				break;
		}
	}
}

// write guitar chord diagram's frame

void NMusicXMLExport::outputFrame(NChordDiagram *diag)
{
	out_ << "\t\t\t\t<frame>\n";
	out_ << "\t\t\t\t\t<frame-strings>6</frame-strings>\n";
	out_ << "\t\t\t\t\t<frame-frets>5</frame-frets>\n";
	out_ << "\t\t\t\t\t<first-fret>" << (int) diag->getFirst()
	     << "</first-fret>\n";
	for (int i = 0; i < 6; i++) {
		int fret = diag->getStrings()[i];
		if (fret != -1) {
			out_ << "\t\t\t\t\t<frame-note>\n";
			out_ << "\t\t\t\t\t\t<string>" << 6-i << "</string>\n";
			out_ << "\t\t\t\t\t\t<fret>" << fret << "</fret>\n";
			out_ << "\t\t\t\t\t</frame-note>\n";
		}
	}
	out_ << "\t\t\t\t</frame>\n";
}

// write the time modification

void NMusicXMLExport::outputTimeMod(NMusElement *elem)
{
	if ((elem->getType() & PLAYABLE) && ( elem->playable()->status_ & STAT_TUPLET) ) {
		out_ << "\t\t\t\t<time-modification>\n";
		out_ << "\t\t\t\t\t<actual-notes>" << (int) elem->playable()->getNumNotes() << "</actual-notes>\n";
		out_ << "\t\t\t\t\t<normal-notes>" << (int) elem->playable()->getPlaytime() << "</normal-notes>\n";
		out_ << "\t\t\t\t</time-modification>\n";
	}
}

// write the time description

void NMusicXMLExport::outputMeter(NTimeSig *timesig) {
	if (timesig) {
		out_ << "\t\t\t\t<time>\n";
		out_ << "\t\t\t\t\t<beats>" << timesig->getNumerator() << "</beats>\n";
		out_ << "\t\t\t\t\t<beat-type>" << timesig->getDenominator() << "</beat-type>\n";
		out_ << "\t\t\t\t</time>\n";
	}
}

// write the key signature

void NMusicXMLExport::outputKeySig(NKeySig *key) {
	badinfo *bad;
	int count;
	status_type kind;

	out_ << "\t\t\t\t<key>\n";
	out_ << "\t\t\t\t\t<fifths>";

	if (key->isRegular(&kind, &count)) {
		switch(kind) {
			case STAT_CROSS:
				switch (count) {
					case 0: out_ << 0; break;
					case 1: out_ << 1; break;
					case 2: out_ << 2; break;
					case 3: out_ << 3; break;
					case 4: out_ << 4; break;
					case 5: out_ << 5; break;
					case 6: out_ << 6; break;
					case 7: out_ << 7; break;
					default: NResource::abort("NMusicXMLExport::outputKeySig", 1);
				}
				break;
			case STAT_FLAT:
				switch (count) {
					case 0: out_ <<  0; break;
					case 1: out_ << -1; break;
					case 2: out_ << -2; break;
					case 3: out_ << -3; break;
					case 4: out_ << -4; break;
					case 5: out_ << -5; break;
					case 6: out_ << -6; break;
					case 7: out_ << -7; break;
					default: NResource::abort("NMusicXMLExport::outputKeySig", 2);
				}
				break;
			case STAT_NO_ACC:
				out_ << 0;
				break;
			default: NResource::abort("NMusicXMLExport::outputKeySig", 3);
		}
	}
	else {
		bad = new badinfo(MUSICXML_ERRIRREGULAER, 1, 0);
		badlist_.append(bad);
		out_ << 0;
	}
	out_ << "</fifths>\n";
	out_ << "\t\t\t\t</key>\n";
}
