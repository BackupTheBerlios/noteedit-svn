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

#ifndef MYFRAME_WIDGET_H

#define MYFRAME_WIDGET_H

#include <sys/time.h>
#include <qtimer.h>
#include <qthread.h>
#include "config.h"
#include <kmainwindow.h>
#include <kaction.h>
#if QT_VERSION >= 300
#include <qpushbutton.h>
#include <qlabel.h>
#include <qscrollbar.h>
#endif
#include "muselement.h"
#include "tempotrack.h"

#define COUNT_OFFSBUTTONS 5
#define COUNT_CHORDBUTTONS 12

class NMidiMapper;
class NVoice;
class NStaff;
class volumeFrm;
class lyricsFrm;
class smallestRestFrm;
class voiceDiaFrm;
class NKeyOffs;
class NKeySig;
class NMidiEventStr;
class NFileHandler;
class NLilyExport;
class NTimeSig;
class NDbufferWidget;
class staffelFrm;
class listFrm;
class propFrm;
class NInfoTable;
class NZoomSelection;
class QTabWidget;
class staffFrm;
class scorFrm;
class timesigDiaFrm;
class QPushButton;
class NChordDiagram;
class ChordSelector;
class tupletDialogImpl;
class IntPrinter;
class KProcess;

#ifdef WITH_TSE3
#include <tse3/PhraseEdit.h>
class NTSE3Handler;
#endif

class MusicXMLParser;

class KRecentFilesAction;
class KURL;
class KToggleAction;
class KRadioAction;
class scaleFrm;
class staffPropFrm;
class exportFrm;
class saveParametersFrm;
class QSlider;
class QCheckBox;
class NMusElement;
class KToggleAction;
class NNumberDisplay;
class NStaffLayout;
class layoutDef;

class NMainFrameWidget;
class NMainWindow : public KMainWindow
{
	Q_OBJECT
public:
	NMainWindow(QWidget *parent=0, const char *name=0);
        NMainFrameWidget * mainFrameWidget() const;
	void setCloseFromApplication() {closeFromApplication_ = true;}
protected:
	virtual void closeEvent ( QCloseEvent * e );
protected slots:
	void slotCaption( const QString & s );
private:
	bool closeFromApplication_;
};

class NMainFrameWidget : public QWidget
{
	Q_OBJECT
	public:
		NMainFrameWidget (KActionCollection *actObj, bool inPart, QWidget *parent=0, const char *name=0);
		~NMainFrameWidget();
		bool loadFile( const QString & fileName );
		void readStaffsFromXMLFile( const char *fname );
		void setEdited(bool = true);
		void reloadRecentFileList();
		void synchronizeRecentFiles();
		void processMouseEvent(QMouseEvent * evt);
		void processMoveEvent(QMouseEvent * evt);
		void processWheelEvent(QWheelEvent * e );
		void stopTimer() {autoscrollTimer_.stop();}
		void generateClef(int type, int shift);
		void createTuplet(char numNotes, char playtime);
		void performClefChange(int type, int shift);
		void arrangeStaffs(bool create_layout_pixmap);
		void paintNew();
		void setDummyNoteAndAuxLines(QMouseEvent *evt);
		void restoreAllBehindDummyNoteAndAuxLines();
		void updateChordnames();
		bool isPlaying() {return playing_;}
		void grabElementsAccording();
		bool testEditiones();
		void addVoice(NVoice *voice, int numVoices);
		void removeVoice(NVoice *voice, NVoice *newCurrentVoice, int actualVoiceNr, int numVoices);
		void reposit();
		void autosave(int nr);
		void setTempTimesig(int num, int dom);
		bool paramsEnabled();
		int getSaveWidth();
		int getSaveHeight();
		bool withMeasureNums();
		void setParamsEnabled(bool ok);
		void setSaveWidth(int width);
		void setSaveHeight(int height);
		void setWithMeasureNums(bool with);
		QString scTitle_, scSubtitle_, scAuthor_, scLastAuthor_, scCopyright_, scComment_;

		QPtrList<NMusElement> *getClipBoard(int clipBoardNr);
		main_props_str main_props_;
		void exportMusixTeXImm();
		void exportLilyPondImm();
		void exportABCImm();
		void setTempChord(NChordDiagram *cdiagram);
		void RemoveChord();   
    
#ifdef WITH_TSE3
		void createStaffFromPhraseEdit(TSE3::PhraseEdit *phraseEdit);
		bool stillRecording();
#endif
/*-------------------------------- layout --------------------------------------*/
		layoutDef *braceMatrix_;
		layoutDef *bracketMatrix_;
		layoutDef *barCont_;
		int context_rect_left_right_;
		void createLayoutPixmap();
		void updatePainter();

/*------------------- for anthem plugin -----------------------------------------------*/
		void plugButtons(KToolBar *toolbar);
		void unPlugButtons(KToolBar *toolbar);
/*------------------------------------- tools ------------------------------------*/
		static QString checkFileName(QString fileName, char *extension);

	public slots:
		void importRecording();
		void changeZoomValue(int);
		void playAll(bool);
		void quitDialog2();
	signals:
		void caption( const QString & caption );
	protected:
		KActionCollection * actionCollection() const { return m_actionCollection; }
		KActionCollection * m_actionCollection;
		KRecentFilesAction * m_recentFilesAction;
/*------------------------- reaction on QWidget events -------------------------------*/
		virtual void resizeEvent ( QResizeEvent *evt );
		virtual void paintEvent( QPaintEvent * );
	private slots:
/*-------------------------- reaction on pushbutton events ----------------------------- */
		void setToDFull(bool on);
		void setToFull(bool on);
		void setToHalf(bool on);
		void setToQuarter(bool on);
		void setToN8(bool on);
		void setToN16(bool on);
		void setToN32(bool on);
		void setToN64(bool on);
		void setToN128(bool on);
		void setToGN8(bool on);
		void setToGN16(bool on);
		void setToGNS8(bool on);
		void setDotted(bool);
		void setDDotted(bool);
		void setActualTied(bool);
		void setStaccato(bool);
		void setSforzato(bool);
		void setPortato(bool);
		void setStrong_pizzicato(bool);
		void setSforzando(bool);
		void setFermate(bool);
		void setBeamed(bool);
		void setSlured(bool);
		void setTriplet(bool);
		void setArpegg(bool);
		void setPedalOn(bool);
		void setPedalOff(bool);
		void setHidden(bool);
		void setCrossBody(bool);
		void setCross2Body(bool);
		void setCrossCircBody(bool);
		void setRectBody(bool);
		void resetSpecialButtons();
		void quitDialog();
		void setTrianBody(bool);
		void changeActualVoice(int voiceNr);
		void setCross(bool);
		void setFlat(bool);
		void setDCross(bool);
		void setDFlat(bool);
		void setNatur(bool);
		void setStemUp(bool);
		void setStemDown(bool);
		void setSelectMode();
		void setEditMode(bool);
		void allowKbInsert(bool);
		void setKbMode(bool);
		void setKbInsertMode(bool);
		void TSE3record(bool);
		void readNotesFromMidiMapper();

/*--------------------------- reaction on menu events -----------------------------------*/
		void scoreInfo();
		void configure();
		bool newPaper();
		void openNewWindow();
		void zoomIn();
		void zoomOut();
		void chordDialog();
		void createTuplet();
		void keyConfig();
		void closeAllWindows();
		void toggleBarNumbers();
		void toggleStaffNames();
		void toggleAuxLines();
		void toggleStaffContext();
		void toggleDrumUp();
		void setDrumToolbar();
		void gotoDialog();
		void muteDialog();
		void voiceDialog();
		void setStaffProperties();
		void layoutDialog();
		void autoBar();
		void fileOpen();
		void fileOpenRecent(const KURL &);
		void fileSave();
		void fileSaveAs();
/*------------------------------ printing--------------------------------------------*/
    void filePrint(bool);  // Jorge Windmeisser Oliver
    void filePrintPreview();
    void filePrintPreviewFinished(KProcess *);
    void filePrintReceivedStdOut(KProcess *, char *, int);
    void filePrintReceivedStdErr(KProcess *, char *, int);
    void filePrintNoPreview();
    void setupPrinting(bool); // Helper methods for filePrint
    void printWithLilypond(bool, QString, QString);
    void printWithABC(bool, QString, QString);
    void printWithPMX(bool, QString, QString);
    void printWithMusiXTeX(bool, QString, QString);
    void printWithMusicXML(bool, QString, QString);
    void printWithMidi(bool, QString, QString);
    void printWithNative(bool, QString, QString);
/*-----------------------------------------------------------------------------------*/        
		void exportMusiXTeX();
		void exportPMX();
		void exportABC();
		void exportMusicXML();
		void exportLilyPond();
		void exportMidi();
		void importMidi();
		void importMusicXML();
		void setOutputParam();
		void insertRepeatOpen();
		void insertRepeatOpenClose();
		void insertRepeatClose();
		void repeatCountDialog();
		void insertspecEnding1();
		void insertspecEnding2();
		void insertDoubleBar();
		void insertEndBar();
		void insertSegno();
		void insertDalSegno();
		void insertDalSegnoAlFine();
		void insertDalSegnoAlCoda();
		void insertFine();
		void insertCoda();
		void insertRitardando();
		void insertAccelerando();
		void keyDialog();
		void timesigDialog();
		void tempoSigDialog();
		void multiRestDialog();
		void newStaff();
		void deleteStaff();
		void staffMoveDialog();
		void cleanRestsDialog();
		void volChangeDialog();
		void autoBeamDialog();
		void transposeDialog();
		void voiceChangeDialog();
		void insertLine();
		void insertText();
		void clefDialog();
		void changeClefDialog();
		void redAccidentals();
		void collChords();
		void setAllSharp();
		void setAllFlat();
		void multiStaffDialog();
		void cancelMultiStaff();
		void showTipOfTheDay();
/*------------------------- key events --------------------------------------------------*/
		void KE_moveUp();
		void KE_moveDown();
		void KE_moveSemiUp();
		void KE_moveSemiDown();
		void KE_moveLeft();
		void KE_moveStart();
		void KE_moveEnd();
		void KE_moveRight();
		void KE_delete();
		void KE_play();
		void KE_leaveCurrentMode();
		void KE_edit();
		void KE_insertnote();
		void KE_insertchordnote();
		void KE_1();
		void KE_2();
		void KE_3();
		void KE_4();
		void KE_5();
		void KE_6();
		void KE_7();
		void KE_8();
		void KE_9();
		void KE_voice1();
		void KE_voice2();
		void KE_voice3();
		void KE_voice4();
		void KE_voice5();
		void KE_voice6();
		void KE_voice7();
		void KE_voice8();
		void KE_voice9();
		void KE_tie();
		void KE_dot();
		void KE_flat();
		void KE_sharp();
		void KE_natural();
		void KE_bar();
		void KE_remove();
		void KE_removechordnote();
		void KE_tab();
		void KE_insertRest();
		void KE_underscore();
		void KE_keybordInsert();
		
/*------------------------- "note" keys -----------------------------------------------*/

		void KE_pitch_C();
		void KE_pitch_D();
		void KE_pitch_E();
		void KE_pitch_F();
		void KE_pitch_G();
		void KE_pitch_A();
		void KE_pitch_B();


/*--------------------------- reaction on Ok button of the dialogs above ----------------*/
		void changeVoice(int voice = -1);
		void cleanupRests();
		void doAutoBeam();
		void insVolChange();
		void showLyricsDialog();
//		void changeTimesig(int num, int denom);
		void setInsertionKey();
		void changeKey(int idx);

/*-------------------------- reaction on scroll events ---------------------------------*/
		inline void xscroll(int val, bool _repaint = true);
		inline void yscroll(int val, bool _repaint = true);
		void trillLengthChanged(int val); // located in mainframewidget2.cpp
		void trillDisabled();
		void dynamicPosChanged(int val);
		void dynamicKill();
		void dynamicSwitch();
		void vaLengthChanged(int val); // located in mainframewidget2.cpp
		void vaDisabled();

/*-------------------------- reaction on timer events ---------------------------------*/
		void playNext();
	private :
		void enableCriticalButtons(bool enable);
		void pitchToLine(int pitchNumber);
		KAccel *keys_;
		QPtrList<KAction> criticalButtons_;
		KToggleAction *playbutton_;
		KToggleAction *stemUpbutton_;
		KToggleAction *stemDownbutton_;
		KToggleAction *dotbutton_;
		KToggleAction *ddotbutton_;
		KToggleAction *tiebutton_;
		KToggleAction *staccatobutton_;
		KToggleAction *sforzatobutton_;
		KToggleAction *portatobutton_;
		KToggleAction *strong_pizzicatobutton_;
		KToggleAction *sforzandobutton_;
		KToggleAction *fermatebutton_;
		KToggleAction *beambutton_;
		KToggleAction *slurbutton_;
		KToggleAction *tripletbutton_;
		KToggleAction *arpeggbutton_;
		KToggleAction *pedonbutton_;
		KToggleAction *pedoffbutton_;
		NNumberDisplay *voiceDisplay_;
		KToggleAction *offs_buttons_[COUNT_OFFSBUTTONS];
		KToggleAction *selectbutton_;		
		KToggleAction *editbutton_;
		KToggleAction *allowKbInsertButton_;
		KToggleAction *gluebutton_;
		NZoomSelection *zoomselect_;
		KToggleAction *hiddenrestbutton_;
		KToggleAction *kbbutton_;
		KToggleAction *kbInsertButton_;
#ifdef WITH_TSE3
		KToggleAction *recordButton_;
#endif
		KToggleAction *note_buttons_[COUNT_CHORDBUTTONS];
		KToggleAction *crossDrumBu_;
		KToggleAction *cross2DrumBu;
		KToggleAction *crossCricDrumBu_;
		KToggleAction *rectDrumBu_;
		KToggleAction *triaDrumBu_;
		NInfoTable *about_;
		NDbufferWidget *notePart_; //main view of the score
		KAction *lilyPort_;
		int voiceNr_;
/*------------------------------ Dialogs ------------------------------------------------*/

		QDialog *channelDialog_;
		QListBox *channelList_;
		QPushButton *channelOkButton_;
		QPushButton *channelCancButton_;

		listFrm *listDialog_;
		volumeFrm *volChangeDialog_;
		smallestRestFrm *cleanUpRestsDialog_;
		ChordSelector *chordDialog_;

		scaleFrm *scaleFrm_;

		propFrm *genPropDialog_;

		lyricsFrm *lyricsDialog_;
/*
		staffelFrm *timesigDialog_;
*/
		timesigDiaFrm *timesigDialog_;
		staffFrm *multistaffDialog_;
		staffelFrm *clefDialog_;
		staffPropFrm *staffPropFrm_;
//		voiceDiaFrm *voiceDialog_; //deprecated
		tupletDialogImpl *tupletDialog_;

		QDialog *keyDialog_;
		QListBox *keyList_;
		QPushButton *keyOkButton_;
		QPushButton *keyCancButton_;
		NKeyOffs *offs_list_[7];
		QLabel *crosslabel_;
		QLabel *flatlabel_;
		QLabel *naturlabel_;
		QFrame *toolContainer_;
		QTabWidget *tabWid_;
		scorFrm *scoreInfoWin_;
		bool inPart_;
/*-------------------------- preliminary Symbols built durind dialog -----------------*/
		NKeySig *tmpKeysig_;
		NTimeSig *tmpTimeSig_;
		NMusElement *tmpElem_;
		NChordDiagram *tmpChordDiagram_;
		NMusElement *selectedElemForChord_;
		int selectedSign_;
/* ------------------------- initialization ------------------------------------------*/
		void createButtons(QWidget *parent);

/*----------------------------- internal reaction on resize --------------------------*/
		void setScrollableNotePage();
		void preparePixmaps();
		int width_, height_;
		int lastXpos_, lastYHeight_, oldLastXpos_;
		int nettoWidth_, nettoHeight_;
		int paperWidth_, paperHeight_;
		int paperScrollWidth_, paperScrollHeight_;
                int leftx_, topy_, boty_;
/*----------------------------- update of buttons due to selection ------------------*/
		void setButton(int nr);
		void updateInterface(property_type properties, int length);
		void playButtonReset();
/*-----------------------------(re-)storing ----------------------------------------*/
		void writeStaffs(const char *fname);
		bool readStaffs(const char *fname);
		QString actualFname_;
		NFileHandler *fhandler_;
		MusicXMLParser *musicxmlFileReader_;
		NLilyExport *lilyexport_;
		exportFrm *exportDialog_;
		saveParametersFrm *saveParametersDialog_;
		void exportManager( int type );

/*------------------------------ TSE3 -----------------------------------------------*/
#ifdef WITH_TSE3
		NTSE3Handler *tse3Handler_;
#endif
/*---------------------------- positioning ------------------------------------------*/
		void xscrollDuringReplay(int val);
		void computeMidiTimes(bool insertBars, bool doAutoBeam = false);
		QPtrList<NMidiEventStr> currentEvents_, nextEvents_;
		QPtrList<NMidiEventStr> *nextToPlay_, *nextToSearch_;
		QScrollBar  *scrollx_, *scrolly_;
		int lastBarNr_;
/*------------------------------- selection ---------------------------------------*/
		bool editMode_;
		property_type props_before_edit_mode_; /* selected buttons before going to edit mode */
		int length_before_edit_mode_; /* selected note/rest length button before going to edit mode */
		int x0_, y0_, x1_;
		int xori_;
		QRect   selRect_;
		int checkAllStaffsForNoteInsertion(const int line, const QPoint p, property_type *properties, bool *playable, bool *delete_elem, bool *insert_new_note);
		bool checkStaffIntersection(const QPoint p);
		QPtrList<NVoice> voiceList_;
		QPtrList<NStaff> staffList_;
		NVoice *currentVoice_;
		NStaff *currentStaff_;
		void nextElement();
		void prevElement();
		QTimer autoscrollTimer_;
		int help_x0_, help_x1_, help_y_, num_help_lines_;
		int dummy_note_x_, dummy_note_y_;
		QSlider *trillLength_;
		QFrame *trillLengthBase_;
		QSlider *vaLength_;
		QFrame *vaLengthBase_;
		QCheckBox *trillEnabled_;
		QFrame *dynamicBase_;
		QSlider *dynamicPos_;
		QCheckBox *dynamicDisable_;
		QCheckBox *dynamicAlignment_;
		QFrame *vaBase_;
		QCheckBox *vaDisable_;
	private slots:    
		void autoscroll();
		void undo();
		void redo();
/*------------------------------ TSE3 -----------------------------------------------*/
		void createTSE3();
		void playSong();
		void writeTSE3();
		void readTSE3();
		void TSE3MidiOut();
		bool TSE3MidiIn();
		bool TSE3toScore();
		void TSE3ParttoScore();
		void TSE3Filter();
		void completeRecording(bool);
		void completeTSE3toScore(bool ok);
	private:
/*----------------------------- modification -------------------------------------*/
		void moveUp();
		void moveDown();
		void moveSemiToneUp();
		void moveSemiToneDown();
		void moveOctaveUp();
		void moveOctaveDown();
		void deleteElem(bool backspace);
		void deleteBlock();
		void selectWholeStaff();
		int actualOffs_;
		int staffCount_;
		int keyLine_, keyOffs_;
		void forceAccent(property_type acc, bool val);
		void manageToolElement(bool becauseOfInsertion);
/*-------------------------------- layout --------------------------------------*/
		void renewStaffLayout();
		void appendStaffLayoutElem();
		inline void computeLastYHeight();
		QPixmap *layoutPixmap_;
/*-------------------------------- playing --------------------------------------*/
		int contextWidth_;
		QRect contextRec_;
		int myTime_;
		int tempofactor_;
		int changePlayMethodItemId_;
		int notesToPlay_;
		int turnOverOffset_;
		bool playing_;
		NTempoTrack SortedTempoSigs_;
		bool firstNoteActive_; // avoid overwriting first red colored note
		bool playStop_; //stop playing
		struct timeval nextPlayTime_;
		static void add_time(struct timeval *res, struct timeval *now, long msecs);
		unsigned long sub_time(struct timeval *future, struct timeval *now);

		//  wether the score has been modified since it was last saved
		bool editiones_;

		NStaff *nextStaffElemToBePainted_;
		int nextStaffNr_;
		bool nextStaffIsFirstStaff_;
		int newLeft_, newRight_;
		void paintNextStaff();
		QTimer timer_;
#ifdef WITH_TSE3
		QTimer midiInTimer_;
#endif
		double tempo_;
		QPtrList<NMidiEventStr> stopList_;
		void cleanupSelections();
/*-------------------------------- printing -------------------------------*/
#ifdef WITH_DIRECT_PRINTING
                IntPrinter *printer_;
                QString previewFile_;
#endif
/*-------------------------------- static dadabase -------------------------------*/
		static const char *keySigTab_[15];
};

#ifdef WITH_DIRECT_PRINTING

// Jorge Windmeisser Oliver

#include <kdeprint/kprintdialogpage.h>
#include <kprinter.h>
#include <unistd.h>
//#include <qscrollview.h>
 class PrintExportDialogPage : public KPrintDialogPage
 {
 public:
   PrintExportDialogPage( QWidget *tab, QWidget *parent = 0, const char *name = 0 );
   ~PrintExportDialogPage();

   void setTabTitle( QString title );

   //reimplement virtual functions
   void getOptions( QMap<QString,QString>& opts, bool incldef = false );
   void setOptions( const QMap<QString,QString>& opts );
   bool isValid( QString& msg );
 };

class IntPrinter : public KPrinter
 {
 public:
   IntPrinter(QWidget *exportParent);
   ~IntPrinter();
 void doPreparePrinting() { preparePrinting(); }
 QWidget *createExportForm(QString dialogTitle, exportFormat_T format);
 protected:
  PrintExportDialogPage *formatExport_;
  KPrintDialogPage      *pageExport_;
  QWidget               *form_;
 };

#endif /* WITH_DIRECT_PRINTING */

#endif // MYFRAME_WIDGET_H
