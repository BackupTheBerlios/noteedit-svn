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
#include <qpainter.h>
#include <qbitmap.h>
#include "keysig.h"
#include "resource.h"
#include "transpainter.h"
#include "clef.h"

#define SIGN_DIST 19

#define Y_BORDER_UP 40
#define Y_BORDER_DOWN 40
#define Y_BORDERS (Y_BORDER_UP+Y_BORDER_DOWN)
#define CROSS_DRAW_OFFS (Y_BORDER_UP - 30)
#define FLAT_DRAW_OFFS (Y_BORDER_UP - 32)
#define NATUR_DRAW_OFFS (Y_BORDER_UP - 30)
#define PIXMAP_HEIGHT (4*LINE_DIST+Y_BORDERS)

#define LINE2TABIDX(line) (line - MINLINE)

int NKeySig::crossTab_[7] = {3, 0, 4, 1, 5, 2, 6};

int NKeySig::flatTab_[7] = {6, 2, 5, 1, 4, 0, 3};

int NKeySig::nameTab_[7] = {'c', 'd', 'e', 'f', 'g', 'a', 'b'};

char NKeySig::str[128];

NClef NKeySig::defaultClef_(0, &NResource::nullprops_);


NKeySig::NKeySig(main_props_str *main_props, staff_props_str *staff_props) :
	NMusElement(main_props, staff_props) {

	noteState_ = new char[7];
	tempNoteState_ = new char[MAXLINE-MINLINE+1];

	memset(noteState_, STAT_NATUR, 7);
	memset(tempNoteState_, STAT_NO_ACC, MAXLINE-MINLINE+1);
	stateChanged_ = true;
	acClef_ = &defaultClef_;
	actual_ = false;
	pixmapWidth_ = 5;
	resolv_redPixmap_ = resolvPixmap_ = keyPixmap_ = key_redPixmap_ = 0;
	noSignes_ = true;
	resPixmapWidth_ = resolvOffs_ = 0;
	computedPreviousKeySig_ = previousKeySig_ = 0;
	if (staff_props_->base) calculateDimensionsAndPixmaps();
}

void NKeySig::change(NKeySig *ksig) {
	stateChanged_ = true;
	NMusElement::change(ksig);
	memcpy(noteState_, ksig->noteState_, 7);
	memcpy(tempNoteState_, ksig->noteState_, 7);
	acClef_ = ksig->acClef_;
	actual_ = false;
	pixmapWidth_ = 5;
	if (resolv_redPixmap_) delete resolv_redPixmap_;
	if (resolvPixmap_) delete resolvPixmap_;
	if (keyPixmap_) delete keyPixmap_;
	if (key_redPixmap_) delete key_redPixmap_;
	resolv_redPixmap_ = resolvPixmap_ = keyPixmap_ = key_redPixmap_ = 0;
	resPixmapWidth_ = resolvOffs_ = 0;
	computedPreviousKeySig_ = 0;
	if (staff_props_->base) calculateDimensionsAndPixmaps();
}

void NKeySig::setPreviousKeySig(NKeySig *prevKeySig) {
	previousKeySig_ = prevKeySig;
} 

bool NKeySig::isEqual(NKeySig *otherKeysig) {
	int i;
	if (!otherKeysig) return false;
	for (i = 0; i < 7; ++i) {
		if (noteState_[i] != otherKeysig->noteState_[i]) return false;
	}
	return true;
}

void NKeySig::setClef(NClef * ac_clef) {
	acClef_ = ac_clef;

	stateChanged_ = true;
	if (keyPixmap_) delete keyPixmap_;
	if (key_redPixmap_) delete key_redPixmap_;
	keyPixmap_ = 0;
        key_redPixmap_ = 0;
	if (staff_props_->base) calculateDimensionsAndPixmaps();
}

void NKeySig::changeHalfTone(NNote *note) {
	int notenr = acClef_->line2note(note->line);
	int kind, count;
	stateChanged_ = true;
	switch (note->offs) {
		case  1: if (isRegular(&kind, &count))  {
				if (kind == STAT_FLAT) {
					(note->line)++; note->offs = -1;
				}
			 } else if (noteState_[(notenr+1) % 7] == STAT_FLAT) {
				(note->line)++; note->offs = -1; 
			 }
			 break;
		case -1: if (isRegular(&kind, &count)) {
				if (kind == STAT_CROSS) {
					(note->line)--; note->offs = 1;
				}
			 }
			 else if (noteState_[(notenr+6) % 7] == STAT_CROSS) {
				(note->line)--; note->offs = 1;
			 }
			 break;
	}
}

NKeySig *NKeySig::clone() {
	char *tmp1, *tmp2;
	NKeySig *ckeysig;
	ckeysig = new NKeySig(main_props_, staff_props_);

	tmp1 = ckeysig->noteState_;
	tmp2 = ckeysig->tempNoteState_;

	*ckeysig = *this;

	ckeysig->noteState_ = tmp1;
	ckeysig->tempNoteState_ = tmp2;
	memset(ckeysig->tempNoteState_, STAT_NO_ACC, MAXLINE-MINLINE+1);
	memcpy(ckeysig->noteState_, noteState_, 7);
	ckeysig->resolv_redPixmap_ = ckeysig->resolvPixmap_ = ckeysig->keyPixmap_ = ckeysig->key_redPixmap_ = 0;
	ckeysig->previousKeySig_ = ckeysig->computedPreviousKeySig_ = 0;
	ckeysig->resolvOffs_ = 0;
	ckeysig->stateChanged_ = true;

	return ckeysig;
}


NKeySig::~NKeySig() {
	delete noteState_;
	delete tempNoteState_;
	if (keyPixmap_ != 0) delete keyPixmap_;
	if (key_redPixmap_ != 0) delete key_redPixmap_;
	if (resolv_redPixmap_ != 0) delete resolv_redPixmap_; 
	if (resolvPixmap_ != 0) delete resolvPixmap_;
}

int NKeySig::line2Range(int line) {
	int i;
	if (line < 3) {
		while (line < 3) line += 7;
	}
	else if (line > 9) {
		while (line > 9) line -= 7;
	}
	for (i = 0; i < 7; ++i) {
		if (acClef_->noteNumber2Line(i) == line) return i;
		
	}
	NResource::abort("line2Range: internal error");
	return 0;
}

#ifdef KEYSIG_DEBUG
void NKeySig::print() {
	int i;
	printf("stat: ");
	for (i = 0; i < 7; ++i) {
		switch (noteState_[i]) {
			case STAT_CROSS: printf("#");break;
			case STAT_FLAT: printf("&");break;
			case STAT_NATUR: printf("n");break;
			case STAT_NO_ACC: printf(".");break;
			default: printf("?"); break;
		}
	}
	printf("; tmp: ");
	for (i = 0; i < 7; ++i) {
		switch (tempNoteState_[i]) {
			case STAT_CROSS: printf("#");break;
			case STAT_FLAT: printf("&");break;
			case STAT_NATUR: printf("n");break;
			case STAT_NO_ACC: printf(".");break;
			default: printf("?"); break;
		}
	}
	printf("; clef:");
	switch (acClef_->getSubType()) {
		case TREBLE_CLEF: printf("TREBLE_CLEF"); break;
		case BASS_CLEF: printf("BASS_CLEF"); break;
		case DRUM_CLEF: printf("DRUM_CLEF"); break;
		case DRUM_BASS_CLEF: printf("DRUM_BASS_CLEF"); break;
		default: printf("unbekannt: %d\n", acClef_->getSubType()); break;
	}
	printf("\n");
}
#endif

int NKeySig::computeOffs(int line) {
	int pp, idx, state;

	idx = LINE2TABIDX(line);
	if ((state = tempNoteState_[idx]) == STAT_NO_ACC) {
		pp = line2Range(line);
		state = noteState_[pp];
	}
	switch (state) {
		case STAT_CROSS: return 1; break;
		case STAT_FLAT: return -1; break;
		case STAT_DCROSS: return 2; break;
		case STAT_DFLAT: return -2; break;
	}
	return 0;
}

int NKeySig::determineDistanceUp(NNote *note) {
	int midival1, midival2, offs2, line2;
	midival1 = acClef_->line2midiTab_[note->line+LINE_OVERFLOW]+note->offs;
	line2 = note->line+1;
	offs2 = computeOffs(line2);
	midival2 = acClef_->line2midiTab_[note->line+1+LINE_OVERFLOW]+offs2;
	return midival2 - midival1;
}

	

int NKeySig::accNeeded(int line, int offs) {
	int note, state, idx;

	idx = LINE2TABIDX(line);
	if ((state = tempNoteState_[idx]) == STAT_NO_ACC) {
		note = line2Range(line);
		state = noteState_[note]; 
	}
	if (offs == -1 && state == STAT_FLAT) return STAT_NO_ACC;
	if (offs ==  1 && state == STAT_CROSS) return STAT_NO_ACC;
	if (offs == -2 && state == STAT_DFLAT) return STAT_NO_ACC;
	if (offs ==  2 && state == STAT_DCROSS) return STAT_NO_ACC;
	if (offs ==  0 && (state == STAT_FLAT || state == STAT_CROSS || state == STAT_DCROSS || state == STAT_DFLAT)) return STAT_NATUR;
	if (offs ==  1) return STAT_CROSS;
	if (offs == -1) return STAT_FLAT;
	if (offs ==  2) return STAT_DCROSS;
	if (offs == -2) return STAT_DFLAT;
	return STAT_NO_ACC;
}

void NKeySig::resetAtBar() {
	memset(tempNoteState_, STAT_NO_ACC, MAXLINE-MINLINE+1);
}
	
void NKeySig::reset() {
	stateChanged_ = true;
	memset(noteState_, STAT_NATUR, 7); 
	memset(tempNoteState_, STAT_NO_ACC, MAXLINE-MINLINE+1); 
	if (keyPixmap_) delete keyPixmap_;
	if (key_redPixmap_) delete key_redPixmap_;
	keyPixmap_ = 0;
        key_redPixmap_ = 0;
	acClef_ = &defaultClef_;
}

int NKeySig::getState(int note) {
	if (note < 0 || note > 6) {
		NResource::abort("getState: internal error");
	}
	return noteState_[note];
}

	
void NKeySig::setTempAcc(int line, int kind) {
	tempNoteState_[LINE2TABIDX(line)] = kind;
	
}

void NKeySig::setKey(int note, int kind) {
	if (note < 0 || note > 7) {
		NResource::abort("setKey: internal error");
	}
	noteState_[note] = kind;
	if (keyPixmap_) delete keyPixmap_;
	if (key_redPixmap_) delete key_redPixmap_;
	keyPixmap_ = 0;
        key_redPixmap_ = 0;
}

void NKeySig::setRegular(int count, int kind) {
	int i, *tab;
	if (count > 7) return;
	stateChanged_ = true;
	reset();
	switch (kind) {
		case STAT_CROSS: tab = crossTab_;
				  break;
		case STAT_FLAT:  tab = flatTab_;
				  break;
		default: NResource::abort("setRegular: unknown kind");
	}
	for (i = 0; i < count; ++i) {
		noteState_[tab[i]] = kind;
	}
	if (staff_props_->base) calculateDimensionsAndPixmaps();
}

int NKeySig::accCount() {
	int i, count = 0;

	for (i = 0; i < 7; ++i) {
		if (noteState_[i] != STAT_NATUR) count++;
	}
	return count;
}

bool NKeySig::isRegular(int *kind, int *count) {
	bool ok[7];
	int i;
	*kind = STAT_NO_ACC;
	*count = 0;

	if (accCount() == 0)  return true;

	for (i = 0; i < 7; ok[i++] = false);

	for (i = 0; i < 7; ++i) {
		if (noteState_[i] != STAT_NATUR) {
			(*count)++;
			ok[i] = true;
			if (*kind == STAT_NO_ACC) {
				*kind = noteState_[i];
			}
			else if (*kind != noteState_[i]) return false;
		}
	}
	switch (*kind) {
		case STAT_CROSS:
			for (i = 0; i < *count; ++i) {
				if (!ok[crossTab_[i]]) return false;
			}
			return true;
		case STAT_FLAT:
			for (i = 0; i < *count; ++i) {
				if (!ok[flatTab_[i]]) return false;
			}
			return true;
		case STAT_NO_ACC:
		case STAT_NATUR: return false;
		default: NResource::abort("isRegular: internal error");
	}
	return false;
}

char *NKeySig::printKeys() {
	char *cptr;
	int i;
	cptr = NKeySig::str;
	for (i = 0; i < 7; ++i) {
		switch (noteState_[i]) {
			case STAT_CROSS: *cptr++ = nameTab_[i];
				      *cptr++ = '#';
				      *cptr++ = ' ';
				      break;
			case STAT_FLAT:  *cptr++ = nameTab_[i];
				      *cptr++ = '&';
				      *cptr++ = ' ';
				      break;
		}
	}
	*cptr = '\0';
	return NKeySig::str;
}

void NKeySig::addSign(int kind, char pitch) {
	int i;
	bool found = false;
	stateChanged_ = true;
	i = 0;
	while (!found && i < 7) {
		if (!(found = pitch == nameTab_[i])) ++i;
	}
	if (!found) return;
	noteState_[i] = kind;
}

void NKeySig::calculateDimensionsAndPixmaps() {
	int kind, count;
	int i, j, *tab;
	int draw_offs;
	bool noSignesInPrevKey = true;
	QPainter painter;
	QPixmap *pix;
	QBitmap keyBitmap;
	if (!staff_props_->base) return;

	

	nbaseDrawPoint_ = QPoint (xpos_+resolvOffs_, staff_props_->base-Y_BORDER_UP);
	resolvDrawPoint_ = QPoint (xpos_, staff_props_->base-Y_BORDER_UP);
	bbox_ = QRect(xpos_, staff_props_->base , pixmapWidth_+resPixmapWidth_, PIXMAP_HEIGHT);


	if (previousKeySig_ != computedPreviousKeySig_) {
		computedPreviousKeySig_ = previousKeySig_;
		if (previousKeySig_ && !isEqual(previousKeySig_) && previousKeySig_->accCount() != 0) {
			stateChanged_ = true;
			resPixmapWidth_ = NResource::naturPixmap_->width();
			resPixmapWidth_ += SIGN_DIST * previousKeySig_->accCount();
			resolvOffs_ = resPixmapWidth_;

			if (resolvPixmap_) delete resolvPixmap_;
			if (resolv_redPixmap_) delete resolv_redPixmap_;
			resolvPixmap_ = new QPixmap(resPixmapWidth_, PIXMAP_HEIGHT);
			resolv_redPixmap_  = new QPixmap(resPixmapWidth_, PIXMAP_HEIGHT);

			if (previousKeySig_->isRegular(&kind, &count)) {
				if (count) noSignesInPrevKey = false;
				draw_offs = NATUR_DRAW_OFFS;
				if (kind == STAT_CROSS) {
					tab = crossTab_;
				}
				else {
					tab = flatTab_;
				}
				painter.begin(resolvPixmap_);
				painter.setPen(NResource::noPen_);
				painter.setBrush(NResource::backgroundBrush_);
				painter.fillRect(0, 0, resPixmapWidth_, PIXMAP_HEIGHT, NResource::backgroundBrush_);
				for (i = 0; i < count; ++i) {
					painter.drawPixmap(i*SIGN_DIST,4*LINE_DIST+draw_offs-acClef_->getAccPos(kind, i)*LINE_DIST/2, *NResource::naturPixmap_);
				}
				painter.end();
		
				painter.begin(resolv_redPixmap_);
				painter.setPen(NResource::noPen_);
				painter.setBrush(NResource::backgroundBrush_);
				painter.fillRect(0, 0, resPixmapWidth_, PIXMAP_HEIGHT, NResource::backgroundBrush_);
				for (i = 0; i < count; ++i) {
					painter.drawPixmap(i*SIGN_DIST,4*LINE_DIST+draw_offs-acClef_->getAccPos(kind, i)*LINE_DIST/2, *NResource::naturPixmap_);
				}
			
				painter.end();
			}
			else {
				painter.begin(resolvPixmap_);
				painter.setPen(NResource::noPen_);
				painter.setBrush(NResource::backgroundBrush_);
				painter.fillRect(0, 0, resPixmapWidth_, PIXMAP_HEIGHT, NResource::backgroundBrush_);
				for (j = 0, i = 0; i < 7; ++i) {
					draw_offs = NATUR_DRAW_OFFS;
					if (noteState_[i] == STAT_NATUR) continue;
					if (noteState_[i] == STAT_CROSS) {
						tab = crossTab_;
						noSignesInPrevKey = false;
					}
					else {
						tab = flatTab_;
						noSignesInPrevKey = false;
					}
					painter.drawPixmap(j++*SIGN_DIST,4*LINE_DIST+draw_offs-acClef_->noteNumber2Line(i)*LINE_DIST/2, *NResource::naturPixmap_);
				}
				painter.end();
		
				painter.begin(resolv_redPixmap_);
				painter.setPen(NResource::noPen_);
				painter.setBrush(NResource::backgroundBrush_);
				painter.fillRect(0, 0, resPixmapWidth_, PIXMAP_HEIGHT, NResource::backgroundBrush_);
				for (j = 0, i = 0; i < 7; ++i) {
					if (noteState_[i] == STAT_NATUR) continue;
					painter.drawPixmap(j++*SIGN_DIST,4*LINE_DIST+draw_offs-acClef_->noteNumber2Line(i)*LINE_DIST/2, *NResource::naturPixmap_);
				}
				painter.setPen(NResource::blackWidePen_);
				painter.end();
			}
			keyBitmap = *resolvPixmap_;
			resolv_redPixmap_->setMask(keyBitmap);
			resolvPixmap_->setMask(keyBitmap);
			resolvDrawPoint_ = QPoint (xpos_, staff_props_->base-Y_BORDER_UP);
			bbox_ = QRect(xpos_, staff_props_->base , pixmapWidth_+resPixmapWidth_, PIXMAP_HEIGHT);
		}
		else {
			if (resolvPixmap_) delete resolvPixmap_;
			if (resolv_redPixmap_) delete resolv_redPixmap_;
			resolv_redPixmap_ = resolvPixmap_ = 0;
			resolvOffs_ = 0;
		}
	}

	if (stateChanged_) {
		stateChanged_ = false;
		if (accCount() != 0) {
			pixmapWidth_ = NResource::crossPixmap_->width();
			pixmapWidth_ += SIGN_DIST * accCount();
		
			if (keyPixmap_) delete keyPixmap_;
			if (key_redPixmap_) delete key_redPixmap_;
			keyPixmap_ = new QPixmap(pixmapWidth_, PIXMAP_HEIGHT);
			key_redPixmap_ = new QPixmap(pixmapWidth_, PIXMAP_HEIGHT);
		
			if (isRegular(&kind, &count)) {
				noSignes_ = count == 0;
				if (kind == STAT_CROSS) {
					pix = NResource::crossPixmap_;
					tab = crossTab_;
					draw_offs = CROSS_DRAW_OFFS;
				}
				else {
					pix = NResource::flatPixmap_;
					tab = flatTab_;
					draw_offs = FLAT_DRAW_OFFS;
				}
				painter.begin(keyPixmap_);
				painter.setPen(NResource::noPen_);
				painter.setBrush(NResource::backgroundBrush_);
				painter.fillRect(0, 0, pixmapWidth_, PIXMAP_HEIGHT, NResource::backgroundBrush_);
				for (i = 0; i < count; ++i) {
					painter.drawPixmap(i*SIGN_DIST,4*LINE_DIST+draw_offs-acClef_->getAccPos(kind, i)*LINE_DIST/2, *pix);
				}
				painter.end();
		
				pix = (kind == STAT_CROSS) ?  NResource::crossRedPixmap_ : NResource::flatRedPixmap_;
				painter.begin(key_redPixmap_);
				painter.setPen(NResource::noPen_);
				painter.setBrush(NResource::backgroundBrush_);
				painter.fillRect(0, 0, pixmapWidth_, PIXMAP_HEIGHT, NResource::backgroundBrush_);
				for (i = 0; i < count; ++i) {
					painter.drawPixmap(i*SIGN_DIST,4*LINE_DIST+draw_offs-acClef_->getAccPos(kind, i)*LINE_DIST/2, *pix);
				}
			
				painter.end();
			}
			else {
				painter.begin(keyPixmap_);
				painter.setPen(NResource::noPen_);
				painter.setBrush(NResource::backgroundBrush_);
				painter.fillRect(0, 0, pixmapWidth_, PIXMAP_HEIGHT, NResource::backgroundBrush_);
				noSignes_ = true;
				for (j=0, i = 0; i < 7; ++i) {
					if (noteState_[i] == STAT_NATUR) continue;
					if (noteState_[i] == STAT_CROSS) {
						pix = NResource::crossPixmap_;
						tab = crossTab_;
						draw_offs = CROSS_DRAW_OFFS;
						noSignes_ = false;
					}
					else {
						pix = NResource::flatPixmap_;
						tab = flatTab_;
						draw_offs = FLAT_DRAW_OFFS;
						noSignes_ = false;
					}
					painter.drawPixmap(j++*SIGN_DIST,4*LINE_DIST+draw_offs-acClef_->noteNumber2Line(i)*LINE_DIST/2, *pix);
				}
				painter.end();
		
				painter.begin(key_redPixmap_);
				painter.setPen(NResource::noPen_);
				painter.setBrush(NResource::backgroundBrush_);
				painter.fillRect(0, 0, pixmapWidth_, PIXMAP_HEIGHT, NResource::backgroundBrush_);
				for (j = 0, i = 0; i < 7; ++i) {
					if (noteState_[i] == STAT_NATUR) continue;
					pix = (noteState_[i] == STAT_CROSS) ? NResource::crossRedPixmap_ : NResource::flatRedPixmap_;
					painter.drawPixmap(j++*SIGN_DIST,4*LINE_DIST+draw_offs-acClef_->noteNumber2Line(i)*LINE_DIST/2, *pix);
				}
				painter.setPen(NResource::blackWidePen_);
				painter.end();
			}
			keyBitmap = *keyPixmap_;
			key_redPixmap_->setMask(keyBitmap);
			keyPixmap_->setMask(keyBitmap);
			nbaseDrawPoint_ = QPoint (xpos_+resolvOffs_, staff_props_->base-Y_BORDER_UP);
			bbox_ = QRect(xpos_, staff_props_->base , pixmapWidth_+resPixmapWidth_, PIXMAP_HEIGHT);
		}
		else {
			if (keyPixmap_) delete keyPixmap_;
			if (key_redPixmap_) delete key_redPixmap_;
			keyPixmap_ = 0;
			key_redPixmap_ = 0;
			pixmapWidth_ = 0;
		}
	}
	noSignes_ = noSignes_ && noSignesInPrevKey;
}

void NKeySig::draw(int /* dummy */) {
	if (noSignes_) return;
	main_props_->tp->beginTranslated();
	if (resolvPixmap_) {
		main_props_->tp->drawPixmap(resolvDrawPoint_, actual_ ?  *resolv_redPixmap_ : *resolvPixmap_);
	}
	if (keyPixmap_) {
		main_props_->tp->drawPixmap (nbaseDrawPoint_, actual_ ?  *key_redPixmap_ :  *keyPixmap_);
	}
	main_props_->tp->end();
}

/* ------------------------------- methods for context keysig ------------------------------------------*/

void NKeySig::drawContextKeySig() {
	if (noSignes_) return;
	main_props_->tp->beginUnclippedYtranslated();
	main_props_->tp->drawPixmap(main_props_->context_keysig_xpos, nbaseDrawPoint_.y(), *keyPixmap_);
	main_props_->tp->end();
}

void NKeySig::setClefInContextKeysig(NClef * ac_clef) {
	acClef_ = ac_clef;

	if (staff_props_->base) calculateContextPixmap();
}

void NKeySig::changeInContextKeySig(NKeySig *ksig) {
	NMusElement::change(ksig);
	acClef_ = ksig->acClef_;
	memcpy(noteState_, ksig->noteState_, 7);
	pixmapWidth_ = 5;
	if (staff_props_->base) calculateContextPixmap();
}

void NKeySig::calculateContextPixmap() { /* for faster computation of context key */
	int kind, count;
	int i, j, *tab;
	int draw_offs;
	QPainter painter;
	QPixmap *pix;
	QBitmap keyBitmap;
	

	nbaseDrawPoint_ = QPoint (xpos_+resolvOffs_, staff_props_->base-Y_BORDER_UP);
	bbox_ = QRect(xpos_, staff_props_->base , pixmapWidth_, PIXMAP_HEIGHT);

	if (accCount() == 0) {noSignes_ = true; return;}
	pixmapWidth_ = NResource::crossPixmap_->width();
	pixmapWidth_ += SIGN_DIST * accCount();
	
	keyPixmap_ = new QPixmap(pixmapWidth_, PIXMAP_HEIGHT);
	
	if (isRegular(&kind, &count)) {
		noSignes_ = count == 0;
		if (kind == STAT_CROSS) {
			pix = NResource::crossPixmap_;
			tab = crossTab_;
			draw_offs = CROSS_DRAW_OFFS;
		}
		else {
			pix = NResource::flatPixmap_;
			tab = flatTab_;
			draw_offs = FLAT_DRAW_OFFS;
		}
		painter.begin(keyPixmap_);
		painter.setPen(NResource::noPen_);
		painter.setBrush(NResource::backgroundBrush_);
		painter.fillRect(0, 0, pixmapWidth_, PIXMAP_HEIGHT, NResource::backgroundBrush_);
		for (i = 0; i < count; ++i) {
			painter.drawPixmap(i*SIGN_DIST,4*LINE_DIST+draw_offs-acClef_->getAccPos(kind, i)*LINE_DIST/2, *pix);
		}
		painter.end();
	}
	else {
		painter.begin(keyPixmap_);
		painter.setPen(NResource::noPen_);
		painter.setBrush(NResource::backgroundBrush_);
		painter.fillRect(0, 0, pixmapWidth_, PIXMAP_HEIGHT, NResource::backgroundBrush_);
		noSignes_ = true;
		for (j=0, i = 0; i < 7; ++i) {
			if (noteState_[i] == STAT_NATUR) continue;
			if (noteState_[i] == STAT_CROSS) {
				pix = NResource::crossPixmap_;
				tab = crossTab_;
				draw_offs = CROSS_DRAW_OFFS;
				noSignes_ = false;
			}
			else {
				pix = NResource::flatPixmap_;
				tab = flatTab_;
				draw_offs = FLAT_DRAW_OFFS;
				noSignes_ = false;
			}
			painter.drawPixmap(j++*SIGN_DIST,4*LINE_DIST+draw_offs-acClef_->noteNumber2Line(i)*LINE_DIST/2, *pix);
		}
		painter.end();
	}
	keyBitmap = *keyPixmap_;
	keyPixmap_->setMask(keyBitmap);
}
