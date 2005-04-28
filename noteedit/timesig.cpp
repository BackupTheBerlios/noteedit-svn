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

#include <qbitmap.h>
#include "timesig.h"
#include "resource.h"
#include "transpainter.h"
#define TSIG_WIDTH 50
#define TSIG_HEIGHT (4*LINE_DIST)
#define NUM_POS (TSIG_HEIGHT / 2 - 1)
#define DRAW_OFFS 2


NTimeSig::NTimeSig(main_props_str *main_props, staff_props_str *staff_props) :
		 NMusElement(main_props, staff_props) {
	actual_ = false;
	xpos_   = 0;
	numerator_ = denominator_ =  0;

}


void NTimeSig::setSignature(int numerator, int denominator) {
	numerator_ = numerator;
	denominator_ = denominator;
	if (!staff_props_) return;
	if (staff_props_->base) calculateDimensionsAndPixmaps();
}

void NTimeSig::setSignature(NTimeSig *otherTimeSig) {
	numerator_ = otherTimeSig->numerator_;
	denominator_ = otherTimeSig->denominator_;
	if (!staff_props_) return;
	if (staff_props_->base) calculateDimensionsAndPixmaps();
}

NTimeSig *NTimeSig::clone() {
	NTimeSig *ctimesig;

	ctimesig = new NTimeSig(main_props_, staff_props_);

	*ctimesig = *this;
	return ctimesig;
}

int NTimeSig::numOf128th() const {
	return numerator_ * 128 / denominator_;
}


void NTimeSig::calculateDimensionsAndPixmaps() {
	numString_.sprintf("%d", numerator_);
	denomString_.sprintf("%d", denominator_);
	numDrawPoint_ = QPoint( xpos_ + 1, staff_props_->base - DRAW_OFFS + NUM_POS);
	denomDrawPoint_ = QPoint(xpos_ + 1, staff_props_->base - DRAW_OFFS + TSIG_HEIGHT - 1);
	bbox_ = QRect(xpos_, staff_props_->base , TSIG_WIDTH, TSIG_HEIGHT);
}

void NTimeSig::draw(int /* dummy */) {
	main_props_->tp->beginTextDrawing();
	main_props_->tp->setPen(actual_ ? NResource::redPen_ : NResource::blackPen_);
	main_props_->tp->setFont( main_props_->scaledBold_ );
	main_props_->tp->drawScaledText(numDrawPoint_, numString_);
	main_props_->tp->drawScaledText(denomDrawPoint_, denomString_);
	main_props_->tp->end();
}
