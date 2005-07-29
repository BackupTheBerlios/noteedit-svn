#include <qpainter.h>
#include <string.h>
#include "chorddiagram.h"
#include "resource.h"
#include "muselement.h"
#include "transpainter.h"
#include "../kguitar_excerpt/global.h"
#include "../kguitar_excerpt/chord.h"

#define X_DIST 16
#define Y_DIST 16
#define X_BORDER 5 
#define Y_BORDER 5 

#define FRET_STR_X_OFFS (-5*X_DIST/2)
#define FRET_STR_Y_OFFS (2*Y_DIST)
#define CHORD_NAME_X_OFFS (X_DIST)
#define CHORD_NAME_Y_OFFS 120

QPoint NChordDiagram::fretPoint_(FRET_STR_X_OFFS, FRET_STR_Y_OFFS);
QPoint NChordDiagram::ChordNamePoint_(CHORD_NAME_X_OFFS, CHORD_NAME_Y_OFFS);

NChordDiagram::NChordDiagram(int *str, QString chordName, bool showDiagram) {
	setValues(str, chordName, showDiagram);
}

void NChordDiagram::setValues(int *str, QString chordName, bool showDiagram) {
	int barre, eff, i, j;
	bool noff = true;


	firstFret_ = 127;
	barree_count_ = 0;
	chordName_ = chordName;
	showDiagram_ = showDiagram;
	for (i = 0; i < 6; i++) {
		strings_[i] = (char ) str[i];
	}
	for (i = 0; i < 6; i++) {
		if (strings_[i] < firstFret_ && strings_[i] > 0) {
			firstFret_ = strings_[i];
		}
		if (strings_[i] > 5) {
			noff = false;
		}
	}
	if (noff) {
		firstFret_ = 1;
	}
	firstFretStr_.sprintf("fr %d", firstFret_);
	for (i=0;i<NUMFRETS;i++) {
		barre=0;
		while ((strings_[6-barre-1]>=(i+firstFret_)) ||
			   (strings_[6-barre-1]==-1)) {
			barre++;
			if (barre>6-1)
				break;
		}

		while ((strings_[6-barre]!=(i+firstFret_)) && (barre>1))
			barre--;
		eff = 0;
		for (j = 6-barre; j < 6; j++) {
			if (strings_[j] != -1)
				eff++;
		}

		if (eff > 2) {
			if (barree_count_ < 4) {
				barree_[barree_count_][0] = i;
				barree_[barree_count_++][1] = 6 - barre;
			}
		}
	}
}

NChordDiagram::NChordDiagram(bool showDiagramm, QString cname, char* strings_ptr) {
	int i = 0, n, strings[6], digits;

	while (i < 6) {
		while (*strings_ptr != '\0' && *strings_ptr == ' ') strings_ptr++;
		switch (*strings_ptr) {
			case 'o': strings[i++] = 0; strings_ptr++;break;
			case 'x': strings[i++] = -1; strings_ptr++;break;
			default:n = 0;
				digits = 0;
				while (*strings_ptr >= '0' && *strings_ptr <= '9') {
					n *= 10;
					digits++;
					n += *strings_ptr - '0';
					strings_ptr++;
				}
				if (!digits) {
					NResource::abort("NChordDiagram: internal error", 1);
				}
				strings[i++] = n; 
				break;
		}
	}
	if (i != 6) {
		NResource::abort("NChordDiagram: internal error", 2);
	}
	setValues(strings, cname, showDiagramm);
}
	

NChordDiagram::NChordDiagram() {
	int i;

	firstFret_ = 1;
	barree_count_ = 0;
	showDiagram_ = false;
	for (i = 0; i < 6; i++) {
		strings_[i] = (char) 0;
	}
}

NChordDiagram::NChordDiagram(QString cname) {
	int i, j;
	QChar c;

	firstFret_ = 1;
	barree_count_ = 0;
	showDiagram_ = false;
	for (i = 0; i < 6; i++) {
		strings_[i] = (char) 0;
	}
	chordName_ = cname;
	c = chordName_.at(0);
	for (j = 0, i = 1; c != '\0' && c == '_'; i++) {
		if (c == '_') j++;
		c = chordName_.at(i);
	}
	if (j > 0) {
		chordName_ = chordName_.right(chordName_.length() - j);
	}
}

NChordDiagram::NChordDiagram(NChordDiagram *diagramm) {
	memcpy(barree_, diagramm->barree_, sizeof(barree_));
	memcpy(strings_, diagramm->strings_, sizeof(strings_));
	barree_count_ = diagramm->barree_count_;
	firstFret_ = diagramm->firstFret_;
	firstFretStr_ = diagramm->firstFretStr_;
	chordName_ = diagramm->chordName_;
	fretPoint_ = diagramm->fretPoint_;
	ChordNamePoint_ = diagramm->ChordNamePoint_;
	showDiagram_ = diagramm->showDiagram_;
}

NChordDiagram *NChordDiagram::clone() {
	int i, j;
	NChordDiagram *c_diag;
	QChar c;

	c_diag = new NChordDiagram();
	*c_diag = *this;
	c = c_diag->chordName_.at(0);
	for (j = 0, i = 1; c != '\0' && c == '_'; i++) {
		if (c == '_') j++;
		c = c_diag->chordName_.at(i);
	}
	if (j > 0) {
		c_diag->chordName_ = c_diag->chordName_.right(c_diag->chordName_.length() - j);
	}
	return c_diag;
}

void NChordDiagram::transpose(int semitones) {
	ChordSelector::transposeChordName(&chordName_, semitones);
}
	
bool NChordDiagram::isEqual(NChordDiagram *dia2) {
	int i;
	if (chordName_ != dia2->chordName_) return false;
	if (showDiagram_ != dia2->showDiagram_) return false;
	for (i = 0; i < 6; i++) {
		if (strings_[i] != dia2->strings_[i]) return false;
	}
	return true;
}

bool NChordDiagram::isAmbigous(NChordDiagram *dia2) {
	int i;
	if (chordName_ != dia2->chordName_) return false;
	if (showDiagram_ != dia2->showDiagram_) return true;
	for (i = 0; i < 6; i++) {
		if (strings_[i] != dia2->strings_[i]) return true;
	}
	return false;
}

void NChordDiagram::draw(NTransPainter *p, QPoint *startpoint, main_props_str *mainprops) {
#define Z_ROUND(x) ((int) (((x)*(mainprops->zoom)) + 0.5))
#define CIRCLE 10
	int i;

	p->toggleToScaledText(true);
	p->setFont( mainprops->scaledText_ );
	p->drawScaledText(*startpoint+ChordNamePoint_, chordName_);
	if (!showDiagram_) return;
	p->setBrush(NResource::blackBrush_);
	for (i = 0; i <= NUMFRETS; i++) {
		p->drawLine( Z_ROUND(startpoint->x()+X_BORDER), Z_ROUND(startpoint->y()+Y_BORDER+i*Y_DIST),
		 Z_ROUND(startpoint->x()+X_BORDER+(6-1)*X_DIST), Z_ROUND(startpoint->y()+Y_BORDER+i*Y_DIST));
	}
	for (i = 0; i < barree_count_; i++) {
		p->drawRect(Z_ROUND(startpoint->x()+X_BORDER+barree_[i][1]*X_DIST),
			    Z_ROUND(startpoint->y()+Y_BORDER+barree_[i][0]*Y_DIST+(Y_DIST-CIRCLE)/2),
				Z_ROUND((6-1-barree_[i][1])*X_DIST), Z_ROUND(CIRCLE));
	}
	for (i = 0; i < 6; i++) {
		p->drawLine(Z_ROUND(startpoint->x()+X_BORDER+i*X_DIST), Z_ROUND(startpoint->y()+Y_BORDER),
		 Z_ROUND(startpoint->x()+X_BORDER+i*X_DIST), Z_ROUND(startpoint->y()+Y_BORDER+NUMFRETS*Y_DIST));
		if (strings_[i] > 0) {
			p->drawEllipse(Z_ROUND(startpoint->x()+X_BORDER+i*X_DIST-CIRCLE/2), Z_ROUND(startpoint->y()+Y_BORDER+(strings_[i]-firstFret_)*Y_DIST+(Y_DIST-CIRCLE)/2),
			Z_ROUND(CIRCLE), Z_ROUND(CIRCLE));
		}
		else if (strings_[i] == -1) {
			p->drawLine(Z_ROUND(startpoint->x()+X_BORDER+i*X_DIST-CIRCLE/2), Z_ROUND(startpoint->y()+Y_BORDER+Y_DIST/2-CIRCLE/2),
				    Z_ROUND(startpoint->x()+X_BORDER+i*X_DIST+CIRCLE/2), Z_ROUND(startpoint->y()+Y_BORDER+Y_DIST/2+CIRCLE/2));
			p->drawLine(Z_ROUND(startpoint->x()+X_BORDER+i*X_DIST-CIRCLE/2), Z_ROUND(startpoint->y()+Y_BORDER+Y_DIST/2+CIRCLE/2),
				    Z_ROUND(startpoint->x()+X_BORDER+i*X_DIST+CIRCLE/2), Z_ROUND(startpoint->y()+Y_BORDER+Y_DIST/2-CIRCLE/2));
		}
	}
	if (firstFret_ == 1) return;
	p->setFont( mainprops->scaledMiniItalic_ );
	p->drawScaledText(*startpoint+fretPoint_, firstFretStr_);
}

int NChordDiagram::neededWidth() {
	if (showDiagram_) return 2*X_BORDER+(6-1)*X_DIST;
	return 0;
}
