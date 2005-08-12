/*****************************************************************************/
/* This file is a modification of the original "fingerlist.h" from           */
/* the "kguitar" program version 0.4. As of this writing the whole programs  */
/* was available from:                                                       */
/*                                                                           */
/*    http://kguitar.sourceforge.net                                         */
/*                                                                           */
/* The modifications mainly concern KDE3 support.                            */
/*                                                                           */
/*****************************************************************************/
/* J.Anders <ja@informatik.tu-chemnitz.de> 17.05.2002                        */
/*****************************************************************************/

#ifndef FINGERLIST_H
#define FINGERLIST_H

#include <qgridview.h>
#include "global.h"

#define ICONCHORD      55

class TabTrack;

typedef struct {
    int f[MAX_STRINGS];
} fingering;

class FingerList: public QGridView
{
    Q_OBJECT
public:
    FingerList(TabTrack *p, QWidget *parent=0, const char *name=0);

    void addFingering(const int a[MAX_STRINGS]);
    void clear();
	void beginSession();
	void endSession();
    void setFirstChord();

signals:
    void chordSelected(const int *);

protected:
    virtual void paintCell(QPainter *, int row, int col);
    virtual void resizeEvent(QResizeEvent *); 
    virtual void mousePressEvent(QMouseEvent *);

private:
    enum { SCALE=6, CIRCLE=4, CIRCBORD=1, BORDER=3, SPACER=1, FRETTEXT=12 };
    
    int num,perRow;
    QArray<fingering> appl;

    int curSel,oldCol,oldRow;
    TabTrack *parm;
};

#endif
