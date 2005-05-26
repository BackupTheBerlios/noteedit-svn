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

#include "config.h"
#if GCC_MAJ_VERS > 2
#include <iostream>
#else
#include <istream.h>
#endif
#include <qapplication.h>
#include <qthread.h>
#include <qsplashscreen.h>
#include <qdesktopwidget.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <kconfig.h>
#include <kapp.h>
#include <klocale.h>
#include <kstddirs.h>
#if KDE_VERSION >= 220
#include <ktip.h>
#endif
#include "mainframewidget.h"
#include "resource.h"
#include "midimapper.h"
#include "uiconnect.h"
#include "noteedit_part.h"

using namespace std;

static KCmdLineOptions options[] =
{
 { "+file(s)",          I18N_NOOP("Files to load"), 0 },
 { "export-lilypond",   I18N_NOOP("export file to LilyPond"), 0 },
 { "export-musixtex",   I18N_NOOP("export file to MusiXTeX"), 0 },
 { "export-abc",        I18N_NOOP("export file to ABC"), 0 },
 { "noalsa",		I18N_NOOP("Do not create an ALSA midi scheduler"), 0 },
 { "nooss",		I18N_NOOP("Do not create an OSS midi scheduler"), 0 },
#ifdef TSE3_HAS_ARTS
 { "arts",		I18N_NOOP("Create an aRts midi scheduler"), 0 },
#endif
 { 0,0,0 }
};

class splashThread : public QThread {
	public:
	splashThread() {
		// Create splash picture from file
		QString s = locate( "data", "noteedit/resources/" ) + QString("splash.png");
		QPixmap *splashPix_ = new QPixmap( s );
		// Check if splashscreen picture was successfully loaded
		if(splashPix_->isNull()) {
			printf ("Error in loading image [%s]",s.ascii());
			return;
		}
		// Create splash screen
		splash_ = new QSplashScreen( *splashPix_ );
	}
	virtual void run() {
		sleep(1);
		splash_->close();
	}
	void showSplash() {
		splash_->show();
	}
	void changeSplashMessage( QString msg )
	{
		splash_->message( msg );
		KApplication::kApplication()->processEvents();
	}
	private:
	QSplashScreen *splash_;
};


int main( int argc, char **argv )
{
    QString lastFile;
    bool doExportLilyPond;
    bool doExportMusixTeX;
    bool doExportABC;
    bool doCreateAlsa;
    bool doCreateOSS;
#ifdef TSE3_HAS_ARTS
    bool doCreateaRts;
#endif
    /* notice the path because it is destroyed in KDE part mechanism */
    /* but necessary in MusiXTeX export for scripting: */ 
    NResource::userpath_ = getenv("PATH");
    KCmdLineArgs::init(argc, argv, NoteeditFactory::aboutData());
    KCmdLineArgs::addCmdLineOptions( options ); // Add our own options.
    KURL u;
    KApplication a( argc, argv );

    splashThread b;			// Create splash picture from file
    b.showSplash();
    b.changeSplashMessage( i18n("Loading resources...") );
    NResource *nr = new NResource();
    (void)nr;
    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
    b.changeSplashMessage( i18n("Parsing command line...") );
    doExportLilyPond = args->isSet("export-lilypond");
    doExportMusixTeX = args->isSet("export-musixtex");
    doExportABC      = args->isSet("export-abc");
    doCreateAlsa = args->isSet("alsa");
    doCreateOSS = args->isSet("oss");
#ifdef TSE3_HAS_ARTS
    doCreateaRts = args->isSet("arts");
#endif
    if (doExportLilyPond && doExportMusixTeX \
	|| doExportLilyPond && doExportABC \
	|| doExportMusixTeX && doExportABC) {
	cerr << "you cannot specify multiple export \"--export-musixtex\" AND \"--export-lilypond\" AND \"--export-abc\"" << endl;
	exit(-1);
    }
    if (!doCreateAlsa && !doCreateOSS) {
	cerr << "you cannot forbid both: ALSA midi schduler and OSS midi schduler" << endl;
	doCreateAlsa = doCreateOSS = true;
    }
    if (!doCreateAlsa) NResource::schedulerRequest_ &= (~ALSA_SCHEDULER_REQUESTED);
    if (!doCreateOSS) NResource::schedulerRequest_ &= (~OSS_SCHEDULER_REQUESTED);
#ifdef TSE3_HAS_ARTS
    if (doCreateaRts) NResource::schedulerRequest_ |= ARTS_SCHEDULER_REQUESTED;
#endif
    NResource::mapper_ = new NMidiMapper();
    NMainWindow * mainWidget = new NMainWindow;
    switch (args->count()) {
    case 0: if (NResource::startupLoadLastScore_) {
		kapp->config()->setGroup("RecentFiles");
#if KDE_VERSION >= 320
		lastFile = kapp->config()->readPathEntry("File1");
#else
		lastFile = kapp->config()->readEntry("File1");
#endif
		KURL u(lastFile);
		if (u.isValid() ) {
			mainWidget->mainFrameWidget()->loadFile (u.path() );
		}
	    }
	    if (doExportLilyPond) {
		cerr << "\"--export-lilypond\" ignored: file name expected" << endl;
	    }
	    if (doExportMusixTeX) {
		cerr << "\"--export-musixtex\" ignored: file name expected" << endl;
	    }
	    if (doExportABC) {
		cerr << "\"--export-abc\" ignored: file name expected" << endl;
	    }
	    break;
    case 1: u = args->url(0);
	    if ( u.isLocalFile() ) {
		mainWidget->mainFrameWidget()->loadFile( u.path() );
	    	if (doExportMusixTeX) {
			NResource::commandLine_ = true;
			mainWidget->mainFrameWidget()->exportMusixTeXImm();
			exit(0);
		}
	    	if (doExportLilyPond) {
			NResource::commandLine_ = true;
			mainWidget->mainFrameWidget()->exportLilyPondImm();
			exit(0);
		}
	    	if (doExportABC) {
			NResource::commandLine_ = true;
			mainWidget->mainFrameWidget()->exportABCImm();
			exit(0);
		}

	    }
	    if (doExportLilyPond) {
		cerr << "\"--export-lilypond\" ignored: file not found" << endl;
	    }
	    if (doExportMusixTeX) {
		cerr << "\"--export-musixtex\" ignored: file not found" << endl;
	    }
	    if (doExportABC) {
		cerr << "\"--export-abc\" ignored: file not found" << endl;
	    }
	    break;
    default: KCmdLineArgs::usage(); break;
    }
    args->clear(); // Free up memory.
    // Start main window with (default or saved) size
    QDesktopWidget desktop;
    // Default size: dependant on desktop
    QRect mainWinPos( desktop.availableGeometry() );
    // Read saved window positions
    kapp->config()->setGroup("Startup");
    mainWinPos = kapp->config()->readRectEntry( "WindowPos", &mainWinPos );
    // Set geometry of main window to these settings
    mainWidget->setGeometry( mainWinPos  );
    // Restore Toolbar settings
    nr->readToolbarSettings(mainWidget->toolBarIterator());
    //a.setMainWidget( mainWidget );
    mainWidget->show();
#if KDE_VERSION >= 220
	KTipDialog::showTip(locate("data", "noteedit/tips"));
#else
	kapp->config()->setGroup("TipOfDay");
	nr->loadTips(locate("data", "noteedit/tips"));
	if (kapp->config()->readBoolEntry("RunOnStart", true))	tipFrm( 0, NResource::tipNo_ );
#endif
    b.changeSplashMessage( i18n("Ready.") );
    b.start();
    int res = a.exec();
    // Save mainframe widget window settings
    kapp->config()->setGroup("Startup");
    kapp->config()->writeEntry("WindowPos", mainWidget->geometry() );
    //delete nr;
    return res;
}
