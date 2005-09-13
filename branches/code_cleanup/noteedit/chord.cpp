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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "chord.h"
#include "resource.h"
#include "keysig.h"
#include "clef.h"
#include "transpainter.h"
#include "chorddiagram.h"

#define NECK_LENGTH 63
#define GRACE_NECK_LENGTH 44
#define MIN_DIST (2*LINE_DIST)
#define FLAG_DIST 14
#define AUX_L_2 18
#define POINT_DIST 3
#define POINT_OFFS 6
#if QT_VERSION >= 300
#define POINT_RAD 4
#define STACC_SIZE 4
#else
#define POINT_RAD 5
#define STACC_SIZE 5
#endif
#define ACC_DIST 3
#define LYRICS_Y_SPACE 30
#define SLURDIST 12 
#define SLURDIST2 24
#define SLUR_X_DIST 16
#define SLUR_Y_DIST 12
#define STACC_XDDIST 12
#define STACC_XUDIST 16
#define STACC_YUDIST 20
#define STACC_YDDIST 24
#define ACC_XDDIST 12
#define ACC_YDDIST -24
#define ACC_XUDIST -12
#define ACC_YUDIST 16
#define ARPEGG_DIST 15

#define ACC_TDIST 20
#define ACC_BDIST -10
#define MIN_GAP_BETWEEN_TEX_ACCS 4

// among other things, this macro is used for the distance of the trill above the note.
#define ADD_DIST 16
#define TRILL_DIST 40

#define DYNAMIC_DIST     40
#define DYNAMIC_WIDTH	 20

#define PEDAL_X_DIST -20 
#define PEDAL_Y_DIST  20

#define VA_DIST 40
#define VA_BASSA_DIST 70

#define TR_SIGN_WIDTH 40
#define MAXTEXTRILLS 6
#define MAXTEXTVAS 6

#define STROKE_X_1 (-10)
#define STROKE_Y_1 (-15)

#define STROKE_X_2 10
#define STROKE_Y_2 0

#if QT_VERSION >= 300
#define LYRICSWIDTH_FAC(x) ((int) (1.5 * x))
#else
#define LYRICSWIDTH_FAC(x) (x)
#endif

QPoint NChord::StrokeDist1_(STROKE_X_1, STROKE_Y_1);
QPoint NChord::StrokeDist2_(STROKE_X_2, STROKE_Y_2);


#define STEM_LOGIC(line_expression) SET_STATUS((status_ & PROP_GRACE) || (main_props_->actualStemDir == STEM_DIR_AUTO && \
						voices_stem_policy == STEM_POL_INDIVIDUAL && (line_expression) < 4) || \
		    				main_props_->actualStemDir == STEM_DIR_UP || \
		    				(voices_stem_policy == STEM_POL_UP && main_props_->actualStemDir != STEM_DIR_DOWN), status_, PROP_STEM_UP);


int NChord::numTexRows_ = 0;
QList<NNote> NChord::acc_tex_row;

NChord::NChord(main_props_str *main_props, staff_props_str *staff_props, NVoice *voice, int line, int offs, int length, int voices_stem_policy, property_type status) :
		 NPlayable(main_props, staff_props), m_(0.0), n_(0.0) {
	NNote *note;
	trill_ = dynamic_ = va_ = 0;
	dynamicAlign_ = false;
	if (line > MAXLINE) line = MAXLINE;
	else if (line < MINLINE) line = MINLINE;
	xposDecor_ = xpos_ = 0;
	length_ = length;
	voice_ = voice;
	note = new NNote;
	note->status = (status & NOTE_PROP_PART);
	if (length > WHOLE_LENGTH || (status & PROP_GRACE)) {
		note->status &= (~BODY_MASK);
	}
	status_ = (status & PROP_GRACE) ? (status & GRACE_PROP_PART) : (status & (CHORD_PROP_PART | PROP_PEDAL_ON | PROP_PEDAL_OFF | PROP_AUTO_TRIPLET ));
	midiLength_ = computeMidiLength();
	actualNote_ = 0;
	note->line = line;
	note->offs = offs;
	note->nbase_draw_point = QPoint(0,0);
	note->tie_forward = 0;
	note->tie_backward = 0;
	slur_forward_ = 0;
	slur_backward_ = 0;
	note->chordref = this;
	noteList_.append(note);
	auxInfo_.provSlur_ = 0;
	nextBeamedChord_ = 0;
	actual_ = false;
	STEM_LOGIC(line);
	lyrics_ = 0;
	lyricsPoints_ = 0;
	if (status_ & PROP_STEM_UP) {
		status_ |= PROP_STEM_UP_BEFORE_BEAM;
	}
	cdiagram_ = 0;
	calculateFlagCount();
	calculateDimensionsAndPixmaps();
}


NChord* NChord::clone() {
	NChord *cchord;
	NNote *note, *cnote;
	int i;
	cchord = new NChord(main_props_, staff_props_, voice_, 0, 0, length_, STEM_POL_INDIVIDUAL, 0);
	
	cchord->noteList_.first();
	cchord->noteList_.remove();
	for (note = noteList_.first(); note; note = noteList_.next()) {
		cnote = new NNote;
		*cnote = *note;
		cnote->chordref = cchord;
		cchord->noteList_.append(cnote);
	}
	cchord->status_     = status_;
	cchord->trill_	    = trill_;
	cchord->dynamic_    = dynamic_;
	cchord->dynamicAlign_ = dynamicAlign_;
	cchord->va_        = va_;
	cchord->midiLength_ = midiLength_;
	cchord->midiTime_   = midiTime_;
	cchord->nextBeamedChord_ = nextBeamedChord_;
	cchord->actual_ = false;
	cchord->beamList_ = beamList_;
	cchord->tupletList_ = tupletList_;
	cchord->xpos_ = xpos_;
	cchord->xposDecor_ = xposDecor_;
	cchord->xstart_ = xstart_;
	cchord->numTupNotes_ = numTupNotes_;
	cchord->tupRealTime_ = tupRealTime_;
	cchord->tupletMarker_ = tupletMarker_;
	cchord->xend_ = xend_;
	cchord->flagCount_ = flagCount_;
	cchord->slur_forward_ = 0;
	cchord->slur_backward_ = 0;
	if (cdiagram_) {
		cchord->cdiagram_ = new NChordDiagram(cdiagram_);
	}
	else {
		cchord->cdiagram_ = 0;
	}
	if (lyrics_) {
		cchord->lyrics_ = new QString*[NUM_LYRICS];
		for (i = 0; i < NUM_LYRICS; ++i) {
			if (lyrics_[i]) {
				cchord->lyrics_[i] = new QString(*(lyrics_[i]));
			}
			else {
				cchord->lyrics_[i] = 0;
			}
		}
	}
	else {
		cchord->lyrics_ = 0;
	}
	if (lyricsPoints_) {
		cchord->lyricsPoints_ = new QPoint*[NUM_LYRICS];
		for (i = 0; i < NUM_LYRICS; ++i) {
			if (lyricsPoints_[i]) {
				cchord->lyricsPoints_[i] = new QPoint(*(lyricsPoints_[i]));
			}
			else {
				cchord->lyricsPoints_[i] = 0;
			}
		}
	}
	else {
		cchord->lyricsPoints_ = 0;
	}
	return cchord;
}

void NChord::computeStemBefore() {
	int stemups = 0, stemdowns = 0;
	NNote *note;

	for (note = noteList_.first(); note; note = noteList_.next()) {
		if (note->line < 4) stemups++;
		else stemdowns++;
	}
	if (stemdowns > stemups) {
		status_ &= (~PROP_STEM_UP_BEFORE_BEAM);
	}
	else {
		status_ |= PROP_STEM_UP_BEFORE_BEAM;
	}
}

	

int NChord::computeMidiLength() const {
	if (status_ & PROP_TUPLET) {
		return tupRealTime_ * length_ / numTupNotes_;
	}
	switch (status_ & DOT_MASK) {
		case 1: return 3 * length_ / 2;
		case 2: return 7 * length_ / 4;
	}
	return length_;
}
int NChord::getMidiLength(bool forPlayback) const {
	if (!(status_ & PROP_GRACE)) return midiLength_;
	return forPlayback ? INTERNAL_GRACE_MIDI_LENGTH : 0;
}

void NChord::setLyrics(QString *lyrics, int nr) {
	int i;
	if (status_ & PROP_GRACE) return;
	if (nr < 0 || nr >= NUM_LYRICS) return;
	if (!lyrics_) {
		lyrics_ = new QString*[NUM_LYRICS];
		for (i = 0; i < NUM_LYRICS; ++i) {
			lyrics_[i] = 0;
		}
	}
	if (!lyricsPoints_) {
		lyricsPoints_ = new QPoint*[NUM_LYRICS];
		for (i = 0; i < NUM_LYRICS; ++i) {
			lyricsPoints_[i] = 0;
		}
	}
	if (!lyrics_[nr]) {
		lyrics_[nr] = new QString(*lyrics);
	}
	else {
		(*lyrics_[nr]) = *lyrics;
	}
	if (!lyricsPoints_[nr]) {
		lyricsPoints_[nr] = new QPoint();
	}
	calculateDimensionsAndPixmaps();
}

int NChord::intersects_horizontally(const QPoint p) const {
	if (p.x() < narrow_left_) return -1;
	if (p.x() >= narrow_left_ && p.x() <= narrow_right_) return 0;
	return 1;
}
	

void NChord::deleteLyrics(int nr) {
	int i;
	bool found;
	if (nr < 0 || nr >= NUM_LYRICS) return;
	if (lyrics_) {
		if (lyrics_[nr]) {
			delete lyrics_[nr];
			lyrics_[nr] = 0;
		}
		found = false;
		for (i = 0; !found && i < NUM_LYRICS; ++i) {
			found = lyrics_[i] != 0;
		}
		if (!found) {
			delete lyrics_;
			lyrics_ = 0;
		}
	}
	if (lyricsPoints_) {
		if (lyricsPoints_[nr]) {
			delete lyricsPoints_[nr];
			lyricsPoints_[nr] = 0;
		}
		found = false;
		for (i = 0; !found && i < NUM_LYRICS; ++i) {
			found = lyricsPoints_[i] != 0;
		}
		if (!found) {
			delete lyricsPoints_;
			lyricsPoints_ = 0;
		}
	}
}

void NChord::transposeChordDiagram(int semitones) {
	if (!cdiagram_) return;
	if (cdiagram_->showDiagram_) return;
	cdiagram_->transpose(semitones);
}

int NChord::countOfLyricsLines() {
	int i;
	if (!lyrics_) return 0;
	for (i = NUM_LYRICS - 1; i >= 0; i--) {
		if (lyrics_[i]) return (i+1);
	}
	return 0;
}

QString *NChord::getLyrics(int nr) {
	if (!lyrics_) return 0;
	if (nr < 0 || nr >= NUM_LYRICS) return 0;
	if (!lyrics_[nr]) return 0;
#ifdef OLDLYRICS_POLICY
	if (*(lyrics_[nr]) == "-") return 0;
#endif
	return lyrics_[nr];
}

void NChord::breakBeames() {
	NChord *chord;
	for (chord = beamList_->first(); chord; chord = beamList_->next()) {
		chord->status_ &= (~PROP_BEAMED);
		if (chord->status_ & PROP_STEM_UP_BEFORE_BEAM) {
			chord->status_ |= PROP_STEM_UP;
		}	
		else {
			chord->status_ &= (~PROP_STEM_UP);
		}
		chord->calculateDimensionsAndPixmaps();
	}
}

void NChord::changeLength(int length) {
	NNote *note;
	length_ = length;
	if (status_ & PROP_GRACE) {
		if (length < NOTE16_LENGTH || length > NOTE8_LENGTH) return;
	}
	if (length > NOTE8_LENGTH && (status_ & PROP_BEAMED)) {
		breakBeames();
	} 
	if (length > WHOLE_LENGTH) {
		for (note = noteList_.first(); note; note = noteList_.next()) {
			note->status &= (~BODY_MASK);
		}
	}
		
	midiLength_ = computeMidiLength();
	calculateFlagCount();
}

bool NChord::setOctaviationStart(int size) {
	switch (size) {
		case 8: va_ = 0x00018000;
			break;
		case -8:
			va_ = 0x00010000;
			break;
		default: return false;
	}
	return true;
}

bool NChord::setOctaviationStop(int size) {
	switch (size) {
		case 8: va_ = 0x00030000;
			break;
		case -8:
			va_ = 0x00038000;
			break;
		default: return false;
	}
	return true;
}


void NChord::changeBody(property_type bodyType) {
	NNote *note;
	if (status_ & PROP_GRACE) return;
	note = noteList_.at(actualNote_);
	if (note == NULL) {
		NResource::abort("changeBody: internal error");
	}
	note->status &= (~BODY_MASK);
	note->status |= (bodyType & BODY_MASK);
}

void NChord::changeOffs(int offs, NKeySig *actual_keysig) {
	NNote *note;
	note = noteList_.at(actualNote_);
	if (note == NULL) {
		NResource::abort("changeOffs internal error");
	}
	if (offs == UNDEFINED_OFFS) {
		note->offs = actual_keysig->getOffset(note->line);
		note->status &= (~PROP_FORCE);
		return;
	}
	note->offs = offs;
	note->status |= PROP_FORCE;
}

void NChord::setActualTied(bool tied) {
	NNote *note;
	if (status_ & PROP_GRACE) return;
	note = noteList_.at(actualNote_);
	if (note == NULL) {
		NResource::abort("setActualTied: internal error");
	}
	SET_STATUS(tied, note->status, PROP_TIED);
}

void NChord::removeAllTies(){
	NNote *note;

	for (note = noteList_.first(); note; note = noteList_.next()) {
		note->status &= (~(PROP_TIED));
	}
}

void NChord::tieWith(NChord *otherChordBefore) {
	NNote *note, *note2;
	if (status_ & PROP_GRACE) return;
	for (note = noteList_.first(), note2 = otherChordBefore->noteList_.first();
		note; note = noteList_.next(), note2 = otherChordBefore->noteList_.next()) {
		if (note2 == NULL) {
			NResource::abort("NChord::tieWith");
		}
		note->status |= PROP_PART_OF_TIE;
		note->tie_backward = note2;
		note2->status |= PROP_TIED;
		note2->tie_forward = note;
	}
}

void NChord::setArpeggio(bool on) {
	if (status_ & PROP_GRACE) return;
	SET_STATUS(on, status_, PROP_ARPEGG);
}

void NChord::setPedalOn(bool on) {
	if (status_ & PROP_GRACE) return;
	SET_STATUS(on, status_, PROP_PEDAL_ON);
}

void NChord::setPedalOff(bool on) {
	if (status_ & PROP_GRACE) return;
	SET_STATUS(on, status_, PROP_PEDAL_OFF);
}

void NChord::resetSlurForward() {
	status_ &= (~PROP_SLURED);
	slur_forward_ = 0;
}

void NChord::resetSlurBackward() {
	status_ &=  (~PROP_PART_OF_SLUR);
	slur_backward_ = 0;
}

void NChord::setSlured(bool slured, NChord *partner) {
	SET_STATUS(slured, status_, PROP_SLURED);
	if (slured) {
		slur_forward_ = partner;
		partner->slur_backward_ = this;
		partner->status_ |= PROP_PART_OF_SLUR;
		partner->calculateDimensionsAndPixmaps();
	}
	else {
		slur_forward_->status_ &= (~PROP_PART_OF_SLUR);
		slur_forward_->calculateDimensionsAndPixmaps();
		slur_forward_->slur_backward_ = 0;
		slur_forward_ = 0;
	}
	calculateDimensionsAndPixmaps();
}

void NChord::breakSlurConnections() {
	if (status_ & PROP_SLURED) {
		slur_forward_->status_ &= (~PROP_PART_OF_SLUR);
		slur_forward_->slur_backward_ = 0;
		status_ &= (~PROP_SLURED);
	}
	if (status_ & PROP_PART_OF_SLUR) {
		slur_backward_->status_ &= (~PROP_SLURED);
		slur_backward_->slur_forward_ = 0;
		status_ &= (~PROP_PART_OF_SLUR);
	}
}

void NChord::checkSlures() {
	if (status_ & PROP_SLURED) {
		slur_forward_->status_ &= (~PROP_PART_OF_SLUR);
		slur_forward_->slur_backward_ = 0;
		status_ &= (~PROP_SLURED);
		slur_forward_ = 0;
	}
	if (status_ & PROP_PART_OF_SLUR) {
		slur_backward_->status_ &= (~PROP_SLURED);
		slur_backward_->slur_forward_ = 0;
		status_ &= (~PROP_PART_OF_SLUR);
		slur_backward_ = 0;
	}
}

void NChord::setTupletParams(QList<NPlayable> *tupletList, 
				bool last, double m, double n, double tuptexn, int xstart, int xend, char numnotes, char playtime) {
	tupletList_ = tupletList;
	SET_STATUS(last, status_, PROP_LAST_TUPLET);
	status_ |= PROP_TUPLET;
	tupTeXn_ = tuptexn;
	tupm_ = m; tupn_ = n; xstart_ = xstart; xend_ = xend;
	numTupNotes_ = numnotes; tupRealTime_ = playtime;
	switch (numnotes) {
		case 2:  tupletMarker_ = NResource::tuplet2_; break;
		case 3:  tupletMarker_ = NResource::tuplet3_; break;
		case 4:  tupletMarker_ = NResource::tuplet4_; break;
		case 5:  tupletMarker_ = NResource::tuplet5_; break;
		case 6:  tupletMarker_ = NResource::tuplet6_; break;
		case 7:  tupletMarker_ = NResource::tuplet7_; break;
		case 8:  tupletMarker_ = NResource::tuplet8_; break;
		case 9:  tupletMarker_ = NResource::tuplet9_; break;
		case 10: tupletMarker_ = NResource::tuplet10_; break;
	}
	midiLength_ = computeMidiLength();
}

void NChord::setDotted(int dotcount) {
	if (status_ & PROP_GRACE) return;
	status_ &= (~DOT_MASK);
	status_ |= (dotcount & DOT_MASK);
	midiLength_ = computeMidiLength();
}

void NChord::computeBeames(int stemPolicy) {
	computeBeames(beamList_, stemPolicy);
}

bool NChord::equalTiedChord(NChord *chord2) {
	NNote *note, *cnote;

	if (noteList_.count() != chord2->noteList_.count()) return false;
	if ((status_ & PROP_TUPLET) || (chord2->status_ & PROP_TUPLET)) return false;
	for (note = noteList_.first(), cnote = chord2->noteList_.first(); note;
		note = noteList_.next(), cnote = chord2->noteList_.next()) {
		if (!note->tie_forward) return false;
		if (note->tie_forward != cnote) return false;
	}
	return true;
}

void NChord::setStemUp(bool stem_up) {
	if (status_ & PROP_GRACE) return;
	SET_STATUS(stem_up, status_, PROP_STEM_UP);
	SET_STATUS(stem_up, status_, PROP_STEM_UP_BEFORE_BEAM);
	calculateDimensionsAndPixmaps();
}

void NChord::computeLineParams(QList<NChord> *beamList, double *np, double *mp) {
	NChord *chord;
	double sumxi2, sumxi, sumxiyi, sumyi;
	int count;
	double m, n, don, x0, xdist;

	sumxi2 = sumxi = sumxiyi = sumyi = count = 0;
	x0 = beamList->first()->getXpos();
	for (chord = beamList->first(); chord; chord = beamList->next()) {
		xdist = chord->getTopY()->x() - x0;
		sumxi2 += xdist * xdist;
		sumyi += chord->getTopY()->y();
		sumxi += xdist;
		sumxiyi += xdist * chord->getTopY()->y();
		++count;
	}

	
	m = - (sumxi*sumyi-sumxiyi*count)/(don = sumxi2*count-sumxi*sumxi);
	n =  (-sumxi*sumxiyi+sumyi*sumxi2)/don;
	n += m * x0;
	*np = n; *mp = m;
}

void NChord::computeBeames(QList<NChord> *beamList, int stemPolicy) {
#define MANY 1e30
	NChord *chord0, *chord;
	double n, m, nn, minn = MANY, maxn = -MANY;
//#define MID_BEAM
#ifdef MID_BEAM
	bool distOk;
	double nmid;
#endif
	int numStemUp = 0, numStemDown = 0;
	bool forceStemUp;

	computeLineParams(beamList, &n, &m);
	for (chord = beamList->first(); chord; chord = beamList->next()) {
		nn = (double) chord->getTopY()->y() - m * (double) chord->getTopY()->x();
		if (chord->status_ & PROP_STEM_UP) {
			if (nn < minn) minn = nn;
			++numStemUp;
		}
		else {
			if (nn > maxn) maxn = nn;
			++numStemDown;
		}
	}
	if (minn != MANY && maxn  != -MANY) {
#ifdef MID_BEAM
		if (NResource::allowMixedBeames_ && stemPolicy == STEM_POL_INDIVIDUAL) {
			distOk = true;
			double dist;
			nmid = minn + (maxn - minn) / 2.0;
			for (chord = beamList->first(); distOk && chord; chord = beamList->next()) {
				distOk = chord->length_ > NOTE16_LENGTH;
				dist = m * (double) chord->getTopY()->x() + nmid - (double) chord->getRefY();
				if (fabs(dist) < MIN_DIST) {
					distOk = false;
				}
			}
		}
		else {
			distOk = false;
		}
		if (distOk) {
			n = nmid;
		}
		else
#endif
			{
			forceStemUp = ((numStemUp > numStemDown) || stemPolicy == STEM_POL_UP);
			for (chord = beamList->first(); chord; chord = beamList->next()) {
				SET_STATUS(forceStemUp, chord->status_, PROP_STEM_UP);
				chord->calculateDimensionsAndPixmaps();
			}
			computeLineParams(beamList, &n, &m);
			minn = MANY; maxn = -MANY; 
			for (chord = beamList->first(); chord; chord = beamList->next()) {
				nn = (double) chord->getTopY()->y() - m * (double) chord->getTopY()->x();
				if (forceStemUp) {
					if (nn < minn) minn = nn;
				}
				else {
					if (nn > maxn) maxn = nn;
				}
			}
			n = forceStemUp ? minn : maxn;
		}
	}
	else if (maxn != -MANY) {
		n = maxn;
	}
	else {
		n = minn;
	}
		
	chord0 = beamList->first();
	for (chord = beamList->next(); chord; chord = beamList->next()) {
		chord0->setBeamParams(beamList, chord, m, n);
		chord0->calculateDimensionsAndPixmaps();
		chord0 = chord;
	}
	chord0->setBeamParams(beamList, 0, m, n);
	chord0->calculateDimensionsAndPixmaps();
}

void NChord::setBeamParams(QList<NChord> *beamList, NChord *nextChord, double m, double n) {
	status_ |= PROP_BEAMED;
	m_ = m; n_ = n;
	nextBeamedChord_ = nextChord;
	beamList_ = beamList;
}

bool NChord::beamHasOnlyTwoChords() {
	if (!(status_ & PROP_BEAMED)) return false;
	return (beamList_->count() == 2);
}

void NChord::removeFromBeam() {
	char *err = "internal error: removeFromBeam";
	if (!(status_ & PROP_BEAMED)  || beamList_ == 0) {
		NResource::abort(err, 1);
	}
	if (beamList_->find(this) == -1) {
		NResource::abort(err, 2);
	}
	beamList_->remove();
}

void NChord::resetBeamFlags() {
	status_ &= (~(PROP_BEAMED));
	nextBeamedChord_ = 0;
	beamList_ = 0;
}

bool NChord::setActualNote(int line) {
	NNote *note;
	int i;
	for (i = 0, note = noteList_.first(); note; note = noteList_.next(), i++) {
		if (note->line == line) {
			actualNote_ = i;
			return true;
		}
	}
	actualNote_ = 0;
	return false;
}

bool NChord::removeNote(NNote *note, int voices_stem_policy) {
	if (noteList_.find(note) < 0) {
		NResource::abort("removeNote: internal error(1)");
	}
	if (noteList_.count() < 2) return false;
	noteList_.remove();
	actualNote_ = noteList_.at();
	STEM_LOGIC(noteList_.first()->line);
	calculateDimensionsAndPixmaps();
	if (actualNote_ < 0) {
		NResource::abort("removeNote: internal error(2)");
	}
	return true;
}


NNote *NChord::insertNewNote(int line, int offs, int voices_stem_policy, property_type status) {
	NNote *note, *new_part;
	int idx;
	bool found = false;
	note = noteList_.first();
	while (note && !found) {
		if (note->line == line) return 0;
		if (found = note->line > line) {
			idx = noteList_.at();
		}
		else {
			note = noteList_.next();
		}
	}
	new_part = new NNote;
	new_part->line = line;
	new_part->offs = offs;
	if (length_ > WHOLE_LENGTH) {
		status &= (~BODY_MASK);
	}
	new_part->status = status;
	new_part->nbase_draw_point = QPoint(0, 0);
	new_part->tie_forward = 0;
	new_part->tie_backward = 0;
	new_part->chordref = this;
	if (found) {
		noteList_.insert(idx, new_part);
		actualNote_ = idx;
	}
	else {
		actualNote_ = noteList_.count();
		noteList_.append(new_part);
	}
	STEM_LOGIC(noteList_.first()->line);
	calculateDimensionsAndPixmaps();
	return new_part;
}

void NChord::determineStemDir(int voices_stem_policy) {
	STEM_LOGIC(noteList_.first()->line);
}
	

NNote *NChord::searchLine(int line, int min) {
	NNote *note;
	if (noteList_.count() < min) return 0;
	for (note = noteList_.first(); note; note = noteList_.next()) {
		if (note->line == line) {
			return note;
		}
	}
	return 0;
}


bool NChord::deleteNoteAtLine(int line, int voices_stem_policy) {
	NNote *note;
	if (noteList_.count() < 2) return false;
	for (note = noteList_.first(); note; note = noteList_.next()) {
		if (note->line == line) {
			noteList_.remove();
			actualNote_ = noteList_.at();
			STEM_LOGIC(noteList_.first()->line);
			calculateDimensionsAndPixmaps();
			return true;
		}
	}
	return false;
}
	

NChord::~NChord () {
	int i;
	if (status_ & PROP_BEAMED) {
		if (beamList_->find(this) == -1) {
			printf("&GRACE= 0x%x\n", status_ &PROP_GRACE); fflush(stdout);
			NResource::abort("~Note: internal error");
		}
		beamList_->remove();
		if (beamList_->count() == 0) {
			delete beamList_;
		}
	}
	noteList_.setAutoDelete(true);
	noteList_.clear();
	if (lyrics_) {
		for (i = 0; i < NUM_LYRICS; ++i) {
			if (lyrics_[i]) {
				delete lyrics_[i];
			}
		}
		delete lyrics_;
	}
	if (lyricsPoints_) {
		for (i = 0; i < NUM_LYRICS; ++i) {
			if (lyricsPoints_[i]) {
				delete lyricsPoints_[i];
			}
		}
		delete lyricsPoints_;
	}
	if (cdiagram_) {
		delete cdiagram_;
	}
}

void NChord::checkAcc() {
	NNote *note;
	for (note = noteList_.first(); note; note = noteList_.next()) {
		if (note->offs == UNDEFINED_OFFS) { /* can happen after reading */
				if (note->status & PROP_PART_OF_TIE) {
					note->offs = note->tie_backward->offs;
				}
				else {
					note->offs = staff_props_->actual_keysig->getOffset(note->line);
				}
		}
		switch (note->needed_acc = staff_props_->actual_keysig->accentNeeded(note->line, note->offs)) {
			case PROP_NO_ACC: break;
			case PROP_CROSS:
			case PROP_NATUR:
			case PROP_FLAT:
			case PROP_DCROSS:
			case PROP_DFLAT:
			 staff_props_->actual_keysig->setTempAccent(note->line, note->needed_acc);
			break;
		}
		note->status &= (~ACC_MASK);
		if (note->status & PROP_FORCE) {
			switch(note->offs) {
				case -2: note->status |= PROP_DFLAT; break;
				case -1: note->status |= PROP_FLAT; break;
				case  0: note->status |= PROP_NATUR; break;
				case  1: note->status |= PROP_CROSS; break;
				case  2: note->status |= PROP_DCROSS; break;
			}
		}
		else {
			note->status |= (ACC_MASK & note->needed_acc);
		}
	}
}

void NChord::accumulateAccidentals(NKeySig *key) {
	NNote *note;
	for (note = noteList_.first(); note; note = noteList_.next()) {
		if (note->status & PROP_FORCE) {
			switch (note->offs) {
				case  1: key->setTempAccent(note->line, PROP_CROSS); break;
				case -1: key->setTempAccent(note->line, PROP_FLAT); break;
				case  0: key->setTempAccent(note->line, PROP_NATUR); break;
				case  2: key->setTempAccent(note->line, PROP_DCROSS); break;
				case -2: key->setTempAccent(note->line, PROP_DFLAT); break;
			}
		}
		else {
			switch (note->status & ACC_MASK) {
				case PROP_CROSS: key->setTempAccent(note->line, PROP_CROSS); break;
				case PROP_FLAT:  key->setTempAccent(note->line, PROP_FLAT); break;
				case PROP_NATUR: key->setTempAccent(note->line, PROP_NATUR); break;
				case PROP_DCROSS: key->setTempAccent(note->line, PROP_DCROSS); break;
				case PROP_DFLAT:  key->setTempAccent(note->line, PROP_DFLAT); break;
			}
		}
	}
}

void NChord::calculateFlagCount() {
	flagCount_ = 0;
	switch (length_) {
		case NOTE8_LENGTH  : flagCount_ = 1; break;
		case NOTE16_LENGTH : flagCount_ = 2; break;
		case NOTE32_LENGTH : flagCount_ = 3; break;
		case NOTE64_LENGTH : flagCount_ = 4; break;
		case NOTE128_LENGTH: flagCount_ = 5; break;
		default: flagCount_ = 0;
	}
	if (status_ & PROP_GRACE) {
		if (flagCount_ > 2) flagCount_ = 1;
	}
}

void NChord::calculateDimensionsAndPixmaps() {
	xposDecor_ = xpos_;
	if (status_ & PROP_GRACE) {
		calculateGraceChord();
		return;
	}
	int i, last_chord  = NULL_LINE;
	NNote *note;
	int x_aux_offs, x_acc_offs = 0;
	int x_shift_offs = 0, shoffs;
	int min_line, max_line;
	int xbpoint;
	int lyricswidth;
	int yoffs = 0;

	pixmapHeight_ = NECK_LENGTH+1;
	pixmapWidth_ = NResource::nbasePixmapWidth_+12;
	if (length_ < QUARTER_LENGTH && (status_ & PROP_STEM_UP) && !(status_ & PROP_BEAMED) ) pixmapWidth_ += NResource::flagPixmapWidth_;
	min_line = noteList_.first()->line;
	max_line = noteList_.last()->line;
	x_aux_offs = min_line < -1  || max_line > 9 ? AUX_L_2 - NResource::nbasePixmapWidth2_ : 0;
	if (status_ & PROP_ARPEGG) {
		x_acc_offs += ARPEGG_DIST;
	}
	for (note = noteList_.first(); note; note = noteList_.next()) {
		switch (note->status & BODY_MASK) {
			case PROP_BODY_CROSS:
				switch (length_) {
					case HALF_LENGTH:
					case WHOLE_LENGTH: note->bodyPixmap = NResource::perCrossPixmap_;
					            note->redBodyPixmap = NResource::perCrossRedPixmap_;
					            note->greyBodyPixmap = NResource::perCrossGreyPixmap_;
					    	    break;
					default: note->bodyPixmap =  NResource::perCrossPixmap_;
						 note->redBodyPixmap = NResource::perCrossRedPixmap_;
						 note->greyBodyPixmap = NResource::perCrossGreyPixmap_;
						 break;
				}
				break;
			case PROP_BODY_CROSS2:
				switch (length_) {
					case HALF_LENGTH:
					case WHOLE_LENGTH: note->bodyPixmap = NResource::perCross2Pixmap_;
					            note->redBodyPixmap = NResource::perCross2RedPixmap_;
					            note->greyBodyPixmap = NResource::perCross2GreyPixmap_;
					    	    break;
					default: note->bodyPixmap =  NResource::perCross2Pixmap_;
						 note->redBodyPixmap = NResource::perCross2RedPixmap_;
						 note->greyBodyPixmap = NResource::perCross2GreyPixmap_;
						 break;
				}
				break;
			case PROP_BODY_CIRCLE_CROSS:
				switch (length_) {
					case HALF_LENGTH:
					case WHOLE_LENGTH: note->bodyPixmap = NResource::perCrossCircPixmap_;
					            note->redBodyPixmap = NResource::perCrossCircRedPixmap_;
					            note->greyBodyPixmap = NResource::perCrossCircGreyPixmap_;
					    	    break;
					default: note->bodyPixmap =  NResource::perCrossCircPixmap_;
						 note->redBodyPixmap = NResource::perCrossCircRedPixmap_;
						 note->greyBodyPixmap = NResource::perCrossCircGreyPixmap_;
						 break;
				}
				break;
			case PROP_BODY_RECT:
				switch (length_) {
					case HALF_LENGTH:
					case WHOLE_LENGTH: note->bodyPixmap = NResource::perRectPixmap_;
					            note->redBodyPixmap = NResource::perRectRedPixmap_;
					            note->greyBodyPixmap = NResource::perRectGreyPixmap_;
					    	    break;
					default: note->bodyPixmap =  NResource::perRectPixmap_;
						 note->redBodyPixmap = NResource::perRectRedPixmap_;
						 note->greyBodyPixmap = NResource::perRectGreyPixmap_;
						 break;
				}
				break;
			case PROP_BODY_TRIA:
				switch (length_) {
					case HALF_LENGTH:
					case WHOLE_LENGTH: note->bodyPixmap = NResource::perTrianPixmap_;
					            note->redBodyPixmap = NResource::perTrianRedPixmap_;
					            note->greyBodyPixmap = NResource::perTrianGreyPixmap_;
					    	    break;
					default: note->bodyPixmap =  NResource::perTrianPixmap_;
						 note->redBodyPixmap = NResource::perTrianRedPixmap_;
						 note->greyBodyPixmap = NResource::perTrianGreyPixmap_;
						 break;
				}
				break;
			default:
				switch (length_) {
					case HALF_LENGTH:
					case WHOLE_LENGTH: note->bodyPixmap = NResource::fullPixmap_;
				    		    note->redBodyPixmap = NResource::fullRedPixmap_;
				    		    note->greyBodyPixmap = NResource::fullGreyPixmap_;
				    		    break;
	      				case DOUBLE_WHOLE_LENGTH: note->bodyPixmap = NResource::brevePixmap_ ;
						    note->redBodyPixmap   = NResource::breveRedPixmap_;
						    note->greyBodyPixmap = NResource::breveGreyPixmap_;
						    yoffs = -3;
						    break;
					default: note->bodyPixmap =  NResource::nbasePixmap_;
				 		 note->redBodyPixmap = NResource::nbaseRedPixmap_;
				 		 note->greyBodyPixmap = NResource::nbaseGreyPixmap_;
						break;
					}
				break;
		}
		switch (note->status & ACC_MASK) {
			case PROP_NO_ACC: break;
			case PROP_CROSS:
			note->acc_draw_point = QPoint (xpos_+x_acc_offs, 
			staff_props_->base -  LINE_DIST / 2 + 4 * LINE_DIST - note->line * LINE_DIST  / 2 -18);
			x_acc_offs += NResource::crossPixWidth_ + ACC_DIST;
			break;
			case PROP_NATUR:
			note->acc_draw_point = QPoint (xpos_+x_acc_offs, 
			staff_props_->base -  LINE_DIST / 2 + 4 * LINE_DIST - note->line * LINE_DIST  / 2 -20);
			x_acc_offs += NResource::crossPixWidth_ + ACC_DIST;
			break;
			case PROP_FLAT:
			note->acc_draw_point = QPoint (xpos_+x_acc_offs, 
			staff_props_->base -  LINE_DIST / 2 + 4 * LINE_DIST - note->line * LINE_DIST  / 2-18);
			x_acc_offs += NResource::flatPixWidth_ + ACC_DIST;
			break;
			case PROP_DCROSS:
			note->acc_draw_point = QPoint (xpos_+x_acc_offs, 
			staff_props_->base -  LINE_DIST / 2 + 4 * LINE_DIST - note->line * LINE_DIST  / 2 -2);
			x_acc_offs += NResource::dcrossPixWidth_ + ACC_DIST;
			break;
			case PROP_DFLAT:
			note->acc_draw_point = QPoint (xpos_+x_acc_offs, 
			staff_props_->base -  LINE_DIST / 2 + 4 * LINE_DIST - note->line * LINE_DIST  / 2-14);
			x_acc_offs += NResource::dflatPixWidth_ + ACC_DIST;
			break;
		}
	}
	for (note = noteList_.first(); note; last_chord = note->line, note = noteList_.next()) {
		if (note->line - last_chord  == 1) {
			if (x_shift_offs == 0) x_shift_offs = 2*NResource::nbasePixmapWidth2_+1;
			shoffs = shoffs ? 0 : x_shift_offs;
		}
		else {
			shoffs = 0;
		}
		SET_STATUS(shoffs, note->status, PROP_SHIFTED);
		note->acc_offs = x_acc_offs;
		note->nbase_draw_point = QPoint (xpos_+ x_aux_offs + shoffs + x_acc_offs, staff_props_->base -  LINE_DIST / 2 + 4 * LINE_DIST - note->line * LINE_DIST  / 2 + 1 + yoffs);
		note->point_pos1 = QRect(xpos_+ x_aux_offs + shoffs + 2*NResource::nbasePixmapWidth2_ + POINT_DIST + x_acc_offs,
				staff_props_->base -  LINE_DIST / 2 + 4 * LINE_DIST - note->line * LINE_DIST  / 2 + POINT_OFFS, 2*POINT_RAD, 2*POINT_RAD);
		note->point_pos2 = QRect(xpos_+ x_aux_offs + shoffs + 2*NResource::nbasePixmapWidth2_ + 2*POINT_DIST+2*POINT_RAD + x_acc_offs,
				staff_props_->base -  LINE_DIST / 2 + 4 * LINE_DIST - note->line * LINE_DIST  / 2 + POINT_OFFS, 2*POINT_RAD, 2*POINT_RAD);
		if (note->status & (PROP_TIED | PROP_PART_OF_TIE)) {
		    note->tie_start_point_up = QPoint(note->nbase_draw_point.x() + NResource::nbasePixmapWidth2_, note->nbase_draw_point.y() +
                           NResource::nbasePixmapHeight_);
		    note->tie_start_point_down = QPoint(note->nbase_draw_point.x() + NResource::nbasePixmapWidth2_, note->nbase_draw_point.y());
		    note->tie_forward_point_up = QPoint(note->nbase_draw_point.x() + NResource::nbasePixmapWidth2_+2 , note->nbase_draw_point.y() +
                            NResource::nbasePixmapHeight_+4);
		    note->tie_back_point_up = QPoint(note->nbase_draw_point.x() + NResource::nbasePixmapWidth2_-2, note->nbase_draw_point.y() + 
                           NResource::nbasePixmapHeight_+4);
		    note->tie_forward_point_down = QPoint(note->nbase_draw_point.x() + NResource::nbasePixmapWidth2_+2 , note->nbase_draw_point.y() - 4);
		    note->tie_back_point_down = QPoint(note->nbase_draw_point.x() + NResource::nbasePixmapWidth2_-2, note->nbase_draw_point.y() -4);
		}
	}
	if (status_ & (PROP_SLURED | PROP_PART_OF_SLUR)) {
	    note = noteList_.first();
	    slur_start_point_up_ = QPoint(note->nbase_draw_point.x() + NResource::nbasePixmapWidth2_, note->nbase_draw_point.y() +
                          NResource::nbasePixmapHeight_ + SLURDIST);
	    slur_start_point_down_ = QPoint(note->nbase_draw_point.x() + NResource::nbasePixmapWidth2_, note->nbase_draw_point.y() - SLURDIST);
	    slur_forward_point_up_ = QPoint(note->nbase_draw_point.x() + NResource::nbasePixmapWidth2_+SLUR_X_DIST , note->nbase_draw_point.y() +
                           NResource::nbasePixmapHeight_+SLUR_Y_DIST + SLURDIST2);
	    slur_back_point_up_ = QPoint(note->nbase_draw_point.x() + NResource::nbasePixmapWidth2_-SLUR_X_DIST, note->nbase_draw_point.y() + 
                          NResource::nbasePixmapHeight_+SLUR_Y_DIST + SLURDIST2);
	    slur_forward_point_down_ = QPoint(note->nbase_draw_point.x() + NResource::nbasePixmapWidth2_+SLUR_X_DIST , note->nbase_draw_point.y() - SLUR_Y_DIST- SLURDIST2);
	    slur_back_point_down_ = QPoint(note->nbase_draw_point.x() + NResource::nbasePixmapWidth2_-SLUR_X_DIST, note->nbase_draw_point.y() -SLUR_Y_DIST - SLURDIST2);
	}
	
	pixmapHeight_ += (max_line - min_line) * LINE_DIST / 2+NResource::nbasePixmapHeight_;
	if (status_ & PROP_STEM_UP) {
		nbaseLinePoint1_ = QPoint (xpos_+x_aux_offs+ NResource::nbasePixmapWidth_+x_acc_offs-1, staff_props_->base + 4 * LINE_DIST - 1 - min_line * LINE_DIST  / 2);
		nbaseLinePoint2_ = QPoint (xpos_+x_aux_offs+ NResource::nbasePixmapWidth_+x_acc_offs-1, staff_props_->base + 4 * LINE_DIST - 1 - max_line * LINE_DIST  / 2 -
		 	NECK_LENGTH - ((flagCount_ <=  1) ? 0 : ((flagCount_ - 1) * FLAG_DIST)));
		xbpoint = xpos_+x_aux_offs+ NResource::nbasePixmapWidth_-1+x_acc_offs;
		narrow_left_ = nbaseLinePoint1_.x() - NResource::narrow_dist_;
		narrow_right_ = nbaseLinePoint1_.x() + (x_shift_offs ? NResource::narrow_dist_ : 0);
		stacc_point_ = QRect(nbaseLinePoint1_ + QPoint(-STACC_XUDIST, STACC_YUDIST), QSize(2*STACC_SIZE, 2*STACC_SIZE));
	}
	else {
		nbaseLinePoint1_ = QPoint (xpos_+x_aux_offs+ x_shift_offs + x_acc_offs, staff_props_->base + 4 * LINE_DIST - max_line * LINE_DIST  / 2);
		nbaseLinePoint2_ = QPoint (xpos_+x_aux_offs+ x_shift_offs + x_acc_offs, staff_props_->base + 4 * LINE_DIST - min_line * LINE_DIST  / 2 +
		 	NECK_LENGTH + ((flagCount_ <=  1) ? 0 : ((flagCount_ - 1) * FLAG_DIST)));
		xbpoint = xpos_+x_aux_offs+ x_acc_offs+x_shift_offs;
		narrow_left_ = nbaseLinePoint1_.x() - (x_shift_offs ? NResource::narrow_dist_ : 0);
		narrow_right_ = nbaseLinePoint1_.x() + NResource::narrow_dist_;
		stacc_point_ = QRect(nbaseLinePoint1_ + QPoint(STACC_XDDIST, -STACC_YDDIST), QSize(2*STACC_SIZE, 2*STACC_SIZE));
	}
	nbaseLinePoint3_ = QPoint (xbpoint, (int) (m_ * xbpoint + n_));
	if (status_ & PROP_ARPEGG) {
		arpeggDrawPoint_ = QPoint(xpos_-ARPEGG_DIST, staff_props_->base + 4 * LINE_DIST - max_line * LINE_DIST / 2);
		arpeggParts_ = (max_line - min_line) * LINE_DIST / 2 / NResource::arpegPixmapHeight_;
		if (arpeggParts_ < 1) arpeggParts_ = 1;
	}

	// decide about accent alignment
	u1_.setAccentAboveChord_ = false;
	if (noteList_.last() == noteList_.first()) {
	    if (staff_props_->base < nbaseLinePoint2_.y() && staff_props_->base < nbaseLinePoint1_.y()) {
		// above
		acc_point_ = QPoint(xpos_ + x_aux_offs + x_shift_offs + x_acc_offs + ACC_XDDIST, staff_props_->base - ACC_TDIST);
		u1_.setAccentAboveChord_ = true;
	    } else 
		// below
		acc_point_ = QPoint(xpos_ + x_aux_offs + x_shift_offs + x_acc_offs + ACC_XDDIST, staff_props_->base + ( LINE_DIST * 5 ) + ACC_BDIST);
	}
	else 
	    if (status_ & PROP_STEM_UP) 
		acc_point_ = QPoint(nbaseLinePoint1_ + QPoint(ACC_XUDIST, ACC_YUDIST));
	    else {
		u1_.setAccentAboveChord_ = true;
		acc_point_ = QPoint(nbaseLinePoint1_ + QPoint(ACC_XDDIST, ACC_YDDIST));
		}
	if (trill_) {
		if (max_line < 8) {
			trilly_ = staff_props_->base + 4 * LINE_DIST - 8 * LINE_DIST  / 2 - TRILL_DIST;
		}
		else {
			trilly_ = staff_props_->base + 4 * LINE_DIST - max_line * LINE_DIST  / 2 - TRILL_DIST;
		}
		xposDecor_ = xpos_ + ((trill_ < 0) ? -trill_ : trill_) * NResource::trillPixmap_->width();
	}
	if (va_ > 0) {
		if (max_line < 8) {
			vaY_ = staff_props_->base + 4 * LINE_DIST - 8 * LINE_DIST  / 2 - VA_DIST;
		}
		else {
			vaY_ = staff_props_->base + 4 * LINE_DIST - max_line * LINE_DIST  / 2 - VA_DIST;
		}
		xposDecor_ = xpos_ + va_ * VA_LINE_LEN;
	}
	if (va_ < 0) {
		if (min_line >= 0) {
			vaY_ = staff_props_->base + 4 * LINE_DIST + VA_BASSA_DIST;
		}
		else {
			vaY_ = nbaseLinePoint1_.y() + VA_BASSA_DIST;
		}
		xposDecor_ = xpos_ + -va_ * VA_LINE_LEN;
	}
	if (dynamic_) xposDecor_ = xpos_ + dynamic_;
	if (status_ & PROP_STEM_UP) {
		pedal_point_ = nbaseLinePoint1_ + QPoint(PEDAL_X_DIST, PEDAL_Y_DIST);
	}
	else {
		if (status_ & PROP_BEAMED) {
			pedal_point_ = flag_pos_[0] + QPoint(PEDAL_X_DIST, PEDAL_Y_DIST);
		}
		else {
			pedal_point_ = nbaseLinePoint2_ + QPoint(PEDAL_X_DIST, PEDAL_Y_DIST);
		}
	}
	if (pedal_point_.y() < staff_props_->base + 4 * LINE_DIST+ PEDAL_Y_DIST) {
		pedal_point_.setY(staff_props_->base + 4 * LINE_DIST + PEDAL_Y_DIST);
	}
	
	
	if (lyrics_) {
		main_props_->directPainter->beginTextDrawing();
		main_props_->directPainter->setPen(NResource::blackPen_);
		main_props_->directPainter->setFont( *NResource::textFont_ );
		for (i = 0; i < NUM_LYRICS; ++i) {
		   if (lyrics_[i]) {
			(*lyricsPoints_[i]) = QPoint (xpos_, staff_props_->base + 4 * LINE_DIST + i * LYRICS_Y_SPACE + staff_props_->lyricsdist);
			
			/* do not render syntax characters like <, > and *. */
			QString* printedLyrics = new QString(*(lyrics_[i]));
			printedLyrics->remove('<'); printedLyrics->remove('>');
			lyricswidth = LYRICSWIDTH_FAC(main_props_->directPainter->fontMetrics().width(*printedLyrics));
			delete printedLyrics;
			
			/* enlarge chord's pixmapwidth if needed to accommodate lyrics width */
			if (lyricswidth > pixmapWidth_)  pixmapWidth_ = lyricswidth;
		   }
		}
		main_props_->directPainter->end();
	}
	if (status_ & PROP_BEAMED) {
		for (i = 0; i < 5; ++i) {
			flag_pos_[i] = QPoint(xbpoint, (int) (m_ * xbpoint + n_) + ((status_ & PROP_STEM_UP) ? FLAG_DIST : -FLAG_DIST) * i);
		}
	}
	else {
		if (status_ & PROP_STEM_UP) {
			for (i = 0; i < 5; ++i) {
				flag_pos_[i] = QPoint(xbpoint, nbaseLinePoint2_.y() + i * FLAG_DIST);
			}
		}
		else {
			for (i = 0; i < 5; ++i) {
				flag_pos_[i] = QPoint(xbpoint, nbaseLinePoint2_.y() - i * FLAG_DIST - NResource::flagDownPixmapHeight_);
			}
		}
	}
	if (status_ & PROP_LAST_TUPLET) {
		tuplet0_ = QPoint(xstart_, int(xstart_ * tupm_ + tupn_));
		tuplet1_ = QPoint(xend_, int(xend_ * tupm_ + tupn_));
		tuplet00_ = tuplet0_ + QPoint(0, TUPLET_HEIGHT);
		tuplet01_ = tuplet1_ + QPoint(0, TUPLET_HEIGHT);
		tupletDigit_ = tuplet0_ + (tuplet1_ - tuplet0_) / 2 + QPoint(0, -TUPLET_DGIT_DIST);
	}
		
	pixmapWidth_ += x_aux_offs+x_shift_offs + x_acc_offs;
	switch(status_ & DOT_MASK) {
		case 1: pixmapWidth_ += 3*POINT_DIST + 2*POINT_RAD; break;
		case 2: pixmapWidth_ += 4*POINT_DIST+4*POINT_RAD; break;
	}
	if (cdiagram_) {
		cdiagramDrawPoint_ = QPoint(xpos_ + CPOINT_X_OFFS, getTopY3() + CPOINT_Y_OFFS);
		if (pixmapWidth_ < cdiagram_->neededWidth()) pixmapWidth_ = cdiagram_->neededWidth();
	}
	bbox_ = QRect(xpos_, staff_props_->base - pixmapHeight_ -  LINE_DIST / 2 + 5 * LINE_DIST - min_line * LINE_DIST  / 2,
		pixmapWidth_, pixmapHeight_);
}

void NChord::calculateGraceChord() {
	int i, last_chord  = NULL_LINE;
	NNote *note;
	int x_aux_offs, x_acc_offs = 0;
	int x_shift_offs = 0, shoffs;
	int min_line, max_line;
	int xbpoint;
	int lyricswidth;
	int yoffs = 0;

	pixmapHeight_ = NECK_LENGTH+1;
	pixmapWidth_ = NResource::tinyBasePixmapWidth_+12;
	if ((status_ & PROP_STEM_UP) && !(status_ & PROP_BEAMED) ) pixmapWidth_ += NResource::tinyFlagPixmapWidth_;
	min_line = noteList_.first()->line;
	max_line = noteList_.last()->line;
	x_aux_offs = min_line < -1  || max_line > 9 ? AUX_L_2 - NResource::tinyBasePixmapWidth2_ : 0;
	for (note = noteList_.first(); note; note = noteList_.next()) {
		note->bodyPixmap =  NResource::tinyBasePixmap_;
	        note->redBodyPixmap = NResource::tinyBaseRedPixmap_;
		note->greyBodyPixmap = NResource::tinyBaseGreyPixmap_;
		switch (note->status & ACC_MASK) {
			case PROP_NO_ACC: break;
			case PROP_CROSS:
			note->acc_draw_point = QPoint (xpos_+x_acc_offs, 
			staff_props_->base -  LINE_DIST / 2 + 4 * LINE_DIST - note->line * LINE_DIST  / 2 -18);
			x_acc_offs += NResource::crossPixWidth_ + ACC_DIST;
			break;
			case PROP_NATUR:
			note->acc_draw_point = QPoint (xpos_+x_acc_offs, 
			staff_props_->base -  LINE_DIST / 2 + 4 * LINE_DIST - note->line * LINE_DIST  / 2 -20);
			x_acc_offs += NResource::crossPixWidth_ + ACC_DIST;
			break;
			case PROP_FLAT:
			note->acc_draw_point = QPoint (xpos_+x_acc_offs, 
			staff_props_->base -  LINE_DIST / 2 + 4 * LINE_DIST - note->line * LINE_DIST  / 2-18);
			x_acc_offs += NResource::flatPixWidth_ + ACC_DIST;
			break;
			case PROP_DCROSS:
			note->acc_draw_point = QPoint (xpos_+x_acc_offs, 
			staff_props_->base -  LINE_DIST / 2 + 4 * LINE_DIST - note->line * LINE_DIST  / 2 -2);
			x_acc_offs += NResource::dcrossPixWidth_ + ACC_DIST;
			break;
			case PROP_DFLAT:
			note->acc_draw_point = QPoint (xpos_+x_acc_offs, 
			staff_props_->base -  LINE_DIST / 2 + 4 * LINE_DIST - note->line * LINE_DIST  / 2-14);
			x_acc_offs += NResource::dflatPixWidth_ + ACC_DIST;
			break;
		}
	}
	for (note = noteList_.first(); note; last_chord = note->line, note = noteList_.next()) {
		if (note->line - last_chord  == 1) {
			if (x_shift_offs == 0) x_shift_offs = 2*NResource::tinyBasePixmapWidth2_+1;
			shoffs = shoffs ? 0 : x_shift_offs;
		}
		else {
			shoffs = 0;
		}
		SET_STATUS(shoffs, note->status, PROP_SHIFTED);
		note->acc_offs = x_acc_offs;
		note->nbase_draw_point = QPoint (xpos_+ x_aux_offs + shoffs + x_acc_offs, staff_props_->base -  LINE_DIST / 2 + 4 * LINE_DIST - note->line * LINE_DIST  / 2 + 1 + yoffs);
		note->point_pos1 = QRect(xpos_+ x_aux_offs + shoffs + 2*NResource::tinyBasePixmapWidth2_ + POINT_DIST + x_acc_offs,
				staff_props_->base -  LINE_DIST / 2 + 4 * LINE_DIST - note->line * LINE_DIST  / 2 + POINT_OFFS, 2*POINT_RAD, 2*POINT_RAD);
		note->point_pos2 = QRect(xpos_+ x_aux_offs + shoffs + 2*NResource::tinyBasePixmapWidth2_ + 2*POINT_DIST+2*POINT_RAD + x_acc_offs,
				staff_props_->base -  LINE_DIST / 2 + 4 * LINE_DIST - note->line * LINE_DIST  / 2 + POINT_OFFS, 2*POINT_RAD, 2*POINT_RAD);
		if (note->status & (PROP_TIED | PROP_PART_OF_TIE)) {
		    note->tie_start_point_up = QPoint(note->nbase_draw_point.x() + NResource::tinyBasePixmapWidth2_, note->nbase_draw_point.y() +
                           NResource::tinyBasePixmapHeight_);
		    note->tie_start_point_down = QPoint(note->nbase_draw_point.x() + NResource::tinyBasePixmapWidth2_, note->nbase_draw_point.y());
		    note->tie_forward_point_up = QPoint(note->nbase_draw_point.x() + NResource::tinyBasePixmapWidth2_+2 , note->nbase_draw_point.y() +
                            NResource::tinyBasePixmapHeight_+4);
		    note->tie_back_point_up = QPoint(note->nbase_draw_point.x() + NResource::tinyBasePixmapWidth2_-2, note->nbase_draw_point.y() + 
                           NResource::tinyBasePixmapHeight_+4);
		    note->tie_forward_point_down = QPoint(note->nbase_draw_point.x() + NResource::tinyBasePixmapWidth2_+2 , note->nbase_draw_point.y() - 4);
		    note->tie_back_point_down = QPoint(note->nbase_draw_point.x() + NResource::tinyBasePixmapWidth2_-2, note->nbase_draw_point.y() -4);
		}
	}
	if (status_ & (PROP_SLURED | PROP_PART_OF_SLUR)) {
	    note = noteList_.first();
	    slur_start_point_up_ = QPoint(note->nbase_draw_point.x() + NResource::tinyBasePixmapWidth2_, note->nbase_draw_point.y() +
                          NResource::tinyBasePixmapHeight_ + SLURDIST);
	    slur_start_point_down_ = QPoint(note->nbase_draw_point.x() + NResource::tinyBasePixmapWidth2_, note->nbase_draw_point.y() - SLURDIST);
	    slur_forward_point_up_ = QPoint(note->nbase_draw_point.x() + NResource::tinyBasePixmapWidth2_+SLUR_X_DIST , note->nbase_draw_point.y() +
                           NResource::tinyBasePixmapHeight_+SLUR_Y_DIST + SLURDIST2);
	    slur_back_point_up_ = QPoint(note->nbase_draw_point.x() + NResource::tinyBasePixmapWidth2_-SLUR_X_DIST, note->nbase_draw_point.y() + 
                          NResource::tinyBasePixmapHeight_+SLUR_Y_DIST + SLURDIST2);
	    slur_forward_point_down_ = QPoint(note->nbase_draw_point.x() + NResource::tinyBasePixmapWidth2_+SLUR_X_DIST , note->nbase_draw_point.y() - SLUR_Y_DIST- SLURDIST2);
	    slur_back_point_down_ = QPoint(note->nbase_draw_point.x() + NResource::tinyBasePixmapWidth2_-SLUR_X_DIST, note->nbase_draw_point.y() -SLUR_Y_DIST - SLURDIST2);
	}
	
	pixmapHeight_ += (max_line - min_line) * LINE_DIST / 2+NResource::tinyBasePixmapHeight_;
	nbaseLinePoint1_ = QPoint (xpos_+x_aux_offs+ NResource::tinyBasePixmapWidth_+x_acc_offs-1, staff_props_->base + 4 * LINE_DIST - 1 - min_line * LINE_DIST  / 2);
	nbaseLinePoint2_ = QPoint (xpos_+x_aux_offs+ NResource::tinyBasePixmapWidth_+x_acc_offs-1, staff_props_->base + 4 * LINE_DIST - 1 - max_line * LINE_DIST  / 2 -
	 	GRACE_NECK_LENGTH);
	xbpoint = xpos_+x_aux_offs+ NResource::tinyBasePixmapWidth_-1+x_acc_offs;
	narrow_left_ = nbaseLinePoint1_.x() - NResource::nbasePixmapWidth_;
	narrow_right_ = nbaseLinePoint1_.x() + (x_shift_offs ? NResource::nbasePixmapWidth_ : 0);  
	nbaseLinePoint3_ = QPoint (xbpoint, (int) (m_ * xbpoint + n_));
	tuplet0_ = nbaseLinePoint1_ + StrokeDist1_;
	tuplet1_ = nbaseLinePoint2_ + StrokeDist2_;
	
	// decide about accent alignment
	
	if (status_ & PROP_BEAMED) {
		for (i = 0; i < 5; ++i) {
			flag_pos_[i] = QPoint(xbpoint, (int) (m_ * xbpoint + n_) + ((status_ & PROP_STEM_UP) ? FLAG_DIST : -FLAG_DIST) * i);
		}
	}
	else {
		for (i = 0; i < 5; ++i) {
			flag_pos_[i] = QPoint(xbpoint, nbaseLinePoint2_.y() + i * FLAG_DIST);
		}
	}
		
	pixmapWidth_ += x_aux_offs+x_shift_offs + x_acc_offs;
	bbox_ = QRect(xpos_, staff_props_->base - pixmapHeight_ -  LINE_DIST / 2 + 5 * LINE_DIST - min_line * LINE_DIST  / 2,
		pixmapWidth_, pixmapHeight_);
}

int NChord::getTrillEnd() {
	if (!trill_) {
		NResource::abort("getTrillEnd: internal error");
	}
	if (trill_ > 0) {
		return acc_point_.x() + trill_ * NResource::trillPixmap_->width();
	}
	return TR_SIGN_WIDTH + acc_point_.x() + (-trill_-1) * NResource::trillPixmap_->width();
}

int NChord::getVaEnd() {
	if (!va_) {
		NResource::abort("getVaEnd: internal error");
	}
	if (va_ > 0) {
		return acc_point_.x() + va_ * VA_LINE_LEN + VA_LINE_DASH_LEN;
	}
	return acc_point_.x() + (( -va_ + 1 ) * VA_LINE_LEN) + VA_LINE_DASH_LEN;
}

int NChord::getDynamicEnd() {
	if (!dynamic_) {
		NResource::abort("getDynamicEnd internal error");
	}
	return xpos_ + dynamic_;
}

QPoint *NChord::getTopY() {
	return &nbaseLinePoint2_;
}

int NChord::getTopY2() {
	if (status_ & PROP_STEM_UP) return nbaseLinePoint2_.y();
	return nbaseLinePoint1_.y() - 10;
}

int NChord::getTopY3() {
	int ytop = (status_ & PROP_STEM_UP) ? nbaseLinePoint2_.y() : nbaseLinePoint1_.y() - 10;
	return (ytop < staff_props_->base) ? ytop : staff_props_->base;
}

int NChord::getTopX2() {
	if (status_ & PROP_STEM_UP) return nbaseLinePoint2_.x();
	return  nbaseLinePoint2_.x() + NResource::nbasePixmapWidth2_;
}

int NChord::getRefY() {
	if (status_ & PROP_STEM_UP) return staff_props_->base + 4 * LINE_DIST - noteList_.last()->line * LINE_DIST  / 2;
	return staff_props_->base + 4 * LINE_DIST - noteList_.first()->line * LINE_DIST  / 2;
}

double NChord::getBotY() {
	return staff_props_->base - (double) ((noteList_.first()->line - 4) * LINE_DIST) / 2.0;
}


void NChord::addChordDiagram(NChordDiagram *cdiag) {
	if (cdiagram_) {
		delete cdiagram_;
	}
	cdiagram_ = cdiag;
}

NNote *NChord::getActualNote() {
	NNote *note;
	note = noteList_.at(actualNote_);
	if (note == NULL) {
		NResource::abort("getActualNote: internal error");
	}
	return note;
}

void NChord::removeChordDiagram() {
	if (cdiagram_) {
		delete cdiagram_;
		cdiagram_ = 0;
	}
}


void NChord::draw(int flags) {
#define SHORT_BEAM_LENGTH 24
	if (flags &  (DRAW_DIRECT_RED | DRAW_DIRECT_BLACK)) {
		if (bbox_.right() < main_props_->directPainter->getLeftBorder()) return;
	}
	if (status_ & PROP_GRACE) {
		drawGraceChord(flags);
		return;
	}

	int i, j;
	NNote *note;
	QPointArray pa(4);
	NTransPainter *the_painter = (flags & (DRAW_DIRECT_RED | DRAW_DIRECT_BLACK)) ? main_props_->directPainter : main_props_->tp;
	the_painter->beginTranslated();
	QPoint nextPoint;
	int nextx;

	if (flags & DRAW_DIRECT_RED) {
		the_painter->setPen(NResource::redPen_);
	}
	else if (flags & DRAW_INDIRECT_GREY) {
		the_painter->setPen(NResource::greyPen_);
	}
	else {
		the_painter->setPen(NResource::blackPen_);
	}
	for (j = 0, note = noteList_.first(); note; note = noteList_.next(), j++) {
		the_painter->drawPixmap (note->nbase_draw_point, ((actual_ && j == actualNote_) || (flags & DRAW_DIRECT_RED)) ?
			 *(note->redBodyPixmap) : ((flags & DRAW_INDIRECT_GREY) ?  *(note->greyBodyPixmap) : *(note->bodyPixmap)));
		if (flags & DRAW_DIRECT_RED) continue;
		if (status_ & DOT_MASK) {
			the_painter->setBrush(actual_  ? NResource::redBrush_ : NResource::blackBrush_ );
			the_painter->drawPie(note->point_pos1, 0, 360*16);
			if ((status_ & DOT_MASK) > 1) {
				the_painter->drawPie(note->point_pos2, 0, 360*16);
			}
		}
		if ((note->status & PROP_TIED) && note->tie_forward) {
			/* if there's no poliphony, ties are at the opposite side of stems,
			   if there is polyphony, ties are at the same side as stems */
			if ( ((status_ & PROP_STEM_UP) && (voice_->stemPolicy_ == STEM_POL_INDIVIDUAL))
			    || (!(status_ & PROP_STEM_UP) && (voice_->stemPolicy_ == STEM_POL_DOWN)) ) {
		    	pa.setPoint(0, note->tie_start_point_up);
				pa.setPoint(1, note->tie_forward_point_up);
				pa.setPoint(2, note->tie_forward->tie_back_point_up);
				pa.setPoint(3, note->tie_forward->tie_start_point_up);
			} else {
				pa.setPoint(0, note->tie_start_point_down);
				pa.setPoint(1, note->tie_forward_point_down);
				pa.setPoint(2, note->tie_forward->tie_back_point_down);
				pa.setPoint(3, note->tie_forward->tie_start_point_down);
			}
#if QT_VERSION >= 300
			the_painter->drawCubicBezier(pa);
#else
			the_painter->drawQuadBezier(pa);
#endif
		}
		if (note->line < -1) {
			for (i = 0; i < -note->line / 2; ++i) {
				the_painter->drawLine(xpos_+note->acc_offs, staff_props_->base + (5+i) * LINE_DIST, xpos_+note->acc_offs + 2*AUX_L_2, staff_props_->base + (5+i) * LINE_DIST);
			}
		} 
		else if (note->line > 9) {
			for (i = 0; i < (note->line - 8) / 2; ++i) {
				the_painter->drawLine(xpos_+note->acc_offs, staff_props_->base - (i+1) * LINE_DIST, xpos_+note->acc_offs + 2*AUX_L_2, staff_props_->base - (i+1) * LINE_DIST);
			}
		}
		switch (note->status & ACC_MASK) {
			case PROP_CROSS:
			the_painter->drawPixmap (note->acc_draw_point, (flags & DRAW_INDIRECT_GREY) ? *NResource::crossGreyPixmap_ : *NResource::crossPixmap_);
			break;
			case PROP_FLAT:
			the_painter->drawPixmap (note->acc_draw_point, (flags & DRAW_INDIRECT_GREY) ?  *NResource::flatGreyPixmap_ : *NResource::flatPixmap_ );
			break;
			case PROP_DCROSS:
			the_painter->drawPixmap (note->acc_draw_point, (flags & DRAW_INDIRECT_GREY) ?  *NResource::dcrossGreyPixmap_ : *NResource::dcrossPixmap_ );
			break;
			case PROP_DFLAT:
			the_painter->drawPixmap (note->acc_draw_point, (flags & DRAW_INDIRECT_GREY) ?  *NResource::dflatGreyPixmap_ : *NResource::dflatPixmap_ );
			break;
			case PROP_NATUR:
			the_painter->drawPixmap (note->acc_draw_point, (flags & DRAW_INDIRECT_GREY) ? *NResource::naturGreyPixmap_  : *NResource::naturPixmap_ );
			break;
		}
	}
	if (length_ < WHOLE_LENGTH) {
		the_painter->drawLine(nbaseLinePoint1_, (status_ & PROP_BEAMED)  ? nbaseLinePoint3_ : nbaseLinePoint2_);
	}
	if (flags & (DRAW_DIRECT_RED | DRAW_DIRECT_BLACK)) {
		the_painter->end();
		return;
	}
	if (status_ & PROP_STACC) {
		the_painter->setBrush(actual_ ? NResource::redBrush_ : NResource::blackBrush_);
		the_painter->drawPie(stacc_point_, 0, 360*16);
	}
	
	
	if (status_ & PROP_SFORZ)
		// sforzato draw
		if( u1_.setAccentAboveChord_ )
			the_painter->drawPixmap (QPoint(acc_point_.x() - (NResource::sforzatoAbPixmap_->width() / 2 ), acc_point_.y() - (NResource::sforzatoAbPixmap_->height() / 2)), actual_ ? *NResource::sforzatoAbRedPixmap_ :
					    *NResource::sforzatoAbPixmap_ );
		else
			the_painter->drawPixmap (QPoint(acc_point_.x() - (NResource::sforzatoBePixmap_->width() / 2 ), acc_point_.y()), actual_ ? *NResource::sforzatoBeRedPixmap_ :
					    *NResource::sforzatoBePixmap_ );
					    
	if (status_ & PROP_PORTA) 
		// portato draw
		the_painter->drawPixmap (QPoint(acc_point_.x() - (NResource::portatoPixmap_->width() / 2 ), acc_point_.y()), actual_ ? *NResource::portatoRedPixmap_ : 
				    *NResource::portatoPixmap_ );	

	if (status_ & PROP_STPIZ)
		// strong pizzicato draw
		if( u1_.setAccentAboveChord_ )
			the_painter->drawPixmap (QPoint(acc_point_.x() - (NResource::strong_pizzicatoAbPixmap_->width() / 2 ), acc_point_.y() - (NResource::strong_pizzicatoAbPixmap_->height() / 2)), actual_ ? *NResource::strong_pizzicatoAbRedPixmap_ :
					    *NResource::strong_pizzicatoAbPixmap_ );
		else
			the_painter->drawPixmap (QPoint(acc_point_.x() - (NResource::strong_pizzicatoBePixmap_->width() / 2 ), acc_point_.y()), actual_ ? *NResource::strong_pizzicatoBeRedPixmap_ :
					    *NResource::strong_pizzicatoBePixmap_ );

	if (status_ & PROP_SFZND) 
		// sforzando draw
		the_painter->drawPixmap (QPoint(acc_point_.x() - (NResource::sforzandoPixmap_->width() / 2 ), u1_.setAccentAboveChord_ ? acc_point_.y() - (NResource::sforzandoPixmap_->height() / 2) : acc_point_.y() ), actual_ ? *NResource::sforzandoRedPixmap_ : 
				    *NResource::sforzandoPixmap_ );	
	if (status_ & PROP_PEDAL_ON) 
		// pedal on draw
		the_painter->drawPixmap (pedal_point_, actual_ ? *NResource::pedonRedPixmap_ : *NResource::pedonPixmap_ );	
	if (status_ & PROP_PEDAL_OFF) 
		// pedal off draw
		the_painter->drawPixmap (pedal_point_, actual_ ? *NResource::pedoffRedPixmap_ : *NResource::pedoffPixmap_ );	

	if (status_ & PROP_FERMT)
		// fermate draw
		if( u1_.setAccentAboveChord_ )
			the_painter->drawPixmap (QPoint(acc_point_.x() - (NResource::fermateAbPixmap_->width() / 2 ), acc_point_.y() - (NResource::fermateAbPixmap_->height() / 4)), actual_ ? *NResource::fermateAbRedPixmap_ :
					    *NResource::fermateAbPixmap_ );
		else
			the_painter->drawPixmap (QPoint(acc_point_.x() - (NResource::fermateBePixmap_->width() / 2 ), acc_point_.y() - (NResource::fermateAbPixmap_->height() / 3)), actual_ ? *NResource::fermateBeRedPixmap_ :
					    *NResource::fermateBePixmap_ );

	if (status_ & PROP_ARPEGG) {
		for (i = 0; i < arpeggParts_; i++) {
			the_painter->drawPixmap(arpeggDrawPoint_ + QPoint(0, i * NResource::arpegPixmapHeight_), *NResource::arpeggPixmap_);
		}
	}
	if (trill_) {
	    // paints the trill equipment
	    int isneg = TR_SIGN_WIDTH;
	    if (trill_ > 0) {
		the_painter->toggleToScaledText(true);
		the_painter->setFont( main_props_->scaledBoldItalic_ );
		the_painter->drawScaledText( acc_point_.x(), trilly_ + ADD_DIST, "tr");
		the_painter->toggleToScaledText(false);
	    }
	    else {
		isneg = 0;
		trill_ = 1+-trill_;
	    }
	    for (int i = 1; i < trill_; ++i)
	    /*
		the_painter->drawPixmap (QPoint(isneg + acc_point_.x() + (( i - 1 ) * NResource::trillPixmap_->width()), trilly_), actual_ ? *NResource::trillRedPixmap_ : *NResource::trillPixmap_);
		*/
		the_painter->drawPixmap (QPoint(isneg + acc_point_.x() + (( i - 1 ) * NResource::trillPixmap_->width()), trilly_), *NResource::trillPixmap_);
	    if (!isneg)
		trill_ = 1-trill_;
	}
	if (va_ > 0) {
	    // paints the va equipment
	    the_painter->toggleToScaledText(true);
	    the_painter->setFont( main_props_->scaledBoldItalic_ );
	    the_painter->drawScaledText( acc_point_.x(), vaY_ + ADD_DIST, "8va");
	    the_painter->toggleToScaledText(false);
	    for (int i = 1; i < va_; ++i)
		the_painter->drawLine(acc_point_.x() + (( i + 1 ) * VA_LINE_LEN), vaY_,
				      acc_point_.x() + (( i + 1 ) * VA_LINE_LEN) + VA_LINE_DASH_LEN, vaY_);
	}
	if (va_ < 0) {
	    // paints the va bassa equipment
	    the_painter->toggleToScaledText(true);
	    the_painter->setFont( main_props_->scaledBoldItalic_ );
	    the_painter->drawScaledText( acc_point_.x(), vaY_ - ADD_DIST, "8va bassa");
	    the_painter->toggleToScaledText(false);
	    for (int i = -2; i > va_; --i)
		the_painter->drawLine(acc_point_.x() + (( -i + 2 ) * VA_LINE_LEN), vaY_,
				      acc_point_.x() + (( -i + 2 ) * VA_LINE_LEN) + VA_LINE_DASH_LEN, vaY_);
	}

	if ( dynamic_ ) {
	    // painting crescendo...
	    if( dynamicAlign_ ) {
		the_painter->drawLine( QPoint( acc_point_.x() + dynamic_, staff_props_->base + DYNAMIC_DIST + ( LINE_DIST * 5 ) ), QPoint( acc_point_.x(), staff_props_->base + ( DYNAMIC_WIDTH / 2 ) + ( LINE_DIST * 5 ) + DYNAMIC_DIST) );
		the_painter->drawLine( QPoint( acc_point_.x() + dynamic_, staff_props_->base + DYNAMIC_DIST + ( LINE_DIST * 5 ) + DYNAMIC_WIDTH ), QPoint( acc_point_.x(), staff_props_->base + ( DYNAMIC_WIDTH / 2 ) + ( LINE_DIST * 5 ) + DYNAMIC_DIST ) );
	    } else {
		the_painter->drawLine( QPoint( xpos_, staff_props_->base + DYNAMIC_DIST + ( LINE_DIST * 5 )), QPoint(xpos_ + dynamic_, staff_props_->base + ( DYNAMIC_WIDTH / 2 ) + ( LINE_DIST * 5 ) + DYNAMIC_DIST));
		the_painter->drawLine( QPoint( xpos_, staff_props_->base + DYNAMIC_DIST + ( LINE_DIST * 5 ) + DYNAMIC_WIDTH), QPoint(xpos_ + dynamic_, staff_props_->base + ( DYNAMIC_WIDTH / 2 ) + (LINE_DIST * 5 ) + DYNAMIC_DIST));
		}
		xpos_ + dynamic_;

	}

	if ((status_ & PROP_SLURED) && slur_forward_) {
		/* if there's no poliphony, slurs are at the opposite side of stems,
		   if there is polyphony, slurs are at the same side as stems */
		if ( ((status_ & PROP_STEM_UP) && (voice_->stemPolicy_ == STEM_POL_INDIVIDUAL))
		    || (!(status_ & PROP_STEM_UP) && (voice_->stemPolicy_ == STEM_POL_DOWN)) ) {
			pa.setPoint(0, slur_start_point_up_);
			pa.setPoint(1, slur_forward_point_up_);
			pa.setPoint(2, slur_forward_->slur_back_point_up_);
			pa.setPoint(3, slur_forward_->slur_start_point_up_);
		} else {
			pa.setPoint(0, slur_start_point_down_);
			pa.setPoint(1, slur_forward_point_down_);
			pa.setPoint(2, slur_forward_->slur_back_point_down_);
			pa.setPoint(3, slur_forward_->slur_start_point_down_);
		}
#if QT_VERSION >= 300
		the_painter->drawCubicBezier(pa);
#else
		the_painter->drawQuadBezier(pa);
#endif
	}
	if (status_ & PROP_BEAMED) {
		if (flags & DRAW_DIRECT_RED) {
			the_painter->setPen(NResource::redWidePen_);
		}
		else if (flags & DRAW_INDIRECT_GREY) {
			the_painter->setPen(NResource::greyWidePen_);
		}
		else {
			the_painter->setPen(NResource::blackWidePen_);
		}
		if (nextBeamedChord_) {
			for (i = 0;  i < flagCount_; ++i) {
				if (nextBeamedChord_->flagCount_ > i) {
					nextx = nextBeamedChord_->nbaseLinePoint2_.x() - 1;
				}
				else {
					nextx = nbaseLinePoint2_.x() + SHORT_BEAM_LENGTH;
				}
				nextPoint = QPoint(nextx, (int) (m_ * nextx + n_) + ((status_ & PROP_STEM_UP) ? FLAG_DIST : -FLAG_DIST)  * i);
				the_painter->drawLine(flag_pos_[i], nextPoint);
			}
		}
		else {
			for (i = 0;  i < flagCount_; ++i) {
				nextx = nbaseLinePoint2_.x() - SHORT_BEAM_LENGTH;	
				nextPoint = QPoint((int) nextx, (int) (m_ * nextx + n_) + ((status_ & PROP_STEM_UP) ? FLAG_DIST : -FLAG_DIST) * i);
				the_painter->drawLine(nextPoint, flag_pos_[i]);
			}
		}
	}
	else {
		if (status_ & PROP_STEM_UP) {
			for (i = 0;  i < flagCount_; ++i) {
				the_painter->drawPixmap (flag_pos_[i], (flags & DRAW_DIRECT_RED) ?
					 *NResource::flagRedPixmap_ : ((flags & DRAW_INDIRECT_GREY) ?
					 *NResource::flagGreyPixmap_ : *NResource::flagPixmap_));
			}
		}
		else {
			for (i = 0;  i < flagCount_; ++i) {
				the_painter->drawPixmap (flag_pos_[i], (flags & DRAW_DIRECT_RED) ?
					 *NResource::flagDownRedPixmap_ : ((flags & DRAW_INDIRECT_GREY) ?
					 *NResource::flagDownGreyPixmap_ : *NResource::flagDownPixmap_));
			}
		}
	}
	if (status_ & PROP_LAST_TUPLET) {
		the_painter->drawPixmap(tupletDigit_, *tupletMarker_);
		if (!(status_ & PROP_BEAMED)) {
			the_painter->setPen((flags & DRAW_INDIRECT_GREY) ? NResource::greyWidePen_ : NResource::blackWidePen_);
			the_painter->drawLine(tuplet00_, tuplet0_);
			the_painter->drawLine(tuplet0_, tuplet1_);
			the_painter->drawLine(tuplet1_ ,tuplet01_);
		}
	}
	if (lyrics_) {
		the_painter->toggleToScaledText(true);
		the_painter->setFont( main_props_->scaledText_ );
		the_painter->setPen(NResource::lyricPen_);
		for (i = 0; i < NUM_LYRICS; ++i) {
			if (lyrics_[i]) {
				/* removes the syntax characters like <, > and * */
				QString* printedLyrics = new QString(*lyrics_[i]);
				printedLyrics->remove('<');
				printedLyrics->remove('>');
				printedLyrics->remove('*');
				
				/* render lyrics if not empty */
				if (!printedLyrics->isEmpty()) {
					the_painter->drawScaledText(*(lyricsPoints_[i]), *printedLyrics);
				}
				delete printedLyrics;
			}
		}
	}
	if (cdiagram_) {
		cdiagram_->draw(the_painter, &cdiagramDrawPoint_, main_props_);
	}
	the_painter->end();
}

void NChord::drawGraceChord(int flags) {
#define SHORT_BEAM_LENGTH 24
	int i, j;
	NNote *note;
	QPointArray pa(4);
	NTransPainter *the_painter = (flags & (DRAW_DIRECT_RED | DRAW_DIRECT_BLACK)) ? main_props_->directPainter : main_props_->tp;
	the_painter->beginTranslated();
	QPoint nextPoint;
	int nextx;

	if (flags & DRAW_DIRECT_RED) {
		the_painter->setPen(NResource::redPen_);
	}
	else if (flags & DRAW_INDIRECT_GREY) {
		the_painter->setPen(NResource::greyPen_);
	}
	else {
		the_painter->setPen(NResource::blackPen_);
	}
	for (j = 0, note = noteList_.first(); note; note = noteList_.next(), j++) {
		the_painter->drawPixmap (note->nbase_draw_point, ((actual_ &&  j == actualNote_)  || (flags & DRAW_DIRECT_RED)) ?
			 *(note->redBodyPixmap) : ((flags & DRAW_INDIRECT_GREY) ?  *(note->greyBodyPixmap) : *(note->bodyPixmap)));
		if (flags & DRAW_DIRECT_RED) continue;
		if (status_ & DOT_MASK) {
			the_painter->setBrush(actual_  ? NResource::redBrush_ : NResource::blackBrush_ );
			the_painter->drawPie(note->point_pos1, 0, 360*16);
			if ((status_ & DOT_MASK) > 1) {
				the_painter->drawPie(note->point_pos2, 0, 360*16);
			}
		}
		if ((note->status & PROP_TIED) && note->tie_forward) {
			/* if there's no poliphony, ties are at the opposite side of stems,
			   if there is polyphony, ties are at the same side as stems */
			if ( ((status_ & PROP_STEM_UP) && (voice_->stemPolicy_ == STEM_POL_INDIVIDUAL))
		        || (!(status_ & PROP_STEM_UP) && (voice_->stemPolicy_ == STEM_POL_DOWN)) ) {
		    	pa.setPoint(0, note->tie_start_point_up);
				pa.setPoint(1, note->tie_forward_point_up);
				pa.setPoint(2, note->tie_forward->tie_back_point_up);
				pa.setPoint(3, note->tie_forward->tie_start_point_up);
			}
			else {
				pa.setPoint(0, note->tie_start_point_down);
				pa.setPoint(1, note->tie_forward_point_down);
				pa.setPoint(2, note->tie_forward->tie_back_point_down);
				pa.setPoint(3, note->tie_forward->tie_start_point_down);
			}
#if QT_VERSION >= 300
			the_painter->drawCubicBezier(pa);
#else
			the_painter->drawQuadBezier(pa);
#endif
		}
		if (note->line < -1) {
			for (i = 0; i < -note->line / 2; ++i) {
				the_painter->drawLine(xpos_+note->acc_offs, staff_props_->base + (5+i) * LINE_DIST, xpos_+note->acc_offs + 2*AUX_L_2, staff_props_->base + (5+i) * LINE_DIST);
			}
		} 
		else if (note->line > 9) {
			for (i = 0; i < (note->line - 8) / 2; ++i) {
				the_painter->drawLine(xpos_+note->acc_offs, staff_props_->base - (i+1) * LINE_DIST, xpos_+note->acc_offs + 2*AUX_L_2, staff_props_->base - (i+1) * LINE_DIST);
			}
		}
		switch (note->status & ACC_MASK) {
			case PROP_CROSS:
			the_painter->drawPixmap (note->acc_draw_point, (flags & DRAW_INDIRECT_GREY) ? *NResource::crossGreyPixmap_ : *NResource::crossPixmap_);
			break;
			case PROP_FLAT:
			the_painter->drawPixmap (note->acc_draw_point, (flags & DRAW_INDIRECT_GREY) ?  *NResource::flatGreyPixmap_ : *NResource::flatPixmap_ );
			break;
			case PROP_DCROSS:
			the_painter->drawPixmap (note->acc_draw_point, (flags & DRAW_INDIRECT_GREY) ?  *NResource::dcrossGreyPixmap_ : *NResource::dcrossPixmap_ );
			break;
			case PROP_DFLAT:
			the_painter->drawPixmap (note->acc_draw_point, (flags & DRAW_INDIRECT_GREY) ?  *NResource::dflatGreyPixmap_ : *NResource::dflatPixmap_ );
			break;
			case PROP_NATUR:
			the_painter->drawPixmap (note->acc_draw_point, (flags & DRAW_INDIRECT_GREY) ? *NResource::naturGreyPixmap_  : *NResource::naturPixmap_ );
			break;
		}
	}
	the_painter->drawLine(nbaseLinePoint1_, (status_ & PROP_BEAMED)  ? nbaseLinePoint3_ : nbaseLinePoint2_);
	if (length_ == INTERNAL_MARKER_OF_STROKEN_GRACE) {
		the_painter->drawLine(tuplet0_, tuplet1_);
	}
	if (flags & (DRAW_DIRECT_RED | DRAW_DIRECT_BLACK)) {
		the_painter->end();
		return;
	}
	if (status_ & PROP_STACC) {
		the_painter->setBrush(actual_ ? NResource::redBrush_ : NResource::blackBrush_);
		the_painter->drawPie(stacc_point_, 0, 360*16);
	}
	
	if ((status_ & PROP_SLURED) && slur_forward_) {
		pa.setPoint(0, slur_start_point_up_);
		pa.setPoint(1, slur_forward_point_up_);
		pa.setPoint(2, slur_forward_->slur_back_point_up_);
		pa.setPoint(3, slur_forward_->slur_start_point_up_);
#if QT_VERSION >= 300
		the_painter->drawCubicBezier(pa);
#else
		the_painter->drawQuadBezier(pa);
#endif
	}
	if (status_ & PROP_BEAMED) {
		if (flags & DRAW_DIRECT_RED) {
			the_painter->setPen(NResource::redWidePen_);
		}
		else if (flags & DRAW_INDIRECT_GREY) {
			the_painter->setPen(NResource::greyWidePen_);
		}
		else {
			the_painter->setPen(NResource::blackWidePen_);
		}
		if (nextBeamedChord_) {
			for (i = 0;  i < flagCount_; ++i) {
				if (nextBeamedChord_->flagCount_ > i) {
					nextx = nextBeamedChord_->nbaseLinePoint2_.x() - 1;
				}
				else {
					nextx = nbaseLinePoint2_.x() + SHORT_BEAM_LENGTH;
				}
				nextPoint = QPoint(nextx, (int) (m_ * nextx + n_) + ((status_ & PROP_STEM_UP) ? FLAG_DIST : -FLAG_DIST)  * i);
				the_painter->drawLine(flag_pos_[i], nextPoint);
			}
		}
		else {
			for (i = 0;  i < flagCount_; ++i) {
				nextx = nbaseLinePoint2_.x() - SHORT_BEAM_LENGTH;	
				nextPoint = QPoint((int) nextx, (int) (m_ * nextx + n_) + ((status_ & PROP_STEM_UP) ? FLAG_DIST : -FLAG_DIST) * i);
				the_painter->drawLine(nextPoint, flag_pos_[i]);
			}
		}
	}
	else {
		for (i = 0;  i < flagCount_; ++i) {
			the_painter->drawPixmap (flag_pos_[i], (flags & DRAW_DIRECT_RED) ?
				 *NResource::tinyFlagRedPixmap_ : ((flags & DRAW_INDIRECT_GREY) ?
				 *NResource::tinyFlagGreyPixmap_ : *NResource::tinyFlagPixmap_));
		}
	}
	if (status_ & PROP_LAST_TUPLET) {
		the_painter->drawPixmap(tupletDigit_, *tupletMarker_);
		if (!(status_ & PROP_BEAMED)) {
			the_painter->setPen((flags & DRAW_INDIRECT_GREY) ? NResource::greyWidePen_ : NResource::blackWidePen_);
			the_painter->drawLine(tuplet00_, tuplet0_);
			the_painter->drawLine(tuplet0_, tuplet1_);
			the_painter->drawLine(tuplet1_ ,tuplet01_);
		}
	}
	the_painter->end();
}
	
void NChord::moveUp(int up, int voices_stem_policy, NKeySig *key) {
	NNote *note, *note2;
	note = noteList_.at(actualNote_);
	if (note == NULL) {
		NResource::abort("moveUp: internal error");
	}
	if (note->line + up > MAXLINE) return;
	note2 = noteList_.next();
	if (note2) {
		if (note->line + up >= note2->line) return;
	}
	note->line += up;
	if (NResource::moveAccKeysig_) {
		note->offs = key->getOffset(note->line);
	}
	SET_STATUS((main_props_->actualStemDir == STEM_DIR_AUTO && noteList_.first()->line < 4) || main_props_->actualStemDir == STEM_DIR_UP, status_, PROP_STEM_UP);
	STEM_LOGIC(noteList_.first()->line);
}

void NChord::moveDown(int down, int voices_stem_policy, NKeySig *key) {
	NNote *note, *note2;
	note = noteList_.at(actualNote_);
	if (note == NULL) {
		NResource::abort("moveDown: internal error");
	}
	if (note->line - down < MINLINE) return;
	note2 = noteList_.prev();
	if (note2) {
		if (note->line - down <= note2->line) return;
	}
	note->line -= down;
	if (NResource::moveAccKeysig_) {
		note->offs = key->getOffset(note->line);
	}
	SET_STATUS((main_props_->actualStemDir == STEM_DIR_AUTO && noteList_.first()->line < 4) || main_props_->actualStemDir == STEM_DIR_UP, status_, PROP_STEM_UP);
	STEM_LOGIC(noteList_.first()->line);
}

void NChord::moveSemiToneUp(int voices_stem_policy, NClef *clef, NKeySig *ksig) {
	NNote *note, *note2;
	int new_line, new_offs;
	unsigned int pitch;
	note = noteList_.at(actualNote_);
	if (note == NULL) {
		NResource::abort("moveSemiToneUp: internal error");
	}
	pitch = clef->line2Midi( note->line, note->offs );
	pitch++;
	clef->midi2Line(pitch, &new_line, &new_offs, ksig->getSubType());

	if (new_line >= MAXLINE) return;
	note2 = noteList_.next();
	if (note2) {
		if (new_line >= note2->line) return;
	}
	note->line = new_line;
	note->offs = new_offs;
	SET_STATUS((main_props_->actualStemDir == STEM_DIR_AUTO && noteList_.first()->line < 4) || main_props_->actualStemDir == STEM_DIR_UP, status_, PROP_STEM_UP);
	STEM_LOGIC(noteList_.first()->line);
}

void NChord::moveSemiToneDown(int voices_stem_policy, NClef *clef, NKeySig *ksig) {
	NNote *note, *note2;
	int new_line, new_offs;
	unsigned int pitch;
	note = noteList_.at(actualNote_);
	if (note == NULL) {
		NResource::abort("moveSemiToneDown: internal error");
	}
	pitch = clef->line2Midi( note->line, note->offs );
	pitch--;
	clef->midi2Line(pitch, &new_line, &new_offs, ksig->getSubType());
	if (new_line < MINLINE) return;
	note2 = noteList_.prev();
	if (note2) {
		if (new_line <= note2->line) return;
	}
	note->line = new_line;
	note->offs = new_offs;
	SET_STATUS((main_props_->actualStemDir == STEM_DIR_AUTO && noteList_.first()->line < 4) || main_props_->actualStemDir == STEM_DIR_UP, status_, PROP_STEM_UP);
	STEM_LOGIC(noteList_.first()->line);
}


QString *NChord::computeTeXBeam(int maxBeams, unsigned int *beamPool, int *beamNr, int *beamCount, NClef *clef, int maxflags, bool *problem128, bool *toomany) {
	QString *s, t;
	int i, j;
	bool found = false;
	int fc1, fc2 = 0;
	char beamdir;
	int line1, line2;
	NChord *lastChordInBeam;
	int y_thoeretical1, y_thoeretical2;
	double y_practical1, y_practical2;
	*toomany = false;
	*problem128 = false;
	if (!beamList_) {
		NResource::abort("internal error: computeBeamInfo: beamList_ == 0");
	}
	fc1 = flagCount_;
	if (nextBeamedChord_) {
		fc2 = nextBeamedChord_->flagCount_;
	}
	if (fc1 > maxflags) {
		fc1 = maxflags;
		*problem128 = true;
	}
	if (fc2 > maxflags) {
		fc2 = maxflags;
		*problem128 = true;
	}
	beamdir = (status_ & PROP_STEM_UP) ? 'u' : 'l';
	if (this == beamList_->first()) {
		i = 0; found = false;
		while (i < maxBeams && !found) {
			if (!(found = ((1 << i) & (*beamPool)) == 0)) ++i;
		}
		if (!found) {
			*toomany = true;
			*beamNr = -1;
			return 0;
		}
		*beamPool |= (1 << i);
		*beamNr = i;
			
/*
		s = new QString("\\i");
*/
		s = new QString("\\I");
		for (i = 0; i < fc1; ++i) {
			*s += QString("b");
		}
			
		line1 = (status_ & PROP_STEM_UP) ? noteList_.last()->line : noteList_.first()->line;
		y_thoeretical1 = nbaseLinePoint2_.y();
		y_practical1 = m_ * nbaseLinePoint2_.x() + n_;
		line1 += (int) ((y_thoeretical1 - y_practical1) / (((double) LINE_DIST) / 2.0));
		if (line1 < MINLINE || line1 > MAXLINE) {
			printf("NChord::computeTeXBeam: line1 out of range\n");
			if (line1 < MINLINE) line1 = MINLINE;
			else if (line1 > MAXLINE) line1 =  MAXLINE;
		}
		lastChordInBeam = beamList_->last();
		line2 = (status_ & PROP_STEM_UP) ? lastChordInBeam->noteList_.last()->line : lastChordInBeam->noteList_.first()->line;
		y_thoeretical2 = lastChordInBeam->nbaseLinePoint2_.y();
		y_practical2 = m_ * lastChordInBeam->nbaseLinePoint2_.x() + n_;
		line2 += (int) ((y_thoeretical2 - y_practical2) / (((double) LINE_DIST) / 2.0));
		if ((lastChordInBeam->status_ & PROP_STEM_UP) && !(status_ & PROP_STEM_UP)) line2 += 14;
		else if (!(lastChordInBeam->status_ & PROP_STEM_UP) && (status_ & PROP_STEM_UP)) line2 -= 14;
		if (line2 < MINLINE || line2 > MAXLINE) {
			printf("NChord::computeTeXBeam: line2 out of range\n");
			if (line2 < MINLINE) line2 = MINLINE;
			else if (line2 > MAXLINE) line2 =  MAXLINE;
		}
/*
		t.sprintf("%c%d%c{%d}", beamdir, *beamNr, clef->line2TexTab_[line+LINE_OVERFLOW], 
				(int) (atan(-m_) * 180.0 / 3.1415 / 3.0));
*/
		t.sprintf("%c%d%c%c{%d}", beamdir, *beamNr, clef->line2TexTab_[line1+LINE_OVERFLOW],
			 clef->line2TexTab_[line2+LINE_OVERFLOW], lastChordInBeam->sequNr_ - sequNr_);
		*s += t;
		*beamCount = fc1;
		if (fc2 < fc1) {
			*s += QString("\\roff{");
			for (i = fc1; i > fc2; i--) {
				*s += QString("\\t");
				for (j = i; j > 0; j--) {
					*s += QString("b");
				}
				t.sprintf("%c%d", beamdir, *beamNr);
				*s += t;
			}
			*s += QString("}");
			*beamCount = fc2;
		}
		return s;
	}
	if (nextBeamedChord_ == 0) {
		s = new QString();
		for (i = fc1; i > 0; i--) {
			*s += QString("\\t");
			for (j = i; j > 0; j--) {
				*s += QString("b");
			}
			t.sprintf("%c%d", beamdir, *beamNr);
			*s += t;
		}
		*beamCount = 0;
		if (((1 << (*beamNr)) & (*beamPool)) == 0) {
			NResource::abort("NChord::computeTeXBeam: internal error\n", 1);
		}
		*beamPool &= (~(1 << (*beamNr)));
		return s;
	}
	if (*beamCount < fc1) {
		s = new QString("\\n");
		for (i = 0; i < fc1; ++i) {
			*s += QString("b");
		}
		t.sprintf("%c%d", beamdir, *beamNr);
		*s += t;
		*beamCount = fc1;
		if (fc2 < fc1) {
			*s += QString("\\roff{");
			for (i = fc1; i > fc2; i--) {
				*s += QString("\\t");
				for (j = i; j > 0; j--) {
					*s += QString("b");
				}
				t.sprintf("%c%d", beamdir, *beamNr);
				*s += t;
			}
			*s += QString("}");
			*beamCount = fc2;
		}
		return s;
	}
	if (fc2 < fc1) {
		s = new QString();
		for (i = fc1; i > fc2; i--) {
			*s += QString("\\t");
			for (j = i; j > 0; j--) {
				*s += QString("b");
			}
			t.sprintf("%c%d", beamdir, *beamNr);
			*s += t;
		}
		*beamCount = fc2;
		return s;
	}
	if (*beamCount > fc1) {
		printf("beamCount = %d, fc1 = %d\n", *beamCount, fc1);
		NResource::abort("internal error: computeBeamInfo: *beamCount > flagCount_");
	}
	return 0;
}


QString *NChord::computeTeXTie(unsigned int *tiePool, NClef *clef, int maxtie, bool *toomany, bool spare) {
	int tieNr = 0;
	char tieDir;
	bool found = false;
	NNote *note, *firstTiedPart = 0, *lastTiedPart = 0;
	QString *s = 0, t;
	*toomany = false;
	char *err = "internal error: too many ties";

	if (spare) {
		for (note = noteList_.first(); note; note = noteList_.next()) {
			if (note->status & PROP_PART_OF_TIE) {
				note->TeXTieNr = note->tie_backward->TeXTieNr;
				if (note->TeXTieNr >= 0) {
					*toomany = *toomany || note->TeXTieNr >= maxtie;
					if (note->TeXTieNr < maxtie) {
						t.sprintf("\\ttie%d", note->TeXTieNr);
						if (s == 0) s = new QString();
						*s += t;
					}
					(*tiePool) = (*tiePool) & (~(1 << note->TeXTieNr));
				}
			}
		}
		for (note = noteList_.first(); note; note = noteList_.next()) {
			if (note->status & PROP_TIED) {
				note->TeXTieNr = -1;
				if (!firstTiedPart) firstTiedPart = note;
				else lastTiedPart = note;
			}
		}
		if (firstTiedPart) {
			tieNr = 0;
			found = false;
			while (!found && tieNr < 32) {
				if (!(found = (((1 << tieNr) & *tiePool)) == 0)) {
					++tieNr;
				}
			}
			if (!found) {
				NResource::abort(err, 1);
			}
			*toomany = *toomany || tieNr >= maxtie;
			firstTiedPart->TeXTieNr = tieNr;
			if (tieNr < 6) {
				if (s == 0) s = new QString();
				t.sprintf("\\itied%d%c", firstTiedPart->TeXTieNr, 
					clef->line2TexTab_[firstTiedPart->line+LINE_OVERFLOW]);
				*s += t;
			}
			(*tiePool) = (*tiePool) | (1 << tieNr);
		}
		if (lastTiedPart) {
			tieNr = 0;
			found = false;
			while (!found && tieNr < 32) {
				if (!(found = (((1 << tieNr) & *tiePool)) == 0)) {
					++tieNr;
				}
			}
			if (!found) {
				NResource::abort(err, 2);
			}
			*toomany = *toomany || tieNr >= maxtie;
			lastTiedPart->TeXTieNr = tieNr;
			if (tieNr < 6) {
				if (s == 0) s = new QString();
				t.sprintf("\\itieu%d%c", lastTiedPart->TeXTieNr, 
					clef->line2TexTab_[lastTiedPart->line+LINE_OVERFLOW]);
				*s += t;
			}
			(*tiePool) = (*tiePool) | (1 << tieNr);
		}
	}
	else {
		for (note = noteList_.first(); note; note = noteList_.next()) {
			if (note->status & PROP_PART_OF_TIE) {
				note->TeXTieNr = note->tie_backward->TeXTieNr;
				*toomany = *toomany || note->TeXTieNr >= maxtie;
				if (note->TeXTieNr < maxtie) {
					t.sprintf("\\ttie%d", note->TeXTieNr);
					if (s == 0) s = new QString();
					*s += t;
				}
				tieNr = note->TeXTieNr;
				(*tiePool) = (*tiePool) & (~(1 << note->TeXTieNr));
			}
		}
		for (note = noteList_.first(); note; note = noteList_.next()) {
			if (note->status & PROP_TIED) {
				tieNr = 0;
				found = false;
				while (!found && tieNr < 32) {
					if (!(found = (((1 << tieNr) & *tiePool)) == 0)) {
						++tieNr;
					}
				}
				if (!found) {
					printf("internal error: too many ties: (0x%x)\n", *tiePool);
					NResource::abort(err, 3);
					
				}
				*toomany = *toomany || tieNr >= maxtie;
				note->TeXTieNr = tieNr;
				if (tieNr < 6) {
					if (s == 0) s = new QString();
					tieDir = status_ & PROP_STEM_UP ? 'd' : 'u';
					t.sprintf("\\itie%c%d%c", tieDir, note->TeXTieNr, clef->line2TexTab_[note->line+LINE_OVERFLOW]);
					*s += t;
				}
				(*tiePool) = (*tiePool) | (1 << tieNr);
			}
		}
	}
	return s;
}

QString *NChord::computeTeXSlur(unsigned int *slurPool, NClef *clef, int maxslur, bool *toomany) {
	int slurNr = 0;
	char slurDir;
	bool found = false;
	NNote *note;
	QString *s = 0, t;
	*toomany = false;

	if (!(status_ & PROP_SLURED) && !(status_ & PROP_PART_OF_SLUR)) return 0;
	if (status_ & PROP_PART_OF_SLUR) {
		if (status_ & PROP_STEM_UP) {
			note = noteList_.first();
		}
		else {
			note = noteList_.last();
		}
		auxInfo_.TeXSlurNr = slur_backward_->auxInfo_.TeXSlurNr;
		t.sprintf("\\tslur%d%c", auxInfo_.TeXSlurNr, clef->line2TexTab_[note->line+LINE_OVERFLOW]);
		s = new QString();
		*s += t;
		slurNr = auxInfo_.TeXSlurNr;
		(*slurPool) = (*slurPool) & (~(1 << auxInfo_.TeXSlurNr));
	}
	if (status_ & PROP_SLURED) {
		if (status_ & PROP_STEM_UP) {
			note =  noteList_.first();
		}
		else {
			note =  noteList_.last();
		}
		slurNr = 0;
		found = false;
		while (!found && slurNr < 32) {
			if (!(found = (((1 << slurNr) & *slurPool)) == 0)) {
				++slurNr;
			}
		}
		if (!found) {
			printf("internal error: too many slurs: (0x%x)\n", *slurPool);
			NResource::abort("internal error: too many ties");
		}
		*toomany  = slurNr >= maxslur;
		auxInfo_.TeXSlurNr = slurNr;
		if (slurNr < maxslur) {
			if (s == 0) s = new QString();
			slurDir = (status_ & PROP_STEM_UP) ? 'd' : 'u';
			t.sprintf("\\islur%c%d%c", slurDir, auxInfo_.TeXSlurNr, clef->line2TexTab_[note->line+LINE_OVERFLOW]);
			*s += t;
		}
		(*slurPool) = (*slurPool) | (1 << slurNr);
	}
	return s;
}

void NChord::initialize_acc_pos_computation() {
	int oldidx;
	int last_line;
	bool notes_available;
	NNote *note;

	oldidx = noteList_.at();

	for (note = noteList_.first(); note; note = noteList_.next()) {
		note->acc_TeX_pos = -1;
	}

	numTexRows_ = 0;

	do {
		last_line = UNDEFINED_LINE;
		notes_available = false;
		for (note = noteList_.first(); note; note = noteList_.next()) {
			if (note->acc_TeX_pos != -1) continue;
			if ((!(note->status & PROP_FORCE)) && !note->needed_acc) continue;
			if (last_line == UNDEFINED_LINE) {
				note->acc_TeX_pos = numTexRows_;
				last_line = note->line;
				notes_available = true;
			}
			else if (note->line - last_line > MIN_GAP_BETWEEN_TEX_ACCS) {
				note->acc_TeX_pos = numTexRows_;
				last_line = note->line;
				notes_available = true;
			}
		}
		if (notes_available) numTexRows_++;
	}
	while (notes_available);
	noteList_.at(oldidx); 
}

QList<NNote> *NChord::getAccTexRow(int row_nr) {
	int oldidx;
	NNote *note;

	oldidx = noteList_.at();
	acc_tex_row.clear();
	for (note = noteList_.first(); note; note = noteList_.next()) {
		if (note->acc_TeX_pos == row_nr) {
			acc_tex_row.insert(0, note);
		}
	}
	if (acc_tex_row.isEmpty()) {
		NResource::abort("getTexRow: internal error");
	}
	noteList_.at(oldidx);
	return &acc_tex_row;
}

QString *NChord::computeTeXTrill(int hline, unsigned int *trillPool, NClef *clef, struct trill_descr_str *trill_descr, 
                                        bool *nested, bool *toomany) {
	NNote *note;
	int trillnr;
	bool found;
	QString *s;

	*nested = *toomany = false;
	if (!trill_) {
		NResource::abort("computeTeXTrill: internal error");
	}


	if (trill_descr->trill_nr >= 0) {
		*nested = true; return 0;
	}
	note = noteList_.last();
	hline += 10;
	if (hline < 10) hline = 10;
	if (abs(trill_) < 2) {
		s = new QString();
		s->sprintf("\\Trille %c0", clef->line2TexTab_[hline+LINE_OVERFLOW]);
		return s;
	}

	found = false; trillnr = 0;
	while (!found && trillnr < MAXTEXTRILLS) {
		if (((*trillPool) & (1 << trillnr)) == 0) {
			found = true;
		}
		else {
			++trillnr;
		}
	}
	if (!found) {
		*toomany = true; return 0;
	}
	(*trillPool) |= (1 << trillnr);
	trill_descr->trill_nr = trillnr;
	trill_descr->endpos = getTrillEnd();
	s = new QString();
	if (trill_ > 0) {
		s->sprintf("\\ITrille%d%c", trillnr, clef->line2TexTab_[hline+LINE_OVERFLOW]);
	}
	else {
		s->sprintf("\\Itrille%d%c", trillnr, clef->line2TexTab_[hline+LINE_OVERFLOW]);
	}
	return s;
}

QString *NChord::computeTeXVa(bool bassa, int hline, unsigned int *vaPool, NClef *clef, struct trill_descr_str *trill_descr, bool *nested, bool *toomany) {
	NNote *note;
	int va_nr;
	bool found;
	QString *s;

	*nested = *toomany = false;
	if (!va_) {
		NResource::abort("computeTeXVa: internal error");
	}


	if (trill_descr->trill_nr >= 0) {
		*nested = true; return 0;
	}
	note = noteList_.last();
	if (hline > MAXLINE - MINLINE - LINE_OVERFLOW) hline = MAXLINE - MINLINE - LINE_OVERFLOW;
	if (hline < -LINE_OVERFLOW) hline = -LINE_OVERFLOW;
	if (abs(va_) < 2) {
		s = new QString();
		if (va_ > 0) {
			s->sprintf("\\octfinup{%c}{0}", clef->line2TexTab_[hline+LINE_OVERFLOW]);
		}
		else {
			s->sprintf("\\octfindown{%c}{0}", clef->line2TexTab_[hline+LINE_OVERFLOW]);
		}
		return s;
	}

	found = false; va_nr = 0;
	while (!found && va_nr < MAXTEXTVAS) {
		if (((*vaPool) & (1 << va_nr)) == 0) {
			found = true;
		}
		else {
			++va_nr;
		}
	}
	if (!found) {
		*toomany = true; return 0;
	}
	(*vaPool) |= (1 << va_nr);
	trill_descr->trill_nr = va_nr;
	trill_descr->endpos = getVaEnd();
	s = new QString();
	if (va_ > 0) {
		s->sprintf("\\Ioctfinup%d%c", va_nr, clef->line2TexTab_[hline+LINE_OVERFLOW]);
	}
	else {
		s->sprintf("\\Ioctfindown%d%c", va_nr, clef->line2TexTab_[hline+LINE_OVERFLOW]);
	}
	return s;
}
