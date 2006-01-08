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
/*		Reinhard Katzmann, GERMANY			*/
/*		reinhard@suamor.de					*/
/*											*/
/*											*/
/****************************************************************************************/

#ifndef PREVIEWPRINT_H_
#define PREVIEWPRINT_H_

#include <qwidget.h>
#ifndef WITH_SCONS
#include "config.h"
#endif
#include "resource.h"

class KProcess;
class IntPrinter;
class exportFrm;

// Preview Print and Print class
// Original by Jorge Windmeisser Oliver
// Greatly enhanced 2005 by Reinhard Katzmann
class NPreviewPrint : public QWidget
{
	Q_OBJECT
public:
	NPreviewPrint();
	~NPreviewPrint();
	
public slots:
    void filePrint(bool, exportFrm *);

protected slots:
    void filePrintPreviewFinished(KProcess *);
    void filePrintReceivedStdOut(KProcess *, char *, int);
    void filePrintReceivedStdErr(KProcess *, char *, int);
    
protected:
    void setupPrinting(bool); // Helper methods for filePrint
    bool setExistantFile( QString &filePath );
    void printDoExport(KProcess *typesettingProgram);
    bool printDoPreview(const QString fileType);
    bool printDoPrinting(QString fileType);
    
    void printWithLilypond(bool);
    void printWithABC(bool);
    void printWithPMX(bool);
    void printWithMusiXTeX(bool);
    void printWithMusicXML(bool);
    void printWithMidi(bool);
    void printWithNative(bool);

#ifdef WITH_DIRECT_PRINTING
    IntPrinter *printer_;
    QString previewFile_;
    QString fileName_;        // Base file name (temporary)
    QString dirPath_;         // Path where the files will be saved
    QString filePath_;        // Both of the above together
    QString ftsetProg_;       // file name of the typesetting program used to print
    exportFrm *exportDialog_; // Copy of export Dialog instance
#endif
};

#ifdef WITH_DIRECT_PRINTING

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

#endif /*PREVIEWPRINT_H_*/
