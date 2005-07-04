/************************************************************************************/
/* This is the partly modified file "tabtrack.cpp" from                             */
/* the "kguitar" program version 0.4.1. As of this writing the whole programs       */
/* was available from:                                                              */
/*                                                                                  */
/*    http://kguitar.sourceforge.net                                                */
/*                                                                                  */
/* The modifications mainly concern KDE3/Qt3 support.                               */
/*                                                                                  */
/************************************************************************************/
/* J.Anders <ja@informatik.tu-chemnitz.de> 04.09.2002                               */
/************************************************************************************/

#ifndef CHORD_H
#define CHORD_H

#include <qdialog.h>
/*
#include "global.h"
#include "globaloptions.h"

#include <fingers.h>
*/
#include <qcombobox.h>
#include <qcheckbox.h>

#define STEPSIZE     40

class QLineEdit;
class QListBox;
class QButtonGroup;
class QRadioButton;
class QComboBox;
class QLabel;
class ChordList;
class FingerList;
class TabTrack;
class Strumming;
class Fingering;
class NMainFrameWidget;


class ChordSelector: public QDialog {
    Q_OBJECT
public:
    ChordSelector( NMainFrameWidget *mainWidget, const char *name = 0);
    int  app(int x);
    void setApp(int x, int fret);
    void reconfigureMenues();
    static void transposeChordName(QString *cname, int semitones);

    Fingering *fng;
    ChordList *chords;
    void setFingers(char *fingerField);

public slots:
    void detectChord();
    void setStep3();
    void setHighSteps();
    void setStepsFromChord();
    void findSelection();
    void findChords();
    void slOk();
    void slRemove();

private:
    void initChordSelector();
    TabTrack *parm;
    NMainFrameWidget *mainWidget_;

    QLineEdit *chname;
    QListBox *tonic, *step3, *stephigh;
    QComboBox *st[6], *inv, *bassnote;
    QLabel *cnote[7];
    QButtonGroup *complexity;
    QRadioButton *complexer[3];
    FingerList *fnglist;
    QCheckBox *showDiagram_;
    bool setFirst_; // avoid feedback
};

#endif
