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

#include <iostream>
#include <kprocess.h>
#include <kprinter.h>
#include <kmessagebox.h>
#include <kdeprint/kprintdialogpage.h>                     
// LVIFIX: kstandarddirs.h does not exist in KDE 2.2, assume KDE 3.0 specific
#include <kstandarddirs.h>
#include <klocale.h>
#include <qfileinfo.h> 
#include <qdir.h>
#include <qscrollbar.h>
#include "previewprint.h"
#include "midiexport.h"
#include "musixtex.h"
#include "pmxexport.h"
#include "lilyexport.h"
#include "musicxmlexport.h"
#include "abcexport.h"
#include "uiconnect.h"
#include "staffPropFrm.h"

NPreviewPrint::NPreviewPrint()
{
#ifdef WITH_DIRECT_PRINTING
	printer_ = 0;
#endif
}

NPreviewPrint::~NPreviewPrint()
{
#ifdef WITH_DIRECT_PRINTING
	if( printer_ ) delete printer_;
#endif
}

// Setup printer (KDE IntPrinter)
void NPreviewPrint::setupPrinting(bool preview)
{
#ifdef WITH_DIRECT_PRINTING
  // Print preview ?
  if ( preview == false )
  {
    // Setup printer (shows the print dialog)
    if( printer_->setup(this) == false ) 
      KMessageBox::error(0,i18n("Couldn't setup printer"), 
                         kapp->makeStdCaption(i18n("???")));
  }
#endif
}

// Read all the print options and print (preview) with the selected format
// preview: 'true' if the print should be previewed
// exportDialog: Instance of the exportDialog (used for exporting the file and
//               for putting the form into export dialog page)
void NPreviewPrint::filePrint(bool preview, exportFrm *exportDialog)
{
#ifdef WITH_DIRECT_PRINTING

    bool bCustomPrinting = false;
    // Save copy of export Dialog
    exportDialog_ = exportDialog;
    QDir curDir( QDir::currentDirPath() );
    
    // Find program used for printing (preview)
    ftsetProg_=KStandardDirs::findExe( NResource::typesettingProgramInvokation_ );
    // Not found ? -> Tell the user and leave
    if (ftsetProg_.isNull()) {
      KMessageBox::error (0, QString( NResource::typesettingProgramInvokation_ ) + " was not found in your PATH, aborting", "Noteeditor");
      return;
    }      

    // Try to create a temporary file in /tmp
    filePath_=tempnam("/tmp","note_");
    if (filePath_.isNull()) {
      KMessageBox::error (0,"Couldn't access the /tmp directory, aborting", "Noteeditor");
      return;
    }
    // Get file name without path
    fileName_ = QFileInfo( filePath_ ).fileName();
    // Get path without file name
    dirPath_  = QFileInfo( filePath_ ).dirPath( true );

    // CD to temporary directory
    QFileInfo().dir().cd( dirPath_ );

    // Remove printer so the extra tab gets deleted
    if( printer_ ) delete printer_;
    printer_ = new IntPrinter( this );

    // Custom: Format decides which export filter to use
    if( NResource::typesettingProgram_ == 4 )
    {
        bCustomPrinting = true;
	// Find out which format was selected
	switch( NResource::typesettingProgramFormat_ )
	{
	  case 0: // Midi
	    NResource::typesettingProgram_ = 5; // This is now Midi
	    break;
	  case 1: // Lilypond
	    NResource::typesettingProgram_ = 2;
	    break;
	  case 2: // MusicXML
	    NResource::typesettingProgram_ = 6; // This is now MusicXML
	    break;
	  case 3: // ABC Music
	    NResource::typesettingProgram_ = 0;
	    break;
	  case 4: // NoteEdit
	    NResource::typesettingProgram_ = 7; // This is now NoteEdit
	    break;
	}
    }

    // Get the right export
    switch( NResource::typesettingProgram_ )
    {
      case 0: // ABC Music
	printWithABC(preview);
        break;
      case 1: // PMX
	printWithPMX(preview);
        break;
      case 2: // Lilypond
	printWithLilypond(preview);
        break;
      case 3: // MusiXTeX
	printWithMusiXTeX(preview);
        break;
      case 4: // Avoid warning
	break;
      case 5: // Midi
	printWithMidi(preview);
        break;
      case 6: // MusicXML
	printWithMusicXML(preview);
        break;
      case 7: default: // NoteEdit
	printWithNative(preview);
        break;
    }
    // CD back to main directory
    QFileInfo().dir().cd( curDir.absPath() );
#endif /* WITH_DIRECT_PRINTING */
}

// Clean up after print preview was finished.
void NPreviewPrint::filePrintPreviewFinished(KProcess *)
{
#ifdef WITH_DIRECT_PRINTING
    printf("Finished.\n");
    fflush(stdout);
    // Remove preview file on exit of browser
    unlink(previewFile_);
#endif
}

// Show the standard output of the export or preview process
void NPreviewPrint::filePrintReceivedStdOut(KProcess *, char *buffer, int buflen)
{
#ifdef WITH_DIRECT_PRINTING
  // Terminate manually
  buffer[buflen] = 0;
  printf("%s",buffer);
  fflush(stdout);
#endif
}

// Show the error output of the export or preview process
void NPreviewPrint::filePrintReceivedStdErr(KProcess *, char *buffer, int buflen)
{
#ifdef WITH_DIRECT_PRINTING
  // Terminate manually
  buffer[buflen] = 0;
  printf("%s",buffer);
  fflush(stdout);
#endif
}

// Connects stdout/stderr output to methods to show the output of the running export
// Runs the export process and removes the connections afterwards
void NPreviewPrint::printDoExport(KProcess *typesettingProgram)
{
#ifdef WITH_DIRECT_PRINTING
    QValueList<QCString> args = typesettingProgram->args();
    connect( typesettingProgram, SIGNAL( processExited (KProcess *) ), 
             this, SLOT( filePrintPreviewFinished(KProcess *) ) );
    // Output of convert process should be visible on console in case something fails
    connect( typesettingProgram, SIGNAL( receivedStdout(KProcess *, char *, int) ),
	     this, SLOT( filePrintReceivedStdOut(KProcess *, char *, int) ) );
    connect( typesettingProgram, SIGNAL( receivedStderr(KProcess *, char *, int) ),
	     this, SLOT( filePrintReceivedStdErr(KProcess *, char *, int) ) );
    cout << "Exporting with ";
    for ( QValueList<QCString>::Iterator it = args.begin(); it != args.end(); ++it ) {
         cout << *it << " ";
    }
    cout << endl;
    // Start converting exported file to a printable file
    typesettingProgram->start(KProcess::Block, KProcess::All);
    disconnect( typesettingProgram, SIGNAL( processExited (KProcess *) ), 
                this, SLOT( filePrintPreviewFinished(KProcess *) ) );
    disconnect( typesettingProgram, SIGNAL( receivedStdout(KProcess *, char *, int) ),
	        this, SLOT( filePrintReceivedStdOut(KProcess *, char *, int) ) );
    disconnect( typesettingProgram, SIGNAL( receivedStderr(KProcess *, char *, int) ),
	        this, SLOT( filePrintReceivedStdErr(KProcess *, char *, int) ) );
#endif
}

// Checks the existant of the file (including dirPath_ if check fails)
// filePath: File (and optional path) to be checked
bool NPreviewPrint::setExistantFile( QString &filePath )
{
    printf("Previewing file %s\n",previewFile_.ascii());
    if( false == QFileInfo( filePath ).exists() )
      previewFile_ = dirPath_ + "/" + filePath;
    printf("Previewing file %s\n",previewFile_.ascii());
    if( false == QFileInfo( previewFile_ ).exists() )
    {
      KMessageBox::sorry(this, i18n("File was not succesfully converted."), 
                         kapp->makeStdCaption(i18n("???")));
      return false;
    }
    return true;
}

// Shows the exported file (postscript or pdf) with the set preview program
bool NPreviewPrint::printDoPreview(QString fileType)
{
#ifdef WITH_DIRECT_PRINTING
    KProcess previewProgram;
    QString fprevprog=KStandardDirs::findExe( NResource::previewProgramInvokation_ );
    QStringList printpreviewOptions = QStringList::split( " ", QString(NResource::previewOptions_) );
    // Preview File: abcm2ps strangely does not add the path to the created ps file
    previewFile_ = fileName_ + fileType;
    if( false == setExistantFile( previewFile_ ) )
      return false;
    // Replace the %s String by previewFile
    printpreviewOptions.gres("%s",previewFile_);
    previewProgram << fprevprog << printpreviewOptions;
    // Signal exit of program so we can clean up
    connect( &previewProgram, SIGNAL( processExited (KProcess *) ), 
             this, SLOT( filePrintPreviewFinished(KProcess *) ) );
    // Start preview
    previewProgram.start(KProcess::DontCare, KProcess::All);
    disconnect( &previewProgram, SIGNAL( processExited (KProcess *) ), 
                this, SLOT( filePrintPreviewFinished(KProcess *) ) );
#endif
    return true;
}

// Prints the exported file (postscript or pdf) with the KDE print system
bool NPreviewPrint::printDoPrinting(QString fileType)
{
#ifdef WITH_DIRECT_PRINTING
    QString printFile( fileName_ + fileType );
    QStringList printFiles;
    if( false == setExistantFile( printFile ) )
      return false;
    printFiles += printFile;
    printer_->doPreparePrinting();
    // Print file
    printf("Printing with %s\n",printer_->printProgram().ascii());
    if (!printer_->printFiles(printFiles,true))
        unlink(fileName_+fileType);
#endif
    return true;
}

// Print (preview) using the Lilypond export to create the print file 
// preview:  'true', if the print should be previewed
void NPreviewPrint::printWithLilypond(bool preview)
{
#ifdef WITH_DIRECT_PRINTING
    // Init process, export form and printer
    KProcess typesettingProgram;
    struct lily_options lilyOpts;
    NLilyExport lily;
    QStringList printOptions = QStringList::split( " ", QString(NResource::typesettingOptions_) );
    LilypondExportForm *form = (LilypondExportForm *)printer_->createExportForm( exportDialog_->FormatComboBox->text( LILY_PAGE ), EXP_Lilypond );
    // Read options
    exportDialog_->getLilyOptions( lilyOpts );
    // Save options to new form
    exportDialog_->setLilyOptions( *form, lilyOpts );
    setupPrinting(preview);
    // Export file to lilypond
    exportDialog_->doExport( LILY_PAGE, filePath_ + ".ly", false );

    // Replace the %s String by fileName_
    printOptions.gres("%s",fileName_ + ".ly");
    // Which file to use for printing ? fileName_.ps (Option -o !)
    // We probably need to filter the -o option!
    // Do not create a pdf file (broken with my lilypond version)
    typesettingProgram << ftsetProg_ << "--ps" << printOptions;
    printf("Setting working directory: %s\n",dirPath_.ascii());
    typesettingProgram.setWorkingDirectory( dirPath_ );
    // Export file and create postscript file
    printDoExport(&typesettingProgram);
    // Converting succesfull ?
    if (typesettingProgram.normalExit()) 
    {
		// Preview the printable file ?
		if( preview == true )
			printDoPreview(".ps");
		else
			printDoPrinting(".ps");
		unlink(filePath_ + ".ly");
		unlink(filePath_ + ".log");
		unlink(filePath_ + ".pdf");
    }
#endif
}

// Print (preview) using the ABC export to create the print file
// preview:  'true', if the print should be previewed
void NPreviewPrint::printWithABC(bool preview)
{
#ifdef WITH_DIRECT_PRINTING
    // Init process, export form and printer
    KProcess typesettingProgram;
    struct abc_options abcOpts;
    NABCExport abc;
    QStringList printOptions = QStringList::split( " ", QString(NResource::typesettingOptions_) );
    ABCExportForm *form = (ABCExportForm *)printer_->createExportForm( exportDialog_->FormatComboBox->text( ABC_PAGE ), EXP_ABC );
    // Read options
    exportDialog_->getABCOptions( abcOpts );
    // Save options to new form
    exportDialog_->setABCOptions( *form, abcOpts );
    setupPrinting(preview);
    // Export file to abc
    exportDialog_->doExport( ABC_PAGE, filePath_ + ".abc", false );
    
    // Replace the %s String by fileName_
    printOptions.gres("%s",fileName_ + ".abc");
    // Earlier options: << "-O=" << "-c"
    // Which file to use for printing ? Out.ps or fileName_.ps (Option -O= !)
    // We probably need to filter all -O options!
    if( QString(NResource::typesettingOptions_).find("-O=") == -1 && 
        QString(NResource::typesettingOptions_).find("-O =") == -1)
      printOptions.prepend("-O=");
    typesettingProgram << ftsetProg_ << printOptions;
    typesettingProgram.setWorkingDirectory( dirPath_ );
    // Export file and create postscript file
    printDoExport(&typesettingProgram);
    // Converting succesfull ?
    if (typesettingProgram.normalExit()) 
    {
      // Preview the printable file ?
      if( preview == true )
	printDoPreview(".ps");
      else
	printDoPrinting(".ps");
      unlink(filePath_ + ".abc");
    }
#endif
}
    
void NPreviewPrint::printWithPMX(bool preview)
{
#ifdef WITH_DIRECT_PRINTING
    // Init process, export form and printer
    KProcess typesettingProgram;    
    NPmxExport pmx;
    PMXExportForm *form = (PMXExportForm *)printer_->createExportForm( exportDialog_->FormatComboBox->text( PMX_PAGE ), EXP_PMX );
    setupPrinting(preview);
#endif
}
    
void NPreviewPrint::printWithMusiXTeX(bool preview)
{
#ifdef WITH_DIRECT_PRINTING
    // Init process, export form and printer
    KProcess typesettingProgram;    
    exportFrm *formBack=exportDialog_;
    MusiXTeXExportForm *form = (MusiXTeXExportForm *)printer_->createExportForm( exportDialog_->FormatComboBox->text( MUSIX_PAGE ), EXP_MusiXTeX );
    NMusiXTeX musixtex;
    setupPrinting(preview);
#endif
}
    
void NPreviewPrint::printWithMusicXML(bool preview)
{
#ifdef WITH_DIRECT_PRINTING
    // Init process, export form and printer
    KProcess typesettingProgram;    
    exportFrm *formBack=exportDialog_;
    MusicXMLExportForm *form = (MusicXMLExportForm *)printer_->createExportForm( exportDialog_->FormatComboBox->text( MUSICXML_PAGE ), EXP_MusicXML );
    NMusicXMLExport musicxml;
    // Currently we could omit to show the export dialog page as i
    setupPrinting(preview);
#endif
}
    
void NPreviewPrint::printWithMidi(bool preview)
{
#ifdef WITH_DIRECT_PRINTING
    // Init process, export form and printer
    KProcess typesettingProgram;    
    exportFrm *formBack=exportDialog_;
    MidiExportForm *form = (MidiExportForm *)printer_->createExportForm( exportDialog_->FormatComboBox->text( MIDI_PAGE ), EXP_Midi );
    NMidiExport midi;
    setupPrinting(preview);
#endif
}
    
void NPreviewPrint::printWithNative(bool preview)
{
#ifdef WITH_DIRECT_PRINTING
#endif
}

#ifdef WITH_DIRECT_PRINTING
IntPrinter::IntPrinter(QWidget *exportParent)
{
  form_ = 0;
  formatExport_ = new PrintExportDialogPage( exportParent );
  pageExport_ = formatExport_;
}

// Creates the export form.
// dialogTitle: Title for the to be added dialog tab
// format: export format of the file to be printed
QWidget *IntPrinter::createExportForm(QString dialogTitle, exportFormat_T format)
{
  formatExport_->setTabTitle( dialogTitle );
  // Create form dependant from the format
  switch( format )
  {
    case EXP_ABC: // ABC Music
      form_ = new ABCExportForm( pageExport_ );
      break;
    case EXP_PMX: // PMX 
      form_ = new PMXExportForm( pageExport_ );
      break;
    case EXP_Lilypond: // Lilypond 
      form_ = new LilypondExportForm( pageExport_ );
      break;
    case EXP_MusiXTeX: // MusiXTeX 
      form_ = new MusiXTeXExportForm( pageExport_ );
      break;
    case EXP_Midi: // Midi
      form_ = new MidiExportForm( pageExport_ );
      break;
    case EXP_MusicXML: // MusicXML 
      form_ = new MusicXMLExportForm( pageExport_ );
      break;
    case EXP_NoteEdit: default: // NoteEdit
      break;
  }
  // Add form to dialog page
  addDialogPage(pageExport_);
  return form_;
}

// No need to delete our objects as QT does everything for us
IntPrinter::~IntPrinter()
{
}

// RK: Completely simplified the dialog, layout cannot be done within this class
PrintExportDialogPage::PrintExportDialogPage( QWidget *tab, QWidget *parent, const char *name )
 : KPrintDialogPage( parent, name )
{
}

// Sets the dialog page title
void PrintExportDialogPage::setTabTitle( QString title )
{
    setTitle( title );
}

PrintExportDialogPage::~PrintExportDialogPage()
{
}

void PrintExportDialogPage::getOptions( QMap<QString,QString>& /*opts*/, bool /*incldef*/ )
{
}

void PrintExportDialogPage::setOptions( const QMap<QString,QString>& /*opts*/ )
{
}

bool PrintExportDialogPage::isValid( QString& /*msg*/)
{
  return true;
}

#endif /* WITH_DIRECT_PRINTING */
