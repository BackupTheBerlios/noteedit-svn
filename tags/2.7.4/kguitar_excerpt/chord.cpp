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

#include "chord.h"
#include "fingers.h"
#include "fingerlist.h"
#include "chordlist.h"
#include "global.h"
/*
#include "tabsong.h"
#include "strumming.h"
#include "globaloptions.h"
*/
#include <tabtrack.h>

#include <klocale.h>
#include <kmessagebox.h>

#include <qpushbutton.h>
#include <qbuttongroup.h>
#include <qradiobutton.h>
#include <qlistbox.h>
#include <qlineedit.h>
#include <qstring.h>
#include <qlabel.h>
#include <qlayout.h>
#include "../noteedit/mainframewidget.h"
#include "../noteedit/chorddiagram.h"
#include "../noteedit/resource.h"


QString notes_us1[12] = {"C",  "C#", "D",  "D#", "E",  "F",
						 "F#", "G",	 "G#", "A",	 "A#", "B"};
QString notes_us2[12] = {"C",  "Db", "D",  "Eb", "E",  "F",
						 "Gb", "G",	 "Ab", "A",	 "Bb", "B"};
QString notes_us3[12] = {"C",  "Db", "D",  "Eb", "E",  "F",
						 "F#", "G",	 "G#", "A",	 "Bb", "B"};

QString notes_eu1[12] = {"C",  "C#", "D",  "D#", "E",  "F",
						 "F#", "G",	 "G#", "A",	 "A#", "H"};
QString notes_eu2[12] = {"C",  "Db", "D",  "Eb", "E",  "F",
						 "Gb", "G",	 "Ab", "A",	 "Hb", "H"};
QString notes_eu3[12] = {"C",  "Db", "D",  "Eb", "E",  "F",
						 "F#", "G",	 "G#", "A",	 "Hb", "H"};

QString notes_jz1[12] = {"C",  "C#", "D",  "D#", "E",  "F",
						 "F#", "G",	 "G#", "A",	 "B" , "H"};
QString notes_jz2[12] = {"C",  "Db", "D",  "Eb", "E",  "F",
						 "Gb", "G",	 "Ab", "A",	 "B" , "H"};
QString notes_jz3[12] = {"C",  "Db", "D",  "Eb", "E",  "F",
						 "F#", "G",	 "G#", "A",	 "B" , "H"};

void ChordSelector::transposeChordName(QString *cname, int semitones) {
	int i, l, matchlen;
	int longestmatch;
	int match_idx;

	for (i = 0, longestmatch = 0; i < 12; i++) {
		if (cname->find(notes_us1[i]) == 0) {
			matchlen = strlen(notes_us1[i]);
			if (matchlen > longestmatch) {
				longestmatch = matchlen;
				match_idx = i;
			}
		}
	}

	for (i = 0; i < 12; i++) {
		if (cname->find(notes_us2[i]) == 0) {
			matchlen = strlen(notes_us2[i]);
			if (matchlen > longestmatch) {
				longestmatch = matchlen;
				match_idx = i;
			}
		}
	}

	for (i = 0; i < 12; i++) {
		if (cname->find(notes_us3[i]) == 0) {
			matchlen = strlen(notes_us3[i]);
			if (matchlen > longestmatch) {
				longestmatch = matchlen;
				match_idx = i;
			}
		}
	}

	for (i = 0; i < 12; i++) {
		if (cname->find(notes_eu1[i]) == 0) {
			matchlen = strlen(notes_eu1[i]);
			if (matchlen > longestmatch) {
				longestmatch = matchlen;
				match_idx = i;
			}
		}
	}

	for (i = 0; i < 12; i++) {
		if (cname->find(notes_eu2[i]) == 0) {
			matchlen = strlen(notes_eu2[i]);
			if (matchlen > longestmatch) {
				longestmatch = matchlen;
				match_idx = i;
			}
		}
	}

	for (i = 0; i < 12; i++) {
		if (cname->find(notes_eu3[i]) == 0) {
			matchlen = strlen(notes_eu3[i]);
			if (matchlen > longestmatch) {
				longestmatch = matchlen;
				match_idx = i;
			}
		}
	}

	for (i = 0; i < 12; i++) {
		if (cname->find(notes_jz1[i]) == 0) {
			matchlen = strlen(notes_jz1[i]);
			if (matchlen > longestmatch) {
				longestmatch = matchlen;
				match_idx = i;
			}
		}
	}

	for (i = 0; i < 12; i++) {
		if (cname->find(notes_jz2[i]) == 0) {
			matchlen = strlen(notes_jz2[i]);
			if (matchlen > longestmatch) {
				longestmatch = matchlen;
				match_idx = i;
			}
		}
	}

	for (i = 0; i < 12; i++) {
		if (cname->find(notes_jz3[i]) == 0) {
			matchlen = strlen(notes_jz3[i]);
			if (matchlen > longestmatch) {
				longestmatch = matchlen;
				match_idx = i;
			}
		}
	}
	if (longestmatch == 0) return;

	l = cname->length() - longestmatch;
	*cname = cname->right(l);
	match_idx += 120 + semitones;
	match_idx %= 12;
	*cname = cname->insert(0, note_name(match_idx));
}

// return note name for a given note nr and note names setting
QString note_name_res(int num, int names)
{
	if ((num < 0) || (num > 11))
		return "Unknown";

	switch (names) {
	case 0: return notes_us1[num];
	case 1: return notes_us2[num];
	case 2: return notes_us3[num];

	case 3: return notes_eu1[num];
	case 4: return notes_eu2[num];
	case 5: return notes_eu3[num];

	case 6: return notes_jz1[num];
	case 7: return notes_jz2[num];
	case 8: return notes_jz3[num];
	}

	return "Unknown";
}

// return note name for a given note nr and the global note names setting
QString note_name(int num)
{
	return note_name_res(num, NResource::globalNoteNames_);
}


//					   3  5	 7	9  11 13
int stemplate[][6] = {{-1,2, 0, 0, 0, 0 },   // C
                      {-1,2, 2, 0, 0, 0 },   // C7
                      {-1,2, 3, 0, 0, 0 },   // C7M
                      {-1,2, 1, 0, 0, 0 },   // C6
                      {-1,2, 2, 2, 0, 0 },   // C9
                      {-1,2, 2, 2, 2, 0 },   // C11
                      {-1,2, 2, 2, 2, 2 },   // C13
                      {3, 3, 0, 0, 0, 0 },   // Caug
                      {2, 1, 1, 0, 0, 0 },   // Cdim
                      {0, 2, 0, 0, 0, 0 }};  // C5

QString maj7name_[] = {"7M", "maj7", "dom7"};
QString flat_[] = {"-", "b"};
QString sharp_[] = {"+", "#"};

ChordSelector::ChordSelector(NMainFrameWidget *mainWidget, const char *name):
        QDialog(mainWidget, name, TRUE)
{
	mainWidget_ = mainWidget;
        initChordSelector();
}



void ChordSelector::initChordSelector()
{
	parm = new TabTrack(FretTab, i18n("Guitar"), 1, 0, 25, 6, 24);
	setFirst_ = true;

	chname = new QLineEdit(this);
	chname->setMinimumHeight(20);

	// CHORD SELECTOR FOR FINDER WIDGETS

	tonic = new QListBox(this);
	for (int i = 0; i < 12; i++)
		tonic->insertItem(note_name(i));
//	tonic->setFixedVisibleLines(12);
	tonic->setMinimumWidth(40);
	connect(tonic, SIGNAL(highlighted(int)), SLOT(findChords()));

	bassnote = new QComboBox(FALSE, this);
	for (int i = 0; i < 12; i++)
		bassnote->insertItem(note_name(i));
	bassnote->setMinimumSize(40, 20);

	step3 = new QListBox(this);
	step3->insertItem("M");
	step3->insertItem("m");
	step3->insertItem("sus2");
	step3->insertItem("sus4");
//	step3->setFixedVisibleLines(4);
	step3->setMinimumWidth(40);
	connect(step3, SIGNAL(highlighted(int)), SLOT(setStep3()));

	stephigh = new QListBox(this);
	stephigh->insertItem("");
	stephigh->insertItem("7");
	stephigh->insertItem(maj7name_[NResource::globalMaj7_]);
	stephigh->insertItem("6");
	stephigh->insertItem("9");
	stephigh->insertItem("11");
	stephigh->insertItem("13");
	stephigh->insertItem("aug");
	stephigh->insertItem("dim");
	stephigh->insertItem("5");
//	stephigh->setFixedVisibleLines(10);
	stephigh->setMinimumWidth(40);
	connect(stephigh, SIGNAL(highlighted(int)), SLOT(setHighSteps()));

	// st array holds values for each step:
	// st[0] - 3'	 st[1] - 5'	   st[2] - 7'
	// st[3] - 9'	 st[4] - 11'   st[5] - 13'

	QLabel *stlabel[7];
	QString tmp;
	for (int i = 0; i < 7; i++) {
		tmp.setNum(i * 2 + 1);
		tmp = tmp + "\'";
		stlabel[i] = new QLabel(tmp, this);
		stlabel[i]->setAlignment(AlignCenter);

		cnote[i] = new QLabel(this);
		cnote[i]->setAlignment(AlignCenter);

		if (i > 0) {
			st[i - 1] = new QComboBox(FALSE, this);
			st[i - 1]->insertItem("x");
			if ((i == 2) || (i >= 4)) {
				st[i - 1]->insertItem(flat_[NResource::globalFlatPlus_]);
				st[i - 1]->insertItem("0");
				st[i - 1]->insertItem(sharp_[NResource::globalFlatPlus_]);
			}
			connect(st[i - 1], SIGNAL(activated(int)), SLOT(findSelection()));
			connect(st[i - 1], SIGNAL(activated(int)), SLOT(findChords()));
		}
	}

	st[0]->insertItem("2");
	st[0]->insertItem(flat_[NResource::globalFlatPlus_]);
	st[0]->insertItem("3");
	st[0]->insertItem("4");

	st[2]->insertItem("6");
	st[2]->insertItem(flat_[NResource::globalFlatPlus_]);
	st[2]->insertItem("7");

	inv = new QComboBox(FALSE, this);
	inv->insertItem(i18n("Root"));
	inv->insertItem(i18n("Inv #1"));
	inv->insertItem(i18n("Inv #2"));
	inv->insertItem(i18n("Inv #3"));
	inv->insertItem(i18n("Inv #4"));
	inv->insertItem(i18n("Inv #5"));
	inv->insertItem(i18n("Inv #6"));
	connect(inv, SIGNAL(activated(int)), SLOT(findChords()));

	complexity = new QButtonGroup(this);
	complexity->setMinimumSize(90, 70);
	complexer[0] = new QRadioButton(i18n("Usual"), complexity);
	complexer[0]->setGeometry(5, 5, 80, 20);
	complexer[1] = new QRadioButton(i18n("Rare"), complexity);
	complexer[1]->setGeometry(5, 25, 80, 20);
	complexer[2] = new QRadioButton(i18n("All"), complexity);
	complexer[2]->setGeometry(5, 45, 80, 20);
	complexity->setButton(0);
	connect(complexity, SIGNAL(clicked(int)), SLOT(findChords()));

	// CHORD ANALYZER

	fng = new Fingering(parm, this);
	fng->move(230, 10);
	connect(fng, SIGNAL(chordChange()), SLOT(detectChord()));

	chords = new ChordList(this);
	chords->setMinimumWidth(120);
	connect(chords, SIGNAL(highlighted(int)), SLOT(setStepsFromChord()));

	// CHORD FINDER OUTPUT

	fnglist = new FingerList(parm,this);
	connect(fnglist,SIGNAL(chordSelected(const int *)),
	        fng,SLOT(setFingering(const int *)));

	// DIALOG BUTTONS

	QPushButton *ok, *cancel, *remove;

	remove = new QPushButton(i18n("remove chord"), this);
	remove->setMinimumSize(75, 30);
	connect(remove, SIGNAL(clicked()), SLOT(slRemove()));

	ok = new QPushButton(i18n("OK"), this);
	ok->setMinimumSize(75, 30);
	connect(ok, SIGNAL(clicked()), SLOT(slOk()));

	cancel = new QPushButton(i18n("Cancel"), this);
	cancel->setMinimumSize(75, 30);
	connect(cancel, SIGNAL(clicked()), SLOT(reject()));

	showDiagram_ = new QCheckBox(i18n("show diagram"), this);
	showDiagram_->setChecked(true);
	showDiagram_->setMinimumSize(75, 30);



	// LAYOUT MANAGEMENT

	// Main layout
	QBoxLayout *l = new QHBoxLayout(this, 10);

	// Chord finding & analyzing layout
	QBoxLayout *lchord = new QVBoxLayout();
	l->addLayout(lchord, 1);

	// Chord editing layout
	QBoxLayout *lchedit = new QHBoxLayout();
	lchord->addWidget(chname);
	lchord->addLayout(lchedit);
	lchord->addWidget(fnglist, 1);

	// Chord selection (template-based) layout
	QGridLayout *lselect = new QGridLayout(3, 3, 5);
	lchedit->addLayout(lselect);

	lselect->addMultiCellWidget(tonic, 0, 2, 0, 0);
	lselect->addColSpacing(0, 40);

	lselect->addWidget(step3, 0, 1);
	lselect->addWidget(complexity, 1, 1);
	lselect->addWidget(inv, 2, 1);

	lselect->addMultiCellWidget(stephigh, 0, 1, 2, 2);
	lselect->addWidget(bassnote, 2, 2);

	// Chord icon showing layout
	QBoxLayout *lshow = new QVBoxLayout();
	lchedit->addLayout(lshow);

	// Analyzing and showing chord layout
	QBoxLayout *lanalyze = new QHBoxLayout();
	lshow->addLayout(lanalyze);
	lanalyze->addWidget(fng);
	lanalyze->addWidget(chords);

	// Steps editor layout
	QGridLayout *lsteps = new QGridLayout(3, 7, 0);
	lshow->addLayout(lsteps);

	lsteps->addWidget(stlabel[0], 0, 0);
	lsteps->addWidget(cnote[0], 2, 0);

	lsteps->addRowSpacing(0, 15);
	lsteps->addRowSpacing(1, 20);
	lsteps->addRowSpacing(2, 15);
	lsteps->setColStretch(0, 1);

	for (int i = 1; i < 7; i++) {
		lsteps->addWidget(stlabel[i], 0, i);
		lsteps->addWidget(st[i - 1], 1, i);
		lsteps->addWidget(cnote[i], 2, i);
		lsteps->setColStretch(i, 1);
	}

	// Strumming and buttons stuff layout
	QBoxLayout *lstrum = new QVBoxLayout();
	l->addLayout(lstrum);
	lstrum->addStretch(1);
	lstrum->addWidget(showDiagram_);
	lstrum->addWidget(ok);
	lstrum->addWidget(remove);
	lstrum->addWidget(cancel);

	l->activate();

	setCaption(i18n("Chord constructor"));
	resize( QSize(740, 400) ); 
}
void ChordSelector::reconfigureMenues() {
	int i;
	stephigh->changeItem(maj7name_[NResource::globalMaj7_], 2);
	for (i = 0; i < 12; i++) {
		tonic->changeItem(note_name(i), i);
		bassnote->changeItem(note_name(i), i);
	}
	for (i = 0; i < 7; i++) {
		if (i > 0) {
			if ((i == 2) || (i >= 4)) {
				st[i - 1]->changeItem(flat_[NResource::globalFlatPlus_], 0);
				st[i - 1]->changeItem(sharp_[NResource::globalFlatPlus_], 2);
			}
		}
	}
	st[0]->insertItem(flat_[NResource::globalFlatPlus_], 1);
	st[2]->insertItem(flat_[NResource::globalFlatPlus_], 1);
}


// Try to detect some chord forms from a given applicature.
void ChordSelector::detectChord()
{
	bool cn[12];
	int i, j, bass;
	QString name;
	int s3, s5, s7, s9, s11, s13;

	for (i = 0; i < 12; i++)
		cn[i] = FALSE;

	for (i = 0; i < parm->string; i++) {
		j = fng->app(i);
		if (j != -1) {
			j = (j + parm->tune[i]) % 12;
			cn[j] = TRUE;
		}
	}

//	chords->setAutoUpdate(FALSE);
	chords->clearSelection();
	chords->clear();

	for (i = 0; i < 12; i++)  if (cn[i]) {

		bool res = calcSteps(cn, i, s3, s5, s7, s9, s11, s13);
		if (res) {
			ChordListItem *item = new ChordListItem(i, bass, s3, s5,
			                                        s7, s9, s11, s13);
			chords->inSort(item);
		}
	}

#if QT_VERSION < 300
	chords->setAutoUpdate(TRUE);
#endif
	chords->repaint();
}

// identify a chord by trying to match the name with the names
// of the chords that can be generated from the strings given
// in:
// name: chord name
// str: array of fret values for six strings
// out:
// tonic: the chord's tonica or root note
// s3..s13: step values
// returns true if found

// note: magic numbers in for loops with gnn, gm7 and gfp are necessary because
// NResource::globalNoteNames_, ::globalMaj7_ and ::globalFlatPlus_ lack proper
// type definitions

bool identifyChord(QString name, char * str, QString& stp, int& alt, int& s3, int& s5, int& s7, int& s9, int& s11, int& s13)
{
	bool cn[12];
	int i;
	int j;
	TabTrack * parm = new TabTrack(FretTab, i18n("Guitar"), 1, 0, 25, 6, 24);

	for (i = 0; i < 12; i++)
		cn[i] = FALSE;

	for (i = 0; i < parm->string; i++) {
		j = str[i];
		if (j != -1) {
			j = (j + parm->tune[i]) % 12;
			cn[j] = TRUE;
		}
	}

	bool found = false;
	for (i = 0; (i < 12) && !found; i++)  if (cn[i]) {

		bool res = calcSteps(cn, i, s3, s5, s7, s9, s11, s13);
		// loop over all possible values buildName's last three params
		// and compare result with input name, break loop if found
		for (int gnn = 0; (gnn < 9) && !found; gnn++) {
			for (int gm7 = 0; (gm7 < 3) && !found; gm7++) {
				for (int gfp = 0; (gfp < 2) && !found; gfp++) {
					if (name == buildName(i, 0,
						s3, s5, s7, s9, s11, s13,
							gnn, gm7, gfp)) {
						found = true;
					}
				}
			}
		}
	}

	delete parm;
	if (found) {
		i--;		// was incremented once too many in for loop
		stp = notes_us1[i];
		if (stp.length() > 1) {
			stp = stp.left(1);
			alt = 1;
		} else {
			alt = 0;
		}
	} else {
		stp = "";
		alt = 0;
		s3 = s5 = s7 = s9 = s11 = s13 = -1;
	}
	return found;
}

// calculate step values
// in:
// cn: array of 12 bools indicating notes used (cn[0] = C, cn[1] = C#, etc.)
// i:  start index in cn
// out:
// s3..s13: step values
// returns true in case of valid chord

bool calcSteps(bool * cn, int i, int& s3, int& s5, int& s7, int& s9, int& s11, int& s13)
{
	int j;
	int noteok;
	int numnotes=0;		// number of different notes in a chord

	for (j = 0; j < 12; j++) {
		if (cn[j]) {
			numnotes++;
		}
	}

	// Initializing
	s3 = -1; s5 = -1; s7 = -1; s9 = -1; s11 = -1; s13 = -1;
	noteok = numnotes - 1;

	if (cn[i]) {

		// Detecting thirds
		if (cn[(i + 4) % 12]) {
			s3 = 4; noteok--;			// Major
		} else if (cn[(i + 3) % 12]) {
			s3 = 3; noteok--;			// Minor
		} else if (cn[(i + 5) % 12]) {
			s3 = 5; noteok--;			// Sus4
		} else if (cn[(i + 2) % 12]) {
			s3 = 2; noteok--;			// Sus2
		}

		// Detecting fifths
		if (cn[(i + 7) % 12]) {
			s5 = 7; noteok--;			// 5
		} else if (cn[(i+6) % 12]) {
			s5 = 6; noteok--;			// 5-
		} else if (cn[(i+8) % 12]) {
			s5 = 8; noteok--;			// 5+
		}

		// Detecting sevenths
		if (cn[(i + 10) % 12]) {
			s7 = 10;noteok--;			// 7
		} else if (cn[(i + 11) % 12]) {
			s7 = 11;noteok--;			// 7M
		} else if (cn[(i + 9) % 12]) {
			s7 = 9;noteok--;			// 6
		}

		// Detecting 9ths
		if ((cn[(i + 2) % 12]) && (s3 != 2)) {
			s9 = 2;noteok--;			// 9
		} else if ((cn[(i + 3) % 12]) && (s3 != 3)) {
			s9 = 3;noteok--;			// 9+
		} else if (cn[(i + 1) % 12]) {
			s9 = 1;noteok--;			// 9-
		}

		// Detecting 11ths
		if ((cn[(i+5)%12]) && (s3!=5)) {
			s11=5;noteok--;				  // 11
		} else if ((cn[(i+4)%12]) && (s3!=4)) {
			s11=4;noteok--;				  // 11-
		} else if ((cn[(i+6)%12]) && (s5!=6)) {
			s11=6;noteok--;				  // 11+
		}

		// Detecting 13ths
		if ((cn[(i+9)%12]) && (s7!=9)) {
			s13=9;noteok--;
		} else if ((cn[(i+8)%12]) && (s5!=8)) {
			s13=8;noteok--;
		} else if ((cn[(i+10)%12]) && (s7!=10)) {
			s13=10;noteok--;
		}
	}
	return (noteok == 0);
}

void ChordSelector::setStep3()
{
	switch (step3->currentItem()) {
	case 0: st[0]->setCurrentItem(3); break;				// Major
	case 1: st[0]->setCurrentItem(2); break;				// Minor
	case 2: st[0]->setCurrentItem(1); break;				// Sus2
	case 3: st[0]->setCurrentItem(4); break;				// Sus4
	}

	findSelection();
	findChords();
}

void ChordSelector::setStepsFromChord()
{
	setFirst_ = false; // avoid feedback
	ChordListItem *it = chords->currentItemPointer();

	tonic->setCurrentItem(it->tonic());
	for (int i = 0; i < 6; i++)
		st[i]->setCurrentItem(it->step(i));

	findSelection();
	findChords();
	setFirst_ = true;
}

void ChordSelector::setHighSteps()
{
	int j = stephigh->currentItem();

	if (j == -1)
		return;

	for (int i = 0; i < 6; i++)
		if (stemplate[j][i] != -1)
			st[i]->setCurrentItem(stemplate[j][i]);

	findSelection();
	findChords();
}

void ChordSelector::findSelection()
{
	bool ok = TRUE;

	switch (st[0]->currentItem()) {
	case 0: step3->clearSelection(); break;					// no3
	case 1: step3->setCurrentItem(2); break;				// Sus2
	case 2: step3->setCurrentItem(1); break;				// Minor
	case 3: step3->setCurrentItem(0); break;				// Major
	case 4: step3->setCurrentItem(3); break;				// Sus4
	}

	for (uint j = stephigh->count() - 1; j > 0; j--) {
		ok = TRUE;
		for (int i = 0; i < 6; i++) {
			if ((stemplate[j][i] != -1) &&
				(stemplate[j][i] != st[i]->currentItem())) {
				ok = FALSE;
				break;
			}
		}
		if (ok) {
			stephigh->setCurrentItem(j);
			break;
		}
	}
	if (!ok)
		stephigh->clearSelection();
}

void ChordSelector::findChords()
{
	int i, j, k = 0, min, max, bass = 0, muted = 0;
	int app[MAX_STRINGS];				// raw fingering itself
	int ind[MAX_STRINGS];				// indexes in hfret array

	//				    1  5  7   9  11 13
	int toneshift[6] = {0, 7, 10, 2, 5, 9};

	int fb[MAX_STRINGS][MAX_FRETS];	// array with an either -1 or number of note from a chord

	int hfret[MAX_STRINGS][MAX_FRETS];// numbers of frets to hold on every string
	int hnote[MAX_STRINGS][MAX_FRETS];// numbers of notes in a chord that make ^^

	bool needrecalc;					// needs recalculate max/min

	// CALCULATION OF REQUIRED NOTES FOR A CHORD FROM USER STEP INPUT

	int need[7],got[7];

	int t = tonic->currentItem();

	if (t == -1)						// no calculations without tonic
		return;

	int notenum = 1;
	need[0] = t;
	cnote[0]->setText(note_name(t));

	switch (st[0]->currentItem()) {
	case 1: need[1] = (t + 2) % 12; notenum++; break;	  // 2
	case 2: need[1] = (t + 3) % 12; notenum++; break;	  // 3-
	case 3: need[1] = (t + 4) % 12; notenum++; break;	  // 3+
	case 4: need[1] = (t + 5) % 12; notenum++; break;	  // 4
	}

	if (st[0]->currentItem()!=0) {
		cnote[1]->setText(note_name(need[1]));
	} else {
		cnote[1]->clear();
	}

	for (i = 1; i < 6; i++) {
		j = st[i]->currentItem();
		if (j) {
			need[notenum] = (t + toneshift[i] + (j - 2)) % 12;
			cnote[i + 1]->setText(note_name(need[notenum]));
			notenum++;
		} else {
			cnote[i + 1]->clear();
		}
	}

	// BEGIN THE CHORD FILLING SESSION
	fnglist->beginSession();

	// CHECKING IF NOTE NUMBER GREATER THAT AVAILABLE STRINGS

	// Ex: it's impossible to play 13th chords on 6 strings, but it's
	//	   possible on 7 string guitar. This way we optimize things a bit

	if (parm->string<notenum) {
		fnglist->endSession();
		return;
	}

	// CHECKING THE INVERSION NUMBER RANGE

	if (inv->currentItem()>=notenum)
		inv->setCurrentItem(0);

	int span = 3; // maximal fingerspan

	if (complexer[1]->isChecked())
		span = 4;
	if (complexer[2]->isChecked())
		span = 5;

	// PREPARING FOR FINGERING CALCULATION

	for (i = 0; i < parm->string; i++) {
		for (j = 0; j <= parm->frets; j++)
			fb[i][j] = -1;
		for (k=0;k<notenum;k++) {
			j=(need[k]-parm->tune[i]%12+12)%12;
			while (j<=parm->frets) {
				fb[i][j]=k;
				j+=12;
			}
		}
	}

	for (i = 0; i < parm->string; i++) {
		k=1;
		hfret[i][0] = -1;
		hnote[i][0] = -2;
		for (j = 0; j <= parm->frets; j++)
			if (fb[i][j] != -1) {
				hfret[i][k] = j;
				hnote[i][k] = fb[i][j];
				k++;
			}
		hnote[i][k] = -1;
	}

	// After all the previous funky calculations, we would have 2 arrays:
	// hfret[string][index] with numbers of frets where we can hold the string,
	//						(any other fret would make a chord unacceptable)
	// hnote[string][index] with numbers of notes in the chord that correspond
	//						to each hfret array's fret. -1 means end of string,
	//						-2 means muted string.

	for (i = 0; i < parm->string; i++)
		ind[i] = 0;

	min = -1; max = -1; needrecalc = FALSE;

	// MAIN FINGERING CALCULATION LOOP

	i = 0;
	do {
		// end of string not reached
		if (!( (hnote[i][ind[i]]==-1) || ( (!needrecalc) && (max-min>=span)))) {
			if (needrecalc) {
				min=parm->frets+1;max=0;
				for (j=0;j<parm->string;j++) {
					if (hfret[j][ind[j]]>0) {
						if (hfret[j][ind[j]]<min)  min=hfret[j][ind[j]];
						if (hfret[j][ind[j]]>max)  max=hfret[j][ind[j]];
					}
					if (max-min>=span)
						break;
				}
			}
			if (max-min<span) {
				for (k=0;k<notenum;k++)
					got[k]=0;
				k=0;bass=255;muted=0;
				for (j=0;j<parm->string;j++) {
					if (hfret[j][ind[j]]>=0) {
						if (parm->tune[j]+hfret[j][ind[j]]<bass)
							bass=parm->tune[j]+hfret[j][ind[j]];
						if (!got[hnote[j][ind[j]]]) {
							got[hnote[j][ind[j]]]=1;
							k++;
						}
					} else {
						muted++;
					}
				}
			}

			if ((k==notenum) && (max-min<span) && (bass%12==need[inv->currentItem()])) {
				for (j=0;j<parm->string;j++)
					app[j]=hfret[j][ind[j]];
				if (complexer[0]->isChecked()) {
					if ((muted==0) ||										// No muted strings
						((muted==1) && (app[0]==-1)) ||						// Last string muted
						((muted==2) && (app[0]==-1) && (app[1]==-1))) {		// Last and pre-last muted
						fnglist->addFingering(app);
					}
				} else {
					fnglist->addFingering(app);
				}
			}

			i=0;
		} else {						// end of string reached
			ind[i]=0;i++;
			needrecalc=TRUE;
			if (i>=parm->string)
				break;
		}

		if (hfret[i][ind[i]]>min) {
			ind[i]++;
			if (hfret[i][ind[i]]>max)
				max = hfret[i][ind[i]];
			needrecalc=FALSE;
		} else {
			ind[i]++;
			needrecalc=TRUE;
		}
	} while (TRUE);

	fnglist->endSession();
	if (setFirst_) fnglist->setFirstChord();
}

int ChordSelector::app(int x) { return fng->app(x); }
void ChordSelector::setApp(int x, int fret) { fng->setApp(x, fret); }

void ChordSelector::slOk() {
	QString chorname = (chords->currentItem() < 0) ? chords->text(0) : chords->currentText();
	if (chorname.isNull() || chorname.isEmpty()) {
		hide();
		return;
	}
	mainWidget_->setTempChord(new NChordDiagram(fng->getFingerField(), chorname, showDiagram_->isChecked()));
	hide();
}

void ChordSelector::slRemove() {
	mainWidget_->RemoveChord();
	hide();
}

void ChordSelector::setFingers(char *fingerField) {
	int i, fingers[6];

	for (i = 0; i < 6; i++) {
		fingers[i] = fingerField[i];
	}
	fng->setFingering(fingers);
}
	

#include "chord.moc"
