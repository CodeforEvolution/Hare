#include "EncoderListView.h"

#include <string.h>

#include <Debug.h>
#include <ListView.h>
#include <Message.h>
#include <Rect.h>

#include <ColumnListView.h>
#include <ColumnTypes.h>

#include "AppDefs.h"
#include "AppView.h"
#include "CheckMark.h"
#include "CommandConstants.h"
#include "GUIStrings.h"
#include "Settings.h"

EncoderListView::EncoderListView(BRect frame)
	:
	BColumnListView(frame, "listView", B_FOLLOW_ALL,
		B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE | B_NAVIGABLE_JUMP,
		B_FANCY_BORDER, true)
{
	PRINT(("EncoderListView::EncoderListView(BRect)\n"));

	SetSelectionMode(B_MULTIPLE_SELECTION_LIST);
	SetColumnFlags(B_ALLOW_COLUMN_RESIZE);

	InitView();
}

EncoderListView::~EncoderListView()
{
	PRINT(("EncoderListView::~EncoderListView()\n"));

	delete checkMark;

/*	int32 columnDisplayOrder[NUM_OF_COLUMNS];
	int32 numOfSortKeys;
	int32 sortKeys[NUM_OF_COLUMNS];
	CLVSortMode sortModes[NUM_OF_COLUMNS]; */

/*	GetDisplayOrder(columnDisplayOrder);
	numOfSortKeys = GetSorting(sortKeys, sortModes); */

/*	settings->SetColumnDisplayOrder(columnDisplayOrder);
	settings->SetNumberOfSortKeys(numOfSortKeys);
	settings->SetSortKeys(sortKeys);
	settings->SetSortModes(sortModes); */

	bool columnsShown[NUM_OF_COLUMNS];
	float columnWidths[NUM_OF_COLUMNS];
	for (int i = 0; i < NUM_OF_COLUMNS; i++) {
		BColumn* column = ColumnAt(i);
		columnsShown[i] = column->IsVisible();
		columnWidths[i] = column->Width();
	}
	settings->SetColumnsShown(columnsShown);
	settings->SetColumnWidths(columnWidths);
}

void
EncoderListView::InitView()
{
	PRINT(("EncoderListView::InitView()\n"));

	checkMark = new CheckMark();

	float minWidth;
	float maxWidth = 1000;
	int32 truncate = 3;
	
	AddColumn(new BBitmapColumn("", 16, 16, 16, B_ALIGN_CENTER), 0);
	
	minWidth = StringWidth(FILE_COLUMN) + 20;
	AddColumn(new BStringColumn(FILE_COLUMN, minWidth, minWidth, maxWidth, truncate, B_ALIGN_LEFT), 1);

	minWidth = StringWidth(SAVE_AS_COLUMN) + 20;
	AddColumn(new BStringColumn(SAVE_AS_COLUMN, minWidth, minWidth, maxWidth, truncate, B_ALIGN_LEFT), 2);

	minWidth = StringWidth(ARTIST_COLUMN) + 20;
	AddColumn(new BStringColumn(ARTIST_COLUMN, minWidth, minWidth, maxWidth, truncate, B_ALIGN_LEFT), 3);

	minWidth = StringWidth(ALBUM_COLUMN) + 20;
	AddColumn(new BStringColumn(ALBUM_COLUMN, minWidth, minWidth, maxWidth, truncate, B_ALIGN_LEFT), 4);

	minWidth = StringWidth(TITLE_COLUMN) + 20;
	AddColumn(new BStringColumn(TITLE_COLUMN, minWidth, minWidth, maxWidth, truncate, B_ALIGN_LEFT), 5);

	minWidth = StringWidth(YEAR_COLUMN) + 20;
	AddColumn(new BStringColumn(YEAR_COLUMN, minWidth, minWidth, maxWidth, truncate, B_ALIGN_LEFT), 6);

	minWidth = StringWidth(COMMENT_COLUMN) + 20;
	AddColumn(new BStringColumn(COMMENT_COLUMN, minWidth, minWidth, maxWidth, truncate, B_ALIGN_LEFT), 7);

	minWidth = StringWidth(TRACK_COLUMN) + 20;
	AddColumn(new BStringColumn(TRACK_COLUMN, minWidth, minWidth, maxWidth, truncate, B_ALIGN_LEFT), 8);

	minWidth = StringWidth(GENRE_COLUMN) + 20;
	AddColumn(new BStringColumn(GENRE_COLUMN, minWidth, minWidth, maxWidth, truncate, B_ALIGN_LEFT), 9);

//	SetDisplayOrder(settings->ColumnDisplayOrder());

/*	SetSorting(settings->NumberOfSortKeys(), settings->SortKeys(),
			   settings->SortModes()); */

	bool* columnsShown = settings->ColumnsShown();
	float* columnWidths = settings->ColumnWidths();
	for (int i = 0; i < NUM_OF_COLUMNS; i++) {
		BColumn* column = ColumnAt(i);
		column->SetVisible(columnsShown[i]);
		if (columnWidths[i] > 0.0) {
			column->SetWidth(columnWidths[i]);
		}
	}

//	SetSortFunction(sort_function);
}

void
EncoderListView::AttachedToWindow()
{
	PRINT(("EncoderListView::AttachedToWindow()\n"));
}

void
EncoderListView::MessageReceived(BMessage* message)
{
	//PRINT(("EncoderListView::MessageReceived(BMessage*)\n"));

	switch (message->what) {
		default:
			BColumnListView::MessageReceived(message);
	}
}

void
EncoderListView::RefsReceived(BMessage* message)
{
	//The logic is over in AppView for any refs
	AppView* view;
	view->RefsReceived(message);
}

BBitmap*
EncoderListView::GetCheckMark()
{
	return checkMark;
}

/*
int
sort_function(const CLVListItem* item1, const CLVListItem* item2,
	int32 sort_key)
{
	int result = -1;

	const char* item1Text =
		((CLVRefListItem*)item1)->GetColumnContentText(sort_key);

	const char* item2Text =
		((CLVRefListItem*)item2)->GetColumnContentText(sort_key);

	if (item1Text && item2Text) {
		result = strcmp(item1Text, item2Text);
	} else if (!item1Text && !item2Text) {
		result = 0;
	} else if (!item1Text) {
		result = -1;
	} else if (!item2Text) {
		result = 1;
	}

	return(result);
}
*/

