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

#ifndef LINES_H
#define LINES_H

#include <kapp.h>
#include <qbuttongroup.h> 
#include <qhbox.h>

#include "config.h"
#include "lines.h"
#include "exports.h"
#include "scaleEd.h"
#include "smRest.h"
#include "volume.h"
#include "staffElem.h"
#include "listSel.h"
#include "generProps.h"
#include "lyrics.h"
#include "midi.h"
#include "staffSel.h"
#include "filter.h"
#include "tse3Info.h"
#include "expWarn.h"
#include "timesigDia.h"
#if KDE_VERSION < 220
#include "tip.h"
#endif
#include "mupWarning.h"
#include "metronom.h"

#ifdef OLD_STAFFDIALOG
#include "voiceDia.h"
#endif

#define MIDI_PAGE	0
#define MUSIX_PAGE	1
#define ABC_PAGE	2
#define PMX_PAGE	3
#define LILY_PAGE	4
#define MUSICXML_PAGE	5
#define PARAM_PAGE	6

class NTSE3Handler;
class noteSel;
class NStaff;
class NVoice;
class NMainFrameWidget;
class QString;
class staffFrm;
class QCheckListItem;
class NMusElement;
struct main_props_str;

class lineSelWg : public lineSel {

    public:
	lineSelWg( QWidget* parent = 0 );
	int getResult();
	
    private slots:
	void slot_abort();
	void slot_ok();

    private:
	int choosed;
	bool abort;
    
    };

class exportFrm : public exportForm {

    public:
	exportFrm( NMainFrameWidget *mainWidget, QWidget *parent = 0 );
	~exportFrm();
	void initialize( QList<NStaff> *stafflist, QList<NVoice> *voicelist, QString fname );
	void boot();
	bool paramsEnabled();
	void setEnabled(bool ok);
	int getSaveWidth();
	int getSaveHeight();
	bool withMeasureNums();
	void setWithMeasureNums(bool with);
	void setSaveWidth(int width);
	void setSaveHeight(int height);
	
    private slots:
	void closeIt();
	void startExport();
	void texMeasures();
	void musixStaffSig();
	void pmxStaffSig();
	void lilyMeasures();
	void musixLandSlot();
	void pmxLandSlot();
	void lilyLandSlot();
	void lilyStaffSig();
	void abcLandSlot();
	void paramLandSlot();
	
    private:
	QList<NStaff> *staffList_;
	QList<NVoice> *voiceList_;
	QString sourceFile_;
	staffFrm *staffDialog_;
	NMainFrameWidget *mainWidget_;

    };    

class scaleFrm : public scaleForm {

    public:
	scaleFrm( QWidget *parent );
	void boot( QList<NStaff> *staffList_, QScrollBar *scrollx_  );
	bool boot( main_props_str *props_str_, NStaff *currentStaff_, NVoice *currentVoice, NMusElement **tmpElem_, int subtype );
	int boot();
	
    private slots:
	void transSlotOk();
	void transSlotCancel();

    private:
	bool succ_;
	
    };

class filterFrm : public filterForm {

    public:
	filterFrm( NMainFrameWidget *parent, bool modal );
	int item2length(QComboBox* box);
    private slots:
	void averageSlot();
	void cancelSlot();
	void recSlot();
	
    private:
	NMainFrameWidget *mainWidget_;

    };

class metronomFrm : public metronomForm {

    public:
	metronomFrm( QWidget *parent, NTSE3Handler *caller, bool modal );
	void reactivate();

    private slots:
	void startSlot();
	void abortSlot();
	
    private:
	NTSE3Handler *caller_;

    };

class smallestRestFrm : public clRestForm {

    public:
	smallestRestFrm( NMainFrameWidget *parent );
	int item2length();
	bool boot();

    private slots:
	void clSlot();
	void okSlot();

    private:
	bool succ_;
	int oldval_;

    };

class volumeFrm : public volumeForm {

    public:
	volumeFrm( QWidget *parent );
	bool boot();

    private slots:
	void okSlot();
	void chSlot();

    private:
	bool succ_;

    };

#define LIST_VOICE	0
#define LIST_MIDI_DEVS	1
#define LIST_MOVE_STAFF	2

class listFrm : public listForm {

    public:
	listFrm( QWidget *parent );
	bool boot( int val, short int type, const QString & caption, const QString & title, QList<NStaff> *staff = 0 );
	
    private slots:
	void okSlot();
	void chSlot();
	
    public:
	bool succ_;
	
    };

class propFrm : public propForm {

    public:
	propFrm( QWidget *parent );

    public slots:
	void boot();
	
    private slots:
	void slok();
	void slcl();

    };

class lyricsFrm : public lyricsForm {

    public:
	lyricsFrm( QWidget *parent );

    public slots:
	void boot();
	void slCl();
	void slOk();
	void slOp();
	void chngLyr();
	void initNo();
	void slRestor();
	void slCh();

    private:
	signed char prevLyr_;
	QString oldTxt_;
	QString oldField_[6];
    
    };

struct midiFrm : public midiForm {
	midiFrm();
};

#if KDE_VERSION < 220
class tipFrm : public tipForm {

    public:
	tipFrm( QWidget *parent, int &tipNo );

    public slots:
	void slOk();

    };
#endif

#define STAFF_ID_MUTE	0
#define STAFF_ID_AUTOBAR	1
#define STAFF_ID_AUTOBEAM	2
#define STAFF_ID_MERGE	3
#define STAFF_ID_TRACK	4
#define STAFF_ID_EXPORT	5
#define STAFF_ID_MULTI	6

class staffFrm : public staffForm {

    public:
	staffFrm( QWidget *parent );
	void boot( QList<NStaff> *stafflist, char unsigned id, int amount = 0 );

    private slots:
	void slCh();
	void slOk();
	void slUn();
	void slSel();

    private:
	QCheckListItem **items_;
	int staffAmount_;

    public:
	bool abort_;
	
    };

class tse3InfoFrm : public TSE3InfForm {

    public:
	tse3InfoFrm( QWidget *parent );

    public slots:
	void slOk();

    };

class mupWrn : public mupWarning {
    public :
	mupWrn( QWidget *parent );
	void setOutput(QString *output);
    public slots:
	void slOk();
	void slShowDet();
    private:
	static const char *warnTemplate_;
	QString details_;
};

class expWrn : public expWarnDialog {
    public :
	expWrn( QWidget *parent );
	void setOutput(QString head, QString *output);
    public slots:
	void slOk();
	void slShowDet();
    private:
	QString details_;
	QString cap_;
};

#ifdef OLD_STAFFDIALOG
class voiceDiaFrm : public voiceDia {
    public :
	voiceDiaFrm();
	void showDialog(NMainFrameWidget *mainWidget, NStaff *currentStaff, int voiceNum);
    public slots:
	virtual void slOk();
	virtual void slCanc();
	virtual void slAppl();
	virtual void changeActualVoice(int);
	virtual void stemToUp();
	virtual void stemToDown();
	virtual void stemToIndividual();
	virtual void createNewVoice();
	virtual void deleteActualVoice();
    private:
	void setSlidersAndButtons(int voiceNum);
	NStaff *currentStaff_;
	NVoice *currentVoice_;
	NMainFrameWidget *mainWidget_;
	QButtonGroup stemGroup_;
	int newStemDir_;
};
#endif


class staffelFrm : public staffelForm {

    public:
	staffelFrm( NMainFrameWidget *mainWidget );
	int boot( unsigned char type );

    private slots:
	void slOk();
	void slCh();

    private:
	void resizeEvent( QResizeEvent *evt );

    private:
	noteSel *selClass_;
	NMainFrameWidget *mainWidget_;
	int type_;
	bool succ_;

    };

class timesigDiaFrm : public timesigDia {

    public:
	timesigDiaFrm( NMainFrameWidget *mainWidget );
	void showDialog();
    private slots:
	    virtual void slot_24();
	    virtual void slCanc();
	    virtual void slOk();
	    virtual void slot_34();
	    virtual void slot_38();
	    virtual void slot_44();
	    virtual void slot_68();

    private:
	NMainFrameWidget *mainWidget_;
    };


#endif // LINES_H
