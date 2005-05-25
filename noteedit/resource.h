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

#ifndef RESOURCE_H

#define RESOURCE_H

#include <kapp.h>
#include <ktoolbar.h>

#include <qcolor.h>
#include <qlist.h>
#include <qstring.h>
#include <qregexp.h>

#define AUTOSAVE_INTERVAL_MIN  1
#define AUTOSAVE_INTERVAL_MAX 64
#define TURN_OVER_MIN 0
#define TURN_OVER_MAX 300

#define DEFAULT_OVERLENGTH 60
#define DEFAULT_UNDERLENGTH DEFAULT_OVERLENGTH
#define DEFAULT_LYRICSDIST 60

/* brace in layout */

#define LAYOUT_BRACE_X_POS 4
#define LAYOUT_BRACE_WIDTH 8 
#define LAYOUT_BR_ARROW_XRAD 20
#define LAYOUT_BR_ARROW_YRAD 20
#define LAYOUT_BRACEX_ARROW 20
#define LAYOUT_BRACE_ARC_LENGTH 90
#define LAYOUT_MID_ROUNDDIST 16 
#define LAYOUT_BRACEX_TOTAL (LAYOUT_BRACE_X_POS+LAYOUT_BRACE_WIDTH+LAYOUT_BRACEX_ARROW+LAYOUT_BR_ARROW_XRAD)

/* bracket in layout: */

#define BRACE_BRACKET_DIST 6
#define DEFAULT_LAYOUT_BRACKET_X_POS 20
#define LAYOUT_BRACKET_X_OVERLAP 10
#define LAYOUT_BRACKET_WIDTH1 8
#define LAYOUT_BRACKET_WIDTH2 6
#define LAYOUT_BRACKET_XRAD 60
#define LAYOUT_BRACKET_YRAD 100
#define LAYOUT_BRACKET_ARC_LENGTH 25
#define LAYOUT_BRACKET_X_TOTAL (DEFAULT_LAYOUT_BRACKET_X_POS+LAYOUT_BRACKET_WIDTH1+13 /* LAYOUT_BRACKET_XRAD/2*sin(LAYOUT_BRACKET_ARC_LENGTH*16)*PI/180.0 */)



#define LAYOUT_PIXMAP_X_DIST 2
#define RIGHT_PAGE_BORDER 10
#define TOP_BOTTOM_BORDER 10
#define LEFT_SPACE 25

#define NULL_LINE -111

#define STEM_DIR_AUTO 0
#define STEM_DIR_UP 1
#define STEM_DIR_DOWN 2

#define PREFERRED_ZOOM_VAL 9
#define START_WIDTH 600
#define START_HEIGHT 400
#define WINDOWXY_INCR 20
#define MAXWINDOWXY 400

#define FIRST_PART_TIME 0.6
#define SECOND_PART_TIME 0.2
#define THIRD_PART_TIME 0.1
#define FOURTH_PART_TIME 0.1


#define CONTEXT_WIDTH 220
#define CONTEXT_REC_TOP_BOTTOM 20

#define DEFAULT_LEFT_PAGE_BORDER 20
#define DEFAULT_CONTEXT_REC_LEFT_RIGHT (DEFAULT_LEFT_PAGE_BORDER)
#define CONTEXT_CLEF_X_DIST 5
#define DEFAULT_CONTEXT_CLEF_X_POS (DEFAULT_LEFT_PAGE_BORDER + CONTEXT_CLEF_X_DIST)
#define CONTEXT_KEYSIG_X_DIST 65
#define DEFAULT_CONTEXT_KEYSIG_X_POS (DEFAULT_LEFT_PAGE_BORDER + CONTEXT_KEYSIG_X_DIST)

#define NUM_LYRICS 5
#define LYRICS_LINE_LENGTH 1024
#define LENGTH_OF_BAR_CHECK_ARRAY 20

class staff_props_str;
class NMainWindow;
class NMidiMapper;
class NVoice;
class NMainFrameWidget;
class NMusElement;
class NKeySig;
class NClef;
class expWrn;
class NMusixHint;

class KProgress;
class QPen;
class QBrush;
class QPixmap;
class QFont;

class QLineEdit;

class main_props_str;

struct lily_properties {
	bool lilyAvailable;
	bool lilySemicolons;
	bool lilyVarTrills;
	bool lilySluresInGraces;
	bool lilyVersion2;
	bool lilyVersion24;
	bool lilyProperties;
};

class NResource : public QObject  {
	Q_OBJECT
	public:
		NResource();
		~NResource();
		static void germanUmlautsToTeX(QString *str);
		static void readToolbarSettings(QPtrListIterator<KToolBar> toolBarIterator); 
		static void writeToolbarSettings(QPtrListIterator<KToolBar> toolBarIterator); 
		static NResource *nresourceobj_;
		static QBrush backgroundBrush_;
		static QBrush selectionBackgroundBrush_;
		static QBrush tempoSignatureBrush_;
		static QBrush selectedTempoSignatureBrush_;
		static QBrush blackBrush_;
		static QBrush redBrush_;
		static QBrush contextBrush_;
		static QPen noPen_;
		static QPen staffPen_;
		static QPen selectedStaffPen_;
		static QPen barPen_;
		static QPen selectedBarPen_;
		static QPen barNumberPen_;
		static QPen selectedBarNumberPen_;
		static QPen tempoSignaturePen_;
		static QPen selectedTempoSignaturePen_;
		static QPen volumeSignaturePen_;
		static QPen selectedVolumeSignaturePen_;
		static QPen programChangePen_;
		static QPen selectedProgramChangePen_;
		static QPen specialEndingPen_;
		static QPen selectedSpecialEndingPen_;
		static QPen staffNamePen_;
		static QPen selectedStaffNamePen_;
		static QPen lyricPen_;
		static QPen whiteWidePen_;
		static QPen blackWidePen_;
		static QPen redWidePen_;
		static QPen greyWidePen_;
		static QPen greenPen_;
		static QPen redPen_;
		static QPen whitePen_;
		static QPen helpLinePen_;
		static QPen dummyNotePen_;
		static QPen blackPen_;
		static QPen greyPen_;
		static QString resourceDir_;
		static QString fanfareFile_;

		static QPixmap *stopIcon_;
		static QPixmap *tuplet2_;
		static QPixmap *tuplet3_;
		static QPixmap *tuplet4_;
		static QPixmap *tuplet5_;
		static QPixmap *tuplet6_;
		static QPixmap *tuplet7_;
		static QPixmap *tuplet8_;
		static QPixmap *tuplet9_;
		static QPixmap *tuplet10_;

		static QPixmap *time_24Icon_;
		static QPixmap *time_44Icon_;
		static QPixmap *time_34Icon_;
		static QPixmap *time_38Icon_;
		static QPixmap *time_68Icon_;

		static QPixmap *crossIcon_;
		static QPixmap *flatIcon_;
		static QPixmap *naturIcon_;

		static QPixmap *fullPixmap_;
		static QPixmap *fullRedPixmap_;
		static QPixmap *fullGreyPixmap_;
		static QPixmap *fullMagPixmap_;
		static QPixmap *flagPixmap_;
		static QPixmap *flagRedPixmap_;
		static QPixmap *flagGreyPixmap_;
		static QPixmap *tinyFlagPixmap_;
		static QPixmap *tinyFlagRedPixmap_;
		static QPixmap *tinyFlagGreyPixmap_;
		static int flagPixmapWidth_;
		static int tinyFlagPixmapWidth_;
		static QPixmap *flagDownPixmap_;
		static QPixmap *flagDownRedPixmap_;
		static QPixmap *flagDownGreyPixmap_;
		static int flagDownPixmapHeight_;
		static QPixmap *nbasePixmap_;
		static QPixmap *nbaseRedPixmap_;
		static QPixmap *nbaseGreyPixmap_;
		static QPixmap *tinyBasePixmap_;
		static QPixmap *tinyBaseRedPixmap_;
		static QPixmap *tinyBaseGreyPixmap_;

		static QPixmap *r128Pixmap_;
		static QPixmap *r128RedPixmap_;
		static QPixmap *r128GreyPixmap_;
		static QPixmap *r128MagPixmap_;
		static QPixmap *r64Pixmap_;
		static QPixmap *r64RedPixmap_;
		static QPixmap *r64GreyPixmap_;
		static QPixmap *r64MagPixmap_;
		static QPixmap *r32Pixmap_;
		static QPixmap *r32RedPixmap_;
		static QPixmap *r32GreyPixmap_;
		static QPixmap *r32MagPixmap_;
		static QPixmap *r16Pixmap_;
		static QPixmap *r16RedPixmap_;
		static QPixmap *r16GreyPixmap_;
		static QPixmap *r16MagPixmap_;
		static QPixmap *r8Pixmap_;
		static QPixmap *r8RedPixmap_;
		static QPixmap *r8GreyPixmap_;
		static QPixmap *r8MagPixmap_;
		static QPixmap *rquarterPixmap_;
		static QPixmap *rquarterRedPixmap_;
		static QPixmap *rquarterGreyPixmap_;
		static QPixmap *rquarterMagPixmap_;
		static QPixmap *rhalfPixmap_;
		static QPixmap *rhalfRedPixmap_;
		static QPixmap *rhalfGreyPixmap_;
		static QPixmap *rhalfMagPixmap_;
		static QPixmap *rfullPixmap_;
		static QPixmap *rfullRedPixmap_;
		static QPixmap *rfullGreyPixmap_;
		static QPixmap *rfullMagPixmap_;
		static QPixmap *brevePixmap_;
		static QPixmap *breveRedPixmap_;
		static QPixmap *breveGreyPixmap_;

		static QPixmap *crossPixmap_;
		static QPixmap *crossRedPixmap_;
		static QPixmap *crossGreyPixmap_;
		static int crossPixWidth_;
		static QPixmap *dcrossPixmap_;
		static QPixmap *dcrossRedPixmap_;
		static QPixmap *dcrossGreyPixmap_;
		static int dcrossPixWidth_;
		static QPixmap *flatPixmap_;
		static QPixmap *flatRedPixmap_;
		static QPixmap *flatGreyPixmap_;
		static int flatPixWidth_;
		static QPixmap *dflatPixmap_;
		static QPixmap *dflatRedPixmap_;
		static QPixmap *dflatGreyPixmap_;
		static int dflatPixWidth_;
		static QPixmap *naturPixmap_;
		static QPixmap *naturRedPixmap_;
		static QPixmap *naturGreyPixmap_;
		static int naturPixWidth_;

		static QPixmap *repOpenPixmap_;
		static QPixmap *repOpenRedPixMap_;
		static QPixmap *repClosePixmap_;
		static QPixmap *repCloseRedPixMap_;
		static QPixmap *repOpenClosePixmap_;
		static QPixmap *repOpenCloseRedPixMap_;
		static QPixmap *endBarPixmap_;
		static QPixmap *endBarRedPixmap_;

		static QPixmap  *treblePixmap_;
		static QPixmap  *trebleRedPixmap_;
		static QPixmap  *treblepPixmap_;
		static QPixmap  *treblepRedPixmap_;
		static QPixmap  *treblemPixmap_;
		static QPixmap  *treblemRedPixmap_;
		static QPixmap  *bassPixmap_;
		static QPixmap  *bassRedPixmap_;
		static QPixmap  *basspPixmap_;
		static QPixmap  *basspRedPixmap_;
		static QPixmap  *bassmPixmap_;
		static QPixmap  *bassmRedPixmap_;

		static QPixmap  *altoPixmap_;
		static QPixmap  *altoRedPixmap_;
		static QPixmap  *altopPixmap_;
		static QPixmap  *altopRedPixmap_;
		static QPixmap  *altomPixmap_;
		static QPixmap  *altomRedPixmap_;

		static QPixmap  *drumClefPixmap_;
		static QPixmap  *drumClefRedPixmap_;

		static QPixmap  *drumBassClefPixmap_;
		static QPixmap  *drumBassClefRedPixmap_;

		static QPixmap  *segnoPixmap_;
		static QPixmap  *segnoRedPixmap_;
		static QPixmap  *codaPixmap_;
		static QPixmap  *codaRedPixmap_;
		static QPixmap  *dalSegnoAlCodaPixmap_;
		static QPixmap  *dalSegnoAlCodaRedPixmap_;
		static QString  dalSegno_;
		static QString  dalSegnoAlFine_;
		static QString  fine_;
		static QString  ritardando_;
		static QString  accelerando_;

		static QPixmap *sforzatoAbPixmap_;
		static QPixmap *sforzatoAbRedPixmap_;
		static QPixmap *sforzatoBePixmap_;
		static QPixmap *sforzatoBeRedPixmap_;
		static QPixmap *portatoPixmap_;
		static QPixmap *portatoRedPixmap_;
		static QPixmap *strong_pizzicatoAbPixmap_;
		static QPixmap *strong_pizzicatoAbRedPixmap_;
		static QPixmap *strong_pizzicatoBePixmap_;
		static QPixmap *strong_pizzicatoBeRedPixmap_;
		static QPixmap *sforzandoPixmap_;
		static QPixmap *sforzandoRedPixmap_;
		static QPixmap *fermateAbPixmap_;
		static QPixmap *fermateAbRedPixmap_;
		static QPixmap *fermateBePixmap_;
		static QPixmap *fermateBeRedPixmap_;
		static QPixmap *trillPixmap_;
		static QPixmap *trillRedPixmap_;
		static QPixmap *pedonPixmap_;
		static QPixmap *pedonRedPixmap_;
		static QPixmap *pedoffPixmap_;
		static QPixmap *pedoffRedPixmap_;
		static QPixmap *arpeggPixmap_;
		static int arpegPixmapHeight_;

		static QPixmap *perCrossPixmap_;
		static QPixmap *perCrossRedPixmap_;
		static QPixmap *perCrossGreyPixmap_;

		static QPixmap *perCross2Pixmap_;
		static QPixmap *perCross2RedPixmap_;
		static QPixmap *perCross2GreyPixmap_;

		static QPixmap *perCrossCircPixmap_;
		static QPixmap *perCrossCircRedPixmap_;
		static QPixmap *perCrossCircGreyPixmap_;

		static QPixmap *perRectPixmap_;
		static QPixmap *perRectRedPixmap_;
		static QPixmap *perRectGreyPixmap_;

		static QPixmap *perTrianPixmap_;
		static QPixmap *perTrianRedPixmap_;
		static QPixmap *perTrianGreyPixmap_;

		static QPixmap *musixwarn1_;
		static QPixmap *musixwarn2_;

		static NMusixHint *musixHint_;

		static QFont *textFont_;
		static expWrn *exportWarning_;


#if KDE_VERSION >= 220
		//  Temporary kludge to be used until the equivalent is implemented in
		//  kdelibs.                                                   -Erik Sigra
		static void detailedWarningDontShowAgain
		 (QWidget *, const QString &, const QString &, const QString &,
		  const QString &, bool notify=true
		 );
#endif

		static QCursor *cursor_128thnote_;
		static QCursor *cursor_breve_;
		static QCursor *cursor_fullnote_;
		static QCursor *cursor_sixteenthnote_;
		static QCursor *cursor_tinystroke_;
		static QCursor *cursor_32ndnote_;
		static QCursor *cursor_edit_;
		static QCursor *cursor_halfnote_;
		static QCursor *cursor_tinyeight_;
		static QCursor *cursor_64thnote_;
		static QCursor *cursor_eightnote_;
		static QCursor *cursor_quarternote_;
		static QCursor *cursor_tinysixteenth_;


		static QCursor *determineCursor(int length);
		static void printError(QString s);
		static void printWarning(QString s);
		static void abort( QString s, signed char no = -1 );
		static void setAutosave(bool enable, int intervall);
		static int nbasePixmapHeight_;
		static int narrow_dist_;
		static int tinyBasePixmapHeight_;
		static int nbasePixmapWidth_;
		static int tinyBasePixmapWidth_;
		static int nbasePixmapWidth2_;
		static int tinyBasePixmapWidth2_;
		static int noteLength2Button_(int length);
		static int button2Notelength_(int buNr);

		// ABC music , MUP


		static char lyricsbuffer_[NUM_LYRICS][LYRICS_LINE_LENGTH];

		// LilyPond

		static struct lily_properties lilyProperties_;



		//  GENERAL

		//  Autosave
		static bool autosaveEnable_;
		static unsigned int autosaveInterval_;
		static unsigned int turnOverPoint_;

		//  Startup
		static bool startupLoadLastScore_;


		//  showStaffNrs_, showStaffNames_, showAuxLines_, showContext_ and showDrumToolbar_ are cached versions
		//  kapp->config()->readBoolEntry(QString("ShowStaffNrs"), true),
		//  kapp->config()->readBoolEntry(QString("ShowStaffNames"), true),
		//  kapp->config()->readBoolEntry(QString("ShowAuxLines"), true) and
		//  kapp->config()->readBoolEntry(QString("ShowDrumToolbar"), true)
		//  respectively. They are cached because acces to booleans ought to be
		//  faster than the access method above. This extra cache layer is used
		//  because the values are read in drawing code that is executed many
		//  times. It may not make a difference unless used on slow computers with
		//  large datasets.
		//
		//  Note that the calls to kapp->config()->readBoolEntry are already
		//  cached by KConfig so there is no disk access involved in every call.
		//  still this extra chache layer shold increase performance.
		//
		//  Some other values than the three above may be chached as well.
		static bool showStaffNrs_, showStaffNames_;
		static bool autoBeamInsertion_, allowKeyboardInsert_, allowInsertEcho_;
		static bool moveAccKeysig_, showAuxLines_, showDrumToolbar_, showContext_;
		static bool automaticBarInsertion_;
		static bool useMidiPedal_;
		static int defMidiPort_;
		static bool midiPortSet_;
		static int underlength_, overlength_;
		static int schedulerRequest_;
		static int defZoomval_; 
		static staff_props_str nullprops_;
		static NKeySig *nullKeySig_;
		static NClef *nullClef_;
		static QList<NMainWindow> windowList_;
		static NMidiMapper *mapper_;
		static int lastWindowX_, lastWindowY_;
		static NVoice *voiceWithSelectedRegion_;
		static NMainFrameWidget *windowWithSelectedRegion_;
		static bool isGrabbed_;
		static int numOfMultiStaffs_;
		static KProgress *progress_;
		static QString lyrics_[5];
		static bool dontShowMupWarnings_;
#if KDE_VERSION < 220
		static int tipNo_;
		static QList<QString> theTips_;
		void loadTips(QString fname);
#endif
		static bool *staffSelMute_;
		static bool *staffSelAutobar_;
		static bool *staffSelAutobeam_;
		static bool *staffSelMerge_;
		static bool *staffSelTrack_;
		static bool *staffSelExport_;
		static bool *staffSelMulti_;
		static QTimer autoSaveTimer_;
		static bool commandLine_;
		static int globalNoteNames_;
		static int globalMaj7_;
		static int globalFlatPlus_;
		static QString userpath_;
		static QString musixScript_;

		/* orchestral bars (only visible part) */

		static int yPosOfOrchestralBar(int bar_xpos);
		static void resetBarCkeckArray(int newYpos, bool clear);
		static char *volume[];
		static char *noteVal[];
		static char *tripletVal[];
		static char *instrTab[];
	private:
		bool loadPixmaps(QPixmap **black_pixmap, QPixmap **red_pixmap, QString fname);
		bool loadAlternativePixmap(QPixmap **grey_pixmap, QString fname, QString suffix);
		QCursor *loadCursor(char *fname);
		static QRegExp germanAE_;
		static QRegExp germanOE_;
		static QRegExp germanUE_;
		static QRegExp germanae_;
		static QRegExp germanoe_;
		static QRegExp germanue_;
		static QRegExp germanss_;
		static int barCheckArray_[LENGTH_OF_BAR_CHECK_ARRAY];
		static int barCkeckIdx_;
		static int yPosOfBarEnd_;
		static int newYpos_;
	private slots:
		void autosave();
};


#endif // RESOURCE_H
