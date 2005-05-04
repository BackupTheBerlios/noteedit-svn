#include "noteedit_part.h"
#include "config.h"

#include <kinstance.h>
#include <klocale.h>
#include <kaboutdata.h>

#include "mainframewidget.h"
#include "resource.h"
#include "midimapper.h"

#define KPART_START_WIDTH 800
#define KPART_START_HEIGHT 600
extern "C" {
  /**
   * This function is the 'main' function of this part.  It takes
   * the form 'void *init_lib<library name>()  It always returns a
   * new factory object
   */
  void *init_libnoteedit() {
    return new NoteeditFactory;
  }
}

/**
 * We need one static instance of the factory for our C 'main'
 * function
 */
KInstance *NoteeditFactory::s_instance = 0L;

NoteeditFactory::NoteeditFactory() {}

NoteeditFactory::~NoteeditFactory() {
  if (s_instance)
    {
      delete s_instance->aboutData();
      delete s_instance;
    }

  s_instance = 0;
}

#if KDE_VERSION >= 290
QObject *NoteeditFactory::createObject(QObject *parent, const char *name, const char*,
				 const QStringList& )
#else
QObject *NoteeditFactory::create(QObject *parent, const char *name, const char*,
				 const QStringList& )
#endif
{
  QObject *obj = new NoteeditPart((QWidget*)parent, name);
#if KDE_VERSION < 290
  emit objectCreated(obj);
#endif
  return obj;
}

KInstance *NoteeditFactory::instance() {
  if (! s_instance)
    s_instance = new KInstance(aboutData());
  return s_instance;
}

#include "aboutinfo.h"

KAboutData *NoteeditFactory::aboutData() {
	KAboutData *about = new KAboutData
		("noteedit", I18N_NOOP("NoteEdit"), VERSION,
		 "a KDE3/Qt3 based note editor.\nFor help subscribe to one of the\nmailing lists, or post a message\nin one of the public forums. See",
		 KAboutData::License_GPL, 0, 0,
		 //"http://rnvs.informatik.tu-chemnitz.de/~jan/noteedit/noteedit.html",
		"http://noteedit.berlios.de",
		 ADDRESS);
	about->addAuthor
		("Joerg Anders", I18N_NOOP("Main author and maintainer until September 2004"), ADDRESS);
	about->addAuthor
		("Reinhard Katzmann", I18N_NOOP("project manager"), "reinhard@suamor.de");
	about->addAuthor
		("Christian Fasshauer", I18N_NOOP("programmer"), "mseacf@gmx.net");
#ifdef WITH_TSE3
	about->addAuthor
		("Pete Goodliffe", I18N_NOOP("TSE3 sequencer library"),
		 "pete.goodliffe@pace.co.uk");
#endif
	about->addAuthor("Erik Sigra", I18N_NOOP("developer"), "sigra@home.se");
	about->addAuthor
	("David Faure", I18N_NOOP("KDE User Interface"), "faure@kde.org");
	about->addAuthor("Matt Gerassimoff", 0, "mgeras@telocity.com");
	about->addAuthor
		("Leon Vinken", I18N_NOOP("MusicXML interface"),
		 "leon.vinken@hetnet.nl");
	about->addAuthor("Georg Rudolph", I18N_NOOP("lilypond interface"), "georg.rudolph@schwaben.de");
	about->addAuthor("Matevz Jekovec", I18N_NOOP("developer and composer"), "matevz.jekovec@guest.arnes.si");
	return about;
}

NoteeditPart::NoteeditPart(QWidget *parent, const char *name)
	: KParts::ReadOnlyPart(parent, name) {
	setInstance(NoteeditFactory::instance());

	// create a canvas to insert our widget
	QWidget *canvas = new QWidget(parent);
	canvas->setFocusPolicy(QWidget::ClickFocus);
	setWidget(canvas);

	m_extension = new NoteeditBrowserExtension(this);

	nr = new NResource();
	NResource::mapper_ = new NMidiMapper();
	mainWidget_  = new NMainFrameWidget(actionCollection(), true, canvas);
	mainWidget_ ->setGeometry(0, 0, KPART_START_WIDTH, KPART_START_HEIGHT);
	mainWidget_ ->setFocusPolicy(QWidget::ClickFocus);
	mainWidget_ ->show();
	setXMLFile("noteedit_part.rc");
}


NoteeditPart::~NoteeditPart() {
	delete nr;
  	closeURL();
}

bool NoteeditPart::openFile() {
	int ret;
	if (ret = mainWidget_->loadFile(m_file)) {
		QTimer::singleShot(2000, this, SLOT(playStart()));
	}
	return ret;
}

bool NoteeditPart::closeURL() {
	mainWidget_->playAll(false);
	return true;
}

void NoteeditPart::playStart() {mainWidget_->playAll(true);}

NoteeditBrowserExtension::NoteeditBrowserExtension(NoteeditPart *parent)
  : KParts::BrowserExtension(parent, "NoteeditBrowserExtension") {}

NoteeditBrowserExtension::~NoteeditBrowserExtension() {}
