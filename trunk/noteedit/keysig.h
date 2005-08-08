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

#ifndef KEYSIG_H

#define KEYSIG_H

#include "muselement.h"

class NClef;

class NKeySig : public NMusElement {
	public:
		NKeySig(main_props_str *main_props, staff_props_str *staff_props);
		virtual ~NKeySig();
		void change(NKeySig *ksig);                     // copy another key signature into this one
		virtual NKeySig *clone();                       // cloning
 		void reset();                                   // clears the key signature (sets to default)
		void changeHalfTone(NNote *note);               // changes the half notes (b,#), according to the key signature
		void setKey(int note, status_type kind);        // changes the signature of a note
		void setClef(NClef *ac_clef);                   // sets the clef belonging to this key signature
		void setRegular(int count, status_type kind);   // changes the signature to a regular one
		bool isRegular(status_type *kind, int *count);  // tells whether the key signature is regular
		status_type accNeeded(int line, int offs);      // tells what kind of accent is needed to a note(line,offs)
		int computeOffs(int line);                      // determines the offset used for a line (-1, 0, 1, with temporary sigs)
		int determineDistanceUp(NNote *note);           // determines the distance from the 1-line upper note, 1 or 2
		status_type getStatus(int note);                // gives the modifier of a note (without temporary signatures)
		void setTempAcc(int line, status_type kind);    // sets temporary accent for a line (barlines clear these kind of accents)
		void resetAtBar();                              // resets the temporary accents (bar line)
		virtual int getType () const {return T_KEYSIG;} // the type of the NMusElement
		virtual void draw(int flags = 0);               // drawing function
		char *printKeys();                              // converts the keys into an ASCII representation
		void addSign(status_type kind, char pitch);     // adds a new signature to the key signature
		bool isDrawable() {return drawable_;}           // tells whether the key signature is visible (has size)
		bool isEqual(NKeySig *otherKeysig);             // compares two key signatures
		virtual void calculateDimensionsAndPixmaps();   // calculates the drawing pixmaps
		void setPreviousKeySig(NKeySig *prevKeySig);    // Draw, tells the previous key signature (for resolving it)
/* #define KEYSIG_DEBUG */
#ifdef KEYSIG_DEBUG
		void print();                                   // prints out the key signature
#endif
/* ------------------------------- methods for context keysig ------------------------------------------*/
		void drawContextKeySig();
		void changeInContextKeySig(NKeySig *ksig);
		void calculateContextPixmap();
	private:
		int accCount();                          // the number of accents in the key signature
		status_type *noteStatus_;                // the key signature for a note ( C[0] - B[6] )
		status_type *tempNoteStatus_;            // the accent is placed only once in a bar, status whether the note is already accented
		static int nameTab_[7];                  // the names of the notes
		static int crossTab_[7];                 // the regular locations of the crosses
		static int flatTab_[7];                  // the regular locations of the flats
		static char str[128];                    // buffer where the character representation of the key sig stored
		NClef *acClef_;                          // the clef corresponding to that key signature
		int pixmapWidth_, resPixmapWidth_;       // Draw, the width of the key signature, and resolving signatures
		bool statusChanged_;                     // Draw, indicates if redraw is necessary
		bool drawable_;                          // Draw, indicates that the element is visible (no accent -> invisible)
		int resolvOffs_;                         // Draw, offset used for resolving the previous key signature
		QPixmap *keyPixmap_;                     // Draw, the cached pixmap of the key signature
		QPixmap *key_redPixmap_;                 // Draw, the cached red (selected) pixmap of the key signature
		QPixmap *resolvPixmap_;                  // Draw, the cached pixmap of resolving the previous key signature
		QPixmap *resolv_redPixmap_;              // Draw, the cached red (selected) pixmap of resolving the previous key signature
		QPoint nbaseDrawPoint_;                  // Draw, the base point of the key signature
		QPoint resolvDrawPoint_;                 // Draw, the base point of the resolving keys
		NKeySig *previousKeySig_;                // Draw, pointer to the previous key signature
		NKeySig *computedPreviousKeySig_;        // Draw, pointer to the previous key signature by which pixmaps were calculated

		static NClef defaultClef_;               // default clef
};
	


#endif // KEYSIG_H
