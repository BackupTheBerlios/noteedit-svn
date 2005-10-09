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

#ifndef REST_H

#define REST_H

#include "muselement.h"

class QPixmap;

class NRest : public NPlayable {
	public:
		NRest(main_props_str *main_props_, staff_props_str *staff_props, int *y_voice_offs, int length=32, property_type properties = 0);
		~NRest();
		virtual NRest *clone();
		int getMultiRestLength() {return multiRestLength_;}
		void transposeChordDiagram(int semitones);
		virtual void changeLength(int length);
		virtual void draw(int flags = 0);
		virtual int getSubType() const {return length_;}
		virtual int getType() const {return T_REST;}
		virtual void setDotted(int dotcount);
		virtual int getMidiLength(bool = false) const  {return midiLength_;}
		virtual void calculateDimensionsAndPixmaps();
		virtual NChordDiagram *getChordChordDiagram() {return cdiagram_;}
		virtual QPoint *getTopY();
		virtual int getTopY2();
		virtual int getTopX2();
		virtual double getBotY();
		virtual char getNumNotes() {return numTupNotes_;}
		virtual char getPlaytime() {return tupRealTime_;}
		virtual void addChordDiagram(NChordDiagram *cdiag);
		virtual void removeChordDiagram();
		virtual void setTupletParams(QList<NPlayable> *tupelList, bool last, double m,
						 double n , double tuptexn, int xstart, int xend, char numnotes, char playtime);
		void setVoiceOffs(int *voice_offs) {yRestOffs_ = voice_offs;}
		virtual int computeMidiLength() const;
	private:
		int length_;
		int multiRestLength_;
		QPixmap *redPixmap_;
		QPixmap *blackPixmap_;
		QPixmap *greyPixmap_;
		QPoint nbaseDrawPoint_;
		QString lenString_;
		QRect pointPos1_;
		QRect pointPos2_;
		double m_, n_;
		int *yRestOffs_;
		QPoint tuplet0_, tuplet1_;
		QPoint tuplet00_, tuplet01_;
		QPoint tupletDigit_;
		char numTupNotes_, tupRealTime_;
		QPoint topYPoint_;
		NChordDiagram *cdiagram_;
		QPixmap *tupletMarker_;
		QPoint cdiagramDrawPoint_;
};

#endif // REST_H
