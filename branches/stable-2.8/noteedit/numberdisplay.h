#ifndef NUMBERDISPLAY_H

#define NUMBERDISPLAY_H
#include <kaction.h>
#include <qlcdnumber.h> 

class NLCDNumber;

class NNumberDisplay : public KAction {
	Q_OBJECT
	public :
		NNumberDisplay(int min, int max, const QString& text, int accel, QObject* parent, const char* name);
		int getVal();
		void setVal(int val);
		void setMin(int min);
		void setMax(int max);
		virtual int plug( QWidget *w, int index = -1 );
		void emitValueChanged(int val);
		bool isZero();
	signals:
		void valChanged(int val);
	private:
		NLCDNumber *number_;
		int min_, max_;
		QString toolTip_;
};

class NLCDNumber : public QLCDNumber {
	public:
		NLCDNumber (int min, int max, NNumberDisplay *numDisplay, QWidget * parent=0, const char * name=0);
		void setRealValue(int val);
		int getRealValue();
		void setMin(int min);
		void setMax(int max);
	protected:
		virtual void mousePressEvent(QMouseEvent *evt); 
	private:
		int min_, max_;
		NNumberDisplay *numDisplay_;
		int realValue_;
};
		
#endif
