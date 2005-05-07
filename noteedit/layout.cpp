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

#include "layout.h"
#include "staff.h"
#include "resource.h"
#include <string.h>
#include <klocale.h>

#define LAYOUT_BORDER 10 
#define MINWIDTH 700
#define STAFF_LINE_DIST 4
#define STAFF_HEIGHT (STAFF_LINE_DIST*4)
#define STAFF_DIST (STAFF_LINE_DIST * 3)

#define BUTTON_Y_DIST 10
#define BUTTON_X_DIST 10
#define BUTTON_WIDTH 60
#define FUNC_BUTTON_WIDTH 230
#define BUTTON_HEIGHT 25

#define LEFT_STAFF_BORDER 100
#define RIGHT_STAFF_BORDER 5
#define BRACE_X_POINT 160
#define BRACKET_X_POINT 175
#define BAR_X_POINT 180
#define BAR_X_DIST  80
#define NUM_BARS 5

#define TEXTBEGIN 5
#define TEXTOFFS  (2*STAFF_LINE_DIST)


NStaffLayout::NStaffLayout(int staffCount, layoutDef *braceMatrix, layoutDef *bracketMatrix, layoutDef *barCont, QList<NStaff> *stafflist, QWidget *parent, char *name) :
	QDialog(parent, name), 
	quit_("Ok", this), cancel_(i18n ("Cancel"), this), brace_(i18n ("Set brace"), this), bracket_(i18n ("Set bracket"), this), bar_ ( i18n ("Continue bar"), this),
	rembrace_( i18n ("Remove brace"), this), rembracket_(i18n("Remove bracket"), this), rembar_ ( i18n ("Discontinue bar"), this),
	grayColor_(160, 160, 160), whiteBrush_(QColor(255, 255,255)), blackPen_(QColor(0, 0, 0)), bluePen_(QColor(0, 0, 255)), selRectValid_(false), 
	backpixmap_(0), hasChanged_(true) {
		staffCount_ = staffCount;
    		setMinimumSize( MINWIDTH, 
		staffCount_ * (STAFF_HEIGHT+STAFF_DIST)+STAFF_DIST +2*LAYOUT_BORDER+4*BUTTON_Y_DIST+3*BUTTON_HEIGHT);
    		setBackgroundColor( QColor(200, 200, 200) );
		staffList_ = stafflist;
		braceMatrix_ = braceMatrix;
		bracketMatrix_ = bracketMatrix;
		barCont_ = barCont;

		localBraceMatrix_ = new layoutDef[staffCount_];
		localBracketMatrix_ = new layoutDef[staffCount_];
		localBarCont_ = new layoutDef[staffCount_];

		memcpy(localBraceMatrix_, braceMatrix_, staffCount_*sizeof(layoutDef));
		memcpy(localBracketMatrix_, bracketMatrix_, staffCount_*sizeof(layoutDef));
		memcpy(localBarCont_, barCont_, staffCount_*sizeof(layoutDef));

		connect(&quit_, SIGNAL(clicked()), this, SLOT(slOk()));
		connect(&cancel_, SIGNAL(clicked()), this, SLOT(slCancel()));
		connect(&brace_, SIGNAL(clicked()), this, SLOT(slSetBrace()));
		connect(&bracket_, SIGNAL(clicked()), this, SLOT(slSetBracket()));
		connect(&rembrace_, SIGNAL(clicked()), this, SLOT(slRemBrace()));
		connect(&rembracket_, SIGNAL(clicked()), this, SLOT(slRemBracket()));
		connect(&bar_, SIGNAL(clicked()), this, SLOT(slContBar()));
		connect(&rembar_, SIGNAL(clicked()), this, SLOT(slDisContBar()));
#if QT_VERSION >= 300
		setBackgroundMode(Qt::NoBackground);
#else
		setBackgroundMode(NoBackground);
#endif
}

NStaffLayout::~NStaffLayout() {
	if (backpixmap_) delete backpixmap_;
	delete localBraceMatrix_;
	delete localBracketMatrix_;
	delete localBarCont_;
}


void NStaffLayout::resizeEvent(QResizeEvent *evt) {
	if (backpixmap_) delete backpixmap_;
	backpixmap_ = new QPixmap(evt->size().width(), evt->size().height());
	papersize_ = QRect(LAYOUT_BORDER, LAYOUT_BORDER, evt->size().width() - 2* LAYOUT_BORDER, evt->size().height() - LAYOUT_BORDER - 3*BUTTON_HEIGHT - 4 * BUTTON_Y_DIST);
	quit_.setGeometry(BUTTON_X_DIST, evt->size().height() - 3*BUTTON_Y_DIST -3*BUTTON_HEIGHT, BUTTON_WIDTH, BUTTON_HEIGHT);
	cancel_.setGeometry(2*BUTTON_X_DIST + BUTTON_WIDTH,
	evt->size().height() - 3*BUTTON_Y_DIST -3*BUTTON_HEIGHT, 2*BUTTON_WIDTH, BUTTON_HEIGHT);
	brace_.setGeometry(3*BUTTON_X_DIST + 3*BUTTON_WIDTH,
	evt->size().height() - 3*BUTTON_Y_DIST -3*BUTTON_HEIGHT, FUNC_BUTTON_WIDTH, BUTTON_HEIGHT);
	bracket_.setGeometry(3*BUTTON_X_DIST + 3*BUTTON_WIDTH,
	evt->size().height() - 2*BUTTON_Y_DIST -2*BUTTON_HEIGHT, FUNC_BUTTON_WIDTH, BUTTON_HEIGHT);
	bar_.setGeometry(3*BUTTON_X_DIST + 3*BUTTON_WIDTH,
	evt->size().height() - BUTTON_Y_DIST -BUTTON_HEIGHT, FUNC_BUTTON_WIDTH, BUTTON_HEIGHT);
	rembrace_.setGeometry(4*BUTTON_X_DIST + 3*BUTTON_WIDTH+FUNC_BUTTON_WIDTH,
	evt->size().height() - 3*BUTTON_Y_DIST -3*BUTTON_HEIGHT, FUNC_BUTTON_WIDTH, BUTTON_HEIGHT);
	rembracket_.setGeometry(4*BUTTON_X_DIST + 3*BUTTON_WIDTH+FUNC_BUTTON_WIDTH,
	evt->size().height() - 2*BUTTON_Y_DIST -2*BUTTON_HEIGHT, FUNC_BUTTON_WIDTH, BUTTON_HEIGHT);
	rembar_.setGeometry(4*BUTTON_X_DIST + 3*BUTTON_WIDTH+FUNC_BUTTON_WIDTH,
	evt->size().height() - BUTTON_Y_DIST -BUTTON_HEIGHT, FUNC_BUTTON_WIDTH, BUTTON_HEIGHT);
}

void NStaffLayout::paintEvent ( QPaintEvent * )  {
#define LAYOUT_MANY 100000000
	int i, j, k;
	int h0, h1;
	int mid;
	bool insideContStaff;
	NStaff *staff_elem;
	if (!backpixmap_) return;
	minh_ =  LAYOUT_MANY;
	maxh_ = -LAYOUT_MANY;
	backpixmap_->fill(grayColor_);
	p_.begin(backpixmap_);
	p_.fillRect(papersize_, whiteBrush_);
	blackPen_.setWidth(1);
	p_.setPen(bluePen_);
	for (i = 0; i < staffCount_; i++) {
		h0 = LAYOUT_BORDER+STAFF_DIST+i*(STAFF_HEIGHT+STAFF_DIST);
		if (!selRectValid_ || h0 < selRect_.top() ||  h0 + STAFF_HEIGHT > selRect_.bottom()) {
			p_.setPen(blackPen_);
		}
		else {
			p_.setPen(bluePen_);
			if (minh_ == LAYOUT_MANY)  minh_ = i;
			if (maxh_ == -LAYOUT_MANY) maxh_ = i;
			if (i > maxh_) maxh_ = i;
		}
		staff_elem = staffList_->at(i);
		if (!staff_elem->staffName_.isEmpty()) {
			p_.drawText(LAYOUT_BORDER+TEXTBEGIN, h0+TEXTOFFS, staff_elem->staffName_);
		}
		for (j = 0; j < 5; j++) {
			h1 = h0+j*STAFF_LINE_DIST;
			p_.drawLine(LAYOUT_BORDER+BAR_X_POINT, h1, backpixmap_->width()-2*LAYOUT_BORDER-RIGHT_STAFF_BORDER, h1);
		}
	}
	p_.drawLine(LAYOUT_BORDER+BAR_X_POINT, LAYOUT_BORDER+STAFF_DIST, LAYOUT_BORDER+BAR_X_POINT, LAYOUT_BORDER+STAFF_DIST+(staffCount_-1)*(STAFF_HEIGHT+STAFF_DIST)+STAFF_HEIGHT);
	if (selRectValid_) {
		p_.setPen(bluePen_);
		p_.drawRect(selRect_);
	}
	p_.setPen(blackPen_);
	for (i = 0; i < staffCount_; i++) {
		if (localBracketMatrix_[i].valid) {
			blackPen_.setWidth(3);
			p_.setPen(blackPen_);
			p_.drawLine(LAYOUT_BORDER+BRACKET_X_POINT, LAYOUT_BORDER+STAFF_DIST+localBracketMatrix_[i].beg*(STAFF_HEIGHT+STAFF_DIST),
				LAYOUT_BORDER+BRACKET_X_POINT, LAYOUT_BORDER+STAFF_DIST+localBracketMatrix_[i].end*(STAFF_HEIGHT+STAFF_DIST)+STAFF_HEIGHT);
			blackPen_.setWidth(1);
			p_.setPen(blackPen_);
#define BRACKET_XRAD 20
#define BRACKET_YRAD 40
#define BRACKET_ARC_LENGTH 25
			p_.drawArc(LAYOUT_BORDER+BRACKET_X_POINT - BRACKET_XRAD, LAYOUT_BORDER+STAFF_DIST+localBracketMatrix_[i].beg*(STAFF_HEIGHT+STAFF_DIST)-2*BRACKET_YRAD,
				2*BRACKET_XRAD, 2*BRACKET_YRAD, -90*16, BRACKET_ARC_LENGTH*16);
			p_.drawArc(LAYOUT_BORDER+BRACKET_X_POINT - BRACKET_XRAD, LAYOUT_BORDER+STAFF_DIST+localBracketMatrix_[i].end*(STAFF_HEIGHT+STAFF_DIST)+STAFF_HEIGHT,
				2*BRACKET_XRAD, 2*BRACKET_YRAD, (90-BRACKET_ARC_LENGTH)*16, BRACKET_ARC_LENGTH*16);
		}
	}
	p_.setPen(blackPen_);
	for (i = 0; i < staffCount_; i++) {
		if (localBraceMatrix_[i].valid) {
#define BR_ARROW_XRAD 8
#define BR_ARROW_YRAD 8
#define BRACEX_ARROW 8
#define BRACE_ARC_LENGTH 90
#define MID_ROUNDDIST 9
			blackPen_.setWidth(2);
			p_.setPen(blackPen_);
			mid = LAYOUT_BORDER+STAFF_DIST+(localBraceMatrix_[i].beg*(STAFF_HEIGHT+STAFF_DIST)+(localBraceMatrix_[i].end*(STAFF_HEIGHT+STAFF_DIST)
			-localBraceMatrix_[i].beg*(STAFF_HEIGHT+STAFF_DIST))/2)+STAFF_HEIGHT/2;
			p_.drawLine(LAYOUT_BORDER+BRACE_X_POINT, LAYOUT_BORDER+STAFF_DIST+localBraceMatrix_[i].beg*(STAFF_HEIGHT+STAFF_DIST)+BR_ARROW_YRAD,
				LAYOUT_BORDER+BRACE_X_POINT, mid - MID_ROUNDDIST+2);
			p_.drawLine(LAYOUT_BORDER+BRACE_X_POINT, mid + MID_ROUNDDIST-2, 
				LAYOUT_BORDER+BRACE_X_POINT, LAYOUT_BORDER+STAFF_DIST+localBraceMatrix_[i].end*(STAFF_HEIGHT+STAFF_DIST)+STAFF_HEIGHT-BR_ARROW_YRAD);
			p_.drawArc(LAYOUT_BORDER+BRACE_X_POINT - BR_ARROW_XRAD -  BRACEX_ARROW, mid,
				2*BR_ARROW_XRAD, 2*BR_ARROW_YRAD, (90-BRACE_ARC_LENGTH)*16, BRACE_ARC_LENGTH*16);
			p_.drawArc(LAYOUT_BORDER+BRACE_X_POINT - BR_ARROW_XRAD -  BRACEX_ARROW, mid-2*BR_ARROW_YRAD+1,
				2*BR_ARROW_XRAD, 2*BR_ARROW_YRAD, -90*16, BRACE_ARC_LENGTH*16);
			p_.drawArc(LAYOUT_BORDER+BRACE_X_POINT - BR_ARROW_XRAD + BR_ARROW_XRAD,
				LAYOUT_BORDER+STAFF_DIST+localBraceMatrix_[i].beg*(STAFF_HEIGHT+STAFF_DIST),
				2*BR_ARROW_XRAD, 2*BR_ARROW_YRAD, (180-BRACE_ARC_LENGTH)*16, BRACE_ARC_LENGTH*16);
			p_.drawArc(LAYOUT_BORDER+BRACE_X_POINT - BR_ARROW_XRAD + BR_ARROW_XRAD,
				LAYOUT_BORDER+STAFF_DIST+localBraceMatrix_[i].end*(STAFF_HEIGHT+STAFF_DIST)+STAFF_HEIGHT- 2*BR_ARROW_YRAD,
				2*BR_ARROW_XRAD, 2*BR_ARROW_YRAD, 180*16, BRACE_ARC_LENGTH*16);
		}
	}
	blackPen_.setWidth(1);
	p_.setPen(blackPen_);
	for (i = 0; i < staffCount_; i++) {
		insideContStaff = false;
		for (j = 0; j < staffCount_; j++) {
			if (localBarCont_[j].valid) {
				if (i >= localBarCont_[j].beg  &&  i < localBarCont_[j].end ) {
					insideContStaff = true;
					break;
				}
			}
		}
		for (k = 0; k < NUM_BARS; k++) {
			if (insideContStaff && i < staffCount_-1) {
				p_.drawLine(LAYOUT_BORDER+BAR_X_POINT+k*BAR_X_DIST, LAYOUT_BORDER+STAFF_DIST+i*(STAFF_HEIGHT+STAFF_DIST),
				LAYOUT_BORDER+BAR_X_POINT+k*BAR_X_DIST, LAYOUT_BORDER+STAFF_DIST+(i+1)*(STAFF_HEIGHT+STAFF_DIST));
			}
			else {
				p_.drawLine(LAYOUT_BORDER+BAR_X_POINT+k*BAR_X_DIST, LAYOUT_BORDER+STAFF_DIST+i*(STAFF_HEIGHT+STAFF_DIST),
				LAYOUT_BORDER+BAR_X_POINT+k*BAR_X_DIST, LAYOUT_BORDER+STAFF_DIST+i*(STAFF_HEIGHT+STAFF_DIST)+STAFF_HEIGHT);
			}
		}
	}
	p_.end();
	bitBlt(this, 0, 0, backpixmap_);
}

void NStaffLayout::slRemBrace() {
	int i;

	for (i = 0; i < staffCount_; i++) {
		if (!localBraceMatrix_[i].valid) continue;
		if (minh_ >= localBraceMatrix_[i].beg && minh_ <= localBraceMatrix_[i].end || maxh_ >= localBraceMatrix_[i].beg && maxh_ <= localBraceMatrix_[i].end) {
			localBraceMatrix_[i].valid = false;
		}
	}
	repaint();
}

void NStaffLayout::slRemBracket() {
	int i;

	for (i = 0; i < staffCount_; i++) {
		if (!localBracketMatrix_[i].valid) continue;
		if (minh_ >= localBracketMatrix_[i].beg && minh_ <= localBracketMatrix_[i].end || maxh_ >= localBracketMatrix_[i].beg && maxh_ <= localBracketMatrix_[i].end) {
			localBracketMatrix_[i].valid = false;
		}
	}
	repaint();
}

void NStaffLayout::slDisContBar() {
	int i;

	if (!selRectValid_) return;
	if (minh_ == LAYOUT_MANY) return;
	if (maxh_ == -LAYOUT_MANY) return;

	for (i = 0; i < staffCount_; i++) {
		if (!localBarCont_[i].valid) continue;
		if (minh_ >= localBarCont_[i].beg && minh_ <= localBarCont_[i].end || maxh_ >= localBarCont_[i].beg && maxh_ <= localBarCont_[i].end) {
			localBarCont_[i].valid = false;
		}
	}

	repaint();
}

void NStaffLayout::slOk() {
	memcpy(braceMatrix_, localBraceMatrix_, staffCount_*sizeof(layoutDef));
	memcpy(bracketMatrix_, localBracketMatrix_, staffCount_*sizeof(layoutDef));
	memcpy(barCont_, localBarCont_, staffCount_*sizeof(layoutDef));
	hide();
}

void NStaffLayout::slCancel() {
	hasChanged_ = false;
	hide();
}

void NStaffLayout::slSetBrace() {
	int i;

	if (!selRectValid_) return;
	if (minh_ == LAYOUT_MANY) return;
	if (maxh_ == -LAYOUT_MANY) return;
	if (maxh_ - minh_ < 1) return;

	for (i = 0; i < staffCount_; i++) {
		if (!localBraceMatrix_[i].valid) continue;
		if (localBraceMatrix_[i].end >= minh_ && localBraceMatrix_[i].end <= maxh_ || localBraceMatrix_[i].beg >= minh_ && localBraceMatrix_[i].beg <= maxh_ ||
			minh_ >= localBraceMatrix_[i].beg && maxh_ <=  localBraceMatrix_[i].end) {
			localBraceMatrix_[i].end = minh_ - 1;
			if (localBraceMatrix_[i].end  - localBraceMatrix_[i].beg < 2) {
				localBraceMatrix_[i].valid = false;
			}
		}
	}
	for (i = 0; i < staffCount_; i++) {
		if (!localBracketMatrix_[i].valid) continue;
		if (minh_ >= localBracketMatrix_[i].beg && minh_ <= localBracketMatrix_[i].end && maxh_ > localBracketMatrix_[i].end ||
		    maxh_ >= localBracketMatrix_[i].beg && maxh_ <= localBracketMatrix_[i].end && minh_ < localBracketMatrix_[i].beg) {
			localBracketMatrix_[i].end = minh_ - 1;
			if (localBracketMatrix_[i].end <= localBracketMatrix_[i].beg) {
				localBracketMatrix_[i].valid = false;
			}
		}
	}
	for (i = 0; i < staffCount_; i++) {
		if (!localBraceMatrix_[i].valid) {
			localBraceMatrix_[i].beg = minh_;
			localBraceMatrix_[i].end = maxh_;
			localBraceMatrix_[i].valid = true;
			repaint();
			return;
		}
	}
	NResource::abort("slSetBrace: internal error");
}

void NStaffLayout::slSetBracket() {
	int i;

	if (!selRectValid_) return;
	if (minh_ == LAYOUT_MANY) return;
	if (maxh_ == -LAYOUT_MANY) return;

	for (i = 0; i < staffCount_; i++) {
		if (!localBracketMatrix_[i].valid) continue;
		if (localBracketMatrix_[i].end >= minh_ && localBracketMatrix_[i].end <= maxh_ || localBracketMatrix_[i].beg >= minh_ && localBracketMatrix_[i].beg <= maxh_ ||
			minh_ >= localBracketMatrix_[i].beg && maxh_ <=  localBracketMatrix_[i].end) {
			localBracketMatrix_[i].end = minh_ - 1;
			if (localBracketMatrix_[i].end <= localBracketMatrix_[i].beg) {
				localBracketMatrix_[i].valid = false;
			}
		}
	}
	for (i = 0; i < staffCount_; i++) {
		if (!localBraceMatrix_[i].valid) continue;
		if (minh_ >= localBraceMatrix_[i].beg && minh_ <= localBraceMatrix_[i].end && maxh_ > localBraceMatrix_[i].end ||
		    maxh_ >= localBraceMatrix_[i].beg && maxh_ <= localBraceMatrix_[i].end && minh_ < localBraceMatrix_[i].beg) {
			localBraceMatrix_[i].end = minh_ - 1;
			if (localBraceMatrix_[i].end <= localBraceMatrix_[i].beg) {
				localBraceMatrix_[i].valid = false;
			}
		}
	}
	for (i = 0; i < staffCount_; i++) {
		if (!localBracketMatrix_[i].valid) {
			localBracketMatrix_[i].beg = minh_;
			localBracketMatrix_[i].end = maxh_;
			localBracketMatrix_[i].valid = true;
			repaint();
			return;
		}
	}
	NResource::abort("slSetBracket: internal error");
			
}

void NStaffLayout::slContBar() {
	int i;

	if (!selRectValid_) return;
	if (minh_ == LAYOUT_MANY) return;
	if (maxh_ == -LAYOUT_MANY) return;
	if (maxh_ - minh_ < 1) return;
	for (i = 0; i < staffCount_; i++) {
		if (!localBarCont_[i].valid) continue;
		if (localBarCont_[i].end >= minh_ && localBarCont_[i].end <= maxh_ || localBarCont_[i].beg >= minh_ && localBarCont_[i].beg <= maxh_ ||
			minh_ >= localBarCont_[i].beg && maxh_ <=  localBarCont_[i].end) {
			localBarCont_[i].end = minh_ - 1;
			if (localBarCont_[i].end  - localBarCont_[i].beg < 2) {
				localBarCont_[i].valid = false;
			}
		}
	}
	for (i = 0; i < staffCount_; i++) {
		if (!localBarCont_[i].valid) {
			localBarCont_[i].beg = minh_;
			localBarCont_[i].end = maxh_;
			localBarCont_[i].valid = true;
			repaint();
			return;
		}
	}
	NResource::abort("slContBar: internal error");
}
	

void NStaffLayout::mousePressEvent ( QMouseEvent * evt ) {
	selRectValid_ = false;
	p0_ = QPoint(evt->x(), evt->y());
	repaint();
}


void NStaffLayout::mouseMoveEvent ( QMouseEvent * evt ) {
	/* sets the current set area and changes orientation if selected from bottom-up */
	selRect_ = (p0_.y() < evt->y()) ? (QRect(p0_, QPoint( evt->x(), evt->y()))): (QRect(QPoint(evt->x(), evt->y()), p0_));
	selRectValid_ = true;
	repaint();
}


	
#include "layout.moc"
	
