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

#ifndef MIDIEXPORT_H

#define MIDIEXPORT_H
#include <stdio.h>

class NVoice;
class NTimeSig;

class NMidiExport {
	public: 
		void exportMidi(const char *fname, QPtrList<NVoice> *voicelist, char *miditext = 0);
	private:
		void writeTrack(NVoice *voice, NTimeSig *firstTsig);
		void writePgmChange(int time, unsigned char chn, unsigned char pgm);
		void writeByte(unsigned char b);
		void writeWord(unsigned int w);
		void writeDWord(unsigned int dw);
		void writeString(char *s);
		void writeTime(int time);
		void writeNoteOn(int time, unsigned char ch, unsigned char ptch, unsigned char vel);
		void writeNoteOff(int time, unsigned char ch, unsigned char ptch, unsigned char vel);
		void writeCtlChange(int time, unsigned char chn, unsigned char ctl, unsigned char pgm);
		void writeText(int time, const char *s);
		void writeTimeSig(int time, int num, int denom);
		void writeKeySig(int time, int sig);
		void writeTempo(int time, unsigned int tempo);
		void writeCtrlTrack(QPtrList<NVoice> *voilist, char *Title, char *miditext, NTimeSig *firstTsig, int keysig);
		FILE *midiout_;
};


#endif // MIDIEXPORT_H
