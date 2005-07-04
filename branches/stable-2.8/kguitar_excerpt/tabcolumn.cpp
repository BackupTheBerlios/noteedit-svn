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

#include "tabcolumn.h"
#include "config.h"
#if GCC_MAJ_VERS > 2
#include <iostream>
#else
#include <iostream.h>
#endif

using namespace std;

typedef unsigned int uint;

// Gets full real duration of a column, including all effects caused
// by dots, triplets, etc

Q_UINT16 TabColumn::fullDuration()
{
	Q_UINT16 len = l;
	if (flags & FLAG_DOT)  len += len / 2;
	if (flags & FLAG_TRIPLET)  len = len * 2 / 3;
	return len;
}

// Sets the dots, triplets, base duration, etc, based on full
// duration, deriving right combination via trying several ones

void TabColumn::setFullDuration(Q_UINT16 len)
{
	int test = 480;

	flags &= (uint(-1) - FLAG_DOT - FLAG_TRIPLET);

	for (int i = 0; i < 6; i++) {
		if (test == len) {                  // Try normal duration first
			l = len;
			return;
		}
		if (test * 3 / 2 == len) {          // Try dotted duration (x1.5)
			l = len * 2 / 3;
			flags |= FLAG_DOT;
			return;
		}
		if (test * 2 / 3 == len) {          // Try triplet duration (x2/3)
			l = len * 3 / 2;
			flags |= FLAG_TRIPLET;
			return;
		}

		test /= 2;
	}

	//kdDebug() << "Very strange full duration: " << len << ", can't detect, using 120" << endl;
	cerr << "Very strange full duration: " << len << ", can't detect, using 120" << endl;
	l = 120;
}

// Gives all flags except ones that affect duration

uint TabColumn::effectFlags()
{
	return flags & (uint(-1) - FLAG_DOT - FLAG_TRIPLET);
}
