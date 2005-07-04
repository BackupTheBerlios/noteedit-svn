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
/****************************************************************************************/
/*											*/
/*		Leon Vinken, The Netherlands						*/
/*		leon.vinken@hetnet.nl							*/
/*											*/
/****************************************************************************************/

// MusicXML functions common to both import and export

#include "musicxmlcommon.h"

#if GCC_MAJ_VERS > 2
#include <iostream>
#else
#include <iostream.h>
#endif
using namespace std;

// Known MusicXML chord kinds with corresponding step values
// Note: NMusicXMLExport::outputDiagram requires this table to be sorted
// on descending number of notes

MusicXMLChord MxmlChordTab[] = {
//
//         kind                 s3  s5  s7  s9 s11 s13
// 13ths
	{ "dominant-13th",       4,  7, 10, 14, 17, 21 },
	{ "major-13th",          4,  7, 11, 14, 17, 21 },
	{ "minor-13th",          3,  7, 10, 13, 17, 21 },
// 11ths
	{ "dominant-11th",       4,  7, 10, 14, 17, -1 },
	{ "major-11th",          4,  7, 11, 14, 17, -1 },
	{ "minor-11th",          3,  7, 10, 13, 17, -1 },
// Ninths
	{ "dominant-ninth",      4,  7, 10, 14, -1, -1 },
	{ "major-ninth",         4,  7, 11, 14, -1, -1 },
	{ "minor-ninth",         3,  7, 10, 13, -1, -1 },
// Sevenths
	{ "dominant",            4,  7, 10, -1, -1, -1 },
	{ "major-seventh",       4,  7, 11, -1, -1, -1 },
	{ "minor-seventh",       3,  7, 10, -1, -1, -1 },
	{ "diminished-seventh",  3,  6,  9, -1, -1, -1 },
	{ "half-diminished",     3,  6, 10, -1, -1, -1 },
	{ "major-minor",         3,  7, 11, -1, -1, -1 },
// Sixths
	{ "major-sixth",         4,  7,  9, -1, -1, -1 },
	{ "minor-sixth",         3,  7,  9, -1, -1, -1 },
// Triads
	{ "major",               4,  7, -1, -1, -1, -1 },
	{ "minor",               3,  7, -1, -1, -1, -1 },
	{ "augmented",           4,  8, -1, -1, -1, -1 },
	{ "diminished",          3,  6, -1, -1, -1, -1 },
// Suspended
	{ "suspended-second",    2,  7, -1, -1, -1, -1 },
	{ "suspended-fourth",    5,  7, -1, -1, -1, -1 },
// End of table (do not remove)
	{ 0, 0, 0, 0, 0, 0, 0 }
};
