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

#ifndef TRANSPAINTER_H

#define TRANSPAINTER_H
#include <qpainter.h>

class NTransPainter : public QPainter {
	public:
		NTransPainter(QPaintDevice *pd = 0, int transx = 0, int transy = 0);
		void setPaintDevice(QPaintDevice *pd);
		void setXPosition(int transx);
		void setYPosition(int transy);
		void beginTranslated();
		void drawScaledText(QPoint pos, QString text);
		void drawScaledText(int x, int y, QString text);
		void toggleToScaledText(bool on);
		void beginYtranslated();
		void beginUnclippedYtranslated();
		void beginTextDrawing();
		void noticeClipRect( QRect cr );
		int getLeftBorder() {return leftBorder_;}
		int getLeftX() {return transx_;}
		void setZoom(float zoom) {zoom_ = zoom; computeRealLeftBorder();}
	private:
		void computeRealLeftBorder();
		QPaintDevice *pd_;
		int transx_;
		int transy_;
		int leftBorder_;
		float zoom_;
		QRect cr_;
};

#endif // TRANSPAINTER_H
