#ifndef NSCALEEDIT_H
#define NSCALEEDIT_H
#include <qslider.h>
#include "scaleedit.h"

class NScaleEdit : public scaleEdit
{ 
    Q_OBJECT

public:
    NScaleEdit(  QWidget* parent = 0, const char* name = 0, WFlags fl = 0 );
    ~NScaleEdit();
    int getValue() {return ruler->value();}
    void setMinVal(int minval) {ruler->setMinValue(minval);}
    void setMaxVal(int maxval) {ruler->setMaxValue(maxval);}
    void setStartVal(int startval) {ruler->setValue(startval); setEditValue(startval);}
    void setAll(int minval, int maxval, int startval) {
	ruler->setMinValue(minval);
	ruler->setMaxValue(maxval);
	ruler->setValue(startval);
	setEditValue(startval);
    }

public slots:
    void changeSliderPos(const QString&);
    void setEditValue(int);
private:
    QWidget* parent_;

};

#endif // NSCALEEDIT_H
