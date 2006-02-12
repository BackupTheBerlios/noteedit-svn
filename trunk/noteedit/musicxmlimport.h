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
/****************************************************************************************/
/*											*/
/*		Leon Vinken, The Netherlands						*/
/*		leon.vinken@hetnet.nl							*/
/*											*/
/****************************************************************************************/

#ifndef WITH_SCONS
#include "config.h"
#endif
#ifndef MUSICXMLIMPORT_H

#include <qmap.h>
#include <qptrlist.h>
#include <qptrvector.h>
#include <qxml.h>
#include "muselement.h" /* needed for property_type */

int parseMusicXML();

class NChord;
class NChordDiagram;
class NClef;
class NStaff;
class NVoice;
class QString;
class NMainFrameWidget;
class NMusElement;

class SlurDesc
{
public:
	SlurDesc() : chord1(0), stop(0) {}
	NChord * chord1;
	bool stop;
};

typedef QMap<QString,SlurDesc> SlurDescMap;

typedef QMap<int,int> IntIntMap;

typedef QMap<int,NMusElement *> IntMelPtrMap;

class VoiceMapper
{
public:
	VoiceMapper();
	~VoiceMapper();
	void cleanup();
	int get(int staff, int mxmlVoice);
	void set(int staff, int mxmlVoice, int ntdtVoice);
private:
	IntIntMap map;
};

class MusicXMLParser;

class MusicXMLErrorHandler : public QXmlErrorHandler
{
public:
	MusicXMLErrorHandler();
	virtual ~MusicXMLErrorHandler();
	bool warning(const QXmlParseException& exception);
	bool error(const QXmlParseException& exception);
	bool fatalError(const QXmlParseException& exception);
	QString errorString();
	void setParser(MusicXMLParser * p);
private:
	bool fatalReported;
	MusicXMLParser * parser;
};

class MusicXMLParser : public QXmlDefaultHandler
{
public:
	MusicXMLParser();
	bool readStaffs(const char *fname, QPtrList<NVoice> *voilist, QPtrList<NStaff> *stafflist, NMainFrameWidget *mainWidget);
	void setDocumentLocator(QXmlLocator *locator);
	bool startDocument();
	bool startElement(const QString&, const QString&, const QString&,
	                  const QXmlAttributes&);
	bool endElement(const QString&, const QString&, const QString&);
	bool characters(const QString & ch);
	void reportError(const QString& err);
private:
	bool addNote();
	bool addStaff();
	bool addSecondStaff();
	void appendSign(int type);
	void appendText(QString textVal);
	IntMelPtrMap beamStarts;
	int cvtDivsToMidiTime(int divs);
	void fillUntil(int curtime, bool hidden, bool rpterr = true);
	void fillVoices();
	NClef *getClefAt(NStaff *staff, int miditime);
	void handleAttributes();
	void handleBarline();
	void handleClef(NStaff *staff, QString& cli, QString& coc, QString& csi);
	void handleDegree();
	void handleDegreeUpdateStep(int& s, int defval);
	void handleDirection();
	void handleDynamics();
	void handleEndOfMeasure();
	void handleFrameNote();
	void handleHarmony();
	void handleKind(QString& knd);
	void handleMultiRest();
	void handleLyrics();
	void handleMetronome();
	void handleOctavaStart(NChord * chord);
	void handleOctavaStop();
	void handlePedal(NChord * chord);
	void handleTuplet(NMusElement * elem);
	void handleVoice(int staff, int voice);
	void handleVoiceDoStaff(int staff, int voice, NStaff * & staff, bool & mapped);
	int current_max_voice_nr; /* the maximum voice number reached until now */
	void handleWords();
	void initStAttributes();
	void initStBarline();
	void initStDirect();
	void initStFrameNote();
	void initStHarmony();
	void initStNote();
	void initStScorePart();
	void insertRest(int length, bool hidden);
	int mxmlNoteType2Ne(const QString& mxmlNoteType);
	int parseMusicXML();
	NChordDiagram * pendingChordDiagram;
	void reportAll(const QString& lvl, const QString& err);
	void reportWarning(const QString& err);
	void slrhInit();
	void slrhSlurStart(const QString& id);
	void slrhSlurStop(const QString& id);
	void slrhHandleSlurs(NChord * chord);
	SlurDescMap slrhMap;
	void trlhInit();
	void trlhHandleTrills(NChord * chord);
	void trlhSetStatus(const QString& tp);
	NChord * trlhChord1;
	int trlhFirstChrdCm;
	bool trlhInTrill;
	bool trlhTrlMrk;
	void wdghInit();
	void wdghStEl(const QString& tp);
	void wdghAddChrd();
	NMusElement * wdghFirstChordPtr;
	int wdghFirstChrdCm;		// current measure first chord in wedge
	int wdghLastChrdCt;		// current time start last chord in wedge
	int wdghLastChrdCm;		// current measure last chord in wedge
	bool wdghTypeCres;		// true = cresc., false = dimin.
	bool wdghInWedge;		// true if in wedge
	int currentMeasure;
	NStaff *current_staff;
	NVoice *current_voice;
	NStaff *current_2ndstaff;
	int first_voice_2ndstaff;
	bool first_vc_1st_st_mapped;
	bool first_vc_2nd_st_mapped;
	int current_time;		// current time in the current part
	int prev_time;			// previous current time (equals tstart last note)
	int measure_start_time;		// tstart current measure
	NChord * lastChord;		// last chord inserted
	int iDiv;			// divisions
	QPtrVector<QString> partIds;	// part (staff) id's
	QXmlLocator *lctr;
	VoiceMapper vm;
	// state variables for parsing
	// attributes -- initialized in initStAttributes()
	QString stBts;				// beats
	QString stBtt;				// beat-type
	QString stCli;				// clef line
	QString stCln;				// clef number
	QString stCoc;				// clef octave change
	QString stCsi;				// clef sign
	QString stCli2;				// clef 2 line
	QString stCoc2;				// clef 2 octave change
	QString stCsi2;				// clef 2 sign
	QString stDiv;				// divisions
	QString stFif;				// fifths
	QString stMrs;				// multiple-rest
	QString stSta;				// staves
	// barline -- initialized in initStBarline()
	QString stBll;				// barline location
	QString stBst;				// bar style
	QString stRdi;				// repeat direction
	QString stEnr;				// ending nr
	QString stEtp;				// ending type
	// direction -- initialized in initStDirect()
	int     stBtd;				// beat unit dot
	QString stBtu;				// beat unit
	QString stDyn;				// dynamics
	QString stOss;				// octave shift size
	QString stOst;				// octave shift type
	QString stPdl;				// pedal type (start/stop)
	QString stPrm;				// per minute
	QString stWrd;				// words
	// general -- initialized in startDocument()
	QString stCha;				// characters collected
	// harmony -- initialized in initStHarmony()
	bool kindFound;				// valid kind found
	int s3;					// step 3
	int s5;					// step 5
	int s7;					// step 7
	int s9;					// step 9
	int s11;				// step 11
	int s13;				// step 13
	QString stBsa;				// bass alter
	QString stBss;				// bass step
	QString stDga;				// degree alter
	QString stDgt;				// degree type
	QString stDgv;				// degree value
	bool stFrm;				// frame (true if found)
	int stFrn[6];				// frame note (for six strings)
	QString stRta;				// root alter
	QString stRts;				// root step
	// identification -- initialized in startDocument()
	QString stCrt;				// creator
	QString stEnc;				// encoder
	QString stRig;				// rights
	QString stTtl;				// title
	// note (including forward/backup) -- initialized in initStNote()
	property_type properties;
	QString stAcc;				// accidental
	QString stAlt;				// alter
	QString	stAno;				// actual notes
	bool    stArp;				// arpeggiate
	QString	stBea;				// beam
	QString	stBnr;				// beam number
	bool    stCho;				// chord with previous note
	QString stDoc;				// display octave
	QString stDst;				// display step
	int     stDts;				// dots (count)
	QString stDur;				// duration
	QString stFrt;				// fret
	bool    stGls;				// glissando
	bool    stGra;				// grace
	bool    stGsl;				// grace slash
	bool    stHmr;				// hammer-on
	QString stLyn;				// lyrics number
	QString stLyr[NUM_LYRICS];		// lyrics
	QString	stNno;				// normal notes
	QString stOct;				// octave
	QString	stPlc;				// placement of arbitrary text (above, below)
	bool    stPlo;				// pull-off
	bool    stRst;				// rest
	bool	stSlr;				// slur
	QString stStf;				// staff
	QString stStm;				// stem
	QString stStp;				// step
	QString stStr;				// string
	bool    stTie;				// tie start
	bool	stTrl;				// trill-mark
	QString stTtp;				// tuplet type
	QString stTxt;				// text
	QString stTyp;				// type
	QString stVoi;				// voice
	QString stWvl;				// wavy-line (=trill)
	// part (== staff) -- initialized in initStScorePart()
	QString stPid;				// ID
//	QString stPmb;				// MIDI bank
	QString stPmc;				// MIDI channel
	QString stPmp;				// MIDI program
	QString stPnm;				// name
};

#endif /* MUSICXMLIMPORT_H */
