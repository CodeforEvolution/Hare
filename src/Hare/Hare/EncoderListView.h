#ifndef __ENCODER_LIST_VIEW_H__
#define __ENCODER_LIST_VIEW_H__

#include <ColumnListView.h>

class BBitmap;
class BMessage;
class BRect;

class EncoderListView : public BColumnListView {
public:
	EncoderListView(BRect frame);
	~EncoderListView();
	virtual void AttachedToWindow();
	virtual void MessageReceived(BMessage* message);
	virtual void RefsReceived(BMessage* message);
	BBitmap* GetCheckMark();
private:
	void InitView();
	BBitmap* checkMark;
};

/*int sort_function(const CLVListItem* item1, const CLVListItem* item2,
				  int32 sort_key);*/

#endif
