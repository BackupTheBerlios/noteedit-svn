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

#include <math.h>
#include <stdio.h>
#include "muselement.h"
#include "resource.h"
#include "transpainter.h"
#include "clef.h"
#include "chord.h"


main_props_str::main_props_str() : scaledBoldItalicMetrics_(QFont()) /* dummy */ {
			dotcount = 0;
			pedal_on = pedal_off = 
			tied = staccato = sforzato = portato = strong_pizzicato = 
			arpeggio = grace = sforzando = fermate = hidden = false;
			noteBody = 0;
			actualLength = QUARTER_LENGTH;
			actualStemDir = STEM_DIR_AUTO;
			tp              = new NTransPainter();
			tp->setXPosition(-DEFAULT_LEFT_PAGE_BORDER);
			tp->setYPosition(-TOP_BOTTOM_BORDER);
			directPainter              = new NTransPainter();
			directPainter->setXPosition(-(int) ((float) DEFAULT_LEFT_PAGE_BORDER/zoom));
			directPainter->setYPosition(-TOP_BOTTOM_BORDER);
			p                = new NTransPainter();
			p->setYPosition(-TOP_BOTTOM_BORDER);
			left_page_border = DEFAULT_LEFT_PAGE_BORDER;
			context_clef_xpos = DEFAULT_CONTEXT_CLEF_X_POS;
			context_keysig_xpos = DEFAULT_CONTEXT_KEYSIG_X_POS;
		}

main_props_str::~main_props_str() {
			delete tp;
			delete p;
			delete directPainter;
		}


NMusElement::NMusElement(main_props_str *main_props, staff_props_str *staff_props) {
	midiTime_ = status2_ = status_ = trill_ = dynamic_ = slurY_ = va_ = 0;
	staff_props_ = staff_props;
	main_props_ = main_props;
}

void NMusElement::change(NMusElement *elem) {
	status_ = status2_ = trill_ = dynamic_ = slurY_ = 0;
	staff_props_ = elem->staff_props_;
	status_ = elem->status_;
	trill_ = elem->trill_;
	dynamic_ = elem->dynamic_;
	dynamicAlign_ = elem->dynamicAlign_;
	va_ = elem->va_;
	slurY_ = elem->slurY_;
	main_props_ = elem->main_props_;
}


void NMusElement::computeTuplet() {
	computeTuplet(tupletList_, getNumNotes(), getPlaytime());
}

void NMusElement::computeTuplet(QList<NMusElement> *tupletList, char numNotes, char playtime) {
#define TUPLET_DIST 24
	NMusElement *elem, *elem0, *first_tuplet_member = 0;
	double sumxi2, sumxi, sumxiyi, sumyi;
	int count;
	double m, n, nn, don, x0, xdist;
	int xstart, xend;
	bool isFirst;
	double tuptexn, tt;

	sumxi2 = sumxi = sumxiyi = sumyi = count = 0;
	x0 = tupletList->first()->getTopX2();
	for (elem = tupletList->first(); elem; elem = tupletList->next()) {
		if (elem->getType() & PLAYABLE) {
			first_tuplet_member = elem;
			xdist = elem->getTopX2() - x0;
			sumxi2 += xdist  * xdist;
			sumyi += elem->getTopY2();
			sumxi += xdist;
			sumxiyi += xdist * elem->getTopY2();
			count++;
		}
	}
	
	if (count == 0 || !first_tuplet_member) {
		m = 0.0;
		n = tupletList->first()->getTopY2();
	}
	else if (count < 2) {
		m = 0.0;
		n = first_tuplet_member->getTopY2();
	}
	else {
		m = -(sumxi*sumyi-sumxiyi*count)/(don = sumxi2*count-sumxi*sumxi);
		n = (-sumxi*sumxiyi+sumyi*sumxi2)/don;
	}
	isFirst = true;
	tuptexn = -1.0e30;
	n = 1.0e30;
	for (elem = tupletList->first(); elem; elem = tupletList->next()) {
		xdist = elem->getTopX2();
		nn = (double) elem->getTopY2() - m * (double) xdist - TUPLET_DIST;
		tt = (double) elem->getBotY() - m * (double) xdist;
		if (isFirst) {
			isFirst = false;
			xstart = elem->getTopX2();
		}
		if (nn < n) n = nn;
		if (tuptexn < tt) tuptexn = tt;
	}
	xend = tupletList->last()->getTopX2();
	elem0 = tupletList->first();
	for (elem = tupletList->next(); elem; elem = tupletList->next()) {
		elem0->setTupletParams(tupletList, false, m, n, tuptexn, xstart, xend, numNotes, playtime);
		elem0->calculateDimensionsAndPixmaps();
		elem0 = elem;
	}
	elem0->setTupletParams(tupletList, true, m, n, tuptexn, xstart, xend, numNotes, playtime);
	elem0->calculateDimensionsAndPixmaps();
}

void NMusElement::breakTuplet() {
	NMusElement *elem;
	for (elem = tupletList_->first(); elem; elem = tupletList_->next()) {
		elem->status_ &= (~(STAT_TUPLET | STAT_LAST_TUPLET));
		elem->changeLength(elem->getSubType());
	}
}

void NMusElement::unsetTuplet() {
	status_ &= (~(STAT_TUPLET | STAT_LAST_TUPLET));
}

void NMusElement::reposit(int xpos, int sequNr) {
	xpos_ = xpos;
	sequNr_ = sequNr;
	calculateDimensionsAndPixmaps();
}


NMusElement::~NMusElement() {
}

int NMusElement::intersects (const QPoint p) const {
	if (p.x() < bbox_.left()) return -1;
	if (p.x() >= bbox_.left() && p.x() <= bbox_.right() && p.y() >= bbox_.top() && p.y() <= bbox_.bottom()) return 0;
	return 1;
}


QString *NMusElement::computeTeXTuplet(NClef *clef) {
	NMusElement *elem;
	QString *s;
	int line, delta = 0;
	int maxheight = 20000;
	int numNotes, playtime;
	if (!(status_ & STAT_TUPLET)) return 0;
	if (tupletList_ == 0) {
		NResource::abort("internal error: NMusElement::computeTeX: tupletList_ == 0");
	}
	if (this == tupletList_->first()) {
		numNotes = getNumNotes();
		playtime = getPlaytime();
		for (elem = tupletList_->first(); delta == 0 && elem; elem = tupletList_->next()) {
			if (elem->getTopY2() > maxheight) maxheight = elem->getTopY2();
			if (elem->getType() != T_CHORD) continue;
			if (elem->getSubType() > QUARTER_LENGTH) continue;
			if (!(elem->status_ & STAT_STEM_UP)) delta = -4;
		}
		s = new QString();
		if (numNotes == 3 && playtime == 2) {
			line = (int) (((staff_props_->base - (tupm_ * xpos_ + tupTeXn_))) * 2.0 + 0.5) / LINE_DIST;
			line += delta;
			if (line < MINLINE) line = MINLINE;
			else if (line > MAXLINE) line = MAXLINE;
			s->sprintf("\\downtrio{%c}{%d}{%d}", clef->line2TexTab_[line+LINE_OVERFLOW],
				tupletList_->last()->sequNr_ - tupletList_->first()->sequNr_, (int) (atan(-tupm_) * 180.0 / 3.1415 / 2.0));
		}
		else {
			if (maxheight == 20000) maxheight = 0;
			line = (staff_props_->base - maxheight) * 2 / LINE_DIST + 4;
			if (line < 10) line = 10;
			if (line < MINLINE) line = MINLINE;
			else if (line > MAXLINE) line = MAXLINE;
			s->sprintf("\\xtuplet{%d}{%c}", numNotes, clef->line2TexTab_[line+LINE_OVERFLOW]);
		}
		return s;
	}
	return 0;
}

	
