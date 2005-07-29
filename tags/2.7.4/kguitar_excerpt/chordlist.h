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

#ifndef CHORDLIST_H
#define CHORDLIST_H

#include <qlistbox.h>
//#include "global.h"

#include "chordlistitem.h"

class ChordList: public QListBox
{
    Q_OBJECT
public:
    ChordList(QWidget *parent=0, const char *name=0);
    ChordListItem* currentItemPointer();
    void inSort(ChordListItem *it);
};

#endif
