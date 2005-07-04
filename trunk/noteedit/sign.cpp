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
#include "sign.h"
#include "resource.h"
#include "transpainter.h"

#define SIMPLE_BAR_X 8
#define DOUBLE_BAR_DIST 8


NSign::NSign(struct main_props_str *main_props, staff_props_str *staff_props, int type) :
		 NMusElement(main_props, staff_props) {
	actual_ = false;
	xpos_   = 0;
	signType_ = type;
	switch (type) {
		case SPECIAL_ENDING1:
			u1_.barNr = 1;
			valString_.sprintf("%d", u1_.barNr);
			break;
		case SPECIAL_ENDING2:
			u1_.barNr = 2;
			valString_.sprintf("%d", u1_.barNr);
			break;
		case TEMPO_SIGNATURE:
			values_.tempo = 100;
			valString_.sprintf(" = %d", values_.tempo);
			break;
		case VOLUME_SIG:
			values_.volume = 80;
			valString_.sprintf("mf,(%d)", values_.volume);
			break;
		case REPEAT_CLOSE:
			values_.repeatCount = 2;
			break;
		case PROGRAM_CHANGE:
			program_ = 0;
			valString_.sprintf("{%d}", program_);
			break;
	}
	if (staff_props_->base) calculateDimensionsAndPixmaps();
}

NSign *NSign::clone() {
	NSign *csign;
	csign = new NSign(main_props_, staff_props_, signType_);
	*csign = *this;
	csign->actual_ = false;

	return csign;
}

void NSign::setBarNr(int barNr) {
	u1_.barNr = barNr;
	if (signType_ == REPEAT_CLOSE) {
		if (values_.repeatCount > 2) {
			valString_.sprintf("x %d", values_.repeatCount);
			return;
		}
	}
	valString_.sprintf("%d", u1_.barNr);
}

void NSign::setTempo(int tempo) {
	values_.tempo = tempo;
	valString_.sprintf(" = %d", values_.tempo);
}

void NSign::setProgram(int prg) {
	program_ = prg;
	valString_.sprintf("{%d}", program_);
}


void NSign::setVolume(int vol_type, int volume) {
	values_.volume = volume;
	switch(volType_ = vol_type) {
		case V_PPPIANO : valString_.sprintf("ppp(%d)", values_.volume); break;
		case V_PPIANO  : valString_.sprintf("pp(%d)", values_.volume); break;
		case V_PIANO   : valString_.sprintf("p(%d)", values_.volume); break;
		case V_MPIANO  : valString_.sprintf("mp(%d)", values_.volume); break;
		case V_FORTE   : valString_.sprintf("f(%d)", values_.volume); break;
		case V_FFORTE  : valString_.sprintf("ff(%d)", values_.volume); break;
		case V_FFFORTE : valString_.sprintf("fff(%d)", values_.volume); break;
		default        : valString_.sprintf("mf(%d)", values_.volume); break;
	}
}

void NSign::calculateDimensionsAndPixmaps() {
	int ypos;
	switch (signType_) {
		case SIMPLE_BAR: pixmapHeight_ = 6*LINE_DIST;
				pixmapWidth_ = 30+SIMPLE_BAR_X;
				numDrawPoint_ = QPoint(xpos_+17+SIMPLE_BAR_X, staff_props_->base - LINE_DIST / 2);
				ypos = staff_props_->base;
				break;
		case DOUBLE_BAR: pixmapHeight_ = 6*LINE_DIST;
				pixmapWidth_ = 30+SIMPLE_BAR_X+DOUBLE_BAR_DIST;
				numDrawPoint_ = QPoint(xpos_+17+SIMPLE_BAR_X, staff_props_->base - LINE_DIST / 2);
				ypos = staff_props_->base;
				break;
		case REPEAT_OPEN:
				blackPixmap_ = NResource::repOpenPixmap_;
				redPixmap_ = NResource::repOpenRedPixMap_;
				pixmapHeight_ = blackPixmap_->height();
				pixmapWidth_ = blackPixmap_->width() + 6;
				numDrawPoint_ = QPoint(xpos_ + 3, staff_props_->base - LINE_DIST / 2);
				nbaseDrawPoint_ = QPoint(xpos_ + 3, staff_props_->base + LINE_DIST * 4);
				ypos = staff_props_->base;
				break;
		case REPEAT_CLOSE:
				blackPixmap_ = NResource::repClosePixmap_;
				redPixmap_ = NResource::repCloseRedPixMap_;
				pixmapHeight_ = blackPixmap_->height();
				pixmapWidth_ = blackPixmap_->width()+6;
				numDrawPoint_ = QPoint(xpos_ + 3, staff_props_->base - LINE_DIST / 2);
				nbaseDrawPoint_ = QPoint(xpos_ + 3, staff_props_->base + LINE_DIST * 4);
				ypos = staff_props_->base - 1;
				break;
		case REPEAT_OPEN_CLOSE:
				blackPixmap_ = NResource::repOpenClosePixmap_;
				redPixmap_ = NResource::repOpenCloseRedPixMap_;
				pixmapHeight_ = blackPixmap_->height();
				pixmapWidth_ = blackPixmap_->width() + 6;
				numDrawPoint_ = QPoint(xpos_ + 3, staff_props_->base - LINE_DIST / 2);
				nbaseDrawPoint_ = QPoint(xpos_ + 3, staff_props_->base + LINE_DIST * 4);
				ypos = staff_props_->base;
				break;
		case SPECIAL_ENDING1:
		case SPECIAL_ENDING2:
				pixmapHeight_ = 8*LINE_DIST;
				pixmapWidth_ = 50;
				ypos = staff_props_->base;
				nbaseDrawPoint1_ = QPoint(xpos_, staff_props_->base);
				nbaseDrawPoint2_ = QPoint(xpos_, staff_props_->base - 48);
				nbaseDrawPoint3_ = QPoint(xpos_+ 42, staff_props_->base - 48);
				numDrawPoint_ = QPoint(xpos_ + 20, staff_props_->base - 12);
				break;
		case END_BAR:	blackPixmap_ = NResource::endBarPixmap_;
				redPixmap_ = NResource::endBarRedPixmap_;
				pixmapHeight_ = blackPixmap_->height();
				pixmapWidth_ = blackPixmap_->width()+6;
				numDrawPoint_ = QPoint(xpos_ + 3, staff_props_->base - LINE_DIST / 2);
				nbaseDrawPoint_ = QPoint(xpos_ + 3, staff_props_->base + LINE_DIST * 4);
				ypos = staff_props_->base - 1;
				break;
		case TEMPO_SIGNATURE:
				ypos = staff_props_->base-3*LINE_DIST;
				pixmapHeight_ = 4*LINE_DIST;
				pixmapWidth_ = 180;
				nbaseEllipse_ = QRect(xpos_ + 3, staff_props_->base - 3*LINE_DIST/2, 18, 12);
				numDrawPoint_ = QPoint(xpos_ + 24, staff_props_->base - LINE_DIST / 2);
				break;
		case DAL_SEGNO:
		case DAL_SEGNO_AL_FINE:
		case RITARDANDO:
		case ACCELERANDO:
		case FINE:	ypos = staff_props_->base-3*LINE_DIST;
				pixmapHeight_ = 4*LINE_DIST;
				pixmapWidth_ = 100;
				numDrawPoint_ = QPoint(xpos_, staff_props_->base - LINE_DIST);
				break;
		case VOLUME_SIG:
				ypos = staff_props_->base-3*LINE_DIST;
				pixmapHeight_ = 3*LINE_DIST;
				pixmapWidth_ = 180;
				numDrawPoint_ = QPoint(xpos_ + 24, staff_props_->base - LINE_DIST / 2);
				break;
		case PROGRAM_CHANGE:
				ypos = staff_props_->base-3*LINE_DIST;
				pixmapHeight_ = 3*LINE_DIST;
				pixmapWidth_ = 90;
				numDrawPoint_ = QPoint(xpos_ + 24, staff_props_->base - 20);
				break;
		case SEGNO:     blackPixmap_ = NResource::segnoPixmap_;
				redPixmap_ = NResource::segnoRedPixmap_;
				pixmapHeight_ = blackPixmap_->height();
				pixmapWidth_ = blackPixmap_->width();
				ypos = staff_props_->base - 80;
				break;
		case DAL_SEGNO_AL_CODA:
				blackPixmap_ = NResource::dalSegnoAlCodaPixmap_;
				redPixmap_ = NResource::dalSegnoAlCodaRedPixmap_;
				pixmapHeight_ = blackPixmap_->height();
				pixmapWidth_ = blackPixmap_->width();
				ypos = staff_props_->base - 80;
				break;
		case CODA:	blackPixmap_ = NResource::codaPixmap_;
				redPixmap_ = NResource::codaRedPixmap_;
				pixmapHeight_ = blackPixmap_->height();
				pixmapWidth_ = blackPixmap_->width();
				ypos = staff_props_->base - 80;
				break;
		default: NResource::abort("unknown sign");
				break;
	}
	nbaseDrawPoint_ = QPoint (xpos_, ypos);
	bbox_ = QRect(xpos_, ypos , pixmapWidth_, pixmapHeight_);
}

void NSign::draw(int /* dummy */) {
	int y0pos;
	main_props_->tp->beginTranslated();
	switch (signType_) {
		case SIMPLE_BAR:
				main_props_->tp->setPen
					(actual_ ? NResource::selectedBarPen_ : NResource::barPen_);
				if ((y0pos = NResource::yPosOfOrchestralBar(xpos_)) == -1) {
					y0pos = nbaseDrawPoint_.y();
				}
				main_props_->tp->drawLine(xpos_+SIMPLE_BAR_X, y0pos, xpos_+SIMPLE_BAR_X, nbaseDrawPoint_.y()+4* LINE_DIST);
				if (NResource::showStaffNrs_) {
					main_props_->tp->toggleToScaledText(true);
					main_props_->tp->setFont( main_props_->scaledItalic_ );
					main_props_->tp->setPen
						(actual_ ?
						 NResource::selectedBarNumberPen_ : NResource::barNumberPen_
						);
					main_props_->tp->drawScaledText(numDrawPoint_, valString_);
				}
				break;
		case DOUBLE_BAR:
				main_props_->tp->setPen
					(actual_ ? NResource::selectedBarPen_ : NResource::barPen_);
				main_props_->tp->drawLine(xpos_+SIMPLE_BAR_X, nbaseDrawPoint_.y(), xpos_+SIMPLE_BAR_X, nbaseDrawPoint_.y()+4* LINE_DIST);
				main_props_->tp->drawLine(xpos_+SIMPLE_BAR_X+DOUBLE_BAR_DIST, nbaseDrawPoint_.y(), xpos_+SIMPLE_BAR_X+DOUBLE_BAR_DIST, nbaseDrawPoint_.y()+4* LINE_DIST);
				if (NResource::showStaffNrs_) {
					main_props_->tp->toggleToScaledText(true);
					main_props_->tp->setFont( main_props_->scaledItalic_ );
					main_props_->tp->setPen
						(actual_ ?
						 NResource::selectedBarNumberPen_ : NResource::barNumberPen_
						);
					main_props_->tp->drawScaledText(numDrawPoint_, valString_);
				}
				break;
		case REPEAT_CLOSE:
				main_props_->tp->drawPixmap(nbaseDrawPoint_, actual_ ?  *redPixmap_: *blackPixmap_);
				main_props_->tp->setPen(actual_ ? NResource::redPen_ : NResource::blackPen_);
				if (NResource::showStaffNrs_ || values_.repeatCount > 2) {
					main_props_->tp->toggleToScaledText(true);
					main_props_->tp->setFont( main_props_->scaledItalic_ );
					main_props_->tp->drawScaledText(numDrawPoint_, valString_);
				}
				break;
		case END_BAR:
		case REPEAT_OPEN:
		case REPEAT_OPEN_CLOSE:
				main_props_->tp->drawPixmap(nbaseDrawPoint_, actual_ ?  *redPixmap_: *blackPixmap_);
				main_props_->tp->setPen(actual_ ? NResource::redPen_ : NResource::blackPen_);
				if (NResource::showStaffNrs_) {
					main_props_->tp->toggleToScaledText(true);
					main_props_->tp->setFont( main_props_->scaledItalic_ );
					main_props_->tp->drawScaledText(numDrawPoint_, valString_);
				}
				break;
		case TEMPO_SIGNATURE:
				main_props_->tp->setPen(actual_ ? NResource::selectedTempoSignaturePen_ : NResource::tempoSignaturePen_);
				main_props_->tp->setBrush(actual_ ? NResource::redBrush_ : NResource::blackBrush_);
				main_props_->tp->drawPie(nbaseEllipse_, 0, 16*360);
				main_props_->tp->toggleToScaledText(true);
				main_props_->tp->setFont( main_props_->scaledItalic_ );
				main_props_->tp->setPen(actual_ ? NResource::selectedTempoSignaturePen_ : NResource::tempoSignaturePen_);
				main_props_->tp->drawScaledText(numDrawPoint_, valString_);
				break;
		case DAL_SEGNO: main_props_->tp->toggleToScaledText(true);
				main_props_->tp->setFont( main_props_->scaledItalic_ );
				main_props_->tp->setPen(actual_ ? NResource::selectedTempoSignaturePen_ : NResource::tempoSignaturePen_);
				main_props_->tp->drawScaledText(numDrawPoint_, NResource::dalSegno_);
				break;
		case RITARDANDO: main_props_->tp->toggleToScaledText(true);
				main_props_->tp->setFont( main_props_->scaledItalic_ );
				main_props_->tp->setPen(actual_ ? NResource::selectedTempoSignaturePen_ : NResource::tempoSignaturePen_);
				main_props_->tp->drawScaledText(numDrawPoint_, NResource::ritardando_);
				break;
		case ACCELERANDO: main_props_->tp->toggleToScaledText(true);
				main_props_->tp->setFont( main_props_->scaledItalic_ );
				main_props_->tp->setPen(actual_ ? NResource::selectedTempoSignaturePen_ : NResource::tempoSignaturePen_);
				main_props_->tp->drawScaledText(numDrawPoint_, NResource::accelerando_);
				break;
		case DAL_SEGNO_AL_FINE:
				main_props_->tp->toggleToScaledText(true);
				main_props_->tp->setFont( main_props_->scaledItalic_ );
				main_props_->tp->setPen(actual_ ? NResource::selectedTempoSignaturePen_ : NResource::tempoSignaturePen_);
				main_props_->tp->drawScaledText(numDrawPoint_, NResource::dalSegnoAlFine_);
				break;
		case FINE:	main_props_->tp->toggleToScaledText(true);
				main_props_->tp->setFont( main_props_->scaledItalic_ );
				main_props_->tp->setPen(actual_ ? NResource::selectedTempoSignaturePen_ : NResource::tempoSignaturePen_);
				main_props_->tp->drawScaledText(numDrawPoint_, NResource::fine_);
				break;
		case VOLUME_SIG:
				main_props_->tp->toggleToScaledText(true);
				main_props_->tp->setFont( main_props_->scaledItalic_ );
				main_props_->tp->setPen(actual_ ? NResource::selectedVolumeSignaturePen_ : NResource::volumeSignaturePen_);
				main_props_->tp->drawScaledText(numDrawPoint_, valString_);
				break;
		case PROGRAM_CHANGE:
				main_props_->tp->toggleToScaledText(true);
				main_props_->tp->setPen(actual_ ? NResource::selectedProgramChangePen_ : NResource::programChangePen_);
				main_props_->tp->setFont( main_props_->scaledItalic_ );
				main_props_->tp->drawScaledText(numDrawPoint_, valString_);
				break;
		case SPECIAL_ENDING1:
		case SPECIAL_ENDING2:
				main_props_->tp->setPen(actual_ ? NResource::selectedSpecialEndingPen_ : NResource::specialEndingPen_);
				main_props_->tp->drawLine(nbaseDrawPoint1_, nbaseDrawPoint2_);
				main_props_->tp->drawLine(nbaseDrawPoint2_, nbaseDrawPoint3_);
				main_props_->tp->toggleToScaledText(true);
				main_props_->tp->setFont( main_props_->scaledItalic_ );
				main_props_->tp->drawScaledText(numDrawPoint_, valString_);
				break;
		case CODA:
		case DAL_SEGNO_AL_CODA:
		case SEGNO:     main_props_->tp->drawPixmap(nbaseDrawPoint_, actual_ ?  *redPixmap_: *blackPixmap_);
				break;
				
	}
	main_props_->tp->end();
}
