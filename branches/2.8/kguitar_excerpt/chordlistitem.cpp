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

#include "chordlistitem.h"
#include "global.h"
#include "noteedit/resource.h"
#include <qstring.h>

ChordListItem::ChordListItem(int _tonic, int _bass, int s3, int s5, int s7,
							 int s9, int s11, int s13):QListBoxText()
{
	// MEMORIZING STEPS INFO IN THE COMBOBOX SELECTION FORM

	t = _tonic;
	int toneshift[6] = { 3, 7, 10, 2, 5, 9 };
	s[0] = s3;
	s[1] = s5;
	s[2] = s7;
	s[3] = s9;
	s[4] = s11;
	s[5] = s13;

	for (int i = 0; i < 6; i++) {
		if (s[i] == -1)
			s[i] = 0;
		else
			s[i] = s[i] - toneshift[i] + 2;
	}

	// TEXT NAME CONSTRUCTION

	setText(buildName(_tonic, _bass, s3, s5, s7, s9, s11, s13,
		  NResource::globalNoteNames_,
		  NResource::globalMaj7_,
		  NResource::globalFlatPlus_));
}

// translate tonic and step values to chord name
QString buildName(int _tonic, int _bass, int s3, int s5, int s7,
		  int s9, int s11, int s13,
		  int noteNames, int maj7, int flatPlus)
{
	QString name = note_name_res(_tonic, noteNames);

	// Special cases
	if ((s3 == -1) && (s5 == 7) && (s7 == -1) &&
		(s9 == -1) && (s11 == -1) && (s13 == -1)) {
		name = name + "5";
		return name;
	}
	if ((s3 == 4) && (s5 == 8) && (s7 == -1) &&
		(s9 == -1) && (s11 == -1) && (s13 == -1)) {
		name = name + "aug";
		return name;
	}

	if ((s3 == 3) && (s5 == 6) && (s7 == 9)) {
		name = name + "dim";
	} else {
		if (s3 == 3)
			name = name + "m";

		if (s5 == 6)
			name = name + "/5" + flat_[flatPlus];
		if (s5 == 8)
			name = name + "/5" + sharp_[flatPlus];
		if (((s5 == 6) || (s5 == 8)) && ((s7 != -1) || (s9 != -1) ||
										 (s11 != -1) || (s13 != -1)))
			name = name + "/";

		if ((s7 == 10) && (s9 == -1))
			name = name + "7";
		if (s7 == 11)
			name = name + maj7name_[maj7];
		if (s7 == 9)
			name = name + "6";
		if (((s7 == 11) || (s7 == 9))
			&& ((s9 != -1) || (s11 != -1) || (s13 != -1)))
			name = name + "/";
	}

	if ((s7 == -1) && (s9 != -1))
		name = name + "add";
	if ((s9 == 2) && (s11 == -1))
		name = name + "9";
	if (s9 == 1)
		name = name + "9" + flat_[flatPlus];
	if (s9 == 3)
		name = name + "9" + sharp_[flatPlus];
	if (((s9 == 1) || (s9 == 3)) && ((s11 != -1) || (s13 != -1)))
		name = name + "/";

	if ((s9 == -1) && (s11 != -1))
		name = name + "add";
	if ((s11 == 5) && (s13 == -1))
		name = name + "11";
	if (s11 == 6)
		name = name + "11" + sharp_[flatPlus];
	if (s11 == 4)
		name = name + "11" + flat_[flatPlus];
	if (((s11 == 4) || (s11 == 6)) && (s13 != -1))
		name = name + "/";

	if ((s11 == -1) && (s13 != -1))
		name = name + "add";
	if (s13 == 9)
		name = name + "13";
	if (s13 == 10)
		name = name + "13" + sharp_[flatPlus];
	if (s13 == 8)
		name = name + "13" + flat_[flatPlus];

	if (s3 == 2)
		name = name + "sus2";
	if (s3 == 5)
		name = name + "sus4";

	if ((s3 == -1) && (s5 == -1)) {
		name = name + " (no3no5)";
	} else {
		if (s3 == -1)
			name = name + " (no3)";
		if (s5 == -1)
			name = name + " (no5)";
	}

	return name;
}
