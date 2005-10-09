/****************************************************************************/
/*                                                                          */
/* This program is free software; you can redistribute it and/or modify it  */
/* under the terms of the GNU General Public License as published by the    */
/* Free Software Foundation; either version 2 of the License, or (at your   */
/* option) any later version.                                               */
/*                                                                          */
/* This program is distributed in the hope that it will be useful, but      */
/* WITHOUT ANY WARRANTY; without even the implied warranty of               */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General */
/* Public License for more details.                                         */
/*                                                                          */
/* You should have received a copy of the GNU General Public License along  */
/* with this program; (See "LICENSE.GPL"). If not, write to the Free        */
/* Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA       */
/* 02111-1307, USA.                                                         */
/*                                                                          */
/*--------------------------------------------------------------------------*/
/*                                                                          */
/*    Erik Sigra, SWEDEN                                                    */
/*    sigra@home.se                                                         */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

#ifndef VOICEDIALOG_H
#define VOICEDIALOG_H

#include <kdialogbase.h>

#include <qbuttongroup.h>
#include <qradiobutton.h>
#include <qgroupbox.h>
#include <qpushbutton.h>
#include <qslider.h>
#include <qvbox.h>

#include "staff.h"

class VoiceDialog;
class staffPropFrm;
class NMainFrameWidget;


class VoiceBox : public QVBox {
	Q_OBJECT

public:
	VoiceBox(QHBox *, VoiceDialog *, staffPropFrm *, unsigned int, NVoice *);
	~VoiceBox();
	QHBox *getContainer() {return parent_;}
	void apply();
	void renumber(int nr);
	QButtonGroup *stemDirection_;
	QRadioButton *up_;
	QRadioButton *individual_;
	QRadioButton *down_;
	QSlider *restPosition_;
	QPushButton *remove_;


private slots:
	void destroy();

private:
	QHBox *parent_;
	NVoice *theVoice_;
	VoiceDialog *voiceDialog_;
	staffPropFrm *staffPropForm_;
	QLabel *voiceNumber_;

};

class VoiceDialog : public KDialogBase {
	Q_OBJECT

public:
	VoiceDialog(NMainFrameWidget *, int staffNr, QPtrList<NStaff> *);
	~VoiceDialog();
	bool destroyVoice(VoiceBox *rem_box, NVoice *voice);
	int myActivePageIndex()  {
		/* there seems to be a bug in activePageIndex() */
		return activePageIndex() - firstPageIdx_;
	}

private slots:
	void accept();
	void slotApply();
	void slotUser1();

private:
	NMainFrameWidget *mainWidget_;
	QPtrList<QPtrList <VoiceBox> > pageList_;
	QPtrList<NStaff> *staffList_;
	int firstPageIdx_; /* there seems to be a bug in activePageIndex() */

};

#endif //  VOICEDIALOG_H
