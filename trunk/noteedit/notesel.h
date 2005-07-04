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

#ifndef notesel_h
#define notesel_h

//#include <qpainter.h>
#include <qobject.h>

#define IS_CLEF	1
#define IS_CLEF_DISTANCE 2
#define IS_TIME	3

class QFrame;
class QScrollBar;
class QPainter;
class QTimer;
class QPixmap;

class noteSel : public QWidget {

    Q_OBJECT

    public:
	noteSel( QFrame *parent );
	~noteSel();
	void setType( unsigned char type );
	int getSelection();

    public slots:
	void resiz();
	void clearIt();

    private:
	void mousePressEvent( QMouseEvent *evnt );

    private:
	QScrollBar *scroll_;
	QFrame *parent_;
	QPainter *paint_;
	QTimer *timer_;

	int elem_amount_;
	QPixmap *pixms_;
	int *dists_;
	int selected_;
	int type_;

    };


    
#endif // notesel_h
