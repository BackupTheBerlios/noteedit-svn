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

#ifndef STAFF_H

#define STAFF_H
#include "muselement.h"
#include "voice.h"
#include "clef.h"
#include "keysig.h"
#include "clef.h"
#include <qpoint.h>

class VoiceDialog;
class staffPropFrm;

class NStaff {
	public:
		NStaff(int base, int midi_channel, int voice, NMainFrameWidget *mainWidget);
		~NStaff();
		QList<NVoice> voicelist_;
		void setBase(int base);
		void paperDimensiones(int width);
		int getBase() const {return staff_props_.base;}
		void setActual(bool actual) {staff_props_.is_actual = actual;}
		void changeVoice(int voice) {midiVoice_ = voice;}
		int getVoice() const {return midiVoice_;}
		int getVolume() const {return midiVolume_;}
		void setChannel(int channel) {midiChannel_ = channel;}
		int determineMultiRest();
		void setVolume(int vol) {midiVolume_ = vol;}
		int getChannel() const {return midiChannel_;}
		void getElementsAfter(QList<NPositStr> *plist, int mytime, int *num_positions, int *min_time);
		int getWidth() {return width_;}
		void validateKeysig(int startidx, int insertpos);
		int voiceCount() {return voicelist_.count();}
		void startRepositioning();
		void grabElements(NVoice *preferredVoice);
		void grabElementsAccording();
		void deleteBlocksAccording();
		void deleteBlock(NVoice *preferredVoice);
		void autoBeam();
		void autoBar();
		void pasteAtPosition(int xpos, NStaff *from = 0);
		int intersects(const QPoint p) const;
		int reverb_, chorus_, pan_;
		int transpose_;
		void draw(int left, int right);
		void drawContext();
		QString staffName_;
		int overlength_, underlength_;
		bool trimmRegionToWholeStaff(int *x0, int *x1);
		int checkElementForNoteInsertion(const int line, const QPoint p, status_type *status, unsigned int *status2, bool *playable, bool *delete_elem, bool *insertNewNote, int offs) {
			return actualVoice_->checkElementForNoteInsertion(line, p, status, status2, playable, delete_elem, insertNewNote, offs); }
		bool checkElementForElementInsertion(const QPoint p) {
			return actualVoice_->checkElementForElementInsertion(p); }
		int findLineOf(int pitchNumber, int acYLine, int xpos);
		staff_props_str *getStaffPropsAddr() {return &staff_props_;}
		staff_props_str staff_props_;
		NKeySig actualKeysig_;
		NClef actualClef_;
		NVoice *getVoiceNr(int nr) {return voicelist_.at(nr);}
		NVoice *getActualVoice() {return actualVoice_;}
		NVoice *changeActualVoice(int nr);
		NVoice *addNewVoice();
		NClef *pending_clef_;
		int findLastBarTime(int xpos) {return voicelist_.getFirst()->findLastBarTime(xpos);}
		void searchPositionAndUpdateTimesig(int xpos, int *countof128th) {
			return voicelist_.getFirst()->searchPositionAndUpdateTimesig(xpos, countof128th);
		}
		void setCorrectClefAccordingTime(int miditime) {
			voicelist_.getFirst()->setCorrectClefAccordingTime(miditime);
		}
		void transpose(int semitones);
		void setHalfsTo(status_type type);
		void cleanupRests(int shortestRest);
		void checkContext(int xpos) {theFirstVoice_->checkContext(xpos);}
		void resetSpecialElement() {theFirstVoice_->resetSpecialElement();}
		void syncSpecialElement(int xpos) {theFirstVoice_->syncSpecialElement(xpos);}
		void mark() {theFirstVoice_->mark();}
		void gotoMarkedPosition() {theFirstVoice_->gotoMarkedPosition();}

		NMusElement *countBarSymsBetween(int firstXpos, int actualXpos, int *count_of_bar_syms) {
			return theFirstVoice_->countBarSymsBetween(firstXpos, actualXpos, count_of_bar_syms);
		}
		NMusElement *checkSpecialElement(int xpos, int *volta = 0) {return theFirstVoice_->checkSpecialElement(xpos, volta);}
		NMusElement *findBarInStaff(int start_time, int stop_time) {return theFirstVoice_->findBarInStaff(start_time, stop_time);}
		NMusElement *findBarInStaffTillXpos(int start_time, int endXpos) {return theFirstVoice_->findBarInStaffTillXpos(start_time, endXpos);}
		void performClefChange(int type, int shift);
		void collChords();
		void setHalfsAccordingKeySig();
		void addVoices(int nvoices);
		void moveVoice(uint srcNr, uint tgtNr); /* move the voice from number src to tgt */
		void updateVoiceList(QList<NVoice> *voicelist);
		void setMuted(bool muted);
		bool getMuted() {return actualVoice_->muted_;}
		int deleteVoice(NVoice *oldVoice, VoiceDialog *voicedialog, staffPropFrm *staffPropForm);
		int getActualVoiceNr() {return actualVoiceNr_;}
		NClef *playClef_, *repeatClef_, *segnoClef_;
		NKeySig *playKeySig_, *repeatKeySig_, *segnoKeySig_;
		int timeOffset_;
		void setMarker();
		void setSegnoMarker();
		void setCodaMarker(int timeOf2ndCoda);
		void gotoMarker(bool again);
		void gotoSegnoMarker();
		void stopAllVoices();
		void gotoCodaMarker();
		void handleEnding1();
		void startPlaying(int starttime = 0);
		void correctPitchBecauseOfVa();
	private:
		QPoint labelDrawPoint_;
		NVoice *actualVoice_;
		NVoice *theFirstVoice_;
		int width_;
		int nettoWidth_;
		int midiChannel_;
		int midiVoice_;
		int midiVolume_;
		int actualVoiceNr_;
		int yTop_, yBottom_, yMid_;
		NMainFrameWidget *mainWidget_;
		main_props_str *main_props_;
};

#endif /* STAFF_H */
