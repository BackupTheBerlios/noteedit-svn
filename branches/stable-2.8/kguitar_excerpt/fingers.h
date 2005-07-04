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

#ifndef FINGERS_H
#define FINGERS_H

#include <qframe.h>
#include "global.h"

class QScrollBar;
class TabTrack;

class Fingering: public QFrame
{
    Q_OBJECT
public:
    Fingering(TabTrack *p, QWidget *parent = 0, const char *name = 0);

    void setFinger(int string, int fret);

    int  app(int x) { return appl[x]; }
    void setApp(int x, int fret) { appl[x]=fret; }
    int *getFingerField() {return appl;}

public slots:
    void clear();
    void setFirstFret(int fret);
    void setFingering(const int *);

signals:
    void chordChange();

protected:
    virtual void drawContents(QPainter *);
    virtual void mouseMoveEvent(QMouseEvent *);
    virtual void mousePressEvent(QMouseEvent *);
    void         mouseHandle(const QPoint &pos, bool domute);

private:
    enum { SCALE=20, CIRCLE=16, CIRCBORD=2, BORDER=5, SPACER=3,
	   FRETTEXT=10, SCROLLER=15, NOTES=20 };

    QScrollBar *ff;
    TabTrack *parm;

    int appl[MAX_STRINGS];
    int lastff;
};

#endif
