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
/*--------------------------------------------------------------------------------------*/
/*											*/
/*		Joerg Anders, TU Chemnitz, Fakultaet fuer Informatik, GERMANY		*/
/*		ja@informatik.tu-chemnitz.de						*/
/*											*/
/*											*/
/****************************************************************************************/

#include <stdio.h>
#include <qpixmap.h>
#include <kmessagebox.h>
#include <klocale.h>
#include "keysig.h"
#include "clef.h"
#include "resource.h"
#include "transpainter.h"

const unsigned int NClef::line2midiTreble_[MAXLINE-MINLINE+1] = {
 43 /* G */, 45 /* A */, 47 /* B */, 48 /* C */, 50 /* D */, 52 /* E */, 53 /* F */,
 55 /* G */, 57 /* A */, 59 /* B */, 60 /* C */, 62 /* D */, 64 /* E */, 65 /* F */,
 67 /* G */, 69 /* A */, 71 /* B */, 72 /* C */, 74 /* D */, 76 /* E */, 77 /* F */,
 79 /* G */, 81 /* A */, 83 /* B */, 84 /* C */, 86 /* D */, 88 /* E */, 89 /* F */,
 91 /* G */, 93 /* A */, 95 /* B */, 96 /* C */, 98 /* D */ };

const unsigned int NClef::line2midiBass_[MAXLINE-MINLINE+1] = {
 23 /* B */, 24 /* C */, 26 /* D */, 28 /* E */, 29 /* F */, 31 /* G */, 33 /* A */,
 35 /* B */, 36 /* C */, 38 /* D */, 40 /* E */, 41 /* F */, 43 /* G */, 45 /* A */,
 47 /* B */, 48 /* C */, 50 /* D */, 52 /* E */, 53 /* F */, 55 /* G */, 57 /* A */,
 59 /* B */, 60 /* C */, 62 /* D */, 64 /* E */, 65 /* F */, 67 /* G */, 69 /* A */,
 71 /* B */, 72 /* C */, 74 /* D */, 76 /* E */, 78 /* F */};

const unsigned int NClef::line2midiSoprano_[MAXLINE-MINLINE+1] = {
 40 /* E */, 41 /* F */, 43 /* G */, 45 /* A */, 47 /* B */, 48 /* C */, 50 /* D */,
 52 /* E */, 53 /* F */, 55 /* G */, 57 /* A */, 59 /* B */, 60 /* C */, 62 /* D */,
 64 /* E */, 65 /* F */, 67 /* G */, 69 /* A */, 71 /* B */, 72 /* C */, 74 /* D */,
 76 /* E */, 77 /* F */, 79 /* G */, 81 /* A */, 83 /* B */, 84 /* C */, 86 /* D */,
 88 /* E */, 89 /* F */, 91 /* G */, 93 /* A */, 95 /* B */};

const unsigned int NClef::line2midiAlto_[MAXLINE-MINLINE+1] = {
 33 /* A */, 35 /* B */, 36 /* C */, 38 /* D */, 40 /* E */, 41 /* F */, 43 /* G */,
 45 /* A */, 47 /* B */, 48 /* C */, 50 /* D */, 52 /* E */, 53 /* F */, 55 /* G */,
 57 /* A */, 59 /* B */, 60 /* C */, 62 /* D */, 64 /* E */, 65 /* F */, 67 /* G */,
 69 /* A */, 71 /* B */, 72 /* C */, 74 /* D */, 76 /* E */, 77 /* F */, 79 /* G */,
 81 /* A */, 83 /* B */, 84 /* C */, 86 /* D */, 88 /* E */};

const unsigned int NClef::line2midiTenor_[MAXLINE-MINLINE+1] = {
 29 /* F */, 31 /* G */, 33 /* A */, 35 /* B */, 36 /* C */, 38 /* D */, 40 /* E */,
 41 /* F */, 43 /* G */, 45 /* A */, 47 /* B */, 48 /* C */, 50 /* D */, 52 /* E */,
 53 /* F */, 55 /* G */, 57 /* A */, 59 /* B */, 60 /* C */, 62 /* D */, 64 /* E */,
 65 /* F */, 67 /* G */, 69 /* A */, 71 /* B */, 72 /* C */, 74 /* D */, 76 /* E */,
 77 /* F */, 79 /* G */, 81 /* A */, 83 /* B */, 84 /* C */};

const char NClef::line2musixtexTreble_[MAXLINE-MINLINE+1] = {
 'F' /* G */, 'G' /* A */, 'H' /* B */, 'I' /* C */, 'K' /* D */, 'L' /* E */, 'M' /* F */,
 'N' /* G */, 'O' /* A */, 'P' /* B */, 'c' /* C */, 'd' /* D */, 'e' /* E */, 'f' /* F */,
 'g' /* G */, 'h' /* A */, 'i' /* B */, 'j' /* C */, 'k' /* D */, 'l' /* E */, 'm' /* F */,
 'n' /* G */, 'o' /* A */, 'p' /* B */, 'q' /* C */, 'r' /* D */, 's' /* E */, 't' /* F */,
 'u' /* G */, 'v' /* A */, 'w' /* B */, 'x' /* C */, 'y'};

const char NClef::line2musixtexBass_[MAXLINE-MINLINE+1] = {
 'A' /* B */, 'A' /* C */, 'A' /* D */, 'A' /* E */, 'A' /* F */, 'A' /* G */, 'A' /* A */,
 'B' /* B */, 'C' /* C */, 'D' /* D */, 'E' /* E */, 'F' /* F */, 'G' /* G */, 'H' /* A */,
 'I' /* B */, 'J' /* C */, 'K' /* D */, 'L' /* E */, 'M' /* F */, 'N' /* G */, 'O' /* A */,
 'P' /* B */, 'c' /* C */, 'd' /* D */, 'e' /* E */, 'f' /* F */, 'g' /* G */, 'h' /* A */,
 'i' /* B */, 'j' /* C */, 'k' /* D */, 'l' /* E */, 'm' /* F */};

const char NClef::line2musixtexSoprano_[MAXLINE-MINLINE+1] = { /* FIXME: Just copy of Alto clef! */
 'A' /* A */, 'B' /* B */, 'C' /* C */, 'D' /* D */, 'E' /* E */, 'F' /* F */, 'F' /* G */,
 'G' /* A */, 'H' /* B */, 'I' /* C */, 'K' /* D */, 'L' /* E */, 'M' /* F */, 'N' /* G */,
 'O' /* A */, 'P' /* B */, 'c' /* C */, 'd' /* D */, 'e' /* E */, 'f' /* F */, 'g' /* G */, 
 'h' /* A */, 'i' /* B */, 'j' /* C */, 'k' /* D */, 'l' /* E */, 'm' /* F */, 'n' /* G */,
 'o' /* A */, 'p' /* B */, 'q' /* C */, 'r' /* D */, 's' /* E */};

const char NClef::line2musixtexAlto_[MAXLINE-MINLINE+1] = {
 'A' /* A */, 'B' /* B */, 'C' /* C */, 'D' /* D */, 'E' /* E */, 'F' /* F */, 'F' /* G */,
 'G' /* A */, 'H' /* B */, 'I' /* C */, 'K' /* D */, 'L' /* E */, 'M' /* F */, 'N' /* G */,
 'O' /* A */, 'P' /* B */, 'c' /* C */, 'd' /* D */, 'e' /* E */, 'f' /* F */, 'g' /* G */, 
 'h' /* A */, 'i' /* B */, 'j' /* C */, 'k' /* D */, 'l' /* E */, 'm' /* F */, 'n' /* G */,
 'o' /* A */, 'p' /* B */, 'q' /* C */, 'r' /* D */, 's' /* E */};

const char NClef::line2musixtexTenor_[MAXLINE-MINLINE+1] = {
 'A' /* F */, 'A' /* G */, 'A' /* A */, 'B' /* B */, 'C' /* C */, 'D' /* D */, 'E' /* E */,
 'F' /* F */, 'F' /* G */, 'G' /* A */, 'H' /* B */, 'I' /* C */, 'K' /* D */, 'L' /* E */,
 'M' /* F */, 'N' /* G */, 'O' /* A */, 'P' /* B */, 'c' /* C */, 'd' /* D */, 'e' /* E */,
 'f' /* F */, 'g' /* G */, 'h' /* A */, 'i' /* B */, 'j' /* C */, 'k' /* D */, 'l' /* E */,
 'm' /* F */, 'n' /* G */, 'o' /* A */, 'p' /* B */, 'q' /* C */};

/* accidentals positions of clefs: 0-the bottom line, 1-first space, 2-second line, -1-below the bottom line */
const char NClef::trebleSharpPos_[7] = {8, 5, 9, 6, 3, 7, 4};
const char NClef::trebleFlatPos_[7]  = {4, 7, 3, 6, 2, 5, 1}; 
const char NClef::bassSharpPos_[7] = {6, 3, 7, 4, 1, 5, 2};
const char NClef::bassFlatPos_[7]  = {2, 5, 1, 4, 0, 3, -1};
const char NClef::soprSharpPos_[7] = {3, 0, 4, 1, 5, 2, 6};
const char NClef::soprFlatPos_[7]  = {6, 2, 5, 1, 4, 0, 3};
const char NClef::altoSharpPos_[7] = {7, 4, 8, 5, 2, 6, 3};
const char NClef::altoFlatPos_[7]  = {3, 6, 2, 5, 1, 4, 0};
const char NClef::tenorSharpPos_[7] = {9, 6, 3, 7, 4, 8, 5};
const char NClef::tenorFlatPos_[7]  = {5, 8, 4, 7, 3, 6, 2}; 

NClef::NClef(main_props_str *main_props, staff_props_str *staff_props, int kind, int shift) :
		 NMusElement(main_props, staff_props) {
	actual_ = false;
	xpos_   = 0;
	switch (shift) {
		case -8: shift_ = -12; break;
		case  8: shift_ =  12; break;
		default: shift_ =   0; break;
	}
	switch (clefKind_ = kind) {
		case BASS_CLEF: line2midiTab_ = line2midiBass_; 
			 	line2TexTab_ = line2musixtexBass_;
			 	sharpPosTab_ = bassSharpPos_;
			 	flatPosTab_ = bassFlatPos_;
			 	break;
		case SOPRANO_CLEF: line2midiTab_ = line2midiSoprano_; 
			 	line2TexTab_ = line2musixtexSoprano_;
			 	sharpPosTab_ = soprSharpPos_; 
			 	flatPosTab_ = soprFlatPos_;
			 	break;
		case ALTO_CLEF: line2midiTab_ = line2midiAlto_; 
			 	line2TexTab_ = line2musixtexAlto_;
			 	sharpPosTab_ = altoSharpPos_; 
			 	flatPosTab_ = altoFlatPos_;
			 	break;
		case TENOR_CLEF: line2midiTab_ = line2midiTenor_; 
			 	line2TexTab_ = line2musixtexTenor_;
			 	sharpPosTab_ = tenorSharpPos_;
			 	flatPosTab_ = tenorFlatPos_;
			 	break;
		case DRUM_BASS_CLEF: line2midiTab_ = line2midiBass_; 
			 	line2TexTab_ = line2musixtexTreble_; /* !!! DRUM_BASS_CLEF is no real clef */
			 	sharpPosTab_ = bassSharpPos_;
			 	flatPosTab_ = bassFlatPos_;
			 	break;
		default: line2midiTab_ = line2midiTreble_; /* treble and drum clef */
			 line2TexTab_ = line2musixtexTreble_;
			 sharpPosTab_ = trebleSharpPos_;
			 flatPosTab_ = trebleFlatPos_;
			 break;
	}
	calculateDimensionsAndPixmaps();
}


NClef *NClef::clone() {
	NClef *cclef;

	cclef = new NClef(main_props_, staff_props_, clefKind_);
	cclef->actual_ = false;
	cclef->shift_ = shift_;
	return cclef;
}

int NClef::lineOfC4() {
	int line;
	switch (clefKind_) {
		case BASS_CLEF:
		case DRUM_BASS_CLEF: line = 10;
			 	break;
		case SOPRANO_CLEF: line = 0;
			 	break;
		case ALTO_CLEF: line = 4;
			 	break;
		case TENOR_CLEF: line = 6;
			 	break;
		default: line = -2; /* treble and drum clef */
			 break;
	}
	switch (shift_) {
		case 12: line -= 7; break;
		case -12: line += 7; break;
	}
	return line;
}

void NClef::change(NClef *clef) {
	NMusElement::change(clef);
	shift_ = clef->shift_;
	clefKind_ = clef->clefKind_;
	line2midiTab_ = clef->line2midiTab_;
 	line2TexTab_ =  clef->line2TexTab_;
 	sharpPosTab_ =  clef->sharpPosTab_;
 	flatPosTab_ =   clef->flatPosTab_;
	calculateDimensionsAndPixmaps();
}

void NClef::changeKind(int kind) {
	switch (clefKind_ = kind) {
		case BASS_CLEF: line2midiTab_ = line2midiBass_; 
			 	line2TexTab_ = line2musixtexBass_;
			 	sharpPosTab_ = bassSharpPos_;
			 	flatPosTab_ = bassFlatPos_;
			 	break;
		case SOPRANO_CLEF: line2midiTab_ = line2midiSoprano_; 
			 	line2TexTab_ = line2musixtexSoprano_;
			 	sharpPosTab_ = soprSharpPos_; 
			 	flatPosTab_ = soprFlatPos_;
			 	break;
		case ALTO_CLEF: line2midiTab_ = line2midiAlto_; 
			 	line2TexTab_ = line2musixtexAlto_;
			 	sharpPosTab_ = altoSharpPos_; 
			 	flatPosTab_ = altoFlatPos_;
			 	break;
		case TENOR_CLEF: line2midiTab_ = line2midiTenor_; 
			 	line2TexTab_ = line2musixtexTenor_;
			 	sharpPosTab_ = tenorSharpPos_;
			 	flatPosTab_ = tenorFlatPos_;
			 	break;
		case DRUM_BASS_CLEF: line2midiTab_ = line2midiBass_; 
			 	line2TexTab_ = line2musixtexTreble_; /* !!! DRUM_BASS_CLEF is no real clef */
			 	sharpPosTab_ = bassSharpPos_;
			 	flatPosTab_ = bassFlatPos_;
			 	break;
		default: line2midiTab_ = line2midiTreble_; /* treble and drum clef */
			 line2TexTab_ = line2musixtexTreble_;
			 sharpPosTab_ = trebleSharpPos_;
			 flatPosTab_ = trebleFlatPos_;
			 break;
	}
	calculateDimensionsAndPixmaps();
}

void NClef::changeShift(int shift) {
	switch (shift) {
		case -8: shift_ = -12; break;
		case  8: shift_ =  12; break;
		default: shift_ =   0; break;
	}
}

void NClef::setShift(int octave) {
	switch (clefKind_) {
		case TREBLE_CLEF:
		case SOPRANO_CLEF:
		case ALTO_CLEF:
		case TENOR_CLEF:
			switch (octave) {
				case 3: shift_  = -12; break;
				case 5: shift_  =  12; break;
				default: shift_ = 0; break;
			}
			break;
		case BASS_CLEF:
			switch (octave) {
				case 2: shift_  = -12; break;
				case 4: shift_  =  12; break;
				default: shift_ =  0; break;
			}
			break;
		case DRUM_BASS_CLEF:
		case DRUM_CLEF:
			shift_ =  0;
			break;
	}
	calculateDimensionsAndPixmaps();
}

int NClef::getOctave() {
	switch (clefKind_) {
		case TREBLE_CLEF:
		case SOPRANO_CLEF:
		case ALTO_CLEF:
		case TENOR_CLEF:
		case DRUM_CLEF:
			switch (shift_) {
				case -12: return 3;
				case  12: return 5;
				default: return 4;
			}
			break;
		case DRUM_BASS_CLEF:
		case BASS_CLEF:
			switch (shift_) {
				case -12: return 2; 
				case  12:  return 4;
				default:  return 3;
			}
			break;
	}
	return 4;
}
		
	
/* Returns the note pitch on the given note's vertical position. Arguments: int line - note position (0-bottom line, 1-first space, 2-second line, -1-space below the bottom line) ... */
char NClef::line2Name(int line, int *octave, bool lilyexport, bool abcexport) const {
	*octave = 0;
	char c;
	int kind = clefKind_;
	if (lilyexport && (kind == DRUM_CLEF || kind == DRUM_BASS_CLEF)) {
		line += 1;
	}
	else if (!abcexport) {
		switch (kind) {
			case SOPRANO_CLEF: line -= 2; break;
			case ALTO_CLEF: line -= 6; break;
			case TENOR_CLEF: line -= 1; break;
			case DRUM_BASS_CLEF:
			case BASS_CLEF:  line -= 5; break;
			case DRUM_CLEF: break;
			default: break;
		}
	}
	if (!lilyexport && abcexport) {
		switch (kind) {
			case SOPRANO_CLEF: line -= 2; break;
			case ALTO_CLEF: line -= 6; break;
			case TENOR_CLEF: line -= 1; break;
			case DRUM_BASS_CLEF: 
			case BASS_CLEF:  line -= 5; break;
			case DRUM_CLEF: break;
			default: break;
		}
	}

	while (line > 4) {
		line -= 7; (*octave)++;
	}
	while (line < -2) {
		line += 7; (*octave)--;
	}
	switch (line) {
		case -2: c = 'c'; break;
		case -1: c = 'd'; break;
		case  0: c = 'e'; break;
		case  1: c = 'f'; break;
		case  2: c = 'g'; break;
		case  3: c = 'a'; break;
		case  4: c = 'b'; break;
	}
	return c;
}

/* Returns the note pitch on the given note's vertical position in PMX format. */
char NClef::line2PMXName(int line, int *octave) const {
	*octave = 0;
	char c = 'c';

	switch (clefKind_) {
		case DRUM_CLEF:
		case TREBLE_CLEF:
			*octave = 4;
			while (line > 4) {
				line -= 7; (*octave)++;
			}
			while (line < -2) {
				line += 7; (*octave)--;
			}
			switch (line) {
				case -2: c = 'c'; break;
				case -1: c = 'd'; break;
				case  0: c = 'e'; break;
				case  1: c = 'f'; break;
				case  2: c = 'g'; break;
				case  3: c = 'a'; break;
				case  4: c = 'b'; break;
			}
			break;
		case DRUM_BASS_CLEF:
		case BASS_CLEF:  *octave = 3;
			while (line > 9) {
				line -= 7; (*octave)++;
			}
			while (line < 3) {
				line += 7; (*octave)--;
			}
			switch (line) {
				case  3: c = 'c'; break;
				case  4: c = 'd'; break;
				case  5: c = 'e'; break;
				case  6: c = 'f'; break;
				case  7: c = 'g'; break;
				case  8: c = 'a'; break;
				case  9: c = 'b'; break;
			}
		case SOPRANO_CLEF:
			*octave = 4;
			while (line > 6) {
				line -= 7; (*octave)++;
			}
			while (line < 0) {
				line += 7; (*octave)--;
			}
			switch (line) {
				case  0: c = 'c'; break;
				case  1: c = 'd'; break;
				case  2: c = 'e'; break;
				case  3: c = 'f'; break;
				case  4: c = 'g'; break;
				case  5: c = 'a'; break;
				case  6: c = 'b'; break;
			}
			break;
		case ALTO_CLEF:
			*octave = 4;
			while (line > 10) {
				line -= 7; (*octave)++;
			}
			while (line < 4) {
				line += 7; (*octave)--;
			}
			switch (line) {
				case  4: c = 'c'; break;
				case  5: c = 'd'; break;
				case  6: c = 'e'; break;
				case  7: c = 'f'; break;
				case  8: c = 'g'; break;
				case  9: c = 'a'; break;
				case 10: c = 'b'; break;
			}
			break;
		case TENOR_CLEF:
			*octave = 4;
			while (line > 12) {
				line -= 7; (*octave)++;
			}
			while (line < 6) {
				line += 7; (*octave)--;
			}
			switch (line) {
				case  6: c = 'c'; break;
				case  7: c = 'd'; break;
				case  8: c = 'e'; break;
				case  9: c = 'f'; break;
				case 10: c = 'g'; break;
				case 11: c = 'a'; break;
				case 12: c = 'b'; break;
			}
			break;
		default: break;
	}
	return c;
}

int NClef::line2note(int line) const {
	int c = 0;
	switch (clefKind_) {
		case DRUM_BASS_CLEF:
		case BASS_CLEF:  line -= 5; break;
		case SOPRANO_CLEF: line -= 2; break;
		case ALTO_CLEF: line -= 6; break;
		case TENOR_CLEF: line -= 8; break;
		default: break;
	}

	while (line > 4) {
		line -= 7; 
	}
	while (line < -2) {
		line += 7;
	}
	switch (line) {
		case -2: c = 0; break;
		case -1: c = 1; break;
		case  0: c = 2; break;
		case  1: c = 3; break;
		case  2: c = 4; break;
		case  3: c = 5; break;
		case  4: c = 6; break;
	}
	return c;
}

int NClef::name2Line(char name) const {
	int line;
	switch (name) {
		case 'c': line = -2; break;
		case 'd': line = -1; break;
		case 'e': line =  0; break;
		case 'f': line =  1; break;
		case 'g': line =  2; break;
		case 'a': line =  3; break;
		case 'b': line =  4; break;
		default:
			KMessageBox::error
			  (0,
			   i18n("name2Line: internal error"),
			   kapp->makeStdCaption(i18n("name2Line")));
			return NULL_LINE;
	}
	switch (clefKind_) {
		case DRUM_BASS_CLEF:
		case BASS_CLEF: line += 5; break;
		case SOPRANO_CLEF: line += 2; break;
		case ALTO_CLEF: line += 6; break;
		case TENOR_CLEF: line += 1; break;
		default: break;
	}
	return line;
}

int NClef::getAccPos(int kind, int nr) {
	switch (kind) {
		case STAT_CROSS: return sharpPosTab_[nr];
		case STAT_FLAT: return flatPosTab_[nr];
		default: NResource::abort("internal error in NClef::getAccPos");
	}
	return 0;
}

int NClef::noteNumber2Line(int note_number) const {
	switch (clefKind_) {
		case DRUM_BASS_CLEF:
		case BASS_CLEF:
			return note_number + 3;
		case DRUM_CLEF:
		case TREBLE_CLEF:
			if ((note_number += 5) > 9) return note_number - 7;
			return note_number;
		case SOPRANO_CLEF:
			if ((note_number += 7) > 9) return note_number - 7;
			return note_number;
		case ALTO_CLEF:
			if ((note_number += 4) > 9) return note_number - 7;
			return note_number;
		case TENOR_CLEF:
			if ((note_number += 6) > 9) return note_number - 7;
			return note_number;
	}
	return note_number;
}

void NClef::midi2Line(unsigned int midival, int *line, int *offs, NKeySig *ksig) {
	int i, kind, count;
	*line = 0;
	*offs = 0;

	midival -= shift_;
	for (i = 0; i < MAXLINE-MINLINE+1; i++) {
		if (line2midiTab_[i] >= midival) {
			if (line2midiTab_[i] == midival) {
				*line = i + MINLINE;
			}
			else {
				*line = i + MINLINE - 1;
				*offs = 1;
				if (!ksig) return;
				if (!ksig->isRegular(&kind, &count)) return;
				if (kind != STAT_FLAT) return;
				(*line)++;
				*offs = -1;
			}
			return;
		}
	}
}

int NClef::chooseClefType(staffInfoClass *staffInfos, int minMidi, int maxMidi, bool drumchannel) {
	int treble_clef, bass_clef;

	treble_clef = drumchannel ? DRUM_CLEF : TREBLE_CLEF;
	bass_clef = drumchannel ? DRUM_BASS_CLEF : BASS_CLEF;
	if (minMidi >= line2midiTreble_[0] && maxMidi <= line2midiBass_[MAXLINE-MINLINE]) {
		if (maxMidi < 60) {
			staffInfos[0].minMidi = minMidi;
			staffInfos[0].maxMidi = maxMidi;
			staffInfos[0].clefType = bass_clef;
			staffInfos[0].clefShift = 3;
			return 1;
		}
	}
	if (minMidi >= line2midiTreble_[0] && maxMidi <= line2midiTreble_[MAXLINE-MINLINE]) {
		staffInfos[0].minMidi = minMidi;
		staffInfos[0].maxMidi = maxMidi;
		staffInfos[0].clefType = treble_clef;
		staffInfos[0].clefShift = 4;
		return 1;
	}
	if (minMidi >= line2midiBass_[0] && maxMidi <= line2midiBass_[MAXLINE-MINLINE]) {
		staffInfos[0].minMidi = minMidi;
		staffInfos[0].maxMidi = maxMidi;
		staffInfos[0].clefType = bass_clef;
		staffInfos[0].clefShift = 3;
		return 1;
	}
	if (minMidi >= line2midiBass_[0] && maxMidi <= line2midiTreble_[MAXLINE-MINLINE]) { 
		staffInfos[0].minMidi = 60;
		staffInfos[0].maxMidi = maxMidi;
		staffInfos[0].clefType = treble_clef;
		staffInfos[0].clefShift = 4;
		staffInfos[1].minMidi = minMidi;
		staffInfos[1].maxMidi = 59;
		staffInfos[1].clefType = bass_clef;
		staffInfos[1].clefShift = 3;
		return 2;
	}
	if (minMidi < line2midiBass_[0]) {
		staffInfos[0].minMidi = line2midiBass_[0] - 12;
		staffInfos[0].maxMidi = 39;
		staffInfos[0].clefType = bass_clef;
		staffInfos[0].clefShift = 2;
		if (maxMidi < 39) return 1;

		staffInfos[1].minMidi = 40;
		staffInfos[1].maxMidi = line2midiBass_[MAXLINE-MINLINE];
		staffInfos[1].clefType = bass_clef;
		staffInfos[1].clefShift = 3;

		if (maxMidi <= line2midiBass_[MAXLINE-MINLINE]) return 2;

		staffInfos[1].maxMidi = 59;

		staffInfos[2].minMidi = 60;
		staffInfos[2].maxMidi = line2midiTreble_[MAXLINE-MINLINE];
		staffInfos[2].clefType = treble_clef;
		staffInfos[2].clefShift = 4;

		if (maxMidi <= line2midiTreble_[MAXLINE-MINLINE]) return 3;
		staffInfos[2].maxMidi = 81;
		
		staffInfos[3].minMidi = 82;
		staffInfos[3].maxMidi = line2midiTreble_[MAXLINE-MINLINE] + 12;
		staffInfos[3].clefType = treble_clef;
		staffInfos[3].clefShift = 5;
		
		if (maxMidi > line2midiTreble_[MAXLINE-MINLINE] + 12) return 0;
		return 4;
		
	}

	if (maxMidi > line2midiTreble_[MAXLINE-MINLINE]) {
		if (maxMidi > line2midiTreble_[MAXLINE-MINLINE] + 12) return 0;
		staffInfos[0].minMidi = 83;
		staffInfos[0].maxMidi = line2midiTreble_[MAXLINE-MINLINE] + 12;
		staffInfos[0].clefType = treble_clef;
		staffInfos[0].clefShift = 5;

		if (minMidi >= 83) return 1;

		staffInfos[1].minMidi = line2midiTreble_[0];
		staffInfos[1].maxMidi = 82;
		staffInfos[1].clefType = treble_clef;
		staffInfos[1].clefShift = 4;

		if (minMidi >= line2midiTreble_[0]) return 2;

		staffInfos[1].minMidi = 60;

		staffInfos[2].minMidi = line2midiBass_[0];
		staffInfos[2].maxMidi = 59;
		staffInfos[2].clefType = bass_clef;
		staffInfos[2].clefShift = 3;

		if (minMidi >= line2midiBass_[0]) return 3;
		
		staffInfos[2].minMidi = 40;

		staffInfos[3].minMidi = line2midiBass_[0] - 12;
		staffInfos[3].maxMidi = 39;
		staffInfos[3].clefType = bass_clef;
		staffInfos[3].clefShift = 2;
	
		if (minMidi >= line2midiBass_[0] - 12) return 4;

		return 0;
	}
		
	return 0;
}

void NClef::calculateDimensionsAndPixmaps() {
	int ypos, yoffs;
	if (!staff_props_->base) return;
	switch (clefKind_) {
		case TREBLE_CLEF: switch(shift_) {
					case  12: blackPixmap_ = NResource::treblepPixmap_;
						 redPixmap_   = NResource::treblepRedPixmap_;
						 yoffs = 0;
						 break;
					case -12: blackPixmap_ = NResource::treblemPixmap_;
						 redPixmap_   = NResource::treblemRedPixmap_;
						 yoffs = 18;
						 break;
					default: blackPixmap_ = NResource::treblePixmap_;
					         redPixmap_   = NResource::trebleRedPixmap_;
						 yoffs = 5;
						 break;
				}
				ypos = staff_props_->base - 5 * LINE_DIST/2;
				pixmapHeight_ = blackPixmap_->height();
				pixmapWidth_ = blackPixmap_->width();
				break;
		case BASS_CLEF: switch (shift_) {
					case  12: blackPixmap_ = NResource::basspPixmap_;
						 redPixmap_   = NResource::basspRedPixmap_;
						 yoffs = -8;
						 break;
					case -12: blackPixmap_ = NResource::bassmPixmap_;
						 redPixmap_   = NResource::bassmRedPixmap_;
						 yoffs = 16;
						 break;
					default: blackPixmap_ = NResource::bassPixmap_;
						 redPixmap_   = NResource::bassRedPixmap_;
						 yoffs = 2;
						 break;
				}
				ypos = staff_props_->base - 2* LINE_DIST / 2;
				pixmapHeight_ = blackPixmap_->height();
				pixmapWidth_ = blackPixmap_->width();
				break;
		case SOPRANO_CLEF: switch(shift_) {
					case 12: blackPixmap_ = NResource::altopPixmap_;
						 redPixmap_   = NResource::altopRedPixmap_;
						 yoffs = -13;
						 break;
					case -12: blackPixmap_ = NResource::altomPixmap_;
						 redPixmap_   = NResource::altomRedPixmap_;
						 yoffs = 17;
						 break;
					default: blackPixmap_ = NResource::altoPixmap_;
					         redPixmap_   = NResource::altoRedPixmap_;
						 yoffs = 4;
						 break;
				}
				ypos = staff_props_->base + LINE_DIST;
				pixmapHeight_ = blackPixmap_->height();
				pixmapWidth_ = blackPixmap_->width();
				break;
		case ALTO_CLEF: switch(shift_) {
					case 12: blackPixmap_ = NResource::altopPixmap_;
						 redPixmap_   = NResource::altopRedPixmap_;
						 yoffs = -22;
						 break;
					case -12: blackPixmap_ = NResource::altomPixmap_;
						 redPixmap_   = NResource::altomRedPixmap_;
						 yoffs = 8;
						 break;
					default: blackPixmap_ = NResource::altoPixmap_;
					         redPixmap_   = NResource::altoRedPixmap_;
						 yoffs = -4;
						 break;
				}
				ypos = staff_props_->base - LINE_DIST/2;
				pixmapHeight_ = blackPixmap_->height();
				pixmapWidth_ = blackPixmap_->width();
				break;
		case TENOR_CLEF: switch(shift_) {
					case 12: blackPixmap_ = NResource::altopPixmap_;
						 redPixmap_   = NResource::altopRedPixmap_;
						 yoffs = -24;
						 break;
					case -12: blackPixmap_ = NResource::altomPixmap_;
						 redPixmap_   = NResource::altomRedPixmap_;
						 yoffs = 6;
						 break;
					default: blackPixmap_ = NResource::altoPixmap_;
					         redPixmap_   = NResource::altoRedPixmap_;
						 yoffs = -7;
						 break;
				}
				ypos = staff_props_->base - 3 * LINE_DIST/2;
				pixmapHeight_ = blackPixmap_->height();
				pixmapWidth_ = blackPixmap_->width();
				break;
		case DRUM_BASS_CLEF:
				blackPixmap_ = NResource::drumBassClefPixmap_;
				redPixmap_   = NResource::drumBassClefRedPixmap_;
				yoffs = 0;
				ypos = staff_props_->base - 2* LINE_DIST / 2;
				pixmapHeight_ = blackPixmap_->height();
				pixmapWidth_ = blackPixmap_->width();
				break;
		case DRUM_CLEF:
				blackPixmap_ = NResource::drumClefPixmap_;
				redPixmap_   = NResource::drumClefRedPixmap_;
				yoffs = 0;
				ypos = staff_props_->base - 2* LINE_DIST / 2;
				pixmapHeight_ = blackPixmap_->height();
				pixmapWidth_ = blackPixmap_->width();
				break;
		default: NResource::abort("unknown clef");
				break;
	}
	nbaseDrawPoint_ = QPoint (xpos_, ypos+yoffs);
	bbox_ = QRect(xpos_, ypos , pixmapWidth_, pixmapHeight_);
}

void NClef::draw(int /* dummy */) {
	main_props_->tp->beginTranslated();
	main_props_->tp->drawPixmap (nbaseDrawPoint_, actual_ ?  *redPixmap_ :  *blackPixmap_);
	main_props_->tp->end();
}

void NClef::drawContextClef() {
	main_props_->tp->beginUnclippedYtranslated();
	main_props_->tp->drawPixmap(main_props_->context_clef_xpos, nbaseDrawPoint_.y(), *blackPixmap_);
	main_props_->tp->end();
}
