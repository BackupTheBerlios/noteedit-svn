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
		virtual ~NMusElement();

		virtual void change(NMusElement *elem);                 // copy method, copies the NMusElement related values
		virtual NMusElement *clone() = 0;                       // clones the element

		virtual int getType () const  = 0;                      // gives the type of the element (T_CHORD, T_CLEF...)
		virtual int getSubType() const {return 0;}              // gives the sub type of the element
		virtual void draw(int flags = 0) = 0;                   // draws the element onto the screen
		virtual void calculateDimensionsAndPixmaps() = 0;       // calculates the pixmap of the element

		virtual void setStaffProps(staff_props_str *staff_props) {staff_props_ = staff_props;} // staff and main
		virtual void setMainProps(main_props_str *main_props) {main_props_ = main_props;}      // properties

		virtual int getMidiLength (bool = false) const {return 0;}     // midi length of the element

		virtual QRect *getBbox () {return &bbox_;}              // gives the outliner rectangle of the element on the screen
		int intersects(const QPoint p) const;                   // check if the point is inside of the outliner rectangle
		virtual int intersects_horizontally(const QPoint p) const {return intersects(p);} /* for "normal" elements except NChords */

		void setActual(bool ac) {actual_ = ac;}                 // sets the element actual (for red pixmaps)

		void reposit(int xpos, int sequNr_);                    // sets the X position and sequence number of the element
		virtual int getXposDecorated() {return xpos_;}          // ???
		int getXpos() {return xpos_;}                           // the X position of the element (width0+width1+...)
		int getSequNr() {return sequNr_;}                       // the sequence number of the element (0,1,2,...)

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
		bool actual_;                        // flag, on if the element is actual (red)
		int pixmapHeight_;                   // the height of the element's pixmap
		int pixmapWidth_;                    // the width of the element's pixmap
		QRect bbox_;                         // the outliner rectangle on the screen
		staff_props_str *staff_props_;       // staff properties
		main_props_str *main_props_;         // main properties
		int xpos_;                           // the X position of the element on the screen (width0+width1+...)
		int sequNr_;                         // the sequence number of the element (0,1,2,...)
};

class NPlayable : public NMusElement {
	public:
		inline void addProperty( property_type prop ) { properties_ |= prop; }
		inline void removeProperty( property_type prop ) { properties_ &= ~prop; }
		inline void invertProperty( property_type prop ) { properties_ ^= prop; }
		inline void setProperty( property_type prop, bool on ) { if(on) addProperty( prop ); else removeProperty( prop ); }
		inline property_type properties() const { return properties_; }
		inline void setProperties( property_type props ) { properties_ = props; }
		inline property_type hasProperty( property_type prop ) const { return properties_ & prop; }

		NPlayable(main_props_str *main_props, staff_props_str *staff_props);
		virtual void changeLength(int) = 0;

		void breakTuplet();
		void changeTupletList(QList<NPlayable> *tList) {tupletList_ = tList;}
		void resetTupletFlag() { removeProperty(PROP_TUPLET | PROP_LAST_TUPLET); tupletList_ = 0; midiLength_ = computeMidiLength();}
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

		virtual QPoint *getTopY() = 0;
		virtual int getTopY2() = 0;
		virtual int getTopX2() = 0;
		virtual double getBotY() = 0;

	protected:
		property_type properties_; /* element's flags - 64 bit long! */

		QList<NPlayable> *tupletList_;	// all elements in this element's tuplet
		double tupm_; double tupn_;
		int xstart_, xend_;
		double tupTeXn_;
		int midiLength_;
};

#endif // MUSELEMENT_H
