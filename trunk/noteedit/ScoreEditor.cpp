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

#include "ScoreEditor.h"
#ifdef ANTHEM_PLUGIN
#if GCC_MAJ_VERS > 2
#include <istream.h>
#else
#include <istream>
#endif
#include <qapplication.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <kapp.h>
#include <klocale.h>
#include <tse3/PhraseEdit.h>
#include "mainframewidget.h"
#include "midimapper.h"
#include "resource.h"

NResource *ScoreEditor::resource_ = 0;

ScoreEditor::ScoreEditor(TSE3::PhraseEdit *phraseEdit, KToolBar *toolbar,
QWidget *parent) : PhraseEditorBase(phraseEdit, toolbar, parent)
{
    if (resource_ == 0) {
    	resource_ = new NResource();
	NResource::mapper_ = new NMidiMapper(0 /* TODO: the argument mus be the midi scheduler */ );
    }
    mainWidget_ = new NMainFrameWidget(toolbar, this, "mainWidget");
    mainWidget_->plugButtons(toolbar);
    mainWidget_->createStaffFromPhraseEdit(phraseEdit);
}

ScoreEditor::~ScoreEditor() {
	mainWidget_->unPlugButtons(toolbar);
	delete mainWidget_;
}

void ScoreEditor::resizeEvent( QResizeEvent * evt) {
	mainWidget_->resize(evt->size().width(), evt->size().height());
} 

#endif /* ANTHEM_PLUGIN */
	
