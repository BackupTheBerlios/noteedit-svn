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

#ifndef VOICE_H

#define VOICE_H

#include <qregexp.h> 
#include "muselement.h"
#include "midimapper.h"
#include "chorddiagram.h"

class NMusElement;
class NMidiMapper;
class NTimeSig;
class NVoice;
class NNote;
class NMainFrameWidget;
class NRest;
class NSign;
class NChord;
class NClef;
class NKeySig;
class NChordDiagram;
class NTempoTrack;
class NText;

class NPositStr {
	public:
		int ev_type;
		int ev_time;
		NVoice *from;
		NMusElement *elem;
};


class undostr {
	public:
		QList<NMusElement> *backup_area;
		int first_replaced_item;
		int num_of_replaced_items;
		NVoice *ref;
};


#define MAXUNDO 50
#define REASON_UNDO 1
#define REASON_UNDO_DONE 2
#define REASON_REDO_DONE 3
#define TRIPLET_PART_AT_END 1
#define TRIPLET_PART_AT_BEGIN 2

#define DYN_CRESCENDO 3
#define DYN_DECRESCENDO 4

#define MIDI_EVENT_RING 16

#define UNDEFINED_DIST 11111

#define GRACE_PMX_OK      0
#define WARN_MIXED_GRACES 1
#define GRACE_PMX_ERROR   2

class chordDiagramName {
   public:
	chordDiagramName(NChordDiagram *diag, int us) {
		cdiagramm = diag; NumOfUnderscores = us;
	}
	chordDiagramName(QString cname, char *strings, bool showDiagramm) {
		cdiagramm = new NChordDiagram(showDiagramm, cname,strings);
	}
	int NumOfUnderscores;
	NChordDiagram *cdiagramm;
};

class NStaff;

class NVoice {
	public:
/* ------------- creating voice -------------------------------------------------*/
		NVoice(NStaff *staff, NMainFrameWidget *mainWidget, bool isFirstVoice);
		~NVoice();
/*---------------------- setting voice properies -----------------------------------------*/
		void paperDimensiones(int width);
		void setHalfsAccordingKeySig(bool withUndo);
		void setHalfsTo(int type, bool region);
		void emptyVoice();
		void getChordDiagramms(QList<chordDiagramName> *cdiagList, bool *gridsused, bool firstcall, bool *gridproblem);
		int getMidiEndTime() {return midiEndTime_;}
		bool isLast () {return musElementList_.current() == musElementList_.getLast();}
		bool isLastElem(NMusElement *elem) {return elem == musElementList_.getLast();}
		bool inRepeat() {return endingIdx_ >= 0;}
		bool muted_;
		int stemPolicy_;
/*------------------------ getting voice properties ---------------------------------------*/
		main_props_str *getMainPropsAddr() {return main_props_;}
		NStaff *getStaff() {return theStaff_;}
		void printAll();
		void setActualStemDir(int stemDir) {main_props_->actualStemDir = stemDir;}
		void getTempoSigs(NTempoTrack *ttrack, int startTime);
		bool isFirstVoice() {return firstVoice_;}
		void detectABCSpecials(bool *with_drums, bool *with_pedal_marks); /* for abc music export */
		int determineAnacrusis();
		int getMidiTime() const;
		int getMidiPos() const;
		int getCurrentMeasureMidiLength();
		int octave_;
		bool voiceSet_;
		int yRestOffs_;
/*--------------------------------- search for something in voice ----------------------------- */
		int checkElementForNoteInsertion(const int line, const QPoint p, int *state, int *state2, bool *playable, bool *delete_elem, bool *insertNewNote, int offs);
		bool checkElementForElementInsertion(const QPoint p);
		bool deleteActualNote();
		int getElemState(int *state, int *state2, bool *playable);
		void grabElements();
		void findAppropriateElems();
		void trimmRegion(int *x0, int *x1);
		bool trimmRegionToWholeStaff(int *x0, int *x1);
		void deleteBlock();
		void findStartElemAt(int x0, int x1);
		bool wholeTupletDeleted(NMusElement *ac_elem, int posOfFirst, int posOfLast);
		bool wholeBeamDeleted(NChord *ac_elem, int posOfFirst, int posOfLast);
		void pasteAtMidiTime(int dest_time, int part_in_measure, int countof128th, QList<NMusElement> *clipboard);
		void pasteAtPosition(int xpos, QList<NMusElement> *clipboard, bool complete, int *part_in_current_measure, int *dest_midi_time, int *countof128th);
		void pasteAtIndex(QList<NMusElement> *clipBoard, int idx);
		QList<NMusElement> *getClipBoard() {return &clipBoard_;}
		int getBarsymTimeBefore(int till_meascount, int miditime);
		int computeSlurDist(NChord *chord);
		int findTimeOfTrillEnd(NChord *chord, int *destmestime, int *mescount);
		int findTimeOfSlurEnd(NChord *chord, int *destmestime, int *mescount);
		int findTimeOfVaEnd(NChord *chord, int *destmestime, int *mescount);
		int findNextVolumeSignature();
		int findHighestLineInTrill(NChord *chord);
		int findBorderLineInVa(NChord *chord);
		int findNoteCountTillTrillEnd(NChord *chord);
		int findTimeOfDynamicEnd(NChord *chord, int *sourcemestime, int *destmestime, int *mescount);
		int findEndOfCrescendo(NChord *chord);
		bool beginsWithGrace();
/*-------------------------------- drawing voice ----------------------------------------------*/
		void draw(int left, int right, bool is_actual);
		int validateKeysig(int lastbaridx, int insertpos);
		void validateKeysigAccordingPos(int lastbarstartpos, int insertpos);
		void setCorrectClefAccordingTime(int miditime);
/*--------------------- changes due to user interaction ------------------------------------*/
		void release();
		void makeKeysigAndClefActual();
		void moveUp(int up);
		void moveDown(int down);
		void moveSemiToneUp();
		void moveSemiToneDown();
		int makePreviousElementActual(int *state, int *state2);
		int makeNextElementActual(int *state, int *state2);
		void changeActualChord();
		void changeBodyOfActualElement();
		void changeActualStem();
		void changeActualOffs(int offs);
		void setDotted();
		void setAccent(unsigned int type);
		void breakBeames();
		void breakTuplet();
		void breakCopiedTuplets();
		bool checkTuplets(QList<NMusElement> *copielist, QList<NMusElement> *tupletlist);
		bool lastChordContained(QList<NMusElement> *clonelist, QList<NChord> *beamlist);
		bool lastElemContained(QList<NMusElement> *clonelist, QList<NMusElement> *tupletlist);
		bool allElemsContained(QList<NMusElement> *clonelist, QList<NMusElement> *tupletlist);
		void setActualTied();
		void setArpeggio();
		void setPedalOn();
		void setPedalOff();
		void setSlured();
		void resetSlured();
		void setBeamed();
		void setTuplet(char numNotes, char playtime);
		int deleteActualElem(int *state, int *state2, bool backspace);
		bool deleteAtPosition(int y);
		void autoBar();
		void autoBarVoice123andSoOn();
		void autoBeam();
		void checkBeams(int indexOfLastBar, NTimeSig *tsig);
		void cleanupRests(int shortestRest, bool region);
		static int quant(int l, int *dotcount, int maxlength);
		void pubAddUndoElement();
/*------------------------- insertion due to user interaction --------------------*/
		void insertTmpElemAtPosition(int xpos, NMusElement *tmpElem);
		int findLastBarTime(int xpos);
		void insertAtPosition(int el_type, int xpos, int line, int sub_type, int offs, NMusElement *tmpElem = 0);
		bool insertNewNoteAt(int line, const QPoint p, int offs);
		bool insertNewNoteAtCurrent(int line, int offs);
		void insertAfterCurrent(int el_type, int subtype);
		bool insertAfterCurrent(NMusElement *elem);
		void insertBarAt(int xpos);
/*------------------------ playing voice -------------------------------------*/
		void startPlaying(int starttime);
		void stopPlaying();
		NMidiEventStr* getNextMidiEvent(int mtime, bool reachInfo);
		void setMarker();
		void setSegnoMarker();
		void setCodaMarker(int timeOf2ndCoda);
		void gotoMarker(bool again);
		void stopVoice() {stopped_at_fine_ = true;}
		void gotoSegnoMarker();
		void gotoCodaMarker();
		void handleEnding1();
		void skipChord();
		void skipAndInvalidate(bool doSkip = true);
		void skipTeXChord();
		void computeVolumesAndSearchFor2ndCodaSign();
		int repeatIdx_; 
		int repeatCount_;
		int endingIdx_;
		int repeatTime_;
		int segnoIdx_;
		int codaStatus_;
		int segnoTime_;
		int idxOf2ndCodaSign_;
		int timeOf2ndCoda_;
		bool stopped_at_fine_;
		int trillEndPos_, dynEndPos_, vaEndPos_;
		int vaOffset_;
		int dynamicRefVolume_, dynamicRefTime_;
		bool inVolumeCrtlMode_;
		double volIncrease_;
/*------------------------- repositioning voice --------------------------------*/
		NPositStr *getElementAfter(int mtime);
		void startRepositioning();
		void computeMidiTime(bool insertBars, bool doAutoBeam);
		int placeAt(int xpos, int sequNr);
		int findPos(int barNr);
		NKeySig *lastKeySig_;
/* --------------------------- export musixtex -------------------------------------*/
		bool testSpecialEnding(int *num);
/* -----------------------------references between voices -------------------------*/
		void resetSpecialElement();
		void syncSpecialElement(int xpos);
		NMusElement *findBarInStaff(int start_time, int stop_time);
		NMusElement *findBarInStaffTillXpos(int start_time, int endXpos);
		void mark();
		void gotoMarkedPosition();
		NMusElement *countBarSymsBetween(int firstXpos, int actualXpos, int *count_of_bar_syms);
		bool endSeen_; 
		NMusElement *checkSpecialElement(int xpos, int *volta);
/* ---------------------------- export ABC music -----------------------------------*/
		void setIdx(int idx) {u1_.tempTabIdx = idx;}
		int getIdx() { return u1_.tempTabIdx;}
/*-------------------------- writing the voice  ------------------------------------*/
		void prepareForWriting();
		int determineMultiRest();
		NClef *getFirstClef();
		NTimeSig *getFirstTimeSig();
		NKeySig *getFirstKeysig();
		QString determineGraceKind(int *status);
		NMusElement *getCurrentPosition();
		NMusElement *getNextPosition();
		NMusElement *getPrevPosition();
		NMusElement *getCurrentElement() {return currentElement_;}
		bool firstVolume_;
		bool inBeam_, inTuplet_;
/*--------------------------- import MusicXML --------------------------------*/
		NMusElement *getFirstPosition();
		NMusElement *getLastPosition();
		bool removeLastPosition();
		void correctPitchBecauseOfVa(int tstart, int tend, int sign);
		int findElemRef(const NMusElement *elem);
/*--------------------------- export MusicXML --------------------------------*/
		NChord *findLastChordBetweenXpos(int xpos1, int xpos2);
		int getVaAtXpos(int xpos);
/*--------------------------- appending due to reading ---------------------------*/
		void appendNoteAt(int line, int offs, unsigned int status);
		void appendElem(int el_type, int line, int sub_type, int offs = 0, unsigned int status = 0);
		void appendElem(NMusElement *elem);
		void correctReadTrillsSlursAndDynamicsStringsAndVAs();
		bool setProvisionalTrill(int kind, unsigned int at, unsigned int till_meascount, unsigned int till, NMusElement *last_bar_sym);
		bool setProvisionalDynamic(int kind, unsigned int at, unsigned int till_meascount, unsigned int till, NMusElement *last_bar_sym);
		bool setProvisionalSlur(unsigned int at, unsigned int till_meascount, unsigned int till, NMusElement *last_bar_sym);
		bool setProvisionalOctaviation(int kind, unsigned int at, unsigned int till_meascount, unsigned int till, NMusElement *last_bar_sym);
		void setProvisionalString(char *text, int type, unsigned int at, NMusElement *last_bar_sym);
		bool setReadArpeggio(unsigned int at, NMusElement *last_bar_sym);
		bool setReadPedalOn(unsigned int at, NMusElement *last_bar_sym);
		bool setReadPedalOff(unsigned int at, NMusElement *last_bar_sym);
		bool insertChordDiagrammAt(unsigned int at, NChordDiagram *diag, NMusElement *last_bar_sym);
		bool insertSegnoRitardAndAccelAt(unsigned int at, int type, NMusElement *last_bar_sym);
		bool buildBeam(NMusElement *elem0, NMusElement *elem1);
		bool buildTuplet(NMusElement *elem0, NMusElement *elem1, char numNotes, char playtime);
		bool buildTuplet2(NMusElement *elem0, NMusElement *elem1, char numNotes, int playlength, bool dot);
		void connectBeamsAfterReading();
		void insertAtTime(unsigned int time, NMusElement *elem, bool splitRests = false);
		void handleEndAfterMidiImport(int difftime);
		bool insertElemAtTime(unsigned int at, NSign *sign, NMusElement *last_bar_sym);
		void addLyrics(char *charlyrics, int verse);
		void copyLyricsToEditor();
		void collChords();
		QList<NText> provStrings_;
/*------------------------------ Lyrics -------------------------------------------*/
		void updateLyrics();
		int countOfLyricsLines();
		void collectLyrics(QString lyricslist[5]);
/*------------------------------- transposition -----------------------------------------*/
		void checkContext(int xpos);
		void transpose(int semitones, bool region);
/*-----------------------------  clef change ---------------------------------------------*/
		void performClefChange(int type, int shift, bool region, int *dist, int *stop_x);
/* ------------------------------- MuxiXTeX-Export (notice) ------------------------------*/
		NMusElement *getLast() {return musElementList_.getLast();}
		int beamCount_;
		int beamNr_;
	private: 
/*------------------------------ voice properies -----------------------------------------*/
		bool	firstVoice_;
		QList<NMusElement> musElementList_;
		NMusElement *currentElement_;			// the selected element (drawn in red)
								// if no element is selected, then 0
		NMusElement *specialElement_;
		NStaff  *theStaff_;
/*-------------------------------- drawing voice ----------------------------------------------*/
		void drawLines();
/*------------------------------ dealing with read trills -----------------------------*/
		
		NChord *findChordAt(NMusElement *from, int mididist);
		NChord *findChordInMeasureAt(int refpoint, NMusElement *from, int till_meascount, int mididist);
		NChord *findChordWithVAEndMarker(NChord *from); 
		NMusElement *findChordOrRestAt(NMusElement *from, int mididist);
		int findIdxOfNearestElem(NMusElement *from, int mididist);
		int findIdxOfNearestPlayableElem(NMusElement *from, int mididist);
/*-------------------- dealing with ties -----------------------------------------*/
	public:
		void findTieMember(NNote *note);		// musicxmlimport.cpp
		void reconnectFileReadTies(NNote *note);	// musicxmlimport.cpp
	private:
		void handleChordTies(NChord *chord, bool find_member);
		void reconnectTies(NNote *note);
		void breakTies(NChord *chord);
		void reconnectTiesAtferMove(NChord *chord);
		void reconnectDeletedTies(NNote *note);
		void reconnectCopiedTies(NChord *chord);
		void combineChords(int firstIdx, int lastIdx);

/*----------------------- dealing with beams -------------------------------------*/
		void reconnectBeames();
		bool beameEndRequired(QList<NChord> *beamlist_so_far, NTimeSig *timesig, int beats);
/*----------------------- dealing with tuplets -------------------------------------*/
		void reconnectTuplets();
		bool buildTupletList(int x0, int x1, char numNotes, QList<NMusElement> *elemlist);
		void tryToBuildAutoTriplet();
/*----------------------- search -------------------------------------*/
		int searchPositionAndUpdateSigns(int dest_xpos, NMusElement **elem, bool *found, NMusElement **elem_before = 0, 
			int *countof128th = 0, int *lastbaridx = 0, int *lastbarpos = 0, int *lastbartime = 0);
	public:
		void searchPositionAndUpdateTimesig(int dest_xpos, int *countof128th);
/*---------------------- saving voice properties ----------------------------------*/
	private:
		NMusElement *playPosition_;	
		int savePosition_;
		int midiEndTime_;
		NPositStr *pPtr_;
		QList<NNote> virtualChord_;
/*--------------------------- bar numbering -------------------------------------*/
		int barNr_;
/*---------------------------------- undo ---------------------------------------*/
	public:
		int getLastBarNr() {return barNr_;}
		void undo();
		void redo();
		static void resetUndo() {undocounter_ = undoptr_ = 0;}
		static NVoice *undoPossible();
		static NVoice *redoPossible();
	private:
		union {
			int tempTabIdx; /* for abcmusic index assignment */
			int pending_prog_change; /* used during replay */
		} u1_;

		QRegExp wordPattern1_, wordPattern2_, escapedApostroph_;
		main_props_str *main_props_;
		QList<NMusElement> clipBoard_;
		void eliminateRests(QList<NMusElement> *foundRests, int restSum, int over, NChord *lastChord);
		void collectAndInsertPlayable(int startTime, QList<NMusElement> *patterns, int targetLength, bool useExistingElement);
		static undostr undoelem_[MAXUNDO];
		static undostr redoelem_[MAXUNDO];
		NMidiEventStr midievents_[MIDI_EVENT_RING], *actualMidiEvent_;
		NMainFrameWidget *mainWidget_;
		NMusElement *startElement_, *endElement_;	// the selected region / marked block
		int startElemIdx_, endElementIdx_;		// and corresponding index
		static int undocounter_;
		static int undoptr_, lastundoptr_;
		static int redocounter_;
		static int redoptr_;
		void createUndoElement(NMusElement *startElement, int length, int count_of_added_items, int reason = REASON_UNDO);
		void createUndoElement(int startpos, int length, int count_of_added_items, int reason = REASON_UNDO);
		void deleteRange(int startpos, int numelements, int newitems, int reason);
		void deleteLastUndo();
		void invalidateReUndo(bool with_undo = false);
		void setCountOfAddedItems(int count_of_added_items);
		void freeCloneGroup(QList<NMusElement> *group);
		QList<NMusElement> *cloneGroup(int firstidx, int lastidx);
};

#endif // VOICE_H
