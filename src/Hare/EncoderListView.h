#ifndef __ENCODER_LIST_VIEW_H__
#define __ENCODER_LIST_VIEW_H__

#include <ColumnListView.h>

class BBitmap;
class BMessage;
class BRect;
class BetterScrollView;

class EncoderListView : public BColumnListView {
public:
	EncoderListView(BRect frame);
	~EncoderListView();
	virtual void AttachedToWindow();
	virtual void MessageReceived(BMessage* message);
	BBitmap* GetCheckMark();
private:
	void InitView();
	BBitmap* checkMark;
};

#endif
