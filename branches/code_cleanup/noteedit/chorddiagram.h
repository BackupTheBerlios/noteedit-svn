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

#ifndef CHORDDIAGRAM_H

#define CHORDDIAGRAM_H
#include <qpainter.h>
#include <qstring.h>
#include <qpoint.h>

class main_props_str;
class NTransPainter;

class NChordDiagram {
   public: 
	NChordDiagram();
	NChordDiagram(int *str, QString chordName, bool showDiagram);
	NChordDiagram(bool showDiagram, QString cname, char* strings);
	NChordDiagram(QString cname);
	NChordDiagram *clone();
	NChordDiagram(NChordDiagram *diagramm);
	void draw(NTransPainter *p, QPoint *startpoint, main_props_str *mainprops);
	int neededWidth();
	char *getStrings() {return strings_;}
	QString getChordName() {return chordName_;}
	char barree_[4][2];
	char getBarreCount() {return barree_count_;}
	char getFirst() {return firstFret_;}
	void transpose(int seminones);
	bool showDiagram_;
	bool isEqual(NChordDiagram *dia2);
	bool isAmbigous(NChordDiagram *dia2);
	void setValues(int *str, QString chordName, bool showDiagram);
   private:
	char barree_count_;
	char strings_[6];
	char firstFret_;
	QString firstFretStr_;
	QString chordName_;
	static QPoint fretPoint_;
	static QPoint ChordNamePoint_;
};


#endif /* CHORDDIAGRAM_H */
