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

#include "musixhint.h"
#include <kaudioplayer.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qtimer.h> 
#include <stdio.h>
#ifndef TESTING
#include "resource.h"
#endif


#define PART1 0.17
#define PART2 0.32
#define PART3 0.49
#define PART4 1.0
#define END_PART 1.0

#define TIME1 3000
#define TIME2 1000
#define TIME3 1000
#define TIME4 3500
#define HOLD_TIME 12000

#define SCALE1  1.0
#define SCALE2  1.0
#define SCALE3  1.0
#define SCALE4  1.0
#define END_SCALE 1.0

#define START_SCALE 0.5
#define SCALE_PHASES 40
#define ENLAGRE_TIME 3600
#define ENLAGRE_START_PHASE 5
#define PHASE_TIME (ENLAGRE_TIME / SCALE_PHASES)
#define HOLD_PHASE (ENLAGRE_START_PHASE + SCALE_PHASES - 1)

NMusixHint::NMusixHint() : QWidget(0, "MusixHint", WStyle_Customize | WStyle_NoBorderEx | WStyle_StaysOnTop),
#ifdef TESTING
	ok_(false),
#endif
	backpixmap_(0), part_(PART1), phase_(0), scale_(1.0) {
#if QT_VERSION >= 300
	setBackgroundMode(Qt::NoBackground);
#else
	setBackgroundMode(NoBackground);
#endif
#ifdef TESTING
	musixwarn1_ = new QPixmap("warn1.png");
	musixwarn2_ = new QPixmap("warn2.png");
	ok_ = !musixwarn1_->isNull() && !musixwarn2_->isNull();
#endif
}

void NMusixHint::hide() {
	phase_ = 0;
	scale_ = 1.0;
	setGeometry( 10, 10, 1, 1);
#ifdef TESTING
	ok_ = false;
#endif
	if (backpixmap_) delete backpixmap_;
	backpixmap_ = 0;
	QWidget::hide();
}

void NMusixHint::draw() {
	QPainter painter;
	
#ifdef TESTING
	if (!ok_) {
		printf("ok_ == FALSE\n"); return;
	}
#endif
	if (phase_ == 0) {
		setGeometry( 10, 10, 1, 1);
		return;
	}
	painter.begin(backpixmap_);
	painter.fillRect(0, 0, currentPixmap_->width(), currentPixmap_->height(), QBrush(QColor(0, 0, 0)));
	if (phase_ < ENLAGRE_START_PHASE) {
		painter.setClipping(true);
		painter.setClipRect(QRect(QPoint(0,0), QPoint(currentPixmap_->width(), (int) (part_ * scale_ * (double) currentPixmap_->height()))));
	}
	else {
		 painter.setClipping(false);
	}
	painter.scale(scale_, scale_);
	painter.drawPixmap (0, 0, *currentPixmap_);
	painter.end();
	bitBlt(this, 0, 0, backpixmap_);
}


void NMusixHint::show() {
	part_ = PART1;
	scale_ = SCALE1;
	phase_ = 1;
#ifdef TESTING
	currentPixmap_ = musixwarn1_;
	KAudioPlayer::play("fanfare.mp3");
#else
	currentPixmap_ = NResource::musixwarn1_;
	if (!NResource::fanfareFile_.isEmpty()) {
		KAudioPlayer::play(NResource::fanfareFile_);
	}
#endif
	backpixmap_ = new QPixmap(currentPixmap_->width(), currentPixmap_->height());
	setGeometry( 10, 10, currentPixmap_->width(), currentPixmap_->height());
	QTimer::singleShot(TIME1, this, SLOT(change_part()));
	QWidget::show();
}


void NMusixHint::paintEvent ( QPaintEvent * ) {
	if (phase_ == 0) return;
	draw();
}

void NMusixHint::mousePressEvent ( QMouseEvent * e ) {
	if (phase_ == HOLD_PHASE+1) {
		hide();
	}
}

void NMusixHint::keyPressEvent ( QKeyEvent * e ) {
	if (phase_ == HOLD_PHASE+1) {
		hide();
	}
}

void NMusixHint::change_part() {
	switch (phase_) {
		case 1: part_ = PART2;
			scale_= SCALE2;
			phase_ = 2;
			QTimer::singleShot(TIME2, this, SLOT(change_part()));
			repaint();
			break;
		case 2: part_ = PART3;
			scale_= SCALE3;
			phase_ = 3;
			QTimer::singleShot(TIME3, this, SLOT(change_part()));
			repaint();
			break;
		case 3: part_ = PART4;
			scale_= SCALE4;
			phase_ = 4;
			QTimer::singleShot(TIME4, this, SLOT(change_part()));
			repaint();
			break;
		case 4: part_ = END_PART;
			scale_= START_SCALE;
			phase_ = ENLAGRE_START_PHASE;
#ifdef TESTING
			currentPixmap_ = musixwarn2_;
#else
			currentPixmap_ = NResource::musixwarn2_;
#endif
			if (backpixmap_) delete backpixmap_;
			backpixmap_ = new QPixmap(currentPixmap_->width(), currentPixmap_->height());
			setGeometry( 10, 10, currentPixmap_->width(), currentPixmap_->height());
			QTimer::singleShot(PHASE_TIME, this, SLOT(change_part()));
			repaint();
			break;
		case HOLD_PHASE: 
			part_ = END_PART;
			scale_= END_SCALE;
			phase_ = HOLD_PHASE+1;
			QTimer::singleShot(HOLD_TIME, this, SLOT(change_part()));
			repaint();
			break;
		case HOLD_PHASE+1: 
			hide();
			phase_ = 0;
			break;
		case  0: break;
		default: 
			scale_ = START_SCALE + ((double) (phase_ - ENLAGRE_START_PHASE + 1)) / ((double) SCALE_PHASES) * (END_SCALE - START_SCALE);
			part_ = END_PART;
			phase_++;
			QTimer::singleShot(PHASE_TIME, this, SLOT(change_part()));
			repaint();
			break;
			
			


	}
}

#include "musixhint.moc"
