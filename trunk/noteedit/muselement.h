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

#ifndef MUSELEMENT_H

#define MUSELEMENT_H
#include <qpoint.h>
#include <qrect.h>
#include "resource.h"
#include "constants.h"

#include "internals.h"

class NClef;
class NChordDiagram;

class NMusElement {
	public :
		NMusElement(main_props_str *main_props, staff_props_str *staff_props);
		void change(NMusElement *elem);
		status_type status_; /* element's flags - 64 bit long! */
		unsigned int status2_;
		int trill_;
		int dynamic_;
		int va_;
		bool dynamicAlign_;
		int slurY_;
		virtual NMusElement *clone() = 0;
		virtual ~NMusElement();
		virtual void draw(int flags = 0) = 0;
		virtual void setStaffProps(staff_props_str *staff_props) {staff_props_ = staff_props;}
		virtual void setMainProps(main_props_str *main_props) {main_props_ = main_props;}
		virtual void moveUp(int, int, NKeySig *) {};
		virtual void moveDown(int, int, NKeySig *) {};
		virtual void moveSemiToneUp(int, NClef *, NKeySig *) {}
		virtual void moveSemiToneDown(int, NClef *, NKeySig *) {}
		virtual void changeLength(int) {};
		virtual void changeBody(status_type) {};
		virtual void changeOffs(int, NKeySig *) {}
		virtual void setActualTied(bool) {};
		virtual int getSubType() const {return 0;}
		virtual QRect *getBbox () {return &bbox_;} 
		virtual QList<NNote> *getNoteList() {return 0;}
		virtual bool deleteNoteAtLine(int, int) {return false;}
		virtual void deletePart(NNote *) {};
		virtual NNote *searchLine(int , int) { return 0;}
		virtual NNote *insertNewNote(int , int , int , status_type) {return 0;}
		virtual void insertNewNote(NNote *) {};
		virtual NChordDiagram *getChordChordDiagram() {return 0;}
		void reposit(int xpos, int sequNr_);
		virtual int getXposDecorated() {return xpos_;}
		int getXpos() {return xpos_;}
		int getSequNr() {return sequNr_;}
		void setActual(bool ac) {actual_ = ac;}
		virtual int getType () const  = 0;
		int intersects(const QPoint p) const;
		virtual int intersects_horizontally(const QPoint p) const {return intersects(p);} /* for "normal" elements except NChords */
		virtual void setDotted(int) {}
		virtual int getMidiLength (bool = false) const {return 0;}
		virtual bool lastBeamed() {return false;}
		void breakTuplet();
		virtual void breakBeames() {}
		virtual QPoint *getTopY() {return 0;}
		virtual int getTopY2() {return 0;}
		virtual int getTopX2() {return 0;}
		virtual double getBotY() {return 0.0;}
		virtual void addChordDiagram(NChordDiagram *) {}
		virtual void removeChordDiagram() {}
		virtual char getNumNotes() {return 3;}
		virtual char getPlaytime() {return 2;}
		void computeTuplet();
		static void computeTuplet(QList<NMusElement> *tupletlList, char numNotes, char playtime);
		virtual void setTupletParams(QList<NMusElement> *, bool, double, double, double, int, int, char, char) {}
		void changeTupletList(QList<NMusElement> *tList) {tupletList_ = tList;}
		void unsetTuplet();
		QList<NMusElement> *getTupletList() {return tupletList_;}
		void resetTupletFlag() { status_ &= (~((STAT_TUPLET | STAT_LAST_TUPLET))); tupletList_ = 0; midiLength_ = computeMidiLength();}
		int midiTime_;
		virtual void calculateDimensionsAndPixmaps() = 0;
		QString *computeTeXTuplet(NClef *clef);
		bool isFirstInTuplet() {return tupletList_->first() == this;}
		virtual int computeMidiLength() const {return 0;}
	protected:
		bool actual_;
		int pixmapHeight_;
		int pixmapWidth_;
		QRect bbox_;
		staff_props_str *staff_props_;
		main_props_str *main_props_;
		int xpos_;
		int sequNr_;
		int midiLength_;
		double tupm_; double tupn_;
		double tupTeXn_;
		int xstart_, xend_;
		QList<NMusElement> *tupletList_;	// all elements in this element's tuplet
};

#endif // MUSELEMENT_H
