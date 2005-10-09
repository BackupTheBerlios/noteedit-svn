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
/*		Christian Fasshauer							*/
/*		mseacf@gmx.net								*/
/*											*/
/*											*/
/****************************************************************************************/
#include <unistd.h>

#include "config.h"
#if GCC_MAJ_VERS > 2
#include <fstream>
#else
#include <fstream.h>
#endif
#include <qlistbox.h>
#include <kfiledialog.h>
#include <qspinbox.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <qtabwidget.h>
#include <qslider.h>
#include <qlineedit.h>
#include <qmultilineedit.h>
#include <qfiledialog.h>
#include <qfile.h>
#include <qcheckbox.h>
#include <qtextview.h>
#include <qlcdnumber.h> 
#include <qradiobutton.h> 
#if QT_VERSION >= 300
#include <qlistview.h>
#endif
#include "uiconnect.h"
#include "mainframewidget.h"
#include "staff.h"
#include "tse3handler.h"
#include "musixtex.h"
#include "rest.h"
#include "notesel.h"
#include "midiexport.h"
#include "pmxexport.h"
#include "abcexport.h"
#include "filehandler.h"
#include "lilyexport.h"
#include "scaleedit_impl.h"
#include "sign.h"
#include "resource.h"
#include "outputbox.h"
#include "muselement.h"
#ifdef WITH_TSE3
#include "tse3handler.h"
#endif
#include "musicxmlexport.h"


/*-------------------------------- line sel ----------------------------------*/

lineSelWg::lineSelWg( QWidget *parent ) : lineSel( parent, 0, true ) {
	this->bo->setFocus();
}


void lineSelWg::slot_abort() {

    abort = true;
    this->close();
    
    }

void lineSelWg::slot_ok() {

    abort = false;
    this->close();

    }

int lineSelWg::getResult() {

    if( abort )
	return -1;    

    return( ListBox1->currentItem() );

    }

/*-------------------------------------------- exportForm ---------------------------------------*/

exportFrm::exportFrm(NMainFrameWidget *mainWidget,  QWidget *parent ) : 
	exportForm( parent, 0, false ) { // hint: false is only temporary at the modal position, it will be removed if the staff selection is reimplemented.
	mainWidget_ = mainWidget;
	this->be->setFocus();
	this->pmxMeasure->setAll(1, 16, 4);
	this->pmxNum->setAll(0, 100, 1);
	this->pmxSystem->setAll(1, 100, 1);
	if (NResource::musixScript_.isEmpty() || NResource::musixScript_.isNull()) {
		this->musixtexcmd->setText("");
	}
	else {
		this->musixtexcmd->setText(NResource::musixScript_);
	}
	
	/* LilyPond 2.6.x supports utf8 encoding. It should be set as default. */
	if (NResource::lilyProperties_.lilyVersion26)
		this->lilyOutputCoding->setCurrentItem(3);
		
	/* LilyPond <2.2 doesn't support standard page sizes yet */
	if (NResource::lilyProperties_.lilyProperties) {
		this->lilyCPage->setChecked(true);
		this->lilySPage->setEnabled(false);
		this->lilySPageSize->setEnabled(false);
		this->lilySLand->setEnabled(false);
	}
	
    staffDialog_ = new staffFrm( parent );
    
    }

void exportFrm::boot() {
	if( staffList_->isEmpty() ) {
		KMessageBox::sorry
			(0,
			 i18n("There is nothing to export."),
			 kapp->makeStdCaption(i18n("Export"))
			);
		return;
	}
  this->show();

}

exportFrm::~exportFrm() {

    delete staffDialog_;
    
    }

void exportFrm::closeIt() {

    this->close();
    
    }

void exportFrm::initialize( QPtrList<NStaff> *stafflist, QPtrList<NVoice> *voicelist, QString fname ) {

    staffList_ = stafflist;
    voiceList_ = voicelist;
    sourceFile_ = fname;

    }

void exportFrm::startExport() {
    if (this->card->currentPageIndex() == PARAM_PAGE) {
	this->close();
	return;
    }
		

    char *ext[] = { (char *)".mid", (char *)".tex", (char *)".abc", (char *)".pmx", (char *)".ly", (char *)".xml" };
    char *desc[] = { (char *)"MIDI", (char *)"MusiXTeX", (char *)"ABC", (char *)"PMX",  (char *)"LilyPond", (char *)"MusicXML" };


    if( card->currentPageIndex() == MUSIX_PAGE || card->currentPageIndex() == LILY_PAGE ) {
	if(! NResource::staffSelExport_ ) {
	    NResource::staffSelExport_ = new bool[staffList_->count()];
	    for(int unsigned i = 0; i < staffList_->count(); ++i )
		NResource::staffSelExport_[i] = true;
	    }
	bool is = false;
	for( int unsigned i = 0; i < staffList_->count(); ++i )
	    is += NResource::staffSelExport_[i];
	if (!is) {
	    KMessageBox::sorry
				(0,
				 i18n("No staff selected for export."),
				 kapp->makeStdCaption(i18n("Export"))
				);
	    return;
	    }
	}
	

    QString mask;
    QString myFile = sourceFile_;
    if( sourceFile_.isNull() ) 
	myFile.sprintf( "export%s", ext[card->currentPageIndex()] );
    else
	myFile.replace( sourceFile_.find( ".not", -4, true ), 4, ext[card->currentPageIndex()] );
    mask.sprintf( "*%s|%s file (*%s)\n*.*|All files (*.*)", ext[this->card->currentPageIndex()], desc[card->currentPageIndex()], ext[card->currentPageIndex()] );
    QString fileName = NMainFrameWidget::checkFileName(KFileDialog::getSaveFileName( myFile, mask, this), ext[this->card->currentPageIndex()] );

    NMusiXTeX mt;
    NMidiExport me;
    NPmxExport pe;
    NLilyExport le;
    NABCExport abc;
    NMusicXMLExport muxml;

    if( !fileName.isNull() ) {
        switch( this->card->currentPageIndex() ) {
    	case MIDI_PAGE:    
    		me.exportMidi( fileName, voiceList_, (char *) this->midiInfo->text().ascii());
			break;
	    case MUSIX_PAGE:  
			mt.exportStaffs( fileName, staffList_, this,  mainWidget_);
			KMessageBox::information(this, i18n("MusiXTeX export is now complete.\nWarning! The exported file is NOT a LaTeX file! Please use musixtex or pmx parser for compilation and not latex!"), kapp->makeStdCaption(i18n("???")));
			break;
	    case ABC_PAGE:
			abc.exportStaffs( fileName, staffList_, voiceList_->count(), this, mainWidget_ );
			break;
	    case MUSICXML_PAGE:
			muxml.exportStaffs( fileName, staffList_, voiceList_->count(), this, mainWidget_ );
			break;
	    case PMX_PAGE:
			pe.exportStaffs( fileName, staffList_, this, mainWidget_ );
			break;
	    case LILY_PAGE:
			if( !NResource::lilyProperties_.lilyAvailable ) {
	    	if (KMessageBox::warningContinueCancel
				(this,
				 i18n("Actually LilyPond is not supported by your system. Do you realy want to make such an export?"),
				 kapp->makeStdCaption(i18n("LilyPond export")),
				 i18n("&Export")
				)
				== KMessageBox::No
			  ) {
				delete [] NResource::staffSelExport_;
				NResource::staffSelExport_ = 0;
				return;
			}
			}
			le.exportStaffs( fileName, staffList_, this , mainWidget_);
			break;
	    }
	this->close();
	}
    if( NResource::staffSelExport_ ) {
	delete [] NResource::staffSelExport_;
	NResource::staffSelExport_ = 0;
	}
}

void exportFrm::texMeasures() {

    this->measureVal->setEnabled( this->texMeasures_->isChecked() );

    }

void exportFrm::lilyMeasures() {

    this->lilyMeasureVal->setEnabled( this->lilyMeasure->isChecked() );

    }

void exportFrm::musixStaffSig() {

    staffDialog_->boot( staffList_, STAFF_ID_EXPORT );

    }

void exportFrm::lilyStaffSig() {

    staffDialog_->boot( staffList_, STAFF_ID_EXPORT );

    }

void exportFrm::pmxStaffSig() {

    staffDialog_->boot( staffList_, STAFF_ID_EXPORT );

    }

void exportFrm::musixLandSlot() {

    if( this->texLandscape->isChecked() ) {
	this->texWidth->setValue( 250 );
	this->texHeight->setValue( 170 );
	this->texTop->setValue( -24 );
	this->texLeft->setValue( -10 );
	}
    else {
	this->texWidth->setValue( 170 );
	this->texHeight->setValue( 250 );
	this->texTop->setValue( -10 );
	this->texLeft->setValue( -10 );
	}

    }

void exportFrm::lilyLandSlot() {

    if( this->lilyCLand->isChecked() ) {
	this->lilyCWidth->setValue( 250 );
	this->lilyCHeight->setValue( 170 );
	}
    else {
	this->lilyCWidth->setValue( 170 );
	this->lilyCHeight->setValue( 250 );
	}

    }


void exportFrm::pmxLandSlot() {

    if( this->pmxLandscape->isChecked() ) {
	this->pmxWidth->setValue( 250 );
	this->pmxHeight->setValue( 170 );
	}
    else {
	this->pmxWidth->setValue( 170 );
	this->pmxHeight->setValue( 250 );
	}

    }

void exportFrm::paramLandSlot() {

    if( this->paramLand->isChecked() ) {
	this->pWidth->setValue( 250 );
	this->pHeight->setValue( 170 );
	}
    else {
	this->pWidth->setValue( 170 );
	this->pHeight->setValue( 250 );
	}


    }


int exportFrm::getSaveWidth() {
	QString s;
	bool ok;
	int width;

	s = pWidth->text();
	width = s.toInt(&ok);
	if (!ok) width = 213;
	return width;
}


int exportFrm::getSaveHeight() {
	QString s;
	bool ok;
	int height;

	s = pHeight->text();
	height = s.toInt(&ok);
	if (!ok) height = 275;
	return height;
}
bool exportFrm::paramsEnabled() {return paramEnable->isChecked();}
void exportFrm::setEnabled(bool ok) {paramEnable->setChecked(ok);}
bool exportFrm::withMeasureNums() {return paramMeasureNums->isChecked();}
void exportFrm::setWithMeasureNums(bool with) {paramMeasureNums->setChecked(with);}
void exportFrm::setSaveWidth(int width)  {pWidth->setValue(width);}
void exportFrm::setSaveHeight(int height) {pHeight->setValue(height);}

/*----------------------------------------- scaleForm --------------------------------------------*/

scaleFrm::scaleFrm( QWidget *parent ) : scaleForm( parent, 0, true ) {
	this->ok->setFocus();
	scal_ed->setAll(10, 300, 100);
}

void scaleFrm::transSlotOk() {
    
    succ_ = true;
    this->close();

    }

void scaleFrm::transSlotCancel() {

    succ_ = false;
    this->close();

    }

void scaleFrm::boot( QPtrList<NStaff> *stafflist, QScrollBar *scrollx_ ) {
    
#if QT_VERSION >= 300
    this->exec();
#else
    this->show();
#endif

    if( !succ_ )
	return;

    int xpos;
    if( ( xpos = stafflist->first()->getVoiceNr(0)->findPos( this->scal_ed->getValue() ) ) > 5 )
	xpos -= 5;
    scrollx_->setValue( xpos );
        
    }

bool scaleFrm::boot( main_props_str *main_props_, NStaff *currentStaff_, NVoice *currentVoice, NMusElement **tmpElem_, int subtype ) {

    NMusElement *el;

#if QT_VERSION >= 300
    this->exec();
#else
    this->show();
#endif
    if( !succ_ )
	return false;


    if (subtype == TEMPO_SIGNATURE) {
    	NSign *sign = new NSign( main_props_, currentStaff_->getStaffPropsAddr(), subtype );
    	sign->setTempo( this->scal_ed->getValue() );
	el = sign;
        *tmpElem_ = el;
    }
    else if (subtype == MULTIREST) {
	NRest *rest = new NRest(main_props_, currentStaff_->getStaffPropsAddr(), &(currentVoice->yRestOffs_),
		 MULTIREST, this->scal_ed->getValue());
	el = rest;
	*tmpElem_ = el;
    }
    else {
	return false;
    }
    return true;
}

int scaleFrm::boot() {
    
#if QT_VERSION >= 300
    this->exec();
#else
    this->show();
#endif
    
    if(! succ_ )
	return 0;

    return this->scal_ed->getValue();

    }
    
/*------------------------------------------ filterForm -----------------------------------------*/

filterFrm::filterFrm( NMainFrameWidget *parent, bool modal ) : filterForm( parent, 0, modal ) {
	averageSlot();
	this->filSnapDist->insertItem(i18n("Auto"));
	this->filSmTripletNote->insertItem(i18n("Auto"));
	this->filSmTripletNote->insertItem(i18n("None"));
	for (int i = 0; i < 12; ++i) {
		this->filSnapDist->insertItem(i18n(NResource::noteVal[i]));
	}
	for (int i = 0; i < 4; ++i) {
		this->filSmTripletNote->insertItem(i18n(NResource::tripletVal[i]));
	}
	this->filImpRe->setFocus();
	this->filVal1->setAll(0, 127, 0);
	this->filVal2->setAll(0, 127, 127);
	this->filVelSca->setAll(0, 200, 100);
	this->filVolDist->setAll(0, 127, 10);

    mainWidget_ = parent;
    connect( filTSE, SIGNAL( clicked() ), parent, SLOT( TSE3toScore() ) );
    connect( filTSEPart, SIGNAL( clicked() ), parent, SLOT( TSE3ParttoScore() ) );
    
    }

int filterFrm::item2length(QComboBox* box) {
#ifdef WITH_TSE3
        int item = box->currentItem();
        item--;
        if (item < 0) return -1;
        return NTSE3Handler::threwCase(item);
#else
	return 0;
#endif
    }


void filterFrm::averageSlot() {
	if (this->filAvVol->isChecked()) this->filL1->setText(i18n("Dynamic:"));
	else this->filL1->setText(i18n("Set min volume"));
}

void filterFrm::cancelSlot() {

    this->hide();

    }

void filterFrm::recSlot() {

    mainWidget_->importRecording();
    filImpRe->setDown( false );

    }

/*----------------------------------------- metronomForm ----------------------------------------*/

metronomFrm::metronomFrm( QWidget *parent, NTSE3Handler *caller, bool modal ) : metronomForm( parent, 0, modal ) {
	caller_ = caller;
	this->metDev->insertStringList(NResource::mapper_->deviceNameList);
	this->startButt->setFocus();
	this->metTempo->setAll(20, 220, 80);
	this->metBar->setAll(20, 120, 77);
	this->metBeat->setAll(20, 120, 76);
}

void metronomFrm::reactivate() {
#ifdef WITH_TSE3
   this->startButt->setEnabled( true );
#endif
}

void metronomFrm::startSlot() {
#ifdef WITH_TSE3
    this->startButt->setEnabled( false );
    this->caller_->doRecord();
#endif

    }

void metronomFrm::abortSlot() {

    this->hide();
    this->startButt->setEnabled( true );
    
    }

/*---------------------------------------- smallestRestForm -------------------------------------*/

smallestRestFrm::smallestRestFrm( NMainFrameWidget *parent ) : clRestForm( parent, 0, true ) {
	for (int i = 0; i < 12; ++i) {
		this->sel->insertItem(i18n(NResource::noteVal[i]));
	}
	this->btOk->setFocus();
}

void smallestRestFrm::okSlot() {

    this->close();
    succ_ = true;

    }
    
void smallestRestFrm::clSlot() {
    
    this->close();
    succ_ = false;
    this->sel->setCurrentItem( oldval_ );
    
    }

bool smallestRestFrm::boot() {
    
    oldval_ = this->sel->currentItem();
#if QT_VERSION >= 300
    this->exec();
#else
    this->show();
#endif
    return succ_;
    
    }

int smallestRestFrm::item2length() {
        int item = this->sel->currentItem();
    	int val = QUARTER_LENGTH;

        switch (item) {
                case  0: val = WHOLE_LENGTH; break;
                case  1: val = HALF_LENGTH / 2 * 3; break;
                case  2: val = HALF_LENGTH; break;
                case  3: val = QUARTER_LENGTH / 2 * 3; break;
                case  4: val = QUARTER_LENGTH; break;
                case  5: val = NOTE8_LENGTH/ 2 * 3; break;
                case  6: val = NOTE8_LENGTH; break;
                case  7: val = NOTE16_LENGTH / 2 * 3; break;
                case  8: val = NOTE16_LENGTH; break;
                case  9: val = NOTE32_LENGTH / 2 * 3; break;
                case 10: val = NOTE32_LENGTH; break;
                case 11: val = NOTE64_LENGTH; break;
        }

    return val;

}

/*----------------------------------- volumeForm ------------------------------------------------*/

volumeFrm::volumeFrm( QWidget *parent ) : volumeForm( parent, 0, true ) {
	for (int i = 0; i < 8; ++i) {
		this->sel->insertItem(i18n(NResource::volume[i]));
	}
	this->btOk->setFocus();
	this->scal_ed->setAll(0, 127, 100);
}

bool volumeFrm::boot() {

#if QT_VERSION >= 300
    this->exec();
#else
    this->show();
#endif
    return succ_;

    }

void volumeFrm::okSlot() {

    this->hide();
    succ_ = true;
    
    }
    
void volumeFrm::chSlot() {

    this->hide();
    succ_ = false;
    
    }

/*----------------------------------------- listForm --------------------------------------------*/

listFrm::listFrm( QWidget *parent ) : listForm( parent, 0, true ) {
	this->ob->setFocus();
}

bool listFrm::boot( int val, short int type, const QString & caption, const QString & title, QPtrList<NStaff> *staff ) {

    this->choice->clear();

    switch( type ) {
	case LIST_VOICE: {
	    for( int i = 0; i < 128; ++i )
		this->choice->insertItem(i18n("%1. %2").arg(i).arg(i18n(NResource::instrTab[i])));
	    } break;
	case LIST_MOVE_STAFF: {
	    NStaff *stf = staff->first();
	    for( int i = 1; stf; stf = staff->next(), i++ )
		this->choice->insertItem(i18n("Staff %1, %2, %3").arg(i).arg(i18n(NResource::instrTab[stf->getVoice()])).
								arg( stf->staffName_ == "" || stf->staffName_ == QString::null  ?
						        i18n("[unnamed]") : QString(stf->staffName_)));
	    } break;
	}

    this->l1->setText( title );
    this->setCaption( caption );
    this->choice->setCurrentItem( val );
#if QT_VERSION >= 300
    this->exec();
#else
    this->show();
#endif
    return succ_;

    }

void listFrm::chSlot() {
    
    this->hide();
    succ_ = false;
    
    }
    
void listFrm::okSlot() {

    this->hide();
    succ_ = true;
    
    }

/*------------------------------------ propForm -------------------------------------------------*/

propFrm::propFrm( QWidget *parent ) : propForm( parent, 0, true ) {
	this->ok->setFocus();
}

void propFrm::boot() {

    this->autoBeamInsertion->setChecked( NResource::autoBeamInsertion_ );

    this->insKeyb->setChecked( NResource::allowKeyboardInsert_ );

    this->accKeys->setChecked( NResource::moveAccKeysig_ );

		kapp->config()->setGroup("TipOfDay");
		this->showTip->setChecked(kapp->config()->readBoolEntry("RunOnStart", true));

    this->show();

    }

void propFrm::slok() {

		NResource::autoBeamInsertion_ = this->autoBeamInsertion->isChecked();

		NResource::allowKeyboardInsert_ = this->insKeyb->isChecked();

		NResource::moveAccKeysig_ = this->accKeys->isChecked();

		kapp->config()->setGroup("TipOfDay");
		kapp->config()->writeEntry("RunOnStart", showTip->isChecked());

		this->hide();

    }

void propFrm::slcl() {

    this->hide();
    
    }

/*------------------------------------ lyricsForm -----------------------------------------------*/

lyricsFrm::lyricsFrm( QWidget *parent ) : lyricsForm( parent, 0, true ) {
	this->bok->setFocus();
}

void lyricsFrm::boot() {

    this->initNo();
    prevLyr_ = -1;
    this->chngLyr();
    for( int i = 0; i < 5; ++i )
	oldField_[i] = NResource::lyrics_[i];
#if QT_VERSION >= 300
    this->exec();
#else
    this->show();
#endif

    }

void lyricsFrm::initNo() {

    int ci = this->no->currentItem();
    this->no->clear();
    for( int i = 1; i < 6; ++i )
	this->no->insertItem(i18n("Stanza no %1 %2").arg(i).arg( 
		    ( NResource::lyrics_[i - 1] == QString::null || 
		      NResource::lyrics_[i - 1] == "") ? i18n("[empty]") : QString::null));
    this->no->setCurrentItem( ci );

}

void lyricsFrm::slCl() {

	if (KMessageBox::warningYesNo
		(0,
		 i18n("Are you sure to clear the current stanza?"),
		 kapp->makeStdCaption(i18n("Clear stanza")),
		 i18n("C&lear"),
		 i18n("&Cancel")
	  )
	  == KMessageBox::No
		) return;
	
    this->txt->clear();
    NResource::lyrics_[this->no->currentItem()] = QString::null;

    }

/* confirm and close changes */	
void lyricsFrm::slOk() {
    this->chngLyr();
    this->close();
}

/* import lyrics from file */
void lyricsFrm::slOp() {

	QString fn = KFileDialog::getOpenFileName( QString::null, "*.txt|Text files (*.txt)\n*.*|All files (*.*)", this );
	char *nm = (char *) fn.ascii();
	char buf[1024];

	if (fn.isEmpty())
		KMessageBox::sorry
			(0,
			 i18n("File open aborted."),
			 kapp->makeStdCaption(i18n("Import lyrics"))
			);
	else {
		if( access( (char *) nm, R_OK ) ) {
	   	KMessageBox::sorry
				(0,
				 i18n("Unable to open this file."),
				 kapp->makeStdCaption(i18n("Import Lyrics"))
				);
			return;
		}

		fstream ts( (char *) nm, ios::in );
		this->txt->clear();
		while( !ts.eof() ) {
			ts.getline( buf, 1024 );
			this->txt->append( buf );
		}
		ts.close();
		this->repaint();
	}
}

/* new stanza is selected from drop-down menu */
void lyricsFrm::chngLyr() {
	if( prevLyr_ > -1 ) 
		NResource::lyrics_[prevLyr_] = this->txt->text();

	this->txt->clear();
	if( NResource::lyrics_[this->no->currentItem()] )
		this->txt->setText( NResource::lyrics_[this->no->currentItem()] );
	prevLyr_ = this->no->currentItem();

	this->initNo();
	oldTxt_ = this->txt->text();
}

/* revert the changes made in current stanza to the primary state */
void lyricsFrm::slRestor() {
	this->txt->setText( oldTxt_ );
}

/* cancel button - ignore changes and close the dialog */
void lyricsFrm::slCh() {
	for( int i = 0; i < 5; ++i )
	NResource::lyrics_[i] = oldField_[i];
	this->close();
}

#if KDE_VERSION < 220
/*------------------------------- tipForm ------------------------------------*/

tipFrm::tipFrm( QWidget *parent, int &tipNo ) : tipForm( parent, 0, true ) {
	QString *currentTip;
	if (NResource::theTips_.isEmpty()) return;
	if (tipNo >= NResource::theTips_.count()) tipNo = 0;
	if ((currentTip = NResource::theTips_.at(tipNo)) == 0) {
		NResource::abort("tipFrm::tipFrm: internal error");
	}
	this->title->setText( QString( i18n ("<b><i><u><center>Tip of day #%1")  ).arg( tipNo++ + 1 ) );
	this->txt->setText( QString( i18n ("<b><i><u>Did you know ... </u></i></b><br><br>%2") ).arg( KGlobal::locale ()->translate(*currentTip )));
	this->b1->setFocus();
	this->show();
}

void tipFrm::slOk() {

		kapp->config()->setGroup("TipOfDay");
		kapp->config()->writeEntry("RunOnStart", this->showtips->isChecked());
    this->close();

    }
#endif

/*--------------------------------- staffForm ----------------------------*/

staffFrm::staffFrm( QWidget *parent ) : staffForm( parent, 0, true ) {
	this->btOk->setFocus();
}

#undef setOn /* Don't know why. Otherwise with --enable-finel all "setOn" are redefined to */
#undef isOn  /* setChecked and all "isOn" are redefined to "setChecked" which causes errors.(?) */

void staffFrm::boot( QPtrList<NStaff> *stafflist, char unsigned id, int amount ) {
	char *na = (char *)I18N_NOOP("[not available]");
	this->elem->clear();
	int i = 0;
	if (amount)	staffAmount_ = amount;
	else staffAmount_ = stafflist->count();
	items_ = new QCheckListItem*[staffAmount_];
	if (!amount)
		for(NStaff *staff_elem = stafflist->first(); staff_elem; staff_elem = stafflist->next(), ++i ) {
			items_[i] = new QCheckListItem
				((QListView *) this->elem, i18n("Staff %1%2").arg(i < 9 ? " " : "").arg(i + 1), QCheckListItem::CheckBox);
			items_[i]->setText(1, i18n(NResource::instrTab[staff_elem->getVoice()]));
			items_[i]->setText( 2, ( staff_elem->staffName_ == "" || staff_elem->staffName_ == QString::null  ?
			i18n("[unnamed]") : QString(staff_elem->staffName_)));
		}
	else
		for(; i < staffAmount_; ++i ) {
			items_[i] = new QCheckListItem
				((QListView *) this->elem, i18n("Staff %1%2").arg(i < 9 ? " " : "").arg(i + 1), QCheckListItem::CheckBox );
			items_[i]->setText(1, na);
			items_[i]->setText(2, na);
		}
	bool *s;

	switch( id ) {

	case STAFF_ID_MUTE:
		s = NResource::staffSelMute_;
		this->setCaption(kapp->makeStdCaption(i18n("Mute staffs")));
		this->btOk->setText(i18n("&Mute"));
		break;

	case STAFF_ID_AUTOBAR:
		s = NResource::staffSelAutobar_;
		this->setCaption(kapp->makeStdCaption(i18n("Select staffs for autobar")));
		this->btOk->setText(i18n("Aut&obar"));
		break;

	case STAFF_ID_AUTOBEAM:
		s = NResource::staffSelAutobeam_;
		this->setCaption
			(kapp->makeStdCaption(i18n("Select staffs for autobeam")));
		this->btOk->setText(i18n("Aut&obeam"));
		break;

	case STAFF_ID_MERGE:
		s = NResource::staffSelMerge_;
		this->setCaption(kapp->makeStdCaption(i18n("Merge staffs")));
		this->btOk->setText(i18n("&Merge"));
		break;

	case STAFF_ID_TRACK:
		s = NResource::staffSelTrack_;
		this->setCaption(kapp->makeStdCaption(i18n("Create staff from TSE3")));
		this->btOk->setText(i18n("&Create"));
		break;

	case STAFF_ID_EXPORT:
		s = NResource::staffSelExport_;
	  	this->setCaption(kapp->makeStdCaption(i18n("Select staffs for export")));
		this->btOk->setText(i18n("&Select"));
		break;

	case STAFF_ID_MULTI:
		s = NResource::staffSelMulti_;
		this->setCaption(kapp->makeStdCaption(i18n("Multi staff select")));
		this->btOk->setText(i18n("&Select"));
		break;

	}
    if( s ) {
	for( int i = 0; i < staffAmount_; ++i )
	    items_[i]->setOn( s[i] );
	}
    else
	s = new bool[staffAmount_];

#if QT_VERSION >= 300
    this->exec();
#else
    this->show();
#endif
    if( !abort_ ) {
	delete [] s;
	s = new bool[staffAmount_];
	}
    for( int i = 0; i < staffAmount_; ++i ) {
	if( !abort_ ) s[i] = items_[i]->isOn();
	delete items_[i];
	}
    delete items_;

    if( !abort_ ) {
	switch( id ) {
	    case STAFF_ID_MUTE:		NResource::staffSelMute_ = s; break;
	    case STAFF_ID_AUTOBAR:	NResource::staffSelAutobar_ = s; break;
	    case STAFF_ID_AUTOBEAM:	NResource::staffSelAutobeam_ = s; break;
	    case STAFF_ID_MERGE:	NResource::staffSelMerge_ = s; break;
	    case STAFF_ID_TRACK:	NResource::staffSelTrack_ = s; break;
	    case STAFF_ID_EXPORT:	NResource::staffSelExport_ = s; break;
	    case STAFF_ID_MULTI:	NResource::staffSelMulti_ = s; break;
	    }
	}

    }

void staffFrm::slOk() {

    abort_ = false;
    this->close();

    }

void staffFrm::slCh() {

    abort_ = true;
    if (NResource::staffSelMulti_) {
		delete [] NResource::staffSelMulti_;
		NResource::staffSelMulti_ = 0;
    }
    NResource::numOfMultiStaffs_ = 0;
    close();
    }

void staffFrm::slUn() {

    for( int i = 0; i < staffAmount_; ++i )
	items_[i]->setOn( false );

    }

void staffFrm::slSel() {

    for( int i = 0; i < staffAmount_; ++i )
	items_[i]->setOn( true );

    }

/*------------------------------------ staffelForm ------------------------------*/

staffelFrm::staffelFrm( NMainFrameWidget *mainWidget ) : staffelForm( mainWidget, 0, true ) {
	selClass_ = 0;
	mainWidget_ = mainWidget;
	this->bo->setFocus();
}

int staffelFrm::boot( unsigned char type ) {

    selClass_ = new noteSel( this->selBase );
    selClass_->setType( type_ = type );
#if QT_VERSION >= 300
    this->exec();
#else
    this->show();
#endif
    int v = selClass_->getSelection();
    delete selClass_;
    selClass_ = 0;
    if( succ_ )
        return v;
    else
	return -1;

    }

void staffelFrm::resizeEvent( QResizeEvent *evt ) {

    if( selClass_ )
	selClass_->resiz();

    }

void staffelFrm::slOk() {

    int oct = 0;
    succ_ = true;

    this->close();

    switch (type_) {
    	case IS_CLEF:
		
		/* WARNING: constants below depend on number of melodic/non-melodic clefs! For now, all melodic (octaviable) clefs appear at the beginning of list and non-melodic (non-octaviable) at the end. */
		/* First we read and set clef octaviation, */
		if (selClass_->getSelection() < 15) {
			if( selClass_->getSelection() > 4 ) oct = 8;
			if( selClass_->getSelection() > 9 ) oct = -8;
		}
		
		/* then we read clefs for melodic intstruments ... */
		if (selClass_->getSelection() < 15) {
			mainWidget_->generateClef( ( 1 <<  ( selClass_->getSelection() % 5 ) ), oct );
		}
		/* or percussion clefs. */
		else {
			mainWidget_->generateClef( ( 1 <<  ( selClass_->getSelection() - 10 ) ), 0);
		}
		break;
	case IS_CLEF_DISTANCE:
		/* We do the same when changing the clef */
		if (selClass_->getSelection() < 15) {
			if( selClass_->getSelection() > 4 ) oct = 8;
			if( selClass_->getSelection() > 9 ) oct = -8;
		}
		if (selClass_->getSelection() < 15) {
			mainWidget_->performClefChange( ( 1 <<  ( selClass_->getSelection() % 5 )  ), oct );
		}
		else {
			mainWidget_->performClefChange( ( 1 <<  ( selClass_->getSelection() - 10 ) ), 0);
		}
		break;
    	}

	}

void staffelFrm::slCh() {

    succ_ = false;

    this->close();

    }


/*--------------------------------- timesigDiaFrm ------------------------------------*/

timesigDiaFrm::timesigDiaFrm( NMainFrameWidget *mainWidget ) {
	mainWidget_ = mainWidget;
	bu24->setPixmap(*NResource::time_24Icon_);
	bu44->setPixmap(*NResource::time_44Icon_);
	bu34->setPixmap(*NResource::time_34Icon_);
	bu38->setPixmap(*NResource::time_38Icon_);
	bu68->setPixmap(*NResource::time_68Icon_);
	slNumerator->setAll(1, 24, 4);
	slDnom->setAll(1, 24, 4);
	this->OkBu->setFocus();
}

void timesigDiaFrm::showDialog() {
	show();
}

void timesigDiaFrm::slot_24() {
	mainWidget_->setTempTimesig(2, 4);
	slNumerator->setStartVal(2);
	slDnom->setStartVal(4);
	hide();
}
void timesigDiaFrm::slot_34() {
	mainWidget_->setTempTimesig(3, 4);
	slNumerator->setStartVal(3);
	slDnom->setStartVal(4);
	hide();
}
void timesigDiaFrm::slot_38() {
	mainWidget_->setTempTimesig(3, 8);
	slNumerator->setStartVal(3);
	slDnom->setStartVal(8);
	hide();
}
void timesigDiaFrm::slot_44() {
	mainWidget_->setTempTimesig(4, 4);
	slNumerator->setStartVal(4);
	slDnom->setStartVal(4);
	hide();
}
void timesigDiaFrm::slot_68() {
	mainWidget_->setTempTimesig(6, 8);
	slNumerator->setStartVal(6);
	slDnom->setStartVal(8);
	hide();
}

void timesigDiaFrm::slOk() {
	mainWidget_->setTempTimesig(
		slNumerator->getValue(),
		slDnom->getValue());
	hide();
}

void timesigDiaFrm::slCanc() {
	hide();
}
	
/*---------------------------------- tse3info ----------------------------------------*/

tse3InfoFrm::tse3InfoFrm( QWidget *parent ) : TSE3InfForm( parent, 0, true ) {
	this->bok->setFocus();
}

void tse3InfoFrm::slOk() {

    this->close();

    }

/*---------------------------------- mupWrn -----------------------------------------*/

const char *mupWrn::warnTemplate_ =
"Noteedit will save your score, and it can (probably) restore the score.\n"
"But the file doesn't conform to MUP, so it will not work with the MUP interpreter.";

mupWrn::mupWrn( QWidget *parent ) : mupWarning (parent, 0, true ) {
	this->OkBu->setFocus();
}

void mupWrn::setOutput(QString *output) {
	details_ = *output;
}


void mupWrn::slOk() {
	NResource::dontShowMupWarnings_ = showAgain->isChecked();
	this->close();
}

void mupWrn::slShowDet() {
	this->close();
	OutputBox::warning(0, i18n("Saved, but MUP does not work!"), details_, i18n("Save"));
}

/*---------------------------------- expWrn -----------------------------------------*/

expWrn::expWrn( QWidget *parent ) : expWarnDialog (parent, 0, true ) {
	this->OkBu->setFocus();
}

void expWrn::setOutput(QString head, QString *output) {
	details_ = *output;
	Headline->setText(head);
}


void expWrn::slOk() {
	this->close();
}

void expWrn::slShowDet() {
	this->close();
	OutputBox::warning(0, cap_, details_, i18n("Export"));
}

#ifdef OLD_STAFFDIALOG
/* ---------------------------------- voiceDia ---------------------------------------*/

voiceDiaFrm::voiceDiaFrm() : voiceDia() {
	stemGroup_.insert(stemUpBu);
	stemGroup_.insert(stemDownBu);
	stemGroup_.insert(stemIndividualBu);
	connect (voiceNumSlider, SIGNAL( valueChanged(int) ), this, SLOT(changeActualVoice(int)));
	connect (stemUpBu, SIGNAL( clicked () ), this, SLOT ( stemToUp() ));
	connect (stemDownBu, SIGNAL( clicked () ), this, SLOT ( stemToDown() ));
	connect (stemIndividualBu, SIGNAL( clicked () ), this, SLOT ( stemToIndividual() ));
	this->OkBu->setFocus();
}

void voiceDiaFrm::setSlidersAndButtons(int voiceNum) {
	disconnect (voiceNumSlider, SIGNAL( valueChanged(int) ), this, SLOT(changeActualVoice(int)));
	voiceNumSlider->setRange(1, currentStaff_->voiceCount());
	if (voiceNum < -1) {
		voiceNum = currentStaff_->getActualVoiceNr();
		if (voiceNum == -1) voiceNum++;
		voiceCounter->display(voiceNum+1);
		voiceNumSlider->setValue(voiceNum+1);
	}
	if (voiceNum == -1) voiceNum = 0;
	connect (voiceNumSlider, SIGNAL( valueChanged(int) ), this, SLOT(changeActualVoice(int)));
	newStemDir_ = currentVoice_->stemPolicy_;
	switch (newStemDir_) {
		case STEM_POL_INDIVIDUAL: stemIndividualBu->setChecked (true); break;
		case STEM_POL_UP: stemUpBu->setChecked (true); break;
		case STEM_POL_DOWN: stemDownBu->setChecked (true); break;
	}
	restPosSlider->setValue(currentVoice_->yRestOffs_);
}

void voiceDiaFrm::showDialog(NMainFrameWidget *mainWidget, NStaff *currentStaff, int voiceNum) {
	currentStaff_ = currentStaff;
	currentVoice_ = currentStaff_->getVoiceNr(voiceNum == -1 ? 0 : voiceNum);
	if (currentVoice_ == 0) {
		NResource::abort("voiceDiaFrm::showDialog: internal error");
	}
	setSlidersAndButtons(-2);
	mainWidget_ = mainWidget;
	voiceDia::show();
}
void voiceDiaFrm::stemToUp() {newStemDir_ = STEM_POL_UP;}
void voiceDiaFrm::stemToDown() {newStemDir_ = STEM_POL_DOWN;}
void voiceDiaFrm::stemToIndividual() {newStemDir_ = STEM_POL_INDIVIDUAL;}

void voiceDiaFrm::createNewVoice() {
	int voice_num;
	currentVoice_ = currentStaff_->addNewVoice();
	setSlidersAndButtons(-2);
}

void voiceDiaFrm::deleteActualVoice() {
	int voice_num;
	currentVoice_ = currentStaff_->deleteVoice(currentVoice_);
	setSlidersAndButtons(-2);
}

void voiceDiaFrm::changeActualVoice(int nr) {
	currentVoice_ = currentStaff_->getVoiceNr(nr - 1);
	if (currentVoice_ == 0) {
		NResource::abort("voiceDiaFrm::changeActualVoice: internal error");
	}
	setSlidersAndButtons(nr);
}

void voiceDiaFrm::slOk() {
	currentVoice_->yRestOffs_ = restPosSlider->value();
	currentVoice_->stemPolicy_ = newStemDir_;
	mainWidget_->reposit();
	mainWidget_->repaint();
	hide();
}

void voiceDiaFrm::slAppl() {
	currentVoice_->yRestOffs_ = restPosSlider->value();
	currentVoice_->stemPolicy_ = newStemDir_;
	mainWidget_->reposit();
	mainWidget_->repaint();
}

void voiceDiaFrm::slCanc() {
	hide();
}

#endif

