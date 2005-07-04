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

#ifndef DBUFFERWIDGET_H

#define DBUFFERWIDGET_H

#include <qwidget.h>
#include <qdatetime.h> 

class QPixmap;
class NMainFrameWidget;

class NDbufferWidget : public QWidget {
	public:
		NDbufferWidget(NMainFrameWidget *parent, char *name);
		~NDbufferWidget();
/*
		QPixmap *acShowPixmap() {return backpixmap_[showpixmap_];}
*/
		QPixmap *acShowPixmap();
		QPixmap *acWritePixmap() {return backpixmap_[1-showpixmap_];}
		void swap() {showpixmap_ = 1 - showpixmap_;}
		void set1backpixmap(int width, int height);
		void set2backpixmaps();
		void flip();
	protected:
		virtual void paintEvent ( QPaintEvent * ); 
		virtual void mousePressEvent ( QMouseEvent * evt);
		virtual void mouseMoveEvent ( QMouseEvent * evt);
		virtual void mouseReleaseEvent ( QMouseEvent * );
		virtual void wheelEvent ( QWheelEvent * e );
		virtual void leaveEvent( QEvent *);
		virtual void enterEvent( QEvent *);
		NMainFrameWidget *mainFrame_;
		QPixmap *backpixmap_[2];
		int showpixmap_;
};

#endif // DBUFFERWIDGET_H
