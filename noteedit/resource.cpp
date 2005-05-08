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

#include "config.h"
#if GCC_MAJ_VERS > 2
#include <iostream>
#else
#include <istream.h>
#endif
#include <stdio.h>
#include <string.h>
#include <qbitmap.h>
#include <kconfig.h>
#include <kdialogbase.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <qcheckbox.h>
#include <qlayout.h>
#include <qmessagebox.h>
#include <qvbox.h>
#include <qvgroupbox.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <kstddirs.h>
#include <kprogress.h>
#include <qfile.h>
#include <qmultilineedit.h>
#include <qlineedit.h>
#include <qcursor.h>
#if QT_VERSION >= 300
#include <qpen.h>
#include <qstyle.h>
#endif
#include "resource.h"
#include "configuredefaultvalues.h"
#include "midimapper.h"
#include "zoomselection.h"
#include "muselement.h"
#include "mainframewidget.h"
#include "keysig.h"
#include "clef.h"
#include "uiconnect.h"
#include "lilytest.h"
#define HELP_LINE_COLOR (20, 250, 20)
#define DUMMY_NOTE_COLOR (100, 100, 100)
#define DUMMY_NOTE_WIDTH 2

using namespace std;

#define BEAM_LINE_WIDTH 2

char *NResource::volume[] =
{I18N_NOOP("ppp"), I18N_NOOP("pp"), I18N_NOOP("p"),  I18N_NOOP("mp"),
 I18N_NOOP("mf"),  I18N_NOOP("f"),  I18N_NOOP("ff"), I18N_NOOP("fff")
};

char *NResource::noteVal[] =
{I18N_NOOP("full"),         I18N_NOOP("dotted half"), I18N_NOOP("half"),
 I18N_NOOP("dotted quarter"), I18N_NOOP("quarter"),   I18N_NOOP("dotted 8th"),
 I18N_NOOP("8th"),          I18N_NOOP("dotted 16th"), I18N_NOOP("16th"),
 I18N_NOOP("dotted 32th"),  I18N_NOOP("32th"),        I18N_NOOP("64th")
};

char *NResource::tripletVal[] =
{I18N_NOOP("half"),
 I18N_NOOP("quarter"),
 I18N_NOOP("8th"),
 I18N_NOOP("16th")
};

char *NResource::instrTab[128] =
{I18N_NOOP("Piano 1"),          I18N_NOOP("Piano 2"),
 I18N_NOOP("Piano 3"),          I18N_NOOP("Honky-tonk"),
 I18N_NOOP("E.Piano 1"),        I18N_NOOP("E.Piano 2"),
 I18N_NOOP("Harpsichord"),      I18N_NOOP("Clavinet"),
 I18N_NOOP("Celesta"),          I18N_NOOP("Glockenspiel"),
 I18N_NOOP("Music Box"),        I18N_NOOP("Vibraphone"),
 I18N_NOOP("Marimba"),          I18N_NOOP("Xylophone"),
 I18N_NOOP("Tubular Bells"),    I18N_NOOP("Dulcimer"),
 I18N_NOOP("Organ 1"),          I18N_NOOP("Organ 2"),
 I18N_NOOP("Organ 3"),          I18N_NOOP("Church Organ"),
 I18N_NOOP("Reed Organ"),       I18N_NOOP("Accordion"),
 I18N_NOOP("Harmonica"),        I18N_NOOP("Bandoneon"),
 I18N_NOOP("Nylon Guitar"),     I18N_NOOP("Steel Guitar"),
 I18N_NOOP("Jazz Guitar"),      I18N_NOOP("Clean Guitar"),
 I18N_NOOP("Guitar Mutes"),     I18N_NOOP("Overdrive Guitar"), 
 I18N_NOOP("Guitar Harmonics"), I18N_NOOP("Guitar Harmonics"),
 I18N_NOOP("Acoustic Bass"),    I18N_NOOP("Fingered Bass"),
 I18N_NOOP("Picked Bass"),      I18N_NOOP("Fretless Bass"),
 I18N_NOOP("Slap Bass 1"),      I18N_NOOP("Slap Bass 2"),
 I18N_NOOP("Synth Bass 1"),     I18N_NOOP("Synth Bass 2"), 
 I18N_NOOP("Violin"),           I18N_NOOP("Viola"),
 I18N_NOOP("Cello"),            I18N_NOOP("Contrabass"),
 I18N_NOOP("Tremolo Strings"),  I18N_NOOP("Pizzicato"),
 I18N_NOOP("Harp"),             I18N_NOOP("Timpani"),
 I18N_NOOP("Strings"),          I18N_NOOP("Slow Strings"), 
 I18N_NOOP("Synth Strings 1"),  I18N_NOOP("Synth Strings 2"),
 I18N_NOOP("Choir Aahs"),       I18N_NOOP("Voice Oohs"),
 I18N_NOOP("Synth Vox"),        I18N_NOOP("Orchestra Hit"),
 I18N_NOOP("Trumpet"),          I18N_NOOP("Trombone"),
 I18N_NOOP("Tuba"),             I18N_NOOP("Mute Trumpet"), 
 I18N_NOOP("French Horns"),     I18N_NOOP("Brass"),
 I18N_NOOP("Synth Brass 1"),    I18N_NOOP("Synth Brass 2"),
 I18N_NOOP("Soprano Sax"),      I18N_NOOP("Alto Sax"),
 I18N_NOOP("Tenor Sax"),        I18N_NOOP("Baritone Sax"),
 I18N_NOOP("Oboe"),             I18N_NOOP("English Horn"), 
 I18N_NOOP("Bassoon"),          I18N_NOOP("Clarinet"),
 I18N_NOOP("Piccolo"),          I18N_NOOP("Flute"),
 I18N_NOOP("Recorder"),         I18N_NOOP("Pan Flute"),
 I18N_NOOP("Bottle Chiff"),     I18N_NOOP("Shakuhachi"),
 I18N_NOOP("Whistle"),          I18N_NOOP("Ocarina"),
 I18N_NOOP("Square Wave"),      I18N_NOOP("Saw Wave"),
 I18N_NOOP("Synth Calliope"),   I18N_NOOP("Chiffer Lead"),
 I18N_NOOP("Charang"),          I18N_NOOP("Solo Vox"),
 I18N_NOOP("5th Saw Wave"),     I18N_NOOP("Bass & Lead"),
 I18N_NOOP("Fantasia"),         I18N_NOOP("Warm Pad"),
 I18N_NOOP("Poly Synth"),       I18N_NOOP("Space Voice"),
 I18N_NOOP("Bowed Glass"),      I18N_NOOP("Metal Pad"),
 I18N_NOOP("Halo Pad"),         I18N_NOOP("Sweep Pad"),
 I18N_NOOP("Ice Rain"),         I18N_NOOP("Soundtrack"),
 I18N_NOOP("Crystal"),          I18N_NOOP("Atmosphere"), 
 I18N_NOOP("Brightness"),       I18N_NOOP("Goblin"),
 I18N_NOOP("Echo Drops"),       I18N_NOOP("Star Theme"),
 I18N_NOOP("Sitar"),            I18N_NOOP("Banjo"),
 I18N_NOOP("Shamisen"),         I18N_NOOP("Koto"),
 I18N_NOOP("Kalimba"),          I18N_NOOP("Bagpipe"),
 I18N_NOOP("Fiddle"),           I18N_NOOP("Shenai"),
 I18N_NOOP("Tinker Bell"),      I18N_NOOP("Agogo"),
 I18N_NOOP("Steel Drum"),       I18N_NOOP("Wood Block"),
 I18N_NOOP("Taiko Drum"),       I18N_NOOP("Melodic Tom"),
 I18N_NOOP("Synth Drum"),       I18N_NOOP("Reverse Cymbal"),
 I18N_NOOP("Fret Noise"),       I18N_NOOP("Breath Noise"),
 I18N_NOOP("Seashore"),         I18N_NOOP("Bird"),
 I18N_NOOP("Telephone"),        I18N_NOOP("Helicopter"),
 I18N_NOOP("Applause"),         I18N_NOOP("Gun Shot")
};

NResource *NResource::nresourceobj_;
QBrush NResource::backgroundBrush_;
QBrush NResource::selectionBackgroundBrush_;
QBrush NResource::tempoSignatureBrush_;
QBrush NResource::selectedTempoSignatureBrush_;
QBrush NResource::blackBrush_;
QBrush NResource::redBrush_;
QBrush NResource::contextBrush_;
QPen NResource::noPen_;
QPen NResource::staffPen_;
QPen NResource::selectedStaffPen_;
QPen NResource::barPen_;
QPen NResource::selectedBarPen_;
QPen NResource::barNumberPen_;
QPen NResource::selectedBarNumberPen_;
QPen NResource::tempoSignaturePen_;
QPen NResource::selectedTempoSignaturePen_;
QPen NResource::volumeSignaturePen_;
QPen NResource::selectedVolumeSignaturePen_;
QPen NResource::programChangePen_;
QPen NResource::selectedProgramChangePen_;
QPen NResource::specialEndingPen_;
QPen NResource::selectedSpecialEndingPen_;
QPen NResource::staffNamePen_;
QPen NResource::selectedStaffNamePen_;
QPen NResource::lyricPen_;
QPen NResource::whiteWidePen_;
QPen NResource::blackWidePen_;
QPen NResource::redWidePen_;
QPen NResource::greyWidePen_;
QPen NResource::greenPen_;
QPen NResource::redPen_;
QPen NResource::whitePen_;
QPen NResource::helpLinePen_;
QPen NResource::dummyNotePen_;
QPen NResource::blackPen_;
QPen NResource::greyPen_;

QString NResource::resourceDir_;
QString NResource::fanfareFile_;

QPixmap  *NResource::r128Pixmap_;
QPixmap  *NResource::r128RedPixmap_;
QPixmap  *NResource::r128GreyPixmap_;
QPixmap  *NResource::r128MagPixmap_;
QPixmap  *NResource::r64Pixmap_;
QPixmap  *NResource::r64RedPixmap_;
QPixmap  *NResource::r64GreyPixmap_;
QPixmap  *NResource::r64MagPixmap_;
QPixmap  *NResource::r32Pixmap_;
QPixmap  *NResource::r32RedPixmap_;
QPixmap  *NResource::r32GreyPixmap_;
QPixmap  *NResource::r32MagPixmap_;
QPixmap  *NResource::r16Pixmap_;
QPixmap  *NResource::r16RedPixmap_;
QPixmap  *NResource::r16GreyPixmap_;
QPixmap  *NResource::r16MagPixmap_;
QPixmap  *NResource::r8Pixmap_;
QPixmap  *NResource::r8RedPixmap_;
QPixmap  *NResource::r8GreyPixmap_;
QPixmap  *NResource::r8MagPixmap_;
QPixmap  *NResource::rquarterPixmap_;
QPixmap  *NResource::rquarterRedPixmap_;
QPixmap  *NResource::rquarterGreyPixmap_;
QPixmap  *NResource::rquarterMagPixmap_;
QPixmap  *NResource::rfullPixmap_;
QPixmap  *NResource::rfullRedPixmap_;
QPixmap  *NResource::rfullGreyPixmap_;
QPixmap  *NResource::rfullMagPixmap_;
QPixmap  *NResource::brevePixmap_;
QPixmap  *NResource::breveRedPixmap_;
QPixmap  *NResource::breveGreyPixmap_;
QPixmap  *NResource::rhalfPixmap_;
QPixmap  *NResource::rhalfRedPixmap_;
QPixmap  *NResource::rhalfGreyPixmap_;
QPixmap  *NResource::rhalfMagPixmap_;

QPixmap  *NResource::repOpenPixmap_;
QPixmap  *NResource::repOpenRedPixMap_;
QPixmap  *NResource::repClosePixmap_;
QPixmap  *NResource::repCloseRedPixMap_;
QPixmap  *NResource::repOpenClosePixmap_;
QPixmap  *NResource::repOpenCloseRedPixMap_;
QPixmap  *NResource::endBarPixmap_;
QPixmap  *NResource::endBarRedPixmap_;

QPixmap  *NResource::treblePixmap_;
QPixmap  *NResource::trebleRedPixmap_;
QPixmap  *NResource::treblepPixmap_;
QPixmap  *NResource::treblepRedPixmap_;
QPixmap  *NResource::treblemPixmap_;
QPixmap  *NResource::treblemRedPixmap_;
QPixmap  *NResource::bassPixmap_;
QPixmap  *NResource::bassRedPixmap_;
QPixmap  *NResource::basspPixmap_;
QPixmap  *NResource::basspRedPixmap_;
QPixmap  *NResource::bassmPixmap_;
QPixmap  *NResource::bassmRedPixmap_;

QPixmap  *NResource::altoPixmap_;
QPixmap  *NResource::altoRedPixmap_;
QPixmap  *NResource::altopPixmap_;
QPixmap  *NResource::altopRedPixmap_;
QPixmap  *NResource::altomPixmap_;
QPixmap  *NResource::altomRedPixmap_;

QPixmap  *NResource::drumClefPixmap_;
QPixmap  *NResource::drumClefRedPixmap_;

QPixmap  *NResource::drumBassClefPixmap_;
QPixmap  *NResource::drumBassClefRedPixmap_;

QPixmap  *NResource::segnoPixmap_;
QPixmap  *NResource::segnoRedPixmap_;
QPixmap  *NResource::codaPixmap_;
QPixmap  *NResource::codaRedPixmap_;
QPixmap  *NResource::dalSegnoAlCodaPixmap_;
QPixmap  *NResource::dalSegnoAlCodaRedPixmap_;
QString  NResource::dalSegno_ = "D.S.";
QString  NResource::dalSegnoAlFine_ = "D.S. al Fine";
QString  NResource::fine_ = "Fine";
QString  NResource::ritardando_ = "ritard.";
QString  NResource::accelerando_ = "accel.";

QFont    *NResource::textFont_;

QPixmap  *NResource::crossPixmap_;
QPixmap  *NResource::crossRedPixmap_;
QPixmap  *NResource::crossGreyPixmap_;
QPixmap  *NResource::dcrossPixmap_;
QPixmap  *NResource::dcrossRedPixmap_;
QPixmap  *NResource::dcrossGreyPixmap_;
QPixmap  *NResource::flatPixmap_;
QPixmap  *NResource::flatRedPixmap_;
QPixmap  *NResource::flatGreyPixmap_;
QPixmap  *NResource::dflatPixmap_;
QPixmap  *NResource::dflatRedPixmap_;
QPixmap  *NResource::dflatGreyPixmap_;
QPixmap  *NResource::naturPixmap_;
QPixmap  *NResource::naturRedPixmap_;
QPixmap  *NResource::naturGreyPixmap_;

int NResource::crossPixWidth_;
int NResource::dcrossPixWidth_;
int NResource::flatPixWidth_;
int NResource::dflatPixWidth_;
int NResource::naturPixWidth_;

QPixmap  *NResource::stopIcon_;
QPixmap  *NResource::tuplet2_;
QPixmap  *NResource::tuplet3_;
QPixmap  *NResource::tuplet4_;
QPixmap  *NResource::tuplet5_;
QPixmap  *NResource::tuplet6_;
QPixmap  *NResource::tuplet7_;
QPixmap  *NResource::tuplet8_;
QPixmap  *NResource::tuplet9_;
QPixmap  *NResource::tuplet10_;

QPixmap  *NResource::crossIcon_;
QPixmap  *NResource::flatIcon_;
QPixmap  *NResource::naturIcon_;

QPixmap  *NResource::time_24Icon_;
QPixmap  *NResource::time_44Icon_;
QPixmap  *NResource::time_34Icon_;
QPixmap  *NResource::time_38Icon_;
QPixmap  *NResource::time_68Icon_;

QPixmap  *NResource::fullPixmap_;
QPixmap  *NResource::fullRedPixmap_;
QPixmap  *NResource::fullGreyPixmap_;
QPixmap  *NResource::flagPixmap_;
QPixmap  *NResource::tinyFlagPixmap_;
int	  NResource::flagPixmapWidth_;
int	  NResource::tinyFlagPixmapWidth_;
QPixmap  *NResource::flagRedPixmap_;
QPixmap  *NResource::flagGreyPixmap_;
QPixmap  *NResource::tinyFlagRedPixmap_;
QPixmap  *NResource::tinyFlagGreyPixmap_;
QPixmap  *NResource::flagDownPixmap_;
QPixmap  *NResource::flagDownRedPixmap_;
QPixmap  *NResource::flagDownGreyPixmap_;
int       NResource::flagDownPixmapHeight_;
QPixmap  *NResource::nbasePixmap_;
QPixmap  *NResource::nbaseRedPixmap_;
QPixmap  *NResource::nbaseGreyPixmap_;
QPixmap  *NResource::tinyBasePixmap_;
QPixmap  *NResource::tinyBaseRedPixmap_;
QPixmap  *NResource::tinyBaseGreyPixmap_;

QPixmap *NResource::sforzatoAbPixmap_;
QPixmap *NResource::sforzatoAbRedPixmap_;
QPixmap *NResource::sforzatoBePixmap_;
QPixmap *NResource::sforzatoBeRedPixmap_;
QPixmap *NResource::portatoPixmap_;
QPixmap *NResource::portatoRedPixmap_;
QPixmap *NResource::strong_pizzicatoAbPixmap_;
QPixmap *NResource::strong_pizzicatoAbRedPixmap_;
QPixmap *NResource::strong_pizzicatoBePixmap_;
QPixmap *NResource::strong_pizzicatoBeRedPixmap_;
QPixmap *NResource::sforzandoPixmap_;
QPixmap *NResource::sforzandoRedPixmap_;
QPixmap *NResource::fermateAbPixmap_;
QPixmap *NResource::fermateAbRedPixmap_;
QPixmap *NResource::fermateBePixmap_;
QPixmap *NResource::fermateBeRedPixmap_;
QPixmap *NResource::trillPixmap_;
QPixmap *NResource::trillRedPixmap_;
QPixmap *NResource::pedonPixmap_;
QPixmap *NResource::pedonRedPixmap_;
QPixmap *NResource::pedoffPixmap_;
QPixmap *NResource::pedoffRedPixmap_;
QPixmap *NResource::arpeggPixmap_;
int 	NResource::arpegPixmapHeight_;

QPixmap *NResource::perCrossPixmap_;
QPixmap *NResource::perCrossRedPixmap_;
QPixmap *NResource::perCrossGreyPixmap_;

QPixmap *NResource::perCross2Pixmap_;
QPixmap *NResource::perCross2RedPixmap_;
QPixmap *NResource::perCross2GreyPixmap_;

QPixmap *NResource::perCrossCircPixmap_;
QPixmap *NResource::perCrossCircRedPixmap_;
QPixmap *NResource::perCrossCircGreyPixmap_;

QPixmap *NResource::perRectPixmap_;
QPixmap *NResource::perRectRedPixmap_;
QPixmap *NResource::perRectGreyPixmap_;

QPixmap *NResource::perTrianPixmap_;
QPixmap *NResource::perTrianRedPixmap_;
QPixmap *NResource::perTrianGreyPixmap_;

QPixmap *NResource::musixwarn1_;
QPixmap *NResource::musixwarn2_;

int       NResource::nbasePixmapHeight_;
int       NResource::narrow_dist_;
int       NResource::tinyBasePixmapHeight_;
int       NResource::nbasePixmapWidth_;
int       NResource::nbasePixmapWidth2_;
int       NResource::tinyBasePixmapWidth_;
int       NResource::tinyBasePixmapWidth2_;
char 	  NResource::lyricsbuffer_[NUM_LYRICS][LYRICS_LINE_LENGTH];

expWrn   *NResource::exportWarning_;
QTimer    NResource::autoSaveTimer_;
QString   NResource::userpath_;
QString   NResource::musixScript_;

QRegExp NResource::germanAE_("Ä");
QRegExp NResource::germanOE_("Ö");
QRegExp NResource::germanUE_("Ü");
QRegExp NResource::germanae_("ä");
QRegExp NResource::germanoe_("ö");
QRegExp NResource::germanue_("ü");
QRegExp NResource::germanss_("ß");

// cursors

QCursor *NResource::cursor_128thnote_;
QCursor *NResource::cursor_breve_;
QCursor *NResource::cursor_fullnote_;
QCursor *NResource::cursor_sixteenthnote_;
QCursor *NResource::cursor_tinystroke_;
QCursor *NResource::cursor_32ndnote_;
QCursor *NResource::cursor_edit_;
QCursor *NResource::cursor_halfnote_;
QCursor *NResource::cursor_tinyeight_;
QCursor *NResource::cursor_64thnote_;
QCursor *NResource::cursor_eightnote_;
QCursor *NResource::cursor_quarternote_;
QCursor *NResource::cursor_tinysixteenth_;



// LilyPond

struct lily_properties NResource::lilyProperties_;



//  GENERAL

//  Autosave
bool NResource::autosaveEnable_;
unsigned int NResource::autosaveInterval_;
unsigned int NResource::turnOverPoint_;

//  Startup
bool NResource::startupLoadLastScore_;


bool NResource::showStaffNrs_ = true;
bool NResource::showStaffNames_ = true;
bool NResource::autoBeamInsertion_ = true;
int  NResource::defMidiPort_ = 0;
bool NResource::allowKeyboardInsert_ = false;
bool NResource::allowInsertEcho_ = true;
bool NResource::midiPortSet_ = false;
bool NResource::moveAccKeysig_ = true;
bool NResource::automaticBarInsertion_ = true;
bool NResource::useMidiPedal_ = true;
bool NResource::showAuxLines_ = true;
bool NResource::showContext_ = true;
bool NResource::showDrumToolbar_ = true;
int  NResource::underlength_ = DEFAULT_UNDERLENGTH;
int  NResource::overlength_ = DEFAULT_OVERLENGTH;
int  NResource::schedulerRequest_ = ALL_SCHEDULERS;
int  NResource::defZoomval_ = PREFERRED_ZOOM_VAL;
staff_props_str NResource::nullprops_;
QList<NMainWindow> NResource::windowList_;
NMidiMapper *NResource::mapper_;
int NResource::lastWindowX_ = 0;
int NResource::lastWindowY_ = 0;
NVoice *NResource::voiceWithSelectedRegion_ = 0;
NMainFrameWidget *NResource::windowWithSelectedRegion_ = 0;
bool NResource::isGrabbed_ = false;
int NResource::numOfMultiStaffs_ = 0;
KProgress *NResource::progress_;
NKeySig *NResource::nullKeySig_;
NClef *NResource::nullClef_;
bool NResource::commandLine_ = false;
int NResource::globalNoteNames_ = 0;
int NResource::globalMaj7_ = 0;
int NResource::globalFlatPlus_ = 0;

QString NResource::lyrics_[5];
#if KDE_VERSION < 220
int NResource::tipNo_;
QList<QString> NResource::theTips_;
#endif
bool NResource::dontShowMupWarnings_ = false;
bool *NResource::staffSelMute_ = 0;
bool *NResource::staffSelAutobar_ = 0;
bool *NResource::staffSelAutobeam_ = 0;
bool *NResource::staffSelMerge_ = 0;
bool *NResource::staffSelTrack_ = 0;
bool *NResource::staffSelExport_ = 0;
bool *NResource::staffSelMulti_ = 0;

/* orchestral bars (only visible part) */

int NResource::barCheckArray_[LENGTH_OF_BAR_CHECK_ARRAY];
int NResource::barCkeckIdx_;
int NResource::yPosOfBarEnd_;
int NResource::newYpos_;

#if KDE_VERSION >= 220
//  Temporary kludge to be used until the equivalent is implemented in
//  kdelibs. Copied from kdelibs and modified for use here         -Erik Sigra
void NResource::detailedWarningDontShowAgain
 (QWidget *parent, const QString &text, const QString &details,
  const QString &caption = QString::null,
  const QString &dontShowAgainName = QString::null, bool notify
 ) {
	KDialogBase *dialog = new KDialogBase       //  Arguments:
	  (kapp->makeStdCaption(i18n("Save")),      //  const QString &caption
	   KDialogBase::Yes | KDialogBase::Details, //  int buttonMask=Yes|No|Cancel
	   KDialogBase::Yes,                        //  ButtonCode defaultButton=Yes
	   KDialogBase::Yes,                        //  ButtonCode escapeButton=Cancel
     parent,                                  //  QWidget *parent=0
	   "SaveMupWarning",                        //  const char *name=0
	   true,                                    //  bool modal=true
	   false,                                   //  bool separator=false
     i18n("&OK")                              //  QString yes = QString::null
	  );

	QVBox *contents = new QVBox(dialog);
	contents->setSpacing(KDialog::spacingHint() * 2);
	contents->setMargin(KDialog::marginHint() * 2);

	QWidget *topContents = new QWidget(contents);
	QHBoxLayout * topLayout = new QHBoxLayout(topContents);
	topLayout->setSpacing(KDialog::spacingHint() * 2);

	topLayout->addStretch(1);
	QLabel *Image = new QLabel(topContents);
	Image->setPixmap
		(QMessageBox::standardIcon
			(QMessageBox::Warning        //  Can be NoIcon, Information, Warning or Critical.
#if QT_VERSION < 300
			 ,kapp->style().guiStyle() //static_cast<Qt::GUIStyle>(0) // FIXME
#endif
	 		)
		);
	topLayout->add(Image);
	QLabel *Text = new QLabel(text, topContents);
	Text->setMinimumSize(Text->sizeHint());
	topLayout->add(Text);
	topLayout->addStretch(1);

	QVGroupBox *detailsGroup = new QVGroupBox(i18n("Details:"), dialog);
	QLabel *detailsLabel = new QLabel(details, detailsGroup);
	detailsLabel->setMinimumSize(detailsLabel->sizeHint());

	QCheckBox *checkbox = new QCheckBox(i18n("Do not show this message again"), contents);

	dialog->setDetailsWidget(detailsGroup);
	dialog->setMainWidget(contents);

	dialog->exec();
	NResource::dontShowMupWarnings_ = checkbox->isChecked();
  delete dialog;
}
#endif

void NResource::printError(QString s) {
	QMessageBox *mb;
	if (commandLine_) {
		cerr << "Error " << s << endl;
	}
	else {
		mb = new QMessageBox("Error", s,  QMessageBox::Warning, QMessageBox::Ok, 0, 0);
		mb->exec();
		delete mb;
	}
}

void NResource::printWarning(QString s) {
	QMessageBox *mb;
	if (commandLine_) {
		cerr << "Error " << s << endl;
	}
	else {
		mb = new QMessageBox("Error", s,  QMessageBox::Warning, QMessageBox::Ok, 0, 0);
		mb->exec();
		delete mb;
	}
}

void NResource::abort( QString s, signed char no ) {
	cout << char( 7 ); // makes a beep
	cout.flush();	// now we get the beep before the box is opened.
	if (commandLine_) {
		cerr << "An internal error happened somewhere" << endl <<
			"The message is: " << s << " The error code is " << no << endl;
		exit(10);
	}
	else {
		KMessageBox::sorry
		(0,
		 i18n
			("<b>An internal error happened somewhere.<br><br>"
			 "</b>The message is:<br><i>\"%1\"<br>Error Code is: %2</i><br><br>"
			 "The program must be interrupted.<br>"
			).arg( s ).arg( no ),
		 kapp->makeStdCaption(i18n("Internal error"))
		);
		exit( 10 );
	}
}

NResource::NResource() {
	char e[128];
	NResource::nresourceobj_ = this;
	kapp->config()->setGroup("Autosave");
	autosaveEnable_ = kapp->config()->readBoolEntry("Enable", AUTOSAVE_ENABLE);
	autosaveInterval_ = kapp->config()->readUnsignedNumEntry
		("Interval", AUTOSAVE_INTERVAL);
	if
		(AUTOSAVE_INTERVAL_MIN > autosaveInterval_ ||
		 autosaveInterval_ > AUTOSAVE_INTERVAL_MAX
		)
		autosaveInterval_ = AUTOSAVE_INTERVAL;
	setAutosave(autosaveEnable_, autosaveInterval_);
	turnOverPoint_ = kapp->config()->readUnsignedNumEntry
		("TurnOver", AUTOSAVE_INTERVAL);
	if
		( turnOverPoint_ > TURN_OVER_MAX
		)
		turnOverPoint_ = TURN_OVER_MIN;

	kapp->config()->setGroup("Startup");
	startupLoadLastScore_ = kapp->config()->readBoolEntry
		("LoadLastScore", STARTUP_LOAD_LAST_SCORE);

	useMidiPedal_ = kapp->config()->readBoolEntry("UseMidiHold", MIDI_PEDAL);
	startupLoadLastScore_ = kapp->config()->readBoolEntry
		("LoadLastScore", STARTUP_LOAD_LAST_SCORE);

	kapp->config()->setGroup("Colors");
	backgroundBrush_ =	QBrush
		(kapp->config()->readColorEntry("Background", &COLORS_BACKGROUND));
	QColor temp = COLORS_SELECTION_BACKGROUND;
	selectionBackgroundBrush_ = QBrush
		(kapp->config()->readColorEntry
			(QString("SelectionBackground"), &temp)
		);
	contextBrush_ = QBrush
		(kapp->config()->readColorEntry
			(QString("ContextBrush"), &COLORS_CONTEXT_BRUSH)
		);
	staffPen_ = QPen
		(kapp->config()->readColorEntry("Staff", &COLORS_STAFF));
	selectedStaffPen_ = QPen
		(kapp->config()->readColorEntry("SelectedStaff", &COLORS_SELECTED_STAFF));
	barPen_ = QPen
		(kapp->config()->readColorEntry("Bar", &COLORS_BAR));
	selectedBarPen_ = QPen
		(kapp->config()->readColorEntry("SelectedBar", &COLORS_SELECTED_BAR));
	barNumberPen_ = QPen
		(kapp->config()->readColorEntry("BarNumber", &COLORS_BAR_NUMBER));
	selectedBarNumberPen_ = QPen
		(kapp->config()->readColorEntry
			("SelectedBarNumber", &COLORS_SELECTED_BAR_NUMBER)
		);
	tempoSignaturePen_ = QPen
		(kapp->config()->readColorEntry
			("TempoSignature", &COLORS_TEMPO_SIGNATURE)
		);
	selectedTempoSignaturePen_ = QPen
		(kapp->config()->readColorEntry
			("SelectedTempoSignature", &COLORS_SELECTED_TEMPO_SIGNATURE)
		);
	volumeSignaturePen_ = QPen
		(kapp->config()->readColorEntry
			("VolumeSignature", &COLORS_VOLUME_SIGNATURE)
		);
	selectedVolumeSignaturePen_ = QPen
		(kapp->config()->readColorEntry
			("SelectedVolumeSignature", &COLORS_SELECTED_VOLUME_SIGNATURE)
		);
	programChangePen_ = QPen
		(kapp->config()->readColorEntry("ProgramChange", &COLORS_PROGRAM_CHANGE));
	selectedProgramChangePen_ = QPen
		(kapp->config()->readColorEntry
			("SelectedProgramChange", &COLORS_SELECTED_PROGRAM_CHANGE)
		);
	specialEndingPen_ = QPen
		(kapp->config()->readColorEntry("SpecialEnding", &COLORS_SPECIAL_ENDING));
	selectedSpecialEndingPen_ = QPen
		(kapp->config()->readColorEntry
			("SelectedSpecialEnding", &COLORS_SELECTED_SPECIAL_ENDING)
		);
	staffNamePen_ = QPen
		(kapp->config()->readColorEntry("StaffName", &COLORS_STAFF_NAME));
	selectedStaffNamePen_ = QPen
		(kapp->config()->readColorEntry
			("SelectedStaffName", &COLORS_SELECTED_STAFF_NAME)
		);
	lyricPen_ = QPen
		(kapp->config()->readColorEntry(QString("Lyric"), &COLORS_LYRIC));

	//  Others:
	blackBrush_ = QBrush(Qt::black);
	redBrush_   = QBrush(Qt::red);
	noPen_        = QPen(Qt::NoPen);
	redWidePen_   = QPen(Qt::red);
	blackWidePen_ = QPen(Qt::black);
	greyWidePen_  = QPen(QColor(128, 128, 128));
	redWidePen_.setWidth(BEAM_LINE_WIDTH);
	blackWidePen_.setWidth(BEAM_LINE_WIDTH);
	greyWidePen_.setWidth(BEAM_LINE_WIDTH);
	greenPen_ = QPen(Qt::green);
	redPen_   = QPen(Qt::red);
	whitePen_ = QPen(Qt::white);
	blackPen_ = QPen(Qt::black);
	greyPen_  = QPen(QColor(128, 128, 128));
	helpLinePen_ = QPen(QColor HELP_LINE_COLOR );

	dummyNotePen_ = QPen(QColor DUMMY_NOTE_COLOR, DUMMY_NOTE_WIDTH );

	exportWarning_ = new expWrn( 0 );

	kapp->config()->setGroup("View");
	showStaffNrs_ = kapp->config()->readBoolEntry("ShowBarNumbers", true);
	showStaffNames_ = kapp->config()->readBoolEntry("ShowStaffNames", true);
	showAuxLines_ = kapp->config()->readBoolEntry("ShowAuxLines", true);
	showContext_ = kapp->config()->readBoolEntry("ShowStaffContext", true);
	showDrumToolbar_ = kapp->config()->readBoolEntry("ShowDrumToolbar", true);
	defZoomval_ = NZoomSelection::chooseZoomVal
	  (kapp->config()->readNumEntry
	    (QString("DefaultZoom"),
	     NZoomSelection::index2ZoomVal(PREFERRED_ZOOM_VAL)
	    )
	  );

	kapp->config()->setGroup("Startup");
	startupLoadLastScore_ =
		kapp->config()->readBoolEntry("LoadLastScore", STARTUP_LOAD_LAST_SCORE);

	kapp->config()->setGroup("Data");
	autoBeamInsertion_ = kapp->config()->readBoolEntry
		("AllowAutoBeaming", EDITING_ALLOW_AUTO_BEAMING);
	allowKeyboardInsert_ = kapp->config()->readBoolEntry
		("AllowKeyboardInsert", EDITING_ALLOW_KEYBOARD_INSERT);
	allowInsertEcho_ = kapp->config()->readBoolEntry
		("AllowInsertEcho", EDITING_INSERT_ECHO);
	moveAccKeysig_ = kapp->config()->readBoolEntry
		("MoveAccordingKeysig", EDITING_MOVE_ACCORDING_KEYSIG);
	automaticBarInsertion_ = kapp->config()->readBoolEntry
		("AutomaticBarInsertion", EDITING_AUTOMATIC_BAR_INSERTION);
	underlength_ = kapp->config()->readNumEntry
		("DefaultUnderlength", DEFAULT_UNDERLENGTH);
	overlength_ = kapp->config()->readNumEntry
		("DefaultOverlength", DEFAULT_OVERLENGTH);

	kapp->config()->setGroup("Sound");
	schedulerRequest_ =
	  (kapp->config()->readBoolEntry("AllowAlsaScheduler", SEQUENCERS_ALSA) ? ALSA_SCHEDULER_REQUESTED : 0) |
	  (kapp->config()->readBoolEntry("AllowOSSScheduler", SEQUENCERS_OSS) ? OSS_SCHEDULER_REQUESTED : 0);
	midiPortSet_ = kapp->config()->hasKey("DefaultMIDIPort");
	defMidiPort_ = kapp->config()->readNumEntry("DefaultMIDIPort", DEFAULT_MIDI_PORT);

	kapp->config()->setGroup("Chordnames");
	globalNoteNames_ = kapp->config()->readNumEntry ("DefaultNoteNames", DEFAULT_CHORD_NAME_SET);
	globalMaj7_ = kapp->config()->readNumEntry ("DefaultDom7Id", DEFAULT_DOM7_ID);
	globalFlatPlus_ = kapp->config()->readNumEntry ("DefaultFlatPlus", DEFAULT_ALTERATION_SIGN);
	

	kapp->config()->setGroup("General");
	dontShowMupWarnings_ = kapp->config()->readBoolEntry("NoMupWarnings");
	musixScript_ = kapp->config()->readEntry("MusixScript");
#if KDE_VERSION < 220
	tipNo_ = kapp->config()->readNumEntry("TipNo");
#endif


	resourceDir_ = locate( "data", "noteedit/resources/" );
	if (resourceDir_.isEmpty()) {
		cerr << "Can't find apps/noteedit/resources. Check your installation and $KDEDIR, if set" << endl;
		exit(10);
	}
	fanfareFile_ = locate("data", "noteedit/resources/fanfare.mp3");
	windowList_.setAutoDelete(false);
	QString fname = resourceDir_ + "nbase.ppm";
	if (access(QFile::encodeName(fname), F_OK)) {
		cerr << "Can't find " << fname << " under " << resourceDir_ << "! Check your installation." << endl;
		exit(10);
	}
	if (access(QFile::encodeName(fname), R_OK)) {
		cerr << "I have no read permissions for " << fname <<" ! Please check!" << endl;
		exit(10);
	}

	nullprops_.base = 0;
	nullprops_.lyricsdist = 0;
	nullprops_.is_actual = false;
	nullprops_.actual_keysig = 0;

	nullKeySig_ = new NKeySig(0, &nullprops_);
	nullClef_ = new NClef(0, &nullprops_);
	QString s = resourceDir_;
	progress_    = new KProgress;
	progress_->setGeometry(40, 40, 200, 20);
	loadPixmaps(&fullPixmap_, &fullRedPixmap_, "full");
	loadAlternativePixmap(&fullGreyPixmap_, "full", "_grey");
	loadPixmaps(&flagPixmap_, &flagRedPixmap_, "flag");
	loadAlternativePixmap(&flagGreyPixmap_, "flag", "_grey");
	loadPixmaps(&tinyFlagPixmap_, &tinyFlagRedPixmap_, "tinyflag");
	loadAlternativePixmap(&tinyFlagGreyPixmap_, "tinyflag", "_grey");
	loadPixmaps(&flagDownPixmap_, &flagDownRedPixmap_, "flagdown");
	loadAlternativePixmap(&flagDownGreyPixmap_, "flagdown", "_grey");
	loadPixmaps(&nbasePixmap_, &nbaseRedPixmap_, "nbase");
	loadAlternativePixmap(&nbaseGreyPixmap_, "nbase", "_grey");
	loadPixmaps(&tinyBasePixmap_, &tinyBaseRedPixmap_, "tinybase");
	loadAlternativePixmap(&tinyBaseGreyPixmap_, "tinybase", "_grey");
	loadPixmaps(&stopIcon_, 0, "stop_icon");
	loadPixmaps(&tuplet2_, 0, "tuplet2");
	loadPixmaps(&tuplet3_, 0, "tuplet3");
	loadPixmaps(&tuplet4_, 0, "tuplet4");
	loadPixmaps(&tuplet5_, 0, "tuplet5");
	loadPixmaps(&tuplet6_, 0, "tuplet6");
	loadPixmaps(&tuplet7_, 0, "tuplet7");
	loadPixmaps(&tuplet8_, 0, "tuplet8");
	loadPixmaps(&tuplet9_, 0, "tuplet9");
	loadPixmaps(&tuplet10_, 0, "tuplet10");
	loadPixmaps(&crossIcon_, 0, "cross_icon");
	loadPixmaps(&flatIcon_, 0, "flat_icon");
	loadPixmaps(&naturIcon_, 0, "natur_icon");
	loadPixmaps(&r128Pixmap_, &r128RedPixmap_, "r128");
	loadAlternativePixmap(&r128GreyPixmap_, "r128", "_grey");
	loadAlternativePixmap(&r128MagPixmap_, "r128", "_mag");
	loadPixmaps(&r64Pixmap_, &r64RedPixmap_, "r64");
	loadAlternativePixmap(&r64GreyPixmap_, "r64", "_grey");
	loadAlternativePixmap(&r64MagPixmap_, "r64", "_mag");
	loadPixmaps(&r32Pixmap_, &r32RedPixmap_, "r32");
	loadAlternativePixmap(&r32GreyPixmap_, "r32", "_grey");
	loadAlternativePixmap(&r32MagPixmap_, "r32", "_mag");
	loadPixmaps(&r16Pixmap_, &r16RedPixmap_, "r16");
	loadAlternativePixmap(&r16GreyPixmap_, "r16", "_grey");
	loadAlternativePixmap(&r16MagPixmap_, "r16", "_mag");
	loadPixmaps(&r8Pixmap_, &r8RedPixmap_, "r8");
	loadAlternativePixmap(&r8GreyPixmap_, "r8", "_grey");
	loadAlternativePixmap(&r8MagPixmap_, "r8", "_mag");
	loadPixmaps(&rquarterPixmap_, &rquarterRedPixmap_, "rquarter");
	loadAlternativePixmap(&rquarterGreyPixmap_, "rquarter", "_grey");
	loadAlternativePixmap(&rquarterMagPixmap_, "rquarter", "_mag");
	loadPixmaps(&rhalfPixmap_, &rhalfRedPixmap_, "rhalf");
	loadAlternativePixmap(&rhalfGreyPixmap_, "rhalf", "_grey");
	loadAlternativePixmap(&rhalfMagPixmap_, "rhalf", "_mag");
	loadPixmaps(&rfullPixmap_, &rfullRedPixmap_, "rfull");
	loadAlternativePixmap(&rfullGreyPixmap_, "rfull", "_grey");
	loadAlternativePixmap(&rfullMagPixmap_, "rfull", "_mag");
	loadPixmaps(&brevePixmap_, &breveRedPixmap_, "breve");
	loadAlternativePixmap(&breveGreyPixmap_, "breve", "_grey");
	loadPixmaps(&repOpenPixmap_, &repOpenRedPixMap_, "repOpen");
	loadPixmaps(&repClosePixmap_, &repCloseRedPixMap_, "repClose");
	loadPixmaps(&endBarPixmap_, &endBarRedPixmap_, "endbar");
	loadPixmaps(&repOpenClosePixmap_, &repOpenCloseRedPixMap_, "repOpenClose");
	loadPixmaps(&treblePixmap_, &trebleRedPixmap_, "treble");
	loadPixmaps(&treblepPixmap_, &treblepRedPixmap_, "treblep");
	loadPixmaps(&treblemPixmap_, &treblemRedPixmap_, "treblem");
	loadPixmaps(&bassPixmap_, &bassRedPixmap_, "bass");
	loadPixmaps(&basspPixmap_, &basspRedPixmap_, "bassp");
	loadPixmaps(&bassmPixmap_, &bassmRedPixmap_, "bassm");
	loadPixmaps(&altoPixmap_, &altoRedPixmap_, "alto");
	loadPixmaps(&altopPixmap_, &altopRedPixmap_, "altop");
	loadPixmaps(&altomPixmap_, &altomRedPixmap_, "altom");
	loadPixmaps(&drumClefPixmap_, &drumClefRedPixmap_, "drum_clef");
	loadPixmaps(&drumBassClefPixmap_, &drumBassClefRedPixmap_, "drum_bass_clef");
	loadPixmaps(&segnoPixmap_, &segnoRedPixmap_, "segno");
	loadPixmaps(&codaPixmap_, &codaRedPixmap_, "coda");
	loadPixmaps(&dalSegnoAlCodaPixmap_, &dalSegnoAlCodaRedPixmap_, "dalSegnoAlCoda");
	loadPixmaps(&crossPixmap_, &crossRedPixmap_, "cross");
	loadAlternativePixmap(&crossGreyPixmap_, "cross", "_grey");
	crossPixWidth_ = crossPixmap_->width();
	loadPixmaps(&dcrossPixmap_, &dcrossRedPixmap_, "dcross");
	loadAlternativePixmap(&dcrossGreyPixmap_, "dcross", "_grey");
	dcrossPixWidth_ = dcrossPixmap_->width();
	loadPixmaps(&flatPixmap_, &flatRedPixmap_, "flat");
	loadAlternativePixmap(&flatGreyPixmap_, "flat", "_grey");
	flatPixWidth_ = flatPixmap_->width();
	loadPixmaps(&dflatPixmap_, &dflatRedPixmap_, "dflat");
	loadAlternativePixmap(&dflatGreyPixmap_, "dflat", "_grey");
	dflatPixWidth_ = dflatPixmap_->width();
	loadPixmaps(&naturPixmap_, &naturRedPixmap_, "natur");
	loadAlternativePixmap(&naturGreyPixmap_, "natur", "_grey");
	loadPixmaps(&sforzatoAbPixmap_, &sforzatoAbRedPixmap_, "sforzato_ab");
	loadPixmaps(&sforzatoBePixmap_, &sforzatoBeRedPixmap_, "sforzato_be");
	loadPixmaps(&portatoPixmap_, &portatoRedPixmap_, "portato");
	loadPixmaps(&strong_pizzicatoAbPixmap_, &strong_pizzicatoAbRedPixmap_, "strong_pizzicato_ab");
	loadPixmaps(&strong_pizzicatoBePixmap_, &strong_pizzicatoBeRedPixmap_, "strong_pizzicato_be");
	loadPixmaps(&sforzandoPixmap_, &sforzandoRedPixmap_, "sforzando");
	loadPixmaps(&fermateAbPixmap_, &fermateAbRedPixmap_, "fermate_ab");
	loadPixmaps(&fermateBePixmap_, &fermateBeRedPixmap_, "fermate_be");
	loadPixmaps(&trillPixmap_, &pedoffRedPixmap_, "trill");
	loadPixmaps(&pedonPixmap_, &pedonRedPixmap_, "pedon");
	loadPixmaps(&pedoffPixmap_, &pedoffRedPixmap_, "pedoff");
	loadPixmaps(&arpeggPixmap_, 0, "arpegg");
	arpegPixmapHeight_ = arpeggPixmap_->height();

	loadPixmaps(&perCrossPixmap_, &perCrossRedPixmap_, "per_cross");
	loadAlternativePixmap(&perCrossGreyPixmap_, "per_cross", "_grey");

	loadPixmaps(&perCross2Pixmap_, &perCross2RedPixmap_, "per_cross2");
	loadAlternativePixmap(&perCross2GreyPixmap_, "per_cross2", "_grey");

	loadPixmaps(&perCrossCircPixmap_, &perCrossCircRedPixmap_, "per_cross_circ");
	loadAlternativePixmap(&perCrossCircGreyPixmap_, "per_cross_circ", "_grey");

	loadPixmaps(&perRectPixmap_, &perRectRedPixmap_, "per_rect");
	loadAlternativePixmap(&perRectGreyPixmap_, "per_rect", "_grey");

	loadPixmaps(&perTrianPixmap_, &perTrianRedPixmap_, "per_triang");
	loadAlternativePixmap(&perTrianGreyPixmap_, "per_triang", "_grey");

	loadPixmaps(&time_24Icon_, 0, "time_24");
	loadPixmaps(&time_44Icon_, 0, "time_44");
	loadPixmaps(&time_34Icon_, 0, "time_34");
	loadPixmaps(&time_38Icon_, 0, "time_38");
	loadPixmaps(&time_68Icon_, 0, "time_68");

	cursor_128thnote_ = loadCursor("cursor_128thnote.xbm");
	cursor_breve_ = loadCursor("cursor_breve.xbm");
	cursor_fullnote_ = loadCursor("cursor_fullnote.xbm");
	cursor_sixteenthnote_ = loadCursor("cursor_sixteenthnote.xbm");
	cursor_tinystroke_ = loadCursor("cursor_tinystroke.xbm");
	cursor_32ndnote_ = loadCursor("cursor_32ndnote.xbm");
	cursor_edit_ = loadCursor("cursor_edit.xbm");
	cursor_halfnote_ = loadCursor("cursor_halfnote.xbm");
	cursor_tinyeight_ = loadCursor("cursor_tinyeight.xbm");
	cursor_64thnote_ = loadCursor("cursor_64thnote.xbm");
	cursor_eightnote_ = loadCursor("cursor_eightnote.xbm");
	cursor_quarternote_ = loadCursor("cursor_quarternote.xbm");
	cursor_tinysixteenth_ = loadCursor("cursor_tinysixteenth.xbm");

	naturPixWidth_ = naturPixmap_->width();
	nbasePixmapHeight_ = nbasePixmap_->height();
	nbasePixmapWidth2_ = nbasePixmap_->width() / 2;
	tinyBasePixmapHeight_ = tinyBasePixmap_->height();
	nbasePixmapWidth_ = nbasePixmap_->width();
	narrow_dist_ = (int) (1.1 * (double) nbasePixmapWidth_);
	tinyBasePixmapWidth_ = tinyBasePixmap_->width();
	tinyBasePixmapWidth2_ = tinyBasePixmap_->width() / 2;
	flagDownPixmapHeight_ = flagDownPixmap_->height();
	flagPixmapWidth_ = flagPixmap_->width();
	flagPixmapWidth_ = tinyFlagPixmap_->width();
//	textFont_ = new  QFont ("Times" , 36, QFont::Normal, false, QFont::ISO_8859_1 );  
	textFont_ = new  QFont ("Times" , 36, QFont::Bold, false );  

	// check current LilyPond version to decide about the export format.
	lilytest lt;
	lt.check();
}

void NResource::setAutosave(bool enable, int intervall) {
	autosaveEnable_ = enable;
	autosaveInterval_ = intervall;

	if (autosaveEnable_) {
		connect (&autoSaveTimer_, SIGNAL (timeout()), NResource::nresourceobj_, SLOT(autosave()));
		autoSaveTimer_.start(autosaveInterval_*1000*60);
	}
	else {
		disconnect (&autoSaveTimer_, SIGNAL (timeout()), NResource::nresourceobj_, SLOT(autosave()));
		autoSaveTimer_.stop();
	}
}


NResource::~NResource() {
	if (mapper_) {
		delete mapper_;
		mapper_ = 0;
	}

	//  GENERAL

	kapp->config()->setGroup("Autosave");
	kapp->config()->writeEntry(QString("Enable"), autosaveEnable_);
	kapp->config()->writeEntry(QString("Interval"), autosaveInterval_);
	kapp->config()->writeEntry(QString("TurnOver"), turnOverPoint_);

	kapp->config()->setGroup("Startup");
	kapp->config()->writeEntry(QString("LoadLastScore"), startupLoadLastScore_);


	//  COLORS

	kapp->config()->setGroup("Colors");
	kapp->config()->writeEntry("Background", backgroundBrush_.color());
	kapp->config()->writeEntry
		("SelectionBackground", selectionBackgroundBrush_.color());
	kapp->config()->writeEntry
		("ContextBrush", contextBrush_.color());
	kapp->config()->writeEntry("Staff", staffPen_.color());
	kapp->config()->writeEntry("SelectedStaff", selectedStaffPen_.color());
	kapp->config()->writeEntry("Bar", barPen_.color());
	kapp->config()->writeEntry("SelectedBar", selectedBarPen_.color());
	kapp->config()->writeEntry("BarNumber", barNumberPen_.color());
	kapp->config()->writeEntry
		("SelectedBarNumber", selectedBarNumberPen_.color());
	kapp->config()->writeEntry("TempoSignature", tempoSignaturePen_.color());
	kapp->config()->writeEntry
		("SelectedTempoSignature", selectedTempoSignaturePen_.color());
	kapp->config()->writeEntry("VolumeSignature", volumeSignaturePen_.color());
	kapp->config()->writeEntry
		("SelectedVolumeSignature", selectedVolumeSignaturePen_.color());
	kapp->config()->writeEntry("ProgramChange", programChangePen_.color());
	kapp->config()->writeEntry
		("SelectedProgramChange", selectedProgramChangePen_.color());
	kapp->config()->writeEntry("SpecialEnding", specialEndingPen_.color());
	kapp->config()->writeEntry
		("SelectedSpecialEnding", selectedSpecialEndingPen_.color());
	kapp->config()->writeEntry("StaffName", staffNamePen_.color());
	kapp->config()->writeEntry
		("SelectedStaffName", selectedStaffNamePen_.color());
	kapp->config()->writeEntry("Lyric", lyricPen_.color());

	kapp->config()->setGroup("View");
	kapp->config()->writeEntry("ShowBarNumbers", showStaffNrs_);
	kapp->config()->writeEntry("ShowStaffNames", showStaffNames_);
	kapp->config()->writeEntry("ShowAuxLines", showAuxLines_);
	kapp->config()->writeEntry("ShowStaffContext", showContext_);
	kapp->config()->writeEntry("ShowDrumToolbar", showDrumToolbar_);
	kapp->config()->writeEntry("DefaultZoom", NZoomSelection::index2ZoomVal(defZoomval_));
	//  NOTE: This writes the DEFAULT zoom level, not the last zoom level that the user used.

	kapp->config()->setGroup("Data");
	kapp->config()->writeEntry("AllowAutoBeaming", autoBeamInsertion_);
	kapp->config()->writeEntry("AllowKeyboardInsert", allowKeyboardInsert_);
	kapp->config()->writeEntry("AllowInsertEcho", allowInsertEcho_);
	kapp->config()->writeEntry("MoveAccordingKeysig", moveAccKeysig_);
	kapp->config()->writeEntry("AutomaticBarInsertion", automaticBarInsertion_);
	kapp->config()->writeEntry("DefaultUnderlength", underlength_);
	kapp->config()->writeEntry("DefaultOverlength", overlength_);


	//  SOUND

	kapp->config()->setGroup("Sound");
	kapp->config()->writeEntry
	  (QString("AllowAlsaScheduler"), static_cast<bool>(schedulerRequest_ & ALSA_SCHEDULER_REQUESTED));
	kapp->config()->writeEntry
	  (QString("AllowOSSScheduler"), static_cast<bool>(schedulerRequest_ & OSS_SCHEDULER_REQUESTED));
	kapp->config()->writeEntry(QString("DefaultMIDIPort"), defMidiPort_);

	kapp->config()->setGroup("General");
	kapp->config()->writeEntry(QString("NoMupWarnings"), dontShowMupWarnings_);
	if (musixScript_.isEmpty() || musixScript_.isNull()) {
		kapp->config()->writeEntry(QString("MusixScript"), "");
	}
	else {
		kapp->config()->writeEntry(QString("MusixScript"), musixScript_);
	}
#if KDE_VERSION < 220
	kapp->config()->writeEntry(QString("TipNo"), tipNo_);
#endif

	// CHORD NAMES

	kapp->config()->setGroup("Chordnames");
	kapp->config()->writeEntry (QString("DefaultNoteNames"), globalNoteNames_); 
	kapp->config()->writeEntry (QString("DefaultDom7Id"), globalMaj7_);
	kapp->config()->writeEntry (QString("DefaultFlatPlus"), globalFlatPlus_);
	
}

void NResource::autosave() {
	NMainWindow *main_win;
	int i;
	for (i = 0, main_win = NResource::windowList_.first(); main_win; main_win = NResource::windowList_.next(), i++) {
		main_win->mainFrameWidget()->autosave(i);
	}
}

QCursor *NResource::loadCursor(char *fname) {
	QBitmap shape;
	QCursor *cursor;
	QString s;
	char e[128];

	s = resourceDir_ + QString(fname);

	shape = QBitmap(s);
	if (shape.isNull()) {
		sprintf (e, "Error in loading image [%s]",s.ascii());
		abort(e);
	}
	cursor = new QCursor(shape, shape,  7, 7);
	return cursor;
}

bool NResource::loadPixmaps(QPixmap **black_pixmap, QPixmap **red_pixmap, QString fname) {
	QString s;
	QBitmap mask;
	s = resourceDir_;
	s += fname;
	s += QString(".ppm");
	(*black_pixmap)  = new QPixmap(s);
	if ((*black_pixmap)->isNull()) goto an_error;
	s = resourceDir_;
	s += fname;
	s += QString(".xbm");
	mask = QBitmap(s);
	if (mask.isNull())  goto an_error;
	(*black_pixmap)->setMask( mask );
	if (red_pixmap != 0) {
		s = resourceDir_;
		s += fname;
		s += QString("_red.ppm");
		(*red_pixmap) = new QPixmap(s);
		if ((*red_pixmap)->isNull()) goto an_error;
		(*red_pixmap)->setMask( mask );
	}
	return true;

// cf: added to save memory.	
	an_error:
	char *e = new char[27 + s.length()];
	sprintf (e, "Error in loading image [%s]",s.ascii());
	abort (e);
	return false;
}


bool NResource::loadAlternativePixmap(QPixmap **grey_pixmap, QString fname, QString suffix) {
	QString s;
	QBitmap mask;
	s = resourceDir_;
	s += fname;
	s += suffix;
	s += QString(".ppm");
	(*grey_pixmap)  = new QPixmap(s);
	if ((*grey_pixmap)->isNull()) goto an_grey_error;
	s = resourceDir_;
	s += fname;
	s += QString(".xbm");
	mask = QBitmap(s);
	if (mask.isNull())  goto an_grey_error;
	(*grey_pixmap)->setMask( mask );
	return true;

// cf: added to save memory.	
	an_grey_error:
	char *e = new char[27 + s.length()];
	sprintf (e, "Error in loading image [%s]",s.ascii());
	abort (e);
	return false;
}

void NResource::germanUmlautsToTeX(QString *str) {
	str->replace(germanae_, "\\\"a");
	str->replace(germanoe_, "\\\"o");
	str->replace(germanue_, "\\\"u");
	str->replace(germanAE_, "\\\"A");
	str->replace(germanOE_, "\\\"O");
	str->replace(germanUE_, "\\\"U");
	str->replace(germanss_, "\\ss{}");
}

	

#if KDE_VERSION < 220
void NResource::loadTips(QString fname) {
#define MAXLEN 1024
	QString s;
	QString currentTip;
	bool copyMode = false;
	bool newtip = false;
	int pos;
	unsigned int len;
	QRegExp HTMLStart( "<.*html.*>", false ); 
	QRegExp HTMLStop( "<.*/.*html.*>", false ); 
	//QRegExp WhiteAtStart( "^[ \t][ \t]*", false ); 
	QRegExp emptyLine("^[ \t\n]*$", false);

	if (fname.isNull() || fname.isEmpty()) {
		cerr << "cannot find tips" << endl;
		return;
	}
	QFile tipFile ( fname );
	if (!tipFile.open ( IO_ReadOnly ))  {
		 cerr << "cannot open " << fname << endl;
		return;
	}
	 while (tipFile.readLine ( s, MAXLEN ) != -1) {
		if (!copyMode) {
			if ((pos = s.find(HTMLStart)) != -1) {
				copyMode = true;
				if ((pos = s.find('>', pos)) == -1) {
					cerr << "error 1" << endl;
				}
				len = s.length();
				s = s.right(len - pos - 1);
				//s = s.replace(WhiteAtStart, "");
/*
				if (!s.isEmpty() && s.find(emptyLine) == -1) cout << s;
*/
				if (!s.isEmpty() && s.find(emptyLine) == -1)  
					currentTip += s;
			}
		}
		if (copyMode) {
			if ((pos = s.find(HTMLStop)) != -1) {
				copyMode = false;
				newtip = true;
				s = s.left(pos);
			}
			//s = s.replace(WhiteAtStart, "");
/*
			if (!s.isEmpty() && s.find(emptyLine) == -1) cout << s;
*/
			if (!s.isEmpty() && s.find(emptyLine) == -1) 
					currentTip += s;
			if (newtip) {
				newtip = false;
				theTips_.append(new QString(currentTip));
				currentTip.truncate(0);
			}
		}
	 }
	 tipFile.close();
}
#endif

int NResource::noteLength2Button_(int length) {
	int res = -1;
	switch (length) {
		case NOTE128_LENGTH      : res = 9; break;
		case NOTE64_LENGTH       : res = 8; break;
		case NOTE32_LENGTH       : res = 7; break;
		case NOTE16_LENGTH       : res = 6; break;
		case NOTE8_LENGTH        : res = 5; break;
		case QUARTER_LENGTH      : res = 4; break;
		case HALF_LENGTH         : res = 3; break;
		case WHOLE_LENGTH        : res = 2; break;
		case DOUBLE_WHOLE_LENGTH : res = 1; break;
	}
	return res;
}

QCursor * NResource::determineCursor(int length) {
	switch (length) {
		case NOTE128_LENGTH      : return(cursor_128thnote_);
		case NOTE64_LENGTH       : return(cursor_64thnote_);
		case NOTE32_LENGTH       : return(cursor_32ndnote_);
		case NOTE16_LENGTH       : return(cursor_tinysixteenth_);
		case NOTE8_LENGTH        : return(cursor_eightnote_);
		case QUARTER_LENGTH      : return(cursor_quarternote_);
		case HALF_LENGTH         : return(cursor_halfnote_);
		case WHOLE_LENGTH        : return(cursor_fullnote_);
		case DOUBLE_WHOLE_LENGTH : return(cursor_breve_);
	}
	return 0;
}

int NResource::button2Notelength_(int buNr) {
	int res = -1;
	switch (buNr) {
		case 9: res = NOTE128_LENGTH     ; break;
		case 8: res = NOTE64_LENGTH      ; break;
		case 7: res = NOTE32_LENGTH      ; break;
		case 6: res = NOTE16_LENGTH      ; break;
		case 5: res = NOTE8_LENGTH       ; break;
		case 4: res = QUARTER_LENGTH     ; break;
		case 3: res = HALF_LENGTH        ; break;
		case 2: res = WHOLE_LENGTH       ; break;
		case 1: res = DOUBLE_WHOLE_LENGTH; break;
	}
	return res;
}


void NResource::resetBarCkeckArray(int newYpos, bool clear) {
	memset(barCheckArray_+barCkeckIdx_, -1, (LENGTH_OF_BAR_CHECK_ARRAY-barCkeckIdx_)*sizeof(int));
	barCkeckIdx_ = 0;
	yPosOfBarEnd_ = clear ? -1 : newYpos_;
	newYpos_ = newYpos;
}

int NResource::yPosOfOrchestralBar(int bar_xpos) {
	if (barCkeckIdx_ >= LENGTH_OF_BAR_CHECK_ARRAY) return -1;

	if (barCheckArray_[barCkeckIdx_] != bar_xpos) {
		barCheckArray_[barCkeckIdx_++] = bar_xpos;
		return -1;
	}
	barCheckArray_[barCkeckIdx_++] = bar_xpos;
	return yPosOfBarEnd_;
}

#include "resource.moc"
