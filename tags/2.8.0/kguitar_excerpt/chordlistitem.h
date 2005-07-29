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

#ifndef CHORDLISTITEM_H
#define CHORDLISTITEM_H

#include <qlistbox.h>
#include <qstring.h>
/*
#include "global.h"
#include "globaloptions.h"
*/

class ChordListItem: public QListBoxText
{
public:
    ChordListItem(int _tonic, int _bass, int s3, int s5, int s7,
		  int s9, int s11, int s13);
    int tonic() { return t; };
    int step(int x) { return s[x]; };

private:
    int t;
    int s[6];
};

#endif
