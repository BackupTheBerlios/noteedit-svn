/************************************************************************************/
/* This is the partly modified file "tabtrack.cpp" from                             */
/* the "kguitar" program version 0.4.1. As of this writing the whole programs       */
/* was available from:                                                              */
/*                                                                                  */
/*    http://kguitar.sourceforge.net                                                */
/*                                                                                  */
/* The modifications mainly concern KDE3/Qt3 support.                               */
/*                                                                                  */
/************************************************************************************/
/* J.Anders <ja@informatik.tu-chemnitz.de> 04.09.2002                               */
/************************************************************************************/

#include "fingerlist.h"
#include "global.h"
#include "tabtrack.h"

#include "config.h"
#if GCC_MAJ_VERS > 2
#include <istream>
#else
#include <istream.h>
#endif
#include <qpainter.h>
#include <qcolor.h>
#if QT_VERSION >= 300
#include <qstyle.h>
#endif

#include <kglobalsettings.h>

FingerList::FingerList(TabTrack *p, QWidget *parent, const char *name):
#if QT_VERSION < 300
QTableView(parent, name)
#else
QGridView(parent, name)
#endif
{
    parm = p;

#if QT_VERSION < 300
    setTableFlags(Tbl_autoVScrollBar | Tbl_smoothScrolling);
#else
    setVScrollBarMode(QScrollView::Auto);
#endif
    setFrameStyle(Panel | Sunken);
    setBackgroundMode(PaletteBase);
    setFocusPolicy(StrongFocus);
    num = 0; curSel = -1; oldCol = 0; oldRow = 0;

    setCellWidth(ICONCHORD);
    setCellHeight(ICONCHORD);

    setMinimumSize(ICONCHORD + 2, ICONCHORD + 2);
    resize(width(), 3 * ICONCHORD + 2);

    repaint();
}

void FingerList::beginSession() {
#if QT_VERSION < 300
	setAutoUpdate(FALSE);
#endif
	clear();
}

void FingerList::endSession()
{
    // num is overral number of chord fingerings. If it's 0 - then there are no
    // fingerings. In the appl array, indexes should be ranged from 0 to (num-1)
    setNumRows((num - 1) / perRow + 1);

#if QT_VERSION < 300
	setAutoUpdate(TRUE);
	repaint();
#else
    repaintContents(true);
#endif
}

void FingerList::clear()
{
    appl.resize(0);
    num = 0; curSel = -1;
    oldCol = 0; oldRow = 0;
}

void FingerList::addFingering(const int a[MAX_STRINGS])
{
    appl.resize((num + 1) * MAX_STRINGS);

    for (int i = 0; i < MAX_STRINGS; i++)
		appl[num].f[i] = a[i];

    num++;
}

void FingerList::resizeEvent(QResizeEvent *e)
{
#if QT_VERSION < 300
	QTableView::resizeEvent(e);
#else
	QGridView::resizeEvent(e);
#endif
    perRow = width() / ICONCHORD;
    setNumCols(perRow);
    setNumRows((num - 1) / perRow + 1);
#if QT_VERSION >= 300
    repaintContents(true);
#endif
}

void FingerList::mousePressEvent(QMouseEvent *e)
{
    int col = e->x() / ICONCHORD;
#if QT_VERSION < 300
    int row = (e->y() + yOffset()) / ICONCHORD;
#else
    int row = (e->y() + contentsY ()) / ICONCHORD;
#endif

    int n = row * perRow + col;

    if ((n >= 0) && (n < num)) {
		curSel = row * perRow + col;
#if QT_VERSION < 300
		repaint(oldCol * ICONCHORD, oldRow * ICONCHORD - yOffset(),
				ICONCHORD, ICONCHORD);
		repaint(col * ICONCHORD, row * ICONCHORD - yOffset(),
				ICONCHORD, ICONCHORD);
#else
		repaintCell(oldRow, oldCol);
		repaintCell(row, col);
#endif
		oldCol = col;
		oldRow = row;
		emit chordSelected(appl[curSel].f);
    }
}

void FingerList::setFirstChord() {
	if (!num) return;
	oldCol = 0;
	oldRow = 0;
	curSel = 0;
#if QT_VERSION < 300
	repaint(0, 0 - yOffset(), ICONCHORD, ICONCHORD);
#else
	repaintCell(0, 0);
#endif
	chordSelected(appl[0].f);
}
	

void FingerList::paintCell(QPainter *p, int row, int col)
{
    int n = row*perRow+col;

    p->setFont(QFont ("Times" , 10, QFont::Normal, true ));
    if (n<num) {
		int barre, eff;
		QColor back = KGlobalSettings::baseColor();
		QColor fore = KGlobalSettings::textColor();

		// Selection painting

		if (curSel == n) {
			back = QColor(200, 200, 200);
			fore = QColor(0, 0, 255);

			p->setBrush(back);
			p->setPen(NoPen);
			p->drawRect(0,0,ICONCHORD-1,ICONCHORD-1);

			if (hasFocus()) {
				p->setBrush(NoBrush);
				p->setPen(fore);
#if QT_VERSION < 300
				style().drawFocusRect(p, QRect(0, 0, ICONCHORD - 1, ICONCHORD - 1), colorGroup(), 0, TRUE);
#endif
			}
		}

		p->setPen(fore);

		// Horizontal lines

		for (int i = 0; i <= NUMFRETS; i++)
			p->drawLine(SCALE/2+BORDER+FRETTEXT,BORDER+SCALE+2*SPACER+i*SCALE,
						SCALE/2+BORDER+parm->string*SCALE-SCALE+FRETTEXT,
						BORDER+SCALE+2*SPACER+i*SCALE);

		// Beginning fret number

		int firstFret = parm->frets;
		bool noff = TRUE;

		for (int i = 0; i < parm->string; i++) {
			if ((appl[n].f[i] < firstFret) && (appl[n].f[i] > 0))
				firstFret = appl[n].f[i];
			if (appl[n].f[i] > 5)
				noff = FALSE;
		}

		if (noff)
			firstFret = 1;

		if (firstFret > 1) {
			QString fs;
			fs.setNum(firstFret);
			p->drawText(BORDER, BORDER + SCALE + 2 * SPACER, 50, 50,
						AlignLeft | AlignTop, fs);
		}

		// Vertical lines and fingering

		for (int i = 0; i < parm->string; i++) {
			p->drawLine(i * SCALE + BORDER + SCALE / 2 + FRETTEXT,
						BORDER + SCALE + 2 * SPACER,
						i * SCALE + BORDER + SCALE / 2 + FRETTEXT,
						BORDER + SCALE + 2 * SPACER + NUMFRETS * SCALE);
			if (appl[n].f[i] == -1) {
				p->drawLine(i*SCALE+BORDER+CIRCBORD+FRETTEXT,BORDER+CIRCBORD,
							i*SCALE+BORDER+SCALE-CIRCBORD+FRETTEXT,
							BORDER+SCALE-CIRCBORD);
				p->drawLine(i*SCALE+BORDER+SCALE-CIRCBORD+FRETTEXT,BORDER+CIRCBORD,
							i*SCALE+BORDER+CIRCBORD+FRETTEXT,BORDER+SCALE-CIRCBORD);
			} else if (appl[n].f[i]==0) {
				p->setBrush(back);
				p->drawEllipse(i*SCALE+BORDER+CIRCBORD+FRETTEXT,BORDER+CIRCBORD,
							   CIRCLE,CIRCLE);
			} else {
				p->setBrush(fore);
				p->drawEllipse(i*SCALE+BORDER+CIRCBORD+FRETTEXT,
							   BORDER+SCALE+2*SPACER+(appl[n].f[i]-firstFret)*SCALE+
							   CIRCBORD,CIRCLE,CIRCLE);
			}
		}

		// Analyze & draw barre

		p->setBrush(fore);

		for (int i=0;i<NUMFRETS;i++) {
			barre=0;
			while ((appl[n].f[parm->string-barre-1]>=(i+firstFret)) ||
				   (appl[n].f[parm->string-barre-1]==-1)) {
				barre++;
				if (barre>parm->string-1)
					break;
			}

			while ((appl[n].f[parm->string-barre]!=(i+firstFret)) && (barre>1))
				barre--;

			eff = 0;
			for (int j = parm->string-barre; j < parm->string; j++) {
				if (appl[n].f[j] != -1)
					eff++;
			}

			if (eff > 2) {
				p->drawRect((parm->string-barre) * SCALE + SCALE / 2 +
							BORDER + FRETTEXT,
							BORDER + SCALE + 2 * SPACER + i * SCALE + CIRCBORD,
							(barre - 1) * SCALE, CIRCLE);
			}
		}

		p->setBrush(NoBrush);
		p->setPen(SolidLine);
    }
}


#include "fingerlist.moc"
