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

class NChordDiagram;
class NClef;
class NPlayable;
class NChord;
class NRest;
class NKeySig;
class NTimeSig;
class NSign;
class NText;

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
		virtual int getType () const  = 0;
		virtual int getSubType() const {return 0;}
		virtual void draw(int flags = 0) = 0;
		virtual void calculateDimensionsAndPixmaps() = 0;
		virtual void setStaffProps(staff_props_str *staff_props) {staff_props_ = staff_props;}
		virtual void setMainProps(main_props_str *main_props) {main_props_ = main_props;}

		virtual int getMidiLength (bool = false) const {return 0;}

		virtual QRect *getBbox () {return &bbox_;} 
		void reposit(int xpos, int sequNr_);
		virtual int getXposDecorated() {return xpos_;}
		int getXpos() {return xpos_;}
		int getSequNr() {return sequNr_;}
		void setActual(bool ac) {actual_ = ac;}
		int intersects(const QPoint p) const;
		virtual int intersects_horizontally(const QPoint p) const {return intersects(p);} /* for "normal" elements except NChords */
		virtual QPoint *getTopY() {return 0;}
		virtual int getTopY2() {return 0;}
		virtual int getTopX2() {return 0;}
		virtual double getBotY() {return 0.0;}
		int midiTime_;

#define CONVERT_TO(type,classType)       return ((getType() & type) ? (classType *)this : 0 );
		NPlayable * playable() {CONVERT_TO(PLAYABLE,NPlayable)} // converts the element to NPlayable
		NChord * chord()       {CONVERT_TO(T_CHORD,NChord)}     // converts the element to NChord
		NRest * rest()         {CONVERT_TO(T_REST,NRest)}       // converts the element to NRest
		NSign * sign()         {CONVERT_TO(T_SIGN,NSign)}       // converts the element to NSign
		NClef * clef()         {CONVERT_TO(T_CLEF,NClef)}       // converts the element to NClef
		NKeySig * keySig()     {CONVERT_TO(T_KEYSIG,NKeySig)}   // converts the element to NKeySig
		NTimeSig * timeSig()   {CONVERT_TO(T_TIMESIG,NTimeSig)} // converts the element to NTimeSig
		NText * text()         {CONVERT_TO(T_TEXT,NText)}       // converts the element to NText

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
};

class NPlayable : public NMusElement {
	public:
		NPlayable(main_props_str *main_props, staff_props_str *staff_props);
		virtual void changeLength(int) = 0;

		void breakTuplet();
		void changeTupletList(QList<NPlayable> *tList) {tupletList_ = tList;}
		void resetTupletFlag() { status_ &= (~((STAT_TUPLET | STAT_LAST_TUPLET))); tupletList_ = 0; midiLength_ = computeMidiLength();}
		QList<NPlayable> *getTupletList() {return tupletList_;}
		void computeTuplet();
		static void computeTuplet(QList<NPlayable> *tupletlList, char numNotes, char playtime);
		virtual void setTupletParams(QList<NPlayable> *, bool, double, double, double, int, int, char, char) {}
		void unsetTuplet();
		QString *computeTeXTuplet(NClef *clef);
		bool isFirstInTuplet() {return tupletList_->first() == this;}

		virtual NChordDiagram *getChordChordDiagram() = 0;
		virtual void addChordDiagram(NChordDiagram *) = 0;
		virtual void removeChordDiagram() = 0;
		
		virtual void setDotted(int) = 0;
		
		virtual char getNumNotes() = 0;
		virtual char getPlaytime() = 0;
		virtual int computeMidiLength() const = 0;

	protected:
		QList<NPlayable> *tupletList_;	// all elements in this element's tuplet
};

#endif // MUSELEMENT_H
