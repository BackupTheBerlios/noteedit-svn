#include <stdio.h>
#include <qtooltip.h>
#include <klocale.h>
#include "numberdisplay.h"

NNumberDisplay::NNumberDisplay(int min, int max, const QString& text, int accel, QObject* parent, const char* name) :
		KAction(text, accel, parent, name) {
	if (min > max) min = max;
	min_ = min; max_ = max;
	number_ = 0;
	toolTip_ = text;
}

void NNumberDisplay::setMin(int min) {
	if (min > max_) return;
	min_ = min;
	number_->setMin(min_);
}

void NNumberDisplay::setMax(int max) {
	if (max < min_) return;
	max_ = max;
	if (number_) number_->setMax(max_);
}

bool NNumberDisplay::isZero() {
	if (number_ == 0) return false;
	return number_->getRealValue() == 0;
}

int NNumberDisplay::getVal() {
	if (number_) return number_->getRealValue();
	return 0;
}

void NNumberDisplay::setVal(int val) {
	if (number_) number_->setRealValue(val);
}

int NNumberDisplay::plug( QWidget *w, int index) {
	number_ = new NLCDNumber(min_, max_, this, w);
	QToolTip::add(number_, toolTip_);
	number_->setMaximumSize (20, 60);
	number_->setNumDigits(2);
	number_->setRealValue(0);

	return 1;
}

void NNumberDisplay::emitValueChanged(int val) {
	emit valChanged(val);
}

NLCDNumber::NLCDNumber (int min, int max, NNumberDisplay *numDisplay, QWidget *parent, const char * name) :
	 QLCDNumber(parent, name) {
	 QLCDNumber::setMode( QLCDNumber::HEX );
	 min_ = min; max_ = max;
	 numDisplay_ = numDisplay;
	 setRealValue(0);
}

void NLCDNumber::setMin(int min) {
	min_ = min;
	if (min_ > getRealValue()) {
		setRealValue(min_);
	}
}

void NLCDNumber::setMax(int max) {
	max_ = max;
	if (max_ < getRealValue()) {
		setRealValue(max_);
	}
}


void NLCDNumber::mousePressEvent(QMouseEvent *evt) {
	int newval;
	switch (evt->button()) {
		case LeftButton: newval = getRealValue() + 1;
				 if (newval > max_) break;
				 setRealValue(newval);
				 numDisplay_->emitValueChanged(newval);
				 break;

		default:	 newval = getRealValue() - 1;
				 if (newval < min_) break;
				 setRealValue(newval);
				 numDisplay_->emitValueChanged(newval);
				 break;
	 }
}

void NLCDNumber::setRealValue(int val) {
	realValue_ = val;
	display(realValue_ == 0 ? 10 : realValue_);
}

int NLCDNumber::getRealValue() {
	return realValue_;
}
			
