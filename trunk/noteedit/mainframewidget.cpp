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

#include <stdlib.h>
#include <unistd.h>
#include <qapplication.h>
#include <qpopupmenu.h>
#include <qkeycode.h>
#include <kapp.h>
#include <kmessagebox.h>
#include <kstddirs.h>
#include <kkeydialog.h>
#if KDE_VERSION >= 220
#include <ktip.h>
#endif
#if KDE_VERSION >= 290 
#include <kaccel.h>
#endif
#if QT_VERSION >= 300
#include <qcursor.h>
#endif
#include <qpainter.h>
#include <qslider.h>
#include <qtabwidget.h>
#include <kprogress.h>
#include <qtabwidget.h>
#include <qcheckbox.h>
#include <qlineedit.h>
#include <qmultilineedit.h>
#include "mainframewidget.h"
#include "configuredialog.h"
#include "tupletdialog_impl.h"
#include "resource.h"
#include "scaleedit_impl.h"
#include "midimapper.h"
#include "transpainter.h"
#include "clef.h"
#include "scoreinfo.h"
#include "sign.h"
#include "keysig.h"
#include "timesig.h"
#include "keyoffs.h"
#include "filehandler.h"
#include "midiexport.h"
#include "musixtex.h"
#include "pmxexport.h"
#include "lilyexport.h"
#include "musicxmlexport.h"
#include "dbufferwidget.h"
#include "numberdisplay.h"
//#include "clefdialog.h"
#include "staff.h"
#include "lyrics.h"
#include "zoomselection.h"
#include "uiconnect.h"
#include "voicedialog.h"
#include "notesel.h"
#include "chord.h"
#include "text.h"
#include "textDialog_impl.h"
#include "chorddiagram.h"
#include "layout.h"
#include "abcexport.h"
#include "staffPropFrm.h"
#ifdef WITH_TSE3
#include "tse3handler.h"
#endif
// Before Qt 3.0, qtextstream.h (included by qxml.h (included by musicxmlimport.h))
// conflicts with std libs. QT_ALTERNATE_QTSMANIP disables the conflicting definitions
#if QT_VERSION < 300
#define QT_ALTERNATE_QTSMANIP
#endif
#include "musicxmlimport.h"
#include <kstdaction.h>
#include <klocale.h>
#include <kwin.h>
#include <kfiledialog.h>
#include "../kguitar_excerpt/chord.h"
// KToggleAction has setChecked, isChecked, and activate()
// for what QPushButton calls setOn, isOn, and toggle()
#define setOn setChecked
#define isOn isChecked
#define toggle activate

#define PUSHBUTTONWIDTH 40
#define PUSHBUTTONHEIGHT 30

// Not needed anymore
#define MENUBARHEIGHT 0
#define TOOLBARHEIGHT 0

#define QBHEIGHT 20
#define QBWIDTH 40
#define PUSH_DIST 2
#if QT_VERSION >= 300
#define SCROLLBARHEIGHT 16
#else
#define SCROLLBARHEIGHT 14
#endif
#define Y_SCROLL_DIST 100
#define BORDER 4
#define CUR_DIST 10


#define LINE_WIDTH 1
#define MID_LINE_WIDTH 1
#define RIGHT_BORDER 10
#define LISTWIDTH 250
#define KEYOFFSWIDTH 25
#define LIST_DIST 0
#define KEY_OFFS_LABEL_WIDTH 20
#define KEY_OFFS_LABEL_HEIGHT 20
#define KEY_OFFS_LABEL_DIST -20
#define RBORDER 10
#define Y_STAFF_BASE 40

#define SMALL_X_SCROLL 200
#define SMALL_X_SENS_DIST 150

#define SMALL_X_REC_SCROLL 35
#define SMALL_X_REC_SENS_DIST 30

#define STAFF_PORTION 2

#define OVERLENTH_CHECK_INTERVAL 1000

//export pages:

#define TOOL_ELEMENT_WIDTH	300
#define TOOL_ELEMENT_HEIGHT	70
#define ELEM_SPACE 16


const char *NMainFrameWidget::keySigTab_[15] =
{I18N_NOOP("C Major; a minor"),           I18N_NOOP("F Major; d minor"),
 I18N_NOOP("B flat Major; g minor"),      I18N_NOOP("E flat Major; c minor"),
 I18N_NOOP("A flat Major; f minor"),      I18N_NOOP("D flat Major; b flat minor"),
 I18N_NOOP("G flat Major; e flat minor"), I18N_NOOP("C flat Major; a flat minor"),
 I18N_NOOP("G Major; e minor"),           I18N_NOOP("D Major; b minor"),
 I18N_NOOP("A Major; f sharp minor"),     I18N_NOOP("E Major; c sharp minor"),
 I18N_NOOP("B Major; g sharp minor"),     I18N_NOOP("F sharp Major; d sharp minor"),
 I18N_NOOP("C sharp Major; a sharp minor")
};


void NMainFrameWidget::add_time(struct timeval *res, struct timeval *now, long msecs) {
	res->tv_sec  = now->tv_sec;
	res->tv_usec = now->tv_usec + msecs * 1000;
	res->tv_sec += res->tv_usec / (unsigned long) 1000000;
	res->tv_usec = res->tv_usec % 1000000;
}
		
/*!Computes the time in milliseconds between 2 values of type struct timeval
 * @param future minuend
 * @param now subtrahend
 * @return the time between future and now in milliseconds
 */

#define PREPARE_BAR_CHECK_ARRAY(staff_var, staff_nr, bool_connect_var, bool_first_var, work_var) \
			bool_connect_var = false; \
			for (work_var = 0; !bool_connect_var && work_var < staffCount_; work_var++) { \
				if (barCont_[work_var].valid && staff_nr >= barCont_[work_var].beg && staff_nr < barCont_[work_var].end) { \
					bool_connect_var = true; \
				} \
			} \
			if (bool_connect_var) { \
				if (bool_first_var) { \
					NResource::resetBarCkeckArray(staff_var->getBase()+4*LINE_DIST, true); \
					bool_first_var = false; \
				} \
				else { \
					NResource::resetBarCkeckArray(staff_var->getBase()+4*LINE_DIST, false); \
				} \
			} \
			else { \
				if (bool_first_var) { \
					NResource::resetBarCkeckArray(-1, true); \
					bool_first_var = false; \
				} \
				else { \
					NResource::resetBarCkeckArray(-1, false); \
				} \
			}

unsigned long NMainFrameWidget::sub_time(struct timeval *future, struct timeval *now) {
	if (timercmp(future, now, <=)) return 0;
	
	return (future->tv_sec - now->tv_sec) * 1000 + (future->tv_usec - now->tv_usec) / 1000;
}

NMainFrameWidget::NMainFrameWidget (KActionCollection *actObj, bool inPart, QWidget *parent, const char *name) :
    QWidget (parent, name) {
	int i;

	inPart_ = inPart;
#ifdef WITH_TSE3
	tse3Handler_ = new NTSE3Handler(this);
	connect(tse3Handler_, SIGNAL(endRecorded(bool)), this, SLOT(completeRecording(bool)));
	connect(tse3Handler_, SIGNAL(endTSE3toScore(bool)), this, SLOT(completeTSE3toScore(bool)));
#endif
	genPropDialog_ = new propFrm(this);
	lyricsDialog_ = new lyricsFrm( this );
	chordDialog_ = new ChordSelector( this );
	// ----------- KDE user interface -------------
	// Create one action for each menu entry, with text, icon and keyboard shortcut
	// The layout of the menus is described in noteeditui.rc
	// The name of the action is what is matched against the actions in the XML file.
	m_actionCollection = actObj;
	KStdAction::open( this, SLOT(fileOpen()), actionCollection() );
	KStdAction::openNew( this, SLOT(newPaper()), actionCollection() );
	KStdAction::save( this, SLOT(fileSave()), actionCollection() );
	KStdAction::saveAs( this, SLOT(fileSaveAs()), actionCollection() );  
	KStdAction::undo( this, SLOT(undo()), actionCollection() );
	KStdAction::redo( this, SLOT(redo()), actionCollection() );
	KStdAction::zoomIn( this, SLOT(zoomIn()), actionCollection() );
	KStdAction::zoomOut( this, SLOT(zoomOut()), actionCollection() );
	KStdAction::preferences(this, SLOT(configure()), actionCollection() );
        m_recentFilesAction = KStdAction::openRecent( this, SLOT(fileOpenRecent(const KURL &)), actionCollection() );
        m_recentFilesAction->loadEntries( KGlobal::config() );
	new KAction( i18n("&Import MusicXML..."), 0, this, SLOT(importMusicXML()), actionCollection(), "import_musicxml" );
	new KAction( i18n("Export &MIDI..."), 0, this, SLOT(exportMidi()), actionCollection(), "export_midi" );
	new KAction( i18n("Import &MIDI..."), 0, this, SLOT(importMidi()), actionCollection(), "import_midi" );
	new KAction( i18n("Export Musi&XTeX..."), 0, this, SLOT(exportMusiXTeX()), actionCollection(), "export_musixtex" );
	new KAction( i18n("Export &PMX..."), 0, this, SLOT(exportPMX()), actionCollection(), "export_pmx" );
	new KAction( i18n("Export &ABC..."), 0, this, SLOT(exportABC()), actionCollection(), "export_abc" );
	new KAction( i18n("Export M&usicXML..."), 0, this, SLOT(exportMusicXML()), actionCollection(), "export_musicxml" );
	new KAction( i18n("&Output Params..."), 0, this, SLOT(setOutputParam()), actionCollection(), "set_params" );
	lilyPort_ = new KAction( i18n("Export &LilyPond..."), 0, this, SLOT(exportLilyPond()), actionCollection(), "export_lily" );
#ifdef WITH_DIRECT_PRINTING
	new KAction( i18n("Print pre&view..."), "print_preview", 0, this, SLOT(filePrintPreview()), actionCollection(), "print_preview");
	new KAction( i18n("&Print..."), "print", CTRL+Key_P, this, SLOT(filePrintNoPreview()), actionCollection(), "print" );
#endif
	new KAction( i18n("Score in&formation..."), "readme", 0, this, SLOT(scoreInfo()), actionCollection(), "score_information" );
	new KAction( i18n("&Key configuration"), "configure_shortcuts", 0, this, SLOT(keyConfig()), actionCollection(), "keyconfig" );
	new KAction( i18n("&Close"), "exit", 0, this, SLOT(quitDialog()), actionCollection(), "quit" );
	new KAction( i18n("&Quit"), 0, this, SLOT(closeAllWindows()), actionCollection(), "close_all" );
	KToggleAction *actionToggleBarNumbers = new KToggleAction
	  (i18n("Show &bar numbers"), 0, this, SLOT(toggleBarNumbers()), actionCollection(), "view_show_bar_numbers");
	actionToggleBarNumbers->setChecked(NResource::showStaffNrs_);
	KToggleAction *actionToggleStaffNames = new KToggleAction
	  (i18n("Show &staff names"), 0, this, SLOT(toggleStaffNames()), actionCollection(), "view_show_staff_names");
	actionToggleStaffNames->setChecked(NResource::showStaffNames_);
	KToggleAction *actionToggleAuxLines = new KToggleAction
	  (i18n("Show &aux lines"), 0, this, SLOT(toggleAuxLines()), actionCollection(), "view_show_aux_lines");
	actionToggleAuxLines->setChecked(NResource::showAuxLines_);
	KToggleAction *actionToggleContext = new KToggleAction
	  (i18n("Show staff &context"), 0, this, SLOT(toggleStaffContext()), actionCollection(), "view_staff_context");
	actionToggleContext->setChecked(NResource::showContext_);
	KToggleAction *actionToggleDrumToolBar = new KToggleAction
	  (i18n("Show &drum tools"), 0, this, SLOT(toggleDrumUp()), actionCollection(), "view_drum_tools");
	actionToggleDrumToolBar->setChecked(NResource::showDrumToolbar_);
	new KAction( i18n("Reduce &accidentals..."), 0, this, SLOT(redAccidentals()), actionCollection(), "tools_accidentals" );
	new KAction( i18n("&Collect notes..."), 0, this, SLOT(collChords()), actionCollection(), "tools_collect" );
	new KAction( i18n("Set all to sharp"), 0, this, SLOT(setAllSharp()), actionCollection(), "tools_set_sharp" );
	new KAction( i18n("Set all to flat"), 0, this, SLOT(setAllFlat()), actionCollection(), "tools_set_flat" );
	new KAction( i18n("&Transpose..."),  0, this, SLOT(transposeDialog()), actionCollection(), "tools_transpose" );
	new KAction( i18n("C&hange Clef..."),  0, this, SLOT(changeClefDialog()), actionCollection(), "tools_change_clef" );
	new KAction( i18n("T&uplet..."),  0, this, SLOT(createTuplet()), actionCollection(), "create_tuplet" );
	criticalButtons_.append
		(new KAction( i18n("Cle&f..."), "cleficon", 0, this, SLOT(clefDialog()), actionCollection(), "insert_clef"));
	criticalButtons_.append
		(new KAction( i18n("&Key signature..."), "keyicon", 0, this, SLOT(keyDialog()), actionCollection(), "insert_keysig" ));
	criticalButtons_.append
		(new KAction( i18n("Repeat - &Open"), "repopen", 0, this, SLOT(insertRepeatOpen()), actionCollection(), "insert_repeatopen"));
	criticalButtons_.append
		(new KAction( i18n("Re&peat- Open and Close"), "repopenclose", 0, this, SLOT(insertRepeatOpenClose()), actionCollection(), "insert_repeatopenclose"));
	criticalButtons_.append
		(new KAction( i18n("Repeat - &Close"), "repclose", 0, this, SLOT(insertRepeatClose()), actionCollection(), "insert_repeatclose"));
	criticalButtons_.append
		(new KAction( i18n("SpecialEnding&1"), 0, this, SLOT(insertspecEnding1()), actionCollection(), "insert_specialending1" ));
	criticalButtons_.append
		(new KAction( i18n("SpecialEnding&2"), 0, this, SLOT(insertspecEnding2()), actionCollection(), "insert_specialending2" ));
	criticalButtons_.append
		(new KAction( i18n("&Double Bar"), "doublebar", 0, this, SLOT(insertDoubleBar()), actionCollection(), "insert_double_bar"));
	criticalButtons_.append
		(new KAction( i18n("&End Bar"), "endbar", 0, this, SLOT(insertEndBar()), actionCollection(), "insert_end_bar"));
	criticalButtons_.append
		(new KAction( i18n("Time si&gnature..."), "timeicon", 0, this, SLOT(timesigDialog()), actionCollection(), "insert_timesig" ));
	criticalButtons_.append
		(new KAction( i18n("Tempo &signature..."), 0, this, SLOT(tempoSigDialog()), actionCollection(), "insert_temposig" ));
	criticalButtons_.append
		(new KAction( i18n("&Volume change..."), "speaker", 0, this, SLOT(volChangeDialog()), actionCollection(), "insert_volumechange"));
	criticalButtons_.append
		(new KAction( i18n("&Instrument change..."), "voichange", 0, this, SLOT(voiceChangeDialog()), actionCollection(), "insert_voicechange"));
	criticalButtons_.append
		(new KAction( i18n("C&hord..."), 0, this, SLOT(chordDialog()), actionCollection(), "insert_chord"));
	criticalButtons_.append
		(new KAction( i18n("&Line..."), 0, this, SLOT(insertLine()), actionCollection(), "insert_line" )); // HINT: The associated function is located in mainframewidget2.cpp
	criticalButtons_.append
		(new KAction( i18n("&Text ..."), 0, this, SLOT(insertText()), actionCollection(), "insert_text" )); 
	criticalButtons_.append
		(new KAction( i18n("&Segno"), 0, this, SLOT(insertSegno()), actionCollection(), "insert_segno" ));
	criticalButtons_.append
		(new KAction( i18n("&dal Segno"), 0, this, SLOT(insertDalSegno()), actionCollection(), "insert_dal_segno" ));
	criticalButtons_.append
		(new KAction( i18n("dal Segno &al Fine"), 0, this, SLOT(insertDalSegnoAlFine()), actionCollection(), "insert_dal_segno_al_fine" ));
	criticalButtons_.append
		(new KAction( i18n("dal Segno al C&oda"), 0, this, SLOT(insertDalSegnoAlCoda()), actionCollection(), "insert_dal_segno_al_coda" ));
	criticalButtons_.append
		(new KAction( i18n("&Fine"), 0, this, SLOT(insertFine()), actionCollection(), "insert_fine" ));
	criticalButtons_.append
		(new KAction( i18n("&Coda"), 0, this, SLOT(insertCoda()), actionCollection(), "insert_coda" ));
	criticalButtons_.append
		(new KAction( i18n("&Ritardando"), 0, this, SLOT(insertRitardando()), actionCollection(), "insert_ritardando" ));
	criticalButtons_.append
		(new KAction( i18n("&Accelerando"), 0, this, SLOT(insertAccelerando()), actionCollection(), "insert_accelerando" ));
	new KAction( i18n("&Multimeasure rest..."),  0, this, SLOT(multiRestDialog()), actionCollection(), "insert_multi_rest" );
	new KAction( i18n("&New staff"), "filenew", 0, this, SLOT(newStaff()), actionCollection(), "staff_new" );
	new KAction( i18n("&Delete staff"), "editdelete", 0, this, SLOT(deleteStaff()), actionCollection(), "staff_delete" );
	new KAction( i18n("&Move staff..."), "editcopy", 0, this, SLOT(staffMoveDialog()), actionCollection(), "staff_move" );
	new KAction( i18n("M&ute staffs..."), 0, this, SLOT(muteDialog()), actionCollection(), "staff_mute" );
	new KAction( i18n("&Voices..."), 0, this, SLOT(voiceDialog()), actionCollection(), "voice_dialog" );
	new KAction( i18n("&Staff properties..."), "configure", 0, this, SLOT(setStaffProperties()), actionCollection(), "staff_properties" );
	new KAction( i18n("Score &layout..."), 0, this, SLOT(layoutDialog()), actionCollection(), "layout_dialog" );
	criticalButtons_.append
		(new KAction( i18n("&Lyrics..."), "history", 0, this, SLOT(showLyricsDialog()), actionCollection(), "staff_lyrics" ));
#ifdef WITH_TSE3
	new KAction( i18n("&Import recording"), "folder_sound", 0, this, SLOT(importRecording()), actionCollection(), "staff_importrec" );
#endif
	new KAction( i18n("Select &Multi Staff..."),  0, this, SLOT(multiStaffDialog()), actionCollection(), "edit_multistaff" );
	new KAction( i18n("&Goto measure..."), "goto", 0, this, SLOT(gotoDialog()), actionCollection(), "edit_goto" );
	criticalButtons_.append
		(new KAction( i18n("&Auto Bar..."),  0, this, SLOT(autoBar()), actionCollection(), "edit_autobar" ));
	//new KAction( i18n("Auto&beam..."),  0, this, SLOT(autoBeamDialog()), actionCollection(), "edit_autobeam" );
	new KAction( i18n("Auto &Beam..."),  0, this, SLOT(doAutoBeam()), actionCollection(), "edit_autobeam" );
	new KAction( i18n("&Cleanup rests..."), 0, this, SLOT(cleanRestsDialog()), actionCollection(), "edit_cleanuprests" );
	new KAction( i18n("&Set N time repeat..."), "repntimes", 0, this, SLOT(repeatCountDialog()), actionCollection(), "set_ntime_repeat");
	// TODO add Redo/Cut/Copy/Paste to the Edit menu
#ifdef WITH_TSE3
	new KAction( i18n("&Create TSE3 song"), "Wizard", 0, this, SLOT(createTSE3()), actionCollection(), "tse3_createsong" );
	new KAction( i18n("&Play TSE3 song"), "1rightarrow", 0, this, SLOT(playSong()), actionCollection(), "tse3_playsong" );
	new KAction( i18n("&Read TSE3 song"), "filefind", 0, this, SLOT(readTSE3()), actionCollection(), "tse3_readsong" );
	new KAction( i18n("&Write TSE3 song"), "filesaveas", 0, this, SLOT(writeTSE3()), actionCollection(), "tse3_writesong" );
	new KAction( i18n("MIDI &out..."), "midi", 0, this, SLOT(TSE3MidiOut()), actionCollection(), "tse3_midiout" );
	new KAction( i18n("MIDI &in..."), "fileopen", 0, this, SLOT(TSE3MidiIn()), actionCollection(), "tse3_midiin" );
	new KAction( i18n("&Filter dialog..."), "filter", 0, this, SLOT(TSE3Filter()), actionCollection(), "tse3_filterdlg" );
	new KAction( i18n("Inform&ation..."), "help", 0, tse3Handler_, SLOT(printSongInformation()), actionCollection(), "tse3_information" );
	new KAction( i18n("&Merge..."), "view_right", 0, tse3Handler_, SLOT(TSE3Merge()), actionCollection(), "tse3_merge" );
	new KAction( i18n("TSE3 --> &Score"), 0, this, SLOT(TSE3toScore()), actionCollection(), "tse3_score" );
#endif
	new KAction( i18n("New &window"), "window_new", 0, this, SLOT(openNewWindow()), actionCollection(), "window_new_window" );
	new KAction( i18n("Tip of the &day"), "idea", 0, this, SLOT(showTipOfTheDay()), actionCollection(), "help_tipoftheday" );

	// ------------ Actions for toolbar buttons, KDE interface --------
	selectbutton_ = new KToggleAction(i18n("Select mode"), "selector", 0, actionCollection(), "select");
	note_buttons_[0] = new KToggleAction(i18n("Breve"), "breve", 0, actionCollection(), "breve");
	note_buttons_[1] = new KToggleAction(i18n("Full note"), "fullnote", 0, actionCollection(), "full");
	note_buttons_[2] = new KToggleAction(i18n("Half note"), "halfnote", 0, actionCollection(), "half");
	note_buttons_[3] = new KToggleAction(i18n("Quarter note"), "quarternote", 0, actionCollection(), "quarter");
	note_buttons_[4] = new KToggleAction(i18n("Eighth note"), "eightnote", 0, actionCollection(), "n8");
	note_buttons_[5] = new KToggleAction(i18n("Sixteenth note"), "sixteenthnote", 0, actionCollection(), "n16");
	note_buttons_[6] = new KToggleAction(i18n("32nd note"), "32ndnote", 0, actionCollection(), "n32");
	note_buttons_[7] = new KToggleAction(i18n("64th note"), "64thnote", 0, actionCollection(), "n64");
	note_buttons_[8] = new KToggleAction(i18n("128th note"), "128thnote", 0, actionCollection(), "n128");
	note_buttons_[9] = new KToggleAction(i18n("Grace Eighth"), "tinyeight", 0, actionCollection(), "tn8");
	note_buttons_[10] = new KToggleAction(i18n("Grace sixteenth"), "tinysixteenth", 0, actionCollection(), "tn16");
	note_buttons_[11] = new KToggleAction(i18n("Grace Eight Stroken"), "tinystroke", 0, actionCollection(), "tns32");
	selectbutton_->setExclusiveGroup( "notegrp" );
	for ( i = 0; i < COUNT_CHORDBUTTONS; ++i)
	    note_buttons_[i]->setExclusiveGroup( "notegrp" );
		
	dotbutton_ = new KToggleAction(i18n("Dotted note"), "dottednote", 0, actionCollection(), "dot");
	dotbutton_->setExclusiveGroup("dotgrp");
	ddotbutton_ = new KToggleAction(i18n("DoubleDotted note"), "doubledottednote", 0, actionCollection(), "ddot");
	ddotbutton_->setExclusiveGroup("dotgrp");
	tiebutton_ = new KToggleAction(i18n("Tie"), "tiednote", 0, actionCollection(), "tie");
	beambutton_ = new KToggleAction(i18n("Beam"), "beamednote", 0, actionCollection(), "beam");
	slurbutton_ = new KToggleAction(i18n("Slur"), "slurednote", 0, actionCollection(), "slur");
	tripletbutton_ = new KToggleAction(i18n("Triplet"), "tripletnotes", 0, actionCollection(), "triplet");
	hiddenrestbutton_ = new KToggleAction(i18n("Hidden rests"), "hiddenrest", 0, actionCollection(), "hrest");
	staccatobutton_ = new KToggleAction(i18n("Staccato"), "staccatonote", 0, actionCollection(), "staccato");
	staccatobutton_->setExclusiveGroup("accgrp");
	sforzatobutton_ = new KToggleAction(i18n("Sforzato"), "sforzatonote", 0, actionCollection(), "sforzato");
	sforzatobutton_->setExclusiveGroup("accgrp");
	portatobutton_ = new KToggleAction(i18n("Portato"), "portatonote", 0, actionCollection(), "portato");
	portatobutton_->setExclusiveGroup("accgrp");
	fermatebutton_ = new KToggleAction(i18n("Fermata"), "fermatenote", 0, actionCollection(), "fermate");
	fermatebutton_->setExclusiveGroup("accgrp");
	arpeggbutton_ = new KToggleAction(i18n("Arpeggio"), "arpegg", 0, actionCollection(), "arpegg");
	arpeggbutton_->setExclusiveGroup("accgrp");
	pedonbutton_ = new KToggleAction(i18n("Pedal on"), "pedonicon", 0, actionCollection(), "insert_pedal_on");
	pedonbutton_->setExclusiveGroup("accgrp");
	pedoffbutton_ = new KToggleAction(i18n("Pedal off"), "pedofficon", 0, actionCollection(), "insert_pedal_off");
	pedoffbutton_->setExclusiveGroup("accgrp");
	strong_pizzicatobutton_ = new KToggleAction(i18n("Strong pizzicato"), "strpizzicatonote", 0, actionCollection(), "strong_pizzicato");
	strong_pizzicatobutton_->setExclusiveGroup("accgrp");
	sforzandobutton_ = new KToggleAction(i18n("Sforzando"), "sforzandonote", 0, actionCollection(), "sforzando");
	sforzandobutton_->setExclusiveGroup("accgrp");

	offs_buttons_[0] = new KToggleAction(i18n("Sharp"), "cross", 0, actionCollection(), "sharp");
	offs_buttons_[0]->setExclusiveGroup("offsgrp");
	offs_buttons_[1] = new KToggleAction(i18n("Flat"), "flat", 0, actionCollection(), "flat");
	offs_buttons_[1]->setExclusiveGroup("offsgrp");
	offs_buttons_[2] = new KToggleAction(i18n("Natural"), "natur", 0, actionCollection(), "natural");
	offs_buttons_[2]->setExclusiveGroup("offsgrp");
	offs_buttons_[3] = new KToggleAction(i18n("Double sharp"), "doublecross", 0, actionCollection(), "dsharp");
	offs_buttons_[3]->setExclusiveGroup("offsgrp");
	offs_buttons_[4] = new KToggleAction(i18n("Double flat"), "doubleflat", 0, actionCollection(), "dflat");
	offs_buttons_[4]->setExclusiveGroup("offsgrp");

	playbutton_ = new KToggleAction(i18n("Play"), "1rightarrow", 0, actionCollection(), "play");

	// plug undo here

	kbbutton_ = new KToggleAction(i18n("Keyboard") /* ????? */, "kbicon", 0, actionCollection(), "kb");
	kbInsertButton_ = new KToggleAction(i18n("Keyboard insert") /* ????? */, "redkbicon", 0, actionCollection(), "kbi");
#ifdef WITH_TSE3
	recordButton_ = new KToggleAction(i18n("Record"), "recordicon", 0, actionCollection(), "rec");
#endif
	editbutton_ = new KToggleAction(i18n("Edit mode"), "editicon", 0, actionCollection(), "edit");
	allowKbInsertButton_ = new KToggleAction(i18n("Insert from keyboard"), "singlekey",  0, actionCollection(), "kbinsert");
	gluebutton_ = new KToggleAction(i18n("Glue"), "rivet", 0, actionCollection(), "glue");
	stemUpbutton_ = new KToggleAction(i18n("Stem up"), "stemupicon", 0, actionCollection(), "stemUp");
	stemDownbutton_ = new KToggleAction(i18n("Stem down"), "stemdownicon", 0, actionCollection(), "stemDown");

	allowKbInsertButton_->setOn(NResource::allowKeyboardInsert_);

	crossDrumBu_ = new KToggleAction(i18n("Cross notehead style"), "perccross", 0, actionCollection(), "cross_drum");
	crossDrumBu_->setExclusiveGroup( "note_body_group" );
	cross2DrumBu = new KToggleAction(i18n("Plus notehead style"), "perccross2", 0, actionCollection(), "cross2_drum");
	cross2DrumBu->setExclusiveGroup( "note_body_group" );
	crossCricDrumBu_ = new KToggleAction(i18n("Cross-Circle notehead style"), "perccrosscirc", 0, actionCollection(), "cross_cric_drum");
	crossCricDrumBu_->setExclusiveGroup( "note_body_group" );
	rectDrumBu_ = new KToggleAction(i18n("Slash notehead style"), "percrect", 0, actionCollection(), "rect_drum");
	rectDrumBu_->setExclusiveGroup( "note_body_group" );
	triaDrumBu_ = new KToggleAction(i18n("Triangle notehead style"), "perctrian", 0, actionCollection(), "tria_drum");
	triaDrumBu_->setExclusiveGroup( "note_body_group" );
	
	connect(note_buttons_[0],  SIGNAL(toggled(bool)), this, SLOT(setToDFull(bool)));
	connect(note_buttons_[1],  SIGNAL(toggled(bool)), this, SLOT(setToFull(bool)));
	connect(note_buttons_[2],  SIGNAL(toggled(bool)), this, SLOT(setToHalf(bool)));
	connect(note_buttons_[3],  SIGNAL(toggled(bool)), this, SLOT(setToQuarter(bool)));
	connect(note_buttons_[4],  SIGNAL(toggled(bool)), this, SLOT(setToN8(bool)));
	connect(note_buttons_[5],  SIGNAL(toggled(bool)), this, SLOT(setToN16(bool)));
	connect(note_buttons_[6],  SIGNAL(toggled(bool)), this, SLOT(setToN32(bool)));
	connect(note_buttons_[7],  SIGNAL(toggled(bool)), this, SLOT(setToN64(bool)));
	connect(note_buttons_[8],  SIGNAL(toggled(bool)), this, SLOT(setToN128(bool)));
	connect(note_buttons_[9],  SIGNAL(toggled(bool)), this, SLOT(setToGN8(bool)));
	connect(note_buttons_[10], SIGNAL(toggled(bool)), this, SLOT(setToGN16(bool)));
	connect(note_buttons_[11], SIGNAL(toggled(bool)), this, SLOT(setToGNS8(bool)));

	connect(offs_buttons_[0], SIGNAL(toggled(bool)), this, SLOT(setCross(bool)));
	connect(offs_buttons_[1], SIGNAL(toggled(bool)), this, SLOT(setFlat(bool)));
	connect(offs_buttons_[2], SIGNAL(toggled(bool)), this, SLOT(setNatur(bool)));
	connect(offs_buttons_[3], SIGNAL(toggled(bool)), this, SLOT(setDCross(bool)));
	connect(offs_buttons_[4], SIGNAL(toggled(bool)), this, SLOT(setDFlat(bool)));
	connect(&autoscrollTimer_, SIGNAL(timeout()), this, SLOT(autoscroll()));

	connect(playbutton_, SIGNAL(toggled(bool)), this, SLOT(playAll(bool)));
	connect(stemUpbutton_, SIGNAL(toggled(bool)), this, SLOT(setStemUp(bool)));
	connect(stemDownbutton_, SIGNAL(toggled(bool)), this, SLOT(setStemDown(bool)));

	connect(dotbutton_, SIGNAL(toggled(bool)), this, SLOT(setDotted(bool)));
	connect(ddotbutton_, SIGNAL(toggled(bool)), this, SLOT(setDDotted(bool)));
	connect(selectbutton_, SIGNAL(toggled(bool)), this, SLOT(setSelectMode()));
	connect(editbutton_, SIGNAL(toggled(bool)), this, SLOT(setEditMode(bool)));
	connect(allowKbInsertButton_, SIGNAL(toggled(bool)), this, SLOT(allowKbInsert(bool)));
	connect(kbbutton_, SIGNAL(toggled(bool)), this, SLOT(setKbMode(bool)));
	connect(kbInsertButton_, SIGNAL(toggled(bool)), this, SLOT(setKbInsertMode(bool)));
#ifdef WITH_TSE3
	connect(recordButton_, SIGNAL(toggled(bool)), this, SLOT(TSE3record(bool)));
#endif
	connect(tiebutton_, SIGNAL(toggled(bool)), this, SLOT(setActualTied(bool)));
	connect(staccatobutton_, SIGNAL(toggled(bool)), this, SLOT(setStaccato(bool)));
	connect(sforzatobutton_, SIGNAL(toggled(bool)), this, SLOT(setSforzato(bool)));
	connect(portatobutton_, SIGNAL(toggled(bool)), this, SLOT(setPortato(bool)));
	connect(strong_pizzicatobutton_, SIGNAL(toggled(bool)), this, SLOT(setStrong_pizzicato(bool)));
	connect(sforzandobutton_, SIGNAL(toggled(bool)), this, SLOT(setSforzando(bool)));
	connect(fermatebutton_, SIGNAL(toggled(bool)), this, SLOT(setFermate(bool)));
	connect(arpeggbutton_, SIGNAL(toggled(bool)), this, SLOT(setArpegg(bool)));
	connect(pedonbutton_, SIGNAL(toggled(bool)), this, SLOT(setPedalOn(bool)));
	connect(pedoffbutton_, SIGNAL(toggled(bool)), this, SLOT(setPedalOff(bool)));
	connect(beambutton_, SIGNAL(toggled(bool)), this, SLOT(setBeamed(bool)));
	connect(slurbutton_, SIGNAL(toggled(bool)), this, SLOT(setSlured(bool)));
	connect(tripletbutton_, SIGNAL(toggled(bool)), this, SLOT(setTriplet(bool)));
	connect(hiddenrestbutton_, SIGNAL(toggled(bool)), this, SLOT(setHidden(bool)));
	connect(crossDrumBu_, SIGNAL(toggled(bool)), this, SLOT(setCrossBody(bool)));
	connect(cross2DrumBu, SIGNAL(toggled(bool)), this, SLOT(setCross2Body(bool)));
	connect(crossCricDrumBu_, SIGNAL(toggled(bool)), this, SLOT(setCrossCircBody(bool)));
	connect(rectDrumBu_, SIGNAL(toggled(bool)), this, SLOT(setRectBody(bool)));
	connect(triaDrumBu_, SIGNAL(toggled(bool)), this, SLOT(setTrianBody(bool)));
	editMode_ = false;
	notePart_ = new NDbufferWidget(this, (char *)"notepart");
	scrollx_ = new QScrollBar(QScrollBar::Horizontal, this, "scrollx");
	connect(scrollx_, SIGNAL(valueChanged(int)), this , SLOT(xscrollFromWidget(int)));
	scrolly_ = new QScrollBar(QScrollBar::Vertical, this, "scrolly");
	connect(scrolly_, SIGNAL(valueChanged(int)), this , SLOT(yscroll(int)));
	width_ = height_ = 0;
	tmpKeysig_ = 0;	
	paperScrollWidth_ = paperScrollHeight_ = 0;
	paperWidth_ = paperHeight_ = 0;
	tempo_ = DEFAULT_TEMPO;
	lastBarNr_ = 0;
	nextStaffElemToBePainted_ = 0;
	nextStaffNr_ = 0;
	nextStaffIsFirstStaff_ = true;
	fhandler_ = new NFileHandler();
	musicxmlFileReader_ = new MusicXMLParser();
	lilyexport_ = new NLilyExport();
	exportDialog_ = new exportFrm( this );
	saveParametersDialog_ = new saveParametersFrm( this );
	tupletDialog_ = new tupletDialogImpl( this );
	setEdited(false);
	NVoice::resetUndo();
	actualOffs_ = UNDEFINED_OFFS;
	leftx_ = 0;
	topy_ = 0;
	voiceNr_ = 0;
	lastYHeight_ = 0;
	help_x1_ = dummy_note_y_ = -1;
	lastXpos_ = oldLastXpos_ = 0;
	main_props_.lastMidiTime = 0;
	staffList_.append(currentStaff_ = new NStaff(Y_STAFF_BASE +  NResource::overlength_, 0, 0, this));
	voiceList_.append(currentVoice_ = currentStaff_->getVoiceNr(0));
	enableCriticalButtons(true);
	main_props_.voiceDisplay =
	voiceDisplay_ = new NNumberDisplay(0, currentStaff_->voiceCount(), i18n("Selected voice"), 0, actionCollection(), "voicenumber");
	connect(voiceDisplay_, SIGNAL(valChanged(int)), this, SLOT(changeActualVoice(int)));
	currentStaff_->setActual(true);
	lastYHeight_ = 50 + (4+LINE_OVERFLOW/2)*LINE_DIST;
	staffCount_ = 1;
	selectedSign_ = 0;
	playing_ = false;
	changeZoomValue(NResource::defZoomval_);
	computeMidiTimes(false);
	reposit();

	listDialog_ = new listFrm( this );

	zoomselect_ = new NZoomSelection(i18n("Zoom"), 0, actionCollection(), "zoomval", this);

	multistaffDialog_ = new staffFrm( this );

	staffPropFrm_ = new staffPropFrm( this );
	cleanUpRestsDialog_ = new smallestRestFrm( this );

	clefDialog_ = new staffelFrm( this );

	keyDialog_ = new QDialog();
	keyDialog_->setCaption(kapp->makeStdCaption(i18n("Key")));
	keyList_ = new QListBox(keyDialog_);
	keyOkButton_ = new QPushButton(i18n("&OK"), keyDialog_);
	keyOkButton_->setFocus();
	keyCancButton_ = new QPushButton(i18n("&Cancel"), keyDialog_);
	offs_list_[0] = new NKeyOffs("C", 0, keyDialog_, "Coffs");
	offs_list_[1] = new NKeyOffs("D", 1, keyDialog_, "Doffs");
	offs_list_[2] = new NKeyOffs("E", 2, keyDialog_, "Eoffs");
	offs_list_[3] = new NKeyOffs("F", 3, keyDialog_, "Foffs");
	offs_list_[4] = new NKeyOffs("G", 4, keyDialog_, "Goffs");
	offs_list_[5] = new NKeyOffs("A", 5, keyDialog_, "Aoffs");
	offs_list_[6] = new NKeyOffs("B", 6, keyDialog_, "Boffs");
	crosslabel_ = new QLabel(keyDialog_);
	flatlabel_ = new QLabel(keyDialog_);
	naturlabel_ = new QLabel(keyDialog_);

	crosslabel_->setPixmap(*NResource::crossIcon_);
	flatlabel_->setPixmap(*NResource::flatIcon_);
	naturlabel_->setPixmap(*NResource::naturIcon_);

	connect(keyList_, SIGNAL(highlighted(int)), this, SLOT(changeKey(int)));
	connect(keyOkButton_, SIGNAL(clicked()), this, SLOT(setInsertionKey()));
	connect(keyCancButton_, SIGNAL(clicked()), keyDialog_, SLOT(hide()));

	for (i = 0; i < 15; ++i) keyList_->insertItem(i18n(keySigTab_[i]));

	scaleFrm_ = new scaleFrm( this );
	volChangeDialog_ = new volumeFrm( this );

	timesigDialog_ = new timesigDiaFrm ( this );

	keys_ = new KAccel(this);

	keys_->insertItem( i18n( "Move up" ), "KEmoveup",  Key_Up);
	keys_->connectItem( "KEmoveup", this, SLOT( KE_moveUp() ) );
	keys_->insertItem( i18n( "Move down" ), "KEmovedown",  Key_Down);
	keys_->connectItem( "KEmovedown", this, SLOT( KE_moveDown() ) );
	keys_->insertItem( i18n( "Move semi up" ), "KEmoveSemiUp",  CTRL+Key_Up);
	keys_->connectItem( "KEmoveSemiUp", this, SLOT( KE_moveSemiUp() ) );
	keys_->insertItem( i18n( "Move semi down" ), "KEmoveSemiDown",  CTRL+Key_Down);
	keys_->connectItem( "KEmoveSemiDown", this, SLOT( KE_moveSemiDown() ) );
	keys_->insertItem( i18n( "Move left" ), "KEmoveleft",  Key_Left);
	keys_->connectItem( "KEmoveleft", this, SLOT( KE_moveLeft() ) );
	keys_->insertItem( i18n( "Move start" ), "KEmovestart",  Key_Home);
	keys_->connectItem( "KEmovestart", this, SLOT( KE_moveStart() ) );
	keys_->insertItem( i18n( "Move right" ), "KEmoveright",  Key_Right);
	keys_->connectItem( "KEmoveright", this, SLOT( KE_moveRight() ) );
	keys_->insertItem( i18n( "Move end" ), "KEmoveend",  Key_End);
	keys_->connectItem( "KEmoveend", this, SLOT( KE_moveEnd() ) );
	keys_->insertItem( i18n( "Delete forward" ), "KEdelete",  Key_Delete);
	keys_->connectItem( "KEdelete", this, SLOT( KE_delete() ) );
	keys_->insertItem( i18n( "Delete chord note" ), "KEremovechordnote", CTRL+Key_Delete);
	keys_->connectItem( "KEremovechordnote", this, SLOT( KE_removechordnote() ) );
	keys_->insertItem( i18n( "Delete before" ), "KEremove", Key_Backspace);
	keys_->connectItem( "KEremove", this, SLOT( KE_remove() ) );
	keys_->insertItem( i18n( "Toggle play" ), "KEplay", Key_Space);
	keys_->connectItem( "KEplay", this, SLOT( KE_play() ) );
	keys_->insertItem( i18n( "Leave current mode" ), "KEleave", Key_Escape);
	keys_->connectItem( "KEleave", this, SLOT( KE_leaveCurrentMode() ) );	
	keys_->insertItem( i18n( "Toggle edit" ), "KEedit",  Key_E);
	keys_->connectItem( "KEedit", this, SLOT( KE_edit() ) );
	keys_->insertItem( i18n( "Insert chord note" ), "KEinsertchordnote",  CTRL+Key_Return);
	keys_->connectItem( "KEinsertchordnote", this, SLOT( KE_insertchordnote() ) );
	keys_->insertItem( i18n( "Insert note" ), "KEinsertnote",  Key_Return);
	keys_->connectItem( "KEinsertnote", this, SLOT( KE_insertnote() ) );
	
	/* note length selection keys */
	keys_->insertItem( i18n( "Full note" ), "KE1", Key_1);
	keys_->connectItem( "KE1", this, SLOT( KE_1() ) );
	keys_->insertItem( i18n( "Half note" ), "KE2", Key_2);
	keys_->connectItem( "KE2", this, SLOT( KE_2() ) );
	keys_->insertItem( i18n( "set triplet" ), "KE3", Key_3);
	keys_->connectItem( "KE3", this, SLOT( KE_3() ) );
	keys_->insertItem( i18n( "Quarter note" ), "KE4", Key_4);
	keys_->connectItem( "KE4", this, SLOT( KE_4() ) );
	keys_->insertItem( i18n( "Eighth note" ), "KE5", Key_5);
	keys_->connectItem( "KE5", this, SLOT( KE_5() ) );
	keys_->insertItem( i18n( "Sixteenth note" ), "KE6", Key_6);
	keys_->connectItem( "KE6", this, SLOT( KE_6() ) );
	keys_->insertItem( i18n( "32nd note" ), "KE7", Key_7);
	keys_->connectItem( "KE7", this, SLOT( KE_7() ) );
	keys_->insertItem( i18n( "64th note" ), "KE8", Key_8);
	keys_->connectItem( "KE8", this, SLOT( KE_8() ) );
	keys_->insertItem( i18n( "128th note" ), "KE9", Key_9);
	keys_->connectItem( "KE9", this, SLOT( KE_9() ) );
	
	/* voice selection keys */
	keys_->insertItem( i18n( "Select 1st voice" ), "KEVoice1", CTRL+Key_1);
	keys_->connectItem( "KEVoice1", this, SLOT( KE_voice1() ) );
	keys_->insertItem( i18n( "Select 2nd voice" ), "KEVoice2", CTRL+Key_2);
	keys_->connectItem( "KEVoice2", this, SLOT( KE_voice2() ) );
	keys_->insertItem( i18n( "Select 3rd voice" ), "KEVoice3", CTRL+Key_3);
	keys_->connectItem( "KEVoice3", this, SLOT( KE_voice3() ) );
	keys_->insertItem( i18n( "Select 4th voice" ), "KEVoice4", CTRL+Key_4);
	keys_->connectItem( "KEVoice4", this, SLOT( KE_voice4() ) );
	keys_->insertItem( i18n( "Select 5th voice" ), "KEVoice5", CTRL+Key_5);
	keys_->connectItem( "KEVoice5", this, SLOT( KE_voice5() ) );
	keys_->insertItem( i18n( "Select 6th voice" ), "KEVoice6", CTRL+Key_6);
	keys_->connectItem( "KEVoice6", this, SLOT( KE_voice6() ) );
	keys_->insertItem( i18n( "Select 7th voice" ), "KEVoice7", CTRL+Key_7);
	keys_->connectItem( "KEVoice7", this, SLOT( KE_voice7() ) );
	keys_->insertItem( i18n( "Select 8th voice" ), "KEVoice8", CTRL+Key_8);
	keys_->connectItem( "KEVoice8", this, SLOT( KE_voice8() ) );
	keys_->insertItem( i18n( "Select 9th voice" ), "KEVoice9", CTRL+Key_9);
	keys_->connectItem( "KEVoice9", this, SLOT( KE_voice9() ) );
	
	keys_->insertItem( i18n( "Set tied" ), "KEtie", Key_Apostrophe);
	keys_->connectItem( "KEtie", this, SLOT( KE_tie() ) );
	keys_->insertItem( i18n( "Set tied" ), "KEtie1", "Alt+AsciiTilde");
	keys_->connectItem( "KEtie1", this, SLOT( KE_tie() ) );
	keys_->insertItem( i18n( "Set dotted" ), "KEdot", Key_Period);
	keys_->connectItem( "KEdot", this, SLOT( KE_dot() ) );
	keys_->insertItem( i18n( "Flat" ), "KEflat", Key_Minus);
	keys_->connectItem( "KEflat", this, SLOT( KE_flat() ) );
	keys_->insertItem( i18n( "Sharp" ), "KEsharp", Key_NumberSign);
	keys_->connectItem( "KEsharp", this, SLOT( KE_sharp() ) );
	keys_->insertItem( i18n( "Sharp" ), "KEsharp1", Key_Plus);
	keys_->connectItem( "KEsharp1", this, SLOT( KE_sharp() ) );
	keys_->insertItem( i18n( "Natural" ), "KEnatural", Key_N);
	keys_->connectItem( "KEnatural", this, SLOT( KE_natural() ) );
	keys_->insertItem( i18n( "Natural" ), "KEnatural1", "Shift+Equal");
	keys_->connectItem( "KEnatural1", this, SLOT( KE_natural() ) );
	keys_->insertItem( i18n( "Set bar" ), "KEbar", Key_Bar);
	keys_->connectItem( "KEbar", this, SLOT( KE_bar() ) );
	keys_->insertItem( i18n( "Set bar after" ), "KEtab", Key_Tab);
	keys_->connectItem( "KEtab", this, SLOT( KE_tab() ) );
	keys_->insertItem( i18n( "Insert rest" ), "KErest", SHIFT+Key_Return);
	keys_->connectItem( "KErest", this, SLOT( KE_insertRest() ) );
	keys_->insertItem( i18n( "Toggle beam" ), "KEunderscore", "Shift+Underscore");
	keys_->connectItem( "KEunderscore", this, SLOT( KE_underscore() ) );
	keys_->insertItem( i18n( "Keyboard Insert mode" ), "KEkeybordInsert", Key_K);
	keys_->connectItem( "KEkeybordInsert", this, SLOT( KE_keybordInsert() ) );
	keys_->insertItem( i18n( "Goto measure" ), "KEGotoMeasure",  CTRL+Key_G);
	keys_->connectItem( "KEGotoMeasure", this, SLOT( gotoDialog() ) );


/*------------------------- "note" keys -----------------------------------------------*/

	keys_->insertItem( i18n( "Pitch C" ), "KEpitchC", "C");
	keys_->connectItem( "KEpitchC", this, SLOT( KE_pitch_C() ) );
	keys_->insertItem( i18n( "Pitch D" ), "KEpitchD", "D");
	keys_->connectItem( "KEpitchD", this, SLOT( KE_pitch_D() ) );
	keys_->insertItem( i18n( "Pitch E" ), "KEpitchE", SHIFT+Key_E);
	keys_->connectItem( "KEpitchE", this, SLOT( KE_pitch_E() ) );
	keys_->insertItem( i18n( "Pitch F" ), "KEpitchF", "F");
	keys_->connectItem( "KEpitchF", this, SLOT( KE_pitch_F() ) );
	keys_->insertItem( i18n( "Pitch G" ), "KEpitchG", "G");
	keys_->connectItem( "KEpitchG", this, SLOT( KE_pitch_G() ) );
	keys_->insertItem( i18n( "Pitch A" ), "KEpitchA", "A");
	keys_->connectItem( "KEpitchA", this, SLOT( KE_pitch_A() ) );
	keys_->insertItem( i18n( "Pitch B" ), "KEpitchB", "B");
	keys_->connectItem( "KEpitchB", this, SLOT( KE_pitch_B() ) );

	keys_->readSettings();
	connect(&timer_, SIGNAL(timeout()), this, SLOT(playNext()));
	x0_ = x1_ = y0_  = 0;
	selectbutton_->setOn(true); /* go to selection mode, by disabling the other buttons */
	main_props_.directPainter->setPaintDevice(notePart_);
	help_x0_ = -1;
	keyLine_ = NULL_LINE;
	keyOffs_ = 0;

	// the following code prepares the possibility of tool elements for note attribute modification.
	toolContainer_ = new QFrame( this, "", WStyle_NoBorder );
	toolContainer_->resize( TOOL_ELEMENT_WIDTH + 8, TOOL_ELEMENT_HEIGHT );
	toolContainer_->setLineWidth( 2 );
	toolContainer_->setFrameShadow( QFrame::Raised );
	toolContainer_->setFrameShape( QFrame::WinPanel );
	tabWid_ = new QTabWidget( toolContainer_ );
	tabWid_->setGeometry( 2, 2, TOOL_ELEMENT_WIDTH + 4, TOOL_ELEMENT_HEIGHT);
	toolContainer_->hide();

	trillLengthBase_ = new QFrame( toolContainer_ );
	trillLengthBase_->setGeometry( QRect(6, 2, TOOL_ELEMENT_WIDTH, 40 ) );
	trillEnabled_ = new QCheckBox( i18n ( "Enabled" ) , trillLengthBase_ );
	trillEnabled_->setChecked( true );
	trillEnabled_->setGeometry( QRect( 2, 2, 100, 16 ) );
	trillLength_ = new QSlider( QSlider::Horizontal, trillLengthBase_ );
	trillLength_->setGeometry( QRect( 2, 18, TOOL_ELEMENT_WIDTH - 4, 20 ) );
	trillLength_->setMinValue( 0 );
	trillLength_->setMaxValue( 30 );


	connect( trillLength_, SIGNAL( valueChanged( int ) ), this, SLOT( trillLengthChanged( int ) ) );
	connect( trillEnabled_, SIGNAL( clicked() ), this, SLOT( trillDisabled() ) );
	
	tabWid_->insertTab( trillLengthBase_, i18n ( "Trill" ) );
	
	dynamicBase_ = new QFrame( toolContainer_ );
	dynamicBase_->setGeometry( QRect(6, 2, TOOL_ELEMENT_WIDTH, 40 ) );
	dynamicPos_ = new QSlider( QSlider::Horizontal, dynamicBase_ );
	dynamicPos_->setGeometry( QRect( 2, 18, TOOL_ELEMENT_WIDTH - 4,20 ) );
	dynamicPos_->setMinValue( 0 );
	dynamicPos_->setMaxValue( 6000 );
	dynamicDisable_ = new QCheckBox( i18n ( "Enabled" ) , dynamicBase_ );
	dynamicDisable_->setGeometry( QRect( 2, 2, 100, 16 ) );
	dynamicDisable_->setChecked( true );
	dynamicAlignment_ = new QCheckBox( i18n ( "Turn" ) , dynamicBase_ );
	dynamicAlignment_->setGeometry( QRect( TOOL_ELEMENT_WIDTH - 102, 2, 100, 16 ) );
	dynamicAlignment_->setChecked( true );

	connect( dynamicPos_, SIGNAL( valueChanged(int) ), this, SLOT(dynamicPosChanged( int ) ) );
	connect( dynamicDisable_, SIGNAL( clicked() ), this, SLOT( dynamicKill() ) );
	connect( dynamicAlignment_, SIGNAL( clicked() ), this, SLOT( dynamicSwitch() ) );

	tabWid_->insertTab( dynamicBase_, i18n ( "Crescendo" ) );

	vaLengthBase_ = new QFrame( toolContainer_ );
	vaLengthBase_->setGeometry( QRect(6, 2, TOOL_ELEMENT_WIDTH, 40 ) );
	vaDisable_ = new QCheckBox( i18n ( "Enabled" ) , vaLengthBase_ );
	vaDisable_->setChecked( true );
	vaDisable_->setGeometry( QRect( 2, 2, 100, 16 ) );
	vaLength_ = new QSlider( QSlider::Horizontal, vaLengthBase_ );
	vaLength_->setGeometry( QRect( 2, 18, TOOL_ELEMENT_WIDTH - 4, 20 ) );
	vaLength_->setMinValue( 0 );
	vaLength_->setMaxValue( 100 );


	connect( vaLength_, SIGNAL( valueChanged( int ) ), this, SLOT( vaLengthChanged( int ) ) );
	connect( vaDisable_, SIGNAL( clicked() ), this, SLOT( vaDisabled() ) );
	
	tabWid_->insertTab( vaLengthBase_, i18n ( "8va" ) );

	tmpElem_ = 0;
	tmpChordDiagram_ = 0;
	selectedElemForChord_ = 0;
	
	stopList_.setAutoDelete(false);
	nextEvents_.setAutoDelete(false);
	currentEvents_.setAutoDelete(false);
	setSaveWidth(170);
	setSaveHeight(250);
	setParamsEnabled(false);
	braceMatrix_ = new layoutDef[1];
	bracketMatrix_ = new layoutDef[1];
	barCont_ = new layoutDef[1];
	layoutPixmap_ = 0;
	context_rect_left_right_ = DEFAULT_CONTEXT_REC_LEFT_RIGHT;
	createLayoutPixmap();
}

NMainFrameWidget::~NMainFrameWidget() {
        m_recentFilesAction->saveEntries( KGlobal::config() );
	synchronizeRecentFiles();
#ifdef WITH_TSE3
	delete tse3Handler_;
#endif
	delete musicxmlFileReader_;
	delete genPropDialog_;
	delete notePart_;
	delete fhandler_;
	delete lilyexport_;
	delete exportDialog_;
	delete saveParametersDialog_;
	delete listDialog_;
	delete zoomselect_;
	delete multistaffDialog_;
	delete staffPropFrm_;
	delete clefDialog_;
	delete keyDialog_;
	delete cleanUpRestsDialog_;
	delete volChangeDialog_;
	delete lyricsDialog_;
	delete timesigDialog_;
	delete scaleFrm_;
	delete playbutton_;
	delete stemUpbutton_;
	delete stemDownbutton_;
	delete dotbutton_;
	delete tiebutton_;
	delete beambutton_;
	delete slurbutton_;
	delete tripletbutton_;
	delete arpeggbutton_;
	delete editbutton_;
	staffList_.setAutoDelete(true);
	staffList_.clear();
	delete staccatobutton_;
	delete portatobutton_;
	delete strong_pizzicatobutton_;
	delete sforzatobutton_;
	delete trillEnabled_;
	delete trillLength_;
	delete trillLengthBase_;
	delete dynamicPos_;
	delete dynamicDisable_;
	delete dynamicAlignment_;
	delete dynamicBase_;
	delete tabWid_;
	delete toolContainer_;
	delete tupletDialog_;
	delete braceMatrix_;
	delete bracketMatrix_;
	delete barCont_;
	if (layoutPixmap_) delete layoutPixmap_;
}

void NMainFrameWidget::synchronizeRecentFiles() {
	NMainWindow *window;
	for (window = NResource::windowList_.first(); window; window = NResource::windowList_.next()) {
		window->mainFrameWidget()->reloadRecentFileList();
	}
}

void NMainFrameWidget::reloadRecentFileList() {
        m_recentFilesAction->loadEntries( KGlobal::config() );
}

void NMainFrameWidget::setEdited(bool edited) {
	editiones_ = edited;
	if (inPart_) return;
	static_cast<KMainWindow *>(parentWidget())
	->setCaption((!scTitle_.isEmpty() ? (!scSubtitle_.isEmpty() ? (scTitle_ + ": " + scSubtitle_) : scTitle_) : actualFname_), edited);
}

void NMainFrameWidget::scoreInfo() {
	ScoreInfoDialog scoreInfoDialog(this);
	scoreInfoDialog.exec();
}

void NMainFrameWidget::configure() {
	ConfigureDialog configureDialog(this);
	configureDialog.exec();
}

void NMainFrameWidget::plugButtons(KToolBar *toolbar) {
	int i;
	selectbutton_->plug(toolbar);
	for ( i = 0; i < COUNT_CHORDBUTTONS; ++i) {
		note_buttons_[i]->plug(toolbar);
	}
	playbutton_->plug(toolbar);
	stemUpbutton_->plug(toolbar);
	stemDownbutton_->plug(toolbar);
	dotbutton_->plug(toolbar);
	tiebutton_->plug(toolbar);
	beambutton_->plug(toolbar);
	slurbutton_->plug(toolbar);
	tripletbutton_->plug(toolbar);
	for (i = 0; i < 5; ++i) {
		offs_buttons_[i]->plug(toolbar);
	}
	editbutton_->plug(toolbar);
}

void NMainFrameWidget::unPlugButtons(KToolBar *toolbar) {
	int i;
	selectbutton_->unplug(toolbar);
	for ( i = 0; i < COUNT_CHORDBUTTONS; ++i) {
		note_buttons_[i]->unplug(toolbar);
	}
	playbutton_->unplug(toolbar);
	stemUpbutton_->unplug(toolbar);
	stemDownbutton_->unplug(toolbar);
	dotbutton_->unplug(toolbar);
	tiebutton_->unplug(toolbar);
	beambutton_->unplug(toolbar);
	slurbutton_->unplug(toolbar);
	tripletbutton_->unplug(toolbar);
	for (i = 0; i < 5; ++i) {
		offs_buttons_[i]->unplug(toolbar);
	}
	editbutton_->unplug(toolbar);
}

void NMainFrameWidget::paintNew() {
	repaint();
}

void NMainFrameWidget::cleanupSelections() {
	if (NResource::staffSelAutobeam_) delete [] NResource::staffSelAutobeam_;
	NResource::staffSelAutobeam_ = 0;
	if (NResource::staffSelAutobar_) delete [] NResource::staffSelAutobar_;
	NResource::staffSelAutobar_ = 0;
	if (NResource::staffSelMute_) delete [] NResource::staffSelMute_;
	NResource::staffSelMute_ = 0;
}

void NMainFrameWidget::enableCriticalButtons(bool enable) {
	KAction *action;

	for (action = criticalButtons_.first(); action; action = criticalButtons_.next()) {
		action->setEnabled(enable);
	}
}

void NMainFrameWidget::updatePainter() {
	nettoWidth_ = paperWidth_ - (int) ((float) main_props_.left_page_border * main_props_.zoom) - RIGHT_PAGE_BORDER;
	main_props_.tp->noticeClipRect ( QRect((int) ((float) main_props_.left_page_border * main_props_.zoom), TOP_BOTTOM_BORDER, nettoWidth_, nettoHeight_ ));
	main_props_.directPainter->noticeClipRect ( QRect((int) ((float) main_props_.left_page_border * main_props_.zoom), TOP_BOTTOM_BORDER, nettoWidth_, nettoHeight_ ));
	main_props_.p->noticeClipRect ( QRect((int) ((float) main_props_.left_page_border * main_props_.zoom), TOP_BOTTOM_BORDER, nettoWidth_, nettoHeight_ ));
	main_props_.tp->setXPosition(leftx_ - main_props_.left_page_border);
	main_props_.directPainter->setXPosition(leftx_ - main_props_.left_page_border);
}

void NMainFrameWidget::createLayoutPixmap() {
	int y0, y1, mid;
	int i, j;
	bool overlapping = false;
	int pixmap_width;
	int bracket_x_pos = DEFAULT_LAYOUT_BRACKET_X_POS;
	bool has_braces = false;
	bool has_brackets = false;
	NStaff *staff_elem;
	QPainter p;
	QPen pen;
	if (layoutPixmap_) delete layoutPixmap_;
	layoutPixmap_ = 0;
	for (i = 0; !has_braces && i < staffCount_; i++) {
		if (braceMatrix_[i].valid) has_braces = true;
	}
	for (i = 0; !has_brackets && i < staffCount_; i++) {
		if (bracketMatrix_[i].valid) has_brackets = true;
	}
	if (!has_brackets && !has_braces) {
		main_props_.left_page_border = DEFAULT_LEFT_PAGE_BORDER;
		main_props_.context_clef_xpos = DEFAULT_CONTEXT_CLEF_X_POS;
		main_props_.context_keysig_xpos = DEFAULT_CONTEXT_KEYSIG_X_POS;
		context_rect_left_right_ = DEFAULT_CONTEXT_REC_LEFT_RIGHT;
		updatePainter();
		return;
	}
	for (i = 0; i < staffCount_ && !overlapping; i++) {
		if (bracketMatrix_[i].valid) {
			for (j = 0; j < staffCount_ && !overlapping; j++) {
				if (braceMatrix_[j].valid) {
					if (braceMatrix_[j].beg >= bracketMatrix_[i].beg && braceMatrix_[j].end <= bracketMatrix_[i].end) {
						overlapping = true;
					}
				}
			}
		}
	}
	if (overlapping) { /* document includes braces and brackets and they overlap*/
		pixmap_width = LAYOUT_BRACEX_TOTAL + LAYOUT_BRACKET_X_TOTAL + BRACE_BRACKET_DIST - DEFAULT_LAYOUT_BRACKET_X_POS;
		bracket_x_pos = LAYOUT_BRACEX_TOTAL + BRACE_BRACKET_DIST;
		main_props_.left_page_border = pixmap_width - LAYOUT_BRACKET_X_OVERLAP;
	}
	else if (has_brackets && has_braces) { /* document includes both braces and brackets, but they never overlap */
		pixmap_width = LAYOUT_BRACEX_TOTAL;
		bracket_x_pos = DEFAULT_LAYOUT_BRACKET_X_POS;
		main_props_.left_page_border = pixmap_width;
	}
	else if (has_brackets) { /* documents include brackets only */
		pixmap_width = LAYOUT_BRACKET_X_TOTAL;
		main_props_.left_page_border = pixmap_width - LAYOUT_BRACKET_X_OVERLAP;
	}
	else { /* document includes braces only */
		pixmap_width = LAYOUT_BRACEX_TOTAL;
		main_props_.left_page_border = pixmap_width;
	}
	main_props_.context_clef_xpos = pixmap_width + CONTEXT_CLEF_X_DIST;
	main_props_.context_keysig_xpos = pixmap_width + CONTEXT_KEYSIG_X_DIST;
	context_rect_left_right_ = pixmap_width;
	updatePainter();
	for (i = 0; i < staffCount_; i++) {
		if (bracketMatrix_[i].valid) {
			if (!layoutPixmap_) {
				layoutPixmap_ = new QPixmap(pixmap_width, lastYHeight_);
				p.begin(layoutPixmap_);
				p.fillRect(0, 0, pixmap_width, lastYHeight_, NResource::backgroundBrush_);
			}
			staff_elem = staffList_.at(bracketMatrix_[i].beg);
			if (!staff_elem) {
				NResource::abort("createLayoutPixmap: internal error", 1);
			}
			y0 = staff_elem->getBase();
			staff_elem = staffList_.at(bracketMatrix_[i].end);
			if (!staff_elem) {
				NResource::abort("createLayoutPixmap: internal error", 2);
			}
			pen.setWidth(LAYOUT_BRACKET_WIDTH1);
			p.setPen(pen);
			y1 = staff_elem->getBase()+4*LINE_DIST;
			p.drawLine(bracket_x_pos, y0 - LAYOUT_BRACKET_WIDTH2, bracket_x_pos, y1 + LAYOUT_BRACKET_WIDTH2);
			pen.setWidth(LAYOUT_BRACKET_WIDTH2);
			p.setPen(pen);
			p.drawArc(bracket_x_pos - LAYOUT_BRACKET_XRAD, y0-2*LAYOUT_BRACKET_YRAD,
				2*LAYOUT_BRACKET_XRAD, 2*LAYOUT_BRACKET_YRAD, -90*16, LAYOUT_BRACKET_ARC_LENGTH*16);
			p.drawArc(bracket_x_pos - LAYOUT_BRACKET_XRAD, y1,
				2*LAYOUT_BRACKET_XRAD, 2*LAYOUT_BRACKET_YRAD, (90-LAYOUT_BRACKET_ARC_LENGTH)*16, LAYOUT_BRACKET_ARC_LENGTH*16);
		}
	}
	for (i = 0; i < staffCount_; i++) {
		if (braceMatrix_[i].valid) {
			if (!layoutPixmap_) {
				layoutPixmap_ = new QPixmap(pixmap_width, lastYHeight_);
				p.begin(layoutPixmap_);
				p.fillRect(0, 0, pixmap_width, lastYHeight_, NResource::backgroundBrush_);
			}
			pen.setWidth(LAYOUT_BRACE_WIDTH);
			p.setPen(pen);
			staff_elem = staffList_.at(braceMatrix_[i].beg);
			if (!staff_elem) {
				NResource::abort("createLayoutPixmap: internal error", 3);
			}
			y0 = staff_elem->getBase();
			staff_elem = staffList_.at(braceMatrix_[i].end);
			if (!staff_elem) {
				NResource::abort("createLayoutPixmap: internal error", 4);
			}
			y1 = staff_elem->getBase()+4*LINE_DIST;
			mid = y0 + (y1 - y0) / 2;
			p.drawLine(LAYOUT_BRACE_X_POS+LAYOUT_BRACEX_ARROW, y0+LAYOUT_BR_ARROW_YRAD,
				LAYOUT_BRACE_X_POS+LAYOUT_BRACEX_ARROW, mid - LAYOUT_MID_ROUNDDIST);
			p.drawLine(LAYOUT_BRACE_X_POS+LAYOUT_BRACEX_ARROW, mid + LAYOUT_MID_ROUNDDIST, 
				LAYOUT_BRACE_X_POS+LAYOUT_BRACEX_ARROW, y1-LAYOUT_BR_ARROW_YRAD);
			p.drawArc(LAYOUT_BRACE_X_POS- LAYOUT_BR_ARROW_XRAD, mid,
				2*LAYOUT_BR_ARROW_XRAD, 2*LAYOUT_BR_ARROW_YRAD, (90-LAYOUT_BRACE_ARC_LENGTH)*16, LAYOUT_BRACE_ARC_LENGTH*16);
			p.drawArc(LAYOUT_BRACE_X_POS - LAYOUT_BR_ARROW_XRAD, mid-2*LAYOUT_BR_ARROW_YRAD+1,
				2*LAYOUT_BR_ARROW_XRAD, 2*LAYOUT_BR_ARROW_YRAD, -90*16, LAYOUT_BRACE_ARC_LENGTH*16);
			p.drawArc(LAYOUT_BRACE_X_POS+LAYOUT_BRACEX_ARROW - LAYOUT_BR_ARROW_XRAD + LAYOUT_BR_ARROW_XRAD, y0,
				2*LAYOUT_BR_ARROW_XRAD, 2*LAYOUT_BR_ARROW_YRAD, (180-LAYOUT_BRACE_ARC_LENGTH)*16, LAYOUT_BRACE_ARC_LENGTH*16);
			p.drawArc(LAYOUT_BRACE_X_POS+LAYOUT_BRACEX_ARROW - LAYOUT_BR_ARROW_XRAD + LAYOUT_BR_ARROW_XRAD, y1- 2*LAYOUT_BR_ARROW_YRAD,
				2*LAYOUT_BR_ARROW_XRAD, 2*LAYOUT_BR_ARROW_YRAD, 180*16, LAYOUT_BRACE_ARC_LENGTH*16);
		}
	}
	
	if (layoutPixmap_) p.end();
}
			
	

void NMainFrameWidget::renewStaffLayout() {
	delete braceMatrix_;
	delete bracketMatrix_;
	delete barCont_;
	braceMatrix_ = new layoutDef[staffCount_];
	bracketMatrix_ = new layoutDef[staffCount_];
	barCont_ = new layoutDef[staffCount_];
	createLayoutPixmap();
}

void  NMainFrameWidget::appendStaffLayoutElem() {
	layoutDef *tmp;
	int i;
	tmp = new layoutDef[staffCount_];
	for (i = 0; i < staffCount_-1; i++) {
		tmp[i] = braceMatrix_[i];
	}
	delete braceMatrix_;
	braceMatrix_ = tmp;
	tmp = new layoutDef[staffCount_];
	for (i = 0; i < staffCount_-1; i++) {
		tmp[i] = bracketMatrix_[i];
	}
	delete bracketMatrix_;
	bracketMatrix_ = tmp;
	tmp = new layoutDef[staffCount_];
	for (i = 0; i < staffCount_-1; i++) {
		tmp[i] = barCont_[i];
	}
	delete barCont_;
	barCont_ = tmp;
	createLayoutPixmap();
}

	

void NMainFrameWidget::processMouseEvent ( QMouseEvent * evt)  {
#define TRANSX(x) (((int) ((float) (x)/main_props_.zoom + 0.5))+leftx_-main_props_.left_page_border)
#define TRANSY(y) (((int) ((float) (y)/main_props_.zoom + 0.5))+topy_-TOP_BOTTOM_BORDER)
#define RETRANSY(y) ((int) (main_props_.zoom * ((y)-topy_+TOP_BOTTOM_BORDER) + 0.5))
#define TRANS_POINT(x, y) QPoint((int) ((double) (x)/main_props_.zoom + 0.5)+leftx_-main_props_.left_page_border, \
			       (int) ((double) (y)/main_props_.zoom + 0.5)+topy_-TOP_BOTTOM_BORDER)
#define RETRANSY2(y) ((y)+main_props_.zoom*(topy_-TOP_BOTTOM_BORDER))
#define TRANSY2LINE(y, dl, l) dl = (4.0*(double) LINE_DIST-(((y)+main_props_.zoom*(topy_-TOP_BOTTOM_BORDER))/main_props_.zoom-currentStaff_->staff_props_.base))/((double)LINE_DIST/2.0); \
				l = (dl >= 0.0) ? (int) (dl + 0.5)  : (int) (dl - 0.5)
			   	

	property_type properties;
	unsigned int val, newXpos;
	bool playable;
	QPoint p;
	NVoice *voice_elem;
	NStaff *staff_elem, *source_staff;
	int i;
	int line;
	double dline;
	bool delete_elem, insert_new_note;
	NMusElement *elem;

	toolContainer_->hide();


	if (playing_) return;
	keyLine_ = NULL_LINE;
	if ((evt->state() & QEvent::MouseButtonPress)) return;
	
	/* remember the last selected element's Midi time, so it gets restored when needed */
	if (currentVoice_->getCurrentElement())
		main_props_.lastMidiTime = currentVoice_->getCurrentElement()->midiTime_;
		
	switch (evt->button()) {
		case LeftButton:
			if (evt->type() == QEvent::MouseButtonDblClick) {
				selectWholeStaff();
				return;
			}
			if (NResource::windowWithSelectedRegion_) {
				if (NResource::windowWithSelectedRegion_ != this) {
					NResource::windowWithSelectedRegion_->repaint();
				}
			}
			NResource::windowWithSelectedRegion_ = 0;
			if (NResource::staffSelMulti_) {
				delete [] NResource::staffSelMulti_;
				NResource::staffSelMulti_ = 0;
				NResource::numOfMultiStaffs_ = 0;
			}
			x0_ = TRANSX(xori_ = evt->x());
 			p = TRANS_POINT(evt->x(), evt->y());
			if (selectedSign_) {
				checkStaffIntersection(p);
				switch (selectedSign_) {
					case T_KEYSIG:
					case T_CLEF:
					case T_TIMESIG:
					case TEMPO_SIGNATURE:
					case VOLUME_SIG:
					case MULTIREST:
					case SEGNO:
					case DAL_SEGNO:
					case DAL_SEGNO_AL_FINE:
					case DAL_SEGNO_AL_CODA:
					case FINE:
					case CODA:
					case RITARDANDO:
					case ACCELERANDO:
					case PROGRAM_CHANGE:
					case T_TEXT:
						currentVoice_->insertTmpElemAtPosition(TRANSX(evt->x()), tmpElem_);
						setEdited();
						break;
					case CDIAGRAM:
							for (voice_elem = voiceList_.first(); voice_elem; voice_elem = voiceList_.next()) 
						    		if (voice_elem->checkElementForElementInsertion(p))
									goto insert_diagram;
							KMessageBox::sorry(this, i18n("Please choose a note or rest!"), kapp->makeStdCaption(i18n("???")));
							return;
							insert_diagram:
							if (!voice_elem->isFirstVoice()) {
								KMessageBox::sorry(this, i18n("Please choose a note or rest from first voice"), kapp->makeStdCaption(i18n("???")));
								return;
							}
							if (voice_elem->getCurrentElement()->getType() & PLAYABLE) {
								voice_elem->pubAddUndoElement();
								voice_elem->getCurrentElement()->playable()->addChordDiagram(tmpChordDiagram_);
								setEdited();
								tmpChordDiagram_ = 0;
								selectedSign_ = 0;
								reposit();
								repaint();
							}
							else  {
								KMessageBox::sorry(this, i18n("Please choose a note or rest!"), kapp->makeStdCaption(i18n("???")));
							}
							break;
					case DYNAMIC:
					case LNTRILL:
					case TRILL:
					case VA8:
					case VA8_BASSA:
						setEdited();
						voice_elem = voiceList_.first();
						for (; voice_elem; voice_elem = voiceList_.next()) 
						    if (voice_elem->checkElementForElementInsertion(p))
    							goto next;
						KMessageBox::sorry(this, i18n("Please choose a note!"), kapp->makeStdCaption(i18n("???")));
						selectedSign_ = 0;
						return;
						next:
						if (!voice_elem->isFirstVoice()) {
							KMessageBox::sorry(this, i18n("Please choose a note from first voice"), kapp->makeStdCaption(i18n("???")));
							return;
						}
						if( !voice_elem->getCurrentElement()->chord() ) {
							KMessageBox::sorry(this, i18n("Please choose a note!"), kapp->makeStdCaption(i18n("???")));
							return;
						}
						switch(selectedSign_) {
						    case TRILL:	if (voice_elem->getCurrentElement()->chord()->va_ != 0) break;
						    		if (voice_elem->getCurrentElement()->chord()->dynamic_ != 0) break;
								voice_elem->pubAddUndoElement();  
								if( voice_elem->getCurrentElement()->chord()->trill_ ) {
								      if( voice_elem->getCurrentElement()->chord()->trill_ < 0 )
								        voice_elem->getCurrentElement()->chord()->trill_ =
									    -voice_elem->getCurrentElement()->chord()->trill_;
								  }
								else 
								  voice_elem->getCurrentElement()->chord()->trill_ = 4;
								break;
						    case LNTRILL: if (voice_elem->getCurrentElement()->chord()->va_ != 0) break;
						    		  if (voice_elem->getCurrentElement()->chord()->dynamic_ != 0) break;
								  voice_elem->pubAddUndoElement();
								  if( voice_elem->getCurrentElement()->chord()->trill_ ) {
									if( voice_elem->getCurrentElement()->chord()->trill_ > 0 )
									    voice_elem->getCurrentElement()->chord()->trill_ =
										-voice_elem->getCurrentElement()->chord()->trill_;
								  }
								else
								    voice_elem->getCurrentElement()->chord()->trill_ = -3;
								break;
						    case DYNAMIC: if (voice_elem->getCurrentElement()->chord()->trill_ != 0) break;
						    		  if (voice_elem->getCurrentElement()->chord()->va_ != 0) break;
								  voice_elem->pubAddUndoElement();
								  voice_elem->getCurrentElement()->chord()->dynamic_ = 10;
								  voice_elem->getCurrentElement()->chord()->dynamicAlign_ = true;
								  break;
						    case VA8:     if (voice_elem->getCurrentElement()->chord()->trill_ != 0) break;
						    		  if (voice_elem->getCurrentElement()->chord()->dynamic_ != 0) break;
						    		  voice_elem->pubAddUndoElement();
								  voice_elem->getCurrentElement()->chord()->va_ = 10;
								  break;
						    case VA8_BASSA: if (voice_elem->getCurrentElement()->chord()->trill_ != 0) break;
						    		  if (voice_elem->getCurrentElement()->chord()->dynamic_ != 0) break;
						    		  voice_elem->pubAddUndoElement();
								  voice_elem->getCurrentElement()->chord()->va_ = -10;
								  break;
						}
						reposit();
						repaint();
						selectedSign_ = 0;
						manageToolElement(true);
						return;
					default:
						currentVoice_->insertAtPosition(T_SIGN, TRANSX(evt->x()), 0 /* dummy */, selectedSign_, 0, tmpElem_);
						setEdited();
						break;
				}
				setEdited();
				selectedSign_ = 0;
				computeMidiTimes(false);
				reposit();
				repaint();
				newXpos = currentVoice_->getCurrentElement()->getXpos()+
					currentVoice_->getCurrentElement()->getBbox()->width()+CUR_DIST;
				if (newXpos + SMALL_X_SENS_DIST > leftx_ + paperScrollWidth_) {
					scrollx_->setValue(leftx_ + SMALL_X_SCROLL);
				}
			}
			else {
				delete_elem = /*main_props_.actualLength > 0 && !editMode_ &&*/ (evt->state() & ControlButton) != 0;
				insert_new_note = main_props_.actualLength > 0 && !editMode_ && (evt->state() & ControlButton) == 0;
				TRANSY2LINE(evt->y(), dline, line);
				if ( (val = checkAllStaffsForNoteInsertion(line, p, &properties, &playable, &delete_elem, &insert_new_note)) > 0 ) {
					if (editMode_) {
						if (playable) {
							updateInterface(properties, val);
						}
						else
							updateInterface(0, -1);
					}
				}
				else if (editMode_)
					updateInterface(0, -1);

				if (delete_elem) {
					deleteElem(true);
					elem = currentVoice_->getCurrentElement();
					if (elem) {
						newXpos = elem->getXpos();
						if (newXpos - SMALL_X_SENS_DIST< leftx_) {
							scrollx_->setValue(leftx_ - SMALL_X_SCROLL < 0 ? 0 : leftx_ - SMALL_X_SCROLL);
						}
					}
				}
				if (insert_new_note && main_props_.actualLength > 0) {
					TRANSY2LINE(evt->y(), dline, line);
					if (line >= MINLINE && line <= MAXLINE) {
						currentVoice_->insertAtPosition(T_CHORD, TRANSX(evt->x()), line, main_props_.actualLength, actualOffs_);
						setEdited();
						computeMidiTimes(NResource::automaticBarInsertion_, NResource::autoBeamInsertion_);
						reposit();
						newXpos = currentVoice_->getCurrentElement()->getXpos()+
							currentVoice_->getCurrentElement()->getBbox()->width()+CUR_DIST;
						if (newXpos + SMALL_X_SENS_DIST > leftx_ + paperScrollWidth_) {
							scrollx_->setValue(leftx_ + SMALL_X_SCROLL);
						}
					}
				}
				reposit();
				repaint();
				if (!editMode_) resetSpecialButtons();
				break;
		case MidButton:
				if (NResource::windowWithSelectedRegion_) {
 					p = TRANS_POINT(evt->x(), evt->y());
					checkStaffIntersection(p);
					if (NResource::numOfMultiStaffs_) {
						if (!NResource::isGrabbed_) {
							NResource::windowWithSelectedRegion_->grabElementsAccording();
							NResource::isGrabbed_ = true;
						}
						for (i = 0, staff_elem = staffList_.first(); staff_elem && i < NResource::numOfMultiStaffs_;
							 staff_elem = staffList_.next() , i++) {
							if (NResource::staffSelMulti_[i]) {
								if (i >= NResource::windowWithSelectedRegion_->staffList_.count()) break;
								source_staff = NResource::windowWithSelectedRegion_->staffList_.at(i);
								staff_elem->pasteAtPosition(TRANSX(evt->x()), source_staff);
							}
						}
					}
					else {
						if (!NResource::isGrabbed_) {
							NResource::voiceWithSelectedRegion_->getStaff()->grabElements(NResource::voiceWithSelectedRegion_);
							NResource::isGrabbed_ = true;
						}
						currentStaff_->pasteAtPosition(TRANSX(evt->x()), NResource::voiceWithSelectedRegion_->getStaff());
					}
					setEdited();
					computeMidiTimes(false);
					reposit();
					repaint();
				}
			}
			break;
		case RightButton:
				if (main_props_.actualLength > 0 && main_props_.actualLength < DOUBLE_WHOLE_LENGTH) {
 					p = TRANS_POINT(evt->x(), evt->y());
				  	checkStaffIntersection(p);
					currentVoice_->insertAtPosition(T_REST, TRANSX(evt->x()), 0 /* dummy */, main_props_.actualLength, 0);
					setEdited();
					computeMidiTimes(false);
					reposit();
					newXpos = currentVoice_->getCurrentElement()->getXpos()+
						currentVoice_->getCurrentElement()->getBbox()->width()+CUR_DIST;
					if (newXpos + SMALL_X_SENS_DIST > leftx_ + paperScrollWidth_) {
						scrollx_->setValue(leftx_ + SMALL_X_SCROLL);
					}
					repaint();
				}
			resetSpecialButtons();
			break;
	}
}

void NMainFrameWidget::autoscroll() {
#define SENS_DIST 50
#define AUTO_SCROLL_DIST (width_ / 2)
	bool scrolled = true;
	int scrolldist;
	x1_ = TRANSX(cursor().pos().x());
	if (x1_ < leftx_ + SENS_DIST) {
		scrolldist = leftx_ > AUTO_SCROLL_DIST ? AUTO_SCROLL_DIST : leftx_;
		if (scrolldist) {
			scrollx_->setValue(leftx_ - scrolldist);
			x1_ -= scrolldist;
		}
		else {
			autoscrollTimer_.stop();
			scrolled = false;
		}
	}
	else if (x1_ > leftx_ + (int) ((float) width_ / main_props_.zoom)  - SENS_DIST) {
		scrolldist = leftx_ < lastXpos_ + AUTO_SCROLL_DIST ? AUTO_SCROLL_DIST : 0;
		if (scrolldist) {
			scrollx_->setValue(leftx_ + scrolldist);
			x1_ += AUTO_SCROLL_DIST;
		}
		else {
			autoscrollTimer_.stop();
			scrolled = false;
		}
	}
	else {
		autoscrollTimer_.stop();
		scrolled = false;
	}
	if (scrolled) {
		selRect_ = QRect(x1_ > x0_ ? x0_ : x1_, y0_, abs(x0_ - x1_), 4*LINE_DIST);
		repaint();
	}
}

void NMainFrameWidget::processMoveEvent( QMouseEvent * evt)  {
#define MINDIST 10
#define MINREGION 8
#define AUTOSCROLL_RATE 500
	if (main_props_.actualLength >= 0) {
		NResource::windowWithSelectedRegion_ = 0;
		return;
	}
	if (abs(evt->x()-xori_) < MINDIST) return;
	currentStaff_->getActualVoice()->findStartElemAt(x0_, x1_);
	y0_ = currentStaff_->getBase();
	x1_ = TRANSX(xori_ = evt->x());
	if (abs(x1_ - x0_) < MINREGION) return;
	NResource::windowWithSelectedRegion_ = this;
	NResource::voiceWithSelectedRegion_ = currentStaff_->getActualVoice();
	NResource::isGrabbed_ = false;
	NResource::voiceWithSelectedRegion_->trimmRegion(&x0_, &x1_);
	if (x1_ < leftx_ + SENS_DIST || x1_ > leftx_ + (int) ((float) width_ / main_props_.zoom) - SENS_DIST) {
		if (!autoscrollTimer_.isActive()) {
			autoscrollTimer_.start(AUTOSCROLL_RATE);
		}
	}
	selRect_ = QRect(x1_ > x0_ ? x0_ : x1_, y0_, abs(x0_ - x1_), 4*LINE_DIST);
	repaint();
}

void NMainFrameWidget::processWeelEvent(QWheelEvent * e ) {
	if (playing_) return;
	if (e->state() & ControlButton) {
		if (e->state() & ShiftButton) {
			if (e->delta() > 0) {
				moveSemiToneUp();
			}
			else {
				moveSemiToneDown();
			}
		}
		else {
			if (e->delta() > 0) {
				moveUp();
			}
			else {
				moveDown();
			}
		}
	}
	else if (e->state() & ShiftButton) {
		if (e->delta() > 0) {
			moveOctaveUp();
		}
		else {
			moveOctaveDown();
		}
	}
	else {
		scrollx_->setValue(leftx_ - e->delta());
	}
}

void NMainFrameWidget::selectWholeStaff() {
	if (main_props_.actualLength >= 0) {
		NResource::windowWithSelectedRegion_ = 0;
		return;
	}
	if (!currentStaff_->trimmRegionToWholeStaff(&x0_, &x1_)) return;
	NResource::windowWithSelectedRegion_ = this;
	NResource::voiceWithSelectedRegion_ = currentStaff_->getActualVoice();
	NResource::isGrabbed_ = false;
	y0_ = currentStaff_->getBase();
	selRect_ = QRect(x0_, y0_ , x1_ - x0_, 4*LINE_DIST);
	repaint();
}
	

void NMainFrameWidget::deleteBlock() {
	int i;
	NStaff *staff_elem;
	if (selRect_.width() < 10) return;
	if (NResource::numOfMultiStaffs_) {
		for (i = 0, staff_elem = staffList_.first(); staff_elem && i < NResource::numOfMultiStaffs_; staff_elem = staffList_.next(), i++) {
			if (NResource::staffSelMulti_[i]) {
				staff_elem->deleteBlocksAccording();
			}
		}
	}
	else {
		NResource::voiceWithSelectedRegion_->getStaff()->deleteBlock(NResource::voiceWithSelectedRegion_);
	}
	computeMidiTimes(false);
	setEdited();
	NResource::windowWithSelectedRegion_ = 0;
	reposit();
	repaint();
}

/*------------------------- reaction on QWidget events -------------------------------*/

void NMainFrameWidget::resizeEvent ( QResizeEvent *evt ) {
	if( !toolContainer_->isHidden() )
	    toolContainer_->move( width() - toolContainer_->width(), height() - toolContainer_->height() );
	if (!inPart_) setDrumToolbar();
	width_ = evt->size().width(); height_ = evt->size().height();
	scrollx_->setGeometry(BORDER, height()-2*BORDER-SCROLLBARHEIGHT,
			width() - 2*BORDER, SCROLLBARHEIGHT);
	setScrollableNotePage();
}


/*--------------------------- reaction on Ok button of the dialogs above ----------------*/

void NMainFrameWidget::KE_moveUp() {
	if (playing_) return;
	QPoint curpos;
	int ydist;
	if (NResource::allowKeyboardInsert_) {
		curpos = notePart_->mapFromGlobal(cursor().pos());
		if (keyLine_ == NULL_LINE) {
			ydist = TRANSY(curpos.y()) - currentStaff_->getBase();
			keyLine_ = ydist / LINE_DIST;
			keyOffs_ = 0;
		}
		if (keyOffs_) {
			keyOffs_ = 0;
		}
		else {
			keyOffs_ = 1;	
			keyLine_--;
		}
		ydist = keyLine_ * LINE_DIST + keyOffs_ * (LINE_DIST / 2 + 1);
		curpos.setY(RETRANSY(currentStaff_->getBase()+ydist));
		cursor().setPos(notePart_->mapToGlobal(curpos));
		return;
	}
	moveUp();
}

void NMainFrameWidget::KE_moveDown() {
	if (playing_) return;
	QPoint curpos;
	int ydist;
	if (NResource::allowKeyboardInsert_) {
		curpos = notePart_->mapFromGlobal(cursor().pos());
		if (keyLine_ == NULL_LINE) {
			ydist = TRANSY(curpos.y()) - currentStaff_->getBase();
			keyLine_ = ydist / LINE_DIST;
			keyOffs_ = 0;
		}
		if (keyOffs_) {
			keyOffs_ = 0;
			++keyLine_;
		}
		else {
			keyOffs_ = 1;
		}
		ydist = keyLine_ * LINE_DIST + keyOffs_ * (LINE_DIST / 2 + 1);
		curpos.setY(RETRANSY(currentStaff_->getBase()+ydist));
		cursor().setPos(notePart_->mapToGlobal(curpos));
		return;
	}
	moveDown();
}

void NMainFrameWidget::KE_moveSemiUp() {
	if (playing_) return;
	if (NResource::allowKeyboardInsert_) return;
	moveSemiToneUp();
}

void NMainFrameWidget::KE_moveSemiDown() {
	if (playing_) return;
	if (NResource::allowKeyboardInsert_) return;
	moveSemiToneDown();
}

void NMainFrameWidget::KE_moveLeft() {
	if (playing_) return;
	NMusElement *elem;
	int newXpos;
	QPoint curpos;
	prevElement();
	if ((elem = currentVoice_->getCurrentElement()) == 0) return;
	if (!NResource::allowKeyboardInsert_) {
		newXpos = currentVoice_->getCurrentElement()->getXpos();
		if (newXpos - SMALL_X_SENS_DIST< leftx_) {
				scrollx_->setValue(leftx_ - SMALL_X_SCROLL < 0 ? 0 : leftx_ - SMALL_X_SCROLL);
		}
		return;
	}
	curpos = notePart_->mapFromGlobal(cursor().pos());
	newXpos = currentVoice_->getCurrentElement()->getXpos();
	if (newXpos - SMALL_X_SENS_DIST< leftx_) {
		scrollx_->setValue(leftx_ - SMALL_X_SCROLL < 0 ? 0 : leftx_ - SMALL_X_SCROLL);
	}
	curpos.setX((int) ((newXpos-leftx_) * main_props_.zoom)),
	cursor().setPos(notePart_->mapToGlobal(curpos));
}

void NMainFrameWidget::KE_moveStart() {
	if (playing_) return;
	scrollx_->setValue(0);
}
void NMainFrameWidget::KE_moveEnd() {
	if (playing_) return;
	int newXpos;
	newXpos = lastXpos_ - width_;
	if (newXpos < 0) newXpos = 0;
	scrollx_->setValue(newXpos);
}

void NMainFrameWidget::KE_moveRight() {
	if (playing_) return;
	NMusElement *elem;
	int newXpos;
	QPoint curpos;

	nextElement();
	if ((elem = currentVoice_->getCurrentElement()) == 0) return;
	if (!NResource::allowKeyboardInsert_) {
		newXpos = currentVoice_->getCurrentElement()->getXpos();
		currentVoice_->getCurrentElement()->getBbox()->width();
		if (newXpos + SMALL_X_SENS_DIST> leftx_ + paperScrollWidth_) {
			scrollx_->setValue(leftx_ + SMALL_X_SCROLL);
		}
		return;
	}
	curpos = notePart_->mapFromGlobal(cursor().pos());
	newXpos = currentVoice_->getCurrentElement()->getXpos()+
			currentVoice_->getCurrentElement()->getBbox()->width()+CUR_DIST;
	if (newXpos + SMALL_X_SENS_DIST> leftx_ + paperScrollWidth_) {
		scrollx_->setValue(leftx_ + SMALL_X_SCROLL);
	}
	curpos.setX((int) ((newXpos-leftx_) * main_props_.zoom)), cursor().setPos(notePart_->mapToGlobal(curpos));
}

void NMainFrameWidget::KE_delete() {
	if (playing_) return;
	NMusElement *elem;
	QPoint curpos;
	if (NResource::windowWithSelectedRegion_) deleteBlock();
	else deleteElem(false);
	
	if (!NResource::allowKeyboardInsert_) return;
	if ((elem = currentVoice_->getCurrentElement()) == 0) return;
	curpos = notePart_->mapFromGlobal(cursor().pos());
	curpos.setX((int) ((elem->getXpos()+ elem->getBbox()->width()+CUR_DIST) * main_props_.zoom));
	cursor().setPos(notePart_->mapToGlobal(curpos));
}

void NMainFrameWidget::KE_leaveCurrentMode() {
	selectbutton_->setOn(true);
	setSelectMode();
}

void NMainFrameWidget::KE_play() {
	playbutton_->toggle();
}

void NMainFrameWidget::KE_edit() {
	if (playing_) return;
	editbutton_->toggle();
}

void NMainFrameWidget::KE_insertnote() {
	if (playing_) return;
	NMusElement *elem;
	int newXpos;
	int line;
	double dline;
	QPoint curpos;
	if (!NResource::allowKeyboardInsert_ || main_props_.actualLength <= 0) return;
	curpos = notePart_->mapFromGlobal(cursor().pos());
	TRANSY2LINE(curpos.y(), dline, line);
	if (line >= MINLINE && line <= MAXLINE) {
		currentVoice_->insertAtPosition(T_CHORD, TRANSX(curpos.x()), line, main_props_.actualLength, actualOffs_);
		resetSpecialButtons();
	}
	setEdited();
	computeMidiTimes(NResource::automaticBarInsertion_, NResource::autoBeamInsertion_);
	reposit();
	if ((elem = currentVoice_->getCurrentElement()) == 0) return;
	newXpos = currentVoice_->getCurrentElement()->getXpos()+
	          currentVoice_->getCurrentElement()->getBbox()->width()+CUR_DIST;
	if (newXpos + SMALL_X_SENS_DIST> leftx_ + paperScrollWidth_) {
			scrollx_->setValue(leftx_ + SMALL_X_SCROLL);
	}
	curpos.setX((int) ((newXpos-leftx_) * main_props_.zoom)), cursor().setPos(notePart_->mapToGlobal(curpos));
	repaint();
}

void NMainFrameWidget::KE_insertchordnote() {
	if (playing_) return;
	NMusElement *elem;
	int newXpos;
	int line;
	double dline;
	QPoint curpos;
	if (!NResource::allowKeyboardInsert_) return;
	curpos = notePart_->mapFromGlobal(cursor().pos());
	TRANSY2LINE(curpos.y(), dline, line);
	if (line < MINLINE || line > MAXLINE) return;
	if (currentVoice_->insertNewNoteAtCurrent(line, actualOffs_)) {
		setEdited();
		reposit();
		resetSpecialButtons();
		if ((elem = currentVoice_->getCurrentElement()) == 0) return;
		newXpos = currentVoice_->getCurrentElement()->getXpos()+
	          	currentVoice_->getCurrentElement()->getBbox()->width()+CUR_DIST;
		if (newXpos + SMALL_X_SENS_DIST> leftx_ + paperScrollWidth_) {
				scrollx_->setValue(leftx_ + SMALL_X_SCROLL);
		}
		curpos.setX((int) ((newXpos-leftx_) * main_props_.zoom)), cursor().setPos(notePart_->mapToGlobal(curpos));
		repaint();
	}
}

void NMainFrameWidget::KE_1() {
	if (playing_) return;
	note_buttons_[1]->setOn(true);
	setToFull(true);
}
void NMainFrameWidget::KE_2() {
	if (playing_) return;
	note_buttons_[2]->setOn(true);
	setToHalf(true);
}
void NMainFrameWidget::KE_3() {
	if (playing_) return;
	tripletbutton_->toggle();
}
void NMainFrameWidget::KE_4() {
	if (playing_) return;
	note_buttons_[3]->setOn(true);
	setToQuarter(true);
}
void NMainFrameWidget::KE_5() {
	if (playing_) return;
	note_buttons_[4]->setOn(true);
	setToN8(true);
}
void NMainFrameWidget::KE_6() {
	if (playing_) return;
	note_buttons_[5]->setOn(true);
	setToN16(true);
}
void NMainFrameWidget::KE_7() {
	if (playing_) return;
	note_buttons_[6]->setOn(true);
	setToN32(true);
}
void NMainFrameWidget::KE_8() {
	if (playing_) return;
	note_buttons_[7]->setOn(true);
	setToN64(true);
}
void NMainFrameWidget::KE_9() {
	if (playing_) return;
	note_buttons_[8]->setOn(true);
	setToN128(true);
}
void NMainFrameWidget::KE_voice1() {
	if (voiceDisplay_->getVal() == 1) {
		voiceDisplay_->setVal(0);
		changeActualVoice(0);
	} else {
		voiceDisplay_->setVal(1);
		changeActualVoice(1);
	}
}
void NMainFrameWidget::KE_voice2() {
	if (voiceDisplay_->getVal() == 2) {
		voiceDisplay_->setVal(0);
		changeActualVoice(0);
	} else {
		voiceDisplay_->setVal(2);
		changeActualVoice(2);
	}
}
void NMainFrameWidget::KE_voice3() {
	if (voiceDisplay_->getVal() == 3) {
		voiceDisplay_->setVal(0);
		changeActualVoice(0);
	} else {
		voiceDisplay_->setVal(3);
		changeActualVoice(3);
	}
}
void NMainFrameWidget::KE_voice4() {
	if (voiceDisplay_->getVal() == 4) {
		voiceDisplay_->setVal(0);
		changeActualVoice(0);
	} else {
		voiceDisplay_->setVal(4);
		changeActualVoice(4);
	}
}
void NMainFrameWidget::KE_voice5() {
	if (voiceDisplay_->getVal() == 5) {
		voiceDisplay_->setVal(0);
		changeActualVoice(0);
	} else {
		voiceDisplay_->setVal(5);
		changeActualVoice(5);
	}
}
void NMainFrameWidget::KE_voice6() {
	if (voiceDisplay_->getVal() == 6) {
		voiceDisplay_->setVal(0);
		changeActualVoice(0);
	} else {
		voiceDisplay_->setVal(6);
		changeActualVoice(6);
	}
}
void NMainFrameWidget::KE_voice7() {
	if (voiceDisplay_->getVal() == 7) {
		voiceDisplay_->setVal(0);
		changeActualVoice(0);
	} else {
		voiceDisplay_->setVal(7);
		changeActualVoice(7);
	}
}
void NMainFrameWidget::KE_voice8() {
	if (voiceDisplay_->getVal() == 8) {
		voiceDisplay_->setVal(0);
		changeActualVoice(0);
	} else {
		voiceDisplay_->setVal(8);
		changeActualVoice(8);
	}
}
void NMainFrameWidget::KE_voice9() {
	if (voiceDisplay_->getVal() == 9) {
		voiceDisplay_->setVal(0);
		changeActualVoice(0);
	} else {
		voiceDisplay_->setVal(9);
		changeActualVoice(9);
	}
}
void NMainFrameWidget::KE_tie() {
	if (playing_) return;
	tiebutton_->toggle();
}
void NMainFrameWidget::KE_dot() {
	if (playing_) return;
	dotbutton_->toggle();
}
void NMainFrameWidget::KE_flat() {
	if (playing_) return;
	offs_buttons_[1]->toggle();
}
void NMainFrameWidget::KE_sharp() {
	if (playing_) return;
	offs_buttons_[0]->toggle();
}
void NMainFrameWidget::KE_natural() {
	if (playing_) return;
	offs_buttons_[2]->toggle();
}
void NMainFrameWidget::KE_bar() {
	if (playing_) return;
	if (!currentVoice_->isFirstVoice()) return;
	QPoint curpos;
	NMusElement *elem;
	currentVoice_->insertBarAt(cursor().pos().x()-geometry().left());
	computeMidiTimes(false);
	setEdited();
	reposit();
	repaint();
	curpos = notePart_->mapFromGlobal(cursor().pos());
	if ((elem = currentVoice_->getCurrentElement()) == 0) return;
	curpos.setX((int) ((currentVoice_->getCurrentElement()->getXpos()+
			currentVoice_->getCurrentElement()->getBbox()->width()+CUR_DIST) * main_props_.zoom));
	cursor().setPos(notePart_->mapToGlobal(curpos));
}

void NMainFrameWidget::KE_remove() {
	if (playing_) return;
	QPoint curpos;
	NMusElement *elem;
	int newXpos;
	if (NResource::windowWithSelectedRegion_) deleteBlock();
	else deleteElem(true);
	if ((elem = currentVoice_->getCurrentElement()) == 0) return;
	newXpos = elem->getXpos();
	if (newXpos - SMALL_X_SENS_DIST< leftx_) {
		scrollx_->setValue(leftx_ - SMALL_X_SCROLL < 0 ? 0 : leftx_ - SMALL_X_SCROLL);
	}
	if (!NResource::allowKeyboardInsert_) return;
	curpos = notePart_->mapFromGlobal(cursor().pos());
	curpos.setX((int) ((elem->getXpos()+ elem->getBbox()->width()+CUR_DIST-leftx_) * main_props_.zoom));
	cursor().setPos(notePart_->mapToGlobal(curpos));
}

void NMainFrameWidget::KE_removechordnote() {
	if (playing_) return;
	QPoint curpos;
	NMusElement *elem;
	int newXpos;
	if (!NResource::allowKeyboardInsert_) return;
	curpos = notePart_->mapFromGlobal(cursor().pos());
	if (currentVoice_->deleteAtPosition(TRANSY(curpos.y()))) {
		setEdited();
		if ((elem = currentVoice_->getCurrentElement()) == 0) return;
		newXpos = elem->getXpos();
		if (newXpos - SMALL_X_SENS_DIST< leftx_) {
			scrollx_->setValue(leftx_ - SMALL_X_SCROLL < 0 ? 0 : leftx_ - SMALL_X_SCROLL);
		}
		curpos = notePart_->mapFromGlobal(cursor().pos());
		curpos.setX((int) ((elem->getXpos()+ elem->getBbox()->width()+CUR_DIST-leftx_) * main_props_.zoom));
		cursor().setPos(notePart_->mapToGlobal(curpos));
		repaint();
	}
}

void NMainFrameWidget::KE_tab() {
	if (playing_) return;
	if (!currentVoice_->isFirstVoice()) return;
	NMusElement *elem;
	QPoint curpos;
	int newXpos;
	currentVoice_->insertAfterCurrent(T_SIGN, SIMPLE_BAR);
	computeMidiTimes(false);
	reposit();
	repaint();
	if (!NResource::allowKeyboardInsert_) return;
	if ((elem = currentVoice_->getCurrentElement()) == 0) return;
	curpos = notePart_->mapFromGlobal(cursor().pos());
	newXpos = currentVoice_->getCurrentElement()->getXpos()+
			currentVoice_->getCurrentElement()->getBbox()->width()+CUR_DIST;
	if (newXpos + SMALL_X_SENS_DIST> leftx_ + paperScrollWidth_) {
		scrollx_->setValue(leftx_ + SMALL_X_SCROLL);
	}
	curpos.setX((int) ((newXpos-leftx_) * main_props_.zoom)), cursor().setPos(notePart_->mapToGlobal(curpos));
}

void NMainFrameWidget::KE_insertRest() {
	if (playing_) return;
	int newXpos;
	if (!NResource::allowKeyboardInsert_ || main_props_.actualLength <= 0) return;
	NMusElement *elem;
	QPoint curpos;
	curpos = notePart_->mapFromGlobal(cursor().pos());
	currentVoice_->insertAtPosition(T_REST, TRANSX(curpos.x()), 0 /* dummy */, main_props_.actualLength, actualOffs_);
	resetSpecialButtons();
	setEdited();
	computeMidiTimes(false);
	reposit();
	if ((elem = currentVoice_->getCurrentElement()) == 0) return;
	newXpos = currentVoice_->getCurrentElement()->getXpos()+
			currentVoice_->getCurrentElement()->getBbox()->width()+CUR_DIST;
	if (newXpos + SMALL_X_SENS_DIST> leftx_ + paperScrollWidth_) {
			scrollx_->setValue(leftx_ + SMALL_X_SCROLL);
	}
	curpos.setX((int) ((newXpos-leftx_) * main_props_.zoom)), cursor().setPos(notePart_->mapToGlobal(curpos));
	repaint();
}

void NMainFrameWidget::KE_underscore() {
	if (playing_) return;
	beambutton_->toggle();
}
void NMainFrameWidget::KE_keybordInsert() {
	if (playing_) return;
	if (NResource::allowKeyboardInsert_) {
		NResource::allowKeyboardInsert_ = false;
	}
	else {
		NResource::allowKeyboardInsert_ = true;
	}
	allowKbInsertButton_->setOn(NResource::allowKeyboardInsert_);
}

void NMainFrameWidget::KE_pitch_C() {
	pitchToLine(0);
}
void NMainFrameWidget::KE_pitch_D() {
	pitchToLine(1);
}
void NMainFrameWidget::KE_pitch_E() {
	pitchToLine(2);
}
void NMainFrameWidget::KE_pitch_F() {
	pitchToLine(3);
}
void NMainFrameWidget::KE_pitch_G() {
	pitchToLine(4);
}
void NMainFrameWidget::KE_pitch_A() {
	pitchToLine(5);
}
void NMainFrameWidget::KE_pitch_B() {
	pitchToLine(6);
}

void NMainFrameWidget::pitchToLine(int pitchNumber) {
	if (playing_) return;
	int halfLines, offs;
	int newXpos;
	property_type properties;
	NChord *newchord;
	QPoint curpos;
	int ydist;
	curpos = notePart_->mapFromGlobal(cursor().pos());
	if (keyLine_ == NULL_LINE) {
		ydist = TRANSY(curpos.y()) - currentStaff_->getBase();
		keyLine_ = ydist / (LINE_DIST / 2);
		keyOffs_ = 0;
	}
	halfLines = currentStaff_->findLineOf(pitchNumber, 8 - keyLine_, TRANSX(curpos.x()));
	keyLine_ = 8 - halfLines;
	ydist = keyLine_ * LINE_DIST / 2;
	if (kbbutton_->isOn()) {
		offs = currentStaff_->actualKeysig_.getOffset(halfLines);
		if (NResource::allowInsertEcho_) {
			NResource::mapper_->playImmediately(&(currentStaff_->actualClef_), 
				halfLines, offs, currentStaff_->getVoice(), currentStaff_->getChannel(), currentStaff_->getVolume(), currentStaff_->transpose_);
		}
		if (main_props_.actualLength > 0 && kbInsertButton_->isOn()) {
			properties = 0;
			if (main_props_.tied) properties |= PROP_TIED;
			if (main_props_.staccato) properties |= PROP_STACC;
			if (main_props_.sforzato) properties |= PROP_SFORZ;
			if (main_props_.portato) properties |= PROP_PORTA;
			if (main_props_.strong_pizzicato) properties |= PROP_STPIZ;
			if (main_props_.sforzando) properties |= PROP_SFZND;
			if (main_props_.fermate) properties |= PROP_FERMT;
			if (main_props_.grace) properties |= PROP_GRACE;
			if (main_props_.arpeggio) properties |= PROP_ARPEGG;
			properties |= (main_props_.dotcount & DOT_MASK);
			properties |= (main_props_.noteBody & BODY_MASK);
			if (main_props_.pedal_on) properties |= PROP_PEDAL_ON;
			if (main_props_.pedal_off) properties |= PROP_PEDAL_OFF;
			newchord = new NChord(&main_props_, currentStaff_->getStaffPropsAddr(), currentVoice_, halfLines, offs, main_props_.actualLength, currentVoice_->stemPolicy_, properties);
			if (!currentVoice_->insertAfterCurrent(newchord)) return;
			setEdited();
			computeMidiTimes(true);
			reposit();
			newXpos = currentVoice_->getCurrentElement()->getXpos()+ currentVoice_->getCurrentElement()->getBbox()->width()+CUR_DIST;
			if (newXpos + SMALL_X_SENS_DIST > leftx_ + paperScrollWidth_) {
				scrollx_->setValue(leftx_ + SMALL_X_SCROLL);
				return;
			}
			repaint();
		}
		return;
	}
	if (!NResource::allowKeyboardInsert_) return;
	curpos.setY(RETRANSY(currentStaff_->getBase()+ydist));
	cursor().setPos(notePart_->mapToGlobal(curpos));
}


void NMainFrameWidget::paintNextStaff() {
	int j;
	bool isConnected;

	if (!nextStaffElemToBePainted_) return;
	if (nextStaffElemToBePainted_->getBase() > boty_ ) {
		nextStaffElemToBePainted_ = 0;
		return;
	}
	while (nextStaffElemToBePainted_->getBase() < topy_) {
		nextStaffNr_++;
		nextStaffElemToBePainted_ = staffList_.at(nextStaffNr_);
		if (!nextStaffElemToBePainted_) {
			return;
		}
	}
	PREPARE_BAR_CHECK_ARRAY(nextStaffElemToBePainted_, nextStaffNr_, isConnected, nextStaffIsFirstStaff_, j)

	nextStaffElemToBePainted_->draw(newLeft_, newRight_);
	nextStaffNr_++;
	nextStaffElemToBePainted_ = staffList_.at(nextStaffNr_);
}

void NMainFrameWidget::addVoice(NVoice *voice, int numVoices) {
	voiceList_.append(voice);
	voiceDisplay_->setMax(numVoices);
	enableCriticalButtons(currentVoice_->isFirstVoice());
	setEdited();
}

void NMainFrameWidget::removeVoice(NVoice *voice, NVoice *newCurrentVoice, int actualVoiceNr, int numVoices) {
	if (voiceList_.find(voice) == -1) {
		NResource::abort("NMainFrameWidget::removeVoice: internal error");
	}
	voiceList_.remove();
	currentVoice_ = newCurrentVoice;
	enableCriticalButtons(currentVoice_->isFirstVoice());
	voiceDisplay_->setMax(numVoices);
	voiceDisplay_->setVal(actualVoiceNr+1);
	setEdited();
}


void NMainFrameWidget::paintEvent( QPaintEvent * ) {
	int newleftx;
	NStaff *staff_elem;
	int clipleft;
	int i, j;
	bool isConnected;
	bool isFirst;

	if (playing_) {
		if (!firstNoteActive_) {
			notePart_->flip();
			clipleft = (int) ((float) contextWidth_ * main_props_.zoom);

			main_props_.directPainter->noticeClipRect ( QRect(clipleft, TOP_BOTTOM_BORDER, paperWidth_ - RIGHT_PAGE_BORDER - clipleft,  nettoHeight_ ));
			main_props_.tp->noticeClipRect ( QRect(clipleft, TOP_BOTTOM_BORDER, paperWidth_ - RIGHT_PAGE_BORDER - clipleft,  nettoHeight_ ));
		}
		return;
	}
	main_props_.p->begin( notePart_->acShowPixmap());
	main_props_.p->setBrush(NResource::backgroundBrush_);
	main_props_.p->setPen(NResource::noPen_);
	main_props_.p->setPen(editMode_ ? NResource::editModeBorderPen_ : NResource::blackPen_); //  color of border around drawing area
	main_props_.p->drawRect(0, 0, paperWidth_, paperHeight_);
	main_props_.p->end();
	if (NResource::windowWithSelectedRegion_ == this) {
		main_props_.tp->beginTranslated( );
		main_props_.tp->fillRect(selRect_, NResource::selectionBackgroundBrush_);
		main_props_.tp->end();
	}
	if (layoutPixmap_) {
		main_props_.p->beginUnclippedYtranslated();
		main_props_.p->drawPixmap(LAYOUT_PIXMAP_X_DIST, 0, *layoutPixmap_);
		main_props_.p->end();
	}
	isFirst = true;
	for (i = 0, staff_elem = staffList_.first(); staff_elem; i++, staff_elem = staffList_.next()) {
		if (staff_elem->getBase() < topy_ || staff_elem->getBase() > boty_) continue;
		isConnected = false;
		for (j = 0; !isConnected && j < staffCount_; j++) {
			if (barCont_[j].valid && i >= barCont_[j].beg && i < barCont_[j].end) {
				isConnected = true;
			}
		}
		if (isConnected) {
			if (isFirst) {
				NResource::resetBarCkeckArray(staff_elem->getBase()+4*LINE_DIST, true);
				isFirst = false;
			}
			else {
				NResource::resetBarCkeckArray(staff_elem->getBase()+4*LINE_DIST, false);
			}
		}
		else {
			if (isFirst) {
				NResource::resetBarCkeckArray(-1, true);
				isFirst = false;
			}
			else {
				NResource::resetBarCkeckArray(-1, false);
			}
		}
		staff_elem->draw(leftx_, leftx_ + (int) ((float) nettoWidth_ / main_props_.zoom));
	}
	
	/* draw the initial always-present barline connecting all the staves */
	if (staffCount_ > 1) {
		main_props_.tp->beginYtranslated();
		main_props_.tp->setPen(NResource::staffPen_);
		main_props_.tp->drawLine( main_props_.left_page_border, staffList_.getFirst()->getBase(),
			                      main_props_.left_page_border, staffList_.getLast()->getBase() + 4*LINE_DIST );
		main_props_.tp->end();
	}

	notePart_->setMouseTracking(false);
	restoreAllBehindDummyNoteAndAuxLines();
	notePart_->flip();
	if (leftx_ + lastXpos_ < (int) ((float) nettoWidth_ / main_props_.zoom)) {
		scrollx_->setRange ( 0, 0);
	}
	else if (abs(oldLastXpos_ - lastXpos_) > (int) ((float) nettoWidth_ / main_props_.zoom) / 4 || lastXpos_ >= (int) ((float) nettoWidth_ / main_props_.zoom) - SMALL_X_SCROLL) {
		if (lastXpos_ < leftx_) {
			newleftx = lastXpos_ - (int) ((float) nettoWidth_ / main_props_.zoom);
			scrollx_->setValue(newleftx < 0 ? 0 : newleftx);
		}
		scrollx_->setRange ( 0, lastXpos_);
	}
	notePart_->setMouseTracking(NResource::showAuxLines_);
}

/*-------------------------- reaction on pushbutton events ----------------------------- */

void NMainFrameWidget::setSelectMode() {
	stemUpbutton_->setOn(false);
	stemDownbutton_->setOn(false);
	tiebutton_->setOn(false);
	main_props_.actualLength = -1;
	main_props_.tied = false;
	main_props_.grace = false;
	notePart_->setCursor(arrowCursor);
	
	if (editMode_) {
		editbutton_->setOn(false);
		editMode_ = false;
		repaint();
	} 
}

void NMainFrameWidget::setToDFull(bool on) {
	if (playing_) return;
	if (!on && !editMode_) {
		setSelectMode();
		return;
	}
	if (on) {
		main_props_.grace = false;
		main_props_.actualLength = DOUBLE_WHOLE_LENGTH;
	}
	if (on && editMode_) {
		currentVoice_->changeActualChord();
		computeMidiTimes(false);
		setEdited();
		reposit();
		repaint();
	}
	else if (!editMode_) {
		notePart_->setCursor( *NResource::cursor_breve_);
	}
	if (NResource::windowWithSelectedRegion_) {
		NResource::windowWithSelectedRegion_ = 0;
		repaint();
	}
}
void NMainFrameWidget::setToFull(bool on) {
	if (playing_) return;
	if (!on && !editMode_) {
		setSelectMode();
		return;
	}
	if (on) {
		main_props_.grace = false;
		main_props_.actualLength = WHOLE_LENGTH;
	}
	if (on && editMode_) {
		currentVoice_->changeActualChord();
		computeMidiTimes(false);
		setEdited();
		reposit();
		repaint();
	}
	else if (!editMode_) {
		notePart_->setCursor( *NResource::cursor_fullnote_);
	}
	if (NResource::windowWithSelectedRegion_) {
		NResource::windowWithSelectedRegion_ = 0;
		repaint();
	}

}
void NMainFrameWidget::setToHalf(bool on) {
	if (playing_) return;
	if (!on && !editMode_) {
		setSelectMode();
		return;
	}
	if (on) {
		main_props_.grace = false;
		main_props_.actualLength = HALF_LENGTH;
	}
	if (on && editMode_) {
		currentVoice_->changeActualChord();
		computeMidiTimes(false);
		setEdited();
		reposit();
		repaint();
	}
	else if (!editMode_) {
		notePart_->setCursor( *NResource::cursor_halfnote_);
	}
	if (NResource::windowWithSelectedRegion_) {
		NResource::windowWithSelectedRegion_ = 0;
		repaint();
	}
}
void NMainFrameWidget::setToQuarter(bool on) {
	if (playing_) return;
	if (!on && !editMode_) {
		setSelectMode();
		return;
	}
	if (on) {
		main_props_.grace = false;
		main_props_.actualLength = QUARTER_LENGTH;
	}
	if (on && editMode_) {
		currentVoice_->changeActualChord();
		computeMidiTimes(false);
		setEdited();
		reposit();
		repaint();
	}
	else if (!editMode_) {
		notePart_->setCursor( *NResource::cursor_quarternote_);
	}
	if (NResource::windowWithSelectedRegion_) {
		NResource::windowWithSelectedRegion_ = 0;
		repaint();
	}
}
void NMainFrameWidget::setToN8(bool on) {
	if (playing_) return;
	if (!on && !editMode_) {
		setSelectMode();
		return;
	}
	if (on) {
		main_props_.actualLength = NOTE8_LENGTH;
		main_props_.grace = false;
	}
	if (on && editMode_) {
		currentVoice_->changeActualChord();
		computeMidiTimes(false);
		setEdited();
		reposit();
		repaint();
	}
	else if (!editMode_) {
		notePart_->setCursor( *NResource::cursor_eightnote_);
	}
	if (NResource::windowWithSelectedRegion_) {
		NResource::windowWithSelectedRegion_ = 0;
		repaint();
	}
}
void NMainFrameWidget::setToN16(bool on) {
	if (playing_) return;
	if (!on && !editMode_) {
		setSelectMode();
		return;
	}
	if (on) {
		main_props_.actualLength = NOTE16_LENGTH;
		main_props_.grace = false;
	}
	if (on && editMode_) {
		currentVoice_->changeActualChord();
		computeMidiTimes(false);
		setEdited();
		reposit();
		repaint();
	}
	else if (!editMode_) {
		notePart_->setCursor( *NResource::cursor_tinysixteenth_);
	}
	if (NResource::windowWithSelectedRegion_) {
		NResource::windowWithSelectedRegion_ = 0;
		repaint();
	}
}
void NMainFrameWidget::setToN32(bool on) {
	if (playing_) return;
	if (!on && !editMode_) {
		setSelectMode();
		return;
	}
	if (on) {
		main_props_.actualLength = NOTE32_LENGTH;
		main_props_.grace = false;
	}
	if (on && editMode_) {
		currentVoice_->changeActualChord();
		computeMidiTimes(false);
		setEdited();
		reposit();
		repaint();
	}
	else if (!editMode_) {
		notePart_->setCursor( *NResource::cursor_32ndnote_);
	}
	if (NResource::windowWithSelectedRegion_) {
		NResource::windowWithSelectedRegion_ = 0;
		repaint();
	}
}
void NMainFrameWidget::setToN64(bool on) {
	if (playing_) return;
	if (!on && !editMode_) {
		setSelectMode();
		return;
	}
	if (on) {
		main_props_.actualLength = NOTE64_LENGTH;
		main_props_.grace = false;
	}
	if (on && editMode_) {
		currentVoice_->changeActualChord();
		computeMidiTimes(false);
		setEdited();
		reposit();
		repaint();
	}
	else if (!editMode_) {
		notePart_->setCursor( *NResource::cursor_64thnote_);
	}
	if (NResource::windowWithSelectedRegion_) {
		NResource::windowWithSelectedRegion_ = 0;
		repaint();
	}
}
void NMainFrameWidget::setToN128(bool on) {
	if (playing_) return;
	if (!on && !editMode_) {
		setSelectMode();
		return;
	}
	if (on) {
		main_props_.actualLength = NOTE128_LENGTH;
		main_props_.grace = false;
	}
	if (on && editMode_) {
		currentVoice_->changeActualChord();
		computeMidiTimes(false);
		setEdited();
		reposit();
		repaint();
	}
	else if (!editMode_) {
		notePart_->setCursor( *NResource::cursor_128thnote_);
	}
	if (NResource::windowWithSelectedRegion_) {
		NResource::windowWithSelectedRegion_ = 0;
		repaint();
	}
}

void NMainFrameWidget::setToGN8(bool on) {
	if (playing_) return;
	if (!on && !editMode_) {
		setSelectMode();
		return;
	}
	if (!editMode_) {
		notePart_->setCursor( *NResource::cursor_tinyeight_);
	}
	if (on) {
		main_props_.grace = true;
		main_props_.actualLength = NOTE8_LENGTH;
	}
	if (NResource::windowWithSelectedRegion_) {
		NResource::windowWithSelectedRegion_ = 0;
		repaint();
	}
}

void NMainFrameWidget::setToGN16(bool on) {
	if (playing_) return;
	if (!on && !editMode_) {
		setSelectMode();
		return;
	}
	if (!editMode_) {
		notePart_->setCursor( *NResource::cursor_tinysixteenth_);
	}
	if (on) {
		main_props_.grace = true;
		main_props_.actualLength = NOTE16_LENGTH;
	}
	if (NResource::windowWithSelectedRegion_) {
		NResource::windowWithSelectedRegion_ = 0;
		repaint();
	}
}
	
void NMainFrameWidget::setToGNS8(bool on) {
	if (playing_) return;
	if (!on && !editMode_) {
		setSelectMode();
		return;
	}
	if (!editMode_) {
		notePart_->setCursor( *NResource::cursor_tinystroke_);
	}
	if (on) {
		main_props_.grace = true;
		main_props_.actualLength = INTERNAL_MARKER_OF_STROKEN_GRACE;
	}
	if (NResource::windowWithSelectedRegion_) {
		NResource::windowWithSelectedRegion_ = 0;
		repaint();
	}
}


void NMainFrameWidget::setCrossBody(bool on) {
	if (playing_) return;
	if (on) {
		main_props_.noteBody = PROP_BODY_CROSS;
	}
	else {
		main_props_.noteBody &= (~PROP_BODY_CROSS);
	}
	if (editMode_) {
		currentVoice_->changeBodyOfActualElement();
		computeMidiTimes(false);
		setEdited();
		reposit();
		repaint();
	}
	if (NResource::windowWithSelectedRegion_) {
		NResource::windowWithSelectedRegion_ = 0;
		repaint();
	}
}

void NMainFrameWidget::setCross2Body(bool on) {
	if (playing_) return;
	if (on) {
		main_props_.noteBody = PROP_BODY_CROSS2;
	}
	else {
		main_props_.noteBody &= (~PROP_BODY_CROSS2);
	}
	if (editMode_) {
		currentVoice_->changeBodyOfActualElement();
		computeMidiTimes(false);
		setEdited();
		reposit();
		repaint();
	}
	if (NResource::windowWithSelectedRegion_) {
		NResource::windowWithSelectedRegion_ = 0;
		repaint();
	}
}

void NMainFrameWidget::setCrossCircBody(bool on) {
	if (playing_) return;
	if (on) {
		main_props_.noteBody = PROP_BODY_CIRCLE_CROSS;
	}
	else {
		main_props_.noteBody &= (~PROP_BODY_CIRCLE_CROSS);
	}
	if (editMode_) {
		currentVoice_->changeBodyOfActualElement();
		computeMidiTimes(false);
		setEdited();
		reposit();
		repaint();
	}
	if (NResource::windowWithSelectedRegion_) {
		NResource::windowWithSelectedRegion_ = 0;
		repaint();
	}
}

void NMainFrameWidget::setRectBody(bool on) {
	if (playing_) return;
	if (on) {
		main_props_.noteBody = PROP_BODY_RECT;
	}
	else {
		main_props_.noteBody &= (~PROP_BODY_RECT);
	}
	if (editMode_) {
		currentVoice_->changeBodyOfActualElement();
		computeMidiTimes(false);
		setEdited();
		reposit();
		repaint();
	}
	if (NResource::windowWithSelectedRegion_) {
		NResource::windowWithSelectedRegion_ = 0;
		repaint();
	}
}

void NMainFrameWidget::setTrianBody(bool on) {
	if (playing_) return;
	if (on) {
		main_props_.noteBody = PROP_BODY_TRIA;
	}
	else {
		main_props_.noteBody &= (~PROP_BODY_TRIA);
	}
	if (editMode_) {
		currentVoice_->changeBodyOfActualElement();
		computeMidiTimes(false);
		setEdited();
		reposit();
		repaint();
	}
	if (NResource::windowWithSelectedRegion_) {
		NResource::windowWithSelectedRegion_ = 0;
		repaint();
	}
}

void NMainFrameWidget::resetSpecialButtons() {
	int i;
	if (gluebutton_->isOn()) return;
	if (editMode_) return;
	for (i = 0; i < COUNT_OFFSBUTTONS; i++) {
		offs_buttons_[i]->setOn(false);
	}
	actualOffs_ = UNDEFINED_OFFS;
	dotbutton_->setOn(false);
	ddotbutton_->setOn(false);
	main_props_.dotcount = 0;
}
	

void NMainFrameWidget::toggleDrumUp() {
	NResource::showDrumToolbar_ = !NResource::showDrumToolbar_;
	setDrumToolbar();
}

void NMainFrameWidget::setDrumToolbar() {
	NMainWindow *mainWindow = static_cast<NMainWindow *>(parentWidget());
	KToolBar * tb = static_cast<KToolBar *>(mainWindow->child("drum_toolbar", "KToolBar" ) );
        if (NResource::showDrumToolbar_)  {
		tb->show();
	}
	else {
		tb->hide();
		main_props_.noteBody &= (~BODY_MASK);
	}
}


void NMainFrameWidget::playAll(bool on) {
	NMidiEventStr *m_events;
	int min_time = (1 << 30);
	int last_time;
	int start_time;
	NVoice *voice_elem;
	NStaff *staff_elem;
	int midipos;
	struct timeval now;
	if (playing_) {
		playStop_ = true;
		return;
	}
	if (!on) return;
	if (NResource::mapper_->isInUse_) {
		KMessageBox::sorry(this, i18n("MIDI mapper is already in use!"), kapp->makeStdCaption(i18n("Play")));
		playButtonReset();
		return;
	}
	notesToPlay_ = 0;
	playStop_ = false;

	notePart_->setMouseTracking(false);
	restoreAllBehindDummyNoteAndAuxLines();
	currentEvents_.clear();
	nextEvents_.clear();
	stopList_.clear();
	NResource::mapper_->openDevice();
	start_time = currentVoice_->getMidiTime();
	midipos = currentVoice_->getMidiPos();
	turnOverOffset_ = (int) ((double) NResource::turnOverPoint_ / main_props_.zoom);
	if (midipos < leftx_ || midipos > leftx_ + paperScrollWidth_) {
		scrollx_->setValue(currentVoice_->getMidiPos());
	}
	for (voice_elem = voiceList_.first(); voice_elem; voice_elem = voiceList_.next()) {
		voice_elem->getTempoSigs(&SortedTempoSigs_, start_time);
	}
	SortedTempoSigs_.initForPlaying(start_time);
	for (staff_elem = staffList_.first(); staff_elem; staff_elem = staffList_.next()) {
		staff_elem->startPlaying(start_time);
	}
	for (voice_elem = voiceList_.first(); voice_elem; voice_elem = voiceList_.next()) {
		if (voice_elem->muted_) continue;
		m_events = voice_elem->getNextMidiEvent(0, false);
		if (m_events) {
			currentEvents_.append(m_events);
			++notesToPlay_;
		}
		if (m_events && m_events->ev_time < min_time) min_time = m_events->ev_time;
	}
	if (!notesToPlay_) {
		for (voice_elem = voiceList_.first(); voice_elem; voice_elem = voiceList_.next()) {
			voice_elem->stopPlaying();
		}
		playButtonReset();
		KMessageBox::sorry(this, i18n("Nothing to play!"), kapp->makeStdCaption(i18n("Play")));
		notePart_->setMouseTracking(NResource::showAuxLines_);
		return;
	}

	contextWidth_ = NResource::showContext_ ?  CONTEXT_WIDTH  : main_props_.left_page_border;
	if (NResource::showContext_) {
		contextRec_ = QRect((int) ((float) context_rect_left_right_ * main_props_.zoom), (int) ((float) context_rect_left_right_*main_props_.zoom),
				   (int) ((float) (CONTEXT_WIDTH-context_rect_left_right_) * main_props_.zoom), (int) ((float) lastYHeight_*main_props_.zoom));
	}
	preparePixmaps();  // this causes a repaint event because of scrolly_->hide();
	firstNoteActive_ = true; /* Avoid a page flip because of this repaint event!
				  * This is necessary because the repaint event is handled
				  * after end of this method. But this method colors the
				  * first colors red !!
				  */

	last_time = myTime_ = min_time;
	for (m_events = currentEvents_.first(); m_events; m_events = currentEvents_.next()) {
		if (m_events->ev_time == min_time)  {
			m_events->from->skipChord();
			m_events->notehalt->ev_time = min_time + m_events->length;
			stopList_.append(m_events->notehalt);
		}
	}
	min_time = (1 << 30);
	for (voice_elem = voiceList_.first(); voice_elem; voice_elem = voiceList_.next()) {
		if (voice_elem->muted_) continue;
		m_events = voice_elem->getNextMidiEvent(myTime_+1, false);
		if (m_events) {
			nextEvents_.append(m_events);
		}
		if (m_events && m_events->ev_time < min_time) min_time = m_events->ev_time;
	}
	for (m_events = stopList_.first(); m_events; m_events = stopList_.next()) {
		nextEvents_.append(m_events);
		if (m_events->ev_time < min_time) min_time = m_events->ev_time;
	}
	notesToPlay_ = 0;
	for (m_events = nextEvents_.first(); m_events; m_events = nextEvents_.next()) {
		if (m_events->ev_time == min_time)  {
			if (m_events->midi_cmd == MNOTE_OFF) {
				stopList_.find(m_events);
				stopList_.remove();
			}
			else  {
				m_events->from->skipChord();
				m_events->notehalt->ev_time = min_time + m_events->length;
				stopList_.append(m_events->notehalt);
			}
			++notesToPlay_;
		}
	}
	playing_ = true;
#ifdef WITH_TSE3
	kbbutton_->setOn(false);
#endif
	myTime_ = min_time;
	nextToPlay_ = &nextEvents_;
	nextToSearch_ = &currentEvents_;
	NResource::mapper_->isInUse_ = true;
	NResource::mapper_->setPaintDevice(notePart_);
	NResource::mapper_->play_list(&currentEvents_, last_time);

	tempo_ = SortedTempoSigs_.getTempoAtMidiTime(last_time);
	tempofactor_ = 1;
	gettimeofday(&now, NULL);
	add_time(&nextPlayTime_, &now, (int) (((double) (myTime_ - last_time) * 1000.0 * 60.0) / ((double) QUARTER_LENGTH * tempo_)));
	timer_.start((int) (((double) (myTime_ - last_time) * 1000.0 * 60.0) / ((double) QUARTER_LENGTH * tempo_)), true);
}

void NMainFrameWidget::setDotted(bool dotted) {
	if (playing_) return;
	main_props_.dotcount = dotted ? PROP_SINGLE_DOT : 0;
	if (editMode_) {
		currentVoice_->setDotted();
		computeMidiTimes(false);
		setEdited();
		reposit();
		repaint();
	}
}

void NMainFrameWidget::setDDotted(bool ddotted) {
	if (playing_) return;
	main_props_.dotcount = ddotted ? PROP_DOUBLE_DOT : 0;
	if (editMode_) {
		currentVoice_->setDotted();
		computeMidiTimes(false);
		setEdited();
		reposit();
		repaint();
	}
}

void NMainFrameWidget::setActualTied(bool tied) {
	if (playing_) return;
	main_props_.tied = tied;
	if (editMode_) {
		currentVoice_->setActualTied();
		computeMidiTimes(false);
		setEdited();
		reposit();
		repaint();
	}
}

void NMainFrameWidget::setArpegg(bool on) {
	if (playing_) return;
	main_props_.arpeggio = on;
	if (editMode_) {
		currentVoice_->setArpeggio();
		setEdited();
		reposit();
		repaint();
	}
}

void NMainFrameWidget::setPedalOn(bool on) {
	if (playing_) return;
	main_props_.pedal_on = on;
	if (editMode_) {
		currentVoice_->setPedalOn();
		setEdited();
		reposit();
		repaint();
	}
}
void NMainFrameWidget::setPedalOff(bool on) {
	if (playing_) return;
	main_props_.pedal_off = on;
	if (editMode_) {
		currentVoice_->setPedalOff();
		setEdited();
		reposit();
		repaint();
	}
}

void NMainFrameWidget::setBeamed(bool beamed) {
	if (!beamed) {
		currentVoice_->breakBeames();
		repaint();
		setEdited();
		return;
	}
	if (!NResource::windowWithSelectedRegion_) return;
	NResource::voiceWithSelectedRegion_->setBeamed();
	if (!editMode_) {
		beambutton_->setOn(false);
	}
	setEdited();
	repaint();
	repaint();
}

void NMainFrameWidget::setStaccato(bool val)         { this->forceAccent(PROP_STACC, val); }
void NMainFrameWidget::setSforzato(bool val)         { this->forceAccent(PROP_SFORZ, val); }
void NMainFrameWidget::setPortato(bool val)          { this->forceAccent(PROP_PORTA, val); }
void NMainFrameWidget::setStrong_pizzicato(bool val) { this->forceAccent(PROP_STPIZ, val); }
void NMainFrameWidget::setSforzando(bool val)        { this->forceAccent(PROP_SFZND, val); }
void NMainFrameWidget::setFermate(bool val)          { this->forceAccent(PROP_FERMT, val); }

void NMainFrameWidget::setHidden(bool on) {
	if (playing_) return;
	main_props_.hidden = on;
	if (editMode_) {
		currentVoice_->setHidden();
		setEdited();
		reposit();
		repaint();
	}
}

void NMainFrameWidget::forceAccent(property_type acc, bool val) {
	if (playing_) return;
	main_props_.staccato = main_props_.sforzato = main_props_.portato = 
	main_props_.strong_pizzicato = main_props_.sforzando = main_props_.fermate = false;
	switch (acc){
		case PROP_STACC: main_props_.staccato         = val; break;
		case PROP_SFORZ: main_props_.sforzato         = val; break;
		case PROP_PORTA: main_props_.portato  	      = val; break;
		case PROP_STPIZ: main_props_.strong_pizzicato = val; break;
		case PROP_SFZND: main_props_.sforzando        = val; break;
		case PROP_FERMT: main_props_.fermate	      = val; break;
	}
	if (editMode_) {
		currentVoice_->setAccent(acc);
		setEdited();
		repaint();
	}
}


void NMainFrameWidget::manageToolElement(bool becauseOfInsertion) {

    int elcnt = 0;
    NMusElement *elem;
    NChordDiagram *diag;

    if (!editMode_ && !becauseOfInsertion) return;
    elem = currentVoice_->getCurrentElement();
    if( elem && elem->chord() && elem->chord()->trill_ ) {
	bool isneg = false;
	if( currentVoice_->getCurrentElement()->chord()->trill_ < 0 ) {
	    currentVoice_->getCurrentElement()->chord()->trill_ = -currentVoice_->getCurrentElement()->chord()->trill_;
	    isneg = true;
	}
	trillLength_->setValue( currentVoice_->getCurrentElement()->chord()->trill_ );

	if( isneg )
	    currentVoice_->getCurrentElement()->chord()->trill_ = -currentVoice_->getCurrentElement()->chord()->trill_;
	++elcnt;
	tabWid_->setTabEnabled( trillLengthBase_, true );
    }
    else
        tabWid_->setTabEnabled( trillLengthBase_, false );
    

    if( elem && elem->chord() && elem->chord()->dynamic_ ) {
	dynamicPos_->setValue( currentVoice_->getCurrentElement()->chord()->dynamic_ );
	tabWid_->setTabEnabled( dynamicBase_, true );
	++elcnt;
    }
    else 
      tabWid_->setTabEnabled( dynamicBase_, false );

    if( elem && elem->chord() && elem->chord()->va_ ) {
	if( currentVoice_->getCurrentElement()->chord()->va_ < 0 ) {
		vaLength_->setValue( -currentVoice_->getCurrentElement()->chord()->va_ );
	}
	else {
		vaLength_->setValue( currentVoice_->getCurrentElement()->chord()->va_ );
	}

	++elcnt;
	tabWid_->setTabEnabled( vaLengthBase_, true );
    }
    else
        tabWid_->setTabEnabled( vaLengthBase_, false );

    if( elcnt ) {
	toolContainer_->move(width()-toolContainer_->width(), height()-TOOL_ELEMENT_HEIGHT);
	toolContainer_->show();
    }
    else
	toolContainer_->hide();

    if (elem && elem->playable() && (diag = elem->playable()->getChordChordDiagram()) != 0) {
		selectedElemForChord_ = elem;
		chordDialog_->setFingers(diag->getStrings());
		chordDialog_->show();
    }
    else {
	chordDialog_->hide();
    }


    
}

void NMainFrameWidget::setSlured(bool slured) {
	if (!slured) {
		currentVoice_->resetSlured();
		repaint();
		setEdited();
		return;
	}
	if (!NResource::windowWithSelectedRegion_) return;
	NResource::voiceWithSelectedRegion_->setSlured();
	repaint();
	setEdited();
}

// setTriplet -- handle the triplet button

void NMainFrameWidget::setTriplet(bool triplet) {
	main_props_.triplet = triplet;
	if (triplet) {
		if (!NResource::windowWithSelectedRegion_) return;
		NResource::voiceWithSelectedRegion_->setTuplet(3, 2);
	} else {
		currentVoice_->breakTuplet();
	}
	computeMidiTimes(false);
	reposit();
	repaint();
	setEdited();
}

void NMainFrameWidget::changeActualVoice(int voiceNr) {
	if (voiceNr < 0 || voiceNr > currentStaff_->voiceCount())
		return;
	currentVoice_ = currentStaff_->changeActualVoice(voiceNr-1);
	NResource::windowWithSelectedRegion_ = 0;
	repaint();
	enableCriticalButtons(currentVoice_->isFirstVoice());
}

void NMainFrameWidget::setCross(bool on) {
	if (playing_) return;
	actualOffs_ = on ? 1 : UNDEFINED_OFFS;
	if (editMode_) {
		currentVoice_->changeActualOffs(actualOffs_);
		computeMidiTimes(false);
		setEdited();
		reposit();
		repaint();
	}
}

void NMainFrameWidget::setFlat(bool on) {
	if (playing_) return;
	actualOffs_ = on ? -1 : UNDEFINED_OFFS;
	if (editMode_) {
		computeMidiTimes(false);
		currentVoice_->changeActualOffs(actualOffs_);
		setEdited();
		reposit();
		repaint();
	}
}

void NMainFrameWidget::setDCross(bool on) {
	if (playing_) return;
	actualOffs_ = on ? 2 : UNDEFINED_OFFS;
	if (editMode_) {
		currentVoice_->changeActualOffs(actualOffs_);
		computeMidiTimes(false);
		setEdited();
		reposit();
		repaint();
	}
}

void NMainFrameWidget::setDFlat(bool on) {
	if (playing_) return;
	actualOffs_ = on ? -2 : UNDEFINED_OFFS;
	if (editMode_) {
		computeMidiTimes(false);
		currentVoice_->changeActualOffs(actualOffs_);
		setEdited();
		reposit();
		repaint();
	}
}

void NMainFrameWidget::setNatur(bool on) {
	if (playing_) return;
	actualOffs_ = on ? 0 : UNDEFINED_OFFS;
	if (editMode_) {
		currentVoice_->changeActualOffs(actualOffs_);
		computeMidiTimes(false);
		setEdited();
		reposit();
		repaint();
	}
}

void NMainFrameWidget::setStemUp(bool on) {
	if (on && stemDownbutton_->isOn()) {
		stemDownbutton_->setOn(false);
	}
	if (playing_) return;
	if (!on) {
		main_props_.actualStemDir = STEM_DIR_AUTO;
		return;
	}
	main_props_.actualStemDir = STEM_DIR_UP;
	if (editMode_) {
		currentVoice_->changeActualStem();
		setEdited();
	}
	repaint();
}

void NMainFrameWidget::setStemDown(bool on) {
	if (on && stemUpbutton_->isOn()) {
		stemUpbutton_->setOn(false);
	}
	if (playing_) return;
	if (!on) {
		main_props_.actualStemDir = STEM_DIR_AUTO;
		return;
	}
	main_props_.actualStemDir = STEM_DIR_DOWN;
	if (editMode_) {
		currentVoice_->changeActualStem();
		setEdited();
	}
	repaint();
}


void NMainFrameWidget::setEditMode(bool on) {
	editMode_ = on;
	property_type properties;
	unsigned int val, i;
	bool playable;
	QCursor *cursor;
	if (on) {
		selectbutton_->setOn(false);
		props_before_edit_mode_ = 0;
		notePart_->setCursor( *NResource::cursor_edit_);
		
		/* save the selected note/rest length button */
		for (i = 0, length_before_edit_mode_ = -1; i < COUNT_CHORDBUTTONS; i++)
			if (note_buttons_[i]->isChecked()) {
				length_before_edit_mode_ = i;
				break;
			}

		/* save other properties */
		props_before_edit_mode_ |= (BODY_MASK & main_props_.noteBody);
		if (sforzatobutton_->isChecked()) {
			props_before_edit_mode_ |= PROP_SFORZ;
		}
		if (portatobutton_->isChecked()) {
			props_before_edit_mode_ |= PROP_PORTA;
		}
		if (strong_pizzicatobutton_->isChecked()) {
			props_before_edit_mode_ |= PROP_STPIZ;
		}
		if (sforzandobutton_->isChecked()) {
			props_before_edit_mode_ |= PROP_SFZND;
		}
		if (fermatebutton_->isChecked()) {
			props_before_edit_mode_ |= PROP_FERMT;
		}
		if (arpeggbutton_->isChecked()) {
			props_before_edit_mode_ |= PROP_ARPEGG;
		}
		if (staccatobutton_->isChecked()) {
			props_before_edit_mode_ |= PROP_STACC;
		}
		if (tiebutton_->isChecked()) {
			props_before_edit_mode_ |= PROP_TIED;
		}
		if (crossDrumBu_->isChecked()) {
			props_before_edit_mode_ |= PROP_BODY_CROSS;
		}
		if (cross2DrumBu->isChecked()) {
			props_before_edit_mode_ |= PROP_BODY_CROSS2;
		}
		if (crossCricDrumBu_->isChecked()) {
			props_before_edit_mode_ |= PROP_BODY_CIRCLE_CROSS;
		}
		if (rectDrumBu_->isChecked()) {
			props_before_edit_mode_ |= PROP_BODY_RECT;
		}
		if (triaDrumBu_->isChecked()) {
			props_before_edit_mode_ |= PROP_BODY_TRIA;
		}
		if (hiddenrestbutton_->isChecked()) {
			props_before_edit_mode_ |= PROP_HIDDEN;
		}
		val = currentVoice_->getElemState(&properties, &playable);
		if (playable) {
			updateInterface(properties, val);
		}
	}
	else {
		stemUpbutton_->setOn(false);
		stemDownbutton_->setOn(false);
		main_props_.actualStemDir = STEM_DIR_AUTO;
		
		/* turn on appropriate button, set actualLength and grace */
		setButton(length_before_edit_mode_);
		
		/* set appropriate cursor */
		if (length_before_edit_mode_ != -1)
			notePart_->setCursor(*NResource::determineCursor(main_props_.actualLength));
		else notePart_->setCursor(arrowCursor);

		actualOffs_ = UNDEFINED_OFFS;
		for (i = 0; i < COUNT_OFFSBUTTONS; ++i) {
			offs_buttons_[i]->setOn(false);
		}
		/*
		if (props_before_edit_mode_ & PROP_TIED) {
			tiebutton_->setOn(true);
			main_props_.tied = true;
		}
		else {
			tiebutton_->setOn(false);
			main_props_.tied = false;
		}
		*/
		tiebutton_->setOn(false);
		main_props_.tied = false;
		if (props_before_edit_mode_ & PROP_STACC) {
			staccatobutton_->setOn(true);
			main_props_.staccato = true;
		}
		else {
			staccatobutton_->setOn(false);
			main_props_.staccato = false;
		}
		if (props_before_edit_mode_ & PROP_SFZND) {
			sforzatobutton_->setOn(true);
			main_props_.sforzando = true;
		}
		else {
			sforzatobutton_->setOn(false);
			main_props_.sforzando = false;
		}
		if (props_before_edit_mode_ & PROP_PORTA) {
			portatobutton_->setOn(true);
			main_props_.portato = true;
		}
		else {
			portatobutton_->setOn(false);
			main_props_.portato = false;
		}
		if (props_before_edit_mode_ & PROP_STPIZ) {
			strong_pizzicatobutton_->setOn(true);
			main_props_.strong_pizzicato = true;
		}
		else {
			strong_pizzicatobutton_->setOn(false);
			main_props_.strong_pizzicato = false;
		}
		if (props_before_edit_mode_ & PROP_SFZND) {
			sforzandobutton_->setOn(true);
			main_props_.sforzando = true;
		}
		else {
			sforzandobutton_->setOn(false);
			main_props_.sforzando = false;
		}
		if (props_before_edit_mode_ & PROP_FERMT) {
			fermatebutton_->setOn(true);
			main_props_.fermate = true;
		}
		else {
			fermatebutton_->setOn(false);
			main_props_.fermate = false;
		}
		if (props_before_edit_mode_ & PROP_ARPEGG) {
			arpeggbutton_->setOn(true);
			main_props_.arpeggio = true;
		}
		else {
			arpeggbutton_->setOn(false);
			main_props_.arpeggio = false;
		}
		if (props_before_edit_mode_ & PROP_HIDDEN) {
			hiddenrestbutton_->setOn(true);
			main_props_.hidden = true;
		}
		else {
			hiddenrestbutton_->setOn(false);
			main_props_.hidden = false;
		}
		switch (main_props_.noteBody = (props_before_edit_mode_ & BODY_MASK)) {
			case PROP_BODY_CROSS: crossDrumBu_->setOn(true); break;
			case PROP_BODY_CROSS2: cross2DrumBu->setOn(true); break;
			case PROP_BODY_CIRCLE_CROSS: crossCricDrumBu_->setOn(true); break;
			case PROP_BODY_RECT: rectDrumBu_->setOn(true); break;
			case PROP_BODY_TRIA: triaDrumBu_->setOn(true); break;
			default: crossDrumBu_->setOn(false);
				 cross2DrumBu->setOn(false);
				 crossCricDrumBu_->setOn(false);
				 rectDrumBu_->setOn(false);
				 triaDrumBu_->setOn(false);
				 break;
		}
		beambutton_->setOn(false);
		slurbutton_->setOn(false);
		tripletbutton_->setOn(false);
		dotbutton_->setOn(false);
		ddotbutton_->setOn(false);
		main_props_.dotcount = 0;
	}
	repaint(); /* redraw border */
}

void NMainFrameWidget::allowKbInsert(bool on) {
	if (playing_) return;
	NResource::allowKeyboardInsert_ = on;
}

void NMainFrameWidget::setKbMode(bool on) {
	if (on) {
#ifdef WITH_TSE3
		NResource::mapper_->setEchoChannel(currentStaff_->getChannel(), currentStaff_->getVoice());
		connect(&midiInTimer_, SIGNAL(timeout()), this, SLOT(readNotesFromMidiMapper()));
		midiInTimer_.start(20, false);
#endif
	}
	else {
#ifdef WITH_TSE3
		disconnect(&midiInTimer_, SIGNAL(timeout()), this, SLOT(readNotesFromMidiMapper()));
		midiInTimer_.stop();
#endif
		if (kbInsertButton_->isOn()) {
			kbInsertButton_->setOn(false);
		}
	}
}

void NMainFrameWidget::setKbInsertMode(bool on) {
	if (on) {
		if (!kbbutton_->isOn()) {
#ifdef WITH_TSE3
			NResource::mapper_->setEchoChannel(currentStaff_->getChannel(), currentStaff_->getVoice());
			connect(&midiInTimer_, SIGNAL(timeout()), this, SLOT(readNotesFromMidiMapper()));
			midiInTimer_.start(20, false);
#endif
			kbbutton_->setOn(true);
		}
	}
}

void NMainFrameWidget::readNotesFromMidiMapper() {
#ifdef WITH_TSE3
	NChord *newchord;
	property_type properties;
	NMusElement *curElem;
	int line, offs, *pitch;
	int newXpos;
	QPtrList<int> *pitches;

	pitches = NResource::mapper_->readEvents();
	if (!pitches) return;

	if (main_props_.actualLength < 0) {
		delete pitches;
		return;
	}
	if (!kbInsertButton_->isOn()) {
		delete pitches;
		return;
	}
	pitch = pitches->first();
	if ((curElem = currentStaff_->getVoiceNr(0)->getCurrentPosition())) {
		currentStaff_->getVoiceNr(0)->validateKeysig(-1, curElem->getXpos());
	}
	else {
		currentStaff_->getVoiceNr(0)->validateKeysig(-1, 200);
	}
	currentStaff_->actualClef_.midi2Line(*pitch, &line, &offs, currentStaff_->actualKeysig_.getSubType());
	properties = 0;
	if (main_props_.tied) properties |= PROP_TIED;
	if (main_props_.staccato) properties |= PROP_STACC;
	if (main_props_.sforzato) properties |= PROP_SFORZ;
	if (main_props_.portato) properties |= PROP_PORTA;
	if (main_props_.strong_pizzicato) properties |= PROP_STPIZ;
	if (main_props_.sforzando) properties |= PROP_SFZND;
	if (main_props_.fermate) properties |= PROP_FERMT;
	if (main_props_.grace) properties |= PROP_GRACE;
	if (main_props_.arpeggio) properties |= PROP_ARPEGG;
	properties |= (main_props_.dotcount & DOT_MASK);
	properties |= (main_props_.noteBody & BODY_MASK);
	if (main_props_.pedal_on) properties |= PROP_PEDAL_ON;
	if (main_props_.pedal_off) properties |= PROP_PEDAL_OFF;
	newchord = new NChord(&main_props_, currentStaff_->getStaffPropsAddr(), currentVoice_, line, offs, main_props_.actualLength, 
				currentVoice_->stemPolicy_, properties );
	for (pitch = pitches->next(); pitch; pitch = pitches->next()) {
		currentStaff_->actualClef_.midi2Line(*pitch, &line, &offs, currentStaff_->actualKeysig_.getSubType());
		newchord->insertNewNote(line, offs, currentVoice_->stemPolicy_, properties);
	}
	delete pitches;
	if (!currentVoice_->insertAfterCurrent(newchord)) return;
	setEdited();
	computeMidiTimes(NResource::automaticBarInsertion_);
	reposit();
	newXpos = currentVoice_->getCurrentElement()->getXpos()+ currentVoice_->getCurrentElement()->getBbox()->width()+CUR_DIST;
	if (newXpos + SMALL_X_SENS_DIST > leftx_ + paperScrollWidth_) {
		scrollx_->setValue(leftx_ + SMALL_X_SCROLL);
		return;
	}
	repaint();
#endif
}

void NMainFrameWidget::restoreAllBehindDummyNoteAndAuxLines() {
#define DUMMY_NOTE_HEIGHT (2*LINE_DIST)/3
#define DUMMY_NOTE_WIDTH ((4*DUMMY_NOTE_HEIGHT)/ 3)
	int y, i;
	if (help_x0_ >= 0) {
		main_props_.directPainter->beginTranslated();
		main_props_.directPainter->setPen(NResource::helpLinePen_);
		main_props_.directPainter->setRasterOp(XorROP);
		for (i = 0, y = help_y_; i < num_help_lines_; ++i, y += LINE_DIST) {
			main_props_.directPainter->drawLine(help_x0_, y, help_x1_, y);
		}
		main_props_.directPainter->end();
		help_x0_ = -1;
	}
	if (dummy_note_y_ >= 0) {
		main_props_.directPainter->beginTranslated();
		main_props_.directPainter->setPen(NResource::dummyNotePen_);
		main_props_.directPainter->setRasterOp(XorROP);
		main_props_.directPainter->drawEllipse(dummy_note_x_, dummy_note_y_, DUMMY_NOTE_WIDTH, DUMMY_NOTE_HEIGHT);
		main_props_.directPainter->end();
		dummy_note_y_ = -1;
	}

}

void NMainFrameWidget::grabElementsAccording() {
	NStaff *staff_elem;
	for (staff_elem = staffList_.first(); staff_elem; staff_elem = staffList_.next()) {
		staff_elem->grabElementsAccording();
	}
}

QPtrList<NMusElement> *NMainFrameWidget::getClipBoard(int clipBoardNr) {
	voiceList_.at(clipBoardNr);
	return voiceList_.current()->getClipBoard();
}

void NMainFrameWidget::setDummyNoteAndAuxLines(QMouseEvent *evt) {
#define HELP_LINE_LENGTH 60
	int y, i;
	double lined;
	int line;

	restoreAllBehindDummyNoteAndAuxLines();
	if (!NResource::showAuxLines_) return;
	y = TRANSY(evt->y());
	main_props_.directPainter->beginTranslated();
	main_props_.directPainter->setRasterOp(XorROP);
	if (main_props_.actualLength > 0) {
		TRANSY2LINE(evt->y(), lined, line);
		main_props_.directPainter->setPen(NResource::dummyNotePen_);
		if (line <= MAXLINE && line >= MINLINE) {
			dummy_note_x_ = TRANSX(evt->x()) - DUMMY_NOTE_WIDTH / 2;
			dummy_note_y_ = currentStaff_->staff_props_.base - DUMMY_NOTE_HEIGHT / 2  + (8 - line) * LINE_DIST/2;
			main_props_.directPainter->drawEllipse(dummy_note_x_, dummy_note_y_, DUMMY_NOTE_WIDTH, DUMMY_NOTE_HEIGHT);
		}
	}
	main_props_.directPainter->setPen(NResource::helpLinePen_);
	if (currentStaff_->getBase() + 4 * LINE_DIST < y) {
		help_x1_ = TRANSX(evt->x()) + HELP_LINE_LENGTH / 2;
		help_x0_ = help_x1_ - HELP_LINE_LENGTH;
		help_y_ = currentStaff_->getBase() + 5 * LINE_DIST;
		num_help_lines_ = (y - (currentStaff_->getBase() + 4 * LINE_DIST)) / LINE_DIST;
		if (num_help_lines_ >= (LINE_OVERFLOW / 2)) num_help_lines_ = (LINE_OVERFLOW / 2);
		for (i = 0, y = help_y_; i < num_help_lines_; ++i, y += LINE_DIST) {
			main_props_.directPainter->drawLine(help_x0_, y, help_x1_, y);
		}
	}
	else if (currentStaff_->getBase() > y) {
		help_x1_ = TRANSX(evt->x()) + HELP_LINE_LENGTH / 2;
		help_x0_ = help_x1_ - HELP_LINE_LENGTH;
		num_help_lines_ = (currentStaff_->getBase() - y) / LINE_DIST;
		if (num_help_lines_ >= (LINE_OVERFLOW / 2)) num_help_lines_ = (LINE_OVERFLOW / 2);
		help_y_ = currentStaff_->getBase() - num_help_lines_ * LINE_DIST;
		for (i = 0, y = help_y_; i < num_help_lines_; ++i, y += LINE_DIST) {
			main_props_.directPainter->drawLine(help_x0_, y, help_x1_, y);
		}
	}
	main_props_.directPainter->end();
}

void NMainFrameWidget::updateChordnames() {
	chordDialog_->reconfigureMenues();
}

/*--------------------------- reaction on menu events -----------------------------------*/

bool NMainFrameWidget::newPaper() {
	if (playing_) return false;
	if (editiones_) {
		switch (KMessageBox::warningYesNoCancel
		         (this,
		          i18n("Your document contains unsaved changes.\n"
		               "Do you want to save your changes or discard them?"),
		          kapp->makeStdCaption(i18n("New")),
		          i18n("&Save"),
		          i18n("&Discard")
		         )
		       ) {
			case KMessageBox::Cancel: return false;
			case KMessageBox::No:     break;
			default:
				fileSave();
				break;
		}
	}
	voiceList_.setAutoDelete(false);
	voiceList_.clear();
	staffList_.setAutoDelete(true);
	staffList_.clear();
	staffList_.setAutoDelete(false);
	setEdited(false);
	scTitle_.truncate(0);
	scSubtitle_.truncate(0);
	scAuthor_.truncate(0);
	scLastAuthor_.truncate(0);
	scCopyright_.truncate(0);
	scComment_.truncate(0);
	currentStaff_ = staffList_.first();
	staffList_.append(currentStaff_ = new NStaff(Y_STAFF_BASE +  NResource::overlength_, 0, 0, this));
	voiceList_.append(currentVoice_ = currentStaff_->getVoiceNr(0));
	enableCriticalButtons(true);
	voiceDisplay_->setMax(currentStaff_->voiceCount());
	voiceDisplay_->setVal(0);
	staffCount_ = staffList_.count();
	currentStaff_->setChannel(0);
	currentStaff_->changeVoice(0);
	currentStaff_->setActual(true);
	currentStaff_->setBase( NResource::overlength_  + Y_STAFF_BASE);
	lastYHeight_ = voiceList_.last()->getStaff()->getBase()+voiceList_.last()->getStaff()->underlength_;
	actualFname_ = QString();
	parentWidget()->setCaption( !scTitle_.isEmpty() ? (!scSubtitle_.isEmpty() ? (scTitle_ + ": " + scSubtitle_) : scTitle_) : actualFname_ );
	emit caption("NoteEdit");
	tempo_ = DEFAULT_TEMPO;
	NVoice::resetUndo();
	NResource::windowWithSelectedRegion_ = 0;
	scrollx_->setValue(0); // includes repaint()
	currentStaff_->setVolume(80);
	reposit();
	setScrollableNotePage();
	setSaveWidth(170);
	setSaveHeight(250);
	setParamsEnabled(false);
	delete braceMatrix_;
	delete bracketMatrix_;
	delete barCont_;
	braceMatrix_ = new layoutDef[1];
	bracketMatrix_ = new layoutDef[1];
	barCont_ = new layoutDef[1];
	layoutPixmap_ = 0;
	renewStaffLayout();
	main_props_.left_page_border = DEFAULT_LEFT_PAGE_BORDER;
	main_props_.context_clef_xpos = DEFAULT_CONTEXT_CLEF_X_POS;
	main_props_.context_keysig_xpos = DEFAULT_CONTEXT_KEYSIG_X_POS;
	context_rect_left_right_ = DEFAULT_CONTEXT_REC_LEFT_RIGHT;
	repaint();
	return true;
}

const char* noteedit_file_pattern = "*.not|NoteEdit (*.not)\n*|All Files (*)";
const char* midi_file_pattern = "*.mid|MIDI (*.mid)\n*|All Files (*)";
const char* tse3_file_pattern = "*.tse3|TSE3 (*.tse3)\n*|All Files (*)";
const char* xml_file_pattern = "*.xml|MusicXML (*.xml)\n*|All Files (*)";

void NMainFrameWidget::fileOpen() {
	if (playing_) return;

	if (editiones_) {
		switch (KMessageBox::warningYesNoCancel
		         (this,
		          i18n("Your document contains unsaved changes.\n"
		               "Do you want to save your changes or discard them?"),
		          kapp->makeStdCaption(i18n("Open")),
		          i18n("&Save"),
		          i18n("&Discard")
		         )
		       ) {
			case KMessageBox::Cancel: return;
			case KMessageBox::No:     break;
		  default:
				fileSave();
				break;
		}
	}
	QString fileName = KFileDialog::getOpenFileName( QString::null, noteedit_file_pattern, this );
	if (!fileName.isNull() ) {
		loadFile( fileName );
		KURL url;
		url.setPath( fileName );
		m_recentFilesAction->addURL( url );
        	m_recentFilesAction->saveEntries( KGlobal::config() );
		synchronizeRecentFiles();
	}
}

bool NMainFrameWidget::loadFile( const QString & fileName )
{
	NVoice *voice_elem;
#ifdef WITH_TSE3
	kbbutton_->setOn(false);
#endif
	if (readStaffs(fileName)) {
		actualFname_ = fileName;
		parentWidget()->setCaption( !scTitle_.isEmpty() ? (!scSubtitle_.isEmpty() ? (scTitle_ + ": " + scSubtitle_) : scTitle_) : actualFname_ );
		tempo_ = DEFAULT_TEMPO;
		setScrollableNotePage();
		NResource::windowWithSelectedRegion_ = 0;
		reposit();
		arrangeStaffs(true);
		for (voice_elem = voiceList_.first(); voice_elem; voice_elem = voiceList_.next()) {
			voice_elem->correctReadTrillsSlursAndDynamicsStringsAndVAs();
		}
		scrollx_->setValue(0); // includes repaint()
		setEdited(false);
		stemUpbutton_->setOn(false);
		stemDownbutton_->setOn(false);
		main_props_.actualStemDir = STEM_DIR_AUTO;
		reposit();
		repaint();
		return true;
	}
	return false;
}

void NMainFrameWidget::importMusicXML() {
	if (playing_) return;

	if (editiones_) {
		switch (KMessageBox::warningYesNoCancel
		         (this,
		          i18n("Your document contains unsaved changes.\n"
		               "Do you want to save your changes or discard them?"),
		          kapp->makeStdCaption(i18n("Open")),
		          i18n("&Save"),
		          i18n("&Discard")
		         )
		       ) {
			case KMessageBox::Cancel: return;
			case KMessageBox::No:     break;
		  default:
				fileSave();
				break;
		}
	}
	QString fileName = KFileDialog::getOpenFileName( QString::null, xml_file_pattern, this );
	if (!fileName.isNull() ) {
		readStaffsFromXMLFile( fileName );
	}
}

void NMainFrameWidget::readStaffsFromXMLFile(const char *fname) {
	NVoice *voice_elem;
	if (playing_) return;
#ifdef WITH_TSE3
	kbbutton_->setOn(false);
#endif

	if (!musicxmlFileReader_->readStaffs(fname , &voiceList_, &staffList_, this)) {
		return;
		
	}
	setEdited(false);
	staffCount_ = staffList_.count();

	for (voice_elem = voiceList_.first(); voice_elem; voice_elem = voiceList_.next()) {
		voice_elem->paperDimensiones(paperScrollWidth_);
	}
	currentStaff_ = staffList_.first();
	currentStaff_->setActual(true);
	currentVoice_ = currentStaff_->getVoiceNr(0);
	enableCriticalButtons(true);
	voiceDisplay_->setMax(currentStaff_->voiceCount());
	voiceDisplay_->setVal(0);
	lastYHeight_ = voiceList_.last()->getStaff()->getBase()+voiceList_.last()->getStaff()->underlength_;
	computeMidiTimes(false);
	selectedSign_ = 0;
	NVoice::resetUndo();
	setSelectMode();
	main_props_.tp->setYPosition(-TOP_BOTTOM_BORDER);
	main_props_.directPainter->setYPosition(-TOP_BOTTOM_BORDER);
	main_props_.p->setYPosition(-TOP_BOTTOM_BORDER);
	cleanupSelections();
/*
	LVIFIX: following causes coredump on export:
	actualFname_.truncate(0);
	but following causes "specified directory does not exist" error,
	also followed by a core dump (after export is complete):
	actualFname_ = fname;
	finally, replacing extension .xml by .not works OK
*/
	actualFname_ = fname;
	if (actualFname_.right(4).lower() == ".xml") {
		actualFname_.truncate(actualFname_.length() - 4);
		actualFname_ += ".not";
	}
	parentWidget()->setCaption( !scTitle_.isEmpty() ? (!scSubtitle_.isEmpty() ? (scTitle_ + ": " + scSubtitle_) : scTitle_) : actualFname_ );
	tempo_ = DEFAULT_TEMPO;
	setScrollableNotePage();
	NResource::windowWithSelectedRegion_ = 0;
	reposit();
	arrangeStaffs(true);
	// Needed for trills and dynamics, as xpos is not known while parsing
	// not used for slurs, which use chord pointers instead of xpos
	for (voice_elem = voiceList_.first(); voice_elem; voice_elem = voiceList_.next()) {
		voice_elem->correctReadTrillsSlursAndDynamicsStringsAndVAs();
	}
	scrollx_->setValue(0); // includes repaint()
	setEdited(false);
	stemUpbutton_->setOn(false);
	stemDownbutton_->setOn(false);
	main_props_.actualStemDir = STEM_DIR_AUTO;
	reposit();
	repaint();
}

// Called when an item is selected from the "recent files" submenu
void NMainFrameWidget::fileOpenRecent( const KURL & u )
{
	if (!testEditiones()) return;
	ASSERT(u.isLocalFile());
	loadFile( u.path() );
	m_recentFilesAction->addURL( u );
        m_recentFilesAction->saveEntries( KGlobal::config() );
	synchronizeRecentFiles();
}

void NMainFrameWidget::showLyricsDialog() {
	if (playing_) return;
	currentVoice_->copyLyricsToEditor();
	lyricsDialog_->boot();
	currentVoice_->updateLyrics();
	setEdited();
	reposit();
	repaint();
}

void NMainFrameWidget::fileSave() {
	if (actualFname_.isNull())
		fileSaveAs();
	else
		writeStaffs(actualFname_);
}



void NMainFrameWidget::fileSaveAs() {
	QString fileName = this->checkFileName(KFileDialog::getSaveFileName( QString::null, noteedit_file_pattern, this ), (char *)".not");

	if (!fileName.isNull() ) {
		writeStaffs(fileName);
		actualFname_ = fileName;
		emit caption( !scTitle_.isEmpty() ? (!scSubtitle_.isEmpty() ? (scTitle_ + ": " + scSubtitle_) : scTitle_) : actualFname_ );

		KURL url;
		url.setPath( fileName );
		m_recentFilesAction->addURL( url );
        	m_recentFilesAction->saveEntries( KGlobal::config() );
		synchronizeRecentFiles();
	}
	repaint();
}


/*-------------------------------------- print -----------------------------------*/
/*------------------------------ Jorge Windmeisser Oliver ------------------------*/

#include <kprocess.h>
#include <kprinter.h>
#include <kdeprint/kprintdialogpage.h>                     
#if KDE_VERSION >= 300
// LVIFIX: kstandarddirs.h does not exist in KDE 2.2, assume KDE 3.0 specific
#include <kstandarddirs.h>
#endif

void NMainFrameWidget::filePrintPreview() {
#ifdef WITH_DIRECT_PRINTING
	filePrint(true);
#endif
}

void NMainFrameWidget::filePrintNoPreview() {
#ifdef WITH_DIRECT_PRINTING
	filePrint(false);
#endif
}         

void NMainFrameWidget::setupPrinting(bool preview, IntPrinter *printer)
{
#ifdef WITH_DIRECT_PRINTING
  // Print preview ?
  if ( preview == false )
  {
    // Setup printer (shows the print dialog)
    if( printer->setup(this) == false ) 
      KMessageBox::error(0,i18n("Couldn't setup printer"), 
                         kapp->makeStdCaption(i18n("???")));
  }
#endif
}

void NMainFrameWidget::filePrint(bool preview) {
#ifdef WITH_DIRECT_PRINTING

    bool bCustomPrinting = false;

    // No printing possible during playback
    if (playing_) return;
    
    // Find program used for printing (preview)
    QString ftsetprog=KStandardDirs::findExe( NResource::typesettingProgramInvokation_ );
    // Not found ? -> Tell the user and leave
    if (ftsetprog.isNull()) {
      KMessageBox::error (0, QString( NResource::typesettingProgramInvokation_ ) + " was not found in your PATH, aborting", "Noteeditor");
      return;
    }      

    // Try to create a temporary file in /tmp
    QString tmpFile=tempnam("/tmp","note_");
    if (tmpFile.isNull()) {
      KMessageBox::error (0,"Couldn't access the /tmp directory, aborting", "Noteeditor");
      return;
    }

    // Custom: Format decides which export filter to use
    if( NResource::typesettingProgram_ == 4 )
    {
        bCustomPrinting = true;
	// Find out which format was selected
	switch( NResource::typesettingProgramFormat_ )
	{
	  case 0: // Midi
	    NResource::typesettingProgram_ = 5; // This is now Midi
	    break;
	  case 1: // Lilypond
	    NResource::typesettingProgram_ = 2;
	    break;
	  case 2: // MusicXML
	    NResource::typesettingProgram_ = 6; // This is now MusicXML
	    break;
	  case 3: // ABC Music
	    NResource::typesettingProgram_ = 0;
	    break;
	  case 4: // NoteEdit
	    NResource::typesettingProgram_ = 7; // This is now NoteEdit
	    break;
	}
    }

    // Get the right export
    switch( NResource::typesettingProgram_ )
    {
      case 0: // ABC Music
	printWithABC(preview, ftsetprog, tmpFile);
        break;
      case 1: // PMX
	printWithPMX(preview, ftsetprog, tmpFile);
        break;
      case 2: // Lilypond
	printWithLilypond(preview, ftsetprog, tmpFile);
        break;
      case 3: // MusiXTeX
	printWithMusiXTeX(preview, ftsetprog, tmpFile);
        break;
      case 4: // Avoid warning
	break;
      case 5: // Midi
	printWithMidi(preview, ftsetprog, tmpFile);
        break;
      case 6: // MusicXML
	printWithMusicXML(preview, ftsetprog, tmpFile);
        break;
      case 7: default: // NoteEdit
	printWithNative(preview, ftsetprog, tmpFile);
        break;
    }
#endif /* WITH_DIRECT_PRINTING */
}

void NMainFrameWidget::printWithLilypond(bool preview, QString ftsetprg, QString tmpFile)
{
#ifdef WITH_DIRECT_PRINTING
    // Init process, export form and printer
    KProcess typesettingProgram;
    IntPrinter *printer=new IntPrinter(tmpFile);
    NLilyExport lily;
    struct lily_options lilyOpts;
    KPrintDialogPage *lilyExport=new PrintExportDialogPage(exportDialog_->FormatComboBox->text( LILY_PAGE ),this);
    LilypondExportForm *form = new LilypondExportForm( lilyExport );
    // Read options
    exportDialog_->getLilyOptions( lilyOpts );
    // Save options to new form
    exportDialog_->setLilyOptions( *form, lilyOpts );
    printer->addDialogPage(lilyExport);
    setupPrinting(preview, printer);
    delete form;
    delete lilyExport;
    delete printer;
#endif
}

// 
void NMainFrameWidget::filePrintPreviewFinished(KProcess *)
{
#ifdef WITH_DIRECT_PRINTING
    printf("Finished.\n");
    fflush(stdout);
    // Remove preview file on exit of browser
    unlink(previewFile_);
#endif
}

void NMainFrameWidget::filePrintReceivedStdOut(KProcess *, char *buffer, int buflen)
{
#ifdef WITH_DIRECT_PRINTING
  // Terminate manually
  buffer[buflen] = 0;
  printf("%s",buffer);
  fflush(stdout);
#endif
}

void NMainFrameWidget::filePrintReceivedStdErr(KProcess *, char *buffer, int buflen)
{
#ifdef WITH_DIRECT_PRINTING
  // Terminate manually
  buffer[buflen] = 0;
  printf("%s",buffer);
  fflush(stdout);
#endif
}

void NMainFrameWidget::printWithABC(bool preview, QString ftsetprg, QString tmpFile)
{
#ifdef WITH_DIRECT_PRINTING
    // Init process, export form and printer
    KProcess typesettingProgram;
    struct abc_options abcOpts;
    IntPrinter *printer=new IntPrinter(tmpFile);
    NABCExport abc;
    QStringList printOptions = QStringList::split( " ", QString(NResource::typesettingOptions_) );

    KPrintDialogPage *abcExport=new PrintExportDialogPage(exportDialog_->FormatComboBox->text( ABC_PAGE ),this);
    ABCExportForm *form = new ABCExportForm( abcExport );
    // Read options
    exportDialog_->getABCOptions( abcOpts );
    // Save options to new form
    exportDialog_->setABCOptions( *form, abcOpts );
    printer->addDialogPage(abcExport);
    setupPrinting(preview, printer);
    // Export file to abc
    abc.exportStaffs( tmpFile, &staffList_, voiceList_.count(), exportDialog_, this );
    
    // Replace the %s String by tmpFile
    printOptions.gres("%s",tmpFile);
    // Earlier options: << "-O=" << "-c"
    // Which file to use for printing ? Out.ps or tmpFile.ps (Option -O= !)
    // We probably need to filter all -O options!
    if( printOptions.find("-O=") == false && printOptions.find("-O =") == false)
      printOptions.prepend("-O= ");
    typesettingProgram << ftsetprg << printOptions;
    connect( &typesettingProgram, SIGNAL( processExited (KProcess *) ), 
             this, SLOT( filePrintPreviewFinished(KProcess *) ) );
    // Output of convert process should be visible on console in case something fails
    connect( &typesettingProgram, SIGNAL( receivedStdout(KProcess *, char *, int) ),
	     this, SLOT( filePrintReceivedStdOut(KProcess *, char *, int) ) );
    connect( &typesettingProgram, SIGNAL( receivedStderr(KProcess *, char *, int) ),
	     this, SLOT( filePrintReceivedStdErr(KProcess *, char *, int) ) );
    // Start converting exported file to a printable file
    typesettingProgram.start(KProcess::Block, KProcess::All);
    disconnect( &typesettingProgram, SIGNAL( processExited (KProcess *) ), 
                this, SLOT( filePrintPreviewFinished(KProcess *) ) );
    disconnect( &typesettingProgram, SIGNAL( receivedStdout(KProcess *, char *, int) ),
	        this, SLOT( filePrintReceivedStdOut(KProcess *, char *, int) ) );
    disconnect( &typesettingProgram, SIGNAL( receivedStderr(KProcess *, char *, int) ),
	        this, SLOT( filePrintReceivedStdErr(KProcess *, char *, int) ) );
    // Converting succesfull ?
    if (typesettingProgram.normalExit()) 
    {
      // Preview the printable file ?
      if( preview == true )
      {
	KProcess previewProgram;
	QString fprevprog=KStandardDirs::findExe( NResource::previewProgramInvokation_ );
	QStringList printpreviewOptions = QStringList::split( " ", QString(NResource::previewOptions_) );
	// Preview File: abcm2ps strangely does not add the path to the created ps file
        previewFile_ = QFileInfo( tmpFile ).fileName() + ".ps";
	if( false == QFileInfo( previewFile_ ).exists() )
          previewFile_ = tmpFile + ".ps";
	if( false == QFileInfo( previewFile_ ).exists() )
	{
	  KMessageBox::sorry(this, i18n("File was not succesfully be converted."), 
                             kapp->makeStdCaption(i18n("???")));
          delete form;
	  delete abcExport;
	  delete printer;
	  return;
	}
        // Replace the %s String by previewFile
        printpreviewOptions.gres("%s",previewFile_);
	previewProgram << fprevprog << printpreviewOptions;
	// Signal exit of program so we can clean up
	connect( &previewProgram, SIGNAL( processExited (KProcess *) ), 
                 this, SLOT( filePrintPreviewFinished(KProcess *) ) );
	// Start preview
	previewProgram.start(KProcess::DontCare, KProcess::All);
	disconnect( &previewProgram, SIGNAL( processExited (KProcess *) ), 
                    this, SLOT( filePrintPreviewFinished(KProcess *) ) );
      }
      else
      {
        QString printFile;
        printFile = QFileInfo( tmpFile ).fileName() + ".ps";
	if( false == QFileInfo( printFile ).exists() )
          printFile = tmpFile + ".ps";
	if( false == QFileInfo( printFile ).exists() )
	{
	  KMessageBox::sorry(this, i18n("File was not succesfully be converted."), 
                             kapp->makeStdCaption(i18n("???")));
          delete form;
	  delete abcExport;
	  delete printer;
	  return;
	}
        printer->doPreparePrinting();
	// Print file
        if (!printer->printFiles(printFile,true))
          unlink(tmpFile+".ps");
      }
      unlink(tmpFile);
    }
    delete form;
    delete abcExport;
    delete printer;
#endif
}
    
void NMainFrameWidget::printWithPMX(bool preview, QString ftsetprg, QString tmpFile)
{
#ifdef WITH_DIRECT_PRINTING
    // Init process, export form and printer
    KProcess typesettingProgram;    
    IntPrinter *printer=new IntPrinter(tmpFile);
    NPmxExport pmx;
    KPrintDialogPage *pmxExport=new PrintExportDialogPage(exportDialog_->FormatComboBox->text( PMX_PAGE ),this);
    PMXExportForm *form;
    printer->addDialogPage(pmxExport);
    setupPrinting(preview, printer);
#endif
}
    
void NMainFrameWidget::printWithMusiXTeX(bool preview, QString ftsetprg, QString tmpFile)
{
#ifdef WITH_DIRECT_PRINTING
    // Init process, export form and printer
    KProcess typesettingProgram;    
    exportFrm *formBack=exportDialog_;
    MusiXTeXExportForm *form;
    IntPrinter *printer=new IntPrinter(tmpFile);
    NMusiXTeX musixtex;
    KPrintDialogPage *musixtexExport=new PrintExportDialogPage(exportDialog_->FormatComboBox->text( MUSIX_PAGE ),this);
    printer->addDialogPage(musixtexExport);
    setupPrinting(preview, printer);
#endif
}
    
void NMainFrameWidget::printWithMusicXML(bool preview, QString ftsetprg, QString tmpFile)
{
#ifdef WITH_DIRECT_PRINTING
    // Init process, export form and printer
    KProcess typesettingProgram;    
    exportFrm *formBack=exportDialog_;
    MusicXMLExportForm *form;
    IntPrinter *printer=new IntPrinter(tmpFile);
    NMusicXMLExport musicxml;
    // Currently we could omit to show the export dialog page as i
    KPrintDialogPage *musicxmlExport=new PrintExportDialogPage(exportDialog_->FormatComboBox->text( MUSICXML_PAGE ),this);
    printer->addDialogPage(musicxmlExport);
    setupPrinting(preview, printer);
#endif
}
    
void NMainFrameWidget::printWithMidi(bool preview, QString ftsetprg, QString tmpFile)
{
#ifdef WITH_DIRECT_PRINTING
    // Init process, export form and printer
    KProcess typesettingProgram;    
    exportFrm *formBack=exportDialog_;
    MidiExportForm *form;
    IntPrinter *printer=new IntPrinter(tmpFile);
    NMidiExport midi;
    KPrintDialogPage *midiExport=new PrintExportDialogPage(exportDialog_->FormatComboBox->text( MIDI_PAGE ),this);
    printer->addDialogPage(midiExport);
    setupPrinting(preview, printer);
#endif
}
    
void NMainFrameWidget::printWithNative(bool preview, QString ftsetprg, QString tmpFile)
{
#ifdef WITH_DIRECT_PRINTING
#endif
}

void NMainFrameWidget::exportMidi() {	this->exportManager( MIDI_PAGE ); }
void NMainFrameWidget::exportMusiXTeX() { this->exportManager( MUSIX_PAGE ); }
void NMainFrameWidget::exportPMX() {	this->exportManager( PMX_PAGE ); }
void NMainFrameWidget::exportABC() {	this->exportManager( ABC_PAGE ); }
void NMainFrameWidget::exportLilyPond() {	this->exportManager( LILY_PAGE ); }
void NMainFrameWidget::setOutputParam() {	saveParametersDialog_->show(); }
void NMainFrameWidget::exportMusicXML() {	this->exportManager( MUSICXML_PAGE ); }

void NMainFrameWidget::importMidi() {
	if (playing_) return;
	
#ifdef WITH_TSE3
	if (!TSE3MidiIn()) return;
	if (!TSE3toScore()) return;
	KMessageBox::information(this, i18n("MIDI import is now complete. Please use Edit-->AutoBar, Edit-->AutoBeam and other tools for better score layout.\nYou can also read more about MIDI import and other useful tools in documentation!"), kapp->makeStdCaption(i18n("???")));
#else
	KMessageBox::sorry(this, i18n("MIDI import is performed by the TSE3 library, but the library is not present!\nPlease look in the MIDI import section in documentation for more information!"), kapp->makeStdCaption(i18n("???")));
#endif
}

void NMainFrameWidget::exportManager( int type ) {
    if (playing_) return;

    exportDialog_->FormatComboBox->setCurrentItem( type );
    exportDialog_->showExportForm( type );
    exportDialog_->initialize( &staffList_, &voiceList_, actualFname_);
    exportDialog_->boot();

}

void NMainFrameWidget::exportMusixTeXImm() {
	NResource::staffSelExport_ = 0;
	// RK: Several options have no default value!!
	musixtex_options musixOpts;
	musixOpts.width  = 170;
	musixOpts.height = 250;
	musixOpts.top    = -24;
	musixOpts.left   = -10;
	musixOpts.size   = 1;
	musixOpts.bar    = true;
	musixOpts.ties   = false;
	exportDialog_->setMusiXTeXOptions(musixOpts);
        NMusiXTeX mt;
	QRegExp notSuff(".not$");
	QString fname(actualFname_);
	fname.replace(notSuff, ".tex");
	mt.exportStaffs(fname, &staffList_, exportDialog_, this);
}

void NMainFrameWidget::exportLilyPondImm() {
	NResource::staffSelExport_ = 0;
	struct lily_options lilyOpts;
	// RK: Several options have no default value!!
	lilyOpts.customWidth = 170;
	lilyOpts.customHeight = 250;
	lilyOpts.voice = false;
	lilyOpts.beam = false;
	lilyOpts.ties = false;
	lilyOpts.stem = false;
	lilyOpts.drumNotes = false;
	lilyOpts.volume = 1;
	lilyOpts.measure = true;
	exportDialog_->setLilyOptions(lilyOpts);
	NLilyExport le;
	QRegExp notSuff(".not$");
	QString fname(actualFname_);
	fname.replace(notSuff, ".ly");
	le.exportStaffs(fname, &staffList_, exportDialog_, this);
}

void NMainFrameWidget::exportABCImm() {
	NResource::staffSelExport_ = 0;
	struct abc_options abcOpts;
	abcOpts.width        = 210;
	abcOpts.height       = 297;
	abcOpts.staffSep     = 16;
	abcOpts.exprAbove    = false;
	abcOpts.scale        = 75;
	abcOpts.measNumInBox = false;
	exportDialog_->setABCOptions(abcOpts);
	NABCExport abc;
	QRegExp notSuff(".not$");
	QString fname(actualFname_);
	fname.replace(notSuff, ".abc");
	abc.exportStaffs(fname, &staffList_, voiceList_.count(), exportDialog_, this);
}

void NMainFrameWidget::chordDialog() {
	if (tmpChordDiagram_) {
		delete tmpChordDiagram_;
		tmpChordDiagram_ = 0;
		selectedElemForChord_ = 0;
	}
	chordDialog_->show();
}

void NMainFrameWidget::createTuplet() {
	tupletDialog_->show();
}

void NMainFrameWidget::setTempChord(NChordDiagram *cdiagram) {
	if (!cdiagram) return;
	if (selectedElemForChord_ && selectedElemForChord_->playable()) {
		selectedElemForChord_->playable()->addChordDiagram(cdiagram);
		selectedElemForChord_ = 0;
		reposit();
		repaint();
		setEdited();
	}
	else {
		tmpChordDiagram_ = cdiagram;
		selectedSign_ = CDIAGRAM;
	}
}

void NMainFrameWidget::RemoveChord() {
	if (selectedElemForChord_ && selectedElemForChord_->playable()) {
		selectedElemForChord_->playable()->removeChordDiagram();
		selectedElemForChord_ = 0;
	}
}

void NMainFrameWidget::zoomIn() {
	if (playing_) return;
	zoomselect_->zoomIn();
}

void NMainFrameWidget::zoomOut() {
	if (playing_) return;
	zoomselect_->zoomOut();
}

void NMainFrameWidget::openNewWindow() {
	NMainWindow *newWindow = new NMainWindow;
	if ((NResource::lastWindowX_ += WINDOWXY_INCR) > MAXWINDOWXY) NResource::lastWindowX_ = 0;
	if ((NResource::lastWindowY_ += WINDOWXY_INCR) > MAXWINDOWXY) NResource::lastWindowY_ = 0;
	newWindow->setGeometry( NResource::lastWindowX_, NResource::lastWindowY_, START_WIDTH, START_HEIGHT);
	newWindow->show();
}

bool NMainFrameWidget::testEditiones() {
	if (editiones_) {
		switch (KMessageBox::warningYesNoCancel
		         (this,
		          i18n("Your document contains unsaved changes.\n"
		               "Do you want to save your changes or discard them?"),
		          kapp->makeStdCaption(i18n("Close")),
		          i18n("&Save"),
		          i18n("&Discard")
		         )
	         ) {
			case KMessageBox::Cancel: return false;
			case KMessageBox::No:     return true;
		  default:
				fileSave();
				break;
		}
	}
	return true;
}

void NMainFrameWidget::keyConfig() {
	KKeyDialog::configure( keys_, true, 0 );
}


void NMainFrameWidget::quitDialog() {
	if (playing_) return;
	if (!testEditiones()) return;
	if (NResource::windowList_.count() > 1) {
		NMainWindow *mainWindow = static_cast<NMainWindow *>(parentWidget());
		NResource::windowList_.removeRef(mainWindow);
		mainWindow->setCloseFromApplication();
		mainWindow->close(true);
	}
	else {
		NMainWindow *mainWindow = static_cast<NMainWindow *>(parentWidget());
		NResource::windowList_.removeRef(mainWindow);
		delete NResource::nresourceobj_;
		mainWindow->setCloseFromApplication();
		mainWindow->close(true);
		qApp->quit();
	}
}

void NMainFrameWidget::quitDialog2() {
	NMainWindow *mainWindow = static_cast<NMainWindow *>(parentWidget());
	if (playing_) return;
	if (!testEditiones()) return;
	
	NResource::writeToolbarSettings(mainWindow->toolBarIterator());
	NResource::defZoomval_ = zoomselect_->chooseZoomVal((int)(main_props_.zoom * 200.0f+0.5f));
	
	if (NResource::windowList_.count() > 1) {
		NResource::windowList_.removeRef(mainWindow);
		mainWindow->setCloseFromApplication();
	}
	else {
		NResource::windowList_.removeRef(mainWindow);
		delete NResource::nresourceobj_;
		mainWindow->setCloseFromApplication();
	}
}

void NMainFrameWidget::closeAllWindows() {
	NMainWindow *mainWindow = NResource::windowList_.first();
	NMainFrameWidget *frameWindow;
	if (playing_) return;
	if (NResource::windowList_.count() > 1) {
		if (KMessageBox::warningYesNo (this,
			i18n("Do you really want to close all windows?"),
			kapp->makeStdCaption(i18n("Close all")),
			i18n("&Close all")) != KMessageBox::Yes)
				return;
	}
        NResource::writeToolbarSettings(mainWindow->toolBarIterator());
	while (!NResource::windowList_.isEmpty()) {
		mainWindow = NResource::windowList_.first();
		frameWindow = (NMainFrameWidget *) mainWindow->centralWidget();
		if (!frameWindow->testEditiones()) return;
		NResource::windowList_.removeRef(mainWindow );
		mainWindow->setCloseFromApplication();
		mainWindow->close(true);
	}
	delete NResource::nresourceobj_;
	qApp->quit();
}

void NMainFrameWidget::toggleBarNumbers() {
	NResource::showStaffNrs_ = !NResource::showStaffNrs_;
	repaint();
}

void NMainFrameWidget::toggleStaffNames() {
	NResource::showStaffNames_ = !NResource::showStaffNames_;
}

void NMainFrameWidget::toggleAuxLines() {
	NResource::showAuxLines_ = ! NResource::showAuxLines_;
}

void NMainFrameWidget::toggleStaffContext() {
	NResource::showContext_ = !NResource::showContext_;
}

void NMainFrameWidget::gotoDialog() {
	if (playing_) return;
	scaleFrm_->desc->setText(i18n("<center>Please choose the target measure number!</center>"));
	scaleFrm_->chkbox->hide();
	scaleFrm_->scal_ed->setAll(0, lastBarNr_ - 1, 0);
	scaleFrm_->setCaption(kapp->makeStdCaption(i18n("Goto")));
	scaleFrm_->ok->setText(i18n("Goto"));
	scaleFrm_->boot( &staffList_, scrollx_ );
}

void NMainFrameWidget::muteDialog() {
	NStaff *staff_elem;
	int i;
	if (NResource::staffSelMute_) delete [] NResource::staffSelMute_;
	NResource::staffSelMute_ = new bool[staffList_.count()];
	for (i = 0, staff_elem = staffList_.first(); staff_elem; staff_elem = staffList_.next(), i++) {
		NResource::staffSelMute_[i] = staff_elem->getMuted();
	}
	multistaffDialog_->boot( &staffList_, STAFF_ID_MUTE );
	for( i = 0, staff_elem = staffList_.first(); staff_elem; staff_elem = staffList_.next(), i++ )
	    staff_elem->setMuted(NResource::staffSelMute_[i]);
}


void NMainFrameWidget::voiceDialog() {
	staffPropFrm_->boot( staffList_.find(currentStaff_), &staffList_ );
	// Show tab "Voice"
	staffPropFrm_->tabWidget_->setCurrentPage( 1 );
	createLayoutPixmap();
}

void NMainFrameWidget::setStaffProperties() {
	if (playing_) return;	
	staffPropFrm_->boot( staffList_.find(currentStaff_), &staffList_ );
	// Show tab "Staff"
	staffPropFrm_->tabWidget_->setCurrentPage( 0 );
	createLayoutPixmap();
}

void NMainFrameWidget::layoutDialog() {
	NStaffLayout *layout = new NStaffLayout(staffCount_, braceMatrix_, bracketMatrix_, barCont_, &staffList_, 0, (char *)"layout"); 
#if QT_VERSION >= 300
    	layout->exec();
#else
    	layout->show();
#endif
	if (layout->hasChanged()) setEdited();
	delete layout;
	createLayoutPixmap();
	repaint();
}

void NMainFrameWidget::autoBar() {
	if( NResource::staffSelAutobar_ ) delete [] NResource::staffSelAutobar_;
	NResource::staffSelAutobar_ = 0;
	multistaffDialog_->boot( &staffList_, STAFF_ID_AUTOBAR );
	if( !NResource::staffSelAutobar_ )
	    return;
	NStaff *staff;
	int i;
	for (i = 0, staff = staffList_.first(); staff; staff = staffList_.next(), i++)
	    if (NResource::staffSelAutobar_[i]) 
		staff->autoBar();
	computeMidiTimes(false);
	reposit();
	repaint();
	setEdited();
}

void NMainFrameWidget::doAutoBeam() {
	if (NResource::staffSelAutobeam_) delete [] NResource::staffSelAutobeam_;
	NResource::staffSelAutobeam_ = 0;
	multistaffDialog_->boot( &staffList_, STAFF_ID_AUTOBEAM );
	NStaff *staff;
	//bool includeRests;
	int i;

	if( !NResource::staffSelAutobeam_ )
	    return;
	//includeRests = scaleFrm_->chkbox->isChecked();
	for (i = 0, staff = staffList_.first(); staff; staff = staffList_.next(), i++) {
		if (NResource::staffSelAutobeam_[i]) staff->autoBeam(/* includeRests */);
	}
	reposit();
	repaint();
	NResource::progress_->hide();
	setEdited();
}

void NMainFrameWidget::generateClef(int type, int shift) {
	if (playing_) return;
	selectedSign_ = T_CLEF;
	tmpElem_ = new NClef(currentVoice_->getMainPropsAddr(), currentStaff_->getStaffPropsAddr(), type, shift);
}

void NMainFrameWidget::createTuplet(char numNotes, char playtime) {
	if (!NResource::windowWithSelectedRegion_) return;
	NResource::voiceWithSelectedRegion_->setTuplet(numNotes, playtime);
	computeMidiTimes(false);
	reposit();
	repaint();
	setEdited();
}

void NMainFrameWidget::performClefChange(int type, int shift) {
	if (playing_) return;
	currentStaff_->performClefChange(type, shift);
	reposit();
	repaint();
}


void NMainFrameWidget::clefDialog() {
	clefDialog_->boot( IS_CLEF );
}

void NMainFrameWidget::changeClefDialog() {
	clefDialog_->boot( IS_CLEF_DISTANCE );
	setEdited();
}

void NMainFrameWidget::redAccidentals() {
	if (playing_) return;
	currentStaff_->setHalfsAccordingKeySig();
	setEdited();
	reposit();
	repaint();

}
void NMainFrameWidget::collChords() {
	if (playing_) return;
	currentStaff_->collChords();
	setEdited();
	computeMidiTimes(false);
	reposit();
	repaint();
}

void NMainFrameWidget::setAllSharp() {
	if (playing_) return;
	currentStaff_->setHalfsTo(PROP_CROSS);
	setEdited();
	reposit();
	repaint();
}

void NMainFrameWidget::setAllFlat() {
	if (playing_) return;
	currentStaff_->setHalfsTo(PROP_FLAT);
	setEdited();
	reposit();
	repaint();
}

void NMainFrameWidget::insertRepeatOpen() {
	if (playing_) return;
	selectedSign_ = REPEAT_OPEN;
}

void NMainFrameWidget::insertRepeatOpenClose() {
	if (playing_) return;
	selectedSign_ = REPEAT_OPEN_CLOSE;
}


void NMainFrameWidget::insertRepeatClose() {
	if (playing_) return;
	selectedSign_ = REPEAT_CLOSE;
}

void NMainFrameWidget::insertspecEnding1() {
	if (playing_) return;
	selectedSign_ = SPECIAL_ENDING1;
}

void NMainFrameWidget::insertspecEnding2() {
	if (playing_) return;
	selectedSign_ = SPECIAL_ENDING2;
}

void NMainFrameWidget::insertDoubleBar() {
	if (playing_) return;
	selectedSign_ = DOUBLE_BAR;
}

void NMainFrameWidget::insertEndBar() {
	if (playing_) return;
	selectedSign_ = END_BAR;
}

void NMainFrameWidget::insertText() {
	if (playing_) return;
	QString text;
	NTextDialogImpl textDialog;
	textDialog.exec();
	text = textDialog.getText();
	if (text.length() == 0) return;
	selectedSign_ = T_TEXT;
	tmpElem_ = new NText(currentVoice_->getMainPropsAddr(), currentStaff_->getStaffPropsAddr(), text, textDialog.isUpText() ? TEXT_UPTEXT : TEXT_DOWNTEXT);
}

void NMainFrameWidget::insertSegno() {
	if (playing_) return;
	selectedSign_ = SEGNO;
	tmpElem_ = new NSign(currentVoice_->getMainPropsAddr(), currentStaff_->getStaffPropsAddr(), SEGNO);
}

void NMainFrameWidget::insertDalSegno() {
	if (playing_) return;
	selectedSign_ = DAL_SEGNO;
	tmpElem_ = new NSign(currentVoice_->getMainPropsAddr(), currentStaff_->getStaffPropsAddr(), DAL_SEGNO);
}

void NMainFrameWidget::insertRitardando() {
	if (playing_) return;
	selectedSign_ = RITARDANDO;
	tmpElem_ = new NSign(currentVoice_->getMainPropsAddr(), currentStaff_->getStaffPropsAddr(), RITARDANDO);
}
void NMainFrameWidget::insertAccelerando() {
	if (playing_) return;
	selectedSign_ = ACCELERANDO;
	tmpElem_ = new NSign(currentVoice_->getMainPropsAddr(), currentStaff_->getStaffPropsAddr(), ACCELERANDO);
}

void NMainFrameWidget::insertDalSegnoAlFine() {
	if (playing_) return;
	selectedSign_ = DAL_SEGNO_AL_FINE;
	tmpElem_ = new NSign(currentVoice_->getMainPropsAddr(), currentStaff_->getStaffPropsAddr(), DAL_SEGNO_AL_FINE);
}

void NMainFrameWidget::insertDalSegnoAlCoda() {
	if (playing_) return;
	selectedSign_ = DAL_SEGNO_AL_CODA;
	tmpElem_ = new NSign(currentVoice_->getMainPropsAddr(), currentStaff_->getStaffPropsAddr(), DAL_SEGNO_AL_CODA);
}

void NMainFrameWidget::insertFine() {
	if (playing_) return;
	selectedSign_ = FINE;
	tmpElem_ = new NSign(currentVoice_->getMainPropsAddr(), currentStaff_->getStaffPropsAddr(), FINE);
}

void NMainFrameWidget::insertCoda() {
	if (playing_) return;
	selectedSign_ = CODA;
	tmpElem_ = new NSign(currentVoice_->getMainPropsAddr(), currentStaff_->getStaffPropsAddr(), CODA);
}


void NMainFrameWidget::keyDialog() {
	int i, h;
	keyOkButton_->setDown(false);
	keyCancButton_->setDown(false);
	keyDialog_->setCaption(kapp->makeStdCaption(i18n("Key")));
	keyDialog_->setGeometry(40, 40, LISTWIDTH + LIST_DIST + 7*KEYOFFSWIDTH + RIGHT_BORDER, 24 * (10+2) + PUSHBUTTONHEIGHT);
	keyDialog_->setMinimumSize(LISTWIDTH + LIST_DIST + 7*KEYOFFSWIDTH + RIGHT_BORDER, 24 * (10+2) + PUSHBUTTONHEIGHT);
	keyDialog_->setMaximumSize(LISTWIDTH + LIST_DIST + 7*KEYOFFSWIDTH + RIGHT_BORDER, 24 * (10+2) + PUSHBUTTONHEIGHT);
	keyList_->setGeometry(10, 10, LISTWIDTH-50, 24 * 10);
	if (!tmpKeysig_) tmpKeysig_ = new NKeySig(currentVoice_->getMainPropsAddr(), currentStaff_->getStaffPropsAddr());
	if (keyList_->currentItem()==-1) keyList_->setCurrentItem(0);
	changeKey(keyList_->currentItem());
	for (i = 0; i < 7; ++i) {
		offs_list_[i]->setKeysigObj(tmpKeysig_);
		offs_list_[i]->setGeometry(LISTWIDTH + LIST_DIST+i*KEYOFFSWIDTH, 10, KEYOFFSWIDTH, keyList_->height()/2);
	}
	h = (keyList_->height()/2 - KEY_OFFS_UP_DIST-KEY_OFFS_BOTTOM_DIST) / 3;
	crosslabel_->setGeometry(LISTWIDTH + KEY_OFFS_LABEL_DIST, 10 + KEY_OFFS_UP_DIST, KEY_OFFS_LABEL_WIDTH, KEY_OFFS_LABEL_HEIGHT);
	flatlabel_->setGeometry(LISTWIDTH + KEY_OFFS_LABEL_DIST, 10 + KEY_OFFS_UP_DIST + h, KEY_OFFS_LABEL_WIDTH, KEY_OFFS_LABEL_HEIGHT);
	naturlabel_->setGeometry(LISTWIDTH + KEY_OFFS_LABEL_DIST, 10 + KEY_OFFS_UP_DIST + 2*h, KEY_OFFS_LABEL_WIDTH, KEY_OFFS_LABEL_HEIGHT);
	keyOkButton_->setGeometry(40, keyList_->height() + 24, 2*PUSHBUTTONWIDTH, PUSHBUTTONHEIGHT);
	keyCancButton_->setGeometry(40 + 3*PUSHBUTTONWIDTH, keyList_->height() + 24, 2*PUSHBUTTONWIDTH, PUSHBUTTONHEIGHT);
	keyDialog_->show();
}


void NMainFrameWidget::timesigDialog() {
	 timesigDialog_->showDialog();
}

void NMainFrameWidget::setTempTimesig(int num, int dom) {
	tmpTimeSig_ = new NTimeSig(currentVoice_->getMainPropsAddr(), currentStaff_->getStaffPropsAddr());
	tmpTimeSig_->setSignature(num, dom);
	tmpElem_ = tmpTimeSig_;
	selectedSign_ = T_TIMESIG;
}

bool NMainFrameWidget::paramsEnabled() {return saveParametersDialog_->paramsEnabled();}
int NMainFrameWidget::getSaveWidth() {return saveParametersDialog_->getSaveWidth();}
bool NMainFrameWidget::withMeasureNums() {return saveParametersDialog_->withMeasureNums();}
int NMainFrameWidget::getSaveHeight() {return saveParametersDialog_->getSaveHeight();}
void NMainFrameWidget::setParamsEnabled(bool ok) {saveParametersDialog_->setEnabled(ok);}
void NMainFrameWidget::setSaveWidth(int width)  {saveParametersDialog_->setSaveWidth(width);}
void NMainFrameWidget::setSaveHeight(int height) {saveParametersDialog_->setSaveHeight(height);}
void NMainFrameWidget::setWithMeasureNums(bool with) {saveParametersDialog_->setWithMeasureNums(with);}

void NMainFrameWidget::newStaff() {
	int staffYpos = 0;
	if (playing_) return;
	currentStaff_->setActual(false);
	staffList_.append(currentStaff_  = new NStaff(staffYpos+NResource::underlength_, staffCount_ % 16, 0, this));
	voiceList_.append(currentVoice_ = currentStaff_->getVoiceNr(0));
	enableCriticalButtons(true);
	voiceDisplay_->setMax(currentStaff_->voiceCount());
	voiceDisplay_->setVal(1);
	currentStaff_->setActual(true);
	arrangeStaffs(false);
	++staffCount_;
	appendStaffLayoutElem();
	reposit();
	setScrollableNotePage();
	repaint();
	setEdited();
	cleanupSelections();
}

void NMainFrameWidget::deleteStaff() {
	if (playing_) return;
	if (staffCount_ == 1) {
		KMessageBox::sorry
		  (this,
		   i18n("Can't delete last staff."),
		   kapp->makeStdCaption(i18n("Delete Staff"))
		  );
		return;
	}
	if (KMessageBox::warningYesNo
	     (0,
	      i18n("This deletes current staff! Are you sure?"),
	      kapp->makeStdCaption(i18n("Delete Staff")),
	      i18n("&Delete")
	     )
	    != KMessageBox::Yes
	   ) return;
	if (staffList_.find(currentStaff_) < 0) {
		NResource::abort("NMainFrameWidget::deleteStaff: internal error", 1);
	}
	staffList_.remove();
	currentStaff_->updateVoiceList(&voiceList_);
	delete currentStaff_;
	staffCount_--;
	if ((currentStaff_ = staffList_.current()) == 0) {
		NResource::abort("NMainFrameWidget::deleteStaff: internal error", 2);
	}
	if ((currentVoice_ = currentStaff_->getVoiceNr(0)) == 0) {
		NResource::abort("NMainFrameWidget::deleteStaff: internal error", 3);
	}
	enableCriticalButtons(currentVoice_->isFirstVoice());
	arrangeStaffs(false); 
	renewStaffLayout(); /*  includes createLayoutPixmap() */
	voiceDisplay_->setMax(currentStaff_->voiceCount());
	voiceDisplay_->setVal(1);
	currentStaff_->setActual(true);
	reposit();
	setScrollableNotePage();
	repaint();
	setEdited();
	cleanupSelections();
}

void NMainFrameWidget::staffMoveDialog() {
	listDialog_->boot
	  (0, LIST_MOVE_STAFF, kapp->makeStdCaption(i18n("Staff moving")),
	   i18n("Please choose the target position:"), &staffList_
	  );
	if( !listDialog_->succ_ )
	    return;

	int cur_idx;
	lastYHeight_ = 0;
	char *err = (char *)"moveStaff: internal error";
	if (staffList_.find(currentStaff_) == -1) {
		NResource::abort(err, 1);
	}
	cur_idx = staffList_.at();
	if (listDialog_->choice->currentItem() == cur_idx) return;
	staffList_.remove();
	if (listDialog_->choice->currentItem() == staffCount_ - 1) {
		staffList_.append(currentStaff_);
	}
	else {
		staffList_.insert(listDialog_->choice->currentItem(), currentStaff_);
	}
	arrangeStaffs(false);
	renewStaffLayout();
	setEdited();
	reposit();
	repaint();
}

void NMainFrameWidget::cancelMultiStaff() {
	if (NResource::staffSelMulti_) delete [] NResource::staffSelMulti_;
	NResource::staffSelMulti_ = 0;
	NResource::numOfMultiStaffs_ = 0;
}

void NMainFrameWidget::multiStaffDialog() {
	if (NResource::windowWithSelectedRegion_ == 0) {	//returns immediately if selection iz none
		KMessageBox::sorry(this, i18n("Please select a region first!"), kapp->makeStdCaption(i18n("MultiStaff")));
		return;
	}
	
	if (NResource::staffSelMulti_) delete [] NResource::staffSelMulti_;
	NResource::staffSelMulti_ = 0;
	NResource::numOfMultiStaffs_ = staffList_.count();
	multistaffDialog_->boot( &staffList_, STAFF_ID_MULTI );
}

void NMainFrameWidget::tempoSigDialog() {
	scaleFrm_->desc->setText(i18n("<center>Set tempo:</center>"));
	scaleFrm_->scal_ed->setAll(40, 300, 100);
	scaleFrm_->chkbox->hide();
	scaleFrm_->setCaption(kapp->makeStdCaption(i18n("Set new tempo")));
	scaleFrm_->ok->setText(i18n("Set new temp&o"));
	if( scaleFrm_->boot( &main_props_, currentStaff_, currentVoice_, &tmpElem_, TEMPO_SIGNATURE ) )
	    selectedSign_ = TEMPO_SIGNATURE;
}

void NMainFrameWidget::multiRestDialog() {
	scaleFrm_->desc->setText(i18n("<center>Set count of Measures:</center>"));
	scaleFrm_->scal_ed->setAll(1, 200, 2);
	scaleFrm_->chkbox->hide();
	scaleFrm_->setCaption(kapp->makeStdCaption(i18n("multi rest")));
	scaleFrm_->ok->setText(i18n("Set count of m&easures"));
	if( scaleFrm_->boot( &main_props_, currentStaff_, currentVoice_, &tmpElem_, MULTIREST ) )
	    selectedSign_ = MULTIREST;
}

void NMainFrameWidget::cleanRestsDialog() {
	if( cleanUpRestsDialog_->boot() )
	    this->cleanupRests();
}

void NMainFrameWidget::volChangeDialog() {
	if( volChangeDialog_->boot() )
	    insVolChange();
}

void NMainFrameWidget::autoBeamDialog() {
	scaleFrm_->chkbox->show();
	scaleFrm_->desc->setText(i18n("<center>How many notes:</center>"));
	scaleFrm_->scal_ed->setMinVal( 2 );
	scaleFrm_->scal_ed->setMaxVal( 16 );
	scaleFrm_->scal_ed->setStartVal( 4 );
	scaleFrm_->setCaption(kapp->makeStdCaption(i18n("Autobeam")));
	scaleFrm_->ok->setText(i18n("&Start"));
	if ( ! scaleFrm_->boot() )
	    return;

	doAutoBeam();

}

void NMainFrameWidget::repeatCountDialog() {
	NMusElement *elem;
	if ((elem = currentVoice_->getCurrentElement()) == 0  || elem->getType() != T_SIGN || elem->getSubType() != REPEAT_CLOSE) {
		KMessageBox::sorry(this, i18n("Please choose a repeat close sign!"), kapp->makeStdCaption(i18n("???")));
		return;
	}
	scaleFrm_->chkbox->hide();
	scaleFrm_->desc->setText(i18n("<center>Please set the repeat count!</center>"));
	scaleFrm_->scal_ed->setMinVal( 2 );
	scaleFrm_->scal_ed->setMaxVal( 16 );
	scaleFrm_->scal_ed->setStartVal( 2 );
	scaleFrm_->setCaption(kapp->makeStdCaption(i18n("Repeat count")));
	scaleFrm_->ok->setText(i18n("&Set"));
	if ( ! scaleFrm_->boot() )
	    return;

	((NSign *) elem)->setRepeatCount(scaleFrm_->scal_ed->getValue());
	setEdited();
	reposit();
	repaint();

}

void NMainFrameWidget::transposeDialog() {
	NStaff *staff_elem;
	int i, semitones;

	scaleFrm_->chkbox->hide();
	scaleFrm_->desc->setText(i18n("<center>Semitones:</center>"));
	scaleFrm_->scal_ed->setMinVal( -12 );
	scaleFrm_->scal_ed->setMaxVal( 12 );
	scaleFrm_->scal_ed->setStartVal( 0 );
	scaleFrm_->setCaption(kapp->makeStdCaption(i18n("Transpose")));
	scaleFrm_->ok->setText(i18n("&Transpose"));
	if (NResource::numOfMultiStaffs_) {
		if (NResource::windowWithSelectedRegion_ == 0 || NResource::windowWithSelectedRegion_ == this) {
			semitones = scaleFrm_->boot();
			for (i = 0, staff_elem = staffList_.first(); staff_elem && i < NResource::numOfMultiStaffs_;
				 staff_elem = staffList_.next() , i++) {
				if (NResource::staffSelMulti_[i]) {
					staff_elem->transpose (semitones);
				}
			}
			setEdited();
		}
	}
	else {
		currentStaff_->transpose( scaleFrm_->boot() );
		setEdited();
	}
	reposit();
	repaint();
}


void NMainFrameWidget::voiceChangeDialog() {
	if (listDialog_->boot
	     (currentStaff_->getVoice(),
	      LIST_VOICE,
	      kapp->makeStdCaption(i18n("Voice selection")),
	      i18n("<center>Select your voice here:</center>")
	     )
	   ) this->changeVoice();
}

/*--------------------------- reaction on Ok button of the dialogs above ----------------*/


void NMainFrameWidget::cleanupRests() {
	int smallestRest;

	smallestRest = cleanUpRestsDialog_->item2length();
	currentStaff_->cleanupRests(smallestRest);
	computeMidiTimes(false);
	reposit();
	repaint();
	setEdited();
}

void NMainFrameWidget::insVolChange() {
	NSign *sign;
	sign = new NSign(&main_props_, currentStaff_->getStaffPropsAddr(), VOLUME_SIG);
	sign->setVolume( volChangeDialog_->sel->currentItem(), volChangeDialog_->scal_ed->getValue() );
	tmpElem_ = sign;
	selectedSign_ = VOLUME_SIG;
}

void NMainFrameWidget::changeVoice(int voice) {
	NSign *sign;
	sign = new NSign(&main_props_, currentStaff_->getStaffPropsAddr(), PROGRAM_CHANGE);
	if( voice < 0 )
	    voice = listDialog_->choice->currentItem();
	sign->setProgram(voice);
	tmpElem_ = sign;
	selectedSign_ = PROGRAM_CHANGE;
}

void NMainFrameWidget::arrangeStaffs(bool create_layout_pixmap) {
	NStaff *staff_elem;
	lastYHeight_ = Y_STAFF_BASE;
	if (create_layout_pixmap) createLayoutPixmap(); /* calculate layout pixmaps */
	for (staff_elem = staffList_.first(); staff_elem; staff_elem = staffList_.next()) {
		staff_elem->setBase(lastYHeight_+staff_elem->overlength_);
		lastYHeight_ += staff_elem->underlength_ + staff_elem->overlength_ + STAFF_HIGHT;
	}
	if (create_layout_pixmap) createLayoutPixmap(); /* redraw pixmaps */
	reposit();
	repaint();
	setEdited();
}


void NMainFrameWidget::changeZoomValue(int zval) {
	main_props_.zoom = zoomselect_->computeZoomVal(zval);
	main_props_.tp->setZoom(main_props_.zoom);
	main_props_.p->setZoom(main_props_.zoom);
	main_props_.directPainter->setZoom(main_props_.zoom);
	main_props_.scaledText_ = QFont ("Times" , (int) (main_props_.zoom * 36.0), QFont::Normal);
	main_props_.scaledItalic_ = QFont ("Times" , (int) (main_props_.zoom * 36.0), QFont::Normal, true);
	main_props_.scaledMiniItalic_ = QFont ("Times" , (int) (main_props_.zoom * 24.0), QFont::Normal, true);
	main_props_.scaledBold_ = QFont ("Times" , (int) (main_props_.zoom * 48.0), QFont::Bold);
	main_props_.scaledBold2_ = QFont ("Times" , (int) (main_props_.zoom * 36.0), QFont::Bold);
	main_props_.scaledBoldItalic_ = QFont ("Times" , (int) (main_props_.zoom * 40.0), QFont::Bold, true);
	main_props_.scaledBoldItalicMetrics_ = QFontMetrics(main_props_.scaledBoldItalic_);
	setScrollableNotePage();
	xscrollFromWidget(leftx_);
}

void NMainFrameWidget::setInsertionKey() {
	int i;

	if (tmpKeysig_ == 0) return;
	for (i = 0; i < 7; ++i) {
		offs_list_[i]->setKeysigObj(0);
	}
	tmpElem_ = tmpKeysig_;
	tmpKeysig_ = 0; /* cleans up tmpKeysig so a new one gets created the next time Keydialog() is called */
	selectedSign_ = T_KEYSIG;
	keyDialog_->hide();
}

void NMainFrameWidget::changeKey(int idx) {
	int i;
	int count;
	property_type kind;

	if (idx > 7) {
		count = idx - 7;
		kind = PROP_CROSS;
	}
	else {
		count = idx;
		kind = PROP_FLAT;
	}
	tmpKeysig_->setRegular(count, kind);
	for (i = 0; i < 7; ++i) {
		offs_list_[i]->setKeysigObj(0); // avoid feedback
		offs_list_[i]->set(PROP_NATUR);
	}
	for (i = 0; i < 7; ++i) {
		offs_list_[i]->set(tmpKeysig_->getAccent(i));
	}
	for (i = 0; i < 7; ++i) {
		offs_list_[i]->setKeysigObj(tmpKeysig_); // avoid feedback
	}
}

/*-------------------------- reaction on scroll events ---------------------------------*/

void NMainFrameWidget::xscrollFromWidget(int val) {
	if (playing_) return;
        leftx_ = val;
	main_props_.tp->setXPosition(val-main_props_.left_page_border);
	main_props_.directPainter->setXPosition(val-main_props_.left_page_border);
	repaint();
}

void NMainFrameWidget::xscrollDuringReplay(int val) {
	NStaff *staff_elem;
	bool scrollException = false;
	bool isConnected;
	int i, j;
	if (val < leftx_ || val > leftx_ + paperScrollWidth_ + 100) { // repeat or al Coda
		scrollException = true;
	}
        leftx_ = val - turnOverOffset_;
	if (scrollException) {
        	leftx_ = val;
		main_props_.tp->setXPosition(leftx_ - main_props_.left_page_border - contextWidth_);
		main_props_.p->begin( notePart_->acWritePixmap() );
		main_props_.p->setBrush(NResource::backgroundBrush_);
		main_props_.p->setPen(NResource::noPen_);
		main_props_.p->setPen(editMode_ ? NResource::editModeBorderPen_ : NResource::blackPen_); //  color of border around drawing area
		main_props_.p->drawRect(0, 0, paperWidth_, paperHeight_);
		if (NResource::showContext_) {
			main_props_.p->fillRect(contextRec_, NResource::contextBrush_);
		}
		main_props_.p->end();
		nextStaffIsFirstStaff_ = true;
		for (i = 0, staff_elem = staffList_.first(); staff_elem; i++, staff_elem = staffList_.next()) {
			if (staff_elem->getBase() < topy_) continue;
			if (staff_elem->getBase() > boty_) break;
			PREPARE_BAR_CHECK_ARRAY(staff_elem, i, isConnected, nextStaffIsFirstStaff_, j)
			staff_elem->draw(leftx_, leftx_ + paperScrollWidth_ - contextWidth_);
		}
		nextStaffElemToBePainted_ = 0;
	}
	while (nextStaffElemToBePainted_) {
		if (nextStaffElemToBePainted_->getBase() > boty_) {
			nextStaffElemToBePainted_ = 0;
			break;
		}
		if (nextStaffElemToBePainted_->getBase() < topy_) {
			nextStaffNr_++;
			nextStaffElemToBePainted_ = staffList_.at(nextStaffNr_);
			continue;
		}
		if (nextStaffElemToBePainted_) {
			PREPARE_BAR_CHECK_ARRAY(nextStaffElemToBePainted_, nextStaffNr_, isConnected, nextStaffIsFirstStaff_, j)
			nextStaffElemToBePainted_->draw(newLeft_, newRight_);
			nextStaffNr_++;
			nextStaffElemToBePainted_ = staffList_.at(nextStaffNr_);
		}
	}
	newLeft_ = leftx_ + paperScrollWidth_ - turnOverOffset_;
	newRight_ = newLeft_+ paperScrollWidth_ - contextWidth_;
	if (NResource::showContext_) {
		for (staff_elem = staffList_.first(); staff_elem; staff_elem = staffList_.next()) {
			if (staff_elem->getBase() < topy_) continue;
			if (staff_elem->getBase() > boty_) break;
			staff_elem->drawContext();
		}
	}	
	if (layoutPixmap_) {
		main_props_.p->beginUnclippedYtranslated();
		main_props_.p->drawPixmap(LAYOUT_PIXMAP_X_DIST, 0, *layoutPixmap_);
		main_props_.p->end();
	}
	notePart_->swap();
	main_props_.tp->setXPosition(newLeft_ - main_props_.left_page_border - contextWidth_);
	main_props_.tp->setPaintDevice(notePart_->acWritePixmap());
	main_props_.p->setPaintDevice(notePart_->acWritePixmap());
	main_props_.p->begin( notePart_->acWritePixmap() );
	main_props_.p->setBrush(NResource::backgroundBrush_);
	main_props_.p->setPen(NResource::noPen_);
	main_props_.p->setPen(editMode_ ? NResource::editModeBorderPen_ : NResource::blackPen_); //  color of border around drawing area
	main_props_.p->drawRect(0, 0, paperWidth_, paperHeight_);
	if (NResource::showContext_) {
		main_props_.p->fillRect(contextRec_, NResource::contextBrush_);
	}
	main_props_.p->end();
	nextStaffElemToBePainted_ = staffList_.first();
	nextStaffNr_ = 0;
	nextStaffIsFirstStaff_ = true;
	if (scrollException) {
		main_props_.directPainter->setXPosition(val-main_props_.left_page_border-contextWidth_);
	}
	else {
		main_props_.directPainter->setXPosition(val-main_props_.left_page_border- turnOverOffset_) ;
	}
	repaint();
}

void NMainFrameWidget::yscroll(int val) {
        topy_ = val;
	boty_ = val + (int) ((float) paperHeight_ / main_props_.zoom);
	main_props_.tp->setYPosition(val-TOP_BOTTOM_BORDER);
	main_props_.directPainter->setYPosition(val-TOP_BOTTOM_BORDER);
	main_props_.p->setYPosition(val-TOP_BOTTOM_BORDER);
	repaint();
}

/*-------------------------- reaction on timer events ---------------------------------*/

void NMainFrameWidget::playNext() {
	NMidiEventStr *m_events;
	QPtrList<NMidiEventStr> *tmp;
	NVoice *voice_elem;
	int min_time = (1 << 30);
	int last_time = myTime_;
	int pxpos;
	struct timeval now;
	struct timeval tempTime;

	if (playStop_) {
		NResource::mapper_->stopAllNotes(nextToPlay_);
		for (voice_elem = voiceList_.first(); voice_elem; voice_elem = voiceList_.next()) {
			voice_elem->stopPlaying();
		}
		playStop_ = playing_ = false;
		SortedTempoSigs_.clear();
		NResource::mapper_->isInUse_ = false;
		setScrollableNotePage();
		playButtonReset();
		main_props_.tp->setXPosition(leftx_-main_props_.left_page_border);
		main_props_.directPainter->setXPosition(leftx_-main_props_.left_page_border);
		repaint();
		notePart_->setMouseTracking(NResource::showAuxLines_);
		return;
	}

	nextToSearch_->clear();
	for (voice_elem = voiceList_.first(); voice_elem; voice_elem = voiceList_.next()) {
		m_events = voice_elem->getNextMidiEvent(myTime_+1, false);
		if (m_events) {
			nextToSearch_->append(m_events);
			if (m_events->ev_time < min_time) min_time = m_events->ev_time;
		}
	}

	for (m_events = stopList_.first(); m_events; m_events = stopList_.next()) {
		nextToSearch_->append(m_events);
		if (m_events->ev_time < min_time) {
			min_time = m_events->ev_time;
		}
	}
	notesToPlay_ = 0;
	for (m_events = nextToSearch_->first(); m_events; m_events = nextToSearch_->next()) {
		if (m_events->ev_time == min_time)  {
			pxpos = m_events->xpos;
			++notesToPlay_;
			if (m_events->midi_cmd == MNOTE_OFF) {
				stopList_.find(m_events);
				stopList_.remove();
			}
			else  {
				m_events->notehalt->ev_time = min_time + m_events->length;
				stopList_.append(m_events->notehalt);
				m_events->from->skipChord();
			}
		}
	}
	firstNoteActive_ = false;
	if (pxpos > leftx_ + paperScrollWidth_ - turnOverOffset_) {
		if (notesToPlay_) {
			scrollx_->setValue(leftx_ + paperScrollWidth_ - contextWidth_);
			xscrollDuringReplay(leftx_ + paperScrollWidth_ - contextWidth_);
		}
	}
	NResource::mapper_->play_list(nextToPlay_, last_time);
	tempo_ = SortedTempoSigs_.getTempoAtMidiTime(last_time);
	if (!notesToPlay_) {
		for (voice_elem = voiceList_.first(); voice_elem; voice_elem = voiceList_.next()) {
			voice_elem->stopPlaying();
		}
		playStop_ = playing_ = false;
		NResource::mapper_->isInUse_ = false;
		SortedTempoSigs_.clear();
		setScrollableNotePage();
		main_props_.tp->setXPosition(leftx_-main_props_.left_page_border);
		repaint();
		playButtonReset();
		return;
	}
	myTime_ = min_time;
	add_time(&tempTime, &nextPlayTime_, (int) (((double) ((myTime_ - last_time)) * 1000.0 * 60.0) / ((double) QUARTER_LENGTH * tempo_)));

	nextPlayTime_ = tempTime;
	tmp = nextToSearch_;
	nextToSearch_ = nextToPlay_;
	nextToPlay_ = tmp;
	if (nextStaffElemToBePainted_) {
		paintNextStaff();
	}
		
	if (pxpos <  leftx_ || pxpos > leftx_ + paperScrollWidth_ - turnOverOffset_ + 100) { // repeat or al Coda
		scrollx_->setValue(pxpos);
		xscrollDuringReplay(pxpos);
	}
	gettimeofday(&now, NULL);
	timer_.start(sub_time(&nextPlayTime_, &now), true);
}


/*----------------------------- internal reaction on resize --------------------------*/

void NMainFrameWidget::setScrollableNotePage() {
	NVoice *voice_elem;
	if (playing_) return;
	paperHeight_ = height_-MENUBARHEIGHT-TOOLBARHEIGHT-3*BORDER-SCROLLBARHEIGHT;
	paperScrollHeight_ = (int) ((float) paperHeight_ / main_props_.zoom);
	boty_ = topy_ + paperScrollHeight_;
	if (paperScrollHeight_ < lastYHeight_ + Y_SCROLL_DIST) {
		scrolly_->setGeometry(width_ - BORDER - SCROLLBARHEIGHT, MENUBARHEIGHT + TOOLBARHEIGHT,
		   	SCROLLBARHEIGHT, height_-MENUBARHEIGHT-TOOLBARHEIGHT-3*BORDER-SCROLLBARHEIGHT);
		scrolly_->setSteps (10, (int) ((float) height_ / main_props_.zoom));
		scrolly_->setRange(0, lastYHeight_);
		scrolly_->show();
		scrolly_->setValue(0);
		paperWidth_ = width_ - 3*BORDER - SCROLLBARHEIGHT;
	}
	else {
		topy_ = 0;
		boty_ = paperScrollHeight_;
		main_props_.tp->setYPosition(-TOP_BOTTOM_BORDER);
		main_props_.directPainter->setYPosition(-TOP_BOTTOM_BORDER);
		main_props_.p->setYPosition(-TOP_BOTTOM_BORDER);
		scrolly_->hide();
		paperWidth_ = width_ - 2*BORDER;
	}

	nettoWidth_ = paperWidth_ - (int) ((float) main_props_.left_page_border * main_props_.zoom) - RIGHT_PAGE_BORDER;
	nettoHeight_ = paperHeight_ - 2*TOP_BOTTOM_BORDER;
	paperScrollWidth_ = (int) ((float) paperWidth_ / main_props_.zoom);

	notePart_->setGeometry(BORDER, height()-3*BORDER-SCROLLBARHEIGHT-paperHeight_, paperWidth_, paperHeight_);
	notePart_->set1backpixmap(paperWidth_, paperHeight_);
	main_props_.tp->setPaintDevice(notePart_->acShowPixmap());
	main_props_.directPainter->setPaintDevice(notePart_);
	main_props_.p->setPaintDevice(notePart_->acShowPixmap());
	updatePainter();

	for (voice_elem = voiceList_.first(); voice_elem; voice_elem = voiceList_.next()) {
		voice_elem->paperDimensiones(paperScrollWidth_);
	}
	scrollx_->setSteps (10, (int) ((float) width_ / main_props_.zoom));
	if (!playing_) notePart_->setMouseTracking(NResource::showAuxLines_);

}

void NMainFrameWidget::preparePixmaps() {
	scrolly_->hide();
	notePart_->set2backpixmaps();

	newLeft_ = leftx_ + paperScrollWidth_ - turnOverOffset_;
	newRight_ = newLeft_ + paperScrollWidth_ - contextWidth_;
	main_props_.tp->setXPosition(newLeft_ - main_props_.left_page_border - contextWidth_);
	main_props_.tp->setPaintDevice(notePart_->acWritePixmap());
	main_props_.p->setPaintDevice(notePart_->acWritePixmap());
	main_props_.p->begin( notePart_->acWritePixmap() );
	main_props_.p->setBrush(NResource::backgroundBrush_);
	main_props_.p->setPen(NResource::noPen_);
	main_props_.p->setPen(editMode_ ? NResource::editModeBorderPen_ : NResource::blackPen_); //  color of border around drawing area
	main_props_.p->drawRect(0, 0, paperWidth_, paperHeight_);
	if (NResource::showContext_) {
		main_props_.p->fillRect(contextRec_, NResource::contextBrush_);
	}
	main_props_.p->end();
	nextStaffElemToBePainted_ = staffList_.first();
	nextStaffNr_ = 0;
	nextStaffIsFirstStaff_ = true;
}

/*----------------------------- update of buttons due to selection ------------------*/

/* Set Checked the note button according to the note length nr.
   Also, set main_props_.actualLength and main_props_.grace.
   nr >= 0 -> check note_buttons_[nr]
   nr < 0 -> check selectbutton_
*/
void NMainFrameWidget::setButton(int nr) {
	if ((editMode_) && (nr < 0)) return;
	if (nr >= 0) {
		note_buttons_[nr]->setOn(true);
	}
	else {
		selectbutton_->setOn(true);
	}

	main_props_.actualLength = NResource::button2Notelength_(length_before_edit_mode_);
	main_props_.grace = (length_before_edit_mode_ >= 9);
}

/* Updates buttons, labels and internals (main_props_) according to the musElement's properties:
   properties - general properties (accidentals, stems, beams, articulation, gracenotes)
   length - element length type. If -1 given, element doesn't have MIDI length (eg. barlines). */
void NMainFrameWidget::updateInterface(property_type properties, int length) {
	// avoid feedback
	// Note from David: you can use blockSignals instead (avoids duplicating code for connect/disconnect)
        // (For the KDE interface, this is not even necessary)
	// Ok! Thank you (J.Anders)

	beambutton_->setOn (properties & PROP_BEAMED);
	dotbutton_->setOn (properties & PROP_SINGLE_DOT);
	ddotbutton_->setOn (properties & PROP_DOUBLE_DOT);
	tiebutton_->setOn (properties & PROP_TIED);
	slurbutton_->setOn (properties & PROP_SLURED);
	tripletbutton_->setOn (properties & PROP_TUPLET);
	hiddenrestbutton_->setOn (properties & PROP_HIDDEN);
	main_props_.hidden = (properties & PROP_HIDDEN);
	staccatobutton_->setOn (properties & PROP_STACC);
	sforzatobutton_->setOn (properties & PROP_SFORZ);
	portatobutton_->setOn (properties & PROP_PORTA);
	strong_pizzicatobutton_->setOn (properties & PROP_STPIZ);
	sforzandobutton_->setOn (properties & PROP_SFZND);
	fermatebutton_->setOn (properties & PROP_FERMT);
	arpeggbutton_->setOn (properties & PROP_ARPEGG);
	pedonbutton_->setOn (properties & PROP_PEDAL_ON);
	pedoffbutton_->setOn (properties & PROP_PEDAL_OFF);

	stemUpbutton_->setOn(properties & PROP_STEM_UP);
	stemDownbutton_->setOn(!(properties & PROP_STEM_UP));
        offs_buttons_[0]->setOn(properties & PROP_CROSS);
	if (properties & PROP_CROSS) {
            actualOffs_ = 1;
        }
        offs_buttons_[1]->setOn(properties & PROP_FLAT);
	if (properties & PROP_FLAT) {
            actualOffs_ = -1;
        }
        offs_buttons_[3]->setOn(properties & PROP_DCROSS); 
	if (properties & PROP_DCROSS) {
            actualOffs_ = 2;
        }
        offs_buttons_[4]->setOn(properties & PROP_DFLAT); 
	if (properties & PROP_DFLAT) {
            actualOffs_ = -2;
        }
        offs_buttons_[2]->setOn(properties & PROP_NATUR);
	if (properties & PROP_NATUR) {
            actualOffs_ = 0;
        }
	if (!(properties & ACC_MASK)) {
	    actualOffs_ = UNDEFINED_OFFS;
	}
	main_props_.dotcount = (properties & DOT_MASK);
	main_props_.tied = (properties & PROP_TIED);
	main_props_.staccato = (properties & PROP_STACC);
	main_props_.sforzato = (properties & PROP_SFORZ);
	main_props_.portato  = (properties & PROP_PORTA);
	main_props_.strong_pizzicato = (properties & PROP_STPIZ);
	main_props_.sforzato  = (properties & PROP_SFZND);
	main_props_.fermate   = (properties & PROP_FERMT);
	main_props_.grace     = (properties & PROP_GRACE);
	main_props_.actualLength = length;
	main_props_.pedal_on = (properties & PROP_PEDAL_ON); 
	main_props_.pedal_off = (properties & PROP_PEDAL_OFF); 
	if (properties & PROP_STEM_UP) {
		main_props_.actualStemDir = STEM_DIR_UP;
	}
	else if (properties & STEM_DIR_DOWN) {
		main_props_.actualStemDir = STEM_DIR_DOWN;
	}
	else {
		main_props_.actualStemDir = STEM_DIR_AUTO;
	}
	main_props_.noteBody = (properties & BODY_MASK);
	switch (main_props_.noteBody) {
		case PROP_BODY_CROSS: crossDrumBu_->setOn(true); break;
		case PROP_BODY_CROSS2: cross2DrumBu->setOn(true); break;
		case PROP_BODY_CIRCLE_CROSS: crossCricDrumBu_->setOn(true); break;
		case PROP_BODY_RECT: rectDrumBu_->setOn(true); break;
		case PROP_BODY_TRIA: triaDrumBu_->setOn(true); break;
		default: crossDrumBu_->setOn(false);
			 cross2DrumBu->setOn(false);
			 crossCricDrumBu_->setOn(false);
			 rectDrumBu_->setOn(false);
			 triaDrumBu_->setOn(false);
	}

	/* turns on appropriate note length button */
	if (!main_props_.grace)
		setButton(NResource::noteLength2Button_(length));
	else
		/* grace note buttons are just shifted right for 5 buttons */
		setButton(NResource::noteLength2Button_(length)+5);
}

void NMainFrameWidget::playButtonReset() {
	playbutton_->setOn(false);
}

/*-----------------------------(re-)storing ----------------------------------------*/


void NMainFrameWidget::writeStaffs(const char *fname) {
	if (playing_) return;
#ifdef WITH_TSE3
	kbbutton_->setOn(false);
#endif
	if (fhandler_->writeStaffs(fname , &staffList_, this, true)) {
		setEdited(false);
	}
}

void NMainFrameWidget::autosave(int nr) {
	if (playing_) return;
	QString savname;
	if (actualFname_.isNull()) {
		savname.sprintf("unnamed%d.not.sav", nr);
		fhandler_->writeStaffs(savname, &staffList_, this, false);
	}
	else {
		fhandler_->writeStaffs(actualFname_ + QString(".sav"), &staffList_, this, false);
	}
}
	

bool NMainFrameWidget::readStaffs(const char *fname) {
	NVoice *voice_elem;
	if (playing_) return false;
#ifdef WITH_TSE3
	kbbutton_->setOn(false);
#endif

	if (!fhandler_->readStaffs(fname , &voiceList_, &staffList_, this)) {
		return false;
		
	}
	setEdited(false);
	staffCount_ = staffList_.count();

	for (voice_elem = voiceList_.first(); voice_elem; voice_elem = voiceList_.next()) {
		voice_elem->paperDimensiones(paperScrollWidth_);
	}
	currentStaff_ = staffList_.first();
	currentStaff_->setActual(true);
	currentVoice_ = currentStaff_->getVoiceNr(0);
	enableCriticalButtons(true);
	voiceDisplay_->setMax(currentStaff_->voiceCount());
	voiceDisplay_->setVal(0);
	lastYHeight_ = voiceList_.last()->getStaff()->getBase()+voiceList_.last()->getStaff()->underlength_;
	computeMidiTimes(false);
	selectedSign_ = 0;
	NVoice::resetUndo();
	setSelectMode();
	main_props_.tp->setYPosition(-TOP_BOTTOM_BORDER);
	main_props_.directPainter->setYPosition(-TOP_BOTTOM_BORDER);
	main_props_.p->setYPosition(-TOP_BOTTOM_BORDER);
	cleanupSelections();
	return true;
}

/*---------------------------- positioning ------------------------------------------*/

void NMainFrameWidget::reposit() {
	NPositStr *posit;
	QPtrList<NPositStr> plist;
	int min_time;
	int maxwidth, width;
	int current_xpos = LEFT_SPACE;
	int num_positions;
	bool only_playables;
	int sequNr = 0;
	NStaff *staff_elem;
	myTime_ = 0;

	if (playing_) return;

	for (staff_elem = staffList_.first(); staff_elem; staff_elem = staffList_.next()) {
		staff_elem->startRepositioning();
	}
	while(1) {
		plist.clear();
		num_positions = 0;
		min_time = (1 << 30);
		maxwidth = 0;
		for (staff_elem = staffList_.first(); staff_elem; staff_elem = staffList_.next()) {
			staff_elem->getElementsAfter(&plist, myTime_, &num_positions, &min_time);
		}
		if (!num_positions) {
			lastBarNr_ = voiceList_.first()->getLastBarNr();
			oldLastXpos_ = lastXpos_;
			lastXpos_ = current_xpos;
			return;
		}
		only_playables = true;
		for (posit = plist.first(); posit; posit = plist.next()) {
			if (posit->ev_time == min_time && !(posit->ev_type & PLAYABLE)) only_playables = false;
		}
		for (posit = plist.first(); posit; posit = plist.next()) {
			if (only_playables) {
				if (posit->ev_time == min_time)  {
					width = posit->from->placeAt(current_xpos, sequNr) + ELEM_SPACE;
					if (width > maxwidth) maxwidth = width;
					delete posit;
				}
			}
			else {
				if (posit->ev_time == min_time && !(posit->ev_type & PLAYABLE))   {
					width = posit->from->placeAt(current_xpos, sequNr) + ELEM_SPACE;
					if (width > maxwidth) maxwidth = width;
					delete posit;
				}
			}
		}
		current_xpos += maxwidth;
		++sequNr;
		if (only_playables)
			myTime_ = min_time+1;
	}

}

void NMainFrameWidget::computeMidiTimes(bool insertBars, bool doAutoBeam) {
	NVoice *voice_elem;
	for (voice_elem = voiceList_.first(); voice_elem; voice_elem = voiceList_.next()) {
		voice_elem->getStaff()->staff_props_.measureLength = voice_elem->getCurrentMeasureMidiLength();
		voice_elem->computeMidiTime(insertBars, doAutoBeam && voice_elem == currentVoice_);
	}
}

/*------------------------------- selection ---------------------------------------*/

int NMainFrameWidget::checkAllStaffsForNoteInsertion(const int line, const QPoint p, property_type *properties, bool *playable, bool *delete_elem, bool *insertNewNote) {
	int val;
	NMusElement *elem;

	if (playing_) return -1;
	if (!checkStaffIntersection(p)) return -1;
	if ((val = currentStaff_->checkElementForNoteInsertion(line, p, properties, playable, delete_elem, insertNewNote, actualOffs_)) > 0) {
		manageToolElement(false);
		return val;
	}
	else if (editMode_) {
    		elem = currentVoice_->getCurrentElement();
		if (elem && elem->getType() == T_TEXT) {
			((NText *) elem)->startTextDialog();
		}
	}
	return -1;
}

bool NMainFrameWidget::checkStaffIntersection(const QPoint p) {
	if (playing_) return false;
	NStaff *staff_elem;
	int idx = -1, i, dist, mindist = 10000000;

	if (currentStaff_->intersects(p) != -1) {currentStaff_->setActual(true); return true;}
	for (i = 0, staff_elem = staffList_.first(); staff_elem; staff_elem = staffList_.next(), i++) {
		dist = staff_elem->intersects(p);
		if (dist >= 0 && dist < mindist) {
			mindist = dist;
			idx = i;
		}
	}
	if (idx == -1) {
		currentStaff_->setActual(false);
		currentVoice_->release();
		return false;
	}
	currentStaff_->setActual(false);
	currentVoice_->release();
	currentStaff_->draw(leftx_, leftx_ + nettoWidth_);
	currentStaff_ = staffList_.at(idx);
	currentVoice_ = currentStaff_->getActualVoice();
	enableCriticalButtons(currentVoice_->isFirstVoice());
	currentStaff_->setActual(true);
	currentStaff_->draw(leftx_, leftx_ + nettoWidth_);
	voiceDisplay_->setMax(currentStaff_->voiceCount());
	voiceDisplay_->setVal(currentStaff_->getActualVoiceNr()+1);
#ifdef WITH_TSE3
	NResource::mapper_->setEchoChannel(currentStaff_->getChannel(), currentStaff_->getVoice());
#endif
	return true;
}

void NMainFrameWidget::nextElement() {
	if (playing_) return;
	property_type properties;
	unsigned int val;
	val = currentVoice_->makeNextElementActual(&properties);
	if (editMode_) {
		/* First clean up the interface, so the new element doesn't inherit any properties from the old one. */
		updateInterface(0, -1);
		
		if (currentVoice_->getCurrentElement()->playable())
			updateInterface(properties, val);
		else
			updateInterface(properties, -1);
	}
	manageToolElement(false);
	repaint();
}


void NMainFrameWidget::prevElement() {
	if (playing_) return;
	property_type properties;
	unsigned int val;
	val = currentVoice_->makePreviousElementActual(&properties);
	if (editMode_) {
		/* First clean up the interface, so the new element doesn't inherit any properties from the old one. */
		updateInterface(0, -1);
		
		if (currentVoice_->getCurrentElement()->playable())
			updateInterface(properties, val);
		else
			updateInterface(properties, -1);
	}
	manageToolElement(false);
	repaint();
}

void NMainFrameWidget::undo() {
	NVoice *ref;
	if ((ref = NVoice::undoPossible()) == 0) return;
	ref->undo();
	computeMidiTimes(false);
	reposit();
	repaint();
}

void NMainFrameWidget::redo() {
	NVoice *ref;
	if ((ref = NVoice::redoPossible()) == 0) return;
	ref->redo();
	computeMidiTimes(false);
	reposit();
	repaint();
}

/*------------------------------ TSE3 -----------------------------------------------*/
void NMainFrameWidget::createTSE3() {
#ifdef WITH_TSE3
	if (recordButton_->isChecked()) return;
	kbbutton_->setOn(false);
	tse3Handler_->createTSE3(&voiceList_);
#endif

}
void NMainFrameWidget::playSong() {
#ifdef WITH_TSE3
	if (recordButton_->isChecked()) return;
	kbbutton_->setOn(false);
	tse3Handler_->playSong();
#endif
}

void NMainFrameWidget::writeTSE3() {
#ifdef WITH_TSE3
	if (recordButton_->isChecked()) return;
	if (playing_) return;
	kbbutton_->setOn(false);
	QString fileName = this->checkFileName(KFileDialog::getSaveFileName( QString::null, tse3_file_pattern, this ), (char *)".tse3");
	if (fileName.isNull() )  return;

	if (!tse3Handler_->writeTSE3(QFile::encodeName(fileName)))
		KMessageBox::sorry
		  (this,
		   i18n("Error writing file \"%1\".").arg(fileName),
		   kapp->makeStdCaption(i18n("Write TSE3"))
		  );
#endif
}

void NMainFrameWidget::TSE3Filter() {
#ifdef WITH_TSE3
	tse3Handler_->initFiltering();
#endif
}

void NMainFrameWidget::importRecording() {
#ifdef WITH_TSE3
	if (recordButton_->isChecked()) return;
	if (KMessageBox::warningYesNo
	     (this,
	      i18n("This overrides the current staff! Continue?"),
	      kapp->makeStdCaption(i18n("Import Recording")),
	      i18n("&Import Recording"),
	      i18n("&Cancel")
	     )
	    == KMessageBox::No
	   ) return;
	tse3Handler_->TSE3Rec2Staff(currentStaff_, &voiceList_);
	currentStaff_->changeActualVoice(-1);
	voiceDisplay_->setMax(currentStaff_->voiceCount());
	voiceDisplay_->setVal(currentStaff_->getActualVoiceNr()+1);
	setEdited();
#endif
}

#ifdef WITH_TSE3
void NMainFrameWidget::createStaffFromPhraseEdit(TSE3::PhraseEdit *phraseEdit) {
	tse3Handler_->TSE3PhraseEditToStaff(phraseEdit, currentStaff_);
	setEdited();
}

bool NMainFrameWidget::stillRecording() {
    return recordButton_->isChecked();
    }
#endif

void NMainFrameWidget::completeRecording(bool ok) {
#ifdef WITH_TSE3
	if (!ok) {
		recordButton_->setOn(false);
		return;
	}
	setEdited();
	computeMidiTimes(false);
        reposit();
	setScrollableNotePage();
	repaint();
#endif
}

bool NMainFrameWidget::TSE3toScore() {
#ifdef WITH_TSE3
	if (recordButton_->isChecked()) return false;
	if (playing_) return false;
	if (KMessageBox::warningYesNo(0, i18n("This will clear the existing document. Are you sure?"),
					kapp->makeStdCaption(i18n("Confirmation"))) == KMessageBox::No)
		return false;
	kbbutton_->setOn(false);
	newPaper();
	tse3Handler_->TSE3toScore(&staffList_, &voiceList_, false);
	return true;
#endif
	return false;
}

void NMainFrameWidget::TSE3ParttoScore() {
#ifdef WITH_TSE3
	if (recordButton_->isChecked()) return;
	if (playing_) return;
	kbbutton_->setOn(false);
	tse3Handler_->TSE3toScore(&staffList_, &voiceList_, true);
#endif
}

void NMainFrameWidget::completeTSE3toScore(bool ok) {
#ifdef WITH_TSE3
	NVoice *voice;
	double partTime, readyPart;
	int i;
	int maxmiditime = 0, difftime;
	if (!ok)  {
		KMessageBox::error
		  (this,
		   i18n("Error creating Score"),
		   kapp->makeStdCaption(i18n("Creating Score"))
		  );
		NResource::progress_->hide();
		return;
	}
	currentVoice_= voiceList_.first();
	currentStaff_ = currentVoice_->getStaff();
	enableCriticalButtons(true);
	staffCount_ = staffList_.count();
	voiceDisplay_->setMax(currentStaff_->voiceCount());
	lastYHeight_ = voiceList_.last()->getStaff()->getBase()+voiceList_.last()->getStaff()->underlength_;
	currentStaff_->setActual(true);
	setEdited(false);
	computeMidiTimes(false);
	for (voice = voiceList_.first(); voice; voice = voiceList_.next()) {
		if (voice->getMidiEndTime() > maxmiditime) maxmiditime = voice->getMidiEndTime();
	}
	maxmiditime += WHOLE_LENGTH;

	readyPart = 100.0 * FIRST_PART_TIME;
	partTime = (double) voiceList_.count() / (100.0 * SECOND_PART_TIME) + 1.e-20;
	for (i = 0, voice = voiceList_.first(); voice; voice = voiceList_.next(), i++) {
		NResource::progress_->setValue((int) (readyPart + (double) (i+1) / partTime));
		difftime = maxmiditime - voice->getMidiEndTime();
		voice->handleEndAfterMidiImport(difftime);
	}
	computeMidiTimes(false);
	tse3Handler_->insertTimeAndKeySigs(&staffList_);
	computeMidiTimes(false);
	renewStaffLayout();
	createLayoutPixmap();
	setScrollableNotePage();
        reposit();
	scrollx_->setValue(0); // includes repaint()
	cleanupSelections();
	tempo_ = DEFAULT_TEMPO;
	NResource::progress_->hide();
	repaint();
#endif
}

void NMainFrameWidget::TSE3MidiOut() {
#ifdef WITH_TSE3
	if (recordButton_->isChecked()) return;
	if (playing_) return;
	kbbutton_->setOn(false);
	QString fileName = this->checkFileName(KFileDialog::getSaveFileName(QString::null, midi_file_pattern, this), (char *)".mid");
	if (fileName.isNull() )  return;
	if (!tse3Handler_->TSE3MidiOut(fileName.local8Bit()))
		KMessageBox::sorry
		  (this,
		   i18n("Error writing file \"%1\".").arg(fileName),
		   kapp->makeStdCaption(i18n("TSE3 MIDI out"))
		  );
#endif
}

bool NMainFrameWidget::TSE3MidiIn() {
#ifdef WITH_TSE3
	if (recordButton_->isChecked()) return false;
	if (playing_) return false;
	kbbutton_->setOn(false);
	QString fileName = KFileDialog::getOpenFileName( QString::null, midi_file_pattern, this );
	if (fileName.isNull() )  return false;
	if (!tse3Handler_->TSE3MidiIn(fileName.local8Bit())) {
		KMessageBox::sorry
		  (this,
		   i18n("File read error \"%1\".").arg(fileName),
		   kapp->makeStdCaption(i18n("TSE3 MIDI in"))
		  );
		return false;
	}
	repaint();
	return true;
#endif
	return false;
}

void NMainFrameWidget::TSE3record(bool on) {
#ifdef WITH_TSE3
	if (!on) {
		importRecording();
		return;
	}
	if (playing_) return;
	kbbutton_->setOn(false);
	if (!tse3Handler_->TSE3record(currentStaff_->getChannel(), &voiceList_)) {
		disconnect(recordButton_, SIGNAL(toggled(bool)), this, SLOT(TSE3record(bool)));
		recordButton_->setOn(false);
		connect(recordButton_, SIGNAL(toggled(bool)), this, SLOT(TSE3record(bool)));
	}
#endif
}
void NMainFrameWidget::readTSE3() {
#ifdef WITH_TSE3
	if (playing_) return;
	kbbutton_->setOn(false);
	QString fileName = KFileDialog::getOpenFileName( QString::null, tse3_file_pattern, this);
	if (fileName.isNull() )  return;
	if (!tse3Handler_->readTSE3(fileName.ascii()))
		KMessageBox::sorry
		  (this,
		   i18n("File read error \"%1\".").arg(fileName),
		   kapp->makeStdCaption(i18n("Read TSE3"))
		  );
#endif
}

/*----------------------------- modification -------------------------------------*/

void NMainFrameWidget::moveUp() {
	if (playing_) return;
	currentVoice_->moveUp(1);
	setEdited();
	reposit();
	repaint();
}

void NMainFrameWidget::moveDown() {
	if (playing_) return;
	currentVoice_->moveDown(true);
	setEdited();
	reposit();
	repaint();
}

void NMainFrameWidget::moveSemiToneUp() {
	if (playing_) return;
	currentVoice_->moveSemiToneUp();
	setEdited();
	reposit();
	repaint();
}

void NMainFrameWidget::moveSemiToneDown() {
	if (playing_) return;
	currentVoice_->moveSemiToneDown();
	setEdited();
	reposit();
	repaint();
}

void NMainFrameWidget::moveOctaveUp() {
	if (playing_) return;
	currentVoice_->moveUp(7);
	setEdited();
	reposit();
	repaint();
}

void NMainFrameWidget::moveOctaveDown() {
	if (playing_) return;
	currentVoice_->moveDown(7);
	setEdited();
	reposit();
	repaint();
}

void NMainFrameWidget::deleteElem(bool backspace) {
	property_type properties;
	unsigned int val;
	if (playing_) return;
	val = currentVoice_->deleteActualElem(&properties, backspace);
	if (editMode_)
		if (currentVoice_->getCurrentElement()->playable())
			updateInterface(properties, val);
		else
			updateInterface(properties, -1);

	computeMidiTimes(false);
	if (!editiones_) setEdited((int)val != -1);
	reposit();
	repaint();
}

/*----------------------------- KDE main window -------------------------------------*/

NMainWindow::NMainWindow(QWidget *parent, const char *name )
    : KMainWindow(parent, name), closeFromApplication_(false)
{
	QWidget * w = new NMainFrameWidget( actionCollection(), false, this  );
	NResource::windowList_.append(this);
	setCentralWidget( w );
	//w->setFocus();
	connect( w, SIGNAL( caption( const QString & ) ), this, SLOT( slotCaption( const QString & ) ) );

	// Now build the GUI
	createGUI();

	// Remove "Report Bug" menu item from help menu
	KAction *rbug = actionCollection()->action(KStdAction::stdName(KStdAction::ReportBug));
	if( rbug )
	{
	  rbug->unplugAll();
	  actionCollection()->remove(rbug);
	}
	else
	  printf("Could not remove ReportBug menu item\n");
}

void NMainWindow::slotCaption( const QString & s )
{
	setCaption( s );
}



NMainFrameWidget * NMainWindow::mainFrameWidget() const
{
	return static_cast<NMainFrameWidget *>( centralWidget() );
}

void NMainWindow::closeEvent ( QCloseEvent * e ) {
	if (!closeFromApplication_) mainFrameWidget()->quitDialog2();
	if (closeFromApplication_) KMainWindow::closeEvent(e);
}

/*------------------------------------- tools ------------------------------------*/

QString NMainFrameWidget::checkFileName(QString fileName, char *extension)
{
	if (!fileName.isNull() ) {
		if (fileName.find(extension, -strlen(extension), true) < 0)
	    	fileName+=extension;
		if (!access(fileName, F_OK))  {
			if (KMessageBox::warningYesNo
			     (0,
			      i18n("File \"%1\" already exists! Overwrite?").arg(fileName),
			      kapp->makeStdCaption(i18n("File already exists"))
			     )
		      == KMessageBox::No
			   ) return 0;
		}
	}
	return fileName;
}

/*------------------------------------- help -------------------------------------*/

void NMainFrameWidget::showTipOfTheDay() {
#if KDE_VERSION >= 220
	KTipDialog::KTipDialog::showTip(locate("data", "noteedit/tips"), true);
#else
	tipFrm( 0, NResource::tipNo_ );
#endif
}

#ifdef WITH_DIRECT_PRINTING

// RK: Completely simplified the dialog, layout cannot be done within this class
PrintExportDialogPage::PrintExportDialogPage( QString title, QWidget *tab, QWidget *parent, const char *name )
 : KPrintDialogPage( parent, name )
{
    setTitle( title );
}

PrintExportDialogPage::~PrintExportDialogPage()
{
}

void PrintExportDialogPage::getOptions( QMap<QString,QString>& /*opts*/, bool /*incldef*/ )
{
}

void PrintExportDialogPage::setOptions( const QMap<QString,QString>& /*opts*/ )
{
}

bool PrintExportDialogPage::isValid( QString& /*msg*/)
{
  return true;
}

#endif /* WITH_DIRECT_PRINTING */

#include "mainframewidget.moc"
