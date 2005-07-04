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

#include "tempotrack.h"
#include "midimapper.h"
#define TEMPO_GRANULATITY NOTE16_LENGTH

NTempoTrack::NTempoTrack() {
	orderedSigns_.setAutoDelete(true);
}
	
void NTempoTrack::initForPlaying(int startTime) {
	NSign *sign, *sign1;
	currentTempo_ = DEFAULT_TEMPO;
	timeOfNextTempoSig_ = -1;
	resolveRitardandoAndAccelerando();
	for (sign = orderedSigns_.first(); sign; sign = orderedSigns_.next()) {
		if (sign->getRealMidiTime() >= startTime) {
			break;
		}
		currentTempo_ = sign->getTempo();
	}
	if (sign) {
		if (sign->getRealMidiTime() == startTime) {
			currentTempo_ = sign->getTempo();
			if (sign1 = orderedSigns_.next()) {
				timeOfNextTempoSig_ = sign1->getRealMidiTime();
			}
		}
		else {
			timeOfNextTempoSig_ = sign->getRealMidiTime();
		}
	}
}

int NTempoTrack::getTempoAtMidiTime(int midiTime) {
	NSign *sign, *sign1;
	if (timeOfNextTempoSig_ == -1 || midiTime < timeOfNextTempoSig_ ) return currentTempo_;
	for (sign = orderedSigns_.current(); sign; sign = orderedSigns_.next())  {
		if (sign->getRealMidiTime() >= midiTime) {
			break;
		}
	}
	if (sign) {
		currentTempo_ = sign->getTempo();
		if (sign1 = orderedSigns_.next()) {
			timeOfNextTempoSig_ = sign1->getRealMidiTime();
		}
		else {
			timeOfNextTempoSig_ = -1;
		}
	}
	else {
		timeOfNextTempoSig_ = -1;
	}
	return currentTempo_;
}
	

void NTempoTrack::insertTempoSign(NSign *sign) {
	if (orderedSigns_.contains(sign)) return;
	orderedSigns_.inSort(sign);
}

NSign *NTempoTrack::first() {
	return orderedSigns_.first();
}

NSign *NTempoTrack::next() {
	return orderedSigns_.next();
}

void NTempoTrack::clear() {
	orderedSigns_.clear();
}

void NTempoTrack::resolveRitardandoAndAccelerando() {
	NSign *sign, *newsign;
	NSign *lasttemposign = 0, *nexttemposign;
	int tempo0, midiTime0, i, idx;
	double tempodist, midiTimeDist;
	int numTempoSigns;

	sign = orderedSigns_.first();
	while (sign) {
		switch (sign->getSubType()) {
			case RITARDANDO:
			case ACCELERANDO: if (!lasttemposign) {
						orderedSigns_.remove();
						sign = orderedSigns_.current();
						break;
					  }
					  idx = orderedSigns_.at();
					  nexttemposign = orderedSigns_.next();
					  if (!nexttemposign) {
					  	orderedSigns_.remove();
						sign = orderedSigns_.current();
						break;
					  }
					  orderedSigns_.at(idx);
					  if (nexttemposign->getSubType() != TEMPO_SIGNATURE) {
					  	orderedSigns_.remove();
						sign = orderedSigns_.current();
						break;
					  }
					  numTempoSigns = (nexttemposign->getRealMidiTime() - lasttemposign->getRealMidiTime()) / (TEMPO_GRANULATITY + 1);
					  if (numTempoSigns < 3) {
					  	orderedSigns_.remove();
						sign = orderedSigns_.current();
						break;
					  }
					  tempo0 = lasttemposign->getTempo();
					  midiTime0 = lasttemposign->getRealMidiTime();
					  tempodist = (double) (nexttemposign->getTempo() - lasttemposign->getTempo()) / (double) numTempoSigns;
					  midiTimeDist = (double) (nexttemposign->getRealMidiTime() - lasttemposign->getRealMidiTime()) / (double) numTempoSigns;
					  orderedSigns_.remove();
					  sign = orderedSigns_.current();
					  for (i = 1; i < numTempoSigns; i++) {
						newsign = new NSign(0, &NResource::nullprops_, TEMPO_SIGNATURE);
						newsign->setTempo(tempo0 + (int) ((double) i * tempodist));
						newsign->setRealMidiTime(midiTime0 + ((int) ((double) i * midiTimeDist)));
						orderedSigns_.insert(idx + i - 1, newsign);
					  }
					  sign = orderedSigns_.at(idx + numTempoSigns - 1);
					  break;
			case TEMPO_SIGNATURE: lasttemposign = sign;
					 sign = orderedSigns_.next();
					 break;
			default: NResource::abort("resolveRitardandoAndAccelerando: internal error");
		}
	}
}
