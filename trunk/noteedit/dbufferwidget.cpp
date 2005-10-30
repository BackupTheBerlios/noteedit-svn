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
#include <qdatetime.h> 
#include "dbufferwidget.h"
#include "mainframewidget.h"


NDbufferWidget::NDbufferWidget(NMainFrameWidget *parent = 0, char *name = 0) :
	 QWidget(parent, name) {
	backpixmap_ [0] = 0;
	backpixmap_ [1] = 0;
	mainFrame_ = parent;
	showpixmap_ = 0;
	setMouseTracking(true);
#if QT_VERSION >= 300
	setBackgroundMode(Qt::NoBackground);
#else
	setBackgroundMode(NoBackground);
#endif

}

NDbufferWidget::~NDbufferWidget() {
	if (backpixmap_[1]) delete backpixmap_[1];
	if (backpixmap_[0]) delete backpixmap_[0];
}

void NDbufferWidget::set2backpixmaps() {
	if (backpixmap_ [0] == 0) {
		NResource::abort("internal error: set2backpixmaps: backpixmap_ [0] == 0");
	}

	backpixmap_[1] = new QPixmap(backpixmap_[0]->width(), backpixmap_[0]->height());
}

QPixmap *NDbufferWidget::acShowPixmap() {
	return backpixmap_[showpixmap_];
}

void NDbufferWidget::set1backpixmap(int width, int height) {
	if (width <= 0 || height <= 0) return;
	if (backpixmap_[1]) delete backpixmap_[1];
	if (backpixmap_[0]) delete backpixmap_[0];
	backpixmap_[1] = 0;
	backpixmap_[0] = new QPixmap(width, height);
	showpixmap_ = 0;
}
	

void NDbufferWidget::flip() {
	bitBlt(this, 0, 0, backpixmap_[showpixmap_]);
}

void NDbufferWidget::paintEvent ( QPaintEvent * ) {
	bitBlt(this, 0, 0, backpixmap_[showpixmap_]);
}

void NDbufferWidget::mousePressEvent ( QMouseEvent * evt)  {
	mainFrame_->processMouseEvent(evt);
}

void NDbufferWidget::leaveEvent( QEvent *) {
	setMouseTracking(false);
	mainFrame_->restoreAllBehindDummyNoteAndAuxLines();
	if (!mainFrame_->isPlaying()) setMouseTracking(NResource::showAuxLines_);
}

void NDbufferWidget::enterEvent( QEvent *) {
	setMouseTracking(false);
	mainFrame_->restoreAllBehindDummyNoteAndAuxLines();
	if (!mainFrame_->isPlaying()) setMouseTracking(NResource::showAuxLines_);
}

void NDbufferWidget::mouseMoveEvent ( QMouseEvent * evt)  {
	mainFrame_->setDummyNoteAndAuxLines(evt);
	if (!(evt->state() & LeftButton))  return;
	mainFrame_->processMoveEvent(evt);
}

void NDbufferWidget::wheelEvent( QWheelEvent * e ) {
	mainFrame_->processWheelEvent(e);
}

void NDbufferWidget::mouseReleaseEvent ( QMouseEvent * )  {
	mainFrame_->stopTimer();
}

