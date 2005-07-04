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

#include "transpainter.h"
#include <stdio.h>

NTransPainter::NTransPainter(QPaintDevice *pd, int transx, int transy) :
	QPainter() {
	pd_ = pd;
	transx_ = transx;
	transy_ = transy;
	zoom_ = 1.0;
	computeRealLeftBorder();
}


void NTransPainter::setPaintDevice(QPaintDevice *pd) {
	pd_ = pd;
}

void NTransPainter::setXPosition(int transx) {
	transx_ = (int) (zoom_ * transx);
	computeRealLeftBorder();
}

void NTransPainter::setYPosition(int transy) {
	transy_ = (int) (zoom_ * transy);
}

void NTransPainter::beginTranslated() {
	begin (pd_);
	setClipRect(cr_);
	translate (-transx_, -transy_);
	scale(zoom_, zoom_);
}

void NTransPainter::drawScaledText(QPoint pos, QString text) {
	pos *= zoom_;
	drawText(pos, text);
}

void NTransPainter::drawScaledText(int x, int y, QString text) {
	drawText((int) (x * zoom_), (int) (y * zoom_), text);
}

void NTransPainter::beginTextDrawing() {
	begin (pd_);
	setClipRect(cr_);
	translate (-transx_, -transy_);
}

void NTransPainter::toggleToScaledText(bool on) {
	end();
	begin (pd_);
	setClipRect(cr_);
	if (on) {
		translate (-transx_, -transy_);
	}
	else {
		translate (-transx_, -transy_);
		scale(zoom_, zoom_);
	}
}


void NTransPainter::beginYtranslated() {
	begin (pd_);
	
	setClipRect(cr_);
	translate (0, -transy_);
	scale(zoom_, zoom_);
}

void NTransPainter::beginUnclippedYtranslated() {
#if QT_VERSION >= 300
	begin (pd_, true);
#else
	begin (pd_);
#endif

	
	translate (0, -transy_);
	scale(zoom_, zoom_);
}

void NTransPainter::noticeClipRect( QRect cr ) {
	cr_ = cr;
	computeRealLeftBorder();
}

void NTransPainter::computeRealLeftBorder() {
	leftBorder_ = cr_.left() + (int) ((float) transx_ * zoom_);
}
