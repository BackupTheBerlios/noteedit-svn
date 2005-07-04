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

#include "chordlist.h"

ChordList::ChordList(QWidget *parent, const char *name)
	: QListBox(parent, name)
{
}

void ChordList::inSort(ChordListItem *it)
{
    uint l = ((QString) it->text()).length();
    uint best = 0;

    for (uint i=0;i<count();i++) {
		if (((QString) item(i)->text()).length()<l)
			best++;
		else
			break;
    }

    insertItem(it,best);
}

ChordListItem* ChordList::currentItemPointer()
{
    return (ChordListItem*) item(currentItem());
}

#include "chordlist.moc"
