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

#ifndef LAYOUT_H

#define LAYOUT_H
#include <qdialog.h>
#include <qpainter.h>
#include <qbrush.h>
#include <qpoint.h>
#include <qrect.h>
#include <qpushbutton.h>
#include <qapplication.h>
#include <qpixmap.h>
#include <qstring.h>
#include <qlist.h>
class NStaff;
class layoutDef {
	public:
		layoutDef() {
			valid = false;
		}
		layoutDef(int b, int e) {
			beg = b, end = e, valid = true;
		}
		void set(int b, int e, bool v) {beg = b, end = e, valid = v;}
		int beg, end;
		bool valid;
};

class NStaffLayout : public QDialog {
	Q_OBJECT
	public:
		NStaffLayout(int staffCount, layoutDef *braceMatrix, layoutDef *bracketMatrix, layoutDef *barCont, QList<NStaff> *stafflist, QWidget *parent, char *name);
		~NStaffLayout();
		bool hasChanged() {return hasChanged_;}
	protected:
		virtual void paintEvent ( QPaintEvent * );
		virtual void resizeEvent ( QResizeEvent *evt);
		virtual void mousePressEvent ( QMouseEvent * evt );
		virtual void mouseMoveEvent ( QMouseEvent * evt );
	private:
		int staffCount_;
		QPainter p_;
		QPushButton quit_;
		QPushButton cancel_;
		QPushButton brace_;
		QPushButton bracket_;
		QPushButton bar_;
		QPushButton rembrace_;
		QPushButton rembracket_;
		QPushButton rembar_;
		QColor grayColor_;
		QBrush whiteBrush_;
		QRect papersize_;
		QPen blackPen_;
		QPen bluePen_;
		QRect selRect_;
		bool selRectValid_;
		QPoint p0_;
		QPixmap *backpixmap_;
		QString *nameArray_;
		int minh_, maxh_;
		layoutDef *braceMatrix_;
		layoutDef *bracketMatrix_;
		layoutDef *barCont_;
		layoutDef *localBraceMatrix_;
		layoutDef *localBracketMatrix_;
		layoutDef *localBarCont_;
		QList<NStaff> *staffList_;
		bool hasChanged_;
	private slots:
		void slOk();
		void slCancel();
		void slSetBrace();
		void slSetBracket();
		void slRemBrace();
		void slRemBracket();
		void slContBar();
		void slDisContBar();
};


#endif /* LAYOUT_H */
