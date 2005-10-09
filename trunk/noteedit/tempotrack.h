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

#ifndef TEMPOTRACK_H
#define TEMPOTRACK_H
#include "sign.h"

template<class type> class ordereList : public QPtrList<type> {
	protected:
		virtual int compareItems ( QCollection::Item item1, QCollection::Item item2 ) {
			NSign *sign1, *sign2;
			sign1 = (NSign *) item1;
			sign2 = (NSign *) item2;
			if (sign1->getRealMidiTime() < sign2->getRealMidiTime()) return -1;
			if (sign1->getRealMidiTime() > sign2->getRealMidiTime()) return  1;
			return 0;
		}
};

class NTempoTrack {
	public:
		NTempoTrack();
		void insertTempoSign(NSign *sign);
		NSign *first();
		NSign *next();
		void clear();
		void initForPlaying(int startTime);
		int getTempoAtMidiTime(int MidiTime);
		void resolveRitardandoAndAccelerando();
	private:
		ordereList<NSign> orderedSigns_;
		int timeOfNextTempoSig_;
		int currentTempo_;
};

#endif /* TEMPOTRACK_H */
