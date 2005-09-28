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

#include <qpixmap.h>
#include <stdio.h>
#include "rest.h"
#include "resource.h"
#include "transpainter.h"
#include "chorddiagram.h"

#define REST_POINT_DIST 6
#define REST_POINT_RAD 5

NRest::NRest(main_props_str *main_props, staff_props_str *staff_props, int *y_voice_offs, int length, property_type properties) :
		 NPlayable(main_props, staff_props) {
	length_ = length;
	switch (length) {
		case MULTIREST: properties_ = 0;
				multiRestLength_ = properties;
				break;
		default: properties_ = properties;
			 multiRestLength_ = 0;
			 break;
	}
	midiLength_ = computeMidiLength();
	actual_ = false;
	xpos_ = 0;
	cdiagram_ = 0;
	yRestOffs_ = y_voice_offs;
	calculateDimensionsAndPixmaps();
}

NRest::~NRest() {
	if (cdiagram_) {
		delete cdiagram_;
	}
}

NRest *NRest::clone() {
	NRest *crest;
	crest = new NRest(main_props_, staff_props_, yRestOffs_, length_, false);
	*crest = *this;
	crest->actual_ = false;
	if (cdiagram_) {
		crest->cdiagram_ = new NChordDiagram(cdiagram_);
	}
	else {
		crest->cdiagram_ = 0;
	}
	return crest;
}

int NRest::computeMidiLength() const {
	if (length_ == MULTIREST) {
		return staff_props_->measureLength * multiRestLength_;
	}
	if (hasProperty( PROP_TUPLET )) {
		return tupRealTime_ * length_ / numTupNotes_;
	}
	switch (properties() & DOT_MASK) {
		case 1: return 3 * length_ / 2;
		case 2: return 7 * length_ / 4;
	}
	return length_;
}

void NRest::changeLength(int length) {
	if (length > WHOLE_LENGTH) return;
	length_ = length;
	midiLength_ = computeMidiLength();
}

void NRest::setTupletParams(QList<NPlayable> *tupletList, 
			bool last, double m, double n, double tuptexn, int xstart, int xend, char numnotes, char playtime) {
	tupletList_ = tupletList;
	setProperty(PROP_LAST_TUPLET, last);
	addProperty( PROP_TUPLET );
	tupTeXn_ = tuptexn;
	tupm_ = m; tupn_ = n; xstart_ = xstart; xend_ = xend;
	numTupNotes_ = numnotes; tupRealTime_ = playtime;
	switch (numnotes) {
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

void NRest::transposeChordDiagram(int semitones) {
	if (!cdiagram_) return;
	if (cdiagram_->showDiagram_) return;
	cdiagram_->transpose(semitones);
}

void NRest::calculateDimensionsAndPixmaps() {
	int ypos;
	int lineoffs = 0;
	int yoffs = 0;
	int pointyoffs = 30;
	switch (length_) {
	      case MULTIREST:
			ypos = staff_props_->base-3*LINE_DIST;
			pointPos1_ = QRect(xpos_ + 3, staff_props_->base + 3*LINE_DIST/2, 200, 12);
			lenString_.sprintf("%d", multiRestLength_);
			nbaseDrawPoint_ = QPoint(xpos_ + 80, staff_props_->base + 1*LINE_DIST);
			break;
	      case WHOLE_LENGTH: blackPixmap_ = hasProperty( PROP_HIDDEN ) ? NResource::rfullMagPixmap_ : NResource::rfullPixmap_ ;
			redPixmap_   = NResource::rfullRedPixmap_;
			greyPixmap_   = NResource::rfullGreyPixmap_;
			lineoffs = -2;
			pointyoffs = -2;
			break;
	      case HALF_LENGTH: blackPixmap_ = hasProperty( PROP_HIDDEN ) ? NResource::rhalfMagPixmap_ : NResource::rhalfPixmap_;
			redPixmap_   = NResource::rhalfRedPixmap_;
			greyPixmap_ = NResource::rhalfGreyPixmap_;
			lineoffs = -3;
			pointyoffs = -2;
			yoffs = 5;
			break;
	      case QUARTER_LENGTH: blackPixmap_ = hasProperty( PROP_HIDDEN ) ? NResource::rquarterMagPixmap_ : NResource::rquarterPixmap_;
			redPixmap_   = NResource::rquarterRedPixmap_;
			greyPixmap_   = NResource::rquarterGreyPixmap_;
			lineoffs = -1;
			pointyoffs = 30;
			break;
	      case NOTE8_LENGTH: blackPixmap_ = hasProperty( PROP_HIDDEN ) ? NResource::r8MagPixmap_ : NResource::r8Pixmap_;
			redPixmap_   = NResource::r8RedPixmap_;
			greyPixmap_   = NResource::r8GreyPixmap_;
			lineoffs = -3;
			pointyoffs = 30;
			break;
	      case NOTE16_LENGTH : blackPixmap_ = hasProperty( PROP_HIDDEN ) ? NResource::r16MagPixmap_ : NResource::r16Pixmap_;
			redPixmap_   = NResource::r16RedPixmap_;
			greyPixmap_   = NResource::r16GreyPixmap_;
			lineoffs = -2;
			pointyoffs = 40;
			break;
	      case NOTE32_LENGTH :blackPixmap_ = hasProperty( PROP_HIDDEN ) ? NResource::r32MagPixmap_ : NResource::r32Pixmap_;
			redPixmap_   = NResource::r32RedPixmap_;
			greyPixmap_   = NResource::r32GreyPixmap_;
			lineoffs = -1;
			pointyoffs = 50;
			break;
		case NOTE64_LENGTH : blackPixmap_ = hasProperty( PROP_HIDDEN ) ? NResource::r64MagPixmap_ : NResource::r64Pixmap_;
			redPixmap_   = NResource::r64RedPixmap_;
			greyPixmap_   = NResource::r64GreyPixmap_;
			lineoffs = -1;
			pointyoffs = 60;
			break;
		case NOTE128_LENGTH  :
			blackPixmap_ = hasProperty( PROP_HIDDEN ) ? NResource::r128MagPixmap_ : NResource::r128Pixmap_;
			redPixmap_   = NResource::r128RedPixmap_;
			greyPixmap_   = NResource::r128GreyPixmap_;
			lineoffs = 1;
			pointyoffs = 70;
			break;
                default :
                        qWarning("Unknown rest %d", length_);
                        blackPixmap_ = NResource::r128Pixmap_;
                        redPixmap_   = NResource::r128RedPixmap_;
                        greyPixmap_   = NResource::r128RedPixmap_;
                        lineoffs = 1;
                        break;
	}
	if (hasProperty( PROP_LAST_TUPLET )) {
		tuplet0_ = QPoint(xstart_, xstart_ * tupm_ + tupn_);
		tuplet1_ = QPoint(xend_, xend_ * tupm_ + tupn_);
		tuplet00_ = tuplet0_ + QPoint(0, TUPLET_HEIGHT);
		tuplet01_ = tuplet1_ + QPoint(0, TUPLET_HEIGHT);
		tupletDigit_ = tuplet0_ + (tuplet1_ - tuplet0_) / 2 + QPoint(0, -TUPLET_DGIT_DIST);
	}
	ypos = staff_props_->base - lineoffs * LINE_DIST/2 + yoffs;
	if (cdiagram_) {
		cdiagramDrawPoint_ = QPoint(xpos_ + CPOINT_X_OFFS, staff_props_->base + CPOINT_Y_OFFS);
	}
	if (length_ == MULTIREST) {
		pixmapHeight_ = 3*LINE_DIST;
		pixmapWidth_ = 200;
		if (cdiagram_) {
			if (pixmapWidth_ < cdiagram_->neededWidth()) pixmapWidth_ = cdiagram_->neededWidth();
		}
		bbox_ = QRect(xpos_, staff_props_->base + 3*LINE_DIST/2 , pixmapWidth_, pixmapHeight_);
	}
	else {
		pixmapHeight_ = blackPixmap_->height();
		pixmapWidth_ = blackPixmap_->width();
		if (cdiagram_) {
			if (pixmapWidth_ < cdiagram_->neededWidth()) pixmapWidth_ = cdiagram_->neededWidth();
		}
		pointPos1_ = QRect(xpos_ + pixmapWidth_ + REST_POINT_DIST, ypos+pointyoffs+ ((*yRestOffs_) * LINE_DIST), 2*REST_POINT_RAD, 2*REST_POINT_RAD);
		pointPos2_ = QRect(xpos_ + pixmapWidth_ + 2*REST_POINT_DIST+2*REST_POINT_RAD, ypos+pointyoffs+ ((*yRestOffs_) * LINE_DIST), 2*REST_POINT_RAD, 2*REST_POINT_RAD);
		nbaseDrawPoint_ = QPoint (xpos_, ypos + ((*yRestOffs_) * LINE_DIST));
		switch(properties() & DOT_MASK) {
			case 1: pixmapWidth_ += 3*REST_POINT_DIST + 2*REST_POINT_RAD; break;
			case 2: pixmapWidth_ += 4*REST_POINT_DIST+  4*REST_POINT_RAD; break;
		}
		bbox_ = QRect(xpos_, ypos+(*yRestOffs_ * LINE_DIST) , pixmapWidth_, pixmapHeight_);
	}
	topYPoint_ = QPoint(xpos_ + pixmapWidth_, staff_props_->base + 5);
}

QPoint *NRest::getTopY() {
	return &topYPoint_;
}

int NRest::getTopY2() {
	return staff_props_->base + 5;
}

int NRest::getTopX2() {
	return xpos_;
}

double NRest::getBotY() {
	return staff_props_->base - (double) (-2 * LINE_DIST) / 2.0;
}

void NRest::addChordDiagram(NChordDiagram *cdiag) {
	if (cdiagram_) {
		delete cdiagram_;
	}
	cdiagram_ = cdiag;
}

void NRest::removeChordDiagram() {
	if (cdiagram_) {
		delete cdiagram_;
		cdiagram_ = 0;
	}
}

void NRest::draw(int flags) {
	if (hasProperty(PROP_HIDDEN) && (flags & DRAW_NO_HIDDEN_REST)) return;
	main_props_->tp->beginTranslated();
	if (length_ == MULTIREST) {
		main_props_->tp->setPen(actual_ ? NResource::redPen_ : NResource::blackPen_);
		main_props_->tp->fillRect(pointPos1_, actual_ ? NResource::redBrush_ : NResource::blackBrush_);
		main_props_->tp->toggleToScaledText(true);
		main_props_->tp->setFont( main_props_->scaledBold_ );
		main_props_->tp->drawScaledText(nbaseDrawPoint_, lenString_);
		main_props_->tp->end();
		return;
	}
	main_props_->tp->drawPixmap (nbaseDrawPoint_, actual_ ?
					 *redPixmap_ : ((flags & DRAW_INDIRECT_GREY) ?  *greyPixmap_ : *blackPixmap_));
	if (properties() & DOT_MASK) { //  draw the dot for dotted rests
		main_props_->tp->setPen(actual_ ? NResource::redPen_ : NResource::blackPen_);
		main_props_->tp->setBrush(actual_ ? NResource::redBrush_ : NResource::blackBrush_);
		main_props_->tp->drawPie(pointPos1_ , 0, 320*16);
		if ((properties() & DOT_MASK) > 1) {
			main_props_->tp->drawPie(pointPos2_, 0, 320*16);
		}
	}
	if (hasProperty( PROP_LAST_TUPLET )) {
		main_props_->tp->setPen(NResource::blackWidePen_);
		main_props_->tp->drawPixmap(tupletDigit_, *tupletMarker_);
		main_props_->tp->drawLine(tuplet00_, tuplet0_);
		main_props_->tp->drawLine(tuplet0_, tuplet1_);
		main_props_->tp->drawLine(tuplet1_, tuplet01_);
	}
	if (hasProperty( PROP_FERMT )) { //  fermate draw
		main_props_->tp->drawPixmap(QPoint(
			xpos_ - (NResource::fermateAbPixmap_->width() / 4),
			staff_props_->base + (NResource::fermateAbPixmap_->height() / 4) - (int) (1.8 * 20)
			),
			actual_ ? *NResource::fermateAbRedPixmap_ : *NResource::fermateAbPixmap_ );
	}
	if (cdiagram_) {
		cdiagram_->draw(main_props_->tp, &cdiagramDrawPoint_, main_props_);
	}
	main_props_->tp->end();
}

void NRest::setDotted(int dotcount) {
	if (length_ == MULTIREST) return;
	removeProperty( DOT_MASK );
	addProperty(dotcount & DOT_MASK);
	midiLength_ = computeMidiLength();
}
