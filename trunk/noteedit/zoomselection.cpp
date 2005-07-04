#include "zoomselection.h"
#include "mainframewidget.h"


int NZoomSelection::zoomtab[] = {25, 30, 35, 40, 45, 50, 60, 70, 75, 80, 90, 100, 125, 150, 175, 200, 250, 300, -1};

NZoomSelection::NZoomSelection( const QString& text, int accel, QObject* parent, const char* name, NMainFrameWidget *mainWidget) :
		KAction(text, accel, parent, name) {
	mainWidget_ = mainWidget;
}

void NZoomSelection::zoomIn() {
	int zidx;
	zidx = zvalues_->currentItem();
	if (zidx < 0 || zidx >= (sizeof(zoomtab) / sizeof(int) - 2)) return;
	zidx++;
	zvalues_->setCurrentItem(zidx);
	mainWidget_->changeZoomValue(zidx);
}

void NZoomSelection::zoomOut() {
	int zidx;
	zidx = zvalues_->currentItem();
	if (zidx < 1 || zidx >= (sizeof(zoomtab) / sizeof(int) - 1)) return;
	zidx--;
	zvalues_->setCurrentItem(zidx);
	mainWidget_->changeZoomValue(zidx);
}
	
	
int NZoomSelection::chooseZoomVal(int zoomfac) {
	int i;
	for (i = 0; zoomtab[i] >= 0; i++) {
		if (zoomfac <= zoomtab[i]) return i;
	}
	return PREFERRED_ZOOM_VAL;
}

float NZoomSelection::computeZoomVal(int zidx) {
	if (zidx < 0 || zidx >= (sizeof(zoomtab) / sizeof(int) - 1)) {
		NResource::abort("computeZoomVal: internal error");
	}
	return (float) ((double) zoomtab[zidx] / 200.0);
}

int NZoomSelection::index2ZoomVal(int zidx) {
	if (zidx < 0 || zidx >= (sizeof(zoomtab) / sizeof(int) - 1)) {
		NResource::abort("index2ZoomVal: internal error");
	}
	return zoomtab[zidx];
}

int NZoomSelection::plug( QWidget *w, int index) {
	QString s;
	int i;
	zvalues_ = new QComboBox(w);
	zvalues_->setFocusPolicy(QWidget::NoFocus);
	zvalues_->setMaximumSize (80, 1000);
	for (i = 0; zoomtab[i] >= 0; i++) {
		s.sprintf("%d%%", zoomtab[i]);
		zvalues_->insertItem(s);
	}
	zvalues_->setCurrentItem(NResource::defZoomval_);
	connect(zvalues_, SIGNAL(activated(int)), mainWidget_, SLOT(changeZoomValue(int))); 
	return 0;
}
