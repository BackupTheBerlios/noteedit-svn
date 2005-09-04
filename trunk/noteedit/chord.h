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

#ifndef NCHORD_H

#define NCHORD_H

#include <qpixmap.h>
#include "muselement.h"
#include "voice.h"

#define NORMAL_TRILL 1
#define LONELY_TRILL 2
#define OCTAVIATION1P 3
#define OCTAVIATION1M 4
#define VA_LINE_LEN 30
#define VA_LINE_DASH_LEN (VA_LINE_LEN/4)


#define UNDEFINED_OFFS 111
#define UNDEFINED_LINE 111

struct trill_descr_str {
	int trill_nr, endpos;
};

class NChord: public NPlayable {
	public:
		NChord(main_props_str *main_props, staff_props_str *staff_props, NVoice *voice, int line, int offs, int length, int voices_stem_policy, status_type status = 0 );
		virtual ~NChord();
		virtual NChord *clone();
		virtual void changeLength(int length);
		virtual void changeBody(status_type bodyType);
		virtual void draw(int flags = 0);
		virtual void moveUp(int up, int voices_stem_policy, NKeySig *key = 0);
		virtual void moveDown(int up, int voices_stem_policy, NKeySig *key = 0);
		virtual void moveSemiToneUp(int voices_stem_policy, NClef *clef, NKeySig *ksig = 0);
		virtual void moveSemiToneDown(int voices_stem_policy, NClef *clef, NKeySig *ksig = 0);
		virtual QList<NNote> *getNoteList() {return &noteList_;}
		virtual int getSubType() const {return length_;}
		virtual int getType() const {return T_CHORD;}
		virtual bool deleteNoteAtLine(int line, int voices_stem_policy);
		virtual NNote *searchLine(int line, int min);
		virtual NNote *insertNewNote(int line, int offs, int voices_stem_policy, status_type status);
		virtual NChordDiagram *getChordChordDiagram() {return cdiagram_;}
		void determineStemDir(int voices_stem_policy);
		virtual void setDotted(int dotcount);
		virtual void changeOffs(int offs, NKeySig *actual_keysig);
		virtual void setActualTied(bool tied);
		void removeAllTies();
		void tieWith(NChord *otherChordBefore);
		int getRealStartTime (); /* pay attention to grace notes */
		void setSlured(bool slured, NChord *partner = 0);
		void resetSlurForward();
		void resetSlurBackward();
		void breakSlurConnections();
		void checkSlures();
		int getTeXSlurNr() {return auxInfo_.TeXSlurNr;}
		void setTeXSlurNr(int nr) {auxInfo_.TeXSlurNr = nr;}
		void setPartnerSlurNr (int nr) {slur_forward_->auxInfo_.TeXSlurNr = nr;}
		NNote *getActualNote();
		bool removeNote(NNote *note, int voices_stem_policy);
		NChord *getSlurPartner() { return slur_forward_;}
		NChord *getSlurStart() { return slur_backward_;}
		virtual void breakBeames();
		virtual bool lastBeamed() {return ((status_ & STAT_BEAMED) && nextBeamedChord_ == 0);}
		bool beamHasOnlyTwoChords();
		void removeFromBeam();
		bool setOctaviationStart(int size);
		bool setOctaviationStop(int size);
		virtual int intersects_horizontally(const QPoint p) const;
		virtual int getMidiLength(bool forPlayback = false) const;
		virtual void calculateDimensionsAndPixmaps();
		virtual void calculateFlagCount();
		static void computeBeames(QList<NChord> *beamList, int stemPolicy);
		bool setActualNote(int line);
		bool equalTiedChord(NChord *chord2);
		virtual void setBeamFlag() { status_ |= STAT_BEAMED; nextBeamedChord_ = (NChord *) 1;}
		void resetBeamFlag() {status_ &= (~STAT_BEAMED); nextBeamedChord_ = 0; beamList_ = 0;}
		void computeBeames(int stemPolicy);
		void setStemUp(bool stem_up);
		int getGraceMidiStartTime() { return u1_.graceNoteStartTime_;}
		void setGraceMidiStartTime(int startTime) {u1_.graceNoteStartTime_ = startTime < 0 ? 0 : startTime;}
		void transposeChordDiagram(int semitones);
		int getTrillEnd();
		int getVaEnd();
		int xposDecor_;
		int getDynamicEnd();
		virtual int getXposDecorated() {return xposDecor_;}
		virtual QPoint *getTopY();
		virtual int getTopY2();
		virtual int getTopY3();
		virtual int getTopX2();
		virtual double getBotY();
		virtual void addChordDiagram(NChordDiagram *cdiag);
		virtual void removeChordDiagram();
		void setArpeggio(bool on);
		void setPedalOn(bool on);
		void setPedalOff(bool on);
		int getRefY();
		void setLyrics(QString *lyrics, int nr);
		void deleteLyrics(int nr);
		void computeStemBefore();
		int countOfLyricsLines();
		QString *getLyrics(int nr);
		void setBeamParams(QList<NChord> *beamList, NChord *nextChord, double m, double n);
		void resetBeamFlags();
		virtual int computeMidiLength() const;
		QList<NChord> *getBeamList() { return beamList_;}
		void changeBeamList(QList<NChord> *blist) {beamList_ = blist;}
		void checkAcc();
		void accumulateAccidentals(NKeySig *key);
		virtual char getNumNotes() {return numTupNotes_;}
		virtual char getPlaytime() {return tupRealTime_;}
		virtual void setTupletParams(QList<NPlayable> *tupletList, 
			bool last, double m , double n , double tuptexn, int xstart, int xend, char numnotes, char playtime);
		QString *computeTeXBeam(int maxBeams, unsigned int *beamPool, int *beamNr, int *beamCount, NClef *clef, int maxflags, bool *problem128, bool *toomany);
		QString *computeTeXTie(unsigned int *tiePool, NClef *clef, int maxtie, bool *toomany, bool spare);
		QString *computeTeXSlur(unsigned int *slurPool, NClef *clef, int maxslur, bool *toomany);
		QString *computeTeXTrill(int hline, unsigned int *trillPool, NClef *clef, struct trill_descr_str *trill_descr, 
                                        bool *nested, bool *toomany);
		QString *computeTeXVa(bool bassa, int hline, unsigned int *vaPool, NClef *clef, struct trill_descr_str *trill_descr,
			bool *nested, bool *toomany);
		void initialize_acc_pos_computation();
		QList<NNote> *getAccTexRow(int row_nr);
		int getNumOfTexAccRows() {return numTexRows_;}
		union {
			short TeXSlurNr;
			int provSlur_; /* setProvisionalSlur */
			struct dyanmic_desrc_str {
				unsigned short volume;
				double increase;
			} dynamic_descr;
		} auxInfo_;

		int trill_;
		int dynamic_;
		int va_;
		bool dynamicAlign_;

	private:
		static void computeLineParams(QList<NChord> *beamList, double *np, double *mp);
		void drawGraceChord(int flags);
		void calculateGraceChord();
		int length_;
		NVoice* voice_; /* voice, which this chord belongs to */
		QPoint nbaseLinePoint1_;
		QPoint nbaseLinePoint2_;
		QPoint nbaseLinePoint3_;
		QPoint **lyricsPoints_;
		NChord *nextBeamedChord_;
		int *lyricsdist_;
		QPoint flag_pos_[5];
		QList<NNote> noteList_;
		double m_, n_;
		QList<NChord> *beamList_;
		QPoint tuplet0_, tuplet1_;
		QPoint tuplet00_, tuplet01_;
		QPoint tupletDigit_;
		char numTupNotes_, tupRealTime_;
		QPixmap *tupletMarker_;
		int flagCount_;
		QString **lyrics_;
		QPoint slur_start_point_up_, slur_start_point_down_;
		QPoint slur_forward_point_up_, slur_forward_point_down_;
		QPoint slur_back_point_up_, slur_back_point_down_;
		QRect stacc_point_;
		union {
			bool setAccentAboveChord_;
			int graceNoteStartTime_;
		} u1_;
		QPoint acc_point_;
		QPoint pedal_point_;
		NChord *slur_forward_, *slur_backward_;
		NChordDiagram *cdiagram_;
		QPoint cdiagramDrawPoint_;
		QPoint arpeggDrawPoint_;
		int arpeggParts_;
		int actualNote_;
		int narrow_left_, narrow_right_;
		int trilly_, vaY_;
		static int numTexRows_;
		static QList<NNote> acc_tex_row;
		static QPoint StrokeDist1_, StrokeDist2_;
};

#endif // NCHORD_H
