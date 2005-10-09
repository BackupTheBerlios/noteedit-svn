#include "staffProps.h"

class NMainFrameWidget;
class NStaff;
class NVoice;
class VoiceBox;
class QString;

class staffPropFrm : public staffPropForm {

    Q_OBJECT
    
    public:
	staffPropFrm( QWidget *parent = 0 );
	~staffPropFrm();
        void setValuesFromActualStaff(int staffNr);
	bool destroyVoice(VoiceBox *rem_box, NVoice *voice);
	void boot( int staffNr, QPtrList<NStaff> * );

    private slots:
	void refresh();
	void slotStaffCancel();
	void slotStaffOk();
        void slotStaffNameActivated(int staffNr);
        void slotCreateVoice();
	void slotPropertyChanged();

    private:
        QPtrList<NStaff> *staffList_;
        QPtrList <VoiceBox> voiceList_;
	NStaff *actualStaff_;
	NMainFrameWidget *mw_;
	QString *staffNames_;
	bool mbApply;
	int *values_;
	int currentStaffNr_;
};
