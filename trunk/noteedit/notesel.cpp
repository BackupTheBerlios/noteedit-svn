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
/*		Christian Fasshauer							*/
/*		mseacf@gmx.net								*/
/*											*/
/*											*/
/****************************************************************************************/

#include <stdio.h>
#include <qframe.h>
#include <qscrollbar.h>
#include <qpainter.h>
#include <qtimer.h>
#include <qpixmap.h>
#include <qfont.h>
#include "notesel.h"
#include "resource.h"

#define NSEL_LINE_DIST	9
#define NSEL_LINE_SEP	70
#define NSEL_LINE_TOP	30
#define NSEL_ELEM_DIST	50

noteSel::noteSel( QFrame *parent , staffelForm *parentWindow ) : QWidget( parent ) {
    selected_ = elem_amount_ = amount_ = 0;
	
    parent_ = parent;
    scroll_ = new QScrollBar( 0, 10, 1, 10, 0, QScrollBar::Vertical, this );
    paint_ = new QPainter( this );
    timer_ = new QTimer( this );
    parentWindow_ = parentWindow; //set the parent's window - needed for ok/cancel events
    connect( timer_, SIGNAL( timeout() ), this, SLOT( resiz() ) );
    connect( scroll_, SIGNAL( valueChanged( int ) ), this, SLOT( clearIt() ) );
    timer_->start( 1 );
	resiz();
}

noteSel::~noteSel() {
    delete scroll_;
    delete paint_;

    timer_->stop();
    delete timer_;

    delete [] pixms_;
    delete [] dists_;
}

void noteSel::resiz() {
	QPainter pn( this );
	QPainter bp( this );
	
	this->setGeometry( 2, 2, parent_->width() - 4, parent_->height() - 4 );
	scroll_->setGeometry( this->width() - 15, 0, 15, this->height() );
	
	paint_->setPen( Qt::black );
	bp.setPen( Qt::blue );
	pn.scale( 0.45, 0.45 );
	
	if( scroll_->value() + ( parent_->height() - NSEL_LINE_TOP ) / NSEL_LINE_SEP > elem_amount_ ) 
	scroll_->setValue( scroll_->value() - 1 );
	
	amount_ = 0;
	for( int s = NSEL_LINE_TOP;
	     s + NSEL_LINE_SEP < parent_->height() && amount_ < elem_amount_;
	     s += NSEL_LINE_SEP, amount_++ ) {
		for( int i = 0; i < 5; i++)
			paint_->drawLine( selected_ == amount_ + scroll_->value() ? 20 : 10,
			                  ( i * NSEL_LINE_DIST ) + s,
			                  parent_->width() - int( selected_ == amount_ + scroll_->value() ? 37 : 27 ),
			                  ( i * NSEL_LINE_DIST ) + s
			                );
		if( selected_ == amount_ + scroll_->value() ) {
			bp.drawLine( 10, s - 10, 10, ( 4 * NSEL_LINE_DIST ) + s + 10 );
			bp.drawLine( 10, s - 10, 20, s - 20 );
			bp.drawLine( 10, ( 4 * NSEL_LINE_DIST ) + s + 10, 20, ( 4 * NSEL_LINE_DIST ) + s + 20 );
			bp.drawLine( parent_->width() - 27, s - 10, parent_->width() - 27, ( 4 * NSEL_LINE_DIST ) + s + 10 );
			bp.drawLine( parent_->width() - 27, s - 10, parent_->width() - 37, s - 20 );
			bp.drawLine( parent_->width() - 27, ( 4 * NSEL_LINE_DIST + s + 10 ), parent_->width() - 37, ( 4 * NSEL_LINE_DIST ) + s + 20 );
		}
	
		switch( type_ ) {
			case IS_CLEF_DISTANCE:
			case IS_CLEF:
				pn.drawPixmap( QPoint( NSEL_ELEM_DIST, ( s / 0.45 ) + dists_[amount_ + scroll_->value()] ), pixms_[amount_ + scroll_->value()] );
				break;
			case IS_TIME: 
				pn.drawPixmap( QPoint( NSEL_ELEM_DIST, ( s / 0.45 ) + dists_[0] ), pixms_[0] ); 
				pn.setFont( QFont( "Times", 60 ) );
				pn.drawText( NSEL_ELEM_DIST + 70, ( s / 0.45 ) + 40, QString( "%1" ).arg( int( ( amount_ + scroll_->value() ) % 24 ) + 1 ) );
				pn.drawText( NSEL_ELEM_DIST + 70, ( s / 0.45 ) + 80, QString( "%1" ).arg( 1 << ( ( ( amount_ + scroll_->value() + 1 ) / 25 ) + 1 ) ) );
				break;
		}
	}
	
	amount_--;
	scroll_->setMaxValue( elem_amount_ - amount_ - 1 );
}

void noteSel::clearIt() {
	this->repaint();
}

void noteSel::mousePressEvent( QMouseEvent *event ) {
	selected_ = ( ( ( ( ( event->y() - ( NSEL_LINE_TOP / 2 ) ) / NSEL_LINE_SEP ) ) % 
		( ( parent_->height() - NSEL_LINE_TOP ) / NSEL_LINE_SEP ) % elem_amount_ ) ) + 
		scroll_->value();
	
	this->clearIt();
}

/* Confirm the selection and close the window */
void noteSel::mouseDoubleClickEvent( QMouseEvent * e ) {
	mousePressEvent(e);
	
	if (parentWindow_)
		parentWindow_->slOk();
}

/* Reaction on scroll event */
void noteSel::wheelEvent( QWheelEvent *e ) {
	scroll_->setValue(scroll_->value() - e->delta()*0.01);
}

void noteSel::keyPressEvent( QKeyEvent *e ) {
	switch (e->key()) {
		case Qt::Key_Return:
			parentWindow_->slOk();
			break;
		case Qt::Key_Escape:
			parentWindow_->slCh();
			break;
		case Qt::Key_Up:
			if (selected_ > 0) {
				selected_--;
				if (scroll_->value() > selected_)
					scroll_->setValue(selected_);
				else if (scroll_->value() + amount_ - 1 < selected_)
					scroll_->setValue(selected_ - amount_);
				
				this->clearIt();
			}
			break;
		case Qt::Key_Down:
			if (selected_ < elem_amount_-1) {
				selected_++;
				if (scroll_->value() + amount_ - 1 < selected_)
					scroll_->setValue(selected_ - amount_);
				else if (scroll_->value() > selected_)
					scroll_->setValue(selected_);
				
			    this->clearIt();
			}
			break;
	}
}

int noteSel::getSelection() {
    return (selected_);
}

void noteSel::setType( unsigned char type ) {

    switch( type_ = type ) {
	case IS_CLEF: 
	case IS_CLEF_DISTANCE:
	    elem_amount_ = 17;	
	    pixms_ = new QPixmap[17];
	    dists_ = new int[17];
	    pixms_[0] = *NResource::treblePixmap_;
	    pixms_[1] = *NResource::bassPixmap_;
		pixms_[2] = *NResource::altoPixmap_;
		pixms_[3] = *NResource::altoPixmap_;
	    pixms_[4] = *NResource::altoPixmap_;
	    pixms_[5] = *NResource::treblepPixmap_;
	    pixms_[6] = *NResource::basspPixmap_;
		pixms_[7] = *NResource::altopPixmap_;
	    pixms_[8] = *NResource::altopPixmap_;
	    pixms_[9] = *NResource::altopPixmap_;
	    pixms_[10] = *NResource::treblemPixmap_;
	    pixms_[11] = *NResource::bassmPixmap_;
		pixms_[12] = *NResource::altomPixmap_;
	    pixms_[13] = *NResource::altomPixmap_;
	    pixms_[14] = *NResource::altomPixmap_;
	    pixms_[15] = *NResource::drumClefPixmap_;
	    pixms_[16] = *NResource::drumBassClefPixmap_;
		
	    dists_[0] = -50;
	    dists_[1] = -20;
	    dists_[2] = 20;
		dists_[3] = -20;
	    dists_[4] = -40;
	    dists_[5] = -60;
	    dists_[6] = -30;
	    dists_[7] = 5;
		dists_[8] = -35;
	    dists_[9] = -55;
	    dists_[10] = -40;
	    dists_[11] = -5;
		dists_[12] = 35;
	    dists_[13] = -5;
	    dists_[14] = -25;
	    dists_[15] = -20;
	    dists_[16] = -20;
	     break;
	case IS_TIME:
	    elem_amount_ = 7 * 24;
	    pixms_ = new QPixmap[1];
	    dists_ = new int[1];
	    pixms_[0] = *NResource::treblePixmap_;
	    dists_[0] = -50;
	    break;
	}

    }

#include "notesel.moc"
