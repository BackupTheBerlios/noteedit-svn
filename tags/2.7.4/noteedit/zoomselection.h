#ifndef ZOOMSELECTION_H

#define ZOOMSELECTION_H
#include <qcombobox.h>
#include <kaction.h>
class NMainFrameWidget;


class NZoomSelection : public KAction {
	public:
		NZoomSelection( const QString& text, int accel, QObject* parent, const char* name, NMainFrameWidget *mainWidget);
		virtual int plug( QWidget *w, int index = -1 );
		static int chooseZoomVal(int zoomfac);
		static int index2ZoomVal(int zidx);
		float computeZoomVal(int zidx);
		void zoomIn();
		void zoomOut();
	private:
		QComboBox *zvalues_;
		NMainFrameWidget *mainWidget_;
		static int zoomtab[];
};


#endif /* ZOOMSELECTION_H */
