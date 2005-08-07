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

/*		leon.vinken@hetnet.nl							*/

// Import MusicXML


// LVIFIX: add error handling/reporting for beams

// LVIFIX: check voice naming conventions (voice 1 always the lowest voice ?)

// LVIFIX: many calls to voice->computeMidiTime(false) can probably
// be replaced by something more efficient (e.g. caching the value computed,
// or using getMidiEndTime())

// notes on cross-stave support such as:
// - voice moving between two staves (DebuMandSample, Dichterliebe01)
// - chord with parts in two staves (FaurReveSample)
// when a MusicXML voice (on a two-staff part) moves to the second staff,
// a second NoteEdit staff is allocated.
// the VoiceMapper vm maps MusicXML staff/voice pairs to NoteEdit staffs
// beams and slurs may also span staves (which seems to work for slurs,
// but not for beams)
// LVIFIX: current chord split algorithm ignores beams, tuplets etc.
// in the second staff

// LVIFIX: add support for tuplets w/o tuplet start and stop (SchbAvMaSample)

// LVIFIX: tuplets generate "currentMidiTime < cvMidiEndTime" errors

// LVIFIX: check accel/ritard handling in multi-staff parts
// (Dichterliebe01 measures 12 and 24)
// append to both staves does not seem to give the desired effect
// generalized: should signs (and if so: which) be appended to both staves ?

// LVIFIX: handle direction's placement attribute ?

// LVIFIX: dynamics (see wedge handler) and trills
// dynamic_ and trill_ should contain (till_meascount << 16) | till

// remarks on Qt's error handling (tested on Qt 2.3.1 and 3.0.5)
// - MusicXMLErrorHandler::warning(), error() and errorString() are not called
//   by Qt's parser, only fatalError() is called
// - when one of MusicXMLParser's handlers returns false, fatalError is called
//   with msg="error triggered by consumer"
// - a single error may result in many fatalError() calls
// - when fatalError() is called, the parseException contains valid columnnr,
//   linenr and message, but public and systemId are empty
// - failure to properly match start en end elements (e.g. <aaa></bbb>)
//   results in a "tag mismatch" error. To be able to report which tags
//   don't match, startElement/endElement would have to maintain a stack.

// notes on the "number" attribute (which is used with slurs, tuplets etc.):
// different numbers are needed when features overlap in MusicXML file order
// a reading program should be prepared to handle an arbitrary order.
// ref. common.dtd

// In NoteEdit, the higher (second and up) voices should only contain notes
// and/or rests.

/*

Status overview of features implemented

Accents		supported: staccato, sforzato, portato, strong pizzicato, sforzando, fermate,
			   arpeggio and pedal on/off
Accidentals	supported: sharp, flat and natural, including double sharp and flat
Bar separators	supported: simple, double, end
Beams		supported
Clefs		supported: treble, bass, soprano, alto and tenor clef, not supported: drum and drum_bass clef
Clef change	supported
Chords		supported
Chord diagrams	supported, but excluding bass
Dots		supported, including double dot
Grace notes	supported
Jumps		supported: coda, dal segno, dal segno al coda, dal segno al fine, fine, segno
Key signature	supported, including changing the signature
Lyrics		supported
Multiple voices	supported
Notes		supported: 2, 1, 1/2, 1/4, 1/8, 1/16, 1/32, 1/64, 1/128
Repeats		mostly supported: open, close, open_close, special_ending1, special_ending2
Rests		supported: 2, 1, 1/2, 1/4, 1/8, 1/16, 1/32, 1/64, 1/128,
			  (including hidden and multi-measure rests)
Score info	supported: title, composer and copyright, not supported: subject, last author and comment
Slurs		supported
Stem dir	supported: up, down
Tempo changes	supported: accelerando, ritardando and tempo signature
Ties		supported
Time signature	supported, including changing the signature
Trills		supported (implemented, but needs further debugging)
Tuplets		supported
Volume changes	supported: crescendo, diminuendo and ppp .. fff

More or less supported:
Multistaff (e.g. for piano music NoteEdit should generate one part with two combined staves)
-> mostly works, but no support for layout (a brace between both staffs should be added)

Not supported:
Program change
Staff properties / options
Layout (brace and bracket)
Note shapes: body as cross, alternative cross, cross with circle, rectangle, triangle

*/

#include "resource.h"			// must be before musicxmlimport.h
#include "musicxmlcommon.h"
#include "musicxmlimport.h"

#if GCC_MAJ_VERS > 2
#include <iostream>
#else
#include <iostream.h>
#endif
using namespace std;

#include <qstringlist.h>
#include <qtextstream.h>

#include "chord.h"
#include "chorddiagram.h"
#include "muselement.h"
#include "parsertypes.h"
#include "rest.h"
#include "staff.h"
#include "text.h"
#include "voice.h"
#include "../kguitar_excerpt/global.h"

// Class VoiceMapper

// helper: convert staff/voice number to unique key

static int sv2k(int staff, int voice)
{
	int r = 2 * voice + staff;
	return r;
}

// constructor

VoiceMapper::VoiceMapper()
{
	// nothing
}

// destructor

VoiceMapper::~VoiceMapper()
{
	// nothing
}

// total cleanup: remove all voices

void VoiceMapper::cleanup()
{
	map.clear();
}

// get the noteedit voice number for musicxml voice staff/mxmlVoice
// return -1 if not found

int VoiceMapper::get(int staff, int mxmlVoice)
{
	int k = sv2k(staff, mxmlVoice);
	IntIntMap::Iterator it;
	it = map.find(k);
	int r;
	if (it == map.end()) {
		r = -1;
	} else {
		r = *it;

	}
	return r;
}

// set the noteedit voice number for musicxml voice staff/mxmlVoice
// to ntdtVoice

void VoiceMapper::set(int staff, int mxmlVoice, int ntdtVoice)
{
	int k = sv2k(staff, mxmlVoice);
	IntIntMap::Iterator it;
	it = map.find(k);
	if (it == map.end()) {
		map.insert(k, ntdtVoice);
	}
}


// Class MusicXMLErrorHandler

MusicXMLErrorHandler::MusicXMLErrorHandler()
{
	fatalReported = false;
	parser = 0;
}

bool MusicXMLErrorHandler::warning(const QXmlParseException& exception)
{
	cerr << "MusicXMLErrorHandler::warning"
		<< " col=" << exception.columnNumber()
		<< " line=" << exception.lineNumber()
		<< " msg=" << exception.message()
		<< " pid=" << exception.publicId()
		<< " sid=" << exception.systemId()
		<< endl;
	return true;	// continue parsing
}

bool MusicXMLErrorHandler::error(const QXmlParseException& exception)
{
	cerr << "MusicXMLErrorHandler::error"
		<< " col=" << exception.columnNumber()
		<< " line=" << exception.lineNumber()
		<< " msg=" << exception.message()
		<< " pid=" << exception.publicId()
		<< " sid=" << exception.systemId()
		<< endl;
	return true;	// continue parsing
}

bool MusicXMLErrorHandler::fatalError(const QXmlParseException& exception)
{
	if (exception.message() == "error triggered by consumer") {
		// no need to report this: should already have been done
		// by MusicXMLParser's handler
		fatalReported = true;
	} else {
		if (!fatalReported) {
			if (parser) {
				parser->reportError(exception.message());
			} else {
				cerr << "MusicXMLErrorHandler::fatalError"
					<< " parser=0" << endl;
			}
			fatalReported = true;
		}
	}
	return false;	// do not continue parsing
}

QString MusicXMLErrorHandler::errorString()
{
	return "NoteEdit musicxmlimport error string";
}

void MusicXMLErrorHandler::setParser(MusicXMLParser * p)
{
	parser = p;
}

// Class MusicXMLParser

// readStaffs(): the public interface to the MusicXML parser

bool MusicXMLParser::readStaffs(const char *fname, QList<NVoice> *voilist, QList<NStaff> *stafflist, NMainFrameWidget *mainWidget) {
	int i;
	int parser_return;
	NVoice *voice_elem;
	NStaff *staff_elem;
	QList<NVoice> newVoices;
	QList<NStaff> newStaffs;
	layoutDef *layoutinfo;
	int staffCount;

	parser_params.fname = fname;
	parser_params.mainWidget = mainWidget;
	parser_params.newStaffs = &newStaffs;
	parser_params.newVoices = &newVoices;

	QString strFname(fname);
	// LVIFIX: error handling
	init_parser_variables();
	parser_return = parseMusicXML();
	cleanup_parser_variables();
	if (parser_return != 0) {
		while (!newVoices.isEmpty()) {
			newVoices.first();
			newVoices.current()->emptyVoice();
			newVoices.remove();
		}
		return false;
	}
		
	while (!voilist->isEmpty()) {
		voilist->first();
		voilist->current()->emptyVoice();
		voilist->remove();
	}
	for (voice_elem = newVoices.first(); voice_elem; voice_elem = newVoices.next()) {
		voilist->append(voice_elem);
	}
	stafflist->clear();
	for (staff_elem = newStaffs.first(); staff_elem; staff_elem = newStaffs.next()) {
		stafflist->append(staff_elem);
	}
	mainWidget->scTitle_ = parser_params.scTitle_;
	mainWidget->scSubtitle_ = parser_params.scSubtitle_;
	mainWidget->scAuthor_ = parser_params.scAuthor_;
	mainWidget->scLastAuthor_ = parser_params.scLastAuthor_;
	mainWidget->scCopyright_ = parser_params.scCopyright_;
	mainWidget->scComment_ = parser_params.scComment_;
	mainWidget->setParamsEnabled(parser_params.enableParams);
	mainWidget->setSaveWidth(parser_params.paperwidth);
	mainWidget->setSaveHeight(parser_params.paperheight);
	mainWidget->setWithMeasureNums(parser_params.with_measnum);
	delete mainWidget->braceMatrix_;
	delete mainWidget->bracketMatrix_;
	delete mainWidget->barCont_;
	staffCount = stafflist->count();
	mainWidget->braceMatrix_ = new layoutDef[staffCount];
	mainWidget->bracketMatrix_ = new layoutDef[staffCount];
	mainWidget->barCont_ = new layoutDef[staffCount];
	i = 0;
	for (layoutinfo = parser_params.bracketList.first(); i < staffCount && layoutinfo; layoutinfo = parser_params.bracketList.next()) {
		mainWidget->bracketMatrix_[i++] = *layoutinfo;
	}
	i = 0;
	for (layoutinfo = parser_params.braceList.first(); i < staffCount && layoutinfo; layoutinfo = parser_params.braceList.next()) {
		mainWidget->braceMatrix_[i++] = *layoutinfo;
	}
	i = 0;
	for (layoutinfo = parser_params.contList.first(); i < staffCount && layoutinfo; layoutinfo = parser_params.contList.next()) {
		mainWidget->barCont_[i++] = *layoutinfo;
	}
	return true;
}


// helpers for the parser

// report all (fatal and non-fatal) errors
// LVIFIX: in future, might use something like NResource::printWarning(fullErr)

void MusicXMLParser::reportAll(const QString& lvl, const QString& err)
{
	QString filename(parser_params.fname);
	QString fullErr;
	QString linenr;
	linenr.setNum(lctr->lineNumber());
	fullErr  = "";
	fullErr += lvl;
	fullErr += ": In ";
	fullErr += filename;
	fullErr += " line ";
	fullErr += linenr;
	fullErr += ": ";
	fullErr += err;
	fullErr += "\n";
	cerr << fullErr;
}


// report a warning (non-fatal error, i.e. one which allows parsing to continue)

void MusicXMLParser::reportWarning(const QString& err)
{
	reportAll("Warning", err);
}


// report a fatal error

void MusicXMLParser::reportError(const QString& err)
{
	reportAll("Error", err);
}


// convert MusicXML note type to NoteEdit notelength

int MusicXMLParser::mxmlNoteType2Ne(const QString& mxmlNoteType)
{
	if (mxmlNoteType == "breve") {
		return DOUBLE_WHOLE_LENGTH;
	} else if (mxmlNoteType == "whole") {
		return WHOLE_LENGTH;
	} else if (mxmlNoteType == "half") {
		return HALF_LENGTH;
	} else if (mxmlNoteType == "quarter") {
		return QUARTER_LENGTH;
	} else if (mxmlNoteType == "eighth") {
		return NOTE8_LENGTH;
	} else if (mxmlNoteType == "16th") {
		return NOTE16_LENGTH;
	} else if (mxmlNoteType == "32nd") {
		return NOTE32_LENGTH;
	} else if (mxmlNoteType == "64th") {
		return NOTE64_LENGTH;
	} else if (mxmlNoteType == "128th") {
		return NOTE128_LENGTH;
	} else {
		return 0;
	}
}


// the actual parser itself

// constructor

MusicXMLParser::MusicXMLParser()
	: QXmlDefaultHandler()
{
	current_staff = 0;
	current_voice = 0;
}


// parseMusicXML(): start parsing
//
// in: parser_params.fname contains the name of the file to be parsed
//
// out:
// on succes: returns 0
// at least one staff and one voice are created
// on failure: returns non-zero
// no staffs are allocated (voices are cleaned up by the caller)

int MusicXMLParser::parseMusicXML() {
	QString strFname(parser_params.fname);
	cout << "parseMusicXML reading XML file '" << strFname << "'" << endl;
	// LVIFIX: error handling (opening/reading file etc)
	MusicXMLErrorHandler errHndlr;
/*
// works, but only for utf-8
	QFile xmlFile(strFname);
	QXmlInputSource source(xmlFile);
*/
/**/
// the manual suggests this would work for both utf-8 and -16 but instead
// utf-16 files generate errors: "line 1: error while parsing element"
// changing the encoding to Unicode does not help: utf-8 works, utf-16 doesn't

// See Qt's source code: src/xml/qxml.cpp, QXmlInputSource::readInput()
// tries to detect utf-8 or -16 itself, which overrules xmlFile's encoding
// heavy changes for Qt3, suggest that might work

	QFile xmlFile(strFname);
	xmlFile.open(IO_ReadOnly);
	QTextStream stream(&xmlFile);
	stream.setEncoding(QTextStream::UnicodeUTF8);
//	stream.setEncoding(QTextStream::Unicode);
	QXmlInputSource source(stream);
/**/
	QXmlSimpleReader reader;
	reader.setContentHandler(this);
	reader.setErrorHandler(&errHndlr);
	errHndlr.setParser(this);
	cout << "parseMusicXML parsing XML file ..." << endl;
	// LVIFIX: reader.parse returns OK if the input file doesn't exist
	bool res = reader.parse(source);
	xmlFile.close();
	cout << "parseMusicXML done, result=";
	if (res) {
		cout << "OK" << endl;
		return 0;
	}
	cout << "error" << endl;
	return 1;
}


// start of document handler

bool MusicXMLParser::startDocument()
{
	// LVIFIX: clearing copyright should be done by init_parser_variables
	parser_params.scCopyright_ = "";
	// init global variables: attributes, LVIFIX default to 4/4
	iDiv = 0;
	// init global variables: clear part list
	partIds.clear();
	// init global variables: characters collected
	stCha = "";
	// init global variables: identification
	stCrt = "";
	stEnc = "";
	stRig = "";
	stTtl = "";
	return true;
}

// set document locator handler

void MusicXMLParser::setDocumentLocator(QXmlLocator *locator)
{
	lctr = locator;
}

// start of element handler

// Note: on reading the following input
//
// <score-part id="P1">
//   <part-name></part-name>
//     <score-instrument id="P1-I1">
//       <instrument-name>Voice</instrument-name>
//
// the parser calls
//
// startElement("score-part")
// characters("\n")
// characters("  ")
// startElement("part-name")
// endElement("part-name")
// characters("\n")
// characters("    ")
// startElement("score-instrument")
// characters("\n")
// characters("      ")
// startElement("instrument-name")
// characters("Voice")
// endElement("instrument-name")
//
// As characters() is not called between startElement("part-name") and
// endElement("part-name"), stCha needs to be cleared at each startElement().
// Failing to do so results in reading (previous) whitespace for empty
// elements such as the part-name in this example.

bool MusicXMLParser::startElement( const QString&, const QString&, 
                                   const QString& qName, 
                                   const QXmlAttributes& attributes)
{
	QString Str;
	stCha = "";		// see note above
	if (false) {
	} else if (qName == "attributes") {
    		// re-init attribute specific variables
		initStAttributes();
	} else if (qName == "barline") {
    		// re-init barline specific variables
		initStBarline();
		stBll = attributes.value("location");
	} else if (qName == "beam") {
    		// store the beam number
		stBnr = attributes.value("number");
	} else if (qName == "clef") {
    		// store the clef number
		stCln = attributes.value("number");
	} else if (qName == "direction") {
		stPlc = attributes.value("placement");
		initStDirect();
	} else if (qName == "ending") {
		stEnr = attributes.value("number");
		stEtp = attributes.value("type");
	} else if (qName == "frame") {
		stFrm = true;
	} else if (qName == "frame-note") {
		initStFrameNote();
	} else if (qName == "grace") {
		stGsl = (attributes.value("slash") == "yes");
	} else if (qName == "harmony") {
		initStHarmony();
	} else if (qName == "lyric") {
    		// store the lyric number
		stLyn = attributes.value("number");
	} else if (qName == "measure") {
		currentMeasure++;
		measure_start_time = current_time;
		// special case: prevent stPdl from being accessed by addNote()
		// without a preceding initStDirect() call has been executed
		// same goes for stOss and stOst
		stPdl = "";
		stOss = "";
		stOst = "";
	} else if (qName == "note") {
    		// re-init note specific variables
		initStNote();
		// dynamics can occur both inside direction and note,
		// stDyn must be initialized here too
		// pedal is a direction too, but is handled by the next note,
		// thus initStDirect() should not be called here
		stDyn = "";
	} else if (qName == "octave-shift") {
		stOss = attributes.value("size");
		stOst = attributes.value("type");
	} else if (qName == "part") {
		// start of staff data found
		// use part id to switch to correct staff
		QString id = attributes.value("id");
		int index = -1;
		for (unsigned int i = 0; i < partIds.size(); i++) {
			if (id.compare(*partIds.at(i)) == 0) {
				index = i;
			}
		}
		if (index == -1) {
			// part id not found, which is a fatal error
			Str = "<part> id not found: " + id;
			reportError(Str);
			return false;
		}
		// init vars for staff reading
		current_staff = parser_params.newStaffs->at(index);
		current_2ndstaff = NULL;
		current_voice = current_staff->getVoiceNr(0);
		first_voice_2ndstaff = 0;
		currentMeasure = 0;
		current_time = 0;
		prev_time = 0;
		lastChord = NULL;
		first_vc_1st_st_mapped = false;
		first_vc_2nd_st_mapped = false;
		pendingChordDiagram = 0;
		beamStarts.clear();
		vm.cleanup();
		slrhInit();
		trlhInit();
		wdghInit();
	} else if (qName == "pedal") {
		stPdl = attributes.value("type");
	} else if (qName == "repeat") {
		stRdi = attributes.value("direction");
	} else if (qName == "score-part") {
		// start of staff definition found
		// re-init score part specific variables
		initStScorePart();
		stPid = attributes.value("id");
	} else if (qName == "slur") {
		QString nr = attributes.value("number");
		QString tp = attributes.value("type");
		if (tp == "start") {
			stSlr = true;
			slrhSlurStart(nr);
		} else if (tp == "stop") {
			stSlr = true;
			slrhSlurStop(nr);
		} else {
			Str = "ignoring unknown <slur> type: " + tp;
			reportWarning(Str);
		}
	// note: in MusicXML tie is for sound and tied for notation
	// Noteedit does not make that distinction but responds
	// to either one
	} else if ((qName == "tie") || (qName == "tied")) {
		QString tp = attributes.value("type");
		if (tp == "start") {
			stTie = true;
		}
	} else if (qName == "tuplet") {
    		// store the tuplet type
		stTtp = attributes.value("type");
	} else if (qName == "wedge") {
    		wdghStEl(attributes.value("type"));
	} else if (qName == "wavy-line") {
		QString tp = attributes.value("type");
		trlhSetStatus(tp);
	} else {
		// others (silently) ignored
	}
	return TRUE;
}


// end of element handler

bool MusicXMLParser::endElement( const QString&, const QString&,
                                  const QString& qName)
{
	QString Str;
	if (false) {
	} else if (qName == "accent") {
		status |= STAT_SFZND;
	} else if (qName == "accidental") {
		stAcc = stCha;
	} else if (qName == "actual-notes") {
		stAno = stCha;
	} else if (qName == "alter") {
		stAlt = stCha;
	} else if (qName == "arpeggiate") {
		stArp = true;
	} else if (qName == "attributes") {
		handleAttributes();
		initStAttributes();
	} else if (qName == "backup") {
		// LVIFIX: error handling ?
		current_time -= cvtDivsToMidiTime(stDur.toInt());
	} else if (qName == "bar-style") {
		stBst = stCha;
	} else if (qName == "barline") {
		handleBarline();
//	} else if (qName == "bass-alter") {
//		stBsa = stCha;
//	} else if (qName == "bass-step") {
//		stBss = stCha;
	} else if (qName == "beam") {
		// keep state only for beam level 1
		if (stBnr == "1") {
			stBea = stCha;
		}
	} else if (qName == "beats") {
		stBts = stCha;
	} else if (qName == "beat-type") {
		stBtt = stCha;
	} else if (qName == "beat-unit") {
		stBtu = stCha;
	} else if (qName == "beat-unit-dot") {
		stBtd++;
	} else if (qName == "chord") {
		stCho = TRUE;
	} else if (qName == "clef-octave-change") {
		if (stCln == "2") { stCoc2 = stCha; } else { stCoc = stCha; }
	} else if (qName == "coda") {
		appendSign(CODA);
	} else if (qName == "creator") {
		stCrt = stCha;
	} else if (qName == "degree") {
		handleDegree();
	} else if (qName == "degree-alter") {
		stDga = stCha;
	} else if (qName == "degree-type") {
		stDgt = stCha;
	} else if (qName == "degree-value") {
		stDgv = stCha;
	} else if (qName == "direction") {
		handleDirection();
	} else if (qName == "display-octave") {
		stDoc = stCha;		// LVIFIX: handle this for real
	} else if (qName == "display-step") {
		stDst = stCha;		// LVIFIX: handle this for real
	} else if (qName == "divisions") {
		stDiv = stCha;
		iDiv = stDiv.toInt();
		if (iDiv <= 0) {
			Str = "illegal <divisions> value: " + stDiv;
			reportError(Str);
		}
	} else if (qName == "dot") {
		stDts++;
	} else if (qName == "duration") {
		stDur = stCha;
	} else if (qName == "encoder") {
		stEnc = stCha;
	} else if (qName == "f") {
		stDyn = "f";
	} else if (qName == "fermata") {
		// NoteEdit does not distinguish between upright and inverted
		status |= STAT_FERMT;
	} else if (qName == "ff") {
		stDyn = "ff";
	} else if (qName == "fff") {
		stDyn = "fff";
	} else if (qName == "fifths") {
		stFif = stCha;
	} else if (qName == "forward") {
		// LVIFIX: error handling ?
		current_time += cvtDivsToMidiTime(stDur.toInt());
	} else if (qName == "frame-note") {
		handleFrameNote();
	} else if (qName == "fret") {
		stFrt = stCha;
	} else if (qName == "grace") {
		stGra = true;
	} else if (qName == "harmony") {
		handleHarmony();
	} else if (qName == "identification") {
		// LVIFIX: remove_newlines necessary ?
		parser_params.scAuthor_    = stCrt;
		parser_params.scCopyright_ = stRig;
		parser_params.scTitle_     = stTtl;
	} else if (qName == "kind") {
		handleKind(stCha);
	} else if (qName == "line") {
		if (stCln == "2") { stCli2 = stCha; } else { stCli = stCha; }
	} else if (qName == "measure") {
		handleEndOfMeasure();
//	} else if (qName == "midi-bank") {
//		stPmb = stCha;
	} else if (qName == "mf") {
		stDyn = "mf";
	} else if (qName == "midi-channel") {
		stPmc = stCha;
	} else if (qName == "midi-program") {
		stPmp = stCha;
	} else if (qName == "mp") {
		stDyn = "mp";
	} else if (qName == "multiple-rest") {
		stMrs = stCha;
	} else if (qName == "normal-notes") {
		stNno = stCha;
	} else if (qName == "note") {
		return addNote();
	} else if (qName == "octave") {
		stOct = stCha;
	} else if (qName == "p") {
		stDyn = "p";
	} else if (qName == "part") {
		// end of part: check for unterminated wedges
		if (wdghInWedge) {
			reportError("wedge start without stop");
		}
		// and fixup octave-shifts
		current_staff->correctPitchBecauseOfVa();
	} else if (qName == "per-minute") {
		stPrm = stCha;
	} else if (qName == "pp") {
		stDyn = "pp";
	} else if (qName == "ppp") {
		stDyn = "ppp";
	} else if (qName == "part-name") {
		stPnm = stCha;
	} else if (qName == "rest") {
		stRst = true;
	} else if (qName == "rights") {
		stRig = stCha;
	} else if (qName == "root-alter") {
		stRta = stCha;
	} else if (qName == "root-step") {
		stRts = stCha;
	} else if (qName == "score-part") {
		bool res = addStaff();
		// re-init score part specific variables
		initStScorePart();
		return res;
	} else if (qName == "score-timewise") {
			reportError("not supported: score-timewise");
			return false;
	} else if (qName == "segno") {
		appendSign(SEGNO);
	} else if (qName == "sign") {
		if (stCln == "2") { stCsi2 = stCha; } else { stCsi = stCha; }
	} else if (qName == "staccatissimo") {
		status |= STAT_STPIZ;
	} else if (qName == "staccato") {
		status |= STAT_STACC;
	} else if (qName == "staves") {
		stSta = stCha;
		if (stSta == "0") {
			// found only in combination with print new-system=yes
			// when some measures of a part are not to be printed
			// not supported by NoteEdit, ignore
		} else if (stSta == "1") {
			// OK, do nothing
		} else if (stSta == "2") {
			addSecondStaff();
		} else {
			Str = "illegal <staves> value: " + stSta;
			reportError(Str);
			return false;
		}
	} else if (qName == "staff") {
		stStf = stCha;
	} else if (qName == "stem") {
		stStm = stCha;
	} else if (qName == "step") {
		stStp = stCha;
	} else if (qName == "string") {
		stStr = stCha;
	} else if (qName == "strong-accent") {
		status |= STAT_SFORZ;
	} else if (qName == "tenuto") {
		status |= STAT_PORTA;
	} else if (qName == "text") {
		stTxt = stCha;
		handleLyrics();
	} else if (qName == "trill-mark") {
		stTrl = true;
	} else if (qName == "type") {
		stTyp = stCha;
	} else if (qName == "voice") {
		stVoi = stCha;
	} else if (qName == "words") {
		stWrd = stCha;
	} else if (qName == "work-title") {
		stTtl = stCha;
	// following elements are explicitly ignored, usually because sub-
	// and superelements handle all the work, sometimes because features
	// are not supported by NoteEdit and sometimes they are
	// simply not necessary
	} else if (
			   qName == "accidental-mark"	// not supported by NE
			|| qName == "articulations"
			|| qName == "bass"
			|| qName == "bracket"		// not supported by NE
			|| qName == "capo"		// not supported by NE
			|| qName == "chromatic"		// not supported by importer
			|| qName == "clef"
			|| qName == "cue"		// not supported by NE
			|| qName == "dashes"		// not supported by NE
			|| qName == "degree"
			|| qName == "delayed-turn"
			|| qName == "diatonic"		// not supported by importer
			|| qName == "direction-type"
			|| qName == "dynamics"
			|| qName == "encoding"		// not supported by NE
			|| qName == "encoding-date"	// not supported by NE
			|| qName == "encoding-description"	// LVIFIX: use for comment ?
			|| qName == "end-line"
			|| qName == "end-paragraph"
			|| qName == "ending"
			|| qName == "extend"
			|| qName == "first-fret"
			|| qName == "frame"
			|| qName == "frame-frets"
			|| qName == "frame-strings"
			|| qName == "fret"		// not supported by NE
			|| qName == "group-symbol"	// not supported by importer
			|| qName == "instrument-name"
			|| qName == "instrument"	// not supported by NE
			|| qName == "instruments"	// not supported by NE
			|| qName == "key"
			|| qName == "lyric"
			|| qName == "measure-style"
			|| qName == "metronome"
			|| qName == "midi-instrument"
			|| qName == "mode"
			|| qName == "movement-number"	// not supported by NE
			|| qName == "movement-title"	// not supported by NE
			|| qName == "normal-type"	// not used
			|| qName == "notations"
			|| qName == "notehead"		// not supported by NE
			|| qName == "octave-shift"
			|| qName == "offset"
			|| qName == "ornaments"
			|| qName == "part-list"
			|| qName == "part"
			|| qName == "part-group"	// not supported by importer
			|| qName == "pedal"
			|| qName == "pitch"
			|| qName == "print"
			|| qName == "pull-off"		// not supported by NE, LVIFIX: replace by slur ?
			|| qName == "repeat"
			|| qName == "root"
			|| qName == "score-instrument"
			|| qName == "score-partwise"
			|| qName == "slur"
			|| qName == "sound"
			|| qName == "software"
			|| qName == "staff-details"	// not supported by NE
			|| qName == "staff-lines"	// not supported by NE
			|| qName == "staff-tuning"	// not supported by NE
			|| qName == "string"		// not supported by NE
			|| qName == "syllabic"
			|| qName == "technical" 	// not supported by NE
			|| qName == "tie"
			|| qName == "tied"
			|| qName == "time"
			|| qName == "time-modification"
			|| qName == "transpose"		// not supported by importer
			|| qName == "tuning-octave"	// not supported by NE
			|| qName == "tuning-step"	// not supported by NE
			|| qName == "tuplet"
			|| qName == "turn"		// not supported by NE
			|| qName == "wavy-line"
			|| qName == "wedge"
			|| qName == "work"		// not supported by NE
			|| qName == "work-number"	// not supported by NE
//			|| qName == ""
		  ) {
	} else {
		// LVIFIX: define what to do, e.g. silently ignore unknown
		// elements, or report these as warning
		// for the time being, the last option is chosen
		Str = "skipping <" + qName + ">";
		reportWarning(Str);
	}
	return TRUE;
}


// character(s) handler

bool MusicXMLParser::characters(const QString& ch)
{
	stCha = ch;
	return TRUE;
}


// add a note to the current track
// LVIFIX: error handling: int conversion, limits
// LVIFIX: compare current_time with amount of data in the voice
// if current_time is smaller, report error
// if equal, do nothing
// if larger, insert rests

bool MusicXMLParser::addNote()
{
	// first handle a possible volume change
	handleDynamics();
	// then the actual note
	int dur;
	int line = 0;
	int length = mxmlNoteType2Ne(stTyp);
	int offs = 0;
	bool res;
	QString Str;
	if ((stTyp != "") && (length == 0)) {
		Str = "illegal <type> value: " + stTyp;
		reportWarning(Str);
	}
	if (stGra) {
		dur = 0;
		if (stTyp == "eighth") {
			status |= STAT_GRACE;
			if (stGsl) {
				length = INTERNAL_MARKER_OF_STROKEN_GRACE;
			}
		} else if ( (stTyp == "16th") || (stTyp == "32nd") ) {
			/* FIXME: Only 8th and 16th grace notes are supported by NoteEdit. If shorter found, set it to 16th */
			stTyp = "16th";
			length = mxmlNoteType2Ne(stTyp);
			status |= STAT_GRACE;
		} else {
			Str = "illegal grace note <type>: " + stTyp;
			reportWarning(Str);
		}
	} else {
		dur = stDur.toInt(&res);
		if (!res || (dur < 0)) {
			dur = 0;		// sensible default (?)
			Str = "illegal <duration> value: " + stDur;
			reportWarning(Str);
		}
	}
	if (stTie) {
		status |= STAT_TIED;
	}
	if (stDts == 0) {
		/* do nothing status = 0 */ ;
	} else if (stDts == 1) {
		status |= STAT_SINGLE_DOT;
	} else if (stDts == 2) {
		status |= STAT_DOUBLE_DOT;
	} else {
		Str.setNum(stDts);
		Str = "illegal number of dots: " + Str;
		reportWarning(Str);
	}
	/* J.Anders 12-23-2003 */
	if (stAlt == "1") {
		offs = 1; 
	}
	else if (stAlt == "-1") {
		offs = -1;
	}
	else if (stAlt == "2") {
		offs = 2;
	}
	else if (stAlt == "-2") {
		offs = -2;
	}
	else if (stAlt != "" && stAlt != "0") {
		Str = "illegal <alter> value: " + stAlt;
		reportWarning(Str);
	}
	if (stAcc == "") {
		if (offs == 0) {
			offs = UNDEFINED_OFFS;
		}
	/* End of J.Anders 12-23-2003 */
	} else if (stAcc == "flat-flat") {
		offs = -2; status |= STAT_FORCE;
	} else if (stAcc == "flat") {
		offs = -1; status |= STAT_FORCE;
	} else if (stAcc == "natural") {
		offs =  0; status |= STAT_FORCE;
	} else if (stAcc == "sharp") {
		offs =  1; status |= STAT_FORCE;
	} else if ((stAcc == "double-sharp") || (stAcc == "sharp-sharp")) {
		offs =  2; status |= STAT_FORCE;
	} else {
		Str = "illegal <accidental> value: " + stAcc;
		reportWarning(Str);
	}
		
	int stemPol = STEM_POL_INDIVIDUAL;
	if (stStm == "") {
		// ignore
	} else if (stStm == "up") {
		stemPol = STEM_POL_UP;
	} else if (stStm == "down") {
		stemPol = STEM_POL_DOWN;
	} else if (stStm == "none") {
		// ignore, not supported by NoteEdit
	} else {
		Str = "illegal <stem> dir: " + stStm;
		reportWarning(Str);
	}
	NStaff *staff = NULL;
	if ((stStf == "") || (stStf == "1")) {
		staff = current_staff;
	} else if (stStf == "2") {
		staff = current_2ndstaff;
	} else {
		Str = "illegal <staff>: " + stStf;
		reportError(Str);
	}
	if (stVoi == "") {
		// default to voice 1
		stVoi = "1";
	}

	handleVoice(stStf.toInt(), stVoi.toInt());
	
	// if necessary, fill the voice with hidden rests until the given time
	// is reached (which effectively also converts all <forward>'s into
	// hidden rests).
	// Note: to write visible rests in the first voice only, set
	// hidden = !current_voice->isFirstVoice()
	bool hidden = true;
	if (stCho) {
		// fill in case of (possible) cross-staff chord:
		// need to fill until prev_time instead of current_time
		// and suppress errors
		bool rpterr = false;
		fillUntil(prev_time, hidden, rpterr);
	} else {
		fillUntil(current_time, hidden);
	}
	// LVIFIX: beam error handling is essentially missing
	bool beamStart = ((stBea == "begin"));
	bool beamStop  = ((stBea == "end"));
	if (stRst) {
		if (stTyp == "") {
			// <type> missing, insert rest(s) based on duration
			insertRest(cvtDivsToMidiTime(dur), false);
		} else {
			NRest * rest = new NRest(&(parser_params.mainWidget->main_props_),
					 &(staff->staff_props_),
					 &(current_voice->yRestOffs_),
					 length,
					 status);
			current_voice->appendElem(rest);
			handleTuplet(rest);
			prev_time = current_time;
			current_time += cvtDivsToMidiTime(dur);
		}
	} else {
		NClef *clef = getClefAt(staff, current_time);
		line = clef->name2Line(*(stStp.lower().latin1()));
		line += 7 * (stOct.toInt() - clef->getOctave());
		// tenor clef needs one octave correction,
		// apparently caused by name2line
		if (clef->getSubType() == TENOR_CLEF) {
			line += 7;
		}
		if (stCho) {
			// partial fix for chords having notes in two staves
			// notes with <chord/> should start at prev_time
			// if midiEndTime(current_voice) > prev_time
			//   attach to last note
			// else
			//   fill with hidden rests until prev_time
			//   add new note
			int met = current_voice->getMidiEndTime();
			if (met > prev_time) {
				// apparently chord already exists
				// -> insert into existing chord
				NMusElement *elem = current_voice->getLast();
				if (elem && (elem->getType() == T_CHORD)) {
					NChord * chrd = (NChord *) elem;
					NNote * note = chrd->insertNewNote(line,
								   offs,
								   stemPol,
								   status);
					// handle ties, for first note done
					// by NVoice::appendElem. The .not
					// parser first builds the chord and
					// then calls appendElem, which handles
					// ties for all notes
					if (note) {
						current_voice->reconnectFileReadTies(note);
						if (status & STAT_TIED) {
							current_voice->findTieMember(note);
						}
					}
				} else {
					Str = "<chord/> without preceding note";
					reportWarning(Str);
				}
			} else {
				// apparently chord does not already exist
				// -> append new chord
				NChord * chrd = new NChord(&(parser_params.mainWidget->main_props_),
							   &(staff->staff_props_),
						           line,
							   offs,
							   length,
							   stemPol,
							   status);
				current_voice->appendElem(chrd);
				lastChord = chrd;
			}
		} else {
			NChord * chrd = new NChord(&(parser_params.mainWidget->main_props_),
						   &(staff->staff_props_),
					           line,
						   offs,
						   length,
						   stemPol,
						   status);
			current_voice->appendElem(chrd);
			lastChord = chrd;
			// LVIFIX: this only handles chord diagram at notes
			// perhaps use insertChordDiagrammAt(...) ?
			if (pendingChordDiagram) {
				chrd->addChordDiagram(pendingChordDiagram);
				pendingChordDiagram = 0;
			}
			wdghAddChrd();	// before changing current time
			trlhHandleTrills(chrd);	// same
			prev_time = current_time;
			current_time += cvtDivsToMidiTime(dur);
			int k = sv2k(stStf.toInt(), stVoi.toInt());
			if (beamStart) {
				beamStarts[k] = chrd;
			}
			if (beamStop) {
				if (beamStarts[k]) {
					if (!current_voice->buildBeam(
							beamStarts[k], chrd)) {
						Str = "could not build beam";
						reportWarning(Str);
					}
				} else {
					Str = "beam stop without beam start";
					reportWarning(Str);
				}
				beamStarts[k] = 0;
			}
			if (stSlr) {
				slrhHandleSlurs(chrd);
			}
			for (int i = 0; i < NUM_LYRICS; i++) {
				if (stLyr[i] != "") {
					 chrd->setLyrics(&stLyr[i], i);
				}
			}
			// note: MusicXML adds tuplet info to each note of a
			// chord, while in NoteEdit tuplet info is associated 
			// with the chord itself
			handleTuplet(chrd);
			if (stArp) {
				chrd->setArpeggio(true);
			}
			handlePedal(chrd);
			handleOctavaStart(chrd);
		}
	}
	// re-init note specific variables
	initStNote();
	return TRUE;
}


// add a staff to the current song

bool MusicXMLParser::addStaff()
{
	bool res;
	QString Str;
	int i = parser_params.newStaffs->count();
	// new staff found, append it
	// create staff and first voice
	parser_params.newStaffs->append(
		current_staff = new NStaff(i*(NResource::overlength_+STAFF_HIGHT
					+NResource::underlength_)
					+NResource::overlength_,
					i, 0, parser_params.mainWidget));
	parser_params.newVoices->append(current_staff->getVoiceNr(0));
	// remember part id to staff nr mapping by storing the part id's
	QString *sp = new QString(stPid);
	int sz = partIds.size();
	partIds.resize(sz+1);
	partIds.insert(sz, sp);
	// set staff name
	current_staff->staffName_ = stPnm;	// LVIFIX: check UTF-8 correct
	// set staff's MIDI channel
	if (stPmc == "") {
		// note that size is the partIds size before insertion
		// thus the first channel used will be channel 0
		current_staff->setChannel(sz);
	} else {
		int channel = stPmc.toInt(&res);
		if (res) {
			if (channel < 1 || channel > 16) {
				res = false;
			} else {
				current_staff->setChannel(channel - 1);
			}
		}
		if (!res) {
			Str = "bad channel: " + stPmc;
			reportWarning(Str);
		}
	}

	// LVIFIX: setting current_voice should probably (also ?) be done
	// in something like select_voice_and_staff(...)
	current_voice = parser_params.newVoices->first();

	// set staff's MIDI program
	if (stPmp == "") {
		current_staff->changeVoice(0);
		current_voice->voiceSet_ = true;
	} else {
		int program = stPmp.toInt(&res);
		if (res) {
			if (program < 1 || program > 128) {
				res = false;
			} else {
				current_staff->changeVoice(program - 1);
				current_voice->voiceSet_ = true;
			}
		}
		if (!res) {
			Str = "bad voice: " + stPmp;
			reportWarning(Str);
		}
	}

	return TRUE;
}


// add a second staff to the current one
// this implies:
// - adding a new staff to parser_params.newStaffs after the current staff
//   with relevant parameters copied from the current staff
// - adding "" to partIds at the corresponding position
// - initializing variables for the second staff
//   may need to be done at start of part

bool MusicXMLParser::addSecondStaff()
{
	// add a new staff to parser_params.newStaffs after the current staff
	int i = parser_params.newStaffs->at() + 1;
	parser_params.newStaffs->insert(i,
		current_2ndstaff = new NStaff(i*(NResource::overlength_+STAFF_HIGHT
					+NResource::underlength_)
					+NResource::overlength_,
					i, 0, parser_params.mainWidget));
	parser_params.newVoices->append(current_2ndstaff->getVoiceNr(0));
	// remember part id to staff nr mapping by storing the part id's
	QString *sp = new QString("");
	int sz = partIds.size();
	partIds.resize(sz+1);
	for (int j = sz - 1; j >= i; j--) {
		partIds.insert(j + 1, partIds.at(j));
	}
	partIds.insert(i, sp);
	// copy relevant parameters from the current staff:
	// staff name
	current_2ndstaff->staffName_ = current_staff->staffName_;	// LVIFIX: check UTF-8 correct
	// staff's MIDI channel
	current_2ndstaff->setChannel(current_staff->getChannel());
	// staff's MIDI program
	current_2ndstaff->changeVoice(current_staff->getVoice());
	current_2ndstaff->getVoiceNr(0)->voiceSet_ = true;
	return true;
}


// append an NSign to the current stave(s)
// note: cannot use NVoice::insertSegnoRitardAndAccelAt to insert at current
// time, because that causes segno's etc. at the start of a measure to be
// appended to the end of the previous measure

void MusicXMLParser::appendSign(int type)
{
	NSign * sign;
	NVoice * voice;
	voice = current_staff->getVoiceNr(0);
	sign = new NSign(voice->getMainPropsAddr(),
			current_staff->getStaffPropsAddr(), type);
	voice->appendElem(sign);
	if (current_2ndstaff) {
		voice = current_2ndstaff->getVoiceNr(0);
		sign = new NSign(voice->getMainPropsAddr(),
				current_2ndstaff->getStaffPropsAddr(), type);
		voice->appendElem(sign);
	}
}


// append an NText (arbitrary text) to the current stave(s)
// QString textVal is a text itself

void MusicXMLParser::appendText(QString textVal)
{
	NText * text;
	NVoice * voice;
	int type = (stPlc == "below") ? TEXT_DOWNTEXT : TEXT_UPTEXT;
	
	voice = current_staff->getVoiceNr(0);
	text = new NText(voice->getMainPropsAddr(),
			current_staff->getStaffPropsAddr(),
			textVal, type);
	voice->appendElem(text);
	if (current_2ndstaff) {
		voice = current_2ndstaff->getVoiceNr(0);
		text = new NText(voice->getMainPropsAddr(),
				current_2ndstaff->getStaffPropsAddr(),
				textVal, type);
		voice->appendElem(text);
	}
}


// convert time in divisions to miditime
// note that QUARTER_LENGTH=161280, causing an overflow when divs>13315
// assuming a 32-bit system
// LVIFIX: check validity of divs and iDiv, abort import on error

int MusicXMLParser::cvtDivsToMidiTime(int divs)
{
	return divs * QUARTER_LENGTH / iDiv;
}


// set the actual time to the current time in the current voice
// this implies adding (hidden) rests when necessary
// note that the actual amount of time in voice 0 needs to be determined,
// which may differ from the time signature

// for voices == first voice this implies:
// fill with rests
// note: Recordare's BeetAnGeSample.xml contains:
// <forward> in voice 1 (part P2, start of measure 5)
// <forward> in voice 3, <backup> and creation of voice 4 (part P2, measure 6)

// for voices != first voice this implies:
// determine voice's midi end time
// search from there in voice 0 until end-of-bar or current time reached
// fill with hidden rests
// repeat until done

// LVIFIX: needs cleanup

void MusicXMLParser::fillUntil(int curtime, bool hidden, bool rpterr)
{
	rpterr = false;	// remove this when the caller supplies tuplet stat
			// no errors should be generated for correct tuplets
	QString Str;
	current_voice->computeMidiTime(false, false);
	int cvMidiEndTime = current_voice->getMidiEndTime();
	int currentMidiTime = curtime;
	if (current_voice->isFirstVoice()) {
		if (currentMidiTime > cvMidiEndTime) {
			insertRest(currentMidiTime - cvMidiEndTime, hidden);
		} else if (currentMidiTime == cvMidiEndTime) {
			// nothing
		} else {
			// LVIFIX report error required ?
			// -> yes (exceptions: chord splitting and tuplets)
			// be careful not to break <chord> handling
			if (rpterr) {
				Str = "currentMidiTime < cvMidiEndTime";
				reportWarning(Str);
			}
		}
		return;
	}
	// other voice
	if (currentMidiTime < cvMidiEndTime) {
		// LVIFIX report error required ?
		// -> yes (exception: chord splitting and tuplets)
		// be careful not to break <chord> handling
		if (rpterr) {
			Str = "currentMidiTime < cvMidiEndTime";
			reportWarning(Str);
		}
	}
	// loop over all elements in voice 0:
	//   if barsym is found with cvMidiEndTime < miditime <= currentMidiTime:
	//     insert hidden rests (miditime - cvMidiEndTime)
	//     cvMidiEndTime = miditime
	// if currentMidiTime > cvMidiEndTime:
	//     insert hidden rests (currentMidiTime - cvMidiEndTime)
	NVoice * voice0 = current_staff->getVoiceNr(0);
	if (voice0 == 0) return;
	NMusElement *elem = 0;
	int delta = 0;
	elem = voice0->getFirstPosition();
	while (elem) {
		if (elem->getType() == T_SIGN
		    && (elem->getSubType() & BAR_SYMS)) {
			if ((cvMidiEndTime < elem->midiTime_)
			    && (elem->midiTime_ <= currentMidiTime)) {
			    	delta = elem->midiTime_ - cvMidiEndTime;
				insertRest(delta, hidden);
				cvMidiEndTime += delta;
			}
		}
		elem = voice0->getNextPosition();
	}
	delta = currentMidiTime - cvMidiEndTime;
	if (delta > 0) {
		insertRest(delta, hidden);
		cvMidiEndTime += delta;
	}
	// LVIFIX: following code won't work when clefs change
	// hack: repair the actual clef by setting it to the first clef
//	NClef *c = voice0->getFirstClef();
//	current_staff->actualClef_.changeKind(c->getSubType());
//	current_staff->actualClef_.setShift(c->getOctave());
}


// fill all voices (in both staves, if applicable) with hidden rests
// to make them all end at the same time

void MusicXMLParser::fillVoices()
{
	NStaff *staff;
	NVoice *voice;
	// determine midi end time of all voices
	int i;
	int maxMidiEndTime = 0;
	int midiEndTime;
	staff = current_staff;
	for (i = 0; i < staff->voiceCount(); i++) {
		voice = staff->getVoiceNr(i);
		voice->computeMidiTime(false, false);
		midiEndTime = voice->getMidiEndTime();
		if (midiEndTime > maxMidiEndTime) {
			maxMidiEndTime = midiEndTime;
		}
	}
	if (current_2ndstaff) {
		staff = current_2ndstaff;
		for (i = 0; i < staff->voiceCount(); i++) {
			voice = staff->getVoiceNr(i);
			voice->computeMidiTime(false, false);
			midiEndTime = voice->getMidiEndTime();
			if (midiEndTime > maxMidiEndTime) {
				maxMidiEndTime = midiEndTime;
			}
		}
	}
	// brute force solution to fill all voices with hidden rests
	current_time = maxMidiEndTime;
	staff = current_staff;
	bool hidden = true;
	for (i = 0; i < staff->voiceCount(); i++) {
		current_voice = staff->getVoiceNr(i);
		fillUntil(current_time, hidden);
	}
	if (current_2ndstaff) {
		staff = current_2ndstaff;
		for (i = 0; i < staff->voiceCount(); i++) {
			current_voice = staff->getVoiceNr(i);
			fillUntil(current_time, hidden);
		}
	}
}


// get the clef that is in effect in staff at miditime
// return NResource::nullClef_ if no clef found

NClef * MusicXMLParser::getClefAt(NStaff *staff, int miditime)
{
//	cout << "MusicXMLParser::getClefAt(" << miditime << ")" << endl;
	NClef *clef = NResource::nullClef_;
	NVoice * voice_elem = staff->getVoiceNr(0);
	NMusElement *elem;
	elem = voice_elem->getFirstPosition();
	if (elem == 0) return clef;
	do {
//		cout << " midiTime_=" << elem->midiTime_
//		     << " va_=" << elem->va_
//		     << endl;
		if ((elem->getType() == T_CLEF) && (elem->midiTime_ <= miditime)) {
			clef = (NClef *) elem;
		}
		elem = voice_elem->getNextPosition();
	} while (elem);
	return clef;
}


// handle attributes
// note that the attribute order in the MusicXML files normally differs
// from what is needed in NoteEdit (which uses the order as is printed):
// MusicXML: key, time, clef
// NoteEdit: clef, key, time
// Also, in piano music two clefs are usually (but not always !) found
// Multi-rest is also handled here
// Exception: <staves> is not handled here but in endElement(staves)

void MusicXMLParser::handleAttributes()
{
	bool res;
	QString Str;
	// handle the clef(s)
	handleClef(current_staff, stCli, stCoc, stCsi);
	handleClef(current_2ndstaff, stCli2, stCoc2, stCsi2);
	// handle the key signature
	// if stFif is empty, assume no key signature
	int iFif = 0;
	if (stFif != "") {
		iFif = stFif.toInt(&res);
		if (res) {
			if ((-7 <= iFif) && (iFif <= 7)) {
				int count;
				status_type kind;
				if (iFif < 0) {
					count = -iFif; kind = STAT_FLAT;
				} else {
					count =  iFif; kind = STAT_CROSS;
				}
				if (count > 0) {
					// LVIFIX: check handling accidentals and key signature
					/*
					current_staff->actualKeysig_.setRegular(count, kind);
					current_staff->actualKeysig_.setClef(&(current_staff->actualClef_));
					NKeySig * ksig = new NKeySig(current_voice->getMainPropsAddr(),
								     current_staff->getStaffPropsAddr());
					ksig->change(&(current_staff->actualKeysig_));
					current_voice = current_staff->getVoiceNr(0);
					current_voice->appendElem(ksig);
					*/
					NStaff *staff = 0;
					NVoice *voice = 0;
					staff = current_staff;
					voice = staff->getVoiceNr(0);
					NClef *clef = getClefAt(staff, current_time);
					NKeySig * ksig = new NKeySig(voice->getMainPropsAddr(),
								     staff->getStaffPropsAddr());
					ksig->setRegular(count, kind);
					ksig->setClef(clef);
					voice->appendElem(ksig);
					if (current_2ndstaff) {
						staff = current_2ndstaff;
						voice = staff->getVoiceNr(0);
						NClef *clef = getClefAt(staff, current_time);
						NKeySig * ksig = new NKeySig(voice->getMainPropsAddr(),
									     staff->getStaffPropsAddr());
						ksig->setRegular(count, kind);
						ksig->setClef(clef);
						voice->appendElem(ksig);
					}
				}
			} else {
				res = false;
			}
		}
		if (!res) {
			Str = "illegal <fifths> value: " + stFif;
			reportWarning(Str);
		}
	}
	// handle the time signature
	// if both stBts and stBtt are empty, assume no time signature
	if ((stBts != "") && (stBtt != "")) {
		int iBts = 0;
		int iBtt = 0;
		bool res1 = true;
		bool res2 = true;
		iBts = stBts.toInt(&res1);
		iBtt = stBtt.toInt(&res1);
		// LVIFIX: improve error handling
		if (res1 && res2) {
			NVoice *voice;
			current_staff->staff_props_.measureLength = iBts * 128 / iBtt;
			voice = current_staff->getVoiceNr(0);
			voice->appendElem(T_TIMESIG, iBts, iBtt);
			if (current_2ndstaff) {
				voice = current_2ndstaff->getVoiceNr(0);
				voice->appendElem(T_TIMESIG, iBts, iBtt);
			}
		} else {
			Str = "illegal <time> value: " + stBts + "/" + stBtt;
			reportWarning(Str);
		}
	}
	// handle the multi rest
	handleMultiRest();
}


// handle the barline
// in case of <barline location="left">:
// - first check for bar-style heavy-light plus repeat forward
//   or bar-style light-light, other combinations not allowed
// - then check for ending start
// in case of <barline location="right">:
// - ignore ending
// - check for bar-style:
//   - light-heavy w/ or w/o repeat backward
//   - others (except heavy-light) ?

// note: MusicXML's bar-style light-heavy w/o repeat backward equals
// NoteEdit's EndBar, w/ repeat backward equals NoteEdit's RepeatClose
// in MusicMXL, no equivalent for NoteEdit's RepeatOpenClose exists

void MusicXMLParser::handleBarline()
{
	QString Str;
	if (stBll.isNull() || (stBll == "")) {
		// default in case of no location is right
		stBll = "right";
	}
	if (stBll == "left") {
		if ((stBst == "") && (stRdi == "")) {
			// nothing needs to be done
		} else if ((stBst == "heavy-light") && (stRdi == "forward")) {
			NMusElement *elem;
			NVoice *voice;
			voice = current_staff->getVoiceNr(0); 
			elem = voice->getLastPosition();
			int tp;
			if (elem && ((tp = elem->getType()) == T_SIGN)) {
				int sbtp = elem->getSubType();
				if (sbtp == SIMPLE_BAR) {
					(void) voice->removeLastPosition();
					appendSign(REPEAT_OPEN);
				} else if (sbtp == REPEAT_CLOSE) {
					(void) voice->removeLastPosition();
					appendSign(REPEAT_OPEN_CLOSE);
				}
			} else {
				appendSign(REPEAT_OPEN);
			}
		} else if (stBst == "light-light") {
			NMusElement *elem;
			NVoice *voice;
			voice = current_staff->getVoiceNr(0); 
			elem = voice->getLastPosition();
			int tp;
			if (elem && ((tp = elem->getType()) == T_SIGN)) {
				int sbtp = elem->getSubType();
				if (sbtp == SIMPLE_BAR) {
					(void) voice->removeLastPosition();
				}
			}
			appendSign(DOUBLE_BAR);
		} else {
			Str = "illegal left barline: "
				+ stBst + "/" + stRdi;
			reportWarning(Str);
		}
		if ((stEtp == "") && (stEnr == "")) {
			// nothing needs to be done
		} else if (stEtp == "start") {
			if (stEnr == "1") {
				appendSign(SPECIAL_ENDING1);
			} else if (stEnr == "2") {
				appendSign(SPECIAL_ENDING2);
			} else {
				Str = "illegal ending number: " + stEnr;
				reportWarning(Str);
			}
		} else {
			Str = "illegal ending type: " + stEtp;
			reportWarning(Str);
		}
	} else if (stBll == "right") {
		/* fill the first voice with rests until the needed MIDI time
		   to place the bar is achieved */
		current_voice->computeMidiTime(false, false);
		int neededMidiTime = current_voice->getMidiEndTime();
		current_voice = current_staff->getVoiceNr(0);
		fillUntil(neededMidiTime, true, false);
		
		/* and do the same for 2nd staff if present */
		if (current_2ndstaff) {
			current_voice = current_2ndstaff->getVoiceNr(0);
			fillUntil(neededMidiTime, true, false);
			current_voice = current_staff->getVoiceNr(0);
		}
				
		// note: changing the list of supported subtypes means
		// also changing handleEndOfMeasure()
		if ((stBst == "") && (stRdi == "")) {
			// nothing needs to be done
		} else if ((stBst == "light-heavy") && (stRdi == "backward")) {
			// NoteEdit's RepeatClose
			appendSign(REPEAT_CLOSE);
		} else if ((stBst == "light-heavy") && (stRdi == "")) {
			// NoteEdit's EndBar
			appendSign(END_BAR);
		} else if ((stBst == "light-light") && (stRdi == "")) {
			// NoteEdit's DoubleBar
			appendSign(DOUBLE_BAR);
		} else {
			Str = "illegal right barline: "
				+ stBst + "/" + stRdi;
			reportWarning(Str);
		}
		if ((stEtp != "") && (stEtp != "discontinue")
					&& (stEtp != "stop")) {
			Str = "illegal ending type: " + stEtp;
			reportWarning(Str);
		}
	} else {
		Str = "illegal barline location: " + stBll;
		reportWarning(Str);
	}
}

// append a clef to staff

void MusicXMLParser::handleClef(NStaff *staff, QString& cli, QString& coc, QString& csi)
{
	if (staff == NULL) return;
	if ((cli == "") && (coc == "") && (csi == "")) return;
	bool res = true;
	QString Str;
	// handle the clef
	// if both csi and cli are empty, assume no clef
	int kind = 0;
	int iCoc = 0;
	if (res) {
		if ((csi == "G") && (cli == "2")) {
			kind = TREBLE_CLEF;
		} else if ((csi == "F") && (cli == "4")) {
			kind = BASS_CLEF;
		} else if ((csi == "C") && (cli == "1")) {
			kind = SOPRANO_CLEF;
		} else if ((csi == "C") && (cli == "3")) {
			kind = ALTO_CLEF;
		} else if ((csi == "C") && (cli == "4")) {
			kind = TENOR_CLEF;
		} else {
			Str = "bad clef: ";
			Str += csi;
			Str += cli;
			reportWarning(Str);
			res = false;
		}
	}
	if (res) {
		if (coc == "-1") {
			iCoc = -8;
		} else if ((coc == "") || (coc == "0")) {
			iCoc =  0;
		} else if (coc == "1") {
			iCoc =  8;
		} else {
			Str = "bad octave shift: ";
			Str += coc;
			reportWarning(Str);
			res = false;
		}
	}
	if (res) {
		NClef *c;
		NVoice *voice = staff->getVoiceNr(0);
		c = new NClef(voice->getMainPropsAddr(),
				staff->getStaffPropsAddr(),
				kind, iCoc);
		voice->appendElem(c);
	}
}


// handle octave-shift start, called by addNote()
// stOss and stOst were set by the previous <direction> element
// must make sure setOctaviationStart is called only in a first voice

// notes:
// LVIFIX: due to lack of strict error checking, 8va marks might end up
// at a surprising location (they are simply attached to the next chord,
// which may be far from the intended location)

void MusicXMLParser::handleOctavaStart(NChord * chord)
{
	bool res = true;
	int size = 0;
	QString Str;
	if ((stOst == "") || (stOst == "stop")) {
		// cleanup
		stOss = "";
		stOst = "";
		return; // nothing else to do
	} else if (stOst == "up") {
		size = 8;
	} else if (stOst == "down") {
		size = -8;
	} else {
		Str = "illegal octave-shift type: " + stOst;
		reportWarning(Str);
		res = false;
	}
	if (res) {
		if (stOss == "8") {
			// OK, ignore
		} else {
			Str = "illegal octave-shift size: " + stOss;
			reportWarning(Str);
			res = false;
		}
	}
	if (res) {
		if (current_staff->getVoiceNr(0)->findElemRef(chord) == -1) {
			Str = "octave-shift start outside first voice";
			reportWarning(Str);
			res = false;
		}
	}
	if (res) {
		if (chord->va_) {
			Str = "chord already has octave-shift start or stop";
			reportWarning(Str);
			res = false;
		}
	}
	// valid octave-shift start found
	if (res) {
		chord->setOctaviationStart(size);
	}
	// cleanup
	stOss = "";
	stOst = "";
}


// handle octave-shift stop, called by handleDirection()
// must ignore octave-shift start
// must make sure setOctaviationStart is called only in a first voice

void MusicXMLParser::handleOctavaStop()
{
	bool res = true;
	int size = 8;		// LVIFIX: value is not used
	QString Str;
	if ((stOst == "") || (stOst == "up") || (stOst == "down")) {
		return; // nothing to do
	} else if (stOst == "stop") {
		// OK, ignore
	} else {
		Str = "illegal octave-shift type: " + stOst;
		reportWarning(Str);
		res = false;
	}
	if (res) {
		if (stOss == "8") {
			// OK, ignore
		} else {
			Str = "illegal octave-shift size: " + stOss;
			reportWarning(Str);
			res = false;
		}
	}
	if (res) {
		if (!lastChord) {
			Str = "octave-shift stop without preceding chord";
			reportWarning(Str);
			res = false;
		}
	}
	if (res) {
		if (current_staff->getVoiceNr(0)->findElemRef(lastChord) == -1) {
			Str = "octave-shift stop outside first voice";
			reportWarning(Str);
			res = false;
		}
	}
	if (res) {
		if (lastChord->va_) {
			Str = "chord already has octave-shift start or stop";
			reportWarning(Str);
			res = false;
		}
	}
	// valid octave-shift stop found
	if (res) {
		lastChord->setOctaviationStop(size);
	}
	// cleanup
	stOss = "";
	stOst = "";
}


// handle pedal start/stop, called by addNote()
// stPdl was set by the previous direction

// notes:
// LVIFIX: due to lack of strict error checking, pedal marks might end up
// at a surprising location (they are simply attached to the next chord,
// which may be far from the intended location)
// pedal off should be supported on rests (NoteEdit limitation)

void MusicXMLParser::handlePedal(NChord * chord)
{
	QString Str;
	if (stPdl == "") {
		return; // nothing to do
	} else if (stPdl == "start") {
		chord->status2_ |= STAT2_PEDAL_ON;
	} else if (stPdl == "stop") {
		chord->status2_ |= STAT2_PEDAL_OFF;
	} else {
		Str = "illegal pedal type: " + stPdl;
		reportWarning(Str);
	}
	stPdl = "";
}


// handle tuplets

// LVIFIX: move and init at start of part
static NMusElement * firstElem = 0;

void MusicXMLParser::handleTuplet(NMusElement * elem)
{
	// first check if chord is part of tuplet
	if ((stAno == "") && (stNno == "") && (stTtp == "")) {
		return;
	}
	QString Str;
	// then check error conditions
	// LVIFIX: improve, currently only 3/2 and 6/4 supported
	if ((stAno != "3") && (stAno != "6")) {
		Str = "illegal <actual-notes> value: " + stAno;
		reportWarning(Str);
		return;
	}
	if ((stNno != "2") && (stNno != "4")) {
		Str = "illegal <normal-notes> value: " + stAno;
		reportWarning(Str);
		return;
	}
	if ((stTtp != "start") && (stTtp != "stop") && (stTtp != "")) {
		Str = "illegal <actual> type: " + stTtp;
		reportWarning(Str);
		return;
	}
	if (stTtp == "start") {
		firstElem = elem;
	} else if (stTtp == "stop") {
		if (firstElem == 0) {
			Str = "tuplet stop without start";
			reportWarning(Str);
			return;
		}
		if (!current_voice->buildTuplet(firstElem, elem, stAno.toInt(),
						stNno.toInt())) {
			Str = "could not build tuplet";
			reportWarning(Str);
		}
		firstElem = 0;
	}
}


// handle degree by updating s3..s13

void MusicXMLParser::handleDegree()
{
	QString Str;
	if (!kindFound) {
		Str = "<degree> without valid preceding <kind>";
		reportWarning(Str);
	}
	if      (stDgv ==  "3") { handleDegreeUpdateStep( s3,  4); }
	else if (stDgv ==  "5") { handleDegreeUpdateStep( s5,  7); }
	else if (stDgv ==  "7") { handleDegreeUpdateStep( s7, 10); }
	else if (stDgv ==  "9") { handleDegreeUpdateStep( s9, 14); }
	else if (stDgv == "11") { handleDegreeUpdateStep(s11, 17); }
	else if (stDgv == "13") { handleDegreeUpdateStep(s13, 21); }
	else {
		Str = "invalid <degree-value>: " + stDgv;
		reportWarning(Str);
	}
}


// handle degree: update a specific step

void MusicXMLParser::handleDegreeUpdateStep(int& s, int defval)
{
	QString Str;
	if (stDgt == "add") {
		s = defval + stDga.toInt();
	} else if (stDgt == "subtract") {
		s = -1;
	} else {
		Str = "invalid <degree-type>: " + stDgt;
		reportWarning(Str);
	}
}


// handle direction

void MusicXMLParser::handleDirection()
{
	handleDynamics();
	handleMetronome();
	handleWords();
	handleOctavaStop();
}


// handle volume change, based on both <dynamics> (required)
// and <sound dynamics> (optional) (LVIFIX: not yet implemented)
// need to insert sign with correct type (V_PPPIANO..V_FFFORTE)
// and volume into voice 0 (of both staves!)
// if the MusicXML file does not specify <sound dynamics>, a suitable
// default is supplied.

// note: dynamics may occur both inside a direction (e.g. BeetAnGeSample)
// and inside a note (e.g. FaurReveSample)
// LVIFIX: also support last case

void MusicXMLParser::handleDynamics()
{
	if (stDyn != "") {
		int defvol = 0;
		int voltype;
		int volume = -1;	// LVIFIX: set according to <sound>
		if (stDyn == "ppp") {
			defvol = 10;
			voltype = V_PPPIANO;
		}
		else if (stDyn == "pp") {
			defvol = 26;
			voltype = V_PPIANO;
		}
		else if (stDyn == "p") {
			defvol = 42;
			voltype = V_PIANO;
		}
		else if (stDyn == "mp") {
			defvol = 58;
			voltype = V_MPIANO;
		}
		else if (stDyn == "mf") {
			defvol = 74;
			voltype = V_MEZZO;
		}
		else if (stDyn == "f") {
			defvol = 90;
			voltype = V_FORTE;
		}
		else if (stDyn == "ff") {
			defvol = 106;
			voltype = V_FFORTE;
		}
		else if (stDyn == "fff") {
			defvol = 122;
			voltype = V_FFFORTE;
		}
		else {
			QString Str;
			Str = "illegal dynamics value: " + stDyn;
			reportWarning(Str);
			return;
		}
		if (volume == -1) {
			volume = defvol;
		}
		NSign *sign;
		NVoice * voice;
		voice = current_staff->getVoiceNr(0);
		sign = new NSign(voice->getMainPropsAddr(), voice->getStaff()->getStaffPropsAddr(), VOLUME_SIG);
		sign->setVolume(voltype, volume);
		// convert to units of 128th
		int volTime = current_time / MULTIPLICATOR;
		if (!voice->insertElemAtTime(volTime, sign, 0)) {
			reportWarning("could not insert volume sign (first staff)");
		}
		if (current_2ndstaff) {
			voice = current_2ndstaff->getVoiceNr(0);
			sign = new NSign(voice->getMainPropsAddr(), voice->getStaff()->getStaffPropsAddr(), VOLUME_SIG);
			sign->setVolume(voltype, volume);
			if (!voice->insertElemAtTime(volTime, sign, 0)) {
				reportWarning("could not insert volume sign (second staff)");
			}
		}
	}
}


// handle end of measure

void MusicXMLParser::handleEndOfMeasure()
{
	// fill all voices with hidden rests
	fillVoices();
	// append a simple_bar to both staves, unless handleBarline()
	// already inserted a right barline
	NMusElement *elem;
	elem = current_staff->getVoiceNr(0)->getLastPosition();
	if (elem) {
		int tp = elem->getType();
		int sbtp = elem->getSubType();
		// note: list of subtypes must match with handleBarline()'s
		// list in if "(stBll == "right")"
		if ((tp == T_SIGN)
			&& (   (sbtp == REPEAT_CLOSE)
			    || (sbtp == END_BAR)
			    || (sbtp == DOUBLE_BAR))) {
			return;
		}
	}
	appendSign(SIMPLE_BAR);
}


// handle frame-note

void MusicXMLParser::handleFrameNote()
{
	bool res = true;
	QString Str;
	int f = stFrt.toInt(&res);
	if (!res || (f < 0) || (f > 24)) {
		Str = "illegal fret value: " + stFrt;
		reportWarning(Str);
		return;
	}
	res = true;
	int s = stStr.toInt(&res);
	if (!res || (s < 1) || (s > 6)) {
		Str = "illegal string value: " + stStr;
		reportWarning(Str);
		return;
	}
	stFrn[6 - s] = f;
}


// handle harmony

void MusicXMLParser::handleHarmony()
{
/*
	cout << "MusicXMLParser::handleHarmony()"
		<< " stBsa=" << stBsa
		<< " stBss=" << stBss
		<< " stRta=" << stRta
		<< " stRts=" << stRts
		<< endl;
*/
	QString chordName;
	QString Str;
	// convert root-step and root-alter to tonic value
	int tonic = 0;
	int ind = -1;		// index of this root-step in notes_us1
	for (int i = 0; i < 12; i++) {
		if (stRts == note_name_res(i, 0)) {
			ind = i;
			break;
		}
	}
	if (ind == -1) {
		Str = "illegal root-step value: " + stRts;
		reportWarning(Str);
		return;
	}

	tonic = ind;
	if (stRta == "-1") {
		tonic--;
		if (tonic < 0) {
			tonic = 11;
		}
	} else if ((stRta == "") || (stRta == "0")) {
		; /* do nothing */
	} else if (stRta == "1") {
		tonic++;
		if (tonic > 11) {
			tonic = 0;
		}
	} else {
		Str = "illegal root-alter value: " + stRta;
		reportWarning(Str);
		return;
	}

	// buildName needs steps modulo 12
	if (s9  >= 12) { s9  -= 12; }
	if (s11 >= 12) { s11 -= 12; }
	if (s13 >= 12) { s13 -= 12; }

/*
	cout << "steps 3 5 7 9 11 13:"
	    << " " << s3
	    << " " << s5
	    << " " << s7
	    << " " << s9
	    << " " << s11
	    << " " << s13
	    << endl;
*/

	// build KGuitar-style note name
	chordName = buildName(tonic, /* NOT USED */ 0,
			s3, s5, s7, s9, s11, s13,
			NResource::globalNoteNames_,
			NResource::globalMaj7_,
			NResource::globalFlatPlus_);

	// create a new chord diagram
	pendingChordDiagram = new NChordDiagram(chordName);
	// if a frame was found, then set it's value
	if (stFrm) {
		pendingChordDiagram->setValues(stFrn, chordName, true);
	}
}


// handle kind: convert MusicXML kind to step values
// if successful, set kindFound and s3..s13

void MusicXMLParser::handleKind(QString& knd)
{
	QString Str;
	int ind = -1;		// index of this chord in MxmlChordTab
	for (int i = 0; MxmlChordTab[i].kind != 0; i++) {
		if (knd == MxmlChordTab[i].kind) {
			ind = i;
			break;
		}
	}
	if (ind == -1) {
		Str = "harmony kind not supported: " + knd;
		reportWarning(Str);
		return;
	}
	s3  = MxmlChordTab[ind].s3,
	s5  = MxmlChordTab[ind].s5,
	s7  = MxmlChordTab[ind].s7,
	s9  = MxmlChordTab[ind].s9,
	s11 = MxmlChordTab[ind].s11,
	s13 = MxmlChordTab[ind].s13,
	kindFound = true;
}


// handle multi-rest

void MusicXMLParser::handleMultiRest()
{
	if (stMrs == "") {
		return; // nothing to be done
	}
	int iMrs = 0;
	bool res = true;
	QString Str;
	iMrs = stMrs.toInt(&res);
	if (!res || (iMrs <= 0)) {
		Str = "illegal <multi-rest> value: " + stMrs;
		reportWarning(Str);
	} else {
		NRest *rest;
		NVoice * voice;
		voice = current_staff->getVoiceNr(0);
		rest = new NRest(voice->getMainPropsAddr(),
				voice->getStaff()->getStaffPropsAddr(),
				&(voice->yRestOffs_), MULTIREST, iMrs);
		voice->appendElem(rest);
		if (current_2ndstaff) {
			voice = current_2ndstaff->getVoiceNr(0);
			rest = new NRest(voice->getMainPropsAddr(),
					voice->getStaff()->getStaffPropsAddr(),
					&(voice->yRestOffs_), MULTIREST, iMrs);
			voice->appendElem(rest);
		}
	}
}


// handle lyrics: add a line to the stLyr array.
// in: index in stLyn (lyrics number), text in stTxt.

void MusicXMLParser::handleLyrics()
{
	int i = stLyn.toInt();
	QString Str;
	// check error conditions first
	// note that if stLyn is not an integer, i will be 0, which is caught
	if ((i <= 0) || (i > NUM_LYRICS)) {
		Str = "illegal lyrics number value: " + stLyn;
		reportWarning(Str);
		return;
	}
	stLyr[i - 1] = stTxt;
}


// handle metronome

void MusicXMLParser::handleMetronome()
{
	// check if any value given
	if ((stBtu == "") && (stPrm == "")) {
		return;
	}
	// verify all values are legal
	QString Str;
	if (stBtu == "quarter") {
	} else {
		Str = "illegal beat-unit value: " + stBtu;
		reportWarning(Str);
		return;
	}
	if ((stBtd < 0) || (stBtd > 2)) {
		Str.setNum(stBtd);
		Str = "illegal beat-unit-dot value: " + Str;
		reportWarning(Str);
		return;
	}
	bool res = true;
	int iPrm = stPrm.toInt(&res);
	if (!res || (iPrm <= 0)) {
		Str = "illegal per-minute value: " + stPrm;
		reportWarning(Str);
		return;
	}
	// calcalute NoteEdit's quarter notes per minute value
	int qnpm;
	if (stBtd == 1) {
		qnpm = iPrm * 3 / 2;
	} else if (stBtd == 2) {
		qnpm = iPrm * 7 / 4;
	} else {
		qnpm = iPrm;
	}
	// final check (as in grammar.yy)
	if ((qnpm < 10) || (qnpm > 300)) {
		Str.setNum(qnpm);
		Str = "bad tempo: " + Str;
		reportWarning(Str);
		return;
	}
	// and store it
	// LVIFIX: is it necessary to store the tempo sig in all voices ?
	// MusicXML typically stores the metronome in the first part only
	NSign *sign;
	NVoice * voice;
	voice = current_staff->getVoiceNr(0);
	sign = new NSign(voice->getMainPropsAddr(), voice->getStaff()->getStaffPropsAddr(), TEMPO_SIGNATURE);
	sign->setTempo(qnpm);
	// convert to units of 128th
	int volTime = current_time / MULTIPLICATOR;
	if (!voice->insertElemAtTime(volTime, sign, 0)) {
		reportWarning("could not insert tempo sign (first staff)");
	}
	if (current_2ndstaff) {
		voice = current_2ndstaff->getVoiceNr(0);
		sign = new NSign(voice->getMainPropsAddr(), voice->getStaff()->getStaffPropsAddr(), TEMPO_SIGNATURE);
		sign->setTempo(qnpm);
		if (!voice->insertElemAtTime(volTime, sign, 0)) {
			reportWarning("could not insert tempo sign (second staff)");
		}
	}
}


// handle staff and voice change by setting current_voice
// create voice if necessary
// note: staff_nr and voice_nr start at 1

void MusicXMLParser::handleVoice(int staff_nr, int voice_nr)
{
	current_voice = 0;		// LVIFIX: force crash in case of error
	// default staff is staff 1
	if (staff_nr == 0) {
		staff_nr = 1;
	}
	QString Str;
	// check error conditions first
	if ((staff_nr < 1) || (staff_nr > 2)) {
		Str.setNum(staff_nr);
		Str = "illegal <staff> value: " + Str;
		reportError(Str);
	}
	if (voice_nr <= 0) {
		Str.setNum(voice_nr);
		Str = "illegal <voice> value: " + Str;
		reportError(Str);
	}
	if (staff_nr == 1) {
		handleVoiceDoStaff(staff_nr, voice_nr, current_staff,
					first_vc_1st_st_mapped);
	} else {
		handleVoiceDoStaff(staff_nr, voice_nr, current_2ndstaff,
					first_vc_2nd_st_mapped);
	}
}


// select/map/create a voice for the staff given
// also init beamstart for that voice

// LVIFIX: remove first_vc_???_st_mapped by mapping each staff's first voice
// on staff creation ?

void MusicXMLParser::handleVoiceDoStaff(int staff_nr, int voice_nr,
				NStaff * & staff, bool & mapped)
{
	int ntdtVnr = vm.get(staff_nr, voice_nr);
	if (ntdtVnr >= 0) {
		// voice already exists
		current_voice = staff->getVoiceNr(ntdtVnr);
		return;
	}
	// voice mapper is empty
	if ((!mapped) && (voice_nr==1)) {
		// first note for this staff:
		// voice already exists but not yet mapped
		vm.set(staff_nr, voice_nr, 0);
		mapped = true;
		current_voice = staff->getVoiceNr(0);
	} else {
		// add number of voices needed to accommodate the file's voice number
		// create and map a new voice
		int i;
		for (i = staff->voiceCount(); i < voice_nr; i++) {
			staff->addVoices(1);
			current_voice = staff->getVoiceNr(i);
			vm.set(staff_nr, i+1 , i);
			parser_params.newVoices->append(current_voice);
		}
	}
	int k = sv2k(staff_nr, voice_nr);
	beamStarts[k] = 0;
}


// handle words

void MusicXMLParser::handleWords()
{
	// check if any value given
	if (stWrd == "") {
		return;
	}
	// do the work
	QString Str;
	int type;
	if (stWrd == "accel.") {
		type = ACCELERANDO;
	} else if (stWrd == "D.S.") {
		type = DAL_SEGNO;
	} else if (stWrd == "D.S. al Coda") {
		type = DAL_SEGNO_AL_CODA;
	} else if (stWrd == "D.S. al Fine") {
		type = DAL_SEGNO_AL_FINE;
	} else if (stWrd == "Fine") {
		type = FINE;
	} else if (stWrd == "ritard.") {
		type = RITARDANDO;
	} else {
		appendText(stWrd); // if none of the standard words are found, place an arbitrary text then
		return;
	}
	appendSign(type);
}


// initialize attribute state variables

void MusicXMLParser::initStAttributes()
{
	stBts  = "";
	stBtt  = "";
	stCli  = "";
	stCln  = "";
	stCoc  = "";
	stCsi  = "";
	stCli2 = "";
	stCoc2 = "";
	stCsi2 = "";
	stDiv  = "";
	stFif  = "";
	stMrs  = "";
	stSta  = "";
}


// initialize barline state variables

void MusicXMLParser::initStBarline()
{
	stBll = "";
	stBst = "";
	stRdi = "";
	stEnr = "";
	stEtp = "";
}

// initialize direction state variables

void MusicXMLParser::initStDirect()
{
	stBtd = 0;
	stBtu = "";
	stDyn = "";
	stOss = "";
	stOst = "";
	stPdl = "";
	stPrm = "";
	stWrd = "";
}

// initialize frame-note state variables

void MusicXMLParser::initStFrameNote()
{
	stFrt = "";
	stStr = "";
}

// initialize harmony state variables

void MusicXMLParser::initStHarmony()
{
	kindFound = false;
	s3  = -1;
	s5  = -1;
	s7  = -1;
	s9  = -1;
	s11 = -1;
	s13 = -1;
	stBsa = "";
	stBss = "";
	stDga = "";
	stDgt = "";
	stDgv = "";
	stFrm = false;
	for (int i = 0; i < 6; i++) {
		stFrn[i] = -1;
	}
	stRta = "";
	stRts = "";
}

// initialize note state variables

void MusicXMLParser::initStNote()
{
	status = 0;
	stAcc = "";
	stAlt = "";
	stAno = "";
	stArp = false;
	stBea = "";
	stBnr = "";
	stCho = false;
	stDoc = "";
	stDst = "";
	stDts = 0;
	stDur = "";
	stFrt = "";
	stGls = false;
	stGra = false;
	stGsl = false;
	stHmr = false;
	stLyn = "";
	for (int i = 0; i < NUM_LYRICS; i++) {
		stLyr[i] = "";
	}
	stNno = "";
	stOct = "";
	stPlo = false;
	stRst = false;
	stSlr = false;
	stStf = "";
	stStm = "";
	stStp = "";
	stStr = "";
	stTie = false;
	stTrl = false;
	stTtp = "";
	stTxt = "";
	stTyp = "";
	stVoi = "";
	stWvl = "";
}


// initialize part state variables

void MusicXMLParser::initStScorePart()
{
	stPid = "";
//	stPmb = "";
	stPmc = "";
	stPmp = "";
	stPnm = "";
	currentMeasure = 0;
}


// insert (hidden) rests in the current voice
// length is split in smaller parts if necessary

void MusicXMLParser::insertRest(int length, bool hidden)
{
	for (int i = DOUBLE_WHOLE_LENGTH; i >= NOTE128_LENGTH; i /= 2) {
		while (length >= i) {
			int stat = hidden ? STAT_HIDDEN : 0;
			NRest * rest = new NRest(&(parser_params.mainWidget->main_props_),
						 &(current_voice->getStaff()->staff_props_),
						 &(current_voice->yRestOffs_),
						 i,
						 stat);
			current_voice->appendElem(rest);
			length -= i;
		}
	}
	if (length > 0) {
		// LVIFIX: report error
	}
}


// slrh -- slur handler
// - needs information from endElement for both note and slur
// - multiple slurs may be attached to a single note

// slrhInit() - remove all slur descriptions

void MusicXMLParser::slrhInit()
{
	slrhMap.clear();
}


// slrhSlurStart() - called by endElement(slur) to creat a new slur description
// - in: id

void MusicXMLParser::slrhSlurStart(const QString& id)
{
	slrhMap.insert(id, SlurDesc());
}


// slrhSlurStop() - called by endElement(slur) to mark a slur description
// for actual creation
// - in: id

void MusicXMLParser::slrhSlurStop(const QString& id)
{
	if (slrhMap.contains(id)) {
		slrhMap[id].stop = true;
	} else {
		QString Str = "start not found for slur " + id;
		reportWarning(Str);
	}
}

// slrhHandleSlurs() - called by endElement(note) to either finish
// the slur description or actually create all slurs
// - for all slur descriptions:
//   - if chord1 == 0 then fill-in chord ptr
//   - if chord1 != 0 && stop then create slur from chord1 until chord
// - in: chord ptr

void MusicXMLParser::slrhHandleSlurs(NChord * chord)
{
	SlurDescMap::Iterator it;
	// for slurs that start at this chord (chord1 == 0)
	// set chord1 to the starting chord
	for (it = slrhMap.begin(); it != slrhMap.end(); it++) {
		if (it.data().chord1 == 0) {
			it.data().chord1 = chord;
		}
	}
	// for slurs that stop at this chord (stop == true) create the slur.
	// simply calling slrhMap.remove(it) invalidates iterator it,
	// which causes a core dump. thus it is necessary to maintain a list
	// of keys which is used later to remove these entries.
	QStringList list;
	for (it = slrhMap.begin(); it != slrhMap.end(); it++) {
		if (it.data().stop) {
			NChord * frstChrd = it.data().chord1;
			if (frstChrd) {
				frstChrd->setSlured(true, chord);
			} else {
				QString Str = "start not found for slur "
						+ it.key();
				reportWarning(Str);
			}
			list += it.key();
		}
	}
	QStringList::Iterator lit;
	for (lit = list.begin(); lit != list.end(); ++lit) {
		slrhMap.remove(*lit);
	}
}


// wdgh -- wedge handler
// needs the type attribute:
// - "crescendo/diminuendo" starts wedges (at the next non-grace note chord)
// - "stop" stops wedges (at the previous chord)
// interfaces with NVoice::correctReadTrillSlursAndDynamics():
// - set first chord's dynamicAlign_ to the slur kind
// - set first chord's dynamic_ to the distance from the first to the second
//   chord in NOTE128_LENGTH units
// see also NVoice::setProvisionalDynamic()
// note on Noteedit's file format:
// "<  below 1: 2 til 3.000000;" means wedge from 2nd til 3rd quarter note
// (both included: wedge ends at the right side of the 3rd quarter)
// grammar.yy converts the numbers to units of 128th note starting at 0
// for the first note in the measure.
// "<  below 1: 2 til 1m + 3.000000;" means wedge from 2nd til 3rd quarter note
// in the next measure.
// NVoice::setProvisionalDynamic() sets the wedge's first chord's dynamic_
// to the difference in starting time between the first and second note.
// NVoice::correctReadTrillsSlursAndDynamics() later replaces that with
// the x offset.

// note changes since 2.4.1: dynamic no longer contains the difference
// but the end time (measures to skip in the high word, 128th in the low word)

// wdghInit() - initialize all variables

void MusicXMLParser::wdghInit()
{
	wdghFirstChrdCm = currentMeasure;
	wdghFirstChordPtr = 0;
	wdghLastChrdCt = current_time - measure_start_time;
	wdghLastChrdCm = currentMeasure;
	wdghTypeCres = false;
	wdghInWedge = false;
}


// wdghStEl() - called by startElement(wedge) to start or finish a wedge
// in: type

void MusicXMLParser::wdghStEl(const QString& tp)
{
	QString Str;
	if (tp == "crescendo") {
		wdghInit();
		wdghTypeCres = true;
		wdghInWedge = true;
	} else if (tp == "diminuendo") {
		wdghInit();
		wdghTypeCres = false;
		wdghInWedge = true;
	} else if (tp == "stop") {
		int dm = wdghLastChrdCm - wdghFirstChrdCm;	// measure skip
		int dt = wdghLastChrdCt;			// time skip
		if (!wdghInWedge) {
			Str = "wedge stop without start";
			reportWarning(Str);
		} else if (wdghFirstChordPtr == 0) {
			Str = "wedge without chords";
			reportWarning(Str);
		} else {
			wdghFirstChordPtr->dynamicAlign_ = wdghTypeCres;
			wdghFirstChordPtr->dynamic_ =
				(dm << 16) | (dt / MULTIPLICATOR);
		}
		wdghInit();
	} else {
		Str = "unknown wedge type";
		reportWarning(Str);
	}
}


// wdghAddChrd() - called when a new chord is added by addNote()
// note: no work necessary when not in wedge

void MusicXMLParser::wdghAddChrd()
{
	if (wdghInWedge) {
		if (wdghFirstChordPtr) {
			// not the first chord
			wdghLastChrdCt = current_time - measure_start_time;
			wdghLastChrdCm = currentMeasure;
		} else {
			// the first chord
			wdghFirstChordPtr = current_voice->getCurrentPosition();
			wdghFirstChrdCm = currentMeasure;
		}
	}
}

// trlh -- trill handler
// <trill-mark> determines tr's presence (normal vs. lonely trill)
// <wavy-line type="start"> and ..."stop"> determine wavy line length
// assumptions:
// wavy line starts at left side of note with <wavy-line type="start">
// wavy line stops at right side of note with <wavy-line type="stop">
// wavy line start/stops found only on first note in chord
// note that NoteEdit seems to stop the wavy line at the left side
// of the second note LVIFIX: check with JAnd

// trlhInit() - initialize all trill info

void MusicXMLParser::trlhInit()
{
	trlhChord1 = 0;
	trlhFirstChrdCm = 0;
	trlhInTrill = false;
	trlhTrlMrk = false;
}

// trlhHandleTrills() - called by addNote() to handle trill handler
// state transitions
// - if trill and chord1 == 0 then fill-in chord ptr
// - if not trill and chord1 != 0 then create trill from chord1 until chord
// - if not trill and trill-mark then create one chord "tr" trill
// - in: chord ptr

void MusicXMLParser::trlhHandleTrills(NChord * chord)
{
	if (trlhInTrill && !trlhChord1) {
		trlhChord1 = chord;
		trlhFirstChrdCm = currentMeasure;
	} else if (!trlhInTrill && trlhChord1) {
		int dm = currentMeasure - trlhFirstChrdCm;
		int dt = current_time - measure_start_time;
		trlhChord1->trill_ = (dm << 16) | (dt / MULTIPLICATOR);
		if (!trlhTrlMrk) {
			trlhChord1->trill_ |= 0x8000;
		}
		trlhChord1 = 0;
	} else if (!trlhInTrill && stTrl) {
		// a lone trill-mark (no wavy-line, thus trlhTrlMrk invalid)
		chord->trill_ = +1;
	}
}

// trlhSetStatus() - called by startElement(wavy-line) to start or stop
// a trill
// in: type

void MusicXMLParser::trlhSetStatus(const QString& tp)
{
	QString Str;
	if (tp == "start") {
		trlhChord1 = 0;
		trlhInTrill = true;
		trlhTrlMrk = stTrl;
	} else if (tp == "continue") {
		// ignore
	} else if (tp == "stop") {
		trlhInTrill = false;
	} else {
		Str = "illegal wavy-line type: " + tp;
		reportWarning(Str);
	}
}
