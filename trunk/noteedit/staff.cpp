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

#include "staff.h"
#include "voice.h"
#include "mainframewidget.h"
#include "muselement.h"
#include "transpainter.h"
#include "chord.h"
#include "voicedialog.h"
#include "staffPropFrm.h"
#include <stdio.h>
#include <kmessagebox.h>
#include <klocale.h>

#define OVER 10

#define XLABEL_DIST 40
#define YLABEL_DIST 30

NStaff::NStaff(int base, int midi_channel, int voice, NMainFrameWidget *mainWidget) : 
actualKeysig_(&(mainWidget->main_props_) , &staff_props_),
actualClef_(&(mainWidget->main_props_) , &staff_props_) {
	voicelist_.setAutoDelete(true);
	voicelist_.append(theFirstVoice_ = actualVoice_ = new NVoice(this, mainWidget, true));
	actualVoiceNr_ = -1;
	yTop_ = base - ((LINE_OVERFLOW + 1) / 2) * LINE_DIST;
	yBottom_ = base + (4 + (LINE_OVERFLOW + 1) / 2) * LINE_DIST;
	yMid_ = yBottom_ + (yTop_ - yBottom_) / 2;
	main_props_ = &(mainWidget->main_props_);
	mainWidget_ = mainWidget;
	labelDrawPoint_ = QPoint(main_props_->left_page_border + XLABEL_DIST, base - YLABEL_DIST);
	staff_props_.base = base;
	staff_props_.lyricsdist = DEFAULT_LYRICSDIST;
	staff_props_.is_actual = false;
	staff_props_.actual_keysig = &actualKeysig_;
	staff_props_.measureLength = WHOLE_LENGTH;
	reverb_ = 0;
	chorus_ = 0;
	pan_ = 64;
	midiChannel_ = midi_channel;
	midiVoice_ = voice;
	transpose_ = 0;
	midiVolume_ = 80;
	overlength_ = NResource::overlength_;
	underlength_ =  NResource::underlength_;
}

NStaff::~NStaff() {
	voicelist_.clear();
}

NVoice *NStaff::changeActualVoice(int nr) {
	actualVoice_->release();
	if ((actualVoice_ = voicelist_.at(nr == -1 ? 0 : nr)) == 0) {
		NResource::abort("NStaff::changeActualVoice: internal error");
	}
	actualVoiceNr_ = nr;
	return actualVoice_;
}

NVoice *NStaff::addNewVoice() {
	NVoice *new_voice;
	if (voicelist_.count() >= 9) {
		KMessageBox::sorry
			(0,
			 i18n("Cannot create more than 9 voices per staff."),
			 kapp->makeStdCaption("Create voice")
			);
		return 0;
	}
	voicelist_.append(new_voice = new NVoice(this, mainWidget_, false));
	voicelist_.at(actualVoiceNr_);
	mainWidget_->addVoice(new_voice, voicelist_.count());
	return new_voice;
}

void NStaff::addVoices(int nvoices) {
	int i;
	for (i = 0; i < nvoices; ++i)  {
		voicelist_.append(new NVoice(this, mainWidget_, false));
	}
}

/* Move the voice numbered srcNr before the voice numbered tgtNr and becomes the actual voice */
void NStaff::moveVoice(uint srcNr, uint tgtNr) {
	voicelist_.insert(tgtNr, voicelist_.at(srcNr));
	voicelist_.remove(srcNr+1);
	actualVoice_ = voicelist_.at(tgtNr);
}

void NStaff::updateVoiceList(QList<NVoice> *voicelist) {
	NVoice *voice_elem;

	for (voice_elem = voicelist_.first(); voice_elem; voice_elem = voicelist_.next()) {
		if (voicelist->find(voice_elem) == -1) {
			NResource::abort("Staff::updateVoiceList: internal error");
		}
		voicelist->remove();
	}
}

void NStaff::setMuted(bool muted) {
	NVoice *voice_elem;

	for (voice_elem = voicelist_.first(); voice_elem; voice_elem = voicelist_.next()) {
		voice_elem->muted_ = muted;
	}
}

int NStaff::determineMultiRest() {
	NVoice *voice_elem;
	int len;

	voice_elem = voicelist_.first();
	len = voice_elem->determineMultiRest();
	if (len == 0) return 0;

	for (voice_elem = voicelist_.next(); voice_elem; voice_elem = voicelist_.next()) {
		if (voice_elem->determineMultiRest() != len) return 0;
	}
	return len;
}


#ifdef OLD_STAFFDIALOG
NVoice *NStaff::deleteVoice(NVoice *oldVoice) {
	int voiceNr;
	if (oldVoice->isFirstVoice()) {
		KMessageBox::sorry
			(0,
			 i18n("Cannot remove first voice"),
			 kapp->makeStdCaption(i18n("Delete voice"))
			);
		return actualVoice_;
	}
	if ((voiceNr = voicelist_.find(oldVoice)) == -1) {
		NResource::abort("NStaff::deleteVoice: internal error", 1);
	}
	if (KMessageBox::warningYesNo
		(0,
		 i18n("This deletes voice %1! Are you sure?").arg(voiceNr + 1),
		 kapp->makeStdCaption(i18n("Delete voice")),
		 i18n("&Delete")
		)
	    != KMessageBox::Yes
	   ) return actualVoice_ ;
	voicelist_.remove();
	if ((actualVoice_ = voicelist_.current()) == 0)  {
		NResource::abort("NStaff::deleteVoice: internal error", 3);
	}
	printf(" Stelle 3:actualVoice_ = 0x%x, first = %d, actualVoiceNr_ = %d\n", actualVoice_, actualVoice_->isFirstVoice(), actualVoiceNr_); fflush(stdout);
	actualVoiceNr_ = voicelist_.at();
	mainWidget_->removeVoice(oldVoice, actualVoice_, actualVoiceNr_, voicelist_.count());
	return actualVoice_;
}

#else

int NStaff::deleteVoice(NVoice *oldVoice, VoiceDialog *voicedialog, staffPropFrm *staffPropForm) {
	int voiceNr;
	if (oldVoice->isFirstVoice()) {
		KMessageBox::sorry
			(0,
			 i18n("Cannot remove first voice"),
			 kapp->makeStdCaption(i18n("Delete voice"))
			);
		return -1;
	}
	if ((voiceNr = voicelist_.find(oldVoice)) == -1) {
		NResource::abort("NStaff::deleteVoice: internal error", 1);
	}
	if( staffPropForm )
		if (KMessageBox::warningYesNo
			(staffPropForm,
			i18n("This deletes voice %1! Are you sure?").arg(voiceNr + 1),
			kapp->makeStdCaption(i18n("Delete voice")),
			i18n("&Delete")
			)
		!= KMessageBox::Yes
		) return -1 ;
	else if( voicedialog )
		if (KMessageBox::warningYesNo
			(voicedialog,
			i18n("This deletes voice %1! Are you sure?").arg(voiceNr + 1),
			kapp->makeStdCaption(i18n("Delete voice")),
			i18n("&Delete")
			)
		!= KMessageBox::Yes
		) return -1 ;
	voicelist_.remove();
	if ((actualVoice_ = voicelist_.current()) == 0)  {
		NResource::abort("NStaff::deleteVoice: internal error", 3);
	}
	actualVoiceNr_ = voicelist_.at();
	mainWidget_->removeVoice(oldVoice, actualVoice_, actualVoiceNr_, voicelist_.count());
	return actualVoiceNr_;
}
#endif


void NStaff::setBase(int base) {
	staff_props_.base = base;
	yTop_ = base - ((LINE_OVERFLOW + 1) / 2) * LINE_DIST;
	yBottom_ = base + (4 + (LINE_OVERFLOW + 1) / 2) * LINE_DIST;
	yMid_ = yBottom_ + (yTop_ - yBottom_) / 2;
	labelDrawPoint_ = QPoint(main_props_->left_page_border + XLABEL_DIST, staff_props_.base - YLABEL_DIST);
}

void NStaff::startRepositioning() {
	NVoice *voice_elem;
	for (voice_elem = voicelist_.first(); voice_elem; voice_elem = voicelist_.next()) {
		voice_elem->startRepositioning();
	}
	actualKeysig_.deleteTempAccents();
}

void NStaff::validateKeysig(int startidx, int insertpos) {
	NVoice *voice_elem;
	int lastbarpos;

	voice_elem = voicelist_.first();
	lastbarpos = voice_elem->validateKeysig(startidx, insertpos);
	for (voice_elem = voicelist_.next(); voice_elem; voice_elem = voicelist_.next()) {
		voice_elem->validateKeysigAccordingPos(lastbarpos, insertpos);
	}
}


void NStaff::getElementsAfter(QList<NPositStr> *plist, int mytime, int *num_positions, int *min_time) {
	NPositStr *posit;
	NVoice *voice_elem;
	int i;

	for (i = 0, voice_elem = voicelist_.first(); voice_elem; voice_elem = voicelist_.next(), i++) {
		posit = voice_elem->getElementAfter(mytime);
		if (posit) {
			plist->append(posit);
			(*num_positions)++;
		}
		if (posit) {
			if (posit->ev_time < *min_time) *min_time = posit->ev_time;
		}
	}
}

bool NStaff::trimmRegionToWholeStaff(int *x0, int *x1) {
	return voicelist_.first()->trimmRegionToWholeStaff(x0, x1);
}

int NStaff::findLineOf(int pitchNumber, int acYLine, int xpos) {
	int line = 0;
	int linedist;
	theFirstVoice_->validateKeysig(-1, xpos);
	line = actualClef_.lineOfC4() + pitchNumber;
	linedist = line - acYLine;
	while (linedist > 3)
    	{
          	linedist -= 7;
		line -= 7;
	}
	while (linedist < -3)
	{
        	linedist += 7;
		line += 7;
	}
	if (line > MAXLINE) line -= 7;
	if (line < MINLINE) line += 7;
	return line;
}

void NStaff::paperDimensiones(int width) {
	width_ = width;
	nettoWidth_ = width_ - main_props_->left_page_border - (int) ((float) (RIGHT_PAGE_BORDER) / main_props_->zoom);
}

void NStaff::deleteBlocksAccording() {
	NVoice *voice_elem;

	for (voice_elem = voicelist_.first(); voice_elem; voice_elem = voicelist_.next()) {
		if (voice_elem != NResource::voiceWithSelectedRegion_) {
			voice_elem->findAppropriateElems();
		}
		voice_elem->deleteBlock();
	}
}

void NStaff::autoBeam() {
	NVoice *voice_elem;

	for (voice_elem = voicelist_.first(); voice_elem; voice_elem = voicelist_.next()) {
		voice_elem->autoBeam();
	}
}
void NStaff::autoBar() {
	NVoice *voice_elem;

	voice_elem = voicelist_.first();
	voice_elem->autoBar();
	voice_elem->computeMidiTime(false, false);
	for (voice_elem = voicelist_.next(); voice_elem; voice_elem = voicelist_.next()) {
		theFirstVoice_->resetSpecialElement();
		voice_elem->autoBarVoice123andSoOn();
	}
	for (voice_elem = voicelist_.first(); voice_elem; voice_elem = voicelist_.next()) {
		voice_elem->computeMidiTime(false, false);
		voice_elem->collChords();
	}
}


void NStaff::deleteBlock(NVoice *preferredVoice) {
	NVoice *voice_elem;
	if (actualVoiceNr_ == -1) {
		for (voice_elem = voicelist_.first(); voice_elem; voice_elem = voicelist_.next()) {
			if (voice_elem == preferredVoice) continue;
			voice_elem->findAppropriateElems();
			voice_elem->deleteBlock();
		}
		preferredVoice->deleteBlock();
	}
	else {
		if (actualVoice_ == preferredVoice) {
			actualVoice_->deleteBlock();
		}
		else {
			actualVoice_->findAppropriateElems();
			actualVoice_->deleteBlock();
		}
	}
}

void NStaff::grabElementsAccording() {
	NVoice *voice_elem;

	for (voice_elem = voicelist_.first(); voice_elem; voice_elem = voicelist_.next()) {
		if (voice_elem != NResource::voiceWithSelectedRegion_) {
			voice_elem->findAppropriateElems();
		}
		voice_elem->grabElements();
	}
}

void NStaff::transpose(int semitones) {
	NVoice *voice_elem;
	bool first;

	if (semitones==0) return; //don't touch the score if transposing for 0 semitones
	
	if (actualVoiceNr_ == -1) {
		for (voice_elem = voicelist_.first(), first = true; voice_elem; voice_elem = voicelist_.next(), first = false) {
			if (NResource::windowWithSelectedRegion_ && voice_elem != NResource::voiceWithSelectedRegion_) {
				voice_elem->findAppropriateElems();
			}
			if (!first) {
				theFirstVoice_->prepareForWriting();
			}
			voice_elem->transpose(semitones, NResource::windowWithSelectedRegion_ != 0);
		}
	}
	else {
		if (NResource::windowWithSelectedRegion_ && actualVoice_ != NResource::voiceWithSelectedRegion_) {
			actualVoice_->findAppropriateElems();
		}
		actualVoice_->transpose(semitones, NResource::windowWithSelectedRegion_ != 0);
	}
}

void NStaff::setHalfsTo(status_type type) {
	NVoice *voice_elem;

	if (actualVoiceNr_ == -1) {
		for (voice_elem = voicelist_.first(); voice_elem; voice_elem = voicelist_.next()) {
			if (NResource::windowWithSelectedRegion_ && voice_elem != NResource::voiceWithSelectedRegion_) {
				voice_elem->findAppropriateElems();
			}
			voice_elem->setHalfsTo(type, NResource::windowWithSelectedRegion_ != 0);
		}
	}
	else {
		if (NResource::windowWithSelectedRegion_ && actualVoice_ != NResource::voiceWithSelectedRegion_) {
			actualVoice_->findAppropriateElems();
		}
		actualVoice_->setHalfsTo(type, NResource::windowWithSelectedRegion_ != 0);
	}
}

void NStaff::cleanupRests(int shortestRest) {
	NVoice *voice_elem;

	if (actualVoiceNr_ == -1) {
		for (voice_elem = voicelist_.first();  voice_elem; voice_elem = voicelist_.next()) {
			if (NResource::windowWithSelectedRegion_ && voice_elem != NResource::voiceWithSelectedRegion_) {
				voice_elem->findAppropriateElems();
			}
			voice_elem->cleanupRests(shortestRest, NResource::windowWithSelectedRegion_ != 0);
		}
	}
	else {
		if (NResource::windowWithSelectedRegion_ && actualVoice_ != NResource::voiceWithSelectedRegion_) {
			actualVoice_->findAppropriateElems();
		}
		actualVoice_->setHalfsTo(shortestRest, NResource::windowWithSelectedRegion_ != 0);
	}
}

void NStaff::collChords() {
	NVoice *voice_elem;

	for (voice_elem = voicelist_.first(); voice_elem; voice_elem = voicelist_.next()) {
		voice_elem->collChords();
	}
}

void NStaff::setHalfsAccordingKeySig() {
	NVoice *voice_elem;
	bool first;

	for (voice_elem = voicelist_.first(), first = true; voice_elem; voice_elem = voicelist_.next(), first = false) {
		if (!first) {
			theFirstVoice_->prepareForWriting();
		}
		voice_elem->setHalfsAccordingKeySig(true);
	}
}

void NStaff::performClefChange(int type, int shift) {
	NVoice *voice_elem;
	int dist = UNDEFINED_DIST;
	int stop_x = (1 << 30);

	for (voice_elem = voicelist_.first(); voice_elem; voice_elem = voicelist_.next()) {
		if (NResource::windowWithSelectedRegion_ && voice_elem != NResource::voiceWithSelectedRegion_) {
			voice_elem->findAppropriateElems();
		}
		voice_elem->performClefChange(type, shift, NResource::windowWithSelectedRegion_ != 0, &dist, &stop_x);
	}
}

void NStaff::grabElements(NVoice *preferredVoice) {
	NVoice *voice_elem;

	for (voice_elem = voicelist_.first(); voice_elem; voice_elem = voicelist_.next()) {
		if (voice_elem == preferredVoice) continue;
		voice_elem->findAppropriateElems();
		voice_elem->grabElements();
	}
	preferredVoice->grabElements();
}

void NStaff::startPlaying(int starttime) {
	NVoice *voice_elem;
	segnoClef_ = repeatClef_ = playClef_ = voicelist_.getFirst()->getFirstClef();
	segnoKeySig_ = repeatKeySig_ = playKeySig_ = 0;
	for (voice_elem = voicelist_.first(); voice_elem; voice_elem = voicelist_.next()) {
		voice_elem->startPlaying(starttime);
	}
}

void NStaff::setMarker() {
	NVoice *voice_elem;
	repeatClef_ = playClef_;
	if (playKeySig_) {
		repeatKeySig_ = new NKeySig(main_props_ , &staff_props_);
		repeatKeySig_->changeInContextKeySig(playKeySig_);
	}
	else {
		repeatKeySig_ = 0;
	}
	voice_elem = voicelist_.first();
	for (voice_elem = voicelist_.next(); voice_elem; voice_elem = voicelist_.next()) {
		voice_elem->setMarker();
	}
}

void NStaff::setSegnoMarker() {
	NVoice *voice_elem;
	segnoClef_ = playClef_;
	if (playKeySig_) {
		segnoKeySig_ = new NKeySig(main_props_ , &staff_props_);
		segnoKeySig_->changeInContextKeySig(playKeySig_);
	}
	else {
		segnoKeySig_ = 0;
	}
	voice_elem = voicelist_.first();
	for (voice_elem = voicelist_.next(); voice_elem; voice_elem = voicelist_.next()) {
		voice_elem->setSegnoMarker();
	}
}

void NStaff::setCodaMarker(int timeOf2ndCoda) {
	NVoice *voice_elem;
	int oldidx;

	oldidx = voicelist_.at(); /* it is called during "startPlaying()" */
	voice_elem = voicelist_.first();
	for (voice_elem = voicelist_.next(); voice_elem; voice_elem = voicelist_.next()) {
		voice_elem->setCodaMarker(timeOf2ndCoda);
	}
	if (oldidx >= 0) voicelist_.at(oldidx);
}

void NStaff::gotoMarker(bool again) {
	NVoice *voice_elem;
	playClef_ = repeatClef_;
	if (playKeySig_) {
		/* This delete commented out fixes crash bug #3503, which hitted midi playing
		and export whith multiple repeats. The tool pmem doesn't show a memory leak
		caused by this. Similar constructs should come under investigation, see below?
		The parameter "again" is set but not actually in use. 4/2005.
		*/
		// delete playKeySig_;
		
	}
	playKeySig_ = repeatKeySig_;
	voice_elem = voicelist_.first();
	for (voice_elem = voicelist_.next(); voice_elem; voice_elem = voicelist_.next()) {
		voice_elem->gotoMarker(again);
	}
}

void NStaff::gotoSegnoMarker() {
	NVoice *voice_elem;
	playClef_ = segnoClef_;
	if (playKeySig_) {
		delete playKeySig_;
	}
	playKeySig_ = segnoKeySig_;
	voice_elem = voicelist_.first();
	for (voice_elem = voicelist_.next(); voice_elem; voice_elem = voicelist_.next()) {
		voice_elem->gotoSegnoMarker();
	}
}

void NStaff::stopAllVoices() {
	NVoice *voice_elem;
	voice_elem = voicelist_.first();
	for (voice_elem = voicelist_.next(); voice_elem; voice_elem = voicelist_.next()) {
		voice_elem->stopVoice();
	}
	if (playKeySig_) {
		delete playKeySig_;
	}
}

void NStaff::gotoCodaMarker() {
	NVoice *voice_elem;
	voice_elem = voicelist_.first();
	for (voice_elem = voicelist_.next(); voice_elem; voice_elem = voicelist_.next()) {
		voice_elem->gotoCodaMarker();
	}
}




void NStaff::handleEnding1() {
	NVoice *voice_elem;
	voice_elem = voicelist_.first();
	for (voice_elem = voicelist_.next(); voice_elem; voice_elem = voicelist_.next()) {
		voice_elem->handleEnding1();
	}
}
	

void NStaff::pasteAtPosition(int xpos, NStaff *from) {
	NVoice *srcVoice, *destVoice;
	bool complete = true;
	QList<NMusElement> *clipboard;
	int countof128th, dest_time, part_in_measure;

	if (actualVoiceNr_ == -1) {
		if (from == 0 || from == this) {
			srcVoice = voicelist_.first();
			clipboard = srcVoice->getClipBoard();
			srcVoice->pasteAtPosition(xpos, clipboard, true, &part_in_measure, &dest_time, &countof128th);
			for (srcVoice = voicelist_.next(); srcVoice; srcVoice = voicelist_.next()) {
				clipboard = srcVoice->getClipBoard();
				srcVoice->pasteAtMidiTime(dest_time, part_in_measure, countof128th, clipboard);
			}
		}
		else {
			if (from->voiceCount() > voiceCount()) {
				KMessageBox::sorry
					(0,
					 i18n("The destination staff has less voices than source staff"),
					 kapp->makeStdCaption("paste")
					);
				return;
			}
			destVoice = voicelist_.first(), srcVoice = from->voicelist_.first();
			clipboard = srcVoice->getClipBoard();
			destVoice->pasteAtPosition(xpos, clipboard, true, &part_in_measure, &dest_time, &countof128th);
			for (destVoice = voicelist_.next(), srcVoice = from->voicelist_.next(); destVoice && srcVoice;
		             destVoice = voicelist_.next(), srcVoice = from->voicelist_.next()) {
				clipboard = srcVoice->getClipBoard();
				destVoice->pasteAtMidiTime(dest_time, part_in_measure, countof128th, clipboard);
			}
		}
	}
	else {
		if (from == 0 || from == this) {
			clipboard = actualVoice_->getClipBoard();
			actualVoice_->pasteAtPosition(xpos, clipboard, true, &part_in_measure, &dest_time /* dummy */, &countof128th /* dummy */);
		}
		else {
			srcVoice = from->getActualVoice(); 
			if (!actualVoice_->isFirstVoice() && srcVoice->isFirstVoice()) {
				KMessageBox::sorry
					(0,
					 i18n("You paste a first voice to no-first voice! This deletes all non-first-voice elements!"),
					 kapp->makeStdCaption("paste")
					);
				complete = false;
			}
			clipboard = srcVoice->getClipBoard();
			actualVoice_->pasteAtPosition(xpos, clipboard, complete, &part_in_measure /* dummy */, &dest_time /* dummy */, &countof128th /* dummy */);
		}
	}
}

void NStaff::drawContext() {
	playClef_->drawContextClef();
	if (playKeySig_) {
		playKeySig_->drawContextKeySig();
	}
}
	

void NStaff::draw(int left, int right) {
	int i;
	NVoice *voice_elem;

	main_props_->p->beginYtranslated();

	main_props_->p->setPen
		(staff_props_.is_actual ?
		 NResource::selectedStaffPen_ : NResource::staffPen_
		);
	for (i = 0; i < 5; ++i) {
		 main_props_->p->drawLine(main_props_->left_page_border, staff_props_.base + i * LINE_DIST, main_props_->left_page_border + nettoWidth_, staff_props_.base + i * LINE_DIST);
	}

	main_props_->p->end();
	if (NResource::showStaffNames_ && !staffName_.isEmpty()) {
		main_props_->p->beginUnclippedYtranslated();
		main_props_->p->setPen
			(staff_props_.is_actual ?
		 	NResource::selectedStaffPen_ : NResource::staffPen_
			);
		main_props_->p->toggleToScaledText(true);
		main_props_->p->setFont( main_props_->scaledBold2_ );
		main_props_->p->setPen
			(staff_props_.is_actual ?
			 NResource::selectedStaffNamePen_ : NResource::staffNamePen_
			);
		main_props_->p->drawScaledText(labelDrawPoint_, staffName_);
		main_props_->p->end();
	}
	for (voice_elem = voicelist_.first(); voice_elem; voice_elem = voicelist_.next()) {
		voice_elem->draw(left, right, voice_elem == actualVoice_);
	}
}

int NStaff::intersects(const QPoint p) const {
	if (p.y() < yTop_ || p.y() > yBottom_) return -1;
	return (p.y() > yMid_) ? p.y() - yMid_ : yMid_ - p.y();
}


// Correct pitch read by MusicXML importer (sounding pitch) to NoteEdit's
// notated pitch.
// Must be called before correctReadTrillsSlursAndDynamicsAndVAs() is executed,
// as it relies on the presence of the 8va markers set by the setOctaviation
// functions.

void NStaff::correctPitchBecauseOfVa()
{
	NMusElement *elem;
	int sign = 0;
	int tstart = -1;
	int tend;
	NVoice *voice_elem;
	// search through voice 0 for 8va marks
	voice_elem = getVoiceNr(0);
	for (elem = voice_elem->getFirstPosition();
	     elem;
	     elem = voice_elem->getNextPosition()) {
		if (elem->getType() != T_CHORD) continue;
		switch (elem->va_ & 0x00030000) {
		case 0x00010000:
			tstart = elem->midiTime_;
			sign = (elem->va_ & 0x8000) ? -1 : 1;
			break;
		case 0x00030000:
			tend = elem->midiTime_ + elem->getMidiLength();
			if (tstart != -1) {
				// correct pitch in all voices
				NVoice *v;
				for (v = voicelist_.first();
				     v;
				     v = voicelist_.next()) {
				     	v->correctPitchBecauseOfVa(tstart,
								   tend,
								   sign);
				}
			}
			tstart = -1;
			break;
		default:
			; /* ignore */
		}
	}
}
