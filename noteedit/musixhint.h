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

#ifndef MUSIXHINT_H

#define MUSIXHINT_H

#include <qwidget.h>
/*
#define TESTING
*/

class NMusixHint : public QWidget {
	Q_OBJECT
    public:
    	NMusixHint();
    public slots:
        virtual void show();
        virtual void hide();
    protected:
	virtual void paintEvent ( QPaintEvent * );
	virtual void mousePressEvent ( QMouseEvent * e );
	virtual void keyPressEvent ( QKeyEvent * e );
    private slots:
        void change_part();
    private:
    	void draw();
#ifdef TESTING
    	QPixmap *musixwarn1_;
    	QPixmap *musixwarn2_;
	bool ok_;
#endif
	QPixmap *currentPixmap_;
	QPixmap *backpixmap_;
	double part_;
	int phase_;
	double scale_;
};


#endif /* MUSIXHINT_H */
