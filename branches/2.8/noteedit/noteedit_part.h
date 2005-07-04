#ifndef __noteedit_part_h__
#define __noteedit_part_h__

#include <kparts/browserextension.h>
#include <klibloader.h>

#include "resource.h"

class KAboutData;
class KInstance;
class NoteeditBrowserExtension;
class NMainFrameWidget;

class NoteeditFactory : public KLibFactory {
    Q_OBJECT
public:
    NoteeditFactory();
    virtual ~NoteeditFactory();


#if KDE_VERSION >= 290
    virtual QObject* createObject(QObject* parent = 0, const char* name = 0,
                            const char* classname = "QObject",
                            const QStringList &args = QStringList());
#else
    virtual QObject* create(QObject* parent = 0, const char* name = 0,
                            const char* classname = "QObject",
                            const QStringList &args = QStringList());
#endif

    static KInstance *instance();
    static KAboutData *aboutData();

private:
    static KInstance *s_instance;
};

class NoteeditPart: public KParts::ReadOnlyPart {
    Q_OBJECT
public:
    NoteeditPart(QWidget *parent, const char *name);
    virtual ~NoteeditPart();

    virtual bool closeURL();

protected:
    virtual bool openFile();

private:
    NMainFrameWidget *mainWidget_;
    NoteeditBrowserExtension *m_extension;
	NResource *nr;
private slots:
    void playStart();
};

class NoteeditBrowserExtension : public KParts::BrowserExtension {
    Q_OBJECT
    friend class NoteeditPart;
public:
    NoteeditBrowserExtension(NoteeditPart *parent);
    virtual ~NoteeditBrowserExtension();
};

#endif
