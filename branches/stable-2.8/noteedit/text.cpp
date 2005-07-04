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

#include "text.h"
#include "resource.h"
#include "transpainter.h"
#include "textDialog_impl.h"


#define TEXT_Y_DIST LINE_DIST

NText::NText(struct main_props_str *main_props, staff_props_str *staff_props, QString text, int type) :
		 NMusElement(main_props, staff_props), type_(type) {
	actual_ = false;
	xpos_   = 0;
	text_ = text;
	calculateDimensionsAndPixmaps();
}

NText *NText::clone() {
	NText *ctext;
	ctext = new NText(main_props_, staff_props_, text_, type_);
	*ctext = *this;
	ctext->actual_ = false;

	return ctext;
}

void NText::startTextDialog() {
	QString text;
	NTextDialogImpl textDialog;
	textDialog.setText(text_);
	textDialog.exec();
	text = textDialog.getText();
	if (text.length() == 0) return;
	text_ = text;
	type_ = textDialog.isUpText() ? TEXT_UPTEXT : TEXT_DOWNTEXT;
	calculateDimensionsAndPixmaps();
}


void NText::calculateDimensionsAndPixmaps() {
	QSize size = main_props_->scaledBoldItalicMetrics_.size(Qt::SingleLine, text_);
	if (type_ == TEXT_UPTEXT) {
		textDrawPoint_ = QPoint (xpos_, staff_props_->base  - TEXT_Y_DIST );
	}
	else {
		textDrawPoint_ = QPoint (xpos_, staff_props_->base + 4 * LINE_DIST + TEXT_Y_DIST + size.height());
	}
	bbox_ = QRect(textDrawPoint_.x(), textDrawPoint_.y() - 2 * size.height(), 2 * (int) (1.3 * (double) size.width()), 2 * size.height());
}

void NText::draw(int /* dummy */) {
	main_props_->tp->beginTranslated();
	main_props_->tp->toggleToScaledText(true);
	main_props_->tp->setFont( main_props_->scaledBoldItalic_ );
	main_props_->tp->setPen(actual_ ? NResource::selectedBarPen_ : NResource::barPen_);
	main_props_->tp->drawScaledText(textDrawPoint_, text_);
	main_props_->tp->end();
}
