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
#include <fstream>
#else
#include <fstream.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include "voice.h"
#include "resource.h"
#include "rest.h"
#include "staff.h"
#include "midimapper.h"
#include "timesig.h"
#include "transpainter.h"
#include "mainframewidget.h"
#include "numberdisplay.h"
#include "chord.h"
#include "sign.h"
#include "text.h"
#include "tempotrack.h"
#include "beaming.h"

#include <kmessagebox.h>
#include <klocale.h>

#define PAGEBORDER 8

#define LEFT_BORDER 20
#define SELECTHEIGHT 200

#define TRILL_MIDI_LENGTH (NOTE32_LENGTH)
#define ARPEGGIO_MIDI_LENGTH (NOTE64_LENGTH)
#define DYNAMIC_PRECISION (NOTE32_LENGTH)

#define MAX_LINE_IN_EDITOR 80

undostr NVoice::undoelem_[MAXUNDO];
int NVoice::undoptr_;
int NVoice::undocounter_;
int NVoice::lastundoptr_;

undostr NVoice::redoelem_[MAXUNDO];
int NVoice::redoptr_;
int NVoice::redocounter_;

/* ------------- creating voice -------------------------------------------------*/

NVoice::NVoice(NStaff *staff, NMainFrameWidget *mainWidget, bool isFirstVoice) :
wordPattern1_("[^ \r\n\t][^ \r\n\t]*"), wordPattern2_("<[^>\r\n\t]*>"), escapedApostroph_("\\") {

	int i;
	mainWidget_ = mainWidget;
	firstVoice_ = isFirstVoice;
	theStaff_ = staff;
	main_props_ = &(mainWidget->main_props_);
	midiEndTime_ = 0;
	currentElement_ = 0;
	yRestOffs_ = 0;
	virtualChord_.setAutoDelete(true);
	octave_ = -1;
	firstVolume_ = true;
	voiceSet_ = false;
	muted_ = false;
	stemPolicy_ = STEM_POL_INDIVIDUAL;
	startElement_ = endElement_ = 0;
	invalidateReUndo(true);
	for (i = 0; i < MIDI_EVENT_RING; i += 2) {
		midievents_[i].next = &(midievents_[(i+2) % MIDI_EVENT_RING]);
		midievents_[i].notehalt = &(midievents_[i+1]);
	}
}

NVoice::~NVoice() {
	musElementList_.setAutoDelete(true);
	musElementList_.clear();
	virtualChord_.setAutoDelete(true);
	virtualChord_.clear();
	invalidateReUndo(true);
}



/*---------------------- setting voice properies -----------------------------------------*/

void NVoice::emptyVoice() {
	musElementList_.clear();
	virtualChord_.clear();
	currentElement_ = 0;
	invalidateReUndo(true);
}

void NVoice::getChordDiagramms(QList<chordDiagramName> *cdiagList, bool *gridsused, bool firstcall, bool *gridproblem) {
	NMusElement *elem;
	NChordDiagram *diag;
	chordDiagramName *diag_name;
	int oldidx;
	bool firstElem = true;
	int max_underscores;

	oldidx = musElementList_.at();
	for (elem = musElementList_.first(); elem;  elem = musElementList_.next()) {
		if (!elem->playable() || ((diag = elem->playable()->getChordChordDiagram()) == NULL)) continue;
		max_underscores = -1;
		for (diag_name = cdiagList->first(); diag_name; diag_name = cdiagList->next()) {
			if (diag_name->cdiagramm->isEqual(diag)) {
				max_underscores = -2;
				break;
			}
			if (diag_name->cdiagramm->isAmbigous(diag)) {
				if (diag_name->NumOfUnderscores > max_underscores) max_underscores = diag_name->NumOfUnderscores;
			}
		}
		if (firstcall && firstElem) {
			*gridsused = diag->showDiagram_;
		}
		else {
			if (*gridsused != diag->showDiagram_) {
				*gridproblem = true;
				*gridsused = true;
			}
		}
		if (max_underscores > -2) cdiagList->append(new chordDiagramName(diag, max_underscores+1));
		firstElem = false;
	}
	if (oldidx >= 0) musElementList_.at(oldidx);
				
}

void NVoice::paperDimensiones(int width) {
	NNote *note;
	theStaff_->paperDimensiones(width);
	for (note = virtualChord_.first(); note; note = virtualChord_.next()) {
		note->tie_start_point_up = QPoint((int) ((float) (width +5)  / main_props_->zoom), note->tie_start_point_up.y());
		note->tie_start_point_down = QPoint((int) ((float) (width +5)  / main_props_->zoom), note->tie_start_point_down.y());
		note->tie_back_point_up = QPoint((int) ((float) (width +2)  / main_props_->zoom), note->tie_back_point_up.y());
		note->tie_back_point_down = QPoint((int) ((float) (width +2)  / main_props_->zoom), note->tie_back_point_down.y());
	}
}


/*--------------------------------- search for something in voice ----------------------------- */

int NVoice::getElemState(property_type *properties, bool *playable) {
	NChord *chord;
	*properties = 0;
	*playable = false;
	if( !currentElement_ ) return -1;
	if( !( *playable = (currentElement_->getType() & PLAYABLE))) {
		return -1;
	}
	*properties = currentElement_->playable()->properties_;
	if (currentElement_->getType() == T_CHORD) {
		chord = (NChord *) currentElement_;
		*properties |= chord->getActualNote()->properties;
	}
	return currentElement_->getSubType();
}

int NVoice::checkElementForNoteInsertion(const int line, const QPoint p, property_type *properties, bool *playable, bool *delete_elem, bool *insertNewNote, int offs) {
	bool found;
	NMusElement *ac_elem;
	NChord *chord;
	int val;

	*properties = 0;
	*playable = false;

	found = false;
	ac_elem = musElementList_.first();
	while (!found && ac_elem != 0) {
		switch (val = ac_elem->intersects_horizontally(p)) {
			case 0: found = true; break;
			case -1:
				if (currentElement_) {
					currentElement_->setActual(false);
					currentElement_->draw();
				}
				*delete_elem = 0;
				currentElement_ = 0;
				return -1;
			default: ac_elem = musElementList_.next();
		}
	}
	if (currentElement_) {
		currentElement_->setActual(false);
		currentElement_->draw();
	}
	if (!found) {
		currentElement_ = 0;
		*delete_elem = 0;
		return -1;
	}
	currentElement_ = ac_elem;
	currentElement_->setActual(true);
	if (! (*playable = (currentElement_->getType() & PLAYABLE))) {
		return -1;
	}
	*properties = currentElement_->playable()->properties_;
	if (currentElement_->getType() == T_CHORD) {
		chord = (NChord *) currentElement_;
		if (chord->setActualNote(line))  {
			if (*delete_elem) {
				if (deleteActualNote()) {
					*delete_elem = false; /* Otherwise there's only one note --> delete chord */
				}
			}
			*insertNewNote = false; /* avoid insertion if successfully deleted */
		}
		else {
			if (*insertNewNote) {
				if (line <= MAXLINE && line >= MINLINE) {
					 insertNewNoteAt(line, p, offs);
					*insertNewNote = false; /* Otherwise insert new chord */
				}
				else {
					currentElement_->setActual(false);
					currentElement_ = 0;
					return -1;
				}
			}
			else {
				currentElement_->setActual(false);
				currentElement_ = 0;
			}
			*delete_elem = 0;
			return -1;
		}
		*properties |= chord->getActualNote()->properties;
	}
	return currentElement_->getSubType();
}

bool NVoice::checkElementForElementInsertion(const QPoint p) {
	bool found;
	NMusElement *ac_elem;

	found = false;
	ac_elem = musElementList_.first();
	while (!found && ac_elem != 0) {
		switch (ac_elem->intersects(p)) {
			case 0: found = true; break;
			case -1:
				if (currentElement_) {
					currentElement_->setActual(false);
					currentElement_->draw();
				}
				return false;
			default: ac_elem = musElementList_.next();
		}
	}
	if (currentElement_) {
		currentElement_->setActual(false);
		currentElement_->draw();
	}
	if (!found) {
		currentElement_ = 0;
		return false;
	}
	currentElement_ = ac_elem;
	currentElement_->setActual(true);
	return true;
}

void NVoice::findAppropriateElems() {
	bool found_start, found_end;
	int xpos0, xpos1;

	
	startElement_ = endElement_ = 0;
	if (musElementList_.count() < 1) return;
	if (!NResource::voiceWithSelectedRegion_->startElement_ || !NResource::voiceWithSelectedRegion_->endElement_) return;
	xpos0 = (NResource::voiceWithSelectedRegion_->endElementIdx_ > NResource::voiceWithSelectedRegion_->startElemIdx_) ?
		 NResource::voiceWithSelectedRegion_->startElement_->getXpos() : NResource::voiceWithSelectedRegion_->endElement_->getXpos();
	xpos1 = (NResource::voiceWithSelectedRegion_->endElementIdx_ > NResource::voiceWithSelectedRegion_->startElemIdx_) ?
		 NResource::voiceWithSelectedRegion_->endElement_->getBbox()->right() : NResource::voiceWithSelectedRegion_->startElement_->getBbox()->right();
	startElemIdx_ = searchPositionAndUpdateSigns(xpos0, &startElement_, &found_start);
	endElementIdx_ = searchPositionAndUpdateSigns(xpos1, &endElement_, &found_end);
	
	if (!found_start && !found_end) {
		startElement_ = endElement_ = 0;
		return;
	}
	if (found_start && !found_end) {
		endElementIdx_ = musElementList_.count() - 1;
		endElement_ = musElementList_.getLast();
	}
	else if (!found_start) {
		NResource::abort("NVoice::findAppropriateElems: internal error");
	}
	else {
		if ((endElement_ = musElementList_.prev()) == 0) {
			startElement_ = endElement_ = 0;
			return;
		}
		endElementIdx_ = musElementList_.at();
	}
}


void NVoice::grabElements() {
	int x0, x1, idx;
	bool found;
	NMusElement *ac_elem;

	clipBoard_.clear();
	// if (startElement_ == 0) printf("startElement_ == 0\n");
	// if (endElement_ == 0) printf("endElement_ == 0\n");

	if (!startElement_ || !endElement_) return;
	x0 = (endElementIdx_ > startElemIdx_) ? startElemIdx_ : endElementIdx_;
	x1 = (endElementIdx_ > startElemIdx_) ? endElementIdx_ : startElemIdx_;
	found = false;
	ac_elem = musElementList_.at(x0);

	idx = x0;
	while (idx <= x1  && ac_elem != 0) {
		clipBoard_.append(ac_elem);
		ac_elem = musElementList_.next();
		idx = musElementList_.at();
	}
}

void NVoice::findStartElemAt(int x0, int x1) {
	int xp, dist, last_idx, mindist = (1 << 30);
	bool found = false;
	NMusElement *ac_elem, *last_elem;

	if (x0 <= x1) {
		ac_elem = musElementList_.first();
		while (!found && ac_elem != 0) {
			xp = ac_elem->getXpos();
			dist = abs(xp - x0);
			if (dist <= mindist) {
				mindist = dist;
				last_elem = ac_elem;
				last_idx = musElementList_.at();
				ac_elem = musElementList_.next();
			}
			else {
				startElement_ = last_elem;
				startElemIdx_ = last_idx;
				found = true;
			}
		}
		if (!found) {
			startElement_ = musElementList_.last();
			startElemIdx_ = musElementList_.at();
		}
	}
	else {
		ac_elem = musElementList_.last();
		while (!found && ac_elem != 0) {
			xp = ac_elem->getBbox()->right();
			dist = abs(xp - x0);
			if (dist <= mindist) {
				mindist = dist;
				last_elem = ac_elem;
				last_idx = musElementList_.at();
				ac_elem = musElementList_.prev();
			}
			else {
				startElement_ = last_elem;
				startElemIdx_ = last_idx;
				found = true;
			}
		}
		if (!found) {
			startElement_ = musElementList_.first();
			startElemIdx_ = musElementList_.at();
		}
	}
}


void NVoice::trimmRegion(int *x0, int *x1) {
	int x0n;
	NMusElement *ac_elem;
	bool found;
	if (!startElement_) return;
	found = false;
	if (startElemIdx_ < 0) return;
	ac_elem = musElementList_.at(startElemIdx_);
	// if (ac_elem == 0) printf("ac_elem nicht gefunden\n");
	if (*x0 <= *x1) {
		x0n = startElement_->getBbox()->left();
		while (!found && ac_elem != 0) {
			if (ac_elem->getBbox()->right() > *x1) {
				found = true;
				endElement_ = ac_elem;
				endElementIdx_ = musElementList_.at();
				*x1 = ac_elem->getBbox()->right();
			}
			else {
				ac_elem = musElementList_.next();
			}
		}
		if (!found) {
			endElement_ = musElementList_.last();
			if (endElement_) {
				endElementIdx_ = musElementList_.at();
			}
		}
	}
	else {
		x0n = startElement_->getBbox()->right();
		while (!found && ac_elem != 0) {
			if (ac_elem->getBbox()->left() < *x1) {
				found = true;
				endElement_ = ac_elem;
				endElementIdx_ = musElementList_.at();
				*x1 = ac_elem->getBbox()->left();
			}
			else {
				ac_elem = musElementList_.prev();
			}
		}
		if (!found) {
			endElement_ = musElementList_.first();
			if (endElement_) {
				endElementIdx_ = musElementList_.at();
			}
		}
	}
	*x0 = x0n;
}

bool NVoice::trimmRegionToWholeStaff(int *x0, int *x1) {
	NMusElement *elem;

	findStartElemAt(0, 10 /* dummy */);
	if (!startElement_) return false;
	if (startElemIdx_ < 0) return false;
	elem = musElementList_.getLast();
	if (elem == 0) return false;
	endElement_ = elem;
	endElementIdx_ = musElementList_.count() - 1;
	*x0 = startElement_->getBbox()->left();
	*x1 = elem->getBbox()->right();
	return true;
}


bool NVoice::wholeTupletDeleted(NPlayable *ac_elem, int posOfFirst, int posOfLast) {
	NPlayable *firstTupletElem, *lastTupletElem;

	lastTupletElem = ac_elem->getTupletList()->last();
	firstTupletElem =  ac_elem->getTupletList()->first();
	if (firstTupletElem->getXpos() >= posOfFirst && lastTupletElem->getXpos() <= posOfLast) {
		return true;
	}
	return false;
}
		
bool NVoice::wholeBeamDeleted(NChord *ac_elem, int posOfFirst, int posOfLast) {
	NChord *lastBeamedChord, *firstBeamedChord;

	lastBeamedChord = ac_elem->getBeamList()->last();
	firstBeamedChord = ac_elem->getBeamList()->first();
	return (firstBeamedChord->getXpos() >= posOfFirst && lastBeamedChord->getXpos() <= posOfLast);
}
		

void NVoice::deleteBlock() {
	int x0, x1, idx;
	bool found = false;
	NNote *note;
	QList<NNote> *partlist;
	NMusElement *ac_elem, *stop_elem, *start_elem;
	NChord *chord;

	if (!startElement_ || !endElement_) return;
	x0 = (endElementIdx_ > startElemIdx_) ? startElemIdx_ : endElementIdx_;
	x1 = (endElementIdx_ > startElemIdx_) ? endElementIdx_ : startElemIdx_;
	if (currentElement_) {
		currentElement_->setActual(false);
		currentElement_ = 0;
	}
	idx = x0;
	stop_elem = musElementList_.at(x1);
	start_elem = ac_elem = musElementList_.at(x0);
	
	/* Store the last usable MIDI event */
	main_props_->lastMidiTime = start_elem->midiTime_;

	createUndoElement(x0, x1 - x0 + 1, -(x1 - x0 + 1));
	while (ac_elem != 0 && !found) {
		found = ac_elem == stop_elem;
		if (ac_elem->getType() == T_CHORD) {
			chord = (NChord *) ac_elem;
			if ((chord->properties_ & PROP_TUPLET) && !wholeTupletDeleted(chord, start_elem->getXpos(), stop_elem->getXpos())) {
				chord->breakTuplet();
			}
			if (chord->properties_ & PROP_BEAMED) {
				bool wh;
				if (!(wh = wholeBeamDeleted(chord, start_elem->getXpos(), stop_elem->getXpos())) && (chord->lastBeamed() || chord->beamHasOnlyTwoChords())) {
					chord->breakBeames();
				}
				else if (!wholeBeamDeleted(chord, start_elem->getXpos(), stop_elem->getXpos())) {
					chord->removeFromBeam();
				}
			}
			chord->checkSlures();
			musElementList_.remove();
			partlist = chord->getNoteList();
			for (note = partlist->first(); note; note = partlist->next()) {
				reconnectDeletedTies(note);
			}
		}
		else {
			if (ac_elem->playable() && (ac_elem->playable()->properties_ & PROP_TUPLET) && !wholeTupletDeleted( ac_elem->playable(), start_elem->getXpos(), stop_elem->getXpos())) {
				ac_elem->playable()->breakTuplet();
			}
			musElementList_.remove();
		}
		ac_elem = musElementList_.current();
		idx = musElementList_.at();
	}
}

void NVoice::resetSlured() {
	NChord *chord;
	if (!currentElement_) return;
	if (currentElement_->getType() != T_CHORD) return;
	chord = (NChord *) currentElement_;
	if (!(chord->properties_ & PROP_SLURED)) return;
	createUndoElement(currentElement_, 1, 0);
	chord->setSlured(false);
}

void NVoice::setSlured() {
	bool found;
	int x0, x1, idx;
	NMusElement *ac_elem;
	NChord *slured_chord, *partner;

	if (!startElement_ || !endElement_) return;
	x0 = (endElementIdx_ > startElemIdx_) ? startElemIdx_ : endElementIdx_;
	x1 = (endElementIdx_ > startElemIdx_) ? endElementIdx_ : startElemIdx_;
	if (currentElement_) {
		currentElement_->setActual(false);
		currentElement_ = 0;
	}
	found = false;
	idx = x0;
	ac_elem = musElementList_.at(x0);
	createUndoElement(x0, x1 - x0 + 1, 0);
	while (!found && ac_elem != 0 && idx <= x1) {
		if (ac_elem->getType() == T_CHORD) {
			slured_chord = (NChord *) ac_elem;
			found = true;
		}
		ac_elem = musElementList_.next();
		idx = musElementList_.at();
	}
	if (!found) {
		deleteLastUndo();
		return;
	}
	found = false;
	ac_elem = musElementList_.at(x1);
	while (!found && ac_elem != 0) {
		if (ac_elem->getType() == T_CHORD) {
			partner = (NChord *) ac_elem;
			found = true;
		}
		ac_elem = musElementList_.next();
	}
	if (!found) {
		deleteLastUndo();
		return;
	}
	slured_chord->setSlured(true, partner);
}

void NVoice::reconnectBeames() {
	int oldidx;
	NMusElement *ac_elem;
	NChord *chord;
	QList<NChord> *beamlist = new QList<NChord>();

	oldidx = musElementList_.at();
	ac_elem = currentElement_;
	if (musElementList_.find(currentElement_) == -1) {
		NResource::abort("reconnectBeames: internal error");
	}
	ac_elem->calculateDimensionsAndPixmaps();
	chord = (NChord *) ac_elem;
	beamlist->append((NChord *) ac_elem);
	ac_elem = musElementList_.prev();
	while (ac_elem && ac_elem->playable() && (ac_elem->playable()->properties_ & PROP_BEAMED) && !chord->lastBeamed()) {
		if (ac_elem->getType() & BAR_SYMS) {
			ac_elem = musElementList_.prev();
			continue;
		}
		ac_elem->calculateDimensionsAndPixmaps();
		beamlist->insert(0, (NChord *) ac_elem);
		ac_elem = musElementList_.prev();
	}
	chord->computeBeames(beamlist, stemPolicy_);
	if (oldidx >= 0) musElementList_.at(oldidx);
}

void NVoice::reconnectTuplets() {
	int oldidx;
	int numNotes, playtime;
	NMusElement *ac_elem;
	QList<NPlayable> *tupletlist = new QList<NPlayable>();

	oldidx = musElementList_.at();
	ac_elem = currentElement_;
	if (musElementList_.find(ac_elem) == -1) {
		NResource::abort("NVoice::reconnectTuplets: internal error");
	}
	numNotes = (ac_elem->getType() & PLAYABLE) ? ac_elem->playable()->getNumNotes() : 0;
	playtime = (ac_elem->getType() & PLAYABLE) ? ac_elem->playable()->getPlaytime() : 0;
	ac_elem->calculateDimensionsAndPixmaps();
	if( ac_elem->playable() )
		tupletlist->append( ac_elem->playable() );
	ac_elem = musElementList_.prev();
	while (ac_elem && ac_elem->playable() && (ac_elem->playable()->properties_ & PROP_TUPLET) && !(ac_elem->playable()->properties_ & PROP_LAST_TUPLET)) {
		tupletlist->insert(0, ac_elem->playable() );
		ac_elem = musElementList_.prev();
	}
	if( ac_elem->playable() )
		ac_elem->playable()->computeTuplet(tupletlist, numNotes, playtime);
	if (oldidx >= 0) musElementList_.at(oldidx);
}


void NVoice::breakCopiedTuplets() {
	int oldidx;
	NMusElement *ac_elem;

	oldidx = musElementList_.at();
	ac_elem = currentElement_;
	if( !ac_elem->playable() )
		return;
	ac_elem->playable()->unsetTuplet();
	ac_elem = musElementList_.prev();
	while (ac_elem && ac_elem->playable() && (ac_elem->playable()->properties_ & PROP_TUPLET) && !(ac_elem->playable()->properties_ & PROP_LAST_TUPLET)) {
		ac_elem->playable()->unsetTuplet();
		ac_elem = musElementList_.prev();
	}
	if (oldidx >= 0) musElementList_.at(oldidx);
}



int NVoice::computeSlurDist(NChord *chord) {
	int oldidx,idx;
	NChord *partner;
	char *err = "computeSlurDist: internal error";

	if ((oldidx = musElementList_.find(chord)) == -1) {
		NResource::abort(err, 1);
	}
	if (!(chord->properties_ & PROP_SLURED)) NResource::abort(err, 2);
	partner = chord->getSlurPartner();
	if (!partner) NResource::abort(err, 3);
	idx = musElementList_.find(partner);
	if (idx == -1) NResource::abort(err, 4);
	musElementList_.at(oldidx);
	return idx - oldidx;
}

int NVoice::findHighestLineInTrill(NChord *chord)  {
	int trillendX;
	int oldidx;
	int higestline = (1 << 30); 
	bool found = false;
	char *err = "FindHighestLineInTrill: internal error";
	NMusElement *elem;
	
	oldidx = musElementList_.at();
	if (musElementList_.find(chord) < 0) {
		NResource::abort(err, 1);
	}
	trillendX = chord->getTrillEnd();
	for (elem = musElementList_.current(); elem && !found; elem = musElementList_.next()) {
		if (elem->getBbox()->left() > trillendX) {
			found = true;
		}
		switch (elem->getType()) {
			case T_CHORD: 	if (elem->chord()->getTopY2() < higestline) higestline = elem->chord()->getTopY2();
		}
	}
	if (oldidx >= 0) musElementList_.at(oldidx);
	if (!found || higestline == (1 << 30)) {
		NResource::abort(err, 2);
	}
	return ((theStaff_->staff_props_.base - higestline) / (LINE_DIST / 2));
}

int NVoice::findBorderLineInVa(NChord *chord)  {
	int vaendX;
	int oldidx;
	int higestline = (1 << 30);
	int minline = -2;
	int h;
	bool found = false;
	char *err = "findBorderLineInVa: internal error";
	NMusElement *elem;

	
	oldidx = musElementList_.at();
	if (musElementList_.find(chord) < 0) {
		NResource::abort(err, 1);
	}
	vaendX = chord->getVaEnd();
	for (elem = musElementList_.current(); elem && !found; elem = musElementList_.next()) {
		if (elem->getBbox()->left() > vaendX) {
			found = true;
		}
		switch (elem->getType()) {
			case T_CHORD: if (chord->va_ > 0) {
						if (elem->chord()->getTopY2() < higestline) higestline = elem->chord()->getTopY2();
						break;
					}
					h = elem->chord()->getNoteList()->first()->line - 2;
					if (h < minline) minline = h;
					break;
		}
	}
	if (oldidx >= 0) musElementList_.at(oldidx);
	if (!found || chord->va_ > 0 && higestline == (1 << 30)) {
		higestline = chord->getTopY2();
	}
	if (chord->va_ > 0) {
		return ((theStaff_->staff_props_.base - higestline) / (LINE_DIST / 2)+10);
	}
	return minline - 4;
}


int NVoice::findNoteCountTillTrillEnd(NChord *chord) {
	int notecount = 0, trillendX;
	int oldidx;
	NMusElement *elem;
	bool found = false;

	oldidx = musElementList_.at();
	if (musElementList_.find(chord) < 0) {
		NResource::abort("findNoteCountTillTrillEnd: internal error");
	}
	trillendX = chord->getTrillEnd();
	for (elem = musElementList_.next(); elem && !found; elem = musElementList_.next()) {
		if (elem->getBbox()->left() > trillendX) {
			found = true;
		}
		else {
			switch (elem->getType()) {
				case T_CHORD:
				case T_REST: notecount++;
			}
		}
	}
	if (oldidx >= 0) musElementList_.at(oldidx);
	return notecount;
}

int NVoice::findTimeOfTrillEnd(NChord *chord, int *destmestime, int *mescount) {
	int oldidx;
	int trillendtime, trillendX;
	NMusElement *elem;
	*mescount = 0;
	int measures = 0;
	bool found;
	int idx;
	int lastBarTime = 0;
	oldidx = musElementList_.at();

	if ((idx = musElementList_.find(chord)) < 0) {
		NResource::abort("findTimeOfTrillEnd: internal error");
	}
	found = false;
	for (elem = musElementList_.current(); elem && !found;  elem = musElementList_.prev()) {
		if (elem->getType() == T_SIGN && (elem->getSubType() & BAR_SYMS)) {
			lastBarTime = elem->midiTime_;
			found = true;
		}
	}

	musElementList_.at(idx);

	trillendtime = chord->midiTime_;
	trillendX = chord->getTrillEnd();
	found = false;
	for (elem = musElementList_.next(); elem && !found;  elem = musElementList_.next()) {
		if (elem->getBbox()->left() > trillendX) {
			found = true;
		}
		else if (elem->getType() == T_SIGN && (elem->getSubType() & BAR_SYMS)) {
			measures++;
			lastBarTime = elem->midiTime_;
		}
		else {
			switch (elem->getType()) {
				case T_CHORD:
				case T_REST: trillendtime = elem->midiTime_;
						(*mescount) += measures;
						*destmestime = lastBarTime;
						measures = 0;
			}
		}
	}
	if (oldidx >= 0) musElementList_.at(oldidx);
	return trillendtime;
}


int NVoice::findTimeOfSlurEnd(NChord *chord, int *destmestime, int *mescount) {
	int oldidx;
	int idx;
	NMusElement *elem, *slurpartner, *lastBarSym;
	oldidx = musElementList_.at();
	*mescount = 0;
	*destmestime = 0;

	if ((idx = musElementList_.find(chord)) < 0) {
		NResource::abort("findTimeOfSlurEnd: internal error", 1);
	}
	slurpartner = chord->getSlurPartner();
	if (!firstVoice_) {
		if (lastBarSym = theStaff_->countBarSymsBetween(chord->getXpos(), slurpartner->getXpos(), mescount)) {
			*destmestime = lastBarSym->midiTime_;
		}
		if (oldidx >= 0) musElementList_.at(oldidx);
		return slurpartner->midiTime_;
	}
	if ((idx = musElementList_.find(chord)) < 0) {
		NResource::abort("findTimeOfSlurEnd: internal error", 1);
	}
	for (elem = musElementList_.current();elem;elem = musElementList_.prev()) {
		if (elem->getType() == T_SIGN && (elem->getSubType() & BAR_SYMS)) {
			*destmestime = elem->midiTime_;
			break;
		}
	}
	for (elem = musElementList_.at(idx); elem;  elem = musElementList_.next()) {
		if (elem == slurpartner) {
			if (oldidx >= 0) musElementList_.at(oldidx);
			return slurpartner->midiTime_;

		}
		else if (elem->getType() == T_SIGN && (elem->getSubType() & BAR_SYMS)) {
			(*mescount)++;
			*destmestime = elem->midiTime_;
		}
	}
	NResource::abort("findTimeOfSlurEnd: internal error", 2);
	return 0; /* dummy */
}

int NVoice::findTimeOfVaEnd(NChord *chord, int *destmestime, int *mescount) {
	int oldidx;
	int vaendtime, vaendX;
	NMusElement *elem;
	*mescount = 0;
	int measures = 0;
	bool found;
	int idx;
	int lastBarTime = 0;
	oldidx = musElementList_.at();

	if ((idx = musElementList_.find(chord)) < 0) {
		NResource::abort("findTimeOfVaEnd: internal error");
	}
	found = false;
	for (elem = musElementList_.current(); elem && !found;  elem = musElementList_.prev()) {
		if (elem->getType() == T_SIGN && (elem->getSubType() & BAR_SYMS)) {
			lastBarTime = elem->midiTime_;
			found = true;
		}
	}

	musElementList_.at(idx);

	vaendtime = chord->midiTime_;
	vaendX = chord->getVaEnd();
	found = false;
	for (elem = musElementList_.next(); elem && !found;  elem = musElementList_.next()) {
		if (elem->getBbox()->left() > vaendX) {
			found = true;
		}
		else if (elem->getType() == T_SIGN && (elem->getSubType() & BAR_SYMS)) {
			measures++;
			lastBarTime = elem->midiTime_;
		}
		else {
			switch (elem->getType()) {
				case T_CHORD:
				case T_REST: vaendtime = elem->midiTime_;
						(*mescount) += measures;
						*destmestime = lastBarTime;
						measures = 0;
			}
		}
	}
	if (oldidx >= 0) musElementList_.at(oldidx);
	return vaendtime;
}

int NVoice::findTimeOfDynamicEnd(NChord *chord, int *sourcemestime, int *destmestime, int *mescount) {
	int oldidx;
	int dynamicendtime = -1, dynamicendX;
	int idx;
	NMusElement *elem;
	bool found = false;
	int measures = 0;
	*mescount = 0;
	int lastBarTime = *sourcemestime;

	oldidx = musElementList_.at();
	if ((idx = musElementList_.find(chord)) < 0) {
		NResource::abort("findTimeOfDynamicEnd: internal error");
	}
	dynamicendX = chord->getDynamicEnd();
	for (idx++, elem = musElementList_.next(); elem && !found; idx++, elem = musElementList_.next()) {
		if (elem->getBbox()->left() > dynamicendX) {
			found = true;
		}
		else if (elem->getType() & T_SIGN && (elem->getSubType() & BAR_SYMS)) {
			measures++;
			lastBarTime = elem->midiTime_;
		}
		else {
			switch (elem->getType()) {
				case T_CHORD:
				case T_REST: dynamicendtime = elem->midiTime_;
						(*mescount) += measures;
						*destmestime = lastBarTime;
						measures = 0;
			}
		}
	}
	if (oldidx >= 0) musElementList_.at(oldidx);
	return dynamicendtime;
}

int NVoice::findEndOfCrescendo(NChord *chord) {
	int oldidx;
	int endofcrescendo = -1, dynamicendX;
	NMusElement *elem;
	bool found = false;

	oldidx = musElementList_.at();
	if (musElementList_.find(chord) < 0) {
		NResource::abort("findEndOfCrescendo: internal error");
	}
	dynamicendX = chord->getDynamicEnd();
	elem = chord;
	endofcrescendo = chord->midiTime_ + chord->getMidiLength();
	for (elem = musElementList_.next(); elem && !found; elem = musElementList_.next()) {
		if (elem->getBbox()->left() > dynamicendX) {
			found = true;
		}
		else {
			switch (elem->getType()) {
				case T_CHORD: endofcrescendo = elem->midiTime_ + elem->getMidiLength();
			}
		}
	}
	if (oldidx >= 0) musElementList_.at(oldidx);
	return endofcrescendo;
}

bool NVoice::beginsWithGrace() {
	int oldidx;
	NMusElement *elem;

	oldidx = musElementList_.at();
	elem = musElementList_.first();
	while (elem) {
		if (elem->getType() & PLAYABLE) {
			if (oldidx >= 0) musElementList_.at(oldidx);
			return (elem->getType() == T_CHORD && (elem->chord()->properties_ & PROP_GRACE));
		}
		elem = musElementList_.next();
	}
	if (oldidx >= 0) musElementList_.at(oldidx);
	return false;
}
	
void NVoice::pasteAtPosition(int xpos, QList<NMusElement> *clipboard, bool complete, int *part_in_current_measure, int *dest_midi_time, int *countof128th) {
	int idx, startidx, i, num = 0;
	bool found;
	NChord *chord;
	QList<NChord> lastSluredClones;
	QList<NChord> *beamlist;
	NMusElement *ac_elem, *elem_before = 0, *clone_elem;
	QList<NMusElement> *clonelist;
	int lastbartime;
	ac_elem = clipboard->first();

	lastSluredClones.setAutoDelete(false);
	*part_in_current_measure = *dest_midi_time = 0;
	if (currentElement_)  {
		currentElement_->setActual(false);
		currentElement_ = 0;
	}
	idx = searchPositionAndUpdateSigns(xpos, &ac_elem, &found, &elem_before, countof128th, 0, 0, &lastbartime);
	if (found && elem_before && elem_before->playable() && ac_elem->playable() ) {
		if ((elem_before->playable()->properties_ & PROP_BEAMED) && (ac_elem->playable()->properties_ & PROP_BEAMED)) {
			if (((NChord *) elem_before)->getBeamList() == ((NChord *) ac_elem)->getBeamList()) {
				currentElement_ = musElementList_.prev();
				breakBeames();
				musElementList_.at(idx);
				currentElement_ = 0;
			}
		}
		if ((elem_before->playable()->properties_ & PROP_TUPLET) && (ac_elem->playable()->properties_ & PROP_TUPLET) && 
				elem_before->playable() && ac_elem->playable() ) {
			if ( elem_before->playable()->getTupletList() == ac_elem->playable()->getTupletList()) {
				currentElement_ = musElementList_.prev();
				breakTuplet();
				musElementList_.at(idx);
				currentElement_ = 0;
			}
		}
	}
	if (found) {
		startidx = idx = musElementList_.at();
		*dest_midi_time = ac_elem->midiTime_;
		*part_in_current_measure = ac_elem->midiTime_ - lastbartime;
	}
	else {
		startidx = musElementList_.count();
		if (startidx < 0) {
			startidx = 0;
		}
		else if (musElementList_.count()) {
			*dest_midi_time = musElementList_.getLast()->midiTime_ + musElementList_.getLast()->getMidiLength();
			*part_in_current_measure = musElementList_.getLast()->midiTime_ +  musElementList_.getLast()->getMidiLength() - lastbartime;
		}
	}
	clonelist = new QList<NMusElement>();
	for (ac_elem = clipboard->first(); ac_elem; ac_elem = clipboard->next()) {
		if (!complete) {
			switch (ac_elem->getType()) {
				case T_CHORD: 
				case T_REST: break;
				default: continue;
					 break;
			}
		}
					      
		num++;
		clone_elem = ac_elem->clone();
		if (!complete && ac_elem->getType() == T_CHORD) {
			for (i = 0; i < NUM_LYRICS; ((NChord *) clone_elem)->deleteLyrics(i++));
		}
		clone_elem->setStaffProps(&(theStaff_->staff_props_));
		clone_elem->setMainProps(main_props_);
		if (clone_elem->getType() == T_REST) ((NRest *) clone_elem)->setVoiceOffs(&yRestOffs_);
		clone_elem->setActual(false);
		if (found) {
			musElementList_.insert(idx, clone_elem);
			idx++;
		}
		else {
			musElementList_.append(clone_elem);
		}
		currentElement_ = clone_elem;
		clonelist->append(clone_elem);
		switch (clone_elem->getType()) {
			case T_KEYSIG: ((NKeySig *) clone_elem)->setClef(&theStaff_->actualClef_);
					break;
			case T_CHORD: chord = (NChord *) ac_elem;
				     if (chord->properties_ & PROP_SLURED) {
					lastSluredClones.insert(0, (NChord *) clone_elem);
			             }
				     if ((chord->properties_ & PROP_PART_OF_SLUR)) {
						if (lastSluredClones.isEmpty()) {
							chord->resetSlurBackward();
						}
						else {
							lastSluredClones.first()->setSlured(true, (NChord *) clone_elem);
							lastSluredClones.remove();
						}
				     }
				     reconnectCopiedTies((NChord *) clone_elem);
				     if ( ((NChord *)clone_elem) ->lastBeamed()) {
				     		reconnectBeames();
				     }
			case T_REST:
				     if (clone_elem->playable()->properties_ & PROP_LAST_TUPLET) {
						if (checkTuplets(clipboard, ac_elem->playable()->getTupletList())) {
							reconnectTuplets();
						}
#ifdef AAA /* see below! */
						else {
							breakCopiedTuplets();
						}
#endif
				     }
				     break;
		}
	}
	for (clone_elem = clonelist->first(); clone_elem; clone_elem = clonelist->next()) {
		switch (clone_elem->getType()) {
			case T_CHORD: if (clone_elem->chord()->properties_ & PROP_BEAMED) {
					chord = (NChord *) clone_elem;
					beamlist = chord->getBeamList();
					if (beamlist->count() < 2 || !lastChordContained(clonelist, beamlist)) {
						chord->resetBeamFlag();
					}
				     }
 				     clone_elem->chord()->trill_ = clone_elem->chord()->dynamic_ = 0;
			case T_REST:
				    if (clone_elem->playable()->properties_ & PROP_TUPLET) {
					if (!allElemsContained(clonelist, clone_elem->playable()->getTupletList())) {
						clone_elem->playable()->resetTupletFlag();
					}
				    }
				    break;
		}
	}
	for (chord = lastSluredClones.first(); chord; chord = lastSluredClones.next()) {
		chord->resetSlurForward();
	}
		
	createUndoElement(startidx, 0, clonelist->count());
	delete clonelist;
}


void NVoice::pasteAtMidiTime(int dest_time, int part_in_measure, int countof128th, QList<NMusElement> *clipboard) {
	int idx, startidx, num = 0;
	bool found;
	NChord *chord;
	QList<NChord> lastSluredClones;
	QList<NChord> *beamlist;
	NMusElement *ac_elem, *elem_before = 0, *clone_elem;
	QList<NMusElement> *clonelist;
	property_type properties;
	int dotcount;
	int diff_total, len, len2, lastElemTime;
	NRest *rest;
	int idx_of_first_inserted_rest = -1;

	if (currentElement_)  {
		currentElement_->setActual(false);
		currentElement_ = 0;
	}
	found = false;
	ac_elem = musElementList_.first();
	while (!found && ac_elem != 0) {
		if (ac_elem->midiTime_ < dest_time) {
			elem_before = ac_elem;
			ac_elem = musElementList_.next();
		}
		else {
			found = true;
		}
	}
	if (found && elem_before && elem_before->playable() && ac_elem->playable() ) {
		if ((elem_before->playable()->properties_ & PROP_BEAMED) && (ac_elem->playable()->properties_ & PROP_BEAMED)) {
			if (((NChord *) elem_before)->getBeamList() == ((NChord *) ac_elem)->getBeamList()) {
				idx = musElementList_.at();
				currentElement_ = musElementList_.prev();
				breakBeames();
				musElementList_.at(idx);
				currentElement_ = 0;
			}
		}
		if ((elem_before->playable()->properties_ & PROP_TUPLET) && (ac_elem->playable()->properties_ & PROP_TUPLET) &&
				elem_before->playable() && ac_elem->playable() ) {
			if ( elem_before->playable()->getTupletList() == ac_elem->playable()->getTupletList()) {
				idx = musElementList_.at();
				currentElement_ = musElementList_.prev();
				breakTuplet();
				musElementList_.next();
				currentElement_ = 0;
			}
		}
	}
	lastElemTime = elem_before ? elem_before->midiTime_ + elem_before->getMidiLength() : 0;
	diff_total = dest_time - lastElemTime - part_in_measure;
	clonelist = new QList<NMusElement>();
	if (diff_total > 0) {
		countof128th *= MULTIPLICATOR;
		len = diff_total % countof128th;
		if (len > 0) {
			while (len >= MULTIPLICATOR) {
				len2 = quant(len, &dotcount, DOUBLE_WHOLE_LENGTH);
				len -= dotcount ? 3 * len2 / 2 : len2;
				diff_total -= dotcount ? 3 * len2 / 2 : len2;
				properties = PROP_HIDDEN;
				if (dotcount) properties |= PROP_SINGLE_DOT;
				rest = new NRest(main_props_, theStaff_->getStaffPropsAddr(), &yRestOffs_, len2, properties); 
				if (found) {
					if (idx_of_first_inserted_rest == -1) idx_of_first_inserted_rest = idx;
					musElementList_.insert(idx++, rest);
				}
				else {
					if (idx_of_first_inserted_rest == -1) idx_of_first_inserted_rest = musElementList_.count();
					musElementList_.append(rest);
				}
			}
		}
		while (diff_total >= MULTIPLICATOR) {
			len2 = quant(countof128th, &dotcount, DOUBLE_WHOLE_LENGTH);
			diff_total -= dotcount ? 3 * len2 / 2 : len2;
			properties = PROP_HIDDEN;
			if (dotcount) properties |= PROP_SINGLE_DOT;
			rest = new NRest(main_props_, theStaff_->getStaffPropsAddr(), &yRestOffs_, len2, properties); 
			if (found) {
				if (idx_of_first_inserted_rest == -1) idx_of_first_inserted_rest = idx;
				musElementList_.insert(idx++, rest);
			}
			else {
				if (idx_of_first_inserted_rest == -1) idx_of_first_inserted_rest = musElementList_.count();
				musElementList_.append(rest);
			}
		}
		while (part_in_measure >= MULTIPLICATOR) {
			len2 = quant(part_in_measure, &dotcount, DOUBLE_WHOLE_LENGTH);
			part_in_measure -= dotcount ? 3 * len2 / 2 : len2;
			properties = PROP_HIDDEN;
			if (dotcount) properties |= PROP_SINGLE_DOT;
			rest = new NRest(main_props_, theStaff_->getStaffPropsAddr(), &yRestOffs_, len2, properties); 
			if (found) {
				if (idx_of_first_inserted_rest == -1) idx_of_first_inserted_rest = idx;
				musElementList_.insert(idx++, rest);
			}
			else {
				if (idx_of_first_inserted_rest == -1) idx_of_first_inserted_rest = musElementList_.count();
				musElementList_.append(rest);
			}
		}
	}
	if (found) {
		startidx = idx = musElementList_.at();
	}
	else {
		startidx = musElementList_.count();
		if (startidx < 0) {
			startidx = 0;
		}
	}
	for (ac_elem = clipboard->first(); ac_elem; ac_elem = clipboard->next()) {
		num++;
		clone_elem = ac_elem->clone();
		clone_elem->setStaffProps(&(theStaff_->staff_props_));
		clone_elem->setMainProps(main_props_);
		if (clone_elem->getType() == T_REST) ((NRest *) clone_elem)->setVoiceOffs(&yRestOffs_);
		clone_elem->setActual(false);
		if (found) {
			musElementList_.insert(idx, clone_elem);
			idx++;
		}
		else {
			musElementList_.append(clone_elem);
		}
		currentElement_ = clone_elem;
		clonelist->append(clone_elem);
		switch (clone_elem->getType()) {
			case T_KEYSIG: ((NKeySig *) clone_elem)->setClef(&theStaff_->actualClef_);
					break;
			case T_CHORD: chord = (NChord *) ac_elem;
				     if (chord->properties_ & PROP_SLURED) {
					lastSluredClones.insert(0, (NChord *) clone_elem);
			             }
				     if ((chord->properties_ & PROP_PART_OF_SLUR)) {
						if (lastSluredClones.isEmpty()) {
							chord->resetSlurBackward();
						}
						else {
							lastSluredClones.first()->setSlured(true, (NChord *) clone_elem);
							lastSluredClones.remove();
						}
				     }
				     reconnectCopiedTies((NChord *) clone_elem);
				     if (((NChord *)clone_elem)->lastBeamed()) {
				     		reconnectBeames();
				     }
			case T_REST:
				     if (clone_elem->playable()->properties_ & PROP_LAST_TUPLET) {
						if (checkTuplets(clipboard, ac_elem->playable()->getTupletList())) {
							reconnectTuplets();
						}
#ifdef AAA /* see below! */
						else {
							breakCopiedTuplets();
						}
#endif
				     }
				     break;
		}
	}
	for (clone_elem = clonelist->first(); clone_elem; clone_elem = clonelist->next()) {
		switch (clone_elem->getType()) {
			case T_CHORD: if (clone_elem->chord()->properties_ & PROP_BEAMED) {
					chord = (NChord *) clone_elem;
					beamlist = chord->getBeamList();
					if (beamlist->count() < 2 || !lastChordContained(clonelist, beamlist)) {
						chord->resetBeamFlag();
					}
				     }
			case T_REST:
				    if (clone_elem->playable()->properties_ & PROP_TUPLET) {
					if (!lastElemContained(clonelist, clone_elem->playable()->getTupletList())) {
						clone_elem->playable()->resetTupletFlag();
					}
				    }
				    break;
		}
	}
	for (chord = lastSluredClones.first(); chord; chord = lastSluredClones.next()) {
		chord->resetSlurForward();
	}
	if (idx_of_first_inserted_rest != -1) {
		createUndoElement(idx_of_first_inserted_rest, 0, clonelist->count() + startidx - idx_of_first_inserted_rest);
	}
	else {
		createUndoElement(startidx, 0, clonelist->count());
	}
	delete clonelist;
}
	

/*-------------------------------- drawing voice ----------------------------------------------*/



void NVoice::draw(int left, int right, bool is_actual) {
	int oldidx;
	NMusElement *elem;
	int flags = (is_actual || main_props_->voiceDisplay->isZero()) ? DRAW_INDIRECT : DRAW_INDIRECT_GREY;
	if (main_props_->voiceDisplay->isZero()) {
		flags |= DRAW_NO_HIDDEN_REST;
	}
	
	oldidx = musElementList_.at();
	for (elem=musElementList_.first(); elem != 0; elem=musElementList_.next()) {
		if (elem->getXposDecorated() < left || elem->getXpos() > right) continue;
		elem->draw(flags);
	}
	if (oldidx >= 0) musElementList_.at(oldidx);
}

/*--------------------- changes due to user interaction ------------------------------------*/

void NVoice::release() {
	if (!currentElement_) return;
	currentElement_->setActual(false);
	currentElement_->draw();
	currentElement_ = 0;
}

void NVoice::makeKeysigAndClefActual() {
	int oldidx, idx;
	NMusElement *elem;
	char *err = "makeKeysigAndClefActual: internal error";

	if ((oldidx = musElementList_.find(currentElement_)) < 0) return;
	for (idx = 0, elem = musElementList_.first(); elem && idx < oldidx; elem = musElementList_.next(), idx++) {
		switch (elem->getType()) {
			case T_CLEF: theStaff_->actualClef_.change((NClef *) elem);
				     theStaff_->actualKeysig_.setClef((NClef *) elem);
				     break;
			case T_KEYSIG: theStaff_->actualKeysig_.change((NKeySig*) elem);
					break;
		}
	}
	if (!elem) NResource::abort(err, 1);
	theStaff_->actualKeysig_.deleteTempAccents();
	for (elem = musElementList_.at(oldidx - 1); elem; elem = musElementList_.prev()) {
		if (elem->getType() == T_SIGN && (elem->getSubType() & BAR_SYMS)) break;
		if (elem->getType() != T_CHORD) continue;
		((NChord *) elem)->accumulateAccidentals(&(theStaff_->actualKeysig_));
	}
}
				     
		


void NVoice::moveUp(int up) {
	if (!currentElement_) return;
	if (currentElement_->getType() != T_CHORD) return;
	createUndoElement(currentElement_, 1, 0);
	breakTies((NChord *) currentElement_);
	makeKeysigAndClefActual();
	((NChord *)currentElement_)->moveUp(up, stemPolicy_, &(theStaff_->actualKeysig_));
	reconnectTiesAtferMove((NChord *) currentElement_);
	if (!NResource::allowInsertEcho_) return;
	NResource::mapper_->playImmediately(&(theStaff_->actualClef_), (NChord *) currentElement_,
		theStaff_->getVoice(), theStaff_->getChannel(), theStaff_->getVolume(), theStaff_->transpose_);
}

void NVoice::moveDown(int down) {
	if (!currentElement_) return;
	if (currentElement_->getType() != T_CHORD) return;
	createUndoElement(currentElement_, 1, 0);
	breakTies((NChord *) currentElement_);
	makeKeysigAndClefActual();
	((NChord *)currentElement_)->moveDown(down, stemPolicy_, &(theStaff_->actualKeysig_));
	reconnectTiesAtferMove((NChord *) currentElement_);
	if (!NResource::allowInsertEcho_) return;
	NResource::mapper_->playImmediately(&(theStaff_->actualClef_), (NChord *) currentElement_,
		theStaff_->getVoice(), theStaff_->getChannel(), theStaff_->getVolume(), theStaff_->transpose_);
}


void NVoice::moveSemiToneUp() {
	if (!currentElement_) return;
	if (currentElement_->getType() != T_CHORD) return;
	createUndoElement(currentElement_, 1, 0);
	breakTies((NChord *) currentElement_);
	makeKeysigAndClefActual();
	((NChord *)currentElement_)->moveSemiToneUp(stemPolicy_, &(theStaff_->actualClef_), &(theStaff_->actualKeysig_));
	reconnectTiesAtferMove((NChord *) currentElement_);
	if (!NResource::allowInsertEcho_) return;
	NResource::mapper_->playImmediately(&(theStaff_->actualClef_), (NChord *) currentElement_,
		theStaff_->getVoice(), theStaff_->getChannel(), theStaff_->getVolume(), theStaff_->transpose_);
}

void NVoice::moveSemiToneDown() {
	if (!currentElement_) return;
	if (currentElement_->getType() != T_CHORD) return;
	createUndoElement(currentElement_, 1, 0);
	breakTies((NChord *) currentElement_);
	makeKeysigAndClefActual();
	((NChord *)currentElement_)->moveSemiToneDown(stemPolicy_, &(theStaff_->actualClef_), &(theStaff_->actualKeysig_));
	reconnectTiesAtferMove((NChord *) currentElement_);
	if (!NResource::allowInsertEcho_) return;
	NResource::mapper_->playImmediately(&(theStaff_->actualClef_), (NChord *) currentElement_,
		theStaff_->getVoice(), theStaff_->getChannel(), theStaff_->getVolume(), theStaff_->transpose_);
}

int NVoice::makePreviousElementActual(property_type *properties) {
	*properties = 0;
	
	/* Check if there is actual element. If not, select the nearest at the last known MIDI location when something happened.
	   If it cannot select any (eg. if there aren't any), return -1 */
	if (!currentElement_)
		if ( currentElement_ = selectNearestMidiEvent(main_props_->lastMidiTime) )
			return currentElement_->getSubType();
		else
			return -1;
	
	bool was_playable;

	was_playable = (currentElement_->getType() & PLAYABLE);
	if (musElementList_.find(currentElement_) == -1) {
		NResource::abort("makePreviousElementActual: internal error");
	}

	/* Check, if the current element is already the first one. */
	if (musElementList_.getFirst() != currentElement_)
		/* And if not,, move to the next one. */
		if (musElementList_.prev() == 0) {
			return -1;
		}
	currentElement_->setActual(false);
	currentElement_->draw();
	
	currentElement_ = musElementList_.current();
	currentElement_->setActual(true);
	currentElement_->draw();
	if( currentElement_->playable() ) {
		*properties = currentElement_->playable()->properties_;
	}
	if (currentElement_->getType() == T_CHORD) {
		*properties |= currentElement_->chord()->getNoteList()->first()->properties;
	}
	return currentElement_->getSubType();
}

int NVoice::makeNextElementActual(property_type *properties) {
	*properties = 0;
	
	/* Check if there is actual element. If not, select the nearest at the last known MIDI location when something happened.
	   If it cannot select any (eg. if there aren't any), return -1 */
	if (!currentElement_)
		if ( currentElement_ = selectNearestMidiEvent(main_props_->lastMidiTime) )
			return currentElement_->getSubType();
		else
			return -1;
	
	bool was_playable;

	was_playable = (currentElement_->getType() & PLAYABLE);
	if (musElementList_.find(currentElement_) == -1) {
		NResource::abort("makeNextElementActual: internal error");
	}
	
	/* Check, if the current element is already the last one. */
	if (musElementList_.getLast() != currentElement_)
		/* And if not, move to the next one. */
		if (musElementList_.next() == 0) {
			return -1;
		}
	
	currentElement_->setActual(false);
	currentElement_->draw();
	
	currentElement_ = musElementList_.current();
	currentElement_->setActual(true);
	currentElement_->draw();
	if( currentElement_->playable() ) {
		*properties = currentElement_->playable()->properties_;
	}
	if (currentElement_->getType() == T_CHORD) {
		*properties |= currentElement_->chord()->getNoteList()->first()->properties;
	}
	return currentElement_->getSubType();
}

void NVoice::changeBodyOfActualElement() {
	if (!currentElement_) return;
	createUndoElement(currentElement_, 1, 0);
	if( currentElement_->getType() == T_CHORD )
		currentElement_->chord()->changeBody(main_props_->noteBody);
}


void NVoice::changeActualChord() {
	if (!currentElement_ || !currentElement_->playable() ) return;
	createUndoElement(currentElement_, 1, 0);
	currentElement_->playable()->changeLength(main_props_->actualLength);
	if (currentElement_->playable()->properties_ & PROP_TUPLET) {
		currentElement_->playable()->breakTuplet();
	}
}

void NVoice::changeActualStem() {
	NChord *chord;
	if (!currentElement_) return;
	if (currentElement_->getType() != T_CHORD) return;
	if (currentElement_->getSubType() > HALF_LENGTH) return;
	chord = (NChord *) currentElement_;
	if ((chord->properties_ & PROP_STEM_UP) && main_props_->actualStemDir == STEM_DIR_UP) return;
	if (!(chord->properties_ & PROP_STEM_UP) && main_props_->actualStemDir == STEM_DIR_DOWN) return;
	createUndoElement(currentElement_, 1, 0);
	chord->setStemUp(main_props_->actualStemDir == STEM_DIR_UP);
}

void NVoice::breakBeames() {
	NChord *chord;
	QList<NChord> *beamlist;
	int first_beam_idx, last_beam_idx;
	if (!currentElement_) return;
	chord = (NChord *) currentElement_;
	if (!(chord->properties_ & PROP_BEAMED)) return;
	beamlist = chord->getBeamList();
	first_beam_idx = musElementList_.find(beamlist->first());
	last_beam_idx = musElementList_.find(beamlist->last());
	if (first_beam_idx < 0 || last_beam_idx < 0) {
		NResource::abort("breakBeames: internal error");
	}
	createUndoElement(first_beam_idx, last_beam_idx - first_beam_idx + 1, 0);
	chord->breakBeames();
}

bool NVoice::lastChordContained(QList<NMusElement> *clonelist, QList<NChord> *beamlist) {
	int oldidx = clonelist->at();
	NChord *lastChord = beamlist->last();
	NMusElement *elem;

	for (elem = clonelist->first(); elem; elem = clonelist->next()) {
		if (elem->getType() != T_CHORD) continue;
		if ((NChord *) elem == lastChord) {
			if (oldidx >= 0) {
				clonelist->at(oldidx);
			}
			return true;
		}
	}
	if (oldidx >= 0) {
		clonelist->at(oldidx);
	}
	return false;
}

bool NVoice::allElemsContained(QList<NMusElement> *clonelist, QList<NPlayable> *tupletlist) {
	NMusElement *elem;

	for (elem = tupletlist->first(); elem; elem = tupletlist->next()) {
		if (clonelist->find(elem) < 0) return false;
	}
	return true;
}

bool NVoice::lastElemContained(QList<NMusElement> *clonelist, QList<NPlayable> *tupletlist) {
	int oldidx = clonelist->at();
	NMusElement *lastelem = tupletlist->last();
	NMusElement *elem;

	for (elem = clonelist->first(); elem; elem = clonelist->next()) {
		if (elem->getType() != T_CHORD && elem->getType() != T_REST) continue;
		if (elem == lastelem) {
			if (oldidx >= 0) {
				clonelist->at(oldidx);
			}
			return true;
		}
	}
	if (oldidx >= 0) {
		clonelist->at(oldidx);
	}
	return false;
}


bool NVoice::checkTuplets(QList<NMusElement> *copielist, QList<NPlayable> *tupletlist) {
	int oldidx = copielist->at();
	bool found;
	NMusElement *elem0, *elem1;

	for (elem0 = tupletlist->first(); elem0; elem0 = tupletlist->next()) {
		found = false;
		for (elem1 = copielist->first(); !found && elem1; elem1 = copielist->next()) {
			if (elem1 == elem0) found = true;
		}
		if (!found) {
			if (oldidx >= 0) copielist->at(oldidx);
			return false;
		}
	}
	if (oldidx >= 0) copielist->at(oldidx);
	return true;
}

// breakTuplet -- break the tuplet containing the current element
// in:		void
// returns:	void
	
void NVoice::breakTuplet() {
	int oldidx;
	QList<NPlayable> *tupletlist;
	int first_trip_idx, last_trip_idx;

	if (!currentElement_ || !currentElement_->playable() ) return;
	if (!(currentElement_->playable()->properties_ & PROP_TUPLET)) return;
	oldidx = musElementList_.at();
	tupletlist = currentElement_->playable()->getTupletList();
	first_trip_idx = musElementList_.find(tupletlist->first());
	last_trip_idx = musElementList_.find(tupletlist->last());
	if (first_trip_idx < 0 || last_trip_idx < 0) {
		NResource::abort("breakTuplet: internal error");
	}
	createUndoElement(first_trip_idx, last_trip_idx - first_trip_idx + 1, 0);
	currentElement_->playable()->breakTuplet();
	if (oldidx >= 0) musElementList_.at(oldidx);
}

void NVoice::changeActualOffs(int offs) {
	if (!currentElement_) return;
	if (currentElement_->getType() != T_CHORD) return;
	createUndoElement(currentElement_, 1, 0);
	breakTies((NChord *) currentElement_);
	makeKeysigAndClefActual();
	currentElement_->chord()->changeOffs(offs, &(theStaff_->actualKeysig_));
	reconnectTiesAtferMove((NChord *) currentElement_);
	NResource::mapper_->playImmediately(&(theStaff_->actualClef_), (NChord *) currentElement_,
		theStaff_->getVoice(), theStaff_->getChannel(), theStaff_->getVolume(), theStaff_->transpose_);
}

void NVoice::setDotted() {
	if (!currentElement_) return;
	createUndoElement(currentElement_, 1, 0);
	if( currentElement_->getType() & PLAYABLE )
		currentElement_->playable()->setDotted(main_props_->dotcount);
}

void NVoice::setAccent(unsigned int type){
	if (!currentElement_) return;
	if ((currentElement_->getType() != T_CHORD)
	    && (currentElement_->getType() != T_REST))
		return;
	createUndoElement(currentElement_, 1, 0);
	if (currentElement_->playable()->properties_ & PROP_STACC)
	    currentElement_->playable()->properties_ ^= PROP_STACC;
	for (int i = 19; i <= 23; i++)
	    if (currentElement_->playable()->properties_ & (1 << i))
	        currentElement_->playable()->properties_ ^= (1 << i);
	if (currentElement_->getType() == T_CHORD) {
	    switch(type){
	        case PROP_STACC: currentElement_->chord()->setProperty(PROP_STACC, main_props_->staccato); break;
	        case PROP_SFORZ: currentElement_->chord()->setProperty(PROP_SFORZ, main_props_->sforzato); break;
	        case PROP_PORTA: currentElement_->chord()->setProperty(PROP_PORTA, main_props_->portato); break;
	        case PROP_STPIZ: currentElement_->chord()->setProperty(PROP_STPIZ, main_props_->strong_pizzicato); break;
	        case PROP_SFZND: currentElement_->chord()->setProperty(PROP_SFZND, main_props_->sforzando); break;
	        case PROP_FERMT: currentElement_->chord()->setProperty(PROP_FERMT, main_props_->fermate); break;
	        default: printf("illegal accent, ID: %i\n", type); fflush(stdout); break;
	    }
	} else if ((currentElement_->getType() == T_REST) && (currentElement_->getSubType() != MULTIREST)) {
	    // on normal rest, fermata is allowed
	    switch(type){
	        case PROP_FERMT: currentElement_->rest()->setProperty(PROP_FERMT, main_props_->fermate); break;
	        default: /* ignore other accents */ break;
	    }
	}
}

void NVoice::pubAddUndoElement() {
    createUndoElement(currentElement_, 1, 0);
}

/* Select the musElement nearest to the given MIDI time. */
/* If the midiTime doesn't match any of the elements' MIDI times, it selects the nearest left one (!nearestRight - default) or the right one (nearestRight) */
NMusElement *NVoice::selectNearestMidiEvent(int midiTime, bool nearestRight) {
	if (!musElementList_.count()) return 0;
	
	uint leftElt = 0; /* Index of the current left elt */
	uint rightElt = musElementList_.count() - 1; /* Index of the current right elt */
	uint midElt = (leftElt + rightElt) / 2; /* Index of the current mid elt - always in the half of left&right */
	
	/* if actual element exists, deselect it */
	if (currentElement_)
		currentElement_->setActual(false);
	
	/* Fast bisection algorythm follows */
	while ( 
	       ( /* Elements' MIDI times match exactly with the wanted time. */
	        (musElementList_.at(leftElt)->midiTime_ != midiTime) &&
	        (musElementList_.at(rightElt)->midiTime_ != midiTime) &&
	        (musElementList_.at(midElt)->midiTime_ != midiTime)
	       ) && /* Or the algorythm finishes with approximate match.
	               Only two elements are left in this case - the left one and the mid one always merge. */
	       (musElementList_.at(leftElt) != musElementList_.at(midElt))
	      )
		if (musElementList_.at(midElt)->midiTime_ < midiTime) {
			leftElt = midElt;
			midElt = (leftElt + rightElt) / 2;
		} else {
			rightElt = midElt;
			midElt = (leftElt + rightElt) / 2;
		}
	
	/* MIDI times match exactly: correct current() item already gets selected above, when calling at() function */
	if (musElementList_.current()->midiTime_ == midiTime)
		currentElement_ = musElementList_.current();
	/* MIDI times approximately match:
	   - if the wanted MIDI time is between the right&left elments ones', select the one according to the nearestRight variable */
	else if ( (midiTime < musElementList_.at(rightElt)->midiTime_) && (midiTime > musElementList_.at(leftElt)->midiTime_) )
		if (!nearestRight) currentElement_ = musElementList_.at(leftElt);
		else currentElement_ = musElementList_.at(rightElt);
	/* - if the wanted MIDI time is out of range, select one of the outer elements - the one nearest to the wanted time */
	else if (midiTime > musElementList_.at(rightElt)->midiTime_)
		currentElement_ = musElementList_.at(rightElt);
	else
		currentElement_ = musElementList_.at(leftElt);
	
	currentElement_->setActual(true);
	
	return currentElement_;
}

void NVoice::setActualTied() {
	NNote *note;
	NChord *chord;
	if (!currentElement_) return;
	if (currentElement_->getType() != T_CHORD) return;
	chord = (NChord *) currentElement_;
	note = chord->getActualNote(); 
	if (main_props_->tied) {
		if (note->properties & PROP_TIED) return;
	}
	else {
		if (!(note->properties & PROP_TIED)) return;
	}
	createUndoElement(currentElement_, 1, 0);
	if (main_props_->tied) {
		reconnectTies(note);
		findTieMember(note);
		chord->setActualTied(main_props_->tied);
	}
	else {
		reconnectDeletedTies(note);
		chord->setActualTied(main_props_->tied);
	}
}

/* toggle hidden rest */
void NVoice::setHidden() {
	if (!currentElement_) return;
	if (currentElement_->getType() != T_REST) return;
	createUndoElement(currentElement_, 1, 0);
	currentElement_->rest()->properties_ ^= PROP_HIDDEN;
}

void NVoice::setArpeggio() {
	NChord *chord;
	if (!currentElement_) return;
	if (currentElement_->getType() != T_CHORD) return;
	chord = (NChord *) currentElement_;
	if (main_props_->arpeggio) {
		if (chord->properties_ & PROP_ARPEGG) return;
	}
	else {
		if (!(chord->properties_ & PROP_ARPEGG)) return;
	}
	createUndoElement(currentElement_, 1, 0);
	chord->setArpeggio(main_props_->arpeggio);
}

void NVoice::setPedalOn() {
	NChord *chord;
	if (!currentElement_) return;
	if (currentElement_->getType() != T_CHORD) return;
	chord = (NChord *) currentElement_;
	if (main_props_->pedal_on) {
		if (chord->properties_ & PROP_PEDAL_ON) return;
	}
	else {
		if (!(chord->properties_ & PROP_PEDAL_ON)) return;
	}
	createUndoElement(currentElement_, 1, 0);
	chord->setPedalOn(main_props_->pedal_on);
}

void NVoice::setPedalOff() {
	NChord *chord;
	if (!currentElement_) return;
	if (currentElement_->getType() != T_CHORD) return;
	chord = (NChord *) currentElement_;
	if (main_props_->pedal_off) {
		if (chord->properties_ & PROP_PEDAL_OFF) return;
	}
	else {
		if (!(chord->properties_ & PROP_PEDAL_OFF)) return;
	}
	createUndoElement(currentElement_, 1, 0);
	chord->setPedalOff(main_props_->pedal_off);
}

void NVoice::setBeamed() {
	int count, x0, x1, idx;
	bool found, beamable = true;
	NMusElement *acc_elem;
	QList<NChord> *chordlist;
	NChord *chord;

	if (!startElement_ || !endElement_) return;
	x0 = (endElementIdx_ > startElemIdx_) ? startElemIdx_ : endElementIdx_;
	x1 = (endElementIdx_ > startElemIdx_) ? endElementIdx_ : startElemIdx_;
	idx = x0;
	found = false;
	acc_elem = musElementList_.at(x0);
	chordlist = new QList<NChord>();
	while (!found && acc_elem != 0 && idx <= x1) {
		if (acc_elem->getType() == T_CHORD) {
			if (acc_elem->getSubType() < QUARTER_LENGTH && !(acc_elem->chord()->properties_ & PROP_BEAMED)) {
				chord = (NChord *) acc_elem;
				chordlist->append(chord);
				acc_elem = musElementList_.next();
				idx = musElementList_.at();
				count = 1;
				found = true;
			}
			else {
				acc_elem = musElementList_.next();
				idx = musElementList_.at();
			}
		}
		else {
			acc_elem = musElementList_.next();
			idx = musElementList_.at();
		}
	}
	if (!found) {
		return;
	}
	x0 = idx;
	found = false;
	while (!found && beamable && acc_elem != 0 && idx <= x1) {
		if (acc_elem->getType() == T_CHORD) {
			if (acc_elem->getSubType() > NOTE8_LENGTH) {
				beamable = false;
			}
			else {
				chord = (NChord *) acc_elem;
				chordlist->append(chord);
				acc_elem = musElementList_.next();
				idx = musElementList_.at();
				count++;
			}
		}
		else {
			beamable = false;
		}
	}
	beamable = beamable && count > 1;
	if (!beamable) {
		return;
	}
	
	x0 = musElementList_.find(chordlist->first());
	x1 = musElementList_.find(chordlist->last());
	if (x0 < 0 || x1 < 0) {
		NResource::abort("setBeamed: internal error");
	}
	createUndoElement(x0, x1 - x0 + 1, 0);
	NChord::computeBeames(chordlist, stemPolicy_);
}

// buildTupletList -- build list of tupletable elements between x0 and x1
// (both included) assuming a tuplet size of numNotes into elemlist
// in:		x0: left index
//		x1: right index
//		numNotes: tuplet size
// inout:	elemlist: list to add elements to
// returns:	true iff successful (i.e. list created and tupletable)
// note:	always clears elemlist at start
// note:	LVIFIX tbd: positions musElementList_ at ???

bool NVoice::buildTupletList(int x0, int x1, char numNotes, QList<NPlayable> *elemlist) {
	int count = 0;
	int idx = 0;
	bool found = false;
	bool tupletable = true;
	NMusElement *acc_elem = 0;
	int sum = 0;
	// always clear elemlist at start
	elemlist->clear();
	// check preconditions
	if ((x0 < 0) || (x1 < 0)) return false;
	if (x0 > x1) {
		// swap x0 and x1
		int tmp = x0;
		x0 = x1;
		x1 = tmp;
	}
	// find first playable element in selection
	// init count, elemlist and sum
	found = false;
	acc_elem = musElementList_.at(x0);
	idx = x0;
	while (!found && acc_elem != 0 && idx < x1) {
		if (acc_elem->playable()) {
			sum = acc_elem->getSubType() / MULTIPLICATOR;
			elemlist->append( acc_elem->playable() );
			acc_elem = musElementList_.next();
			count = 1;
			found = true;
		}
		else {
			acc_elem = musElementList_.next();
			idx = musElementList_.at();
		}
	}
	if (!found) {
		return false;
	}
	// loop over remaining playable elements in selection
	// update count, elemlist and sum
	found = false;
	while (!found && tupletable && acc_elem != 0 && idx <= x1) {
		if (acc_elem->playable()) {
			sum += acc_elem->getSubType() / MULTIPLICATOR;
			elemlist->append( acc_elem->playable() );
			acc_elem = musElementList_.next();
			idx = musElementList_.at();
			count++;
		}
		else {
			tupletable = false;
		}
	}
	return tupletable && count > 1 && (sum % numNotes == 0);
}

// setTuplet -- convert the selection into a tuplet of numNotes notes
// in playing time playtime, i.e. setTuplet(3, 2) creates a triplet
// in:		numNotes: tuplet size in notes
//		playtime: playing time for these notes
// returns:	void
// note:	LVIFIX tbd: positions musElementList_ at ???

void NVoice::setTuplet(char numNotes, char playtime) {
	int x0 = 0;
	int x1 = 0;
	QList<NPlayable> *elemlist = 0;

	if (!startElement_ || !endElement_) return;
	x0 = (endElementIdx_ > startElemIdx_) ? startElemIdx_ : endElementIdx_;
	x1 = (endElementIdx_ > startElemIdx_) ? endElementIdx_ : startElemIdx_;

	elemlist = new QList<NPlayable>();
	if (!buildTupletList(x0, x1, numNotes, elemlist)) {
		delete elemlist;
		return;
	}
	x0 = musElementList_.find(elemlist->first());
	x1 = musElementList_.find(elemlist->last());
	if (x0 < 0 || x1 < 0) {
		NResource::abort("setTuplet: internal error");
	}
	createUndoElement(x0, x1 - x0 + 1, 0);
	NPlayable::computeTuplet(elemlist, numNotes, playtime);
	// note: don't delete elemlist here, all tuplet notes refer to it
}

int NVoice::deleteActualElem(property_type *properties, bool backspace) {
	NNote *note;
	QList<NNote> *partlist;
	NChord *chord;
	bool removedLast = false; /* are we deleting the last element? */
	bool removedFirst = false; /* are we deleting the first element? */
	*properties = 0;
	if (!currentElement_) return -1;
	if (musElementList_.isEmpty()) {
		return -1;
	}
	/* Store the last usable MIDI event */
	main_props_->lastMidiTime = currentElement_->midiTime_;
	
	if (currentElement_->getType() == T_CHORD) {
		chord = (NChord *) currentElement_;
		createUndoElement(currentElement_, 1, -1);
		if (chord->properties_ & PROP_TUPLET) {
			chord->breakTuplet();
		}
		if (chord->lastBeamed() || chord->beamHasOnlyTwoChords()) {
			chord->breakBeames();
		}
		else if (chord->properties_ & PROP_BEAMED) {
			chord->removeFromBeam();
		}
		if (musElementList_.find(currentElement_) == -1) {
			NResource::abort("deleteActualElem: internal error", 1);
		}
		removedLast = (musElementList_.current() == musElementList_.getLast());
		removedFirst = (musElementList_.current() == musElementList_.getFirst());
		musElementList_.remove();
		partlist = chord->getNoteList();
		for (note = partlist->first(); note; note = partlist->next()) {
			reconnectDeletedTies(note);
		}
		chord->checkSlures();
	}
	else {
		createUndoElement(currentElement_, 1, -1);
		if (currentElement_->playable() && (currentElement_->playable()->properties_ & PROP_TUPLET) ) {
			currentElement_->playable()->breakTuplet();
		}
		if (musElementList_.find(currentElement_) == -1) {
			NResource::abort("deleteActualElem: internal error", 2);
		}
		removedLast = (musElementList_.current() == musElementList_.getLast());
		removedFirst = (musElementList_.current() == musElementList_.getFirst());
		musElementList_.remove();
	}
	currentElement_ = musElementList_.current();
	if (backspace && currentElement_) {
		if (musElementList_.find(currentElement_) == -1) {
			NResource::abort("deleteActualElem: internal error", 3);
		}
		if (musElementList_.at() != (int) (musElementList_.count() -1)) {
			musElementList_.prev();
		}
	}
	if (!musElementList_.current()) musElementList_.first(); 
	if (currentElement_ = musElementList_.current()) {
		*properties = currentElement_->playable() ? currentElement_->playable()->properties_ : 0;
		if (currentElement_->getType() == T_CHORD) {
			partlist = currentElement_->chord()->getNoteList();
			*properties |= partlist->first()->properties;
		}
		
		 /* if the last element was deleted by Key_Delete or the first element was deleted by Key_Backspace, none get selected */
		if ( (backspace && (!removedFirst)) || (!backspace && (!removedLast)) ) {
			currentElement_->setActual(true);
			return currentElement_->getSubType();
		} else  {
			currentElement_ = 0;
			return 0; /* 0 is returned as we didn't encounter an error, but actually isn't any element selected */
		}
	}
	return -1;
}

bool NVoice::deleteActualNote() {
	NNote *note;
	NChord *chord;
	if (!currentElement_);
	if (currentElement_->getType() != T_CHORD) {
		NResource::abort("deleteAtLine: internal error(1)");
	}
	chord = (NChord *) currentElement_;
	note = chord->getActualNote();
	createUndoElement(currentElement_, 1, 0);
	if (chord->removeNote(note, stemPolicy_)) {
		reconnectDeletedTies(note);
		return true;
	}
	deleteLastUndo();
	return false;
}



bool NVoice::deleteAtPosition(int y) {
	int line;
	bool ok = false;
	NNote *note = 0;

	if (!currentElement_) return false;

	createUndoElement(currentElement_, 1, 0);
	if (y < theStaff_->staff_props_.base) {
		line = 8 - 2 * (y-2 - theStaff_->staff_props_.base) / LINE_DIST;
	}
	else {
		line = 8 - 2 * (y+1 - theStaff_->staff_props_.base) / LINE_DIST;
	}
	if( currentElement_->getType() == T_CHORD )
		note = currentElement_->chord()->searchLine(line, 2);
	if (note) {
		ok = currentElement_->chord()->deleteNoteAtLine(line, stemPolicy_);
		reconnectDeletedTies(note);
	}
	else {
		deleteLastUndo();
	}
	return ok;
}

/*------------------------- insertion due to user interaction --------------------*/


void NVoice::insertTmpElemAtPosition(int xpos, NMusElement *tmpElem) {
	NMusElement *elem;
	bool found;
	int idx;

	tmpElem->setActual(true);
	tmpElem->setStaffProps(&(theStaff_->staff_props_));
	if (musElementList_.isEmpty()) {
		musElementList_.append(tmpElem);
		createUndoElement(musElementList_.at(), 0, 1);
		currentElement_ = musElementList_.first();
		return;
	}
	if (currentElement_) {
		currentElement_->setActual(false);
	}
	found = false;
	elem = musElementList_.first();
	NClef *belongingClef = &(theStaff_->actualClef_);
	while (!found  && elem != 0) {
		if (xpos > elem->getBbox()->x()) {
			if( elem->getType() == T_CLEF )
				belongingClef = (NClef *) elem;
			elem = musElementList_.next();
		}
		else {
			found = true;
			idx = musElementList_.at();
		}
	}
	switch (tmpElem->getType()) {
		case T_KEYSIG: ((NKeySig *) tmpElem)->setClef( belongingClef );
				break;
	}
	if (!found) {
		musElementList_.append(tmpElem);
	}
	else {
		musElementList_.insert(idx, tmpElem);
	}
	currentElement_ = tmpElem;
	createUndoElement(currentElement_, 0, 1);
}

#ifdef XXX
int NVoice::quant(int l, int *dotcount) {
	unsigned int testlength;
	unsigned int deltamin3 = (1<<30), deltamin9 = (1<<30);
	int i, j;
	*dotcount = 0;

	if (l > DOUBLE_WHOLE_LENGTH) return DOUBLE_WHOLE_LENGTH;

	testlength = (0x3 << 8);
	for (i = 8; i > 0; i--) {
		if (testlength > (unsigned int) l) {
			testlength >>= 1;
		}
		else {
			deltamin3 = l - testlength;
			break;
		}
	}

	testlength = (0x9 << 7);
	for (j = 8; j > 0; j--) {
		if (testlength > (unsigned int) l) {
			testlength >>= 1;
		}
		else {
			deltamin9 = l - testlength;
			break;
		}
	}

	if (deltamin9 < deltamin3) {
		*dotcount = 1;
		return (0x3 << j);
	}
	return (0x3 << i);
}
#else
int NVoice::quant(int l, int *dotcount, int maxlength) {
	unsigned int testlength;
	unsigned int deltamin3 = (1<<30), deltamin9 = (1<<30);
	int i, j,  shifts;
	int ret;
	*dotcount = 0;

	if (l > maxlength) return maxlength;

	maxlength /= MULTIPLICATOR / 3;
	l /= MULTIPLICATOR / 3;
	for (shifts = 0; shifts < 9 && (0x3 << shifts) <  maxlength; shifts++);

	testlength = (0x3 << shifts);
	for (i = shifts; i > 0; i--) {
		if (testlength > (unsigned int) l) {
			testlength >>= 1;
		}
		else {
			deltamin3 = l - testlength;
			break;
		}
	}

	testlength = (0x9 << (shifts - 1));
	for (j = shifts; j > 0; j--) {
		if (testlength > (unsigned int) l) {
			testlength >>= 1;
		}
		else {
			deltamin9 = l - testlength;
			break;
		}
	}

	if (deltamin9 < deltamin3) {
		*dotcount = 1;
		ret = (MULTIPLICATOR << j);
		return ret;
	}
	ret = (MULTIPLICATOR << i);
	return ret;
}
#endif

// collectAndInsertPlayable -- remove all elements in patterns from this voice
// and replace them by the minimum number of equivalent elements of total length
// targetlength

void NVoice::collectAndInsertPlayable(	int startTime,
					QList<NMusElement> *patterns,
					int targetLength,
					bool useExistingElement,
					bool beforeBarSig) {	// beforeBarSig: first short note, then the long one
	int len, restlen;
	bool isChord;
	int dotcount;
	int akpos;
	NMusElement *lastPattern;
	NChord *elem2;
	QList<NNote> *noteList;
	NNote *note;

	if (patterns->isEmpty()) {
		NResource::abort("internal error: collectAndInsertPlayable: isEmpty()");
	}
	lastPattern = patterns->last();
	isChord = lastPattern->getType() == T_CHORD;
	while (patterns->count() > 1) {
		if ((akpos = musElementList_.find(patterns->first())) == -1) {
			NResource::abort("internal error: collectAndInsertPlayable: find == -1 (1)");
		}
		musElementList_.remove();
		patterns->remove();
	}
	if ((akpos = musElementList_.find(lastPattern)) == -1) {
		NResource::abort("internal error: collectAndInsertPlayable: find == -1 (2)");
	}
	patterns->remove();
	if (!patterns->isEmpty()) {
		NResource::abort("internal error: collectAndInsertPlayable: patterns != empty");
	}

	if (targetLength < MULTIPLICATOR && useExistingElement) {
		musElementList_.remove();
		musElementList_.at(akpos-1);
		return;
	}

	while (targetLength >= MULTIPLICATOR) {
	        len = quant(targetLength, &dotcount, isChord ? DOUBLE_WHOLE_LENGTH : WHOLE_LENGTH);
		restlen  = targetLength - (dotcount ? 3 * len / 2 : len);
		if (restlen >= MULTIPLICATOR || !useExistingElement) {
	     		elem2 = ((NChord *) lastPattern)->clone();
		}
		else {
			elem2 = (NChord *) lastPattern;
		}
		if (restlen && beforeBarSig) {	// before bar end first the shorter note value
	        	elem2->changeLength(restlen);
	        	elem2->setDotted(0);
		} else {
	        	elem2->changeLength(len);
	        	elem2->setDotted(dotcount);
		}

		elem2->computeMidiLength();
		elem2->midiTime_ = startTime;
		startTime += elem2->getMidiLength();
		if (restlen >= MULTIPLICATOR || !useExistingElement) {
			if (isChord) {
				noteList = elem2->getNoteList();
				for (note = noteList->first(); note; note = noteList->next()) {
					note->properties |= PROP_TIED;
				}
			}
			if (akpos == (int) musElementList_.count()) {
				musElementList_.append(elem2);
			}
			else {
				musElementList_.insert(akpos, elem2);
			}
			if (isChord) {
				for (note = noteList->first(); note; note = noteList->next()) {
					reconnectTies(note);
				}
				for (note = noteList->first(); note; note = noteList->next()) {
					findTieMember(note);
				}
			}
		}
		akpos++;
		targetLength -= elem2->getMidiLength();
	}
	if (musElementList_.find(lastPattern) == -1) {
		NResource::abort("internal error: collectAndInsertPlayable: find == -1 (3)");
	}
	
}

void NVoice::autoBar() {
	int akpos, idxOfFirstBar, barpos;
	int ticks, maxticks;
	bool foundfirstBar = false;
	bool barInserted = false;
	bool go_on;
	int lrest, len1, len2;
	NTimeSig *timesig;
	NRest *rest;
	NMusElement *elem, *nextElem;
	QList <NMusElement> elems;
	NNote *note;
	QList <NNote> *part;

	createUndoElement(0, musElementList_.count(), 0);

	ticks = 0;
	go_on = true;
	for (elem = musElementList_.last(); elem && go_on; elem = musElementList_.prev()) {
		switch (elem->getType()) {
			case T_REST: ticks += elem->getSubType(); break;
			case T_CHORD: go_on = false; break;
			default: break;
		}
	}
	if (ticks < 2*MULTIPLICATOR) {
		musElementList_.append(rest = new NRest(main_props_, &(theStaff_->staff_props_), &yRestOffs_, WHOLE_LENGTH, 0));
		rest->midiTime_ = midiEndTime_ + WHOLE_LENGTH;
		musElementList_.append(rest = new NRest(main_props_, &(theStaff_->staff_props_), &yRestOffs_, WHOLE_LENGTH, 0));
		rest->midiTime_ = midiEndTime_ + 2*WHOLE_LENGTH;
	}

	elem = musElementList_.first();
	while (elem) {
		if (elem->getType() == T_SIGN && elem->getSubType() == SIMPLE_BAR) {
			if (foundfirstBar) {
				elem = musElementList_.next();
				if (!elem) break;
				if (elem->getType() != T_SIGN || ((elem->getSubType() | ( SPECIAL_ENDING1 | SPECIAL_ENDING2 )) == 0)) {
					musElementList_.prev();
					musElementList_.remove();
					elem = musElementList_.current();
				}
			}
			else {
				idxOfFirstBar = musElementList_.at();
				foundfirstBar = true;
				elem = musElementList_.next();
			}
		}
		else {
			elem = musElementList_.next();
		}
	}

	ticks = 0;
	timesig = getFirstTimeSig();
	if (!timesig) {
		maxticks = WHOLE_LENGTH;
	}
	else {
		maxticks = MULTIPLICATOR*timesig->numOf128th();
	}
	if (foundfirstBar) {
		elem = musElementList_.at(idxOfFirstBar + 1);
		if (!elem) {
			elem = musElementList_.first();
		}
	}
	else {
		elem = musElementList_.first();
	}
	for (; elem; elem = musElementList_.next()) {
		akpos = musElementList_.at();
		switch (elem->getType()) {
			case T_TIMESIG: maxticks = MULTIPLICATOR*((NTimeSig *) elem)->numOf128th();
					continue;
			case T_SIGN:
				 if (elem->getSubType() & 
				((SIMPLE_BAR | REPEAT_OPEN | REPEAT_CLOSE | REPEAT_OPEN_CLOSE | SPECIAL_ENDING1 | SPECIAL_ENDING2))){
					ticks = 0;
					continue;
				 }
		}
		if (elem->getType() & PLAYABLE) {
			ticks += elem->getMidiLength();
		}
		if (ticks == maxticks) {
			ticks = 0;
			nextElem = musElementList_.next();
			 if (nextElem && nextElem->getType() == T_SIGN && (nextElem->getSubType() & 
				(SIMPLE_BAR | REPEAT_OPEN | REPEAT_CLOSE | REPEAT_OPEN_CLOSE | SPECIAL_ENDING1 | SPECIAL_ENDING2))){
				continue;
			 }
			 musElementList_.insert(akpos+1, new NSign(main_props_, &(theStaff_->staff_props_), SIMPLE_BAR));
		}
		else if (ticks > maxticks) {
		   	akpos = musElementList_.at();
			if (elem->playable() && (elem->playable()->properties_ & PROP_TUPLET) ) {
				len2 = ticks - maxticks;
				ticks = elem->getMidiLength() - len2;
				akpos = musElementList_.at();
				musElementList_.insert(akpos, new NSign(main_props_, &(theStaff_->staff_props_), SIMPLE_BAR));

			}
			else {
				switch (elem->getType()) {
					case T_CHORD: 
					case T_REST: len2 = ticks - maxticks;
					     	len1 = elem->getMidiLength() - len2;
					     	elems.append(elem);
					     	collectAndInsertPlayable(elem->midiTime_, &elems, len1, false);
					     	akpos = musElementList_.at();
					     	musElementList_.insert(akpos, new NSign(main_props_, &(theStaff_->staff_props_), SIMPLE_BAR));
					     	barpos = akpos;
					     	elems.append(elem);
					     	collectAndInsertPlayable(elem->midiTime_ + len1, &elems, len2, true);
					     	barInserted = true;
					     	break;
				}
				ticks = 0;	
				elem = musElementList_.at(barpos);
			}
		}
	}
	if (!barInserted) {
		return;
	}
	elem = musElementList_.last();
	while (elem && !(elem->getType() == T_SIGN && (elem->getSubType() & BAR_SYMS))) {
		musElementList_.remove();
		elem = musElementList_.last();
	}
#ifdef XXX
	elem = musElementList_.first();
	lrest = 0;
	while (elem) {
		if (elem->getType() == T_REST) {
			elems.append(elem);
			lrest += elem->getMidiLength();
		}
		else if (elem->getType() == T_CHORD  || 
		    elem->getType() == T_SIGN && (elem->getSubType() & BAR_SYMS)) {
			if (lrest >= MULTIPLICATOR) {
				collectAndInsertPlayable(elem->midiTime_, &elems, lrest, true);
			}
			lrest = 0;
		}
		else {
		}
		elem = musElementList_.next();
	}
#endif
	go_on = true;
	idxOfFirstBar = -1;
	for (elem = musElementList_.last(); elem && (elem->getType() != T_REST || elem->midiTime_ > midiEndTime_) && go_on; elem = musElementList_.prev()) {
		switch(elem->getType()) {
			case T_SIGN: if ((elem->getSubType() & BAR_SYMS) == 0) break;
				     idxOfFirstBar = musElementList_.at();
				     break;
			case T_REST: break;
			default: go_on = false;
		}
	}
	if (idxOfFirstBar != -1) {
		elem = musElementList_.last();
		while (elem && musElementList_.at() > idxOfFirstBar) {
			delete elem;
			musElementList_.remove();
			elem = musElementList_.last();
		}
	}

	setCountOfAddedItems(musElementList_.count());
}

void NVoice::autoBarVoice123andSoOn() {
	NMusElement *elem, *specialElement;
	int specialElemTime;
        int barpos, akpos, len1, len2;
	QList <NMusElement> elems;

	createUndoElement(0, musElementList_.count(), 0);
	computeMidiTime(false, false);
	theStaff_->resetSpecialElement();
	for (elem = musElementList_.first(); elem; elem = musElementList_.next()) {
		while ((specialElement = theStaff_->findBarInStaff(elem->midiTime_, elem->midiTime_ + elem->getMidiLength()))) {
			if (elem->playable() && ( elem->playable()->properties_ & PROP_TUPLET) ) {
				elem = musElementList_.next();
				continue;
			}
			specialElemTime = specialElement->midiTime_;
			akpos = musElementList_.at();
			switch (elem->getType()) {
			case T_CHORD: 
			case T_REST: 
			     len2 = elem->midiTime_ + elem->getMidiLength() - specialElemTime;
			     len1 = elem->getMidiLength() - len2;
			     elems.append(elem);
			     collectAndInsertPlayable(elem->midiTime_, &elems, len1, false);
			     akpos = musElementList_.at();
			     barpos = akpos;
			     elems.append(elem);
			     collectAndInsertPlayable(elem->midiTime_ + len1, &elems, len2, true);
		             elem = musElementList_.at(akpos);
			     break;
			}
		}
	}
	setCountOfAddedItems(musElementList_.count());
}

bool NVoice::beameEndRequired(QList<NChord> *beamlist_so_far, NTimeSig *timesig, int beats) {
	int shortestNote = DOUBLE_WHOLE_LENGTH;
	NChord *chord;
	struct rule_str *wild_ptr = NULL, *rule_ptr = NULL, *ptr;
	int best_match = -1;
	int num, denom;

	for (chord = beamlist_so_far->first(); chord; chord = beamlist_so_far->next()) {
		if (chord->getMidiLength(true) < shortestNote) shortestNote = chord->getMidiLength(true);
	}
	num = timesig->getNumerator(); denom = timesig->getDenominator();
	for (ptr = beam_rules_tab__; ptr->function != END_OF_TABLE; ptr++) {
		if (ptr->function!= FUNC_END) continue;
		if (num == ptr->time_num && denom == ptr->time_denom) {
			if (ptr->notelen < 0) {
				wild_ptr = ptr;
			}
			else if (ptr->notelen >= shortestNote) {
				if (best_match < 0) {
					best_match = ptr->notelen;
					rule_ptr = ptr;
				}
				else if (best_match > ptr->notelen) {
					best_match = ptr->notelen;
					rule_ptr = ptr;
				}
			}
		}
	}
	if ((best_match != shortestNote || rule_ptr == NULL) && wild_ptr != NULL) {
		rule_ptr = wild_ptr;
	}
	if (rule_ptr == NULL) return false;
	return (!(beats % rule_ptr->duration));
}
				

void NVoice::autoBeam() {
	NMusElement *elem;
	QList<NChord> *beamlist;
	property_type properties = 0;
	int beats = 0;
	NMusElement *specElem;
	NTimeSig current_timesig(0, 0);

	beamlist = new QList<NChord>();
	createUndoElement(0, musElementList_.count(), 0);
	if (!firstVoice_) {
		theStaff_->resetSpecialElement();
	}

	for (elem = musElementList_.first(); elem; elem = musElementList_.next()) {
		if (!firstVoice_) {
			while (specElem =  theStaff_->checkSpecialElement(elem->getXpos())) {
				if (beamlist->count() > 1) {
					NChord::computeBeames(beamlist, stemPolicy_);
					beamlist = new QList<NChord>();
				}
				else {
					beamlist->clear();
				}
				if (specElem->getType() == T_TIMESIG) {	
					current_timesig.setSignature((NTimeSig *) specElem);
				}
				else if (specElem->getType() & BAR_SYMS) {
				 	beats = 0;
				}
			}
		}
		switch (elem->getType()) {
			case T_CHORD:
				if (elem->getSubType() <= NOTE8_LENGTH) {
					if (beamlist->count() > 0 && (beameEndRequired(beamlist, &current_timesig, beats) || ((elem->chord()->properties_ & PROP_GRACE) != properties))) {
						if (beamlist->count() > 1) {
							NChord::computeBeames(beamlist, stemPolicy_);
						}
						else {
							beamlist->clear();
						}
						beamlist = new QList<NChord>();
						properties = elem->chord()->properties_ & PROP_GRACE;
					}
					beats += elem->getMidiLength(true);
					beamlist->append((NChord *)elem);
				}
				else {
					if (beamlist->count() > 1) {
						NChord::computeBeames(beamlist, stemPolicy_);
						beamlist = new QList<NChord>();
					}
					else {
						beamlist->clear();
					}
				}
				break;
			case T_SIGN:
				if (elem->getSubType() & BAR_SYMS) {
					if (beamlist->count() > 1) {
						NChord::computeBeames(beamlist, stemPolicy_);
						beamlist = new QList<NChord>();
					}
					else {
						beamlist->clear();
					}
					beats = 0;
				}
				break;
			case T_REST:
				beats += elem->getMidiLength(true);
				if (beamlist->count() > 1) {
					NChord::computeBeames(beamlist, stemPolicy_);
					beamlist = new QList<NChord>();
				}
				else {
					beamlist->clear();
				}
				break;
			case T_TIMESIG: 
					current_timesig.setSignature((NTimeSig *) elem);
					break;
		}
	}
	if (beamlist->count() > 1) {
		NChord::computeBeames(beamlist, stemPolicy_);
		beamlist = new QList<NChord>();
	}
	else {
		beamlist->clear();
		delete beamlist;
	}
}

void NVoice::checkBeams(int indexOfLastBar, NTimeSig *tsig) {
	NMusElement *elem, *specElem;
	QList<NChord> *beamlist;
	property_type properties = 0;
	int beats = 0;
	int x0, x1;
	int oldidx;

	if (!NResource::autoBeamInsertion_) return;

	NTimeSig current_timesig(0, 0);
	if (tsig) {
		current_timesig.setSignature(tsig->getNumerator(), tsig->getDenominator());
	}

	beamlist = new QList<NChord>();
	elem = musElementList_.at(indexOfLastBar);
	if (!firstVoice_) {
		theStaff_->resetSpecialElement();
	}

	for (; elem; elem = musElementList_.next()) {
		if (!firstVoice_) {
			while (specElem = theStaff_->checkSpecialElement(elem->getXpos())) {
				if (beamlist->count() > 1) {
					oldidx = musElementList_.at();
					x0 = musElementList_.find(beamlist->first());
					x1 = musElementList_.find(beamlist->last());
					if (x0 < 0 || x1 < 0) {
						NResource::abort("checkBeams: internal error", 1);
					}
					createUndoElement(x0, x1 - x0 + 1, 0);
					if (oldidx >= 0) musElementList_.at(oldidx);
					NChord::computeBeames(beamlist, stemPolicy_);
					beamlist = new QList<NChord>();
				}
				else {
					beamlist->clear();
				}
				if (specElem->getType() == T_TIMESIG) {	
					current_timesig.setSignature((NTimeSig *) specElem);
				}
				else if (specElem->getType() & BAR_SYMS) {
				 	beats = 0;
				}
			}
		}
		switch (elem->getType()) {
			case T_CHORD:
				if (elem->getSubType() <= NOTE8_LENGTH) {
					if (beamlist->count() > 0 && (beameEndRequired(beamlist, &current_timesig, beats) || ((elem->chord()->properties_ & PROP_GRACE) != properties))) {
						if (beamlist->count() > 1) {
							oldidx = musElementList_.at();
							x0 = musElementList_.find(beamlist->first());
							x1 = musElementList_.find(beamlist->last());
							if (x0 < 0 || x1 < 0) {
								NResource::abort("checkBeams: internal error", 1);
							}
							createUndoElement(x0, x1 - x0 + 1, 0);
							if (oldidx >= 0) musElementList_.at(oldidx);
							NChord::computeBeames(beamlist, stemPolicy_);
						}
						else {
							beamlist->clear();
						}
						beamlist = new QList<NChord>();
						properties = elem->chord()->properties_ & PROP_GRACE;
					}
					beats += elem->getMidiLength(true);
					beamlist->append((NChord *)elem);
				}
				else {
					if (beamlist->count() > 1) {
						oldidx = musElementList_.at();
						x0 = musElementList_.find(beamlist->first());
						x1 = musElementList_.find(beamlist->last());
						if (x0 < 0 || x1 < 0) {
							NResource::abort("checkBeams: internal error", 2);
						}
						createUndoElement(x0, x1 - x0 + 1, 0);
						if (oldidx >= 0) musElementList_.at(oldidx);
						NChord::computeBeames(beamlist, stemPolicy_);
						beamlist = new QList<NChord>();
					}
					else {
						beamlist->clear();
					}
				}
				break;
			case T_SIGN:
				if (elem->getSubType() & BAR_SYMS) {
					if (beamlist->count() > 1) {
						oldidx = musElementList_.at();
						x0 = musElementList_.find(beamlist->first());
						x1 = musElementList_.find(beamlist->last());
						if (x0 < 0 || x1 < 0) {
							NResource::abort("checkBeams: internal error", 3);
						}
						createUndoElement(x0, x1 - x0 + 1, 0);
						if (oldidx >= 0) musElementList_.at(oldidx);
						NChord::computeBeames(beamlist, stemPolicy_);
						beamlist = new QList<NChord>();
					}
					else {
						beamlist->clear();
					}
					beats = 0;
				}
				break;
			case T_REST:
				beats += elem->getMidiLength(true);
				if (beamlist->count() > 1) {
					oldidx = musElementList_.at();
					x0 = musElementList_.find(beamlist->first());
					x1 = musElementList_.find(beamlist->last());
					if (x0 < 0 || x1 < 0) {
						NResource::abort("checkBeams: internal error", 4);
					}
					createUndoElement(x0, x1 - x0 + 1, 0);
					if (oldidx >= 0) musElementList_.at(oldidx);
					NChord::computeBeames(beamlist, stemPolicy_);
					beamlist = new QList<NChord>();
				}
				else {
					beamlist->clear();
				}
				break;
			case T_TIMESIG: 
					current_timesig.setSignature((NTimeSig *) elem);
					break;
		}
	}
	if (beamlist->count() > 1) {
		oldidx = musElementList_.at();
		x0 = musElementList_.find(beamlist->first());
		x1 = musElementList_.find(beamlist->last());
		if (x0 < 0 || x1 < 0) {
			NResource::abort("setBeamed: internal error", 5);
		}
		createUndoElement(x0, x1 - x0 + 1, 0);
		if (oldidx >= 0) musElementList_.at(oldidx);
		NChord::computeBeames(beamlist, stemPolicy_);
		beamlist = new QList<NChord>();
	}
	else {
		beamlist->clear();
		delete beamlist;
	}
}

void NVoice::eliminateRests(QList<NMusElement> *foundRests, int restSum, int over, NChord *lastChord) {
	QList<NMusElement> elems;
	int len1;

	if (lastChord->properties_ & PROP_BEAMED) lastChord->breakBeames();
	len1 = lastChord->getMidiLength() + over;
	elems.append(lastChord);
	collectAndInsertPlayable(lastChord->midiTime_, &elems, len1, true);
	collectAndInsertPlayable(lastChord->midiTime_ + len1, foundRests, restSum - over, true);
}

void NVoice::cleanupRests(int shortestRest, bool region) {
	//int x0, x1, idx;
	NChord* lastChord = 0;
	NMusElement *elem, *stop_elem;
	QList<NMusElement> foundRests;
	int restSum = 0;
	int over;
	int xpos0 = -1, xpos1 = -1;
	int idx0 = -1, idx1 = -1;

        if (region) {
	   if (startElement_ && endElement_) {
		if (endElementIdx_ > startElemIdx_) {
			xpos0 = startElement_->getXpos();
			xpos1 = endElement_->getXpos();
			idx0 = startElemIdx_;
			idx1 = endElementIdx_;
		}
		else {
			xpos0 = endElement_->getXpos();
			xpos1 = startElement_->getXpos();
			idx0 = endElementIdx_;
			idx1 = startElemIdx_;
		}
		if ((elem = musElementList_.at(idx0)) == 0) {
			 NResource::abort(" NVoice::setHalfsTo: internal error", 1);
		}
		createUndoElement(idx0, idx1 - idx0 + 1, 0);
	   }
	   else {
		elem = musElementList_.first();
		createUndoElement(0, musElementList_.count(), 0);
	   }
	}
	else {
		elem = musElementList_.first();
		createUndoElement(0, musElementList_.count(), 0);
	}


        for (;elem && (idx0 <= idx1 || xpos1 == -1); elem = musElementList_.next(), idx0++) {
		switch(elem->getType()) {
			case T_CHORD: 
				     if ((over = (restSum % shortestRest)) && lastChord) {
					eliminateRests(&foundRests, restSum, over, lastChord);
					if (musElementList_.find(lastChord) == -1) {
						NResource::abort("internal error: cleanupRests: chord not found");
					}
				     }
				     while (!foundRests.isEmpty()) {
					foundRests.first();
					foundRests.remove();
				     }
				     restSum = 0;
				     lastChord = (NChord *) elem;
				     break;
			case T_SIGN: if (elem->getSubType() & BAR_SYMS) {
					if ((over = (restSum % shortestRest)) && lastChord) {
                                     		eliminateRests(&foundRests, restSum, over , lastChord);
						if (musElementList_.find(lastChord) == -1) {
							NResource::abort("internal error: cleanupRests: chord not found");
						}
					}
					foundRests.clear();
					restSum = 0;
					lastChord = 0;
				     }
				     break;
			case T_REST: restSum += elem->getMidiLength(); 
				     foundRests.append(elem);
				     break;
		}
	}
	setCountOfAddedItems(musElementList_.count());
}

int NVoice::searchPositionAndUpdateSigns(int dest_xpos, NMusElement **elem, bool *found, NMusElement **elem_before /* = 0 */,
					int *countof128th /* = 0 */, int *lastbaridx  /* = 0 */, int *lastbarpos /* = 0 */, 
					int *lastbartime /* = 0 */) {
	*found = false;
	*elem = 0;
	if (lastbaridx) *lastbaridx = 0;
	if (lastbarpos) *lastbarpos = 0;
	if (lastbartime) *lastbartime = 0;
	if (elem_before) *elem_before = 0;
	if (countof128th) *countof128th = 128;
	if (musElementList_.count() < 1) {
		return -1;
	}
	*elem = musElementList_.first();
	while ((*elem) && !(*found)) {
		*found = (*elem)->getBbox()->x() >= dest_xpos;
		if (!(*found)) { 
			switch ((*elem)->getType()) {
			case T_CLEF:
				theStaff_->actualClef_.change((NClef *) (*elem));
				break;
			case T_SIGN:
				if (!((*elem)->getSubType() & BAR_SYMS)) break;
				if (lastbarpos) *lastbarpos = (*elem)->getXpos();
				if (lastbaridx) *lastbaridx = musElementList_.at();
				if (lastbartime) *lastbartime = (*elem)->midiTime_;
				break;
			case T_KEYSIG:
				theStaff_->actualKeysig_.change((NKeySig *) (*elem));
				break;
			case T_TIMESIG:
				if (countof128th) *countof128th = ((NTimeSig *) (*elem))->numOf128th();
				break;
			}
			if (elem_before) *elem_before = *elem;
			*elem = musElementList_.next();
		}
	}
	if (*found) {
		return musElementList_.at();
	}
	*elem = musElementList_.last();
	return musElementList_.at();
}

// search for first element with bbox.x >= dest_xpos
// in:		dest_xpos: the x position to search for
// out:		countof128th: the timesig in effect at that position in 128th
// returns:	void
// note:	if found, positions musElementList_ at the element found
//		if not found, sets musElementList_'s current item to 0

void NVoice::searchPositionAndUpdateTimesig(int dest_xpos, int *countof128th) {
	bool found = false;
	NMusElement *elem;
	*countof128th = 128;
	if (musElementList_.count() < 1) {
		return;
	}
	elem = musElementList_.first();
	while (elem && !found) {
		found = elem->getBbox()->x() >= dest_xpos;
		if (!found) { 
			if (elem->getType() == T_TIMESIG) *countof128th = ((NTimeSig *) elem)->numOf128th();
			elem = musElementList_.next();
		}
	}
}

int NVoice::validateKeysig(int lastbaridx, int insertpos) {
	NMusElement *elem;
	QList<NNote> *noteList;
	NNote *note;
	int lastbarpos;
	bool found;

	if (lastbaridx < 0) {
		searchPositionAndUpdateSigns(insertpos, &elem, &found, 0, 0, &lastbaridx, &lastbarpos);
	}
	else {
		elem = musElementList_.at(lastbaridx);
		lastbarpos = elem->getXpos();
	}
	theStaff_->actualKeysig_.deleteTempAccents();
	for (;elem && elem->getBbox()->x() < insertpos; elem = musElementList_.next()) {
		if (elem->getType() != T_CHORD) continue;
		((NChord *) elem)->accumulateAccidentals(&(theStaff_->actualKeysig_));
	}
	return lastbarpos;
}

void NVoice::validateKeysigAccordingPos(int lastbarpos, int insertpos) {
	NMusElement *elem;
	bool found;

	searchPositionAndUpdateSigns(insertpos, &elem, &found);
	if (!found) return;

	for (;elem && insertpos > elem->getBbox()->x(); elem = musElementList_.next()) {
		if (elem->getType() != T_CHORD) continue;
		((NChord *) elem)->accumulateAccidentals(&(theStaff_->actualKeysig_));
	}
}

void NVoice::setCorrectClefAccordingTime(int miditime) {
	int oldidx;
	NMusElement *elem;

	oldidx = musElementList_.at();
	theStaff_->actualClef_.change(NResource::nullClef_);
	for (elem = musElementList_.first(); elem && elem->midiTime_ <= miditime; elem = musElementList_.next()) {
		if (elem->getType() == T_CLEF) {
			theStaff_->actualClef_.change((NClef *) elem);
		}
	}

	if (oldidx >= 0) musElementList_.at(oldidx);
}



int NVoice::findLastBarTime(int xpos) {
	int lastbartime = 0;
	NMusElement *elem;
	for (elem = musElementList_.first(); elem; elem = musElementList_.next()) {
		if (elem->getXpos() >= xpos) break;
		if (elem->getType() == T_SIGN && (elem->getSubType() & BAR_SYMS)) lastbartime = elem->midiTime_;
	}
	
	return lastbartime;
}

// tryToBuildAutoTriplet -- try to automatically build a triplet containing
// the chord or rest at the current position
// in:		none
// out:		none
// returns:	void

void NVoice::tryToBuildAutoTriplet() {
	int oldidx = musElementList_.at();
	if (oldidx < 0) return;

	// determine index of elements which could become part of triplet
	int ppn = -1;			// index of previous previous note
	int pn = -1;			// index of previous note
	int cn = -1;			// index of current note
	NMusElement *elem = 0;
	elem = musElementList_.current();
	if (elem && elem->playable() && (elem->playable()->properties_ & PROP_AUTO_TRIPLET)
	    && !(elem->playable()->properties_ & PROP_TUPLET))
		cn = musElementList_.at();
	elem = musElementList_.prev();
	if (elem && elem->playable() && (elem->playable()->properties_ & PROP_AUTO_TRIPLET)
	    && !(elem->playable()->properties_ & PROP_TUPLET))
		pn = musElementList_.at();
	elem = musElementList_.prev();
	if (elem && elem->playable() && (elem->playable()->properties_ & PROP_AUTO_TRIPLET)
	    && !(elem->playable()->properties_ & PROP_TUPLET))
		ppn = musElementList_.at();

	QList<NPlayable> *elemlist = new QList<NPlayable>();
	bool ok = false;
	int x0 = ppn;
	int x1 = cn;
	// check if it is possible to build a three element triplet
	if ((ppn >= 0) && (pn >= 0) && (cn >= 0)) {
		ok = buildTupletList(x0, x1, 3, elemlist);
	}
	// if building a three element triplet failed,
	// check if it is possible to build a two element triplet
	if (!ok && (pn >= 0) && (cn >= 0)) {
		x0 = pn;
		ok = buildTupletList(x0, x1, 3, elemlist);
	}
	// if succeeded, actually build triplet, else cleanup
	if (ok) {
		createUndoElement(x0, x1 - x0 + 1, 0);
		NPlayable::computeTuplet(elemlist, 3, 2);
		// note: don't delete elemlist here, all tuplet notes refer to it
	} else {
		// if building the tuplet failed, elemlist is not used anymore
		delete elemlist;
	}
	musElementList_.at(oldidx);
}

void NVoice::insertAtPosition(int el_type, int xpos, int line, int sub_type, int offs, NMusElement *tmpElem) {
	NMusElement *new_elem, *elem_before, *elem;
	bool found, is_chord = false, is_rest = false;
	property_type properties = 0;
	int idx, idx2;
	int lastbaridx = 0;
	int dotcount;
	int countof128th, len, len2, lastElemTime;
	NRest *rest;
	NNote *part;
	NMusElement *specialElem, *startElement = 0;
	int newcount = 0;
	
	if (currentElement_) {
		currentElement_->setActual(false);
	}
	if (!firstVoice_) {
		theStaff_->searchPositionAndUpdateTimesig(xpos, &countof128th);
		idx = searchPositionAndUpdateSigns(xpos, &elem, &found, &elem_before);
	}
	else {
		idx = searchPositionAndUpdateSigns(xpos, &elem, &found, &elem_before, &countof128th, &lastbaridx);
	}
	if (elem_before && found && elem_before->playable() && elem->playable()) {
		if ((elem_before->playable()->properties_ & PROP_BEAMED) && (elem->playable()->properties_ & PROP_BEAMED)) {
			if (((NChord *) elem_before)->getBeamList() == ((NChord *) elem)->getBeamList()) {
				currentElement_ =musElementList_.prev();
				breakBeames();
				musElementList_.at(idx);
			}
		}
		if ((elem_before->playable()->properties_ & PROP_TUPLET) && (elem->playable()->properties_ & PROP_TUPLET) ) {
			if (elem_before->playable()->getTupletList() == elem->playable()->getTupletList()) {
				currentElement_ = musElementList_.prev();
				breakTuplet();
				musElementList_.at(idx);
			}
		}
	}
	if (!firstVoice_) {
		theStaff_->resetSpecialElement();
		lastElemTime = elem_before ? elem_before->midiTime_ + elem_before->getMidiLength() : 0;
		while (specialElem = theStaff_->findBarInStaffTillXpos(lastElemTime, xpos)) {
			len = specialElem->midiTime_ - lastElemTime;
			while (len >= MULTIPLICATOR) {
				len2 = quant(len, &dotcount, WHOLE_LENGTH);
				len -= dotcount ? 3 * len2 / 2 : len2;
				properties = PROP_HIDDEN;
				if (dotcount) properties |= PROP_SINGLE_DOT;
				rest = new NRest(main_props_, theStaff_->getStaffPropsAddr(), &yRestOffs_, len2, properties); 
				if (!startElement) startElement = rest;
				newcount++;
				if (found) {
					musElementList_.insert(idx++, rest);
				}
				else {
					musElementList_.append(rest);
				}
			}
			lastElemTime = specialElem->midiTime_;
		}
	}
	switch (el_type) {
		case T_CHORD:
			is_chord = true;
			properties = PROP_FORCE;
			if (offs == UNDEFINED_OFFS) {
				if (found) {
					idx2 = musElementList_.at();
				}
				else {
					idx2 = musElementList_.count() - 1;
				}
				if (idx2 >= 0) {
					theStaff_->validateKeysig(firstVoice_ ? lastbaridx : -1, xpos);
					offs = theStaff_->actualKeysig_.getOffset(line);
				}
				else {
					offs = 0;
				}
				properties = (unsigned int) 0;
			}
			if (main_props_->tied) properties |= PROP_TIED;
			if (main_props_->staccato) properties |= PROP_STACC;
			if (main_props_->sforzato) properties |= PROP_SFORZ;
			if (main_props_->portato) properties |= PROP_PORTA;
			if (main_props_->strong_pizzicato) properties |= PROP_STPIZ;
			if (main_props_->sforzando) properties |= PROP_SFZND;
			if (main_props_->fermate) properties |= PROP_FERMT;
			if (main_props_->grace) properties |= PROP_GRACE;
			if (main_props_->arpeggio) properties |= PROP_ARPEGG;
			properties |= (main_props_->dotcount & DOT_MASK);
			properties |= (main_props_->noteBody & BODY_MASK);
			if (main_props_->pedal_on) properties |= PROP_PEDAL_ON;
			if (main_props_->pedal_off) properties |= PROP_PEDAL_OFF;
			if (main_props_->triplet) properties |= PROP_AUTO_TRIPLET;
			new_elem = 
			new NChord(main_props_, &(theStaff_->staff_props_), this, line,  offs, main_props_->actualLength, stemPolicy_, properties );
				part = new_elem->chord()->getNoteList()->first();
		break;
		case T_REST:
			is_rest = true;
			properties = main_props_->dotcount | (main_props_->hidden ? PROP_HIDDEN : 0);
			if ((sub_type != MULTIREST) && main_props_->fermate) properties |= PROP_FERMT;
			new_elem = 
			new NRest(main_props_, &(theStaff_->staff_props_), &yRestOffs_, sub_type, properties);
		break;
		case T_SIGN:
			new_elem =
			new NSign(main_props_, &(theStaff_->staff_props_), sub_type);
			break;
		case T_CLEF:
			new_elem = tmpElem;
			break;
		default: NResource::abort("unknown music element");
	}
	new_elem->setActual(true);
	currentElement_ = new_elem;
	if (musElementList_.isEmpty()) {
		if (!startElement) startElement = new_elem;
		newcount++;
		musElementList_.append(new_elem);
		createUndoElement(startElement, 0, newcount);
		musElementList_.first();
		if (is_chord) {
			reconnectTies(part);
		}
		if (is_chord && main_props_->tied) {
			findTieMember(part);
		}
		if (is_chord && NResource::allowInsertEcho_) {
			NResource::mapper_->playImmediately(&(theStaff_->actualClef_), (NChord *) new_elem,
			theStaff_->getVoice(), theStaff_->getChannel(), theStaff_->getVolume(), theStaff_->transpose_);
		}
		return;
	}
	if (!startElement) startElement = new_elem;
	newcount++;
	if (!found) {
		musElementList_.append(new_elem);
	}
	else {
		musElementList_.insert(idx, new_elem);
	}
	createUndoElement(startElement, 0, newcount);
	if (is_chord) {
		reconnectTies(part);
	}
	if (is_chord && main_props_->tied) {
		findTieMember(part);
	}
	if ((is_chord || is_rest) && main_props_->triplet) {
		tryToBuildAutoTriplet();
	}
	if (is_chord && NResource::allowInsertEcho_) {
		NResource::mapper_->playImmediately(&(theStaff_->actualClef_), (NChord *) new_elem,
		theStaff_->getVoice(), theStaff_->getChannel(), theStaff_->getVolume(), theStaff_->transpose_);
	}
}

bool NVoice::insertNewNoteAt(int line, QPoint p, int offs) {
	int lastbaridx = 0;
	int lastbarpos = 0;
	bool found = false;
	property_type properties = PROP_FORCE;
	NMusElement *elem;
	NNote *note = 0;

	if (currentElement_) {
		currentElement_->setActual(false);
	}
	
	elem = musElementList_.first();
	while (!found  && elem != 0) {
		switch (elem->intersects_horizontally(p)) {
			case 0: found = true; break;
			case -1: return false;
			default: 
			   switch (elem->getType()) {
				case T_KEYSIG:
					theStaff_->actualKeysig_.change((NKeySig *) elem);
					break;
				case T_SIGN:
					if (elem->getSubType() & BAR_SYMS) {
						lastbaridx = musElementList_.at();
						lastbarpos = elem->getXpos();
					}
					break;
				 case T_CLEF:
					theStaff_->actualClef_.change((NClef *) elem);
					break;

			   }
			   elem = musElementList_.next();
		}
	}
	if (!found) {
		return false;
	}
	if (offs == UNDEFINED_OFFS) {
		theStaff_->validateKeysig(firstVoice_ ? lastbaridx : -1, elem->getBbox()->x());
		offs = theStaff_->actualKeysig_.getOffset(line);
		properties &= (~PROP_FORCE);
	}
	currentElement_ = elem;
	createUndoElement(currentElement_, 1, 0);
	if (main_props_->tied) properties |= PROP_TIED;
	properties |= main_props_->noteBody;
	if( currentElement_->getType() == T_CHORD )
		note = currentElement_->chord()->insertNewNote(line, offs, stemPolicy_, properties);
	if (note) {
		reconnectTies(note);
		if (main_props_->tied) {
			findTieMember(note);
		}
	}
	else {
		deleteLastUndo();
	}
	if (note && NResource::allowInsertEcho_) {
		NResource::mapper_->playImmediately(&(theStaff_->actualClef_), (NChord *) elem,
		theStaff_->getVoice(), theStaff_->getChannel(), theStaff_->getVolume(), theStaff_->transpose_);
	}
	if (currentElement_) {
		currentElement_->setActual(true);
	}
	return true;
}

bool NVoice::insertNewNoteAtCurrent(int line, int offs) {
	int lastbaridx = 0;
	bool found = false;
	property_type properties = PROP_FORCE;
	NMusElement *elem;
	NNote *note = 0;

	if (!currentElement_)  return false;
	
	elem = musElementList_.first();
	while (!found  && elem != 0) {
		if (!(found = elem == currentElement_)) {
			switch (elem->getType()) {
				case T_KEYSIG:
					theStaff_->actualKeysig_.change((NKeySig *) elem);
					break;
				case T_SIGN:
					if (elem->getSubType() & BAR_SYMS) {
						lastbaridx = musElementList_.at();
					}
					break;
				 case T_CLEF:
					theStaff_->actualClef_.change((NClef *) elem);
					break;

			}
			elem = musElementList_.next();
		}
	}
	if (!found) {
		NResource::abort("insertNewNoteAtCurrent: internal error");
	}
	if (offs == UNDEFINED_OFFS) {
		validateKeysig(lastbaridx, musElementList_.at());
		theStaff_->validateKeysig(lastbaridx, elem->getBbox()->x());
		offs = theStaff_->actualKeysig_.getOffset(line);
		properties &= (~PROP_FORCE);
	}
	currentElement_ = elem;
	createUndoElement(currentElement_, 1, 0);
	if (main_props_->tied) properties |= PROP_TIED;
	if( currentElement_->getType() == T_CHORD )
		note = currentElement_->chord()->insertNewNote(line, offs, stemPolicy_, properties);
	if (note) {
		reconnectTies(note);
		if (main_props_->tied) {
			findTieMember(note);
		}
	}
	else {
		deleteLastUndo();
	}
	if (note && NResource::allowInsertEcho_) {
		NResource::mapper_->playImmediately(&(theStaff_->actualClef_), (NChord *) elem,
		theStaff_->getVoice(), theStaff_->getChannel(), theStaff_->getVolume(), theStaff_->transpose_);
	}
	return true;
}

void NVoice::insertAfterCurrent(int el_type, int subtype) {
	int idx;
	NMusElement *new_elem;
	if ( (!musElementList_.isEmpty()) && (!currentElement_) ) return;
	
	switch (el_type) {
		case T_SIGN: new_elem = new NSign(main_props_, &(theStaff_->staff_props_), subtype);
				break;
		default: return;
	}
	if (currentElement_) currentElement_->setActual(false);
	if ( (currentElement_) && (musElementList_.find(currentElement_) == -1) ) {
		NResource::abort("insertAfterCurrent: internal error");
	}
	if ((!musElementList_.isEmpty()) && (musElementList_.next()) ) {
		idx = musElementList_.at();
		musElementList_.insert(idx, new_elem);
	}
	else {
		musElementList_.append(new_elem);
	}
	currentElement_ = musElementList_.current();
	createUndoElement(musElementList_.at(), 0, 1);
	currentElement_->setActual(true);
}

bool NVoice::insertAfterCurrent(NMusElement *elem) {
	int idx;
	bool is_chord = false;
	NNote *note;
	if ( (!musElementList_.isEmpty()) && (!currentElement_) ) return false;
	if ( (currentElement_) && (musElementList_.find(currentElement_) == -1) ) {
		NResource::abort("insertAfterCurrent: internal error");
	}
	if (elem->getType() == T_CHORD) {
		is_chord = true;
		note = elem->chord()->getNoteList()->first();
	}
	if (currentElement_) currentElement_->setActual(false);
	if ((!musElementList_.isEmpty()) && (musElementList_.next())) {
		idx = musElementList_.at();
		musElementList_.insert(idx, elem);
	}
	else {
		musElementList_.append(elem);
	}
	currentElement_ = musElementList_.current();
	createUndoElement(musElementList_.at(), 0, 1);
	if (is_chord) {
		reconnectTies(note);
	}
	if (is_chord && (note->properties & PROP_TIED)) {
		findTieMember(note);
	}
	currentElement_->setActual(true);
	return true;
}

void NVoice::insertBarAt(int xpos) {
	NSign *new_elem;
	NMusElement *elem;
	bool found;
	int idx;
	new_elem = new NSign(main_props_, &(theStaff_->staff_props_), SIMPLE_BAR);
	found = false;
	elem = musElementList_.first();
	while (!found  && elem != 0) {
		if (xpos > elem->getBbox()->x()) {
			elem = musElementList_.next();
		}
		else {
			found = true;
			idx = musElementList_.at()-1;
		}
	}
	if (!found) {
		musElementList_.append(new_elem);
	}
	else {
		musElementList_.insert(idx, new_elem);
	}
	if (currentElement_) currentElement_->setActual(true);
	currentElement_ = new_elem;
	currentElement_->setActual(true);
}

/*------------------------ playing voice -------------------------------------*/


int NVoice::getMidiTime() const {
	if (currentElement_) {
		return currentElement_->midiTime_;
	}
	return 0;
}

void NVoice::getTempoSigs(NTempoTrack *ttrack, int startTime) {
	NMusElement *elem;
	int actualTempo = DEFAULT_TEMPO;
	NSign *newsign, *sign;
	NChord *chord;
	int ending1Time;
	bool found2ndCoda = false;

	repeatIdx_ = 0;
	repeatTime_ = 0;
	endingIdx_ = -1;
	segnoIdx_ = -1;
	codaStatus_ = -1;
	idxOf2ndCodaSign_ = -2;
	repeatCount_ = 1;
	theStaff_->timeOffset_ = 0;

	for (elem = musElementList_.first(); !found2ndCoda && elem; elem = musElementList_.next()) {
		if (elem->getType() != T_SIGN) continue;
		if (elem->getSubType() != CODA) continue;
		switch (idxOf2ndCodaSign_) {
			case -2: idxOf2ndCodaSign_ = -1; break;
			case -1: idxOf2ndCodaSign_ = musElementList_.at(); timeOf2ndCoda_ = elem->midiTime_; found2ndCoda = true; break;
		}
	}

	for (elem = musElementList_.first(); elem; elem = musElementList_.next()) {
		if (elem->getType() == T_SIGN) {
			switch(elem->getSubType()) {
				case TEMPO_SIGNATURE: sign = (NSign *) elem;
						      actualTempo = sign->getTempo();
						      newsign = new NSign(0, &NResource::nullprops_, TEMPO_SIGNATURE);
						      newsign->setRealMidiTime(sign->midiTime_ + theStaff_->timeOffset_);
						      newsign->setTempo(actualTempo);
						      ttrack->insertTempoSign(newsign);
						break;
				case ACCELERANDO: newsign = new NSign(0, &NResource::nullprops_, ACCELERANDO);
						      newsign->setRealMidiTime(elem->midiTime_ + theStaff_->timeOffset_);
						      ttrack->insertTempoSign(newsign);
						break;
				case RITARDANDO: newsign = new NSign(0, &NResource::nullprops_, RITARDANDO);
						      newsign->setRealMidiTime(elem->midiTime_ + theStaff_->timeOffset_);
						      ttrack->insertTempoSign(newsign);
						break;
				case REPEAT_OPEN: repeatIdx_ = musElementList_.at(); 
						  repeatTime_ = elem->midiTime_;
						  break;
				case REPEAT_CLOSE: if (repeatIdx_ >= 0) {
						  	if (startTime <=  elem->midiTime_) theStaff_->timeOffset_ += elem->midiTime_ - repeatTime_;
							if (repeatCount_ == ((NSign *) elem)->getRepeatCount() - 1)  {
								endingIdx_ = musElementList_.at();
							}
						  	if (++repeatCount_ < ((NSign *) elem)->getRepeatCount()) {
								musElementList_.at(repeatIdx_);
								break;
							}
							musElementList_.at(repeatIdx_);
							repeatIdx_ = -1;
							repeatCount_ = 1;
						  }
						  else {
							endingIdx_ = -1;
						  }
						  break;
				case REPEAT_OPEN_CLOSE:
						  
						  if (repeatIdx_ >= 0) {
							endingIdx_ = musElementList_.at();
						  	if (startTime <=  elem->midiTime_) theStaff_->timeOffset_ += elem->midiTime_ - repeatTime_;
							musElementList_.at(repeatIdx_);
							repeatIdx_ = -1;
						  }
						  else {
							endingIdx_ = -1;
							repeatIdx_ = musElementList_.at(); 
							repeatTime_ = elem->midiTime_;
						  }
						  break;
				case SPECIAL_ENDING1:
						  if (endingIdx_ >= 0) {
							ending1Time = elem->midiTime_;
						  	elem = musElementList_.at(endingIdx_);
							if (startTime <=  elem->midiTime_) theStaff_->timeOffset_ -= elem->midiTime_ - ending1Time;
							endingIdx_ = -1;
						  }
						  break;
				case SEGNO:
						  segnoIdx_ = musElementList_.at();
						  segnoTime_ = elem->midiTime_;
						  break;
				case DAL_SEGNO:   if (segnoIdx_ < 0) break;
						  if (startTime <=  elem->midiTime_) theStaff_->timeOffset_ += elem->midiTime_ - segnoTime_;
						  musElementList_.at(segnoIdx_);
						  segnoIdx_ = -1;
						  break;
				case DAL_SEGNO_AL_CODA:
						  if (segnoIdx_ < 0 || codaStatus_  != -1) break;
						  codaStatus_ = 0;
						  if (startTime <=  elem->midiTime_) theStaff_->timeOffset_ += elem->midiTime_ - segnoTime_;
						  musElementList_.at(segnoIdx_);
						  segnoIdx_ = -2;
						  break;
				case DAL_SEGNO_AL_FINE:
						  if (segnoIdx_ < 0)  break;
						  if (startTime <=  elem->midiTime_) theStaff_->timeOffset_ += elem->midiTime_ - segnoTime_;
						  musElementList_.at(segnoIdx_);
						  segnoIdx_ = -2;
						  break;
				case CODA:	  if (idxOf2ndCodaSign_ < 0 || codaStatus_ < 0) break;
						  if (startTime <=  elem->midiTime_) {
						  	theStaff_->timeOffset_ += elem->midiTime_ - timeOf2ndCoda_;
						  }
						  musElementList_.at(idxOf2ndCodaSign_);
						  idxOf2ndCodaSign_ = -1;
						  break;
			}
		}
		else if (elem->getType() == T_CHORD) {
			chord = (NChord *) elem;
			if ((chord->properties_ & PROP_FERMT) == 0) continue;
			newsign = new NSign(0, &NResource::nullprops_, TEMPO_SIGNATURE);
			newsign->setTempo(actualTempo/2);
			newsign->setRealMidiTime(chord->midiTime_ + theStaff_->timeOffset_);
			ttrack->insertTempoSign(newsign);
			newsign = new NSign(0, &NResource::nullprops_, TEMPO_SIGNATURE);
			newsign->setTempo(actualTempo);
			newsign->setRealMidiTime(chord->midiTime_ + chord->getMidiLength() + theStaff_->timeOffset_);
			ttrack->insertTempoSign(newsign);
		}
	}
}

void NVoice::detectABCSpecials(bool *with_drums, bool *with_pedal_marks) { /* for abc music export */
	*with_drums = false;
	*with_pedal_marks = false;
	NMusElement *elem;
	NNote *note;
	for (elem = musElementList_.first(); elem; elem = musElementList_.next()) {
		if (elem->getType() == T_CHORD) {
			if (elem->chord()->properties_ & (PROP_PEDAL_ON | PROP_PEDAL_OFF)) *with_pedal_marks = true;
			for (note = elem->chord()->getNoteList()->first(); note; note = elem->chord()->getNoteList()->next()) {
				if (note->properties & BODY_MASK) *with_drums = true;
			}
			if (*with_pedal_marks && *with_drums) return;
		}
	}
}

int NVoice::determineAnacrusis() {
	NMusElement *elem;
	int old_idx;
	int total = 0, countof128th = 128;

	old_idx = musElementList_.at();
	for (elem = musElementList_.first(); elem; elem = musElementList_.next()) {
		switch (elem->getType()) {
			case T_CHORD:
			case T_REST: total += elem->getMidiLength();	
				     if (total > MULTIPLICATOR*countof128th) {
					if (old_idx >= 0) musElementList_.at(old_idx);
					return 0;
				     }
				     break;
			case T_TIMESIG:
				countof128th = ((NTimeSig *) elem)->numOf128th();
				break;
			case T_SIGN:
				if ((elem->getSubType() & BAR_SYMS) == 0) break;
				if (old_idx >= 0) musElementList_.at(old_idx);
				return ((total / MULTIPLICATOR) % countof128th);
		}
	}
	if (old_idx >= 0) musElementList_.at(old_idx);
	return total / MULTIPLICATOR;
}
	

int NVoice::getMidiPos() const {
	if (currentElement_) {
		return currentElement_->getXpos();
	}
	return 0;
}

/* The whole masure length in MIDI units */
int NVoice::getCurrentMeasureMidiLength() {
	int i; int pointerOffset = 0;
	int numerator; int denominator; int midiLength;
	
	if (musElementList_.count() == 0) return 4*QUARTER_LENGTH; /* returns default value if musElementList_ is still empty */
	
	/* we try seek out the last placed time signature */
	while (!(
			(musElementList_.current() == musElementList_.getFirst()) || /* if the current elt. is the first one */
			(musElementList_.at() == -1) || /* if the current elt. is the null one */
			(musElementList_.current()->getType() == T_TIMESIG) )) /* if the current element is a time signature */ {
		musElementList_.prev();
		pointerOffset++;
	}
	
	if ((musElementList_.at() == -1) || (musElementList_.current()->getType() != T_TIMESIG)) 
		midiLength = 4 * QUARTER_LENGTH; /* sets the default midi length if no time signatures found */
	else {
		numerator = ((NTimeSig *) musElementList_.current())->getNumerator();
		switch ( ((NTimeSig *) musElementList_.current())->getDenominator() ) {
			case 1:
				midiLength = numerator * WHOLE_LENGTH;
				break;
			case 2:
				midiLength = numerator * HALF_LENGTH;
				break;
			case 4:
				midiLength = numerator * QUARTER_LENGTH;
				break;
			case 8:
				midiLength = numerator * NOTE8_LENGTH;
				break;
			case 16:
				midiLength = numerator * NOTE16_LENGTH;
				break;
			case 32:
				midiLength = numerator * NOTE32_LENGTH;
				break;
			case 64:
				midiLength = numerator * NOTE64_LENGTH;
				break;
			case 128:
				midiLength = numerator * NOTE128_LENGTH;
				break;
		}
	}
	
	/* set the current musElementList item to the original one */
	for (i=0; i < pointerOffset; i++) musElementList_.next();
	
	return midiLength;
}

void NVoice::startPlaying(int starttime) {
	NSign *sign;
	bool programChangeSeen = false;
	int i;

	if (currentElement_) {
		currentElement_->setActual(false);
		currentElement_->draw();
	}
	computeVolumesAndSearchFor2ndCodaSign();
	playPosition_ = musElementList_.first();
	u1_.pending_prog_change = -1;
	while (playPosition_) {
		if (firstVoice_) {
			switch (playPosition_->getType()) {
			case T_SIGN:
				sign = (NSign *) playPosition_;
				switch(playPosition_->getSubType()) {
					case PROGRAM_CHANGE:
						NResource::mapper_->changeProg(theStaff_->getChannel(), sign->getProgram());
							programChangeSeen = true;
							break;
				}
				break;
			case T_CLEF: theStaff_->playClef_ = (NClef *) playPosition_;
				break;
			case T_KEYSIG:  if (theStaff_->playKeySig_) {
						delete theStaff_->playKeySig_;
					}
					theStaff_->playKeySig_ = new NKeySig(main_props_ , &(theStaff_->staff_props_));
					theStaff_->playKeySig_->changeInContextKeySig((NKeySig *) playPosition_); /* for context presentation only */
				break;
			}
		}
		if (playPosition_->midiTime_ >= starttime) {
			break;
		}
		playPosition_ = musElementList_.next();
	}
	repeatIdx_ = 0;
	repeatTime_ = 0;
	endingIdx_ = -1;
	segnoIdx_ = -1;
	codaStatus_ = -1;
	stopped_at_fine_ = false;
	theStaff_->timeOffset_ = 0;
	if (!programChangeSeen) 
		NResource::mapper_->changeProg(theStaff_->getChannel(), theStaff_->getVoice());
	NResource::mapper_->changeReverb(theStaff_->getChannel(), theStaff_->reverb_);
	NResource::mapper_->changeChorus(theStaff_->getChannel(), theStaff_->chorus_);
	NResource::mapper_->changePan(theStaff_->getChannel(), theStaff_->pan_);
	actualMidiEvent_ = &(midievents_[0]);
	for (i = 0; i < MIDI_EVENT_RING; i++) {
		 midievents_[i].valid = false;
		 midievents_[i].midi_prog_change = -1;
		 midievents_[i].partlength = 0;
	}
	dynEndPos_ = vaEndPos_ = trillEndPos_ = vaEndPos_= vaOffset_ = 0;
	inVolumeCrtlMode_ = false;
	theStaff_->pending_clef_ = 0;
}

void NVoice::stopPlaying() {
	NMusElement *elem;
	for (elem = musElementList_.first(); elem; elem = musElementList_.next()) {
		elem->setActual(false);
	}
	if (currentElement_) {
		currentElement_->setActual(true);
	}

}

void NVoice::setMarker() {
	repeatIdx_ = musElementList_.at();
}

void NVoice::setSegnoMarker() {
	segnoIdx_ = musElementList_.at();
}

void NVoice::setCodaMarker(int timeOf2ndCoda) {
	NMusElement *elem;

	for (elem = musElementList_.first(); elem; elem = musElementList_.next()) {
		if (elem->midiTime_ >= timeOf2ndCoda) {
			idxOf2ndCodaSign_ = musElementList_.at();
			return;
		}
	}
}

void NVoice::gotoCodaMarker() {
	if (idxOf2ndCodaSign_ < 0) return;
	playPosition_ = musElementList_.at(idxOf2ndCodaSign_);
	idxOf2ndCodaSign_ = -1;
	actualMidiEvent_->valid = false;
}

void NVoice::gotoMarker(bool again) {
	dynEndPos_ = vaEndPos_ = trillEndPos_ = vaOffset_ = 0;
	if (repeatIdx_ >= 0) {
		playPosition_ = musElementList_.at(repeatIdx_);
		actualMidiEvent_->valid = false;
		if (again) return;
		endingIdx_ = musElementList_.at();
		repeatIdx_ = -1;
	}
	else {
		endingIdx_ = -1;
	}
}

void NVoice::gotoSegnoMarker() {
	if (segnoIdx_ < 0) return;
	dynEndPos_ = vaEndPos_ = trillEndPos_ = vaOffset_ = 0;
	playPosition_ = musElementList_.at(segnoIdx_);
	actualMidiEvent_->valid = false;
}

void NVoice::handleEnding1() {
	if (endingIdx_ >= 0) {
		playPosition_ = musElementList_.at(endingIdx_);
		actualMidiEvent_->valid = false;
		endingIdx_ = -1;
	}
}


NMidiEventStr* NVoice::getNextMidiEvent(int mtime, bool reachInfo) {
	bool found = false;
	bool isGraceNote = false;
	NMidiEventStr *note_halt;
	QList<NNote> *notelist;
	NNote *note;
	bool partOfTie;
	NChord *chord;
	NSign *sign;
	int ending1Time;
	if (muted_ || stopped_at_fine_)  return 0;
	// if (actualMidiEvent_ == 0) {printf("actualMidiEvent_ == 0, firstVoice_ = %d\n", firstVoice_);}
	else if (actualMidiEvent_->valid) {
		return actualMidiEvent_;
	}
	while (!found && playPosition_) {
		switch (playPosition_->getType ()) {
		case T_CLEF:
			theStaff_->pending_clef_ = (NClef *) playPosition_;
			playPosition_ = musElementList_.next();
			break;
		case T_KEYSIG:
			if (theStaff_->playKeySig_) {
				delete theStaff_->playKeySig_; 
			}
			theStaff_->playKeySig_ = new NKeySig(main_props_ , &(theStaff_->staff_props_));
			theStaff_->playKeySig_->change((NKeySig *) playPosition_); /* for context presentation only */
			theStaff_->playKeySig_->setClef(theStaff_->playClef_);
			playPosition_ = musElementList_.next();
			break;
		case T_CHORD:
			chord = (NChord *) playPosition_;
			if (chord->properties_ & PROP_GRACE) {
				if (playPosition_->midiTime_ <= 0) {
					playPosition_ = musElementList_.next();
					continue;
				}
				isGraceNote = true;
				if (chord->getGraceMidiStartTime() + theStaff_->timeOffset_ < mtime) {
/*
					fprintf(stderr, "chord skipped\n");
*/
					playPosition_ = musElementList_.next();
				}
				else {
					found = true;
				}
				break;
			}
			if (playPosition_->midiTime_ + theStaff_->timeOffset_ < mtime) {
/*
				fprintf(stderr, "chord skipped\n");
*/
				playPosition_ = musElementList_.next();
			}
			else {
				found = true;
			}
			break;
		case T_SIGN:
			switch (playPosition_->getSubType()) {
				case REPEAT_OPEN: repeatIdx_ = musElementList_.at(); 
						  repeatTime_ = playPosition_->midiTime_;
						  repeatCount_ = 1;
						  theStaff_->setMarker();
						  break;
				case REPEAT_CLOSE: 
						  if (repeatIdx_ >= 0) {
							dynEndPos_ = vaEndPos_ = trillEndPos_ = vaOffset_ = 0;
						  	theStaff_->timeOffset_ += playPosition_->midiTime_ - repeatTime_;
							if (repeatCount_ == ((NSign *) playPosition_)->getRepeatCount() - 1)  {
								endingIdx_ = musElementList_.at();
							}
						  	if (++repeatCount_ < ((NSign *) playPosition_)->getRepeatCount()) {
								musElementList_.at(repeatIdx_);
								theStaff_->gotoMarker(true);
								break;
							}
							musElementList_.at(repeatIdx_);
							theStaff_->gotoMarker(false);
							repeatIdx_ = -1;
							repeatCount_ = 1;
						  }
						  else {
							endingIdx_ = -1;
						  }
						  break;
				case REPEAT_OPEN_CLOSE:
						  if (repeatIdx_ >= 0) {
							dynEndPos_ = vaEndPos_ = trillEndPos_ = vaOffset_ = 0;
							endingIdx_ = musElementList_.at();
						  	theStaff_->timeOffset_ += playPosition_->midiTime_ - repeatTime_;
							musElementList_.at(repeatIdx_);
							repeatIdx_ = -1;
						  	theStaff_->gotoMarker(false);
						  }
						  else {
							endingIdx_ = -1;
							repeatIdx_ = musElementList_.at(); 
							repeatTime_ = playPosition_->midiTime_;
							theStaff_->setMarker();
						  }
						  break;
				case SPECIAL_ENDING1:
						  if (endingIdx_ >= 0) {
							ending1Time = playPosition_->midiTime_;
						  	playPosition_ = musElementList_.at(endingIdx_);
							theStaff_->timeOffset_ -= playPosition_->midiTime_ - ending1Time;
							endingIdx_ = -1;
						  }
						  theStaff_->handleEnding1();
						  break;
				case SEGNO:
						  segnoIdx_ = musElementList_.at();
						  segnoTime_ = playPosition_->midiTime_;
						  theStaff_->setSegnoMarker();
						  break;
				case DAL_SEGNO:   if (segnoIdx_ < 0) break;
						  dynEndPos_ = vaEndPos_ = trillEndPos_ = vaOffset_ = 0;
						  theStaff_->timeOffset_ += playPosition_->midiTime_ - segnoTime_;
						  musElementList_.at(segnoIdx_);
						  segnoIdx_ = -1;
						  theStaff_->gotoSegnoMarker();
						  break;
				case DAL_SEGNO_AL_CODA:
						  if (segnoIdx_ < 0 || codaStatus_  != -1) break;
						  codaStatus_ = 0;
						  dynEndPos_ = vaEndPos_ = trillEndPos_ = vaOffset_ = 0;
						  theStaff_->timeOffset_ += playPosition_->midiTime_ - segnoTime_;
						  musElementList_.at(segnoIdx_);
						  segnoIdx_ = -2;
						  theStaff_->gotoSegnoMarker();
						  break;
				case DAL_SEGNO_AL_FINE:
						  if (segnoIdx_ < 0)  break;
						  dynEndPos_ = vaEndPos_ = trillEndPos_ = vaOffset_ = 0;
						  theStaff_->timeOffset_ += playPosition_->midiTime_ - segnoTime_;
						  musElementList_.at(segnoIdx_);
						  segnoIdx_ = -2;
						  theStaff_->gotoSegnoMarker();
						  break;
				case CODA:	  if (idxOf2ndCodaSign_ < 0 || codaStatus_ < 0) break;
						  dynEndPos_ = vaEndPos_ = trillEndPos_ = vaOffset_ = 0;
						  theStaff_->timeOffset_ += playPosition_->midiTime_  - timeOf2ndCoda_;
						  musElementList_.at(idxOf2ndCodaSign_);
						  idxOf2ndCodaSign_ = -1;
						  theStaff_->gotoCodaMarker();
						  break;
				case FINE:	  if (segnoIdx_ != -2) break;
						  stopped_at_fine_ = true;
						  theStaff_->stopAllVoices();
						  return 0;
				case PROGRAM_CHANGE:
						u1_.pending_prog_change = ((NSign *) playPosition_)->getProgram();
						break;
			}
			playPosition_ = musElementList_.next();
			break;
		case T_TIMESIG:
			if (reachInfo) {
				actualMidiEvent_->ev_time = (playPosition_->midiTime_ + theStaff_->timeOffset_);
				actualMidiEvent_->midi_cmd = MTIMESIG;
				actualMidiEvent_->midi_channel = 0;
				actualMidiEvent_->volume = 0;
				actualMidiEvent_->valid = false;
				actualMidiEvent_->length = 0;
				actualMidiEvent_->notelist = 0;
				actualMidiEvent_->xpos = 0;
				actualMidiEvent_->from = 0;
				actualMidiEvent_->midi_prog_change = -1;
				actualMidiEvent_->ref = playPosition_;
				actualMidiEvent_->notehalt->valid = false;
				playPosition_ = musElementList_.next();
				return actualMidiEvent_;
			}
			playPosition_ = musElementList_.next();
			break;
		default:
			playPosition_ = musElementList_.next();
			break;
		}
	}
	if (found) {
		chord = (NChord *) playPosition_;
		note_halt = actualMidiEvent_->notehalt;
		if (isGraceNote) {
			actualMidiEvent_->ev_time = (chord->getGraceMidiStartTime() + theStaff_->timeOffset_);
		}
		else {
			actualMidiEvent_->ev_time = (playPosition_->midiTime_ + theStaff_->timeOffset_);
		}
		if (theStaff_->pending_clef_) {
			if (chord->midiTime_ >= theStaff_->pending_clef_->midiTime_) {
				theStaff_->playClef_ = theStaff_->pending_clef_;
				theStaff_->pending_clef_ = 0;
				if (theStaff_->playKeySig_) {
					theStaff_->playKeySig_->setClef(theStaff_->playClef_);
				}
			}
		}
		if (chord->va_) {
			if (chord->va_ < 0) vaOffset_ = -12; else vaOffset_ =  12;
			vaEndPos_ =  chord->getVaEnd();
		}
		if (vaEndPos_ && playPosition_->getBbox()->left() > vaEndPos_) {
			vaEndPos_ = 0;
			vaOffset_ = 0;
		}
		for (note = chord->getNoteList()->first(); note; note = chord->getNoteList()->next()) {
			note->midiPitch = theStaff_->playClef_->line2Midi( note->line, note->offs ) + theStaff_->transpose_+vaOffset_;
		}
		actualMidiEvent_->midi_cmd = MNOTE_ON;
		note_halt->midi_cmd = MNOTE_OFF;
		note_halt->midi_channel = actualMidiEvent_->midi_channel = theStaff_->getChannel();
		note_halt->volume =
		actualMidiEvent_->volume = chord->auxInfo_.dynamic_descr.volume;
		note_halt->valid = true;
		actualMidiEvent_->volum_ctrl_change = -1;
		actualMidiEvent_->trilloffs = note_halt->trilloffs = 0;
		actualMidiEvent_->status = 0;
		if (chord->dynamic_ || playPosition_->getBbox()->left() <= dynEndPos_) {
#ifdef WITH_FADE_IN
			if ( playPosition_->getMidiLength(true) >= 2* DYNAMIC_PRECISION) {
				actualMidiEvent_->special = SPEC_DYNAMIC;
				dynamicRefTime_ = actualMidiEvent_->internalMidiTime = playPosition_->midiTime_;
				dynamicRefVolume_ = actualMidiEvent_->volume;
				actualMidiEvent_->volume = 127;
				inVolumeCrtlMode_ = true;
				actualMidiEvent_->volum_ctrl_change = dynamicRefVolume_;
				actualMidiEvent_->length = DYNAMIC_PRECISION;
				actualMidiEvent_->partlength = playPosition_->getMidiLength(true) / DYNAMIC_PRECISION  - 1;
				if (chord->dynamic_) {
					dynEndPos_ = chord->getDynamicEnd();
					volIncrease_= chord->auxInfo_.dynamic_descr.increase;
				}
				note_halt->valid = false;
			}
			else {
#endif
				actualMidiEvent_->partlength = 0;
				actualMidiEvent_->special = NO_SPECIAL;
				actualMidiEvent_->length = playPosition_->getMidiLength(true);
				actualMidiEvent_->volume = chord->auxInfo_.dynamic_descr.increase;
				if (chord->dynamic_) {
					dynEndPos_ = chord->getDynamicEnd();
				}
				note_halt->valid = true;
#ifdef WITH_FADE_IN
			}
#endif
		}
		else if ((chord->trill_  || playPosition_->getBbox()->left() <= trillEndPos_) && playPosition_->getMidiLength(true) > TRILL_MIDI_LENGTH) {
			note_halt->special = actualMidiEvent_->special = SPEC_TRILL_DOWN;
			actualMidiEvent_->partlength = playPosition_->getMidiLength(true) / TRILL_MIDI_LENGTH - 1;
			actualMidiEvent_->trillDist =
				 theStaff_->actualKeysig_.determineDistanceUp(
				chord->getNoteList()->first());
			note_halt->trilloffs =
			actualMidiEvent_->trilloffs = actualMidiEvent_->trillDist;

			actualMidiEvent_->length = TRILL_MIDI_LENGTH;
			if (chord->trill_) trillEndPos_ = chord->getTrillEnd();
		}
		else if ((chord->properties_ & PROP_ARPEGG) && chord->getNoteList()->count() > 1 && playPosition_->getMidiLength(true) > chord->getNoteList()->count() * ARPEGGIO_MIDI_LENGTH) {
			note_halt->special = actualMidiEvent_->special = SPEC_ARPEGGIO;
			note_halt->arpegg_total = actualMidiEvent_->arpegg_total = chord->getNoteList()->count();
			actualMidiEvent_->partlength = actualMidiEvent_->arpegg_total;
			actualMidiEvent_->length = ARPEGGIO_MIDI_LENGTH;
			actualMidiEvent_->arpegg_current = 0;
			note_halt->valid = false;
		}
		else {
			actualMidiEvent_->length = (playPosition_->playable() && (playPosition_->playable()->properties_ & PROP_STACC)) ? 
			(playPosition_->getMidiLength(true) >> 1) : playPosition_->getMidiLength(true);
			actualMidiEvent_->partlength = 0;
			note_halt->special = actualMidiEvent_->special = NO_SPECIAL;
		}
		if (inVolumeCrtlMode_ && actualMidiEvent_->volum_ctrl_change == -1) {
			notelist = chord->getNoteList();
			for (partOfTie = false, note = notelist->first(); note && !partOfTie; note = notelist->next()) {
				if (note->properties & PROP_PART_OF_TIE) partOfTie = true;
			}
			if (!partOfTie) {
				inVolumeCrtlMode_ = false;
				if (actualMidiEvent_->volum_ctrl_change != -1) {
					note_halt->volume = 
					actualMidiEvent_->volume = actualMidiEvent_->volum_ctrl_change;
				}
				actualMidiEvent_->volum_ctrl_change = 127;
			}
			else {
				actualMidiEvent_->volume = 127;
			}
		}
		if (chord->properties_ & PROP_PEDAL_ON) actualMidiEvent_->status |= MIDI_STAT_PEDAL_ON;
		if (chord->properties_ & PROP_PEDAL_OFF) actualMidiEvent_->status  |= MIDI_STAT_PEDAL_OFF;
		note_halt->notelist = 
		actualMidiEvent_->notelist = chord->getNoteList();
		note_halt->xpos = 
		actualMidiEvent_->xpos = playPosition_->getXpos();
		note_halt->from = 
		actualMidiEvent_->from = this;
		note_halt->midi_prog_change = -1;
		if ((actualMidiEvent_->midi_prog_change = u1_.pending_prog_change) >= 0) {
			u1_.pending_prog_change = -1;
		}
		note_halt->ref = 
		actualMidiEvent_->ref = playPosition_;
		actualMidiEvent_->valid = true;
		return actualMidiEvent_;
	}
	return 0;
}

void NVoice::skipAndInvalidate(bool doSkip) {
	if (doSkip) {
		playPosition_ = musElementList_.next();
	}
	actualMidiEvent_->valid = false;
}

void NVoice::skipTeXChord() {
	playPosition_ = musElementList_.next();
	pPtr_ = 0;
}

int NVoice::findNextVolumeSignature() {
	int pos = musElementList_.at();
	NMusElement *elem;

	for (elem = musElementList_.next(); elem; elem = musElementList_.next()) {
		if (elem->getType() != T_SIGN || elem->getSubType() != VOLUME_SIG) continue;
		if (pos >= 0) musElementList_.at(pos);
		return ((NSign *) elem)->getVolume();
	}
	if (pos >= 0) musElementList_.at(pos);
	return -1;
}


void NVoice::computeVolumesAndSearchFor2ndCodaSign() {
	int volume, endvolume;
	int dynamicEndPos;
	int lastVolume;
	int volumeIncreaseStartTime;
	double currentVolumeIncrease;
	NMusElement *elem;
	NChord *chord;
	NRest *rest;
	int endtime;

	dynamicEndPos = 0; lastVolume = -1;
	idxOf2ndCodaSign_ = -1;
	volume = theStaff_->getVolume();
	for (elem = musElementList_.first(); elem; elem = musElementList_.next()) {
		switch (elem->getType()) {
		case T_CHORD: chord = (NChord *) elem; 
			      if (chord->dynamic_) {	
					chord->auxInfo_.dynamic_descr.volume = volume;
					endvolume = findNextVolumeSignature();
                              		endtime = findEndOfCrescendo(chord);
					volumeIncreaseStartTime = chord->midiTime_;
                              		if (endvolume < 0) {
						chord->auxInfo_.dynamic_descr.increase =
						currentVolumeIncrease = 0.0;
					}
					else {
						chord->auxInfo_.dynamic_descr.increase = 
						currentVolumeIncrease = ((double) (endvolume - volume)) / ((double) (endtime - volumeIncreaseStartTime));
						dynamicEndPos = chord->getDynamicEnd();
					}
			      }
			      else if (chord->getBbox()->left() <= dynamicEndPos) {
					lastVolume = chord->auxInfo_.dynamic_descr.volume = volume +
					 		(int) (currentVolumeIncrease * (chord->midiTime_ - volumeIncreaseStartTime));
			      }
			      else if (lastVolume >= 0) {
					chord->auxInfo_.dynamic_descr.volume = lastVolume;
			      }
			      else {
					chord->auxInfo_.dynamic_descr.volume = volume;
			      }
			      break;
		case T_SIGN: switch (elem->getSubType()) {
				case VOLUME_SIG:
			     		volume = ((NSign *) elem) ->getVolume();
			     		lastVolume = -1;
			     		break;
				case CODA: if (!firstVoice_) break;
					switch (idxOf2ndCodaSign_) {
						case -2: idxOf2ndCodaSign_ = musElementList_.at();
							 timeOf2ndCoda_ = elem->midiTime_;
							 theStaff_->setCodaMarker(timeOf2ndCoda_);
							 break;
						case -1: idxOf2ndCodaSign_ = -2;
							 break;
					}
					break;
			      }
			      break;
		}
	}
}


void NVoice::skipChord() {
	NMidiEventStr *next, *note_halt;
	if (actualMidiEvent_->partlength) {
		next = actualMidiEvent_->next;
		note_halt = next->notehalt;
		next->status = actualMidiEvent_->status;
		next->special = actualMidiEvent_->special;
		note_halt->midi_channel = next->midi_channel = actualMidiEvent_->midi_channel;
		note_halt->notelist = next->notelist = actualMidiEvent_->notelist;
		note_halt->xpos = next->xpos = actualMidiEvent_->xpos;
		note_halt->from = next->from = actualMidiEvent_->from;
		note_halt->ref = next->ref = actualMidiEvent_->ref;
		next->partlength = actualMidiEvent_->partlength - 1;
		next->valid = true;
		switch (actualMidiEvent_->special) {
		   case SPEC_TRILL_UP:
			next->midi_cmd = MNOTE_ON;
			note_halt->midi_cmd = MNOTE_OFF;
			next->trillDist = actualMidiEvent_->trillDist;
			next->length = TRILL_MIDI_LENGTH;
			note_halt->volume = next->volume = actualMidiEvent_->volume;
			note_halt->special = next->special = SPEC_TRILL_DOWN;
			note_halt->trilloffs = next->trilloffs = actualMidiEvent_->trillDist;
			next->ev_time = actualMidiEvent_->ev_time + TRILL_MIDI_LENGTH;
			note_halt->valid = true;
			break;
		   case SPEC_TRILL_DOWN: 
			next->midi_cmd = MNOTE_ON;
			note_halt->midi_cmd = MNOTE_OFF;
			next->trillDist = actualMidiEvent_->trillDist;
			next->length = TRILL_MIDI_LENGTH;
			note_halt->special = next->special = SPEC_TRILL_UP;
			next->ev_time = actualMidiEvent_->ev_time + TRILL_MIDI_LENGTH;
			note_halt->valid = true;
			break;
		   case SPEC_ARPEGGIO:
		   	next->midi_cmd = MNOTE_ON;
			note_halt->midi_cmd = MNOTE_OFF;
			next->trilloffs =  note_halt->trilloffs = 0;
			if (actualMidiEvent_->arpegg_current == actualMidiEvent_->arpegg_total - 1) { 
				next->length = actualMidiEvent_->ref->getMidiLength(true) - actualMidiEvent_->arpegg_total * ARPEGGIO_MIDI_LENGTH;
				next->ev_time = actualMidiEvent_->ev_time-10;  /* avoid to note on again */
				note_halt->ev_time = actualMidiEvent_->ev_time + next->length;
				next->arpegg_current = actualMidiEvent_->arpegg_current + 1;
				next->arpegg_total = actualMidiEvent_->arpegg_total;
				next->volum_ctrl_change = -1;
				next->midi_prog_change = -1;
				note_halt->volume = next->volume = actualMidiEvent_->volume;
				note_halt->special = next->special = SPEC_ARPEGGIO;
				next->valid = true;
				note_halt->valid = true;
			}
			else {
				next->length = ARPEGGIO_MIDI_LENGTH;
				next->ev_time = actualMidiEvent_->ev_time + ARPEGGIO_MIDI_LENGTH;
				next->arpegg_current = actualMidiEvent_->arpegg_current + 1;
				next->arpegg_total = actualMidiEvent_->arpegg_total;
				next->volum_ctrl_change = -1;
				next->midi_prog_change = -1;
				note_halt->volume = next->volume = actualMidiEvent_->volume;
				note_halt->special = next->special = SPEC_ARPEGGIO;
				note_halt->valid = false;
			}
			break;
#ifdef WITH_FADE_IN
		   case SPEC_DYNAMIC:
			next->trilloffs =  note_halt->trilloffs = 0;
			next->midi_cmd = MVOL_CONTROL;
			note_halt->midi_cmd = MNOTE_OFF;
			next->length = DYNAMIC_PRECISION;
			note_halt->volume = next->volume = actualMidiEvent_->volume;
			note_halt->special = next->special = SPEC_DYNAMIC;
			next->ev_time = actualMidiEvent_->ev_time + DYNAMIC_PRECISION;
			next->internalMidiTime = actualMidiEvent_->internalMidiTime + DYNAMIC_PRECISION;
			note_halt->volume = next->volume = dynamicRefVolume_+
				(int) (volIncrease_ * (next->internalMidiTime - dynamicRefTime_));
			note_halt->valid = (next->partlength == 0);
			break;
#endif
		}
		actualMidiEvent_ = actualMidiEvent_->next;
	}
	else {
		playPosition_ = musElementList_.next();
		actualMidiEvent_ = actualMidiEvent_->next;
		actualMidiEvent_->valid = false;
	}
}

/*------------------------- repositioning voice --------------------------------*/

NPositStr* NVoice::getElementAfter(int mtime) {
	bool found = false;
	int oldidx;

	if (!playPosition_) return 0;
	if (pPtr_) return pPtr_;
	while (!found && playPosition_) {
		if (playPosition_->midiTime_ < mtime) {
			KMessageBox::error
				(0,
				 i18n("posit: chord skipped"),
				 kapp->makeStdCaption(i18n("Repositioning voice"))
				);
			playPosition_ = musElementList_.next();
		}
		else {
			if (playPosition_->getType() == T_KEYSIG) {
				((NKeySig *) playPosition_)->setPreviousKeySig(lastKeySig_);
				lastKeySig_ =  ((NKeySig *) playPosition_);
				theStaff_->actualKeysig_.change((NKeySig*) playPosition_);
				theStaff_->actualKeysig_.deleteTempAccents();
				if (theStaff_->actualKeysig_.isDrawable()) {
					found = true;
				}
				else {
					musElementList_.remove();
					playPosition_ = musElementList_.current();
				}
			}
			else {
				found = true;
			}
		}
	}
	if (found) {
		pPtr_ = new NPositStr;
		switch (pPtr_->ev_type = playPosition_->getType()) {
			case T_CHORD:
					if (((NChord *) playPosition_)->properties_ & PROP_GRACE) {
						pPtr_->ev_type = PROP_GRACE; /* make "ev_type & PLAYABLE == false" */
					}
			case T_REST:
					pPtr_->ev_time = playPosition_->midiTime_;
					break;
			default:
					if (playPosition_->getType() == T_CLEF) {
						theStaff_->actualClef_.change((NClef*) playPosition_);
					}
					else if (playPosition_->getType() == T_SIGN && 
						(playPosition_->getSubType() & BAR_SYMS)) {
						
						/* if multirest is present anywhere in the previous measure, count additional multiRestLength amount of measures */
						int pointerOffset = 0;
						do {
							if (musElementList_.current() == musElementList_.getFirst()) break;
							if ((musElementList_.prev()->getType() == T_REST) && (musElementList_.current()->getSubType() == MULTIREST)) 
								((NSign *) playPosition_)->setBarNr( (barNr_ += ((NRest *) musElementList_.current())->getMultiRestLength() - 1) );
							pointerOffset++;
						/* walk through every element until the measureline or the beginning of the document is found */
						} while (!( ((musElementList_.current()->getType() == T_SIGN) && (musElementList_.current()->getSubType() & BAR_SYMS)) || (musElementList_.current() == musElementList_.getFirst()) ));
						
						/* set the current musElementList_ item index to the previous value*/
						int i; for (i=0; i < pointerOffset; i++) musElementList_.next();
						
						/* increase the next bar number by 1 */
						((NSign *) playPosition_)->setBarNr(++barNr_);
						
						theStaff_->actualKeysig_.deleteTempAccents();
					}
					oldidx = musElementList_.at();
					playPosition_ = musElementList_.next();
					if (playPosition_) {
						pPtr_->ev_time = playPosition_->midiTime_;
					}
					else {
						pPtr_->ev_time = midiEndTime_;
					}
					playPosition_ = musElementList_.at(oldidx);
					break;
		}
		QRect *q;
		q = playPosition_->getBbox();
		pPtr_->from = this;
		pPtr_->elem = playPosition_;
		return pPtr_;
	}
	return 0;
}

void NVoice::startRepositioning() {
	theStaff_->actualClef_.change(NResource::nullClef_);
	pPtr_ = 0;
	barNr_ = 1;
	theStaff_->actualKeysig_.change(NResource::nullKeySig_);
	theStaff_->actualKeysig_.setClef(NResource::nullClef_);
	playPosition_ = musElementList_.first();
	lastKeySig_ = 0;
}

int NVoice::findPos(int BarNr) {
	bool found = false;
	NMusElement *elem;
	int res = 0;
	
	for (elem = musElementList_.first(); !found && elem; elem = musElementList_.next()) {
		if (elem->getType() == T_SIGN && elem->getSubType() == SIMPLE_BAR) {
			if (((NSign *) elem)->getBarNr() >= BarNr) {
				found = true;
				res = elem->getXpos();
			}
		}
	}
	return res;
}

#ifdef AAA
// experimental code follows
void NVoice::computeMidiTime(bool insertBars, bool doAutoBeam) {
#define MAXGRACENOTES 5
	int mtime = 0;
	int timeOfLastBar = 0;
	int indexOfLastBar = 0;
	int i, idx, idx0, len1, len2, countBefore, maxticks;
	int num_grace_notes = 0;
	int last_note_time = -1;
	bool chord_seen;
	bool not_grace_seen;
	QList <NMusElement> elems;
	NMusElement *elem;
	NChord *graceNotes[MAXGRACENOTES];
	NTimeSig current_timesig(0, 0);
	
	for (elem = musElementList_.first(); elem; elem = musElementList_.next()) {
		elem->midiTime_ = mtime;
		not_grace_seen = true;
		chord_seen = false;
		switch(elem->getType()) {
			case T_CHORD: if (((NChord *) elem)->properties_ & PROP_GRACE) {
				 	if (num_grace_notes < MAXGRACENOTES) {
						graceNotes[num_grace_notes++] = (NChord *) elem;
					}
					not_grace_seen = false;
				      }
				      else {
					for (i = 0; i < num_grace_notes; i++) {
						graceNotes[i]->setGraceMidiStartTime(mtime - (num_grace_notes - i) * INTERNAL_GRACE_MIDI_LENGTH);
						graceNotes[i]->midiTime_ = mtime;
					}
					num_grace_notes = 0;
				      }
				      chord_seen = true;
				      break;	
			case T_TIMESIG: current_timesig.setSignature((NTimeSig *) elem);
					break;
			case T_SIGN: if (!insertBars || !firstVoice_ ) break;
				     if ((elem->getSubType() & BAR_SYMS) == 0) break;
				     timeOfLastBar = elem->midiTime_; 
				     indexOfLastBar = musElementList_.at();
				     break;
				     
		}
		mtime += elem->getMidiLength();
		if (chord_seen && !not_grace_seen) {
			last_note_time = mtime;
		}
		else if (not_grace_seen) {
			if (last_note_time >= 0 && num_grace_notes) {
				for (i = 0; i < num_grace_notes; i++) {
					graceNotes[i]->setGraceMidiStartTime(last_note_time +  i * INTERNAL_GRACE_MIDI_LENGTH);
				}
			}
			last_note_time =  -1;
			num_grace_notes = 0;
		}
	}
	midiEndTime_ = mtime;
	if (!insertBars || firstVoice_) {if (doAutoBeam) checkBeams(indexOfLastBar, &current_timesig); return;}
	if (midiEndTime_ - timeOfLastBar <= (maxticks = MULTIPLICATOR * current_timesig.numOf128th())) {if (doAutoBeam) checkBeams(indexOfLastBar, &current_timesig); return;}
	elem = musElementList_.at(indexOfLastBar);
	while (elem && elem->midiTime_ + elem->getMidiLength() <= timeOfLastBar + maxticks) {
		elem = musElementList_.next();
	}
	if (!elem) {if (doAutoBeam) checkBeams(indexOfLastBar, &current_timesig);return;}
	idx = musElementList_.at();
	switch (elem->getType()) {
		case T_CHORD: 
		case T_REST: 
			     len2 = elem->midiTime_ + elem->getMidiLength() - (timeOfLastBar + maxticks);
			     len1 = elem->getMidiLength() - len2;
			     elems.append(elem);
			     countBefore = musElementList_.count();
			     collectAndInsertPlayable(&elems, len1, false);
			     idx0 = idx;
			     idx = musElementList_.at();
			     musElementList_.insert(idx, new NSign(main_props_, &(theStaff_->staff_props_), SIMPLE_BAR));
			     elems.append(elem);
			     collectAndInsertPlayable(&elems, len2, true);
			     break;
	}
	createUndoElement(idx0, 0, musElementList_.count() - countBefore);
	elem = musElementList_.at(indexOfLastBar);
	mtime = timeOfLastBar;
	for (; elem; elem = musElementList_.next()) {
		elem->midiTime_ = mtime;
		mtime += elem->getMidiLength();
	}
	midiEndTime_ = mtime;
	if (doAutoBeam) checkBeams(indexOfLastBar, &current_timesig);
}
#else

// computeMidiTime -- compute the midiTime_ for all notes in this voice
// plus the midiEndTime_ for this voice
// in:		insertBars: create new bar if necessary
//		doAutoBeam: try to build beam
// note:	if insertBars is true, the last element in the first voice
//		may be split to make it fit in the bar

void NVoice::computeMidiTime(bool insertBars,  bool doAutoBeam) {
#define MAXGRACENOTES 5
	int mtime = 0;
	int timeOfLastBar = 0;
	int indexOfLastBar = 0;
	int i, idx, len1, len2, maxticks;
	int num_grace_notes = 0;
	int last_note_time = -1;
	bool chord_seen = false;
	bool not_grace_seen;
	QList <NMusElement> elems;
	NMusElement *elem;
	NChord *graceNotes[MAXGRACENOTES];
	NTimeSig current_timesig(0, 0);

	for (elem = musElementList_.first(); elem; elem = musElementList_.next()) {
		elem->midiTime_ = mtime;
		not_grace_seen = true;
		switch(elem->getType()) {
			case T_CHORD: if (((NChord *) elem)->properties_ & PROP_GRACE) {
				 	if (num_grace_notes < MAXGRACENOTES) {
						graceNotes[num_grace_notes++] = (NChord *) elem;
					}
					not_grace_seen = false;
				      }
				      else {
					for (i = 0; i < num_grace_notes; i++) {
						graceNotes[i]->setGraceMidiStartTime(mtime - (num_grace_notes - i) * INTERNAL_GRACE_MIDI_LENGTH);
						graceNotes[i]->midiTime_ = mtime;
					}
					num_grace_notes = 0;
				      }
				      chord_seen = true;
				      break;	
			case T_TIMESIG: current_timesig.setSignature((NTimeSig *) elem);
					break;
			case T_SIGN: if (!insertBars || !firstVoice_ ) break;
				     if ((elem->getSubType() & BAR_SYMS) == 0) break;
				     timeOfLastBar = elem->midiTime_; 
				     indexOfLastBar = musElementList_.at();
				     break;
				     
		}
		mtime += elem->getMidiLength();
		if (chord_seen && !not_grace_seen) {
			last_note_time = mtime;
		}
		else if (not_grace_seen) {
			if (last_note_time >= 0 && num_grace_notes) {
				for (i = 0; i < num_grace_notes; i++) {
					graceNotes[i]->setGraceMidiStartTime(last_note_time +  i * INTERNAL_GRACE_MIDI_LENGTH);
				}
			}
			last_note_time =  -1;
			num_grace_notes = 0;
		}
	}
	midiEndTime_ = mtime;
	if (!current_timesig.getDenominator()) return;
	if (!insertBars || !firstVoice_) {if (doAutoBeam) checkBeams(indexOfLastBar, &current_timesig); return;}
	if (midiEndTime_ - timeOfLastBar <= (maxticks = MULTIPLICATOR * current_timesig.numOf128th())) {if (doAutoBeam) checkBeams(indexOfLastBar, &current_timesig);return;}
	// current (last) bar is overfull
	elem = musElementList_.at(indexOfLastBar);
	while (elem && elem->midiTime_ + elem->getMidiLength() <= timeOfLastBar + maxticks) {
		elem = musElementList_.next();
	}
	if (!elem) {if (doAutoBeam) checkBeams(indexOfLastBar, &current_timesig); return;}
	// an overfull bar (containing more midi time than fits in it) was found,
	// elem is pointing to the element causing that
	int countBefore = 0;
	int idx0 = 0;
	idx = musElementList_.at();
	if ((elem->getType() == T_CHORD)
	    || ((elem->getType() == T_REST) && (elem->getSubType() != MULTIREST))) {
		// split chords and normal rests in two, separated by a simple bar
		len2 = elem->midiTime_ + elem->getMidiLength() - (timeOfLastBar + maxticks);
		len1 = elem->getMidiLength() - len2;
		elems.append(elem);
		countBefore = musElementList_.count();
		collectAndInsertPlayable(elem->midiTime_, &elems, len1, false, true);
		idx0 = idx;
		idx = musElementList_.at();
		musElementList_.insert(idx, new NSign(main_props_, &(theStaff_->staff_props_), SIMPLE_BAR));
		elems.append(elem);
		collectAndInsertPlayable(elem->midiTime_ + len1, &elems, len2, true);
	} else if ((elem->getType() == T_REST) && (elem->getSubType() == MULTIREST)) {
		// behind the multirest, just append a simple bar
		countBefore = musElementList_.count();
		idx0 = idx + 1;
		idx = idx + 1;
		musElementList_.insert(idx, new NSign(main_props_, &(theStaff_->staff_props_), SIMPLE_BAR));
	}
	createUndoElement(idx0, 0, musElementList_.count() - countBefore);
	elem = musElementList_.at(indexOfLastBar);
	mtime = timeOfLastBar;
	for (; elem; elem = musElementList_.next()) {
		elem->midiTime_ = mtime;
		mtime += elem->getMidiLength();
	}
	midiEndTime_ = mtime;
	if (doAutoBeam) checkBeams(indexOfLastBar, &current_timesig);
}
#endif

int NVoice::placeAt(int xpos, int sequNr) {
	int width;
	bool isChord;
	if ( isChord = (playPosition_->getType() == T_CHORD) ) {
		((NChord *) playPosition_)->checkAcc();
	}
	playPosition_->reposit(xpos, sequNr);
	if (isChord && ((NChord *)playPosition_)->lastBeamed()) {
		((NChord *) playPosition_)->computeBeames(stemPolicy_);
	}
	if ( playPosition_->playable() && (playPosition_->playable()->properties_ & PROP_LAST_TUPLET) ) {
		playPosition_->playable()->computeTuplet();
	}
	width =  playPosition_->getBbox()->width();
	playPosition_ = musElementList_.next();
	pPtr_ = 0;
	return width;
}

void NVoice::printAll() {
	int oldidx;
	NMusElement *elem;

	oldidx = musElementList_.at();
	for (elem = musElementList_.first(); elem; elem = musElementList_.next()) {
		switch (elem->getType()) {
			case T_CHORD: printf("T_CHORD"); break;
			case T_REST: printf("T_REST"); break;
			case T_SIGN: printf("T_SIGN"); break;
			case T_CLEF: printf("T_CLEF"); break;
			case T_KEYSIG: printf("T_KEYSIG"); break;
			case T_TIMESIG: printf("T_TIMESIG"); break;
			default: printf("unknown: %d", elem->getType()); break;
		}
		printf("; midiTime_ = %d(%d)\n", elem->midiTime_ / MULTIPLICATOR, elem->midiTime_);
	}
	if (oldidx >= 0) {
		musElementList_.at(oldidx);
	}
	fflush(stdout);
}



/* --------------------------- export musixtex -------------------------------------*/

bool NVoice::testSpecialEnding(int *num) {
	*num = 1;
	if (!playPosition_) return false;
	switch (playPosition_->getType()) {
		case T_SIGN: switch (playPosition_->getSubType()) {
				case SPECIAL_ENDING1: *num = 1; return true;
				case SPECIAL_ENDING2: *num = 2; return true;
			     }
			     break;
	}
	return false;
}


/* -----------------------------references between voices -------------------------*/

void NVoice::resetSpecialElement() {
	specialElement_ = musElementList_.first();
}


void NVoice::syncSpecialElement(int xpos) {
	if (!specialElement_) return;
	if (specialElement_->getXpos() > xpos) return;
	for (;specialElement_ && specialElement_->getXpos() <= xpos; specialElement_ = musElementList_.next());
}

void NVoice::mark() {
	savePosition_ = musElementList_.at();
}

void NVoice::gotoMarkedPosition() {
	if (savePosition_ >= 0) musElementList_.at(savePosition_);
}

NMusElement *NVoice::countBarSymsBetween(int firstXpos, int actualXpos, int *count_of_bar_syms) {
	int oldidx;
	NMusElement *elem, *lastBarSym = 0;
	*count_of_bar_syms = 0;


	oldidx = musElementList_.at();
	for (elem = musElementList_.first(); elem && elem->getXpos() < firstXpos; elem = musElementList_.next()) {
		if (elem->getType() == T_SIGN && (elem->getSubType() & BAR_SYMS)) {
			lastBarSym = elem;
		}
	}
	if (!elem) {
		if (oldidx >= 0) {
			musElementList_.at(oldidx);
		}
		else {
			musElementList_.last(); musElementList_.next();
		}
		return lastBarSym;
	}
	for (;elem && elem->getXpos() <= actualXpos; elem = musElementList_.next()) {
		if (elem->getType() == T_SIGN && (elem->getSubType() & BAR_SYMS)) {
			//if (oldidx >= 0) musElementList_.at(oldidx);
			lastBarSym = elem;
			(*count_of_bar_syms)++;
		}
	}
	if (oldidx >= 0) {
		musElementList_.at(oldidx);
	}
	else {
		musElementList_.last(); musElementList_.next();
	}
	return lastBarSym;
}

// checkSpecialElement -- return the next special element between the previous
// element (as maintained in specialElement_) and xpos
// in:		xpos: stop position
// out:		volta: non-zero indicates special ending
// returns:	next special element (if any), else 0.
// note:	typically used when reading sequentially through higher voices
//		to handle the corresponding special elements stored in the
//		first voice.
// note:	should be called repeatedly, until it returns 0
// note:	specialElement_ is initialized by calling resetSpecialElement()

NMusElement *NVoice::checkSpecialElement(int xpos, int *volta) {
	NMusElement *elem;
	if (volta) *volta = 0;
	if (!specialElement_) return 0;
	if (specialElement_->getXpos() > xpos) return 0;
	while (specialElement_ && specialElement_->getXpos() <= xpos) {
		switch (specialElement_->getType()) {
			case T_SIGN: if ((specialElement_->getSubType() & BAR_SYMS)) {
				       elem = specialElement_;
				       if (volta) {
					  specialElement_ = musElementList_.next();
					  if (specialElement_ && specialElement_->getType() == T_SIGN) {
						switch (specialElement_->getSubType()) {
							case SPECIAL_ENDING1: *volta = 1; break;
							case SPECIAL_ENDING2: *volta = 2; break;
							default: return elem;
						}
					  }
				       }
				       specialElement_ = musElementList_.next();
				       return elem;
				     }
				     break;
			case T_TIMESIG:
			case T_KEYSIG:
			case T_CLEF: elem = specialElement_;
				     specialElement_ = musElementList_.next();
				     return elem;
		}
		specialElement_ = musElementList_.next();
	}
	return 0;
}

NMusElement *NVoice::findBarInStaff(int start_time, int stop_time) {
	if (!specialElement_)  return 0;
	if (specialElement_->midiTime_ >= stop_time) return 0;
	for (;specialElement_ && specialElement_->midiTime_ <= start_time; specialElement_ = musElementList_.next());
	while  (specialElement_ && specialElement_->midiTime_ < stop_time) {
		if (specialElement_->getType() == T_SIGN && (specialElement_->getSubType() & BAR_SYMS)) {
			return specialElement_;
		}
		specialElement_ = musElementList_.next();
	}
	return 0;
}

NMusElement *NVoice::findBarInStaffTillXpos(int start_time, int endXpos) {
	if (!specialElement_)  return 0;
	if (specialElement_->getXpos() >= endXpos) return 0;
	for (;specialElement_ && specialElement_->midiTime_ <= start_time; specialElement_ = musElementList_.next());
	while  (specialElement_ && specialElement_->getXpos() <= endXpos) {
		if (specialElement_->getType() == T_SIGN && (specialElement_->getSubType() & BAR_SYMS)) {
			return specialElement_;
		}
		specialElement_ = musElementList_.next();
	}
	return 0;
}


			                        
				
				     
				   
	

/*-------------------------- writing the voice  ------------------------------------*/

void NVoice::prepareForWriting() {
	theStaff_->actualKeysig_.reset();
	theStaff_->actualClef_.change(NResource::nullClef_);
	musElementList_.first();
}


NClef* NVoice::getFirstClef() {
	NMusElement *elem;

	for (elem = musElementList_.first(); elem; elem = musElementList_.next()) {
		if (elem->getType() == T_CLEF) {
			return (NClef *) elem;
		}
	}
	return NResource::nullClef_;
}

int NVoice::determineMultiRest() {
	NMusElement *elem;
	bool go_on = true;
	int oldidx;
	int multirestlen = 0;

	oldidx = musElementList_.at();
	for (elem = musElementList_.current(); elem && go_on; elem = musElementList_.next()) {
		switch(elem->getType()) {
			case T_SIGN:
				switch (elem->getSubType ()) {
					case REPEAT_OPEN:
					case REPEAT_CLOSE:
					case TEMPO_SIGNATURE: break;
					default: go_on = false;
				}
				break;
			case T_REST:
				switch (elem->getSubType ()) {
					case MULTIREST: multirestlen = ((NRest *) elem)->getMultiRestLength();
							go_on = false;
							break;
					default: go_on = false;
				}
				break;
			default: go_on = false;
		}
	}
	if (oldidx >= 0) musElementList_.at(oldidx);
	return multirestlen;
}

NTimeSig* NVoice::getFirstTimeSig() {
	NMusElement *elem;

	for (elem = musElementList_.first(); elem; elem = musElementList_.next()) {
		switch(elem->getType()) {
			case T_TIMESIG:
				return (NTimeSig *) elem;
			case T_SIGN:
				if (elem->getSubType() == SIMPLE_BAR) {
					return 0;
				}
				break;
		}
	}
	return 0;
}


NKeySig *NVoice::getFirstKeysig() {
	int i = 0;
	int oldidx;
	bool clefFound = false;
	NMusElement *elem;

	oldidx = musElementList_.at();
	for (elem = musElementList_.first(); elem; elem = musElementList_.next()) {
		i++;
		switch(elem->getType()) {
			case T_CLEF:
				clefFound = true;
				break;
			case T_KEYSIG:
				if (oldidx >= 0) musElementList_.at(oldidx);
				return (NKeySig *) elem;
			case T_SIGN:
				if (elem->getSubType() == SIMPLE_BAR) {
					if (oldidx >= 0) musElementList_.at(oldidx);
					return (clefFound ? NResource::nullKeySig_ : 0); /* if clef found, but no key sig. return no accidentals key sig. (C-major), if no clef found, return null */
				}
				break;
		}
	}
	if (oldidx >= 0) musElementList_.at(oldidx);
	return NResource::nullKeySig_;
}

NMusElement *NVoice::getCurrentPosition() {
	return musElementList_.current();
}

NMusElement *NVoice::getNextPosition() {
	return musElementList_.next();
}

NMusElement *NVoice::getPrevPosition() {
	return musElementList_.prev();
}

NMusElement *NVoice::getFirstPosition() {
	return musElementList_.first();
}

NMusElement *NVoice::getLastPosition() {
	return musElementList_.last();
}

bool NVoice::removeLastPosition() {
	return musElementList_.removeLast();
}

int NVoice::findElemRef(const NMusElement *elem) {
	return musElementList_.findRef(elem);
}

QString NVoice::determineGraceKind(property_type *properties) {
	QString graceString;
	int oldidx;
	NMusElement *elem;
	bool grace_after;
	bool stroken = false;
	bool slured = false;
	bool sixteenth = false;
	int grace_count = 0;

	
	*properties = GRACE_PMX_OK;
	elem = musElementList_.current();
	if (elem == 0 || elem->getType() != T_CHORD || !(elem->chord()->properties_ & PROP_GRACE)) {
		NResource::abort("NVoice::determineGraceKind: internal error");
	}
	oldidx = musElementList_.at();
	elem = musElementList_.prev();
	grace_after = (elem && elem->getType() == T_CHORD && !(elem->chord()->properties_ & PROP_GRACE));
	elem = musElementList_.at(oldidx);
	while (elem->getType() == T_CHORD && (elem->chord()->properties_ & PROP_GRACE)) {
		grace_count++;
		if (elem->getSubType() == INTERNAL_MARKER_OF_STROKEN_GRACE) {
			stroken = true;
		}
		else if (stroken || sixteenth) {
			*properties = WARN_MIXED_GRACES;
		}
	        else if (elem->getSubType() == NOTE16_LENGTH) {
			if (stroken) {
				*properties = WARN_MIXED_GRACES;
			}
			sixteenth = true;
		}
		if (elem->chord()->properties_ & PROP_SLURED) slured = true;
		elem = musElementList_.next();
	}
	musElementList_.at(oldidx);
	if (elem->getType() == T_CHORD) grace_after = false;
	else if (!grace_after) *properties = GRACE_PMX_ERROR;
	if (grace_count > 1) graceString.sprintf("G%d", grace_count); 
	else graceString = "G";
	if (grace_after) graceString += "A";
	if (sixteenth) graceString += "m2";
	if (stroken) graceString += "x";
	if (slured) graceString += "s";
	return graceString;
}
	
		
	
	

/*-------------------- dealing with ties -----------------------------------------*/

void NVoice::handleChordTies(NChord *chord, bool find_member) {
	NNote *note;
	QList<NNote> *noteList;

	noteList = chord->getNoteList();
	for (note = noteList->first(); note; note = noteList->next()) {
		reconnectTies(note);
		if (find_member && (note->properties & PROP_TIED)) {
			findTieMember(note);
		}
	}
}

void NVoice::findTieMember(NNote *part) {
	int oldidx;
	int oldpartidx;
	bool found = false;
	NMusElement *mus_elem;
	NNote *note, *new_part;
	QList<NNote> *noteList;

	oldidx = musElementList_.at();
	if (musElementList_.find(part->chordref) == -1) {
		NResource::abort("findTieMember: internal error", 1);
	}
	mus_elem = musElementList_.next();
	while (!found && mus_elem) {
		if (mus_elem->getType() == T_CHORD) {
			noteList = mus_elem->chord()->getNoteList();
			oldpartidx = noteList->at();
			for (note = noteList->first(); !found && note; note = noteList->next()) {
				if (note->line == part->line && note->offs == part->offs) {
					found = true;
					part->tie_forward = note;
					note->tie_backward = part;
					note->properties |= PROP_PART_OF_TIE;
				}
			}
			if (oldpartidx >= 0)  noteList->at(oldpartidx);
		}
		mus_elem = musElementList_.next();
	}
	if (!found) {
		new_part = new NNote;
		new_part->line = part->line;
		new_part->offs  = part->offs;
		new_part->tie_forward = 0;
		new_part->tie_backward = part;
		new_part->properties = PROP_VIRTUAL;
		new_part->properties &= (~PROP_TIED);
		new_part->properties |= PROP_PART_OF_TIE;
		new_part->tie_start_point_up = QPoint((int) ((float) (theStaff_->getWidth() +main_props_->tp->getLeftX()) / main_props_->zoom+5),
				 part->nbase_draw_point.y()+NResource::nbasePixmapHeight_);
		new_part->tie_start_point_down = QPoint((int) ((float) (theStaff_->getWidth() +main_props_->tp->getLeftX()) / main_props_->zoom+5),
				 part->nbase_draw_point.y());
		new_part->tie_back_point_up = QPoint((int) ((float) (theStaff_->getWidth() +main_props_->tp->getLeftX()) / main_props_->zoom+2),
				 part->nbase_draw_point.y()+NResource::nbasePixmapHeight_+4);
		new_part->tie_back_point_down = QPoint((int) ((float) (theStaff_->getWidth() +main_props_->tp->getLeftX()) / main_props_->zoom+2),
				 part->nbase_draw_point.y()+NResource::nbasePixmapHeight_-4);
		new_part->chordref = 0;
		part->tie_forward = new_part;
		virtualChord_.append(new_part);
	}
	if (oldidx >= 0) musElementList_.at(oldidx);
}

void NVoice::reconnectTies(NNote *part) {
	int oldidx;
	NNote *note, *successor;
	NMusElement *mus_elem;
	QList<NNote> *noteList;

	oldidx = musElementList_.at();
	if (musElementList_.find(part->chordref) == -1) {
		NResource::abort("reconnectTies: internal error");
	}
	for (mus_elem = musElementList_.prev(); mus_elem; mus_elem = musElementList_.prev()) {
		if (mus_elem->getType() != T_CHORD) continue;
		noteList = mus_elem->chord()->getNoteList();
		for (note = noteList->first(); note; note = noteList->next()) {
			if (note->line == part->line && note->offs == part->offs) {
				if (note->properties & PROP_TIED) {
					successor = note->tie_forward;
					note->tie_forward = part;
					part->tie_backward = note;
					part->properties |= PROP_PART_OF_TIE;
					if (successor->properties & PROP_VIRTUAL) {
						if (virtualChord_.find(successor) == -1) {
							NResource::abort("reconnectTies: problem with virtual chord");
						}
						virtualChord_.remove();
					}
					else if (successor != part) {
						successor->tie_backward = 0;
						successor->properties &= (~PROP_PART_OF_TIE);
					}
				}
				else {
					part->tie_backward = 0;
					part->properties &= (~PROP_PART_OF_TIE);
				}
				if (oldidx >= 0) musElementList_.at(oldidx);
				return;
			}
		}
	}
	part->tie_backward = 0;
	part->properties &= (~PROP_PART_OF_TIE);
	if (oldidx >= 0) musElementList_.at(oldidx);
}

void NVoice::reconnectFileReadTies(NNote *part) {
	int oldidx;
	NNote *note;

	oldidx = musElementList_.at();
	for (note = virtualChord_.first(); note; note = virtualChord_.next()) {
		if (note->properties & PROP_PART_OF_TIE) {
			if (note->line == part->line) {
				part->tie_backward = note->tie_backward;
				part->tie_backward->tie_forward = part;
				part->offs = note->offs;
				part->properties |= PROP_PART_OF_TIE;
				virtualChord_.remove();
				if (oldidx >= 0) musElementList_.at(oldidx);
				return;
			}
		}
	}
	if (oldidx >= 0) musElementList_.at(oldidx);
}

void NVoice::reconnectDeletedTies(NNote *part) {
	NNote *before;
	if (part->properties & PROP_TIED) {
		if (part->tie_forward->properties & PROP_VIRTUAL) {
			if (virtualChord_.find(part->tie_forward) != -1) {
				virtualChord_.remove();
			}
			else {
				NResource::abort("reconnectDeletedTies: error with virtual chord");
			}
		}
		else {
			part->tie_forward->tie_backward = 0;
			part->tie_forward->properties &= (~PROP_PART_OF_TIE);
		}
	}
	if (part->properties & PROP_PART_OF_TIE) {
		before = part->tie_backward;
		findTieMember(before);
	}
}

void NVoice::breakTies(NChord *chord) {
	NNote *note;
	for (note = chord->getNoteList()->first(); note; note = chord->getNoteList()->next()) {
		if (note->properties & PROP_TIED) {
			if (note->tie_forward->properties & PROP_VIRTUAL) {
				if (virtualChord_.find(note->tie_forward) != -1) {
					virtualChord_.remove();
				}
				else {
					NResource::abort("breakTies: error with virtual chord");
				}
			}
			else {
				note->tie_forward->properties &= (~PROP_PART_OF_TIE);
				note->tie_forward->tie_backward = 0;
			}
		}
		note->tie_forward = 0;
	}
}


void NVoice::reconnectTiesAtferMove(NChord *chord) {
	NNote *note;
	for (note = chord->getNoteList()->first(); note; note = chord->getNoteList()->next()) {
		if (note->properties & PROP_PART_OF_TIE) {
			findTieMember(note->tie_backward);
		}
		note->properties &= (~PROP_PART_OF_TIE);
		note->tie_backward = 0;
		reconnectTies(note);
		if (note->properties & PROP_TIED) {
			findTieMember(note);
		}
	}
}

void NVoice::reconnectCopiedTies(NChord *chord) {
	chord->calculateDimensionsAndPixmaps();
	handleChordTies(chord, true);
	chord->calculateDimensionsAndPixmaps();
}


/*--------------------------- appending due to reading ---------------------------*/


void NVoice::appendNoteAt(int line, int offs, property_type properties) {
	NNote *note;
	if( musElementList_.current()->getType() != T_CHORD )
		return;
	note = musElementList_.current()->chord()->insertNewNote(line, offs, STEM_POL_INDIVIDUAL, properties);
	if (note) {
		reconnectTies(note);
		if (properties & PROP_TIED) {
			findTieMember(note);
		}
	}
}

bool NVoice::buildTuplet(NMusElement *elem0, NMusElement *elem1, char numNotes, char playtime) {
	bool found = false;
	QList<NPlayable> *tupletlist;

	if (musElementList_.find(elem1) == -1) return false;
	if (musElementList_.find(elem0) == -1) return false;

	tupletlist = new QList<NPlayable>();
	for (;elem0  && !found; elem0 = musElementList_.next()) {
		found = elem0 == elem1;
		if (!(elem0->playable())) return false;
		tupletlist->append( elem0->playable() );
	}
	NPlayable::computeTuplet(tupletlist, numNotes, playtime);
	return true;
}

/* only called in grammar.yy the MusicXML importer requires  */
/* the ability to build beams that cross signs, except	     */
/* barsyms 						     */

bool NVoice::buildTuplet2(NMusElement *elem0, NMusElement *elem1, char numNotes, int playlength, bool dot) {
	bool found = false;
	int tupletsum = 0;
	int length;
	QList<NPlayable> *tupletlist;

	if (musElementList_.find(elem1) == -1) return false;
	if (musElementList_.find(elem0) == -1) return false;

	tupletlist = new QList<NPlayable>();
	for (;elem0  && !found; elem0 = musElementList_.next()) {
		found = elem0 == elem1;
		if (!(elem0->playable())) {
			delete tupletlist;
			return true; /* omit tuplet */
		}
		tupletsum += elem0->getSubType();
		tupletlist->append( elem0->playable() );
	}
	length = (128 / playlength * MULTIPLICATOR) / (tupletsum / numNotes);
	if (dot) {length = length * 3; length /= 2;}
	NPlayable::computeTuplet(tupletlist, numNotes, length);
	return true;
}

void NVoice::insertAtTime(unsigned int time, NMusElement *elem, bool splitPlayables) {
	NMusElement *el, *lastPlayable;
	NRest *newRest;
	NChord *newChord;
	QList<NNote> *noteList;
	NNote *note;
	int dotcount;
	int idx, lastPlayableIdx = -1;		// also a flag if *lastPlayable valid
	int len, len1, len2, restlen;
	bool lastPlayableUsed;

	computeMidiTime(false, false);
	if (time > midiEndTime_) {
		musElementList_.append(elem);
		computeMidiTime(false, false);
		return;
	}

	for (el = musElementList_.first(); el; el = musElementList_.next()) {
		if (el->midiTime_ >= (int) time && (el->getType() & PLAYABLE)) {
			if (		splitPlayables &&
					lastPlayableIdx >=0 &&				// needs to come before lastPlayable data
					!(el->playable()->properties_ & PROP_TUPLET) &&
					!(lastPlayable->playable()->properties_ & PROP_TUPLET) &&
					el->midiTime_ > (int) time &&
			    		lastPlayable->midiTime_ + lastPlayable->getMidiLength() > time) {
				len1 = time - lastPlayable->midiTime_;
				len2 = lastPlayable->getMidiLength() - len1;
				musElementList_.at(lastPlayableIdx);
				switch (lastPlayable->getType()) {
					case T_CHORD: 
						     while (len1 >= MULTIPLICATOR) {
						     	newChord = ((NChord *) lastPlayable)->clone();
						        len = quant(len1, &dotcount, DOUBLE_WHOLE_LENGTH);
						        newChord->changeLength(len);
						        newChord->setDotted(dotcount);
						        noteList = newChord->getNoteList();
						        for (note = noteList->first(); note; note = noteList->next()) {
								  note->properties |= PROP_TIED;
						        }
						        musElementList_.insert(lastPlayableIdx, newChord);
						        for (note = noteList->first(); note; note = noteList->next()) {
								reconnectTies(note);
						        }
						        for (note = noteList->first(); note; note = noteList->next()) {
								findTieMember(note);
						        }
							lastPlayableIdx++;
							len1 -= newChord->getMidiLength();
						     }
						     musElementList_.insert(lastPlayableIdx, elem);
						     lastPlayableIdx++;
						     while (len2 >= MULTIPLICATOR) {
						        len = quant(len2, &dotcount, DOUBLE_WHOLE_LENGTH);
							restlen  = len2 - (dotcount ? 3 * len / 2 : len);
							if (restlen >= MULTIPLICATOR) {
						     		newChord = ((NChord *) lastPlayable)->clone();
								lastPlayableUsed = false;
							}
							else {
								newChord = (NChord *)lastPlayable;
								lastPlayableUsed = true;
							}
						        newChord->changeLength(len);
						        newChord->setDotted(dotcount);
							if (restlen > 2) {
						           noteList = newChord->getNoteList();
						           for (note = noteList->first(); note; note = noteList->next()) {
								  note->properties |= PROP_TIED;
						           }
							   if (!lastPlayableUsed) {
						           	musElementList_.insert(lastPlayableIdx, newChord);
							   }
						           for (note = noteList->first(); note; note = noteList->next()) {
								reconnectTies(note);
						           }
						           for (note = noteList->first(); note; note = noteList->next()) {
								findTieMember(note);
						           }
							   lastPlayableIdx++;
							}
							len2 -= newChord->getMidiLength();
						     }
						     break;
					case T_REST: 
						     musElementList_.remove();
						     while (len1 >= MULTIPLICATOR) {
							len = quant(len1, &dotcount, WHOLE_LENGTH);
							newRest = new NRest(main_props_, &(theStaff_->staff_props_), &yRestOffs_, len, dotcount);
							musElementList_.insert(lastPlayableIdx, newRest);
							lastPlayableIdx++;
							len1 -= newRest->getMidiLength();
						     }
						     musElementList_.insert(lastPlayableIdx, elem);
						     lastPlayableIdx++;
						     while (len2 >= MULTIPLICATOR) {
							len = quant(len2, &dotcount, WHOLE_LENGTH);
							newRest = new NRest(main_props_, &(theStaff_->staff_props_), &yRestOffs_, len, dotcount);
							musElementList_.insert(lastPlayableIdx, newRest);
							lastPlayableIdx++;
							len2 -= newRest->getMidiLength();
						     }
						     break;
				}
			}
			else {
				idx = musElementList_.at();
				if (idx < 0) idx = 0;
				musElementList_.insert(idx, elem);
			}
			return;
		}
		if (el->getType() & PLAYABLE) {
			lastPlayable = (NRest *) el;
			lastPlayableIdx = musElementList_.at();
		}
	}
}			

void NVoice::handleEndAfterMidiImport(int difftime) {
	int len, dotcount;
	NRest *rest;

	while (difftime >= MULTIPLICATOR) {
		len = quant(difftime, &dotcount, WHOLE_LENGTH);
		rest = new NRest(main_props_, &(theStaff_->staff_props_), &yRestOffs_, len, firstVoice_ ? dotcount : (dotcount | PROP_HIDDEN));
		musElementList_.append(rest);
		difftime -= rest->getMidiLength();
	}
}

bool NVoice::insertElemAtTime(unsigned int at, NSign *sign, NMusElement *last_bar_sym) {
	int idx;

	if ((idx = findIdxOfNearestPlayableElem(last_bar_sym, at*MULTIPLICATOR)) == -1) return false;

	if (idx == -2) {
		musElementList_.append(sign);
		return true;
	}
	else {
		musElementList_.insert(idx, sign);
		musElementList_.last();
	}
	return true;
}

void NVoice::addLyrics(char *charlyrics, int verse) {
	int idx1, len1;
	int idx2, len2;
	bool found;
	NMusElement *elem;
	QString word;
	QString lyrics;

	found = false;
	elem = musElementList_.last();
	while (!found && elem) {
		if (!(found = (elem->getType() == T_SIGN && (elem->getSubType() & BAR_SYMS)))) elem = musElementList_.prev();
	}
	if (!found) elem = musElementList_.first();
	if (!elem) {
		musElementList_.last();
		return;
	}
	lyrics = QString::fromUtf8(charlyrics);
	lyrics.replace(escapedApostroph_, "");
	idx1 = wordPattern1_.match(lyrics, 0, &len1, true); 
	if ((idx2 = wordPattern2_.match(lyrics, 0, &len2, true)) != -1) {
		if (idx2 <= idx1) {
			idx1 = idx2; len1 = len2;
		}
	}
	while (idx1 >= 0 && elem) {
		word = lyrics.mid(idx1, len1);
		found = false;
		while (!found && elem) {
			if (!(found = elem->getType() == T_CHORD &&
				 !(elem->chord()->getNoteList()->first()->properties & PROP_PART_OF_TIE) && !(elem->chord()->properties_ & PROP_GRACE))) {
					elem = musElementList_.next();
			}
		}
		if (!found) {
			musElementList_.last();
			return;
		}
		if (word != "<>") {
			((NChord *) elem)->setLyrics(&word, verse);
		}
		elem = musElementList_.next();
		lyrics.remove(0, idx1 + len1);
		idx1 = wordPattern1_.match(lyrics, 0, &len1, true); 
		if ((idx2 = wordPattern2_.match(lyrics, 0, &len2, true)) != -1) {
			if (idx2 <= idx1) {
				idx1 = idx2; len1 = len2;
			}
		}
	}
}

void NVoice::copyLyricsToEditor() {
    int i;
    NChord *chord;
    NMusElement *elem;
    QString *s;
    int length;
    for( i = 0; i < NUM_LYRICS; i++ ) {
	NResource::lyrics_[i].truncate(0);
    }
    for (i = 0; i < NUM_LYRICS; i++) {
	length = 0;
	for (elem = musElementList_.first(); elem; elem = musElementList_.next()) {
		if (elem->getType() != T_CHORD) continue;
		chord = (NChord *) elem;
		if (s = chord->getLyrics(i)) {
		   NResource::lyrics_[i] += *s;
		   if ((length += s->length()) > MAX_LINE_IN_EDITOR) {
		   	NResource::lyrics_[i].append('\n');
			length = 0;
		   }
		   else {
		   	NResource::lyrics_[i].append(' ');
		   }
		}
	}
    }
}
    

void NVoice::updateLyrics() {
    NMusElement *elem;
    QString word;
    int idx1, len1;
    int idx2, len2;

    for( int i = 0; i < NUM_LYRICS; i++ ) {
	for (elem = musElementList_.first(); elem; elem = musElementList_.next()) {
	    if (elem->getType() != T_CHORD) continue;
	    ((NChord *) elem)->deleteLyrics(i);
	}
    }    

    for( int i = 0; i < NUM_LYRICS; i++ ) {
	if( !NResource::lyrics_[i].isEmpty() ) {
	    idx1 = wordPattern1_.match(NResource::lyrics_[i], 0, &len1, true);
	    if ((idx2 = wordPattern2_.match(NResource::lyrics_[i], 0, &len2, true)) != -1) {
		if (idx2 <= idx1) {
			idx1 = idx2; len1 = len2;
		}
	    }
	    elem = musElementList_.first();
	    while (elem  && idx1 >= 0) {
		if (elem->getType() != T_CHORD || (elem->chord()->getNoteList()->first()->properties & PROP_PART_OF_TIE)
			||  (elem->chord()->properties_ & PROP_GRACE)) {
			elem = musElementList_.next();
			continue;
		}
		word = NResource::lyrics_[i].mid(idx1, len1);
		((NChord *) elem)->setLyrics(&word, i);
		NResource::lyrics_[i].remove(0, idx1 + len1);
		idx1 = wordPattern1_.match(NResource::lyrics_[i], 0, &len1, true); 
		if ((idx2 = wordPattern2_.match(NResource::lyrics_[i], 0, &len2, true)) != -1) {
			if (idx2 <= idx1) {
				idx1 = idx2; len1 = len2;
			}
		}
		elem = musElementList_.next();
	    }
	}
   }
}

int NVoice::countOfLyricsLines() {
	int max = 0;
	NMusElement *elem;

	for (elem = musElementList_.first(); elem; elem = musElementList_.next()) {
		if (elem->getType() != T_CHORD) continue;
		if (((NChord *) elem)->countOfLyricsLines() > max)  max = ((NChord *) elem)->countOfLyricsLines();
	}
	return max;
}

void NVoice::collectLyrics(QString lyricslist[NUM_LYRICS]) {
	int i, length;
	NChord *chord;
	NMusElement *elem;
	QString *s;
	bool lyricsAdded[NUM_LYRICS];

	for (i = 0; i < NUM_LYRICS; i++) {
		lyricslist[i].truncate(0);
		lyricsAdded[i] = false;
	}
	for (i = 0; i < NUM_LYRICS; i++) {
		for (elem = musElementList_.first(); elem; elem = musElementList_.next()) {
			if (elem->getType() != T_CHORD) continue;
			chord = (NChord *) elem;
			if (s = chord->getLyrics(i)) {
				if (s->compare("-") == 0) {
					lyricslist[i] += ".";
					length ++;
				}
				else {
		   			lyricslist[i] += *s;
					length += s->length();
				}
				lyricsAdded[i] = true;
			}
			else {
		   		lyricslist[i].append(".");
				length += 3;
			}
			if (lyricslist[i].right(1).compare("-")) {
				lyricslist[i].append(" ");
				length++;
				if (length > MAX_LINE_IN_EDITOR) {
		   			lyricslist[i].append('\n');
					length = 0;
				}
		   	}
		}
	}
	for (i = 0; i < NUM_LYRICS; i++) {
		if (!lyricsAdded[i]) {
			lyricslist[i].truncate(0);
		}
	}
}

/*------------------------------- transposition -----------------------------------------*/

void NVoice::checkContext(int xpos) {
	NMusElement *elem;
	for (elem = musElementList_.first(); elem && elem->getXpos() < xpos; elem = musElementList_.next()) {
		switch(elem->getType()) {
		case T_CLEF: theStaff_->actualClef_.change((NClef *) elem);
			     theStaff_->actualKeysig_.setClef((NClef *) elem);
			     break;
		case T_KEYSIG: theStaff_->actualKeysig_.change((NKeySig *) elem);
				break;
		}
	}
}

void NVoice::transpose(int semitones, bool region) {
	NMusElement *elem, *elem1;
	NNote *note;
	NChord *chord;
	NRest *rest;
	int midi_pitch;
	int line, offs;
	int xpos0 = -1, xpos1 = -1;
	int idx0 = -1, idx1 = -1;
	QList<NNote> tied_notes;
	QList<NNote> part_of_tied_notes;

	theStaff_->actualClef_.change(NResource::nullClef_);
	theStaff_->actualKeysig_.change(NResource::nullKeySig_);        
	if (region) {
	   if (startElement_ && endElement_) {
		if (endElementIdx_ > startElemIdx_) {
			xpos0 = startElement_->getXpos();
			xpos1 = endElement_->getXpos();
			idx0 = startElemIdx_;
			idx1 = endElementIdx_;
		}
		else {
			xpos0 = endElement_->getXpos();
			xpos1 = startElement_->getXpos();
			idx0 = endElementIdx_;
			idx1 = startElemIdx_;
		}
		if ((elem1 = elem = musElementList_.at(idx0)) == 0) {
			NResource::abort("NVoice::transpose: internal error", 1);
		}
		theStaff_->actualClef_.change(NResource::nullClef_);
		theStaff_->actualKeysig_.change(NResource::nullKeySig_);        
		bool keySigFound = false, clefFound = false;
		for (;elem1 && !(keySigFound && clefFound) ; elem1 = musElementList_.prev()) {
			if (!clefFound && elem1->getType() == T_CLEF) {
				theStaff_->actualClef_.change((NClef *) elem1);
				clefFound = true;
			}else if (!keySigFound && elem1->getType() == T_KEYSIG) {
				theStaff_->actualKeysig_.change((NKeySig *) elem1);
				keySigFound = true;
			}
		}
		theStaff_->actualKeysig_.setClef( &theStaff_->actualClef_ );
		if ((elem = musElementList_.at(idx0)) == 0) {
			NResource::abort("NVoice::transpose: internal error", 2);
		}
		createUndoElement(idx0, idx1 - idx0 + 1, 0);
	   }
	   else {
		elem = musElementList_.first();
		createUndoElement(0, musElementList_.count(), 0);
	   }
	}
	else {
		elem = musElementList_.first();
		createUndoElement(0, musElementList_.count(), 0);
	}
        for (;elem && (idx0 <= idx1 || xpos1 == -1); elem = musElementList_.next(), idx0++) {
		if (!firstVoice_) theStaff_->checkContext(elem->getXpos());
		switch(elem->getType()) {
		case T_CHORD:
			chord = (NChord *) elem;
			for (note = chord->getNoteList()->first(); note; note = chord->getNoteList()->next()) {
				if (xpos1 != -1) {
					if (note->properties & PROP_TIED) tied_notes.append(note);
					if (note->properties & PROP_PART_OF_TIE) part_of_tied_notes.append(note);
				}
				midi_pitch = theStaff_->actualClef_.line2Midi( note->line, note->offs );
				midi_pitch += semitones;
				theStaff_->actualClef_.midi2Line(midi_pitch, &line, &offs, theStaff_->actualKeysig_.getSubType() );
				if (line <= MAXLINE && line >= MINLINE) {
					note->line = line; note->offs = offs;
					note->properties &= ~(PROP_FORCE);
				}
			}
			chord->determineStemDir(stemPolicy_);
			if (chord->lastBeamed()) {
				NChord::computeBeames(chord->getBeamList(), stemPolicy_);
			}
			chord->transposeChordDiagram(semitones);
			break;
		case T_REST:
			rest = (NRest *)elem;
			rest->transposeChordDiagram(semitones);
			break;
		case T_CLEF:
			theStaff_->actualClef_.change((NClef *) elem); 
			theStaff_->actualKeysig_.setClef( &theStaff_->actualClef_ );
			break;
		case T_KEYSIG:
			theStaff_->actualKeysig_.change((NKeySig *) elem);
			theStaff_->actualKeysig_.setClef( &theStaff_->actualClef_ );
			break;
		}
	}
	if (xpos1 == -1) return;
	for (note = tied_notes.first(); note; note = tied_notes.next()) {
		if (note->tie_forward->chordref->getXpos() < xpos1) continue;
		if (note->tie_forward->properties & PROP_VIRTUAL) {
			if (virtualChord_.find(note->tie_forward) != -1) {
				virtualChord_.remove();
			}
			else {
				NResource::abort("NVoice::transpose: internal error", 3);
			}
		}
		else {
			note->tie_forward->tie_backward = 0;
			note->tie_forward->properties &= (~PROP_PART_OF_TIE);
		}
		note->properties &= (~PROP_TIED);
	}
	for (note = part_of_tied_notes.first(); note; note = part_of_tied_notes.next()) {
		if (!(note->properties & PROP_PART_OF_TIE)) continue;
		if (note->tie_backward->chordref->getXpos() >= xpos0) continue;
		findTieMember(note->tie_backward);
		note->tie_backward = 0;
		note->properties &= (~PROP_PART_OF_TIE);
	}
}

void NVoice::combineChords(int firstIdx, int lastIdx) {
	int oldidx;
	NMusElement *elem;
	NChord *chord, *newChord;
	QList<NNote> *noteList, *noteList1, *noteList2;
	NChord *firstChord, *lastChord;
	NNote *note, *note1, *note2;
	bool first;
	bool endOfList = (lastIdx == musElementList_.count() - 1);
	int i, idx, dotcount;
	int MIDIlength = 0, newlen, len2, length = lastIdx - firstIdx + 1;
	oldidx = musElementList_.at();


	firstChord = (NChord *) musElementList_.at(firstIdx);
	lastChord = (NChord *) musElementList_.at(lastIdx);

	for (i = 0, elem = musElementList_.at(firstIdx); i < length; elem = musElementList_.next(), i++) {
		MIDIlength += elem->getMidiLength ();
	}
	noteList1 = firstChord->getNoteList();
	noteList2 = lastChord->getNoteList();
	i = 0; 
	chord = (NChord *) musElementList_.at(firstIdx);
	while (i < length) {
		if (chord->properties_ & PROP_BEAMED) {
			chord->breakBeames();
		}
		if (chord->properties_ & (PROP_SLURED | PROP_PART_OF_SLUR)) {
			chord->breakSlurConnections();
		}
		musElementList_.remove();
		i++;
	}
	first = true; idx = firstIdx;
	newlen = 0; idx = firstIdx;
	while (MIDIlength >= MULTIPLICATOR) {
		len2 = quant(MIDIlength, &dotcount, DOUBLE_WHOLE_LENGTH);
		MIDIlength -= dotcount ? 3 * len2 / 2 : len2;
		newChord = firstChord->clone();
		noteList = newChord->getNoteList();
		for (note = noteList->first(), note1 = noteList1->first(), note2 = noteList2->first(); note; 
			note = noteList->next(), note1 = noteList1->next(), note2 = noteList2->next()) {
			note->properties = 0;
			if (first) {
				SET_NOTE_PROPERTY ((note1->properties & PROP_PART_OF_TIE), note->properties, PROP_PART_OF_TIE);
			}
			else {
				note->properties |= PROP_PART_OF_TIE;
			}
			if (MIDIlength < MULTIPLICATOR) {
				SET_NOTE_PROPERTY ((note2->properties & PROP_TIED), note->properties, PROP_TIED);
			}
			else {
				note->properties |= PROP_TIED;
			}
			first = false;
		}
		newChord->properties_ = dotcount;
		newChord->setProperty(PROP_STEM_UP, (firstChord->properties_ & PROP_STEM_UP_BEFORE_BEAM));
		newChord->setProperty(PROP_STEM_UP_BEFORE_BEAM, (firstChord->properties_ & PROP_STEM_UP_BEFORE_BEAM));
		newChord->changeLength(len2);
		if (endOfList) {
			musElementList_.append(newChord);
		}
		else {
			musElementList_.insert(idx++, newChord);
		}
		newlen++;
	}
	chord = (NChord *) musElementList_.at(firstIdx);
	for (i = 0; i < newlen; chord = (NChord *) musElementList_.next(), i++) {
		handleChordTies(chord, i == newlen - 1);
	}
	delete firstChord;
        delete lastChord;
	setCountOfAddedItems(musElementList_.count());
	oldidx -= length;
	if (endOfList) {
		musElementList_.last();
	}
	else {
		musElementList_.at(oldidx);
	}
}


void NVoice::collChords() {
	NMusElement *elem;
	NChord *first = 0, *last, *chord, *chordBefore;
	QList<NMusElement> restlist;
	int firstIdx, lastIdx;
	int restlen;
	int takt;


	restlist.setAutoDelete(false);
	createUndoElement(0, musElementList_.count(), 0);
	if (!firstVoice_) {
		theStaff_->resetSpecialElement();
	}
	
	for (elem = musElementList_.first(); elem; elem = musElementList_.next()) {
		if (elem->getType() != T_CHORD || (elem->chord()->properties_ & PROP_TUPLET)) {
			if (first && firstIdx != lastIdx) {
				combineChords(firstIdx, lastIdx);
			}
			first = 0;
			continue;
		}
		if (!firstVoice_ && first) {
			if (theStaff_->findBarInStaff(chordBefore->midiTime_, elem->midiTime_ + 1)) {
				if (firstIdx != lastIdx) {
					combineChords(firstIdx, lastIdx);
				}
				first = 0;
				continue;
			}
		}

		chordBefore = chord;
		chord = (NChord *) elem;
		
		if (first) {
			if (chordBefore->equalTiedChord(chord)) {
				last = chord;
				lastIdx = musElementList_.at();
			}
			else if (firstIdx != lastIdx) {
				combineChords(firstIdx, lastIdx);
				chordBefore = first = chord;
				firstIdx = lastIdx = musElementList_.at();
			}
			else {
				chordBefore = first = chord;
				firstIdx = lastIdx = musElementList_.at();
			}
		}
		else {
			chordBefore = first = chord;
			firstIdx = lastIdx = musElementList_.at();
		}
	}
	if (first && firstIdx != lastIdx) {
		combineChords(firstIdx, lastIdx);
	}

	if (!firstVoice_) {
		theStaff_->resetSpecialElement();
	}

	restlen = 0;
	for (elem = musElementList_.first(); elem; elem = musElementList_.next()) {
		if (elem->getType() != T_REST || (elem->rest()->properties_ & PROP_TUPLET)) {
			if (restlist.count() > 1) {
				collectAndInsertPlayable(restlist.first()->midiTime_, &restlist, restlen, true);
			}
			restlist.clear();
			restlen = 0;
			continue;
		}
		if (!firstVoice_ && restlist.count()) {
			if (theStaff_->findBarInStaff(restlist.last()->midiTime_, elem->midiTime_ + 1)) {
				if (restlist.count() > 1) {
					collectAndInsertPlayable(restlist.first()->midiTime_, &restlist, restlen, true);
				}
				restlist.clear();
				restlen = 0;
				continue;
			}
		}

		restlist.append(elem);
		restlen += elem->getMidiLength();
	}
	if (restlist.count() > 1) {
		collectAndInsertPlayable(restlist.first()->midiTime_, &restlist, restlen, true);
	}
	setCountOfAddedItems(musElementList_.count());
}

/*------------------------------- clef change  -----------------------------------------*/

void NVoice::performClefChange(int type, int shift, bool region, int *dist, int *stop_x) {
	NMusElement *elem, *elem_before = 0, *elem1;
	NNote *note;
	int line;
	bool stop_working = false, chord_seen = false;
	int xpos0 = -1, xpos1 = -1;
	int idx0 = -1, idx1 = -1;
	int dd = 0;
	QList<NNote> tied_notes;
	QList<NNote> part_of_tied_notes;
	NClef clef(main_props_, &(theStaff_->staff_props_));

	if (*dist != UNDEFINED_DIST) dd = *dist;
	
	theStaff_->actualClef_.change(NResource::nullClef_);
	if (region) {
	   if (startElement_ && endElement_) {
		if (endElementIdx_ > startElemIdx_) {
			xpos0 = startElement_->getXpos();
			xpos1 = endElement_->getXpos();
			idx0 = startElemIdx_;
			idx1 = endElementIdx_;
		}
		else {
			xpos0 = endElement_->getXpos();
			xpos1 = startElement_->getXpos();
			idx0 = endElementIdx_;
			idx1 = startElemIdx_;
		}
		if ((elem1 = elem = musElementList_.at(idx0)) == 0) {
			NResource::abort("NVoice::performClefChange: internal error", 1);
		}
		theStaff_->actualClef_.change(NResource::nullClef_);
		for (;elem1; elem1 = musElementList_.prev()) {
			if (elem1->getType() == T_CLEF) {
				theStaff_->actualClef_.change((NClef *) elem1);
				break;
			}
		}
		if ((elem = musElementList_.at(idx0)) == 0) {
			NResource::abort("NVoice::performClefChange: internal error", 2);
		}
		createUndoElement(idx0, idx1 - idx0 + 1, 0);
	   }
	   else {
		elem = musElementList_.first();
		createUndoElement(0, musElementList_.count(), 0);
	   }
	}
	else {
		elem = musElementList_.first();
		createUndoElement(0, musElementList_.count(), 0);
	}
        for (;elem && (*stop_x > elem->getXpos()) && (idx0 <= idx1 || xpos1 == -1) && !stop_working; elem_before = elem, elem = musElementList_.next(), idx0++) {
		switch(elem->getType()) {
		case T_CHORD:
			chord_seen = true;
			for (note = elem->chord()->getNoteList()->first(); note; note = elem->chord()->getNoteList()->next()) {
				if (xpos1 != -1) {
					if (note->properties & PROP_TIED) tied_notes.append(note);
					if (note->properties & PROP_PART_OF_TIE) part_of_tied_notes.append(note);
				}
				line = note->line + dd;
				while (line > MAXLINE) line -= 7;
				while (line < MINLINE) line += 7;
				note->line = line; 
			}
			((NChord *) elem)->determineStemDir(stemPolicy_);
			break;
		case T_CLEF:
			if (chord_seen) {
				stop_working = true;
				if (stop_x) *stop_x = elem->getXpos();
				break;
			}
			clef = NClef(main_props_, &(theStaff_->staff_props_), type, shift);
			dd = clef.lineOfC4() - ((NClef *) elem)->lineOfC4();
			if (*dist == UNDEFINED_DIST) *dist = dd;
			((NClef *) elem)->change(&clef);
			theStaff_->actualClef_.change(&clef);
			break;
		case T_KEYSIG: ((NKeySig *) elem)->setClef(&theStaff_->actualClef_);
			break;
		}
	}
	if (xpos1 == -1) return;
	for (note = tied_notes.first(); note; note = tied_notes.next()) {
		if (note->tie_forward->chordref->getXpos() < xpos1) continue;
		if (note->tie_forward->properties & PROP_VIRTUAL) {
			if (virtualChord_.find(note->tie_forward) != -1) {
				virtualChord_.remove();
			}
			else {
				NResource::abort("NVoice::transpose: internal error", 3);
			}
		}
		else {
			note->tie_forward->tie_backward = 0;
			note->tie_forward->properties &= (~PROP_PART_OF_TIE);
		}
		note->properties &= (~PROP_TIED);
	}
	for (note = part_of_tied_notes.first(); note; note = part_of_tied_notes.next()) {
		if (!(note->properties & PROP_PART_OF_TIE)) continue;
		if (note->tie_backward->chordref->getXpos() >= xpos0) continue;
		findTieMember(note->tie_backward);
		note->tie_backward = 0;
		note->properties &= (~PROP_PART_OF_TIE);
	}
}
		

/* only called in grammar.yy and musicxmlimport.cpp          */
/* the MusicXML importer requires the ability to build beams */
/* that cross signs, except barsyms                          */
bool NVoice::buildBeam(NMusElement *elem0, NMusElement *elem1) {
	bool found = false;
	QList<NChord> *beamlist;
	NChord *chord, *prevchord = 0;

	if (musElementList_.find(elem1) == -1) return false;
	if (musElementList_.find(elem0) == -1) return false;
	beamlist = new QList<NChord>();
	while (!found && elem0 != 0) {
		found = elem0 == elem1;
		if (elem0->getType() == T_CHORD) {
			if (elem0->getSubType() >= QUARTER_LENGTH || (elem0->chord()->properties_ & PROP_BEAMED)) {
				delete beamlist;
				return false;
			}
			chord = (NChord *) elem0;
			if (prevchord != 0) {
				prevchord->setBeamParams(beamlist, chord, 0.0, 1.0);
			}
			beamlist->append(chord);
			prevchord = chord;
			elem0 = musElementList_.next();
		}
		else if (elem0->getType() == T_REST) {
			elem0 = musElementList_.next();
		}
		else if (elem0->getType() == T_SIGN) {
			// disallow barsyms
			if ((elem0->getSubType()) & BAR_SYMS) {
				//return false;
				for (chord = beamlist->first(); chord; chord = beamlist->next()) {
					chord->resetBeamFlags() ;
				}
				delete beamlist;
				return true; // omit beam
			}
			// but ignore all others
			elem0 = musElementList_.next();
		}
		else {
			delete beamlist;
			return false;
		}
	}
	chord = (NChord *) elem1;
	chord->setBeamParams(beamlist, 0, 0.0, 1.0);
	if (!found || beamlist->count() < 2) {
		delete beamlist;
		return false;
	}
	/* set PROP_STEM_UP_BEFORE_BEAM so that after beam break */
	/* the standard rules apply				 */
	for (chord = beamlist->first(); chord; chord = beamlist->next()) {
		chord->computeStemBefore();
	}
	NChord::computeBeames(beamlist, stemPolicy_);
	return true;
}

void NVoice::appendElem(int el_type, int line, int sub_type, int offs, property_type properties) {
	NMusElement *new_elem;
	NNote *note;
	NTimeSig *timesig;
	NKeySig *tmpkeysig;
	NClef *tmpclef;
	bool is_chord = false;

	switch (el_type) {
		case T_CHORD:
			is_chord = true;
			new_elem = new NChord(main_props_, &(theStaff_->staff_props_), this, line, offs, sub_type, stemPolicy_, properties);
			note = new_elem->chord()->getNoteList()->first();
			break;
		case T_REST:
			new_elem = 
			new NRest(main_props_, &(theStaff_->staff_props_), &yRestOffs_, sub_type, properties);
			break;
		case T_SIGN:
			new_elem =
			new NSign(main_props_, &(theStaff_->staff_props_), sub_type);
			break;
		case T_CLEF: tmpclef = new NClef(main_props_, &(theStaff_->staff_props_));
			     tmpclef->change(&(theStaff_->actualClef_));
			     tmpclef->setStaffProps(&(theStaff_->staff_props_));
			     new_elem = tmpclef;
			break;
		case T_KEYSIG:
			tmpkeysig = new NKeySig(main_props_, &(theStaff_->staff_props_));
			tmpkeysig->change(&(theStaff_->actualKeysig_));
			tmpkeysig->setStaffProps(&(theStaff_->staff_props_));
			tmpkeysig->setClef(&(theStaff_->actualClef_));
			tmpkeysig->reposit(0, 0);
			theStaff_->actualKeysig_ = *tmpkeysig;
			new_elem = tmpkeysig;
			break;
		case T_TIMESIG:
			timesig = new NTimeSig(main_props_, &(theStaff_->staff_props_));
			timesig->setSignature(line, sub_type);
			new_elem = timesig;
			break;
		default: NResource::abort("unknown music element");
	}
	musElementList_.append(new_elem);
	if (is_chord) {
		reconnectFileReadTies(note);
		if (properties & PROP_TIED) {
			findTieMember(note);
		}
	}
}

void NVoice::appendElem(NMusElement *elem) {
	QList<NNote> *noteList;
	NNote *note;
	NChord *chord;
	musElementList_.append(elem);
	if (elem->getType() == T_CHORD) {
		chord = (NChord *) elem;
		noteList = chord->getNoteList();
		for (note = noteList->first(); note; note = noteList->next()) {
			reconnectFileReadTies(note);
			if (note->properties & PROP_TIED) {
				findTieMember(note);
			}
		}
	}
}

int NVoice::getBarsymTimeBefore(int till_meascount, int miditime) {
	NMusElement *elem;
	int actualmiditime = 0;
	int lastbarsymtime = 0;
	for (elem =  musElementList_.first(); elem && actualmiditime <= miditime; elem =  musElementList_.next()) {
		actualmiditime += elem->getMidiLength();
		if (elem->getType() == T_SIGN && (elem->getSubType() & BAR_SYMS)) {
			lastbarsymtime = actualmiditime;
		}
	}
	while (till_meascount > 0 && elem) {
		for (;till_meascount > 0 && elem; elem = musElementList_.next()) {
			actualmiditime += elem->getMidiLength();
			if (elem->getType() == T_SIGN && (elem->getSubType() & BAR_SYMS)) {
				till_meascount--;
			}
		}
		if (!elem) {
			NResource::abort("NVoice::getBarsymTimeBefore", 1);
		}
		lastbarsymtime = actualmiditime;
	}
	return lastbarsymtime;
}


NChord *NVoice::findChordAt(NMusElement *from, int mididist) {
	bool minimum_set = false;
	int diff, mindist = (1 << 30);
	NMusElement *elem;
	NChord *nearestchord = 0;
	int actualmiditime, timeOfLastBarsym;
	bool found;


	if (!firstVoice_) {
		actualmiditime = 0;
		for (elem =  musElementList_.first(); elem && elem != from; elem =  musElementList_.next()) {
			actualmiditime += elem->getMidiLength();
		}
		timeOfLastBarsym = theStaff_->getVoiceNr(0)->getBarsymTimeBefore(0, actualmiditime);
		actualmiditime = 0;
		for (elem = musElementList_.first(); elem; elem = musElementList_.next()) {
			if (actualmiditime >= timeOfLastBarsym) break;
			actualmiditime += elem->getMidiLength();
		}
		if (from) {
			found = false;
			for (; elem; elem = musElementList_.next()) {
				actualmiditime += elem->getMidiLength();
				if (found) break;
				if (elem == from) found = true;
			}
		}
		if (!elem) return 0;
		if (timeOfLastBarsym == actualmiditime) {
			actualmiditime = 0;
		}
		else {
			actualmiditime = elem->getMidiLength();
		}
	}
	else {
		if (from == 0) {
			if ((elem = musElementList_.first()) == 0) return 0;
		}
		else {
			if (musElementList_.find(from) == -1) return 0;
		}
		actualmiditime = 0;
		elem = musElementList_.next();
	}
	found = false;
	while (!found && elem)  {
		if (elem->getType() == T_CHORD && !(elem->chord()->properties_ & PROP_GRACE )) {
			diff = actualmiditime - mididist;
			if (diff < 0) diff = -diff;
			if (mindist > diff) {
				mindist = diff;
				minimum_set = true;
				nearestchord = (NChord *) elem;
			}
			else if (minimum_set) {
				found = true;
			}
		}
		actualmiditime += elem->getMidiLength();
		elem = musElementList_.next();
	}
	return nearestchord;
}

NChord* NVoice::findChordWithVAEndMarker(NChord *from) {
	int oldidx;
	NMusElement *elem;
	NChord *chord;

	oldidx = musElementList_.at();
	if (musElementList_.find(from) < 0) {
		if (oldidx >= 0) musElementList_.at(oldidx);
		return 0;
	}

	for(elem = musElementList_.current(); elem; elem =  musElementList_.next()) {
		if (elem->getType() != T_CHORD) continue;
		chord = (NChord *) elem;
		if (!(chord->va_ & 0x00020000)) continue;
		return chord;
	}
	return 0;
}

NChord *NVoice::findChordInMeasureAt(int refpoint, NMusElement *from, int till_meascount, int mididist) {
	bool found = false, minimum_set = false;
	int diff, mindist = (1 << 30);
	NMusElement *elem;
	NChord *nearestchord = 0;
	int actualmiditime, timeOfLastBarsym;
	int idx_of_last_bar_sym;


	if (!firstVoice_) {
		actualmiditime = 0;
		for (elem =  musElementList_.first(); elem && elem != from; elem =  musElementList_.next()) {
			actualmiditime += elem->getMidiLength();
		}
		timeOfLastBarsym = theStaff_->getVoiceNr(0)->getBarsymTimeBefore(till_meascount, actualmiditime);
		actualmiditime = 0;
		for (elem = musElementList_.first(); elem; elem = musElementList_.next()) {
			actualmiditime += elem->getMidiLength();
			if (actualmiditime > timeOfLastBarsym) break;
		}
		if (!elem) return 0;
		actualmiditime = 0;
	}
	else {
		if (from == 0) {
			if ((elem = musElementList_.first()) == 0) return 0;
			actualmiditime = 0;
		}
		else {
			actualmiditime = 0;
			timeOfLastBarsym = 0;
			idx_of_last_bar_sym = -1;
			for (elem = musElementList_.first(); elem && elem != from; elem = musElementList_.next()) {
				actualmiditime += elem->getMidiLength();
				if (elem->getType() == T_SIGN && (elem->getSubType() & BAR_SYMS)) {
					timeOfLastBarsym = actualmiditime;
					idx_of_last_bar_sym = musElementList_.at();
				}
			}
			if (elem == 0) {
				return 0;
			}
			if (idx_of_last_bar_sym < 0) {
				if ((elem = musElementList_.first()) == 0) return 0;
			}
			else {
				elem = musElementList_.at(idx_of_last_bar_sym);
			}
			actualmiditime = 0;
		}
		if (till_meascount > 0) {
			for (elem = musElementList_.next();till_meascount > 0 && elem; elem = musElementList_.next()) {
				if (elem->getType() == T_SIGN && (elem->getSubType() & BAR_SYMS)) {
					till_meascount--;
				}
			}
			actualmiditime = 0;
		}
		if (!elem) {
			NResource::abort("NVoice::findChordInMeasureAt", 1);
		}

	}
			
	while (!found && elem)  {
		if (elem->getType() == T_CHORD && !(elem->chord()->properties_ & PROP_GRACE )) {
			diff = actualmiditime - mididist;
			if (diff < 0) diff = -diff;
			if (mindist > diff) {
				mindist = diff;
				minimum_set = true;
				nearestchord = (NChord *) elem;

			}
			else if (minimum_set) {
				found = true;
			}
		}
		actualmiditime += elem->getMidiLength();
		elem = musElementList_.next();
	}
	if (!found) {
		for (elem = musElementList_.last(); !found && elem && elem->getXpos() > refpoint; elem = musElementList_.prev()) {
			if (elem->getType() == T_CHORD && !(elem->chord()->properties_ & PROP_GRACE )) {
				found = true;
				nearestchord = (NChord *) elem;
			}
		}
	}
	return nearestchord;
}

int NVoice::findIdxOfNearestPlayableElem(NMusElement *from, int mididist) {
	NMusElement *elem;
	bool found = false, minimum_set = false;
	int diff, mindist = (1 << 30);
	int actualmiditime;
	int idx, minidx = -1;

	if (from == 0) {
		if ((elem = musElementList_.first()) == 0) return -1;
		actualmiditime = 0;
	}
	else {
		if (musElementList_.find(from) == -1) return -1;
		actualmiditime = from->getMidiLength();
	}
	elem = musElementList_.next();
	while (!found && elem)  {
		if (!(elem->getType() & PLAYABLE)) {elem = musElementList_.next(); continue;}
		diff = actualmiditime - mididist;
		if (diff < 0) diff = -diff;
		if (mindist > diff) {
			idx = musElementList_.at();
			if (idx >= 0) {
				mindist = diff;
				minimum_set = true;
				minidx = idx;
			}
		}
		else if (mindist != diff && minimum_set) {
			found = true;
		}
		actualmiditime += elem->getMidiLength();
		elem = musElementList_.next();
	}
	if (!found) {
		diff = actualmiditime - mididist;
		if (diff < 0) diff = -diff;
		if (mindist > diff) {
			return -2;
		}
	}
	return minidx;
}
int NVoice::findIdxOfNearestElem(NMusElement *from, int mididist) {
	NMusElement *elem;
	bool found = false, minimum_set = false;
	int diff, mindist = (1 << 30);
	int actualmiditime;
	int idx, minidx = -1;

	if (from == 0) {
		if ((elem = musElementList_.first()) == 0) return -1;
		actualmiditime = 0;
	}
	else {
		if (musElementList_.find(from) == -1) return -1;
		actualmiditime = from->getMidiLength();
	}
	elem = musElementList_.next();
	while (!found && elem)  {
		diff = actualmiditime - mididist;
		if (diff < 0) diff = -diff;
		if (mindist > diff) {
			idx = musElementList_.at();
			if (idx >= 0) {
				mindist = diff;
				minimum_set = true;
				minidx = idx;
			}
		}
		else if (mindist != diff && minimum_set) {
			found = true;
		}
		actualmiditime += elem->getMidiLength();
		elem = musElementList_.next();
	}
	if (!found) {
		diff = actualmiditime - mididist;
		if (diff < 0) diff = -diff;
		if (mindist > diff) {
			return -2;
		}
	}
	return minidx;
}

NMusElement *NVoice::findChordOrRestAt(NMusElement *from, int mididist) {
	bool found = false, minimum_set = false;
	int diff, mindist = (1 << 30);
	NMusElement *elem;
	NMusElement *nearestelem = 0;
	int actualmiditime = 0;

	if (from == 0) {
		if (musElementList_.first() == 0) return 0;
	}
	else {
		if (musElementList_.find(from) == -1) return 0;
		actualmiditime += from->getMidiLength();
	}
	elem = musElementList_.next();
	while (!found && elem)  {
		if (elem->getType() == T_REST || elem->getType() == T_CHORD && !(elem->playable()->properties_ & PROP_GRACE)) {
			diff = actualmiditime - mididist;
			if (diff < 0) diff = -diff;
			if (mindist > diff) {
				mindist = diff;
				minimum_set = true;
				nearestelem =  elem;
			}
			else if (minimum_set) {
				found = true;
			}
		}
		actualmiditime += elem->getMidiLength();
		elem = musElementList_.next();
	}
	return nearestelem;
}

void NVoice::correctReadTrillsSlursAndDynamicsStringsAndVAs() {
	NMusElement *elem;
	NChord *chord1, *chord2;
	NText *text;
	int idx;
	int dest_time;
	int pos1;
	int sign = 1;
	int xpos1, xpos2;
	int dist;
	unsigned int till_meascount;
	char *err = "correctReadTrillsSlursAndDynamicsStringsAndVAs: internal error";
	for (elem = musElementList_.first(); elem; elem = musElementList_.next()) {
		if (elem->getType() != T_CHORD) continue;
		chord1 = (NChord *) elem;
		if (chord1->trill_) {
			dist = MULTIPLICATOR * (chord1->trill_ & 0x7fff);
			till_meascount = (chord1->trill_ >> 16);
			sign = (chord1->trill_ & 0x8000) ? -1 : 1;
			pos1 = musElementList_.at();
			xpos1 = chord1->getXpos();
			chord2 = findChordInMeasureAt(chord1->getXpos(), chord1, till_meascount, dist);
			if (chord2 == 0) {
				NResource::abort(err, 1);
			}
			xpos2 = chord2->getXpos();
			if (xpos2 < xpos1) xpos2 = xpos1;
			chord1->trill_ = sign * ((xpos2 - xpos1) / NResource::trillPixmap_->width() + 1);
			musElementList_.at(pos1);
		}
		if (chord1->va_) {
			sign = (chord1->va_ & 0x8000) ? -1 : 1;
			pos1 = musElementList_.at();
			xpos1 = chord1->getXpos();
			if (chord1->va_ & 0x00010000) {
				if (chord1->va_ & 0x00020000) {
					chord1->va_ = 0x0;
					chord2 = 0;
				}
				else {
					chord2 = findChordWithVAEndMarker(chord1);
					if (chord2 == 0) {
						chord1->va_ = 0;
					}
					if (chord2 == chord1) {
						chord1->va_ = 0;
					}
				}
			}
			else {
				dist = MULTIPLICATOR * (chord1->va_ & 0x7fff);
				till_meascount = (chord1->va_ >> 17);
				chord2 = findChordInMeasureAt(chord1->getXpos(), chord1, till_meascount, dist);
				if (chord2 == 0) {
					NResource::abort(err, 2);
				}
			}
			if (chord2) {
				xpos2 = chord2->getXpos();
				if (xpos2 < xpos1) xpos2 = xpos1;
				if (sign > 0) {
					chord1->va_ = (xpos2 - xpos1 - VA_LINE_DASH_LEN) / VA_LINE_LEN + 1;
				}
				else {
					chord1->va_ = -((xpos2 - xpos1 - VA_LINE_DASH_LEN) / VA_LINE_LEN);
				}
				musElementList_.at(pos1);
			}
		}
		if (chord1->dynamic_) {
			dist = MULTIPLICATOR * (chord1->dynamic_ & 0xffff);
			if (dist < 0) {
				NResource::abort(err, 3);
			}
			till_meascount = (chord1->dynamic_ >> 16);
			pos1 = musElementList_.at();
			xpos1 = chord1->getXpos();
			chord2 = findChordInMeasureAt(chord1->getXpos(), chord1, till_meascount, dist);
			if (chord2 == 0) {
				NResource::abort(err, 4);
			}
			xpos2 = chord2->getBbox()->right();
			chord1->dynamic_ = xpos2 - xpos1;
			musElementList_.at(pos1);
		}
		if (chord1->auxInfo_.provSlur_) {
			dist = MULTIPLICATOR * (chord1->auxInfo_.provSlur_ & 0xffff);
			till_meascount = (chord1->auxInfo_.provSlur_ >> 16);
			pos1 = musElementList_.at();
			chord2 = findChordInMeasureAt(chord1->getXpos(), chord1, till_meascount, dist);
			if (chord2 == 0) {
				NResource::abort(err, 5);
			}
			chord1->setSlured(true, chord2);
			musElementList_.at(pos1);
		}
	}
	text = provStrings_.first();
	while (text) {
		if (text->barSym_) {
			dest_time = text->barSym_->midiTime_ + text->destinationTime_;
		}
		else {
			dest_time = text->destinationTime_;
		}
		for (elem = musElementList_.first(); elem && (!(elem->getType() & PLAYABLE) || elem->midiTime_ < dest_time); elem = musElementList_.next());
		if (elem) {
			idx = musElementList_.at();
			musElementList_.insert(idx, text);
			text->midiTime_ = elem->midiTime_;
		}
		else {
			musElementList_.append(text);
			text->midiTime_ = dest_time;
		}
		provStrings_.remove();
		text = provStrings_.first();
	}
		

}

bool NVoice::setProvisionalTrill(int kind, unsigned int at, unsigned int till_meascount, unsigned int till, NMusElement *last_bar_sym) {
	NChord *chord;

	if ((chord = findChordAt(last_bar_sym, at*MULTIPLICATOR)) == 0) return false;

	if (till - at < 3 && till_meascount < 1) {
		chord->trill_ = (kind != NORMAL_TRILL) ? (1 | 0x8000) : 1;
		return true;
	}
	
	chord->trill_ = till;
	if (kind != NORMAL_TRILL) {
		chord->trill_ |= 0x8000;
	}
	chord->trill_ |= (till_meascount << 16);
	return true;
}

void NVoice::setProvisionalString(char *text, int type, unsigned int at, NMusElement *last_bar_sym) {
	NText *textElem = new NText(main_props_, theStaff_->getStaffPropsAddr(), text, type == 0 ? TEXT_UPTEXT : TEXT_DOWNTEXT);
	textElem->destinationTime_ = MULTIPLICATOR * (at - 1);
	textElem->barSym_ = last_bar_sym;
	provStrings_.append(textElem);
}

bool NVoice::setProvisionalOctaviation(int kind, unsigned int at, unsigned int till_meascount, unsigned int till, NMusElement *last_bar_sym) {
	NChord *chord;

	if ((chord = findChordAt(last_bar_sym, at*MULTIPLICATOR)) == 0) return false;

	if (till - at < 3 && till_meascount < 1) {
		chord->va_ = (kind != OCTAVIATION1P) ? (1 | 0x8000) : 1;
		return true;
	}
	
	chord->va_ = till;
	if (kind != OCTAVIATION1P) {
		chord->va_ |= 0x8000;
	}
	chord->va_ |= (till_meascount << 17);
	return true;
}

bool NVoice::setProvisionalSlur(unsigned int at, unsigned int till_meascount, unsigned int till, NMusElement *last_bar_sym) {
	NChord *chord;

	
	if ((chord = findChordAt(last_bar_sym, at*MULTIPLICATOR)) == 0) return false;

	if (till - at < 3 && till_meascount < 1) {
		chord->auxInfo_.provSlur_ = 1;
	}
	
	chord->auxInfo_.provSlur_ = till;
	chord->auxInfo_.provSlur_ |= (till_meascount << 16);
	return true;
}

bool NVoice::setProvisionalDynamic(int kind, unsigned int at, unsigned int till_meascount, unsigned int till, NMusElement *last_bar_sym) {
	NChord *chord;

	if ((chord = findChordAt(last_bar_sym, at*MULTIPLICATOR)) == 0) return false;
	
	chord->dynamicAlign_ = kind == DYN_DECRESCENDO;
		
	chord->dynamic_ = till;
	chord->dynamic_ |= (till_meascount << 16);
	return true;
}

bool NVoice::setReadArpeggio(unsigned int at, NMusElement *last_bar_sym) {
	NChord *chord;

	if ((chord = findChordAt(last_bar_sym, at*MULTIPLICATOR)) == 0) return false;
	
	chord->setArpeggio(true);
	return true;
}

bool NVoice::setReadPedalOn(unsigned int at, NMusElement *last_bar_sym) {
	NChord *chord;

	if ((chord = findChordAt(last_bar_sym, at*MULTIPLICATOR)) == 0) return false;
	
	chord->setPedalOn(true);
	return true;
}

bool NVoice::setReadPedalOff(unsigned int at, NMusElement *last_bar_sym) {
	NChord *chord;

	if ((chord = findChordAt(last_bar_sym, at*MULTIPLICATOR)) == 0) return false;
	
	chord->setPedalOff(true);
	return true;
}

bool NVoice::insertChordDiagrammAt(unsigned int at, NChordDiagram *diag, NMusElement *last_bar_sym) {
	NMusElement *elem;

	if ((elem = findChordOrRestAt(last_bar_sym, at*MULTIPLICATOR)) == 0) return false;
		
	elem->playable()->addChordDiagram(diag);
	return true;
}

bool NVoice::insertSegnoRitardAndAccelAt(unsigned int at, int type, NMusElement *last_bar_sym) {
	NSign *sign;
	int idx;
	if ((idx = findIdxOfNearestElem(last_bar_sym, at*MULTIPLICATOR)) == -1) return false;
	
	sign = new NSign(main_props_, &(theStaff_->staff_props_), type);
	if (idx == -2) {
		musElementList_.append(sign);
	}
	else {
		musElementList_.insert(idx, sign);
		musElementList_.last();
	}
	return true;
}

void NVoice::setHalfsAccordingKeySig(bool withUndo) {
	NMusElement *elem;
	NKeySig *keysig = NResource::nullKeySig_;
	NNote *note;
	NClef *clef;
	QList<NNote> *noteList;

	if (withUndo) {
		createUndoElement(0, musElementList_.count(), 0);
	}

	for (elem =  musElementList_.first(); elem; elem = musElementList_.next()) {
		if (!firstVoice_) theStaff_->checkContext(elem->getXpos());
		switch (elem->getType()) {
			case T_CLEF: clef = (NClef*) elem; break;
			case T_KEYSIG: keysig = (NKeySig *) elem;
				       keysig->setClef(clef);
				     break;
			case T_CHORD: if (!keysig) break;
				     noteList = elem->chord()->getNoteList();			
				     for (note = noteList->first(); note; note = noteList->next()) {
					keysig->changeHalfTone(note);
					note->properties &= (~PROP_FORCE);
				     }
				     break;
		}
	}
}

void NVoice::setHalfsTo(int type, bool region) {
	NMusElement *elem;
	NNote *note;
	QList<NNote> *noteList;
	int xpos0 = -1, xpos1 = -1;
	int idx0 = -1, idx1 = -1;

        if (region) {
	   if (startElement_ && endElement_) {
		if (endElementIdx_ > startElemIdx_) {
			xpos0 = startElement_->getXpos();
			xpos1 = endElement_->getXpos();
			idx0 = startElemIdx_;
			idx1 = endElementIdx_;
		}
		else {
			xpos0 = endElement_->getXpos();
			xpos1 = startElement_->getXpos();
			idx0 = endElementIdx_;
			idx1 = startElemIdx_;
		}
		if ((elem = musElementList_.at(idx0)) == 0) {
			 NResource::abort(" NVoice::setHalfsTo: internal error", 1);
		}
		createUndoElement(idx0, idx1 - idx0 + 1, 0);
	   }
	   else {
		elem = musElementList_.first();
		createUndoElement(0, musElementList_.count(), 0);
	   }
	}
	else {
		elem = musElementList_.first();
		createUndoElement(0, musElementList_.count(), 0);
	}
        for (;elem && (idx0 <= idx1 || xpos1 == -1); elem = musElementList_.next(), idx0++) {
		switch (elem->getType()) {
			case T_CHORD: noteList = elem->chord()->getNoteList();			
				     for (note = noteList->first(); note; note = noteList->next()) {
					if (note->offs == 1 && type == PROP_FLAT) {
						note->line++; note->offs = -1;
					}
					else if (note->offs == -1 && type == PROP_CROSS) {
						note->line--; note->offs = 1;
					}
				     }
				     break;
		}
	}
}
	

/*---------------------------------- undo ---------------------------------------*/


void NVoice::pasteAtIndex(QList<NMusElement> *clipBoard, int idx) {
	int oldidx;
	NMusElement *ac_elem;
	NChord *chord;
	bool lastelem = (idx >= (int) musElementList_.count());

	oldidx = musElementList_.at();
	if (currentElement_)  {
		currentElement_->setActual(false);
		currentElement_->draw();
		currentElement_ = 0;
	}
	for (ac_elem = clipBoard->first(); ac_elem; ac_elem = clipBoard->next()) {
		ac_elem->setStaffProps(&(theStaff_->staff_props_));
		ac_elem->setActual(false);
		currentElement_ = ac_elem;
		if (lastelem) {
			musElementList_.append(ac_elem);
		}
		else {
			musElementList_.insert(idx, ac_elem);
			idx++;
		}
		switch (ac_elem->getType()) {
			case T_KEYSIG: ((NKeySig *) ac_elem)->setClef(&(theStaff_->actualClef_));
					break;
			case T_CHORD: chord = (NChord *) ac_elem;
				     reconnectCopiedTies(chord);
			case T_REST:
				     if (ac_elem->playable()->properties_ & PROP_LAST_TUPLET) {
						reconnectTuplets();
				     }
				     break;
		}
	}
	if (oldidx >= 0) musElementList_.at(oldidx);
}	

void NVoice::deleteRange(int startpos, int numelements, int newitems, int reason) {
	int oldidx;
	NNote *note;
	QList<NNote> *noteList;
	NMusElement *ac_elem;
	NChord *chord;

	oldidx = musElementList_.at();
	if (currentElement_) {
		currentElement_->setActual(false);
	}
	createUndoElement(startpos, numelements, newitems, reason);
	if (numelements) {
		ac_elem = musElementList_.at(startpos);
		currentElement_ = 0;
	}
	
	while (numelements-- && ac_elem) {
		if (ac_elem->getType() == T_CHORD) {
			chord = (NChord *) ac_elem;
			chord->checkSlures();
			musElementList_.remove();
			noteList = chord->getNoteList();
			for (note = noteList->first(); note; note = noteList->next()) {
				reconnectDeletedTies(note);
			}
		}
		else {
			musElementList_.remove();
			ac_elem = musElementList_.current();
		}
		ac_elem = musElementList_.current();
	}
	if (oldidx >= 0) musElementList_.at(oldidx);
}

NVoice *NVoice::undoPossible() {
	if (undocounter_  < 1) return 0;
	return undoelem_[(undoptr_+MAXUNDO - 1) % MAXUNDO].ref;
}

NVoice *NVoice::redoPossible() {
	if (redocounter_  < 1) return 0;
	return redoelem_[(redoptr_+MAXUNDO - 1) % MAXUNDO].ref;
}



QList<NMusElement> *NVoice::cloneGroup(int firstidx, int lastidx) {
	bool found = false;
	NMusElement *elem, *lastelem, *cloneelem;
	NChord *slurpartner = 0, *chord, *slured_chord;
	QList<NChord> *clonebeamlist = 0;
	QList<NMusElement> *clonelist;
	char *err = "cloneGroup: internal error";

	if (lastidx < firstidx) return 0;
	clonelist = new QList<NMusElement>();
	lastelem = musElementList_.at(lastidx);
	elem = musElementList_.at(firstidx);
	if (elem == 0 || lastelem == 0) {
		NResource::abort( err, 1);
	}
	while (!found && elem) {
		clonelist->append(cloneelem = elem->clone());
		if (elem->getType() == T_CHORD) {
			chord = (NChord *) elem;
			if (chord == slurpartner) {
				if (slurpartner == 0) {
					NResource::abort( err, 2);
				}
				slured_chord->setSlured(true, (NChord *) cloneelem);
				slurpartner = 0;
			}
			if (chord->properties_ & PROP_SLURED) {
				slured_chord = (NChord *) cloneelem;
				slurpartner = chord->getSlurPartner();
			}
			if (chord->properties_ & PROP_BEAMED) {
				if (clonebeamlist == 0) {
					clonebeamlist = new QList<NChord>();
				}
				clonebeamlist->append((NChord *) cloneelem);
				if (chord->lastBeamed()) {
					NChord::computeBeames(clonebeamlist, stemPolicy_);
					clonebeamlist = 0;
				}
			}
		}
		found = elem == lastelem;
		elem = musElementList_.next();
	}
	if (!found) {
		NResource::abort( err, 3);
	}
	return clonelist;
}

void NVoice::undo() {
	int newitems;
	if (currentElement_) {
		currentElement_->setActual(false);
		currentElement_ = 0;
	}
	undoptr_ = (undoptr_ + MAXUNDO - 1) % MAXUNDO;
	undocounter_--;
	newitems = -undoelem_[undoptr_].num_of_replaced_items;
	if (undoelem_[undoptr_].backup_area != 0) {
		newitems += undoelem_[undoptr_].backup_area->count();
	}
	deleteRange(undoelem_[undoptr_].first_replaced_item, undoelem_[undoptr_].num_of_replaced_items, newitems, REASON_UNDO_DONE);
	if (undoelem_[undoptr_].backup_area != 0) {
		pasteAtIndex(undoelem_[undoptr_].backup_area, undoelem_[undoptr_].first_replaced_item);
	}
}

void NVoice::redo() {
	int newitems;
	if (currentElement_) {
		currentElement_->setActual(false);
		currentElement_ = 0;
	}
	redoptr_ = (redoptr_ + MAXUNDO - 1) % MAXUNDO;
	redocounter_--;
	newitems = -redoelem_[redoptr_].num_of_replaced_items;
	if (redoelem_[redoptr_].backup_area != 0) {
		newitems += redoelem_[redoptr_].backup_area->count();
	}
	deleteRange(redoelem_[redoptr_].first_replaced_item, redoelem_[redoptr_].num_of_replaced_items, newitems, REASON_REDO_DONE);
	if (redoelem_[redoptr_].backup_area != 0) {
		pasteAtIndex(redoelem_[redoptr_].backup_area, redoelem_[redoptr_].first_replaced_item);
	}
}

void NVoice::createUndoElement(NMusElement *startElement, int length, int count_of_added_items, int reason) {
	int idx, oldidx;
	oldidx = musElementList_.at();
	if ((idx = musElementList_.find(startElement)) == -1) {
		NResource::abort("createUndoElement(a): internal error");
	}
	createUndoElement(idx, length, count_of_added_items, reason);
	if (oldidx >= 0) musElementList_.at(oldidx);
}

void NVoice::createUndoElement(int startpos, int length, int count_of_added_items, int reason) {
	int oldidx;
	int oldidx1, minidx, maxidx, elemidx;
	int firstidx, lastidx;
	bool limits_changed;
	NMusElement *elem;
	NChord *chord;
	QList<NChord> *beamlist;
	QList<NPlayable> *tupletlist;
	minidx = startpos;
	maxidx = minidx + length - 1;
	char *err = "createUndoElement:: internal error";

	oldidx = musElementList_.at();
	if (length) {
	  do {
		limits_changed = false;
		elem = musElementList_.at(minidx);
		for (;length && elem; elem = musElementList_.next(), length--) {
			elemidx = musElementList_.at();
			property_type properties_ = elem->playable() ? elem->playable()->properties_ : 0;
			if ( properties_ & PROP_TUPLET) {
				oldidx1 = musElementList_.at();
				if (oldidx1 < 0) {
					NResource::abort(err, 1);
				}
				tupletlist = elem->playable()->getTupletList();
				firstidx = musElementList_.find(tupletlist->first());
				lastidx = musElementList_.find(tupletlist->last());
				if (firstidx < 0 || lastidx < 0) {
					NResource::abort(err, 2);
				}
				if (firstidx > elemidx) {
					NResource::abort(err, 3);
				}
				if (lastidx < elemidx) {
					NResource::abort(err, 4);
				}
				if (firstidx < minidx) {minidx = firstidx; limits_changed = true;}
				if (lastidx > maxidx) {maxidx = lastidx; limits_changed = true;}
				musElementList_.at(oldidx1);
			}
			if (properties_ & PROP_BEAMED) {
				oldidx1 = musElementList_.at();
				if (oldidx1 < 0) {
					NResource::abort(err, 5);
				}
				chord = (NChord *) elem;
				beamlist = chord->getBeamList();
				firstidx = musElementList_.find(beamlist->first());
				lastidx = musElementList_.find(beamlist->last());
				if (firstidx < 0 || lastidx < 0) {
					NResource::abort(err, 6);
				}
				if (firstidx > elemidx) {
					NResource::abort(err, 7);
				}
				if (lastidx < elemidx) {
					NResource::abort(err, 8);
				}
				if (firstidx < minidx) {minidx = firstidx; limits_changed = true;}
				if (lastidx > maxidx) {maxidx = lastidx; limits_changed = true;}
				musElementList_.at(oldidx1);
			}
			if (properties_ & PROP_SLURED) {
				oldidx1 = musElementList_.at();
				if (oldidx1 < 0) {
					NResource::abort(err, 9);
				}
				chord = (NChord *) elem;
				firstidx = musElementList_.find(chord);
				lastidx = musElementList_.find(chord->getSlurPartner());
				if (firstidx < 0 || lastidx < 0) {
					NResource::abort(err, 10);
				}
				if (firstidx > elemidx) {
					NResource::abort(err, 11);
				}
				if (lastidx < elemidx) {
					NResource::abort(err, 12);
				}
				if (firstidx < minidx) {minidx = firstidx; limits_changed = true;}
				if (lastidx > maxidx) {maxidx = lastidx; limits_changed = true;}
				musElementList_.at(oldidx1);
			}
			if (properties_ & PROP_PART_OF_SLUR) {
				oldidx1 = musElementList_.at();
				if (oldidx1 < 0) {
					NResource::abort(err, 9);
				}
				chord = (NChord *) elem;
				lastidx = musElementList_.find(chord);
				firstidx = musElementList_.find(chord->getSlurStart());
				if (firstidx < 0 || lastidx < 0) {
					NResource::abort(err, 10);
				}
				if (firstidx > elemidx) {
					NResource::abort(err, 11);
				}
				if (lastidx < elemidx) {
					NResource::abort(err, 12);
				}
				if (firstidx < minidx) {minidx = firstidx; limits_changed = true;}
				if (lastidx > maxidx) {maxidx = lastidx; limits_changed = true;}
				musElementList_.at(oldidx1);
			}
		}
		if (limits_changed) {
			length = lastidx - firstidx + 1;
		}
	   }
	   while (limits_changed);
	}
	if (length) {
		NResource::abort( err, 13);
	}
	if (reason == REASON_UNDO_DONE) {
		if (redocounter_ == MAXUNDO) {
			freeCloneGroup(redoelem_[redoptr_].backup_area);
		}
		redoelem_[redoptr_].backup_area = cloneGroup(minidx, maxidx);
		redoelem_[redoptr_].first_replaced_item = minidx;
		redoelem_[redoptr_].num_of_replaced_items = maxidx - minidx + 1 + count_of_added_items;
		redoelem_[redoptr_].ref = this;
		redoptr_ = (redoptr_ + 1) % MAXUNDO;
		if (++redocounter_  > MAXUNDO) redocounter_ = MAXUNDO;
	}
	else {
		if (reason == REASON_UNDO && redocounter_) {
			invalidateReUndo();
		}
		if (undocounter_ == MAXUNDO) {
			freeCloneGroup(undoelem_[undoptr_].backup_area);
		}
		undoelem_[undoptr_].backup_area = cloneGroup(minidx, maxidx);
		undoelem_[undoptr_].first_replaced_item = minidx;
		undoelem_[undoptr_].num_of_replaced_items = maxidx - minidx + 1 + count_of_added_items;
		undoelem_[undoptr_].ref = this;
		lastundoptr_ = undoptr_;
		undoptr_ = (undoptr_ + 1) % MAXUNDO;
		if (++undocounter_  > MAXUNDO) undocounter_ = MAXUNDO;
	}
	if (oldidx >= 0) musElementList_.at(oldidx);
}

void NVoice::invalidateReUndo(bool with_undo) {
	while (redocounter_ > 0) {
		redoptr_ = (redoptr_ + MAXUNDO - 1) % MAXUNDO;
		redocounter_--;
		freeCloneGroup(redoelem_[redoptr_].backup_area);
	}
	if (!with_undo) return;
	while (undocounter_ > 0) {
		undoptr_ = (undoptr_ + MAXUNDO - 1) % MAXUNDO;
		undocounter_--;
		freeCloneGroup(undoelem_[undoptr_].backup_area);
	}
}

void NVoice::deleteLastUndo() {
	undoptr_ = (undoptr_ + MAXUNDO - 1) % MAXUNDO;
	undocounter_--;
}

void NVoice::setCountOfAddedItems(int count_of_added_items) {
	undoelem_[lastundoptr_].num_of_replaced_items = count_of_added_items;
}


void NVoice::freeCloneGroup(QList<NMusElement> *group) {
	if (group == 0) return;
	group->setAutoDelete(true);
	group->clear();
	delete group;
}

/*--------------------------- export MusicXML --------------------------------*/

// find last non-grace chord starting between xpos1 and xpos2

NChord *NVoice::findLastChordBetweenXpos(int xpos1, int xpos2)
{
	int lower;
	int upper;
	int oldidx;
	NMusElement *elem;
	NChord *chord = 0;

	if (xpos1 <= xpos2) {
		lower = xpos1;
		upper = xpos2;
	} else {
		lower = xpos2;
		upper = xpos1;
	}

	oldidx = musElementList_.at();
	for (elem = musElementList_.first(); elem; elem = musElementList_.next()) {
		if ((xpos1 <= elem->getBbox()->left())
		     && (elem->getBbox()->left() < xpos2)
		     && (elem->getType() == T_CHORD)
		     && !(elem->chord()->properties_ & PROP_GRACE)) {
			chord = (NChord *) elem;
		}
	}
	if (oldidx >= 0) musElementList_.at(oldidx);
	return chord;
}

// find the octava value that is in effect at xpos
// i.e. search for the last va line that starts at or before xpos
// and ends after xpos

int NVoice::getVaAtXpos(int xpos)
{
	int va = 0;
	int oldidx;
	NChord *chord;
	NMusElement *elem;

	oldidx = musElementList_.at();
	for (elem = musElementList_.first(); elem && elem->getXpos() <= xpos; elem = musElementList_.next()) {
		if (elem->getType() == T_CHORD) {
			chord = (NChord *) elem;
			if (chord->va_ && (chord->getVaEnd() > xpos)) {
				va = (chord->va_ > 0) ? +1 : -1;
			}
		}
	}

	if (oldidx >= 0) musElementList_.at(oldidx);
	return va;
}

// for all notes in all chords starting at or after tstart and ending before
// tend, correct the pitch by sign octaves as required for 8va handling

void NVoice::correctPitchBecauseOfVa(int tstart, int tend, int sign)
{
	int oldidx;
	NMusElement *elem;
	NChord *chord = 0;
	NNote *note = 0;

	oldidx = musElementList_.at();
	for (elem = musElementList_.first();
	     elem && elem->midiTime_ < tend;
	     elem = musElementList_.next()) {
		if ((elem->getType() == T_CHORD)
		    && (elem->midiTime_ >= tstart)) {
			chord = (NChord *) elem;
			for (note = chord->getNoteList()->first();
			     note;
			     note = chord->getNoteList()->next()) {
			     	note->line -= 7 * sign;
			}
		}
	}

	if (oldidx >= 0) musElementList_.at(oldidx);
}
