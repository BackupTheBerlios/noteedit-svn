#ifndef SCORE_EDITOR_H

#define SCORE_EDITOR_H

#include <config.h>
#ifdef ANTHEM_PLUGIN

#include <anthem/phraseeditors/PhraseEditor.h>
class NMainFrameWidget;
class NResource;

class ScoreEditor : public PhraseEditorBase
{
    public:

        ScoreEditor(TSE3::PhraseEdit *phraseEdit, KToolBar *toolbar,
                   QWidget *parent);
        virtual ~ScoreEditor();
    protected:
	virtual void resizeEvent ( QResizeEvent * evt);
    private:
	NMainFrameWidget *mainWidget_;
	static NResource *resource_;
};

#endif /* ANTHEM_PLUGIN */
#endif /* SCORE_EDITOR_H */
