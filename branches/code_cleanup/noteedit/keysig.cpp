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
#include <iostream>
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
	int i;
	accents_ = new status_type[7];
	tempAccents_ = new status_type[MAXLINE-MINLINE+1];

	/* clear the accents_ and tempAccents_ arrays to PROP_NATUR and PROP_NO_ACC */
	for (i=0; i<7; accents_[i++] = PROP_NATUR);
	for (i=0; i<(MAXLINE-MINLINE+1); tempAccents_[i++]=PROP_NO_ACC);
	
	statusChanged_ = true;
	acClef_ = &defaultClef_;
	actual_ = false;
	pixmapWidth_ = 5;
	resolv_redPixmap_ = resolvPixmap_ = keyPixmap_ = key_redPixmap_ = 0;
	drawable_ = false;
	resPixmapWidth_ = resolvOffs_ = 0;
	computedPreviousKeySig_ = previousKeySig_ = 0;
	calculateDimensionsAndPixmaps();
}

void NKeySig::change(NKeySig *ksig) {
	statusChanged_ = true;
	NMusElement::change(ksig);
	memcpy(accents_, ksig->accents_, sizeof(status_type)*7);
	memcpy(tempAccents_, ksig->accents_, sizeof(status_type)*7);
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
	calculateDimensionsAndPixmaps();
}

void NKeySig::setPreviousKeySig(NKeySig *prevKeySig) {
	previousKeySig_ = prevKeySig;
} 

bool NKeySig::isEqual(NKeySig *otherKeysig) {
	if (!otherKeysig) return false;
	for (int i = 0; i < 7; ++i) {
		if (accents_[i] != otherKeysig->accents_[i]) return false;
	}
	return true;
}

int NKeySig::getSubType() const {
	status_type type = PROP_NO_ACC;
	for( int i = 0; i < 7; i++ ) {
		if( accents_[ i ] != PROP_NATUR ) {
			if( type != PROP_NO_ACC && type != accents_[ i ] )
				return PROP_NO_ACC;
			type = accents_[ i ];
		}
	}
	return type;
}

void NKeySig::setClef(NClef * ac_clef) {
	acClef_ = ac_clef;

	statusChanged_ = true;
	if (keyPixmap_) delete keyPixmap_;
	if (key_redPixmap_) delete key_redPixmap_;
	keyPixmap_ = 0;
        key_redPixmap_ = 0;
	calculateDimensionsAndPixmaps();
}

void NKeySig::changeHalfTone(NNote *note) {
	int notenr = acClef_->line2NoteNumber(note->line);
	status_type kind;
	int count;
	statusChanged_ = true;
	switch (note->offs) {
		case  1: if (isRegular(&kind, &count))  {
				if (kind == PROP_FLAT) {
					(note->line)++; note->offs = -1;
				}
			 } else if (accents_[(notenr+1) % 7] == PROP_FLAT) {
				(note->line)++; note->offs = -1; 
			 }
			 break;
		case -1: if (isRegular(&kind, &count)) {
				if (kind == PROP_CROSS) {
					(note->line)--; note->offs = 1;
				}
			 }
			 else if (accents_[(notenr+6) % 7] == PROP_CROSS) {
				(note->line)--; note->offs = 1;
			 }
			 break;
	}
}

NKeySig *NKeySig::clone() {
	status_type *tmp1;
	status_type *tmp2;
	NKeySig *ckeysig;
	ckeysig = new NKeySig(main_props_, staff_props_);

	tmp1 = ckeysig->accents_;
	tmp2 = ckeysig->tempAccents_;

	*ckeysig = *this;

	ckeysig->accents_ = tmp1;
	ckeysig->tempAccents_ = tmp2;

	/* clear tempAccents_ array with PROP_NO_ACC */
	for (int i=0; i<(MAXLINE-MINLINE+1); tempAccents_[i++]=PROP_NO_ACC);
	memcpy(ckeysig->accents_, accents_, sizeof(status_type)*7);

	ckeysig->resolv_redPixmap_ = ckeysig->resolvPixmap_ = ckeysig->keyPixmap_ = ckeysig->key_redPixmap_ = 0;
	ckeysig->previousKeySig_ = ckeysig->computedPreviousKeySig_ = 0;
	ckeysig->resolvOffs_ = 0;
	ckeysig->statusChanged_ = true;

	return ckeysig;
}


NKeySig::~NKeySig() {
	delete accents_;
	delete tempAccents_;
	if (keyPixmap_ != 0) delete keyPixmap_;
	if (key_redPixmap_ != 0) delete key_redPixmap_;
	if (resolv_redPixmap_ != 0) delete resolv_redPixmap_; 
	if (resolvPixmap_ != 0) delete resolvPixmap_;
}

#ifdef KEYSIG_DEBUG
void NKeySig::print() {
	int i;
	printf("stat: ");
	for (i = 0; i < 7; ++i) {
		switch (accents_[i]) {
			case PROP_CROSS: printf("#");break;
			case PROP_FLAT: printf("&");break;
			case PROP_NATUR: printf("n");break;
			case PROP_NO_ACC: printf(".");break;
			default: printf("?"); break;
		}
	}
	printf("; tmp: ");
	for (i = 0; i < 7; ++i) {
		switch (tempAccents_[i]) {
			case PROP_CROSS: printf("#");break;
			case PROP_FLAT: printf("&");break;
			case PROP_NATUR: printf("n");break;
			case PROP_NO_ACC: printf(".");break;
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

int NKeySig::getOffset(int line) {
	int pp, idx;
	status_type status;

	idx = LINE2TABIDX(line);
	if ((status = tempAccents_[idx]) == PROP_NO_ACC) {
		pp = acClef_->line2NoteNumber(line);
		status = accents_[pp];
	}
	switch (status) {
		case PROP_CROSS: return 1; break;
		case PROP_FLAT: return -1; break;
		case PROP_DCROSS: return 2; break;
		case PROP_DFLAT: return -2; break;
	}
	return 0;
}

int NKeySig::determineDistanceUp(NNote *note) {
	int midival1, midival2, offs2, line2;
	midival1 = acClef_->line2Midi( note->line, note->offs );
	line2 = note->line+1;
	offs2 = getOffset(line2);
	midival2 = acClef_->line2Midi( line2, offs2 );
	return midival2 - midival1;
}

	

status_type NKeySig::accentNeeded(int line, int offs) {
	int note, idx;
	status_type status;

	idx = LINE2TABIDX(line);
	if ((status = tempAccents_[idx]) == PROP_NO_ACC) {
		note = acClef_->line2NoteNumber(line);
		status = accents_[note]; 
	}
	if (offs == -1 && status == PROP_FLAT) return PROP_NO_ACC;
	if (offs ==  1 && status == PROP_CROSS) return PROP_NO_ACC;
	if (offs == -2 && status == PROP_DFLAT) return PROP_NO_ACC;
	if (offs ==  2 && status == PROP_DCROSS) return PROP_NO_ACC;
	if (offs ==  0 && (status == PROP_FLAT || status == PROP_CROSS || status == PROP_DCROSS || status == PROP_DFLAT)) return PROP_NATUR;
	if (offs ==  1) return PROP_CROSS;
	if (offs == -1) return PROP_FLAT;
	if (offs ==  2) return PROP_DCROSS;
	if (offs == -2) return PROP_DFLAT;
	return PROP_NO_ACC;
}

void NKeySig::deleteTempAccents() {
	/* clear tempAccents_ array to PROP_NO_ACC */
	for (int i=0; i<(MAXLINE-MINLINE+1); tempAccents_[i++]=PROP_NO_ACC);
}
	
void NKeySig::reset() {
	statusChanged_ = true;
	int i;

	/* clear the accents_ and tempAccents_ arrays to PROP_NATUR and PROP_NO_ACC */
	for (i=0; i<7; accents_[i++] = PROP_NATUR);
	for (i=0; i<(MAXLINE-MINLINE+1); tempAccents_[i++]=PROP_NO_ACC);

	if (keyPixmap_) delete keyPixmap_;
	if (key_redPixmap_) delete key_redPixmap_;
	keyPixmap_ = 0;
        key_redPixmap_ = 0;
	acClef_ = &defaultClef_;
}

	
void NKeySig::setTempAccent(int line, status_type kind) {
	tempAccents_[LINE2TABIDX(line)] = kind;
	
}

status_type NKeySig::getAccent(int note) {
	if (note < 0 || note > 6) {
		NResource::abort("getAccent(): internal error");
	}
	return accents_[note];
}

void NKeySig::setAccent(int note, status_type kind) {
	if (note < 0 || note > 7) {
		NResource::abort("setAccent(): internal error");
	}
	accents_[note] = kind;
	if (keyPixmap_) delete keyPixmap_;
	if (key_redPixmap_) delete key_redPixmap_;
	keyPixmap_ = 0;
        key_redPixmap_ = 0;
}

void NKeySig::setRegular(int count, status_type kind) {
	int i, *tab;
	if (count > 7) return;
	statusChanged_ = true;
	reset();
	switch (kind) {
		case PROP_CROSS: tab = crossTab_;
				  break;
		case PROP_FLAT:  tab = flatTab_;
				  break;
		default: NResource::abort("setRegular(): unknown kind");
				  return; // eliminate warnings
	}
	for (i = 0; i < count; ++i)
		accents_[tab[i]] = kind;
	calculateDimensionsAndPixmaps();
}

int NKeySig::accentCount() {
	int i, count = 0;

	for (i = 0; i < 7; ++i) {
		if (accents_[i] != PROP_NATUR) count++;
	}
	return count;
}

bool NKeySig::isRegular(status_type *kind, int *count) {
	bool ok[7];
	int i;
	*kind = PROP_NO_ACC;
	*count = 0;

	if (accentCount() == 0)  return true;

	for (i = 0; i < 7; ok[i++] = false);

	for (i = 0; i < 7; ++i) {
		if (accents_[i] != PROP_NATUR) {
			(*count)++;
			ok[i] = true;
			if (*kind == PROP_NO_ACC) {
				*kind = accents_[i];
			}
			else if (*kind != accents_[i]) return false;
		}
	}
	switch (*kind) {
		case PROP_CROSS:
			for (i = 0; i < *count; ++i) {
				if (!ok[crossTab_[i]]) return false;
			}
			return true;
		case PROP_FLAT:
			for (i = 0; i < *count; ++i) {
				if (!ok[flatTab_[i]]) return false;
			}
			return true;
		case PROP_NO_ACC:
		case PROP_NATUR: return false;
		default: NResource::abort("isRegular(): internal error");
	}
	return false;
}

char *NKeySig::toString() {
	char *cptr;
	int i;
	cptr = NKeySig::str;
	for (i = 0; i < 7; ++i) {
		switch (accents_[i]) {
			case PROP_CROSS: *cptr++ = nameTab_[i];
				      *cptr++ = '#';
				      *cptr++ = ' ';
				      break;
			case PROP_FLAT:  *cptr++ = nameTab_[i];
				      *cptr++ = '&';
				      *cptr++ = ' ';
				      break;
		}
	}
	*cptr = '\0';
	return NKeySig::str;
}

void NKeySig::setAccentByNoteName(char pitch, status_type kind) {
	for( int i=0; i < 7; i++ )
		if( pitch == nameTab_[i] ) {
			setAccent(i, kind );
			return;
		}
}

void NKeySig::calculateDimensionsAndPixmaps() {
	status_type kind;
	int count;
	int i, j;
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
		int naturKeyCount = 0;
		for( i=0; i != 7; i++ )
			if( accents_[ i ] != previousKeySig_->accents_[ i ] && previousKeySig_->accents_[ i ] != PROP_NATUR )
				naturKeyCount++;
		if (previousKeySig_ && !isEqual(previousKeySig_) && naturKeyCount != 0) {
			statusChanged_ = true;
			resPixmapWidth_ = NResource::naturPixmap_->width();
			resPixmapWidth_ += SIGN_DIST * naturKeyCount;
			resolvOffs_ = resPixmapWidth_;

			if (resolvPixmap_) delete resolvPixmap_;
			if (resolv_redPixmap_) delete resolv_redPixmap_;
			resolvPixmap_ = new QPixmap(resPixmapWidth_, PIXMAP_HEIGHT);
			resolv_redPixmap_  = new QPixmap(resPixmapWidth_, PIXMAP_HEIGHT);

			if (previousKeySig_->isRegular(&kind, &count)) {
				if (count) noSignesInPrevKey = false;
				draw_offs = NATUR_DRAW_OFFS;
				painter.begin(resolvPixmap_);
				painter.setPen(NResource::noPen_);
				painter.setBrush(NResource::backgroundBrush_);
				painter.fillRect(0, 0, resPixmapWidth_, PIXMAP_HEIGHT, NResource::backgroundBrush_);
				for (i = j = 0; i < count; ++i) {
					int note = ( kind == PROP_FLAT ) ? flatTab_[ i ] : crossTab_[ i ];
					if( accents_[ note ] != previousKeySig_->accents_[ note ] && previousKeySig_->accents_[ note ] != PROP_NATUR )
						painter.drawPixmap(j++*SIGN_DIST,4*LINE_DIST+draw_offs-acClef_->getAccPos(kind, i)*LINE_DIST/2, *NResource::naturPixmap_);
				}
				painter.end();
		
				painter.begin(resolv_redPixmap_);
				painter.setPen(NResource::noPen_);
				painter.setBrush(NResource::backgroundBrush_);
				painter.fillRect(0, 0, resPixmapWidth_, PIXMAP_HEIGHT, NResource::backgroundBrush_);
				for (i = j = 0; i < count; ++i) {
					int note = ( kind == PROP_FLAT ) ? flatTab_[ i ] : crossTab_[ i ];
					if( accents_[ note ] != previousKeySig_->accents_[ note ] && previousKeySig_->accents_[ note ] != PROP_NATUR )
						painter.drawPixmap(j++*SIGN_DIST,4*LINE_DIST+draw_offs-acClef_->getAccPos(kind, i)*LINE_DIST/2, *NResource::naturPixmap_);
				}
			
				painter.end();
			}
			else {
				painter.begin(resolvPixmap_);
				painter.setPen(NResource::noPen_);
				painter.setBrush(NResource::backgroundBrush_);
				painter.fillRect(0, 0, resPixmapWidth_, PIXMAP_HEIGHT, NResource::backgroundBrush_);
				draw_offs = NATUR_DRAW_OFFS;
				for (j = 0, i = 0; i < 7; ++i) {
					if (previousKeySig_->accents_[i] == PROP_NATUR) continue;
					noSignesInPrevKey = false;
					if (previousKeySig_->accents_[i] == accents_[ i ] ) continue;
					painter.drawPixmap(j++*SIGN_DIST,4*LINE_DIST+draw_offs-acClef_->noteNumber2Line(i)*LINE_DIST/2, *NResource::naturPixmap_);
				}
				painter.end();
		
				painter.begin(resolv_redPixmap_);
				painter.setPen(NResource::noPen_);
				painter.setBrush(NResource::backgroundBrush_);
				painter.fillRect(0, 0, resPixmapWidth_, PIXMAP_HEIGHT, NResource::backgroundBrush_);
				for (j = 0, i = 0; i < 7; ++i) {
					if (previousKeySig_->accents_[i] == PROP_NATUR) continue;
					if (previousKeySig_->accents_[i] == accents_[ i ] ) continue;
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

	if (statusChanged_) {
		statusChanged_ = false;
		if (accentCount() != 0) {
			pixmapWidth_ = NResource::crossPixmap_->width();
			pixmapWidth_ += SIGN_DIST * accentCount();
		
			if (keyPixmap_) delete keyPixmap_;
			if (key_redPixmap_) delete key_redPixmap_;
			keyPixmap_ = new QPixmap(pixmapWidth_, PIXMAP_HEIGHT);
			key_redPixmap_ = new QPixmap(pixmapWidth_, PIXMAP_HEIGHT);
		
			if (isRegular(&kind, &count)) {
				drawable_ = count != 0;
				if (kind == PROP_CROSS) {
					pix = NResource::crossPixmap_;
					draw_offs = CROSS_DRAW_OFFS;
				}
				else {
					pix = NResource::flatPixmap_;
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
		
				pix = (kind == PROP_CROSS) ?  NResource::crossRedPixmap_ : NResource::flatRedPixmap_;
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
				drawable_ = false;
				for (j=0, i = 0; i < 7; ++i) {
					if (accents_[i] == PROP_NATUR) continue;
					if (accents_[i] == PROP_CROSS) {
						pix = NResource::crossPixmap_;
						draw_offs = CROSS_DRAW_OFFS;
						drawable_ = true;
					}
					else {
						pix = NResource::flatPixmap_;
						draw_offs = FLAT_DRAW_OFFS;
						drawable_ = true;
					}
					painter.drawPixmap(j++*SIGN_DIST,4*LINE_DIST+draw_offs-acClef_->noteNumber2Line(i)*LINE_DIST/2, *pix);
				}
				painter.end();
		
				painter.begin(key_redPixmap_);
				painter.setPen(NResource::noPen_);
				painter.setBrush(NResource::backgroundBrush_);
				painter.fillRect(0, 0, pixmapWidth_, PIXMAP_HEIGHT, NResource::backgroundBrush_);
				for (j = 0, i = 0; i < 7; ++i) {
					if (accents_[i] == PROP_NATUR) continue;
					pix = (accents_[i] == PROP_CROSS) ? NResource::crossRedPixmap_ : NResource::flatRedPixmap_;
					draw_offs = (accents_[i] == PROP_CROSS) ? CROSS_DRAW_OFFS : FLAT_DRAW_OFFS;
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
	drawable_ = drawable_ || !noSignesInPrevKey;
}

void NKeySig::draw(int /* dummy */) {
	if (!drawable_) return;
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
	if (!drawable_) return;
	main_props_->tp->beginUnclippedYtranslated();
	main_props_->tp->drawPixmap(main_props_->context_keysig_xpos, nbaseDrawPoint_.y(), *keyPixmap_);
	main_props_->tp->end();
}

void NKeySig::changeInContextKeySig(NKeySig *ksig) {
	NMusElement::change(ksig);
	acClef_ = ksig->acClef_;
	memcpy(accents_, ksig->accents_, sizeof(status_type)*7);
	pixmapWidth_ = 5;
	if (staff_props_->base) calculateContextPixmap();
}

void NKeySig::calculateContextPixmap() { /* for faster computation of context key */
	status_type kind;
	int count;
	int i, j;
	int draw_offs;
	QPainter painter;
	QPixmap *pix;
	QBitmap keyBitmap;
	

	nbaseDrawPoint_ = QPoint (xpos_+resolvOffs_, staff_props_->base-Y_BORDER_UP);
	bbox_ = QRect(xpos_, staff_props_->base , pixmapWidth_, PIXMAP_HEIGHT);

	if (accentCount() == 0) {drawable_ = false; return;}
	pixmapWidth_ = NResource::crossPixmap_->width();
	pixmapWidth_ += SIGN_DIST * accentCount();
	
	keyPixmap_ = new QPixmap(pixmapWidth_, PIXMAP_HEIGHT);
	
	if (isRegular(&kind, &count)) {
		drawable_ = count != 0;
		if (kind == PROP_CROSS) {
			pix = NResource::crossPixmap_;
			draw_offs = CROSS_DRAW_OFFS;
		}
		else {
			pix = NResource::flatPixmap_;
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
		drawable_ = false;
		for (j=0, i = 0; i < 7; ++i) {
			if (accents_[i] == PROP_NATUR) continue;
			if (accents_[i] == PROP_CROSS) {
				pix = NResource::crossPixmap_;
				draw_offs = CROSS_DRAW_OFFS;
				drawable_ = true;
			}
			else {
				pix = NResource::flatPixmap_;
				draw_offs = FLAT_DRAW_OFFS;
				drawable_ = true;
			}
			painter.drawPixmap(j++*SIGN_DIST,4*LINE_DIST+draw_offs-acClef_->noteNumber2Line(i)*LINE_DIST/2, *pix);
		}
		painter.end();
	}
	keyBitmap = *keyPixmap_;
	keyPixmap_->setMask(keyBitmap);
}
