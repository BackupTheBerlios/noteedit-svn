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

// Global defines

#define MAX_STRINGS   12
#define MAX_FRETS     24
#define NUMFRETS      5

// Global utility functions

class QString;
extern QString flat_[];
extern QString sharp_[];
extern QString maj7name_[];

QString buildName(int _tonic, int _bass, int s3, int s5, int s7, int s9, int s11, int s13, int noteNames, int maj7, int flatPlus);
bool calcSteps(bool * cn, int i, int& s3, int& s5, int& s7, int& s9, int& s11, int& s13);
bool identifyChord(QString name, char * str, QString& stp, int& alt, int& s3, int& s5, int& s7, int& s9, int& s11, int& s13);
QString note_name(int);
QString note_name_res(int, int);
