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

#include <stdlib.h>
#include <unistd.h>
#include <qapplication.h>
#include <qpopupmenu.h>
#include <qkeycode.h>
#include <qmessagebox.h>
#include <qpainter.h>
#include "mainframewidget.h"
#include "resource.h"
#include "uiconnect.h"
#include "voice.h"
#include <kaction.h>
#include <kstdaction.h>
#include <klocale.h>
#include <kwin.h>
#include <kfiledialog.h>
#include <qcheckbox.h>

#define CHS_TRILL	0
#define CHS_LNTRILL	1
#define CHS_DYN		2
#define CHS_VA8		3
#define CHS_VA8BASSA	4

#define modifyPrerequisite if (playing_) return;

/*------------------------------------ reaction on menu events -----------------------------------*/

void NMainFrameWidget::insertLine() {

    modifyPrerequisite;
    
    lineSelWg *wdg = new lineSelWg( 0 );
#if QT_VERSION >= 300
    wdg->exec();
#else
    wdg->show();
#endif
    switch( wdg->getResult() ) {
	case CHS_TRILL:   selectedSign_ = TRILL; 	break;
	case CHS_LNTRILL: selectedSign_ = LNTRILL;	break;
	case CHS_DYN:	  selectedSign_ = DYNAMIC;	break;
	case CHS_VA8:	  selectedSign_ = VA8;	break;
	case CHS_VA8BASSA:  selectedSign_ = VA8_BASSA;	break;

    }
    delete wdg;

}

/*------------------------------------ internal reactions -----------------------------------------*/

void NMainFrameWidget::trillLengthChanged(int val) {

    if( currentVoice_->getCurrentElement()->trill_ < 0 )
	currentVoice_->getCurrentElement()->trill_ = -val;
    else
	currentVoice_->getCurrentElement()->trill_ = val;

    repaint();
}

void NMainFrameWidget::trillDisabled() {

    currentVoice_->getCurrentElement()->trill_ = 0;
    this->trillEnabled_->setChecked( true );
    manageToolElement(true);
    repaint();

}

void NMainFrameWidget::dynamicPosChanged(int val) {

    currentVoice_->getCurrentElement()->dynamic_ = val;
    repaint();
    
}

void NMainFrameWidget::dynamicKill() {
    
    currentVoice_->getCurrentElement()->dynamic_ = 0;
    dynamicDisable_->setChecked(true);
    manageToolElement(true);
    repaint();
    
}

void NMainFrameWidget::dynamicSwitch() {

    currentVoice_->getCurrentElement()->dynamicAlign_ = this->dynamicAlignment_->isChecked();
    repaint();

}

void NMainFrameWidget::vaLengthChanged(int val) {

    if (currentVoice_->getCurrentElement()->va_ < 0) {
    	currentVoice_->getCurrentElement()->va_ = -val;
    }
    else {
    	currentVoice_->getCurrentElement()->va_ = val;
    }
    repaint();
    
}

void NMainFrameWidget::vaDisabled() {
    
    currentVoice_->getCurrentElement()->va_ = 0;
    vaDisable_->setChecked(true);
    manageToolElement(true);
    repaint();
    
}

