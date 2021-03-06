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

#ifndef TABCOLUMN_H
#define TABCOLUMN_H

#include "global.h"
#include <qglobal.h>
#include <qglobal.h>

// Durations as in MIDI:
// 480 = whole
// 240 = half
// 120 = quarter
// 60  = eighth
// 30  = 16th
// 15  = 32nd

#define FLAG_ARC        1
#define FLAG_DOT        2
#define FLAG_PM         4
#define FLAG_TRIPLET	8

#define EFFECT_HARMONIC 1
#define EFFECT_ARTHARM  2
#define EFFECT_LEGATO   3
#define EFFECT_SLIDE    4
#define EFFECT_LETRING	5

#define NULL_NOTE       -1
#define DEAD_NOTE       -2

class TabColumn {
public:
	int l;                              // Duration of note or chord
	char a[MAX_STRINGS];                // Number of fret
	char e[MAX_STRINGS];                // Effect parameter
	uint flags;                         // Various flags

	Q_UINT16 fullDuration();
	void setFullDuration(Q_UINT16 len);

	uint effectFlags();
};

#endif
