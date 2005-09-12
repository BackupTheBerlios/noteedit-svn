#include "scaleedit_impl.h"
#include <qstring.h>
#include <qgroupbox.h>
#include <qlineedit.h>

/* 
 *  Constructs a NScaleEdit which is a child of 'parent', with the 
 *  name 'name' and widget flags set to 'f' 
 */
NScaleEdit::NScaleEdit(  QWidget* parent,  const char* name, WFlags fl )
    : scaleEdit( parent, name, fl )
{
	parent_ = parent;
	connect( text, SIGNAL( returnPressed() ), this, SLOT( slotReturnPressed()));
}


/*  
 *  Destroys the object and frees any allocated resources
 */
NScaleEdit::~NScaleEdit()
{
    // no need to delete child widgets, Qt does it all for us
}

/* 
 * public slot
 */
void NScaleEdit::changeSliderPos(const QString& s)
{
	bool ok; 
	int val;

	val = s.toInt(&ok);
	if (!ok) return;
	if (val < ruler->minValue() || val > ruler->maxValue()) return;
	ruler->setValue(val);
}

void NScaleEdit::slotReturnPressed()
{
  emit returnPressed();
}

/* 
 * public slot
 */
void NScaleEdit::setEditValue(int val)
{
	QString s;
	if (val < ruler->minValue() || val > ruler->maxValue()) return;
	s.sprintf("%d", val);
	text->setText(s);
	emit valueChanged(val);
}

