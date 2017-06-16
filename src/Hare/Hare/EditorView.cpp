#include "EditorView.h"

#include <stdlib.h>
#include <string.h>

#include <Button.h>
#include <CheckBox.h>
#include <Debug.h>
#include <Entry.h>
#include <List.h>
#include <ListView.h>
#include <Menu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Message.h>
#include <Messenger.h>
#include <OptionPopUp.h>
#include <RadioButton.h>
#include <Rect.h>
#include <TextControl.h>

#include <ColumnTypes.h>

#include "GenreList.h"

#include "AppDefs.h"
#include "CommandConstants.h"
#include "GUIStrings.h"
#include "RefRow.h"

EditorView::EditorView(BRect frame)
	:
	BBox(frame, "editorView")
{
	PRINT(("EditorView::EditorView(BRect)\n"));
}

EditorView::~EditorView()
{
	PRINT(("EditorView::~EditorView()\n"));
}

void
EditorView::InitView()
{
	PRINT(("EditorView::InitView()\n"));

	SetLabel(EDITOR_LABEL);

	int space = 6;

	numSelected = 0;
	selectedIndexes = 0;

	font_height fh;
	GetFontHeight(&fh);
	float height = fh.ascent + fh.descent + fh.leading;

	char* left[] = {
		ARTIST_COLUMN,
		ALBUM_COLUMN,
		TITLE_COLUMN,
		TRACK_COLUMN,
		YEAR_COLUMN,
		0
	};
	char* right[] = {
		COMMENT_COLUMN,
		GENRE_COLUMN,
		0
	};
	float leftWidth = 0;
	float rightWidth = 400;

	int i = 0;

	/*
		while(left[i])
		{
			if(StringWidth(left[i]) > leftWidth)
			{
				leftWidth = StringWidth(left[i]);
			}
			i++;
		}

		i=0;
		while(right[i])
		{
			if(StringWidth(right[i]) > rightWidth)
			{
				rightWidth = StringWidth(right[i]);
			}
			i++;
		}
	*/

	//use cb to determine spacing of box at different fonts
	BCheckBox cb(BRect(0, 0, 0, 0), 0, 0, 0);
	cb.ResizeToPreferred();

	BRect leftCB = Bounds();
	leftCB.InsetBy(space, 2 * space);
	leftCB.right = 200; //leftCB.left + leftWidth + cb.Frame().Width();
	leftCB.bottom = 100; //leftCB.top + height;

	BRect leftTC = leftCB;
	leftTC.left = leftCB.right + space / 2;
	leftTC.right = leftTC.left + 110;

	BRect rightCB = leftCB;
	rightCB.left = leftTC.right + space;
	rightCB.right = rightCB.left + rightWidth + cb.Frame().Width();

	BRect rightTC = rightCB;
	rightTC.left = rightCB.right + space / 2;
	rightTC.right = rightTC.left + 110;

	artistCheckBox = new BCheckBox(leftCB, "artistCB", ARTIST_COLUMN,
								   new BMessage(MSG_ARTIST_CB));
	artistTextControl = new BTextControl(leftTC, "artistTC", 0, "", 0);

	leftCB.OffsetBy(0, height + space);
	leftTC.OffsetBy(0, height + space);

	albumCheckBox = new BCheckBox(leftCB, "albumCB", ALBUM_COLUMN,
								  new BMessage(MSG_ALBUM_CB));
	albumTextControl = new BTextControl(leftTC, "albumTC", 0, "", 0);

	leftCB.OffsetBy(0, height + space);
	leftTC.OffsetBy(0, height + space);

	titleCheckBox = new BCheckBox(leftCB, "titleCB", TITLE_COLUMN,
								  new BMessage(MSG_TITLE_CB));
	titleTextControl = new BTextControl(leftTC, "titleTC", 0, "", 0);

	leftCB.OffsetBy(0, height + space);
	leftTC.OffsetBy(0, height + space);

	trackCheckBox = new BCheckBox(leftCB, "trackCB", TRACK_COLUMN,
								  new BMessage(MSG_TRACK_CB));
	trackTextControl = new BTextControl(leftTC, "trackTC", 0, "", 0);

	leftCB.OffsetBy(0, height + space);
	leftTC.OffsetBy(0, height + space);

	yearCheckBox = new BCheckBox(leftCB, "yearCB", YEAR_COLUMN,
								 new BMessage(MSG_YEAR_CB));
	yearTextControl = new BTextControl(leftTC, "yearTC", 0, "", 0);

	commentCheckBox = new BCheckBox(rightCB, "commentCB", COMMENT_COLUMN,
									new BMessage(MSG_COMMENT_CB));
	commentTextControl = new BTextControl(rightTC, "commentTC", 0, "", 0);

	rightCB.OffsetBy(0, height + space);
	rightTC.OffsetBy(0, height + space);

	genreCheckBox = new BCheckBox(rightCB, "genreCB", GENRE_COLUMN,
								  new BMessage(MSG_GENRE_CB));

	rightTC.top += (10 - space);
	rightTC.bottom = 150; // rightTC.top + (5*space) + (2*height);
	genreBox = new BBox(rightTC, "genreBox");

	// INSIDE THE BOX
	{
		BMenu* menu = new BMenu(GENRE_COLUMN);
		menu->SetLabelFromMarked(true);
		int genres = GenreList::NumGenres();
		BList genreList;
		for (int i = 0; i < genres; i++) {
			char* genre = GenreList::Genre(i);
			genreList.AddItem(genre);
		}

		genreList.SortItems(&GenreList::GenreSort);
		char last = 'A';
		char current;
		for (int i = 0; i < genres; i++) {
			char* genre = (char*)genreList.ItemAt(i);
			current = genre[0];
			if (current != last) {
				menu->AddSeparatorItem();
			}
			menu->AddItem(new BMenuItem(genre,
										new BMessage(GENRE_SEL_CHNGD)));
			last = current;
		}

		BRect ctrlFrame = genreBox->Bounds();
		ctrlFrame.InsetBy(space, space);
		ctrlFrame.bottom = ctrlFrame.top + height;
		genreMenuField = new BMenuField(ctrlFrame, "genreMenuField", 0, menu);

		ctrlFrame.OffsetBy(0, height + (2 * space));
		genreTextControl = new BTextControl(ctrlFrame, "genreTextControl", 0, "", 0);
	}
	// DONE INSIDE THE BOX

	BRect buttonFrame = Bounds();
	buttonFrame.InsetBy(2 * space, 2 * space);
	buttonFrame.top = buttonFrame.bottom - 25;
	buttonFrame.left = buttonFrame.right - 60;
	applyButton = new BButton(buttonFrame, "applyButton", APPLY_BTN,
							  new BMessage(APPLY_MSG), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);

	AddChild(artistCheckBox);
	AddChild(artistTextControl);

	AddChild(albumCheckBox);
	AddChild(albumTextControl);

	AddChild(titleCheckBox);
	AddChild(titleTextControl);

	AddChild(trackCheckBox);
	AddChild(trackTextControl);

	AddChild(yearCheckBox);
	AddChild(yearTextControl);

	AddChild(commentCheckBox);
	AddChild(commentTextControl);

	AddChild(genreCheckBox);
	AddChild(genreBox);
	genreBox->AddChild(genreMenuField);
	genreBox->AddChild(genreTextControl);

	AddChild(applyButton);

	ResizeToPreferred();
}

void
EditorView::AttachedToWindow()
{
	PRINT(("EditorView::AttachedToWindow()\n"));

	InitView();

	artistCheckBox->SetTarget(this);
	albumCheckBox->SetTarget(this);
	titleCheckBox->SetTarget(this);
	yearCheckBox->SetTarget(this);
	commentCheckBox->SetTarget(this);
	trackCheckBox->SetTarget(this);
	genreCheckBox->SetTarget(this);
	genreMenuField->Menu()->SetTargetForItems(this);
	applyButton->SetTarget(this);

	artistCheckBox->SetValue(B_CONTROL_OFF);
	albumCheckBox->SetValue(B_CONTROL_OFF);
	titleCheckBox->SetValue(B_CONTROL_OFF);
	yearCheckBox->SetValue(B_CONTROL_OFF);
	commentCheckBox->SetValue(B_CONTROL_OFF);
	trackCheckBox->SetValue(B_CONTROL_OFF);
	genreCheckBox->SetValue(B_CONTROL_OFF);

	EnableCheckBoxes(false);
	applyButton->SetEnabled(false);
	applyButton->MakeDefault(true);

	SetEnabled(artistCheckBox, artistTextControl);
	SetEnabled(albumCheckBox, albumTextControl);
	SetEnabled(titleCheckBox, titleTextControl);
	SetEnabled(yearCheckBox, yearTextControl);
	SetEnabled(commentCheckBox, commentTextControl);
	SetEnabled(trackCheckBox, trackTextControl);
	SetEnabled(genreCheckBox, genreMenuField);
	SetEnabled(genreCheckBox, genreTextControl);
}

void
EditorView::GetPreferredSize(float* width, float* height)
{
	PRINT(("EditorView::GetPreferredSize(float* float*)\n"));

	int space = 6;

	*width = commentTextControl->Frame().right + space;
	*height = yearTextControl->Frame().bottom + space;
	float ht = 100; //genreBox->Frame().bottom + 3*space +
	//	applyButton->Frame().Height();
	if (ht > *height) {
		*height = ht;
	}
}

void
EditorView::Apply()
{
	PRINT(("EditorView::Apply()\n"));

	BMessage message(APPLY_ATTRIBUTE_CHANGES);

	if (artistCheckBox->Value() == B_CONTROL_ON) {
		message.AddString("artist", artistTextControl->Text());
	}

	if (albumCheckBox->Value() == B_CONTROL_ON) {
		message.AddString("album", albumTextControl->Text());
	}

	if (titleCheckBox->Value() == B_CONTROL_ON) {
		message.AddString("title", titleTextControl->Text());
	}

	if (yearCheckBox->Value() == B_CONTROL_ON) {
		message.AddString("year", yearTextControl->Text());
	}

	if (commentCheckBox->Value() == B_CONTROL_ON) {
		message.AddString("comment", commentTextControl->Text());
	}

	if (trackCheckBox->Value() == B_CONTROL_ON) {
		message.AddString("track", trackTextControl->Text());
	}

	if (genreCheckBox->Value() == B_CONTROL_ON) {
		if (strcmp(genreTextControl->Text(), "") != 0) {
			message.AddString("genre", genreTextControl->Text());
		} else {
			BMenu* menu = genreMenuField->Menu();
			BMenuItem* item = menu->FindMarked();
			message.AddString("genre", item->Label());
		}
	}

	for (int i = 0; i < numSelected; i++) {
		message.AddInt32("index", selectedIndexes[i]);
	}

	BMessenger parentMessenger(Parent());
	parentMessenger.SendMessage(&message);
}

void
EditorView::ListSelectionChanged(BMessage* message)
{
	PRINT(("EditorView::ListSelectionChanged(BMessage*)\n"));

	free(selectedIndexes);

	BColumnListView* colListView;
	message->FindPointer("source", (void**)&colListView);

	type_code index_type;
	message->GetInfo("index", &index_type, &numSelected);

	selectedIndexes = (int32*)malloc(numSelected * sizeof(int32));

	for (int i = 0; i < numSelected; i++) {
		message->FindInt32("index", i, &(selectedIndexes[i]));
	}

	int32 index;
	message->FindInt32("index", &index);

	if (numSelected == 0) {
		artistCheckBox->SetValue(B_CONTROL_OFF);
		albumCheckBox->SetValue(B_CONTROL_OFF);
		titleCheckBox->SetValue(B_CONTROL_OFF);
		yearCheckBox->SetValue(B_CONTROL_OFF);
		commentCheckBox->SetValue(B_CONTROL_OFF);
		trackCheckBox->SetValue(B_CONTROL_OFF);
		genreCheckBox->SetValue(B_CONTROL_OFF);

		EnableCheckBoxes(false);
		applyButton->SetEnabled(false);
		SetControlValues(0);
	} else if (numSelected == 1) {
		artistCheckBox->SetValue(B_CONTROL_ON);
		albumCheckBox->SetValue(B_CONTROL_ON);
		titleCheckBox->SetValue(B_CONTROL_ON);
		yearCheckBox->SetValue(B_CONTROL_ON);
		commentCheckBox->SetValue(B_CONTROL_ON);
		trackCheckBox->SetValue(B_CONTROL_ON);
		genreCheckBox->SetValue(B_CONTROL_ON);

		EnableCheckBoxes(true);
		applyButton->SetEnabled(true);

		BRefRow* row = (BRefRow*)colListView->RowAt(index);
		SetControlValues(row);
	} else {
		artistCheckBox->SetValue(B_CONTROL_ON);
		albumCheckBox->SetValue(B_CONTROL_ON);
		titleCheckBox->SetValue(B_CONTROL_OFF);
		yearCheckBox->SetValue(B_CONTROL_ON);
		commentCheckBox->SetValue(B_CONTROL_ON);
		trackCheckBox->SetValue(B_CONTROL_OFF);
		genreCheckBox->SetValue(B_CONTROL_ON);

		EnableCheckBoxes(true);
		applyButton->SetEnabled(true);

		BRefRow* row = (BRefRow*)colListView->RowAt(index);
		SetControlValues(row);
	}

	SetEnabled(artistCheckBox, artistTextControl);
	SetEnabled(albumCheckBox, albumTextControl);
	SetEnabled(titleCheckBox, titleTextControl);
	SetEnabled(yearCheckBox, yearTextControl);
	SetEnabled(commentCheckBox, commentTextControl);
	SetEnabled(trackCheckBox, trackTextControl);
	SetEnabled(genreCheckBox, genreMenuField);
	SetEnabled(genreCheckBox, genreTextControl);
}

void
EditorView::EnableCheckBoxes(bool value)
{
	artistCheckBox->SetEnabled(value);
	albumCheckBox->SetEnabled(value);
	titleCheckBox->SetEnabled(value);
	yearCheckBox->SetEnabled(value);
	commentCheckBox->SetEnabled(value);
	trackCheckBox->SetEnabled(value);
	genreCheckBox->SetEnabled(value);
}

void
EditorView::SetControlValues(BRefRow* row)
{
	artistTextControl->SetText("");
	albumTextControl->SetText("");
	titleTextControl->SetText("");
	yearTextControl->SetText("");
	commentTextControl->SetText("");
	trackTextControl->SetText("");
	genreTextControl->SetText("");
	
	BStringField* tmpStringField;

	if (row) {
		tmpStringField = (BStringField*)row->GetField(ARTIST_COLUMN_INDEX);
		const char* artist = tmpStringField->String();
		if (!artist) artist = "";
		tmpStringField = (BStringField*)row->GetField(ALBUM_COLUMN_INDEX);
		const char* album = tmpStringField->String();
		if (!album) album = "";
		tmpStringField = (BStringField*)row->GetField(TITLE_COLUMN_INDEX);
		const char* title = tmpStringField->String();
		if (!title) title = "";
		tmpStringField = (BStringField*)row->GetField(YEAR_COLUMN_INDEX);
		const char* year = tmpStringField->String();
		if (!year) year = "";
		tmpStringField = (BStringField*)row->GetField(COMMENT_COLUMN_INDEX);
		const char* comment = tmpStringField->String();
		if (!comment) comment = "";
		tmpStringField = (BStringField*)row->GetField(TRACK_COLUMN_INDEX);
		const char* track = tmpStringField->String();
		if (!track) track = "";
		tmpStringField = (BStringField*)row->GetField(GENRE_COLUMN_INDEX);
		const char* genre = tmpStringField->String();
		if (!genre) genre = "";

		artistTextControl->SetText(artist);
		albumTextControl->SetText(album);
		titleTextControl->SetText(title);
		yearTextControl->SetText(year);
		commentTextControl->SetText(comment);
		trackTextControl->SetText(track);

		BMenu* menu = genreMenuField->Menu();
		BMenuItem* menuItem = menu->FindItem(genre);

		if (menuItem && (strcmp(genre, "") != 0)) {
			menuItem->SetMarked(true);
		} else {
			menuItem = menu->FindItem("Other");
			menuItem->SetMarked(true);
			genreTextControl->SetText(genre);
		}
	}
}

void
EditorView::MakeFocus(bool focused)
{
	PRINT(("EditorView::MakeFocus(bool)\n"));

	artistCheckBox->MakeFocus(focused);
}

void
EditorView::SetEnabled(bool value)
{
	PRINT(("EditorView::SetEnabled(bool)\n"));

	EnableCheckBoxes(value);
	artistTextControl->SetEnabled(value);
	albumTextControl->SetEnabled(value);
	titleTextControl->SetEnabled(value);
	yearTextControl->SetEnabled(value);
	commentTextControl->SetEnabled(value);
	trackTextControl->SetEnabled(value);
	genreTextControl->SetEnabled(value);
	genreMenuField->SetEnabled(value);
	applyButton->SetEnabled(value);

	if (value) {
		SetEnabled(artistCheckBox, artistTextControl);
		SetEnabled(albumCheckBox, albumTextControl);
		SetEnabled(titleCheckBox, titleTextControl);
		SetEnabled(yearCheckBox, yearTextControl);
		SetEnabled(commentCheckBox, commentTextControl);
		SetEnabled(trackCheckBox, trackTextControl);
		SetEnabled(genreCheckBox, genreMenuField);
		SetEnabled(genreCheckBox, genreTextControl);
	}
}

void
EditorView::SetEnabled(BCheckBox* checkbox, BControl* control)
{
	PRINT(("EditorView::SetEnabled(BCheckBox*,BControl*)\n"));

	if (checkbox->Value() == B_CONTROL_ON) {
		control->SetEnabled(true);
	} else {
		control->SetEnabled(false);
	}
}

void
EditorView::SetEnabled(BCheckBox* checkbox, BMenuField* menufield)
{
	PRINT(("EditorView::SetEnabled(BCheckBox*,BMenuField*)\n"));

	if (checkbox->Value() == B_CONTROL_ON) {
		menufield->SetEnabled(true);
	} else {
		menufield->SetEnabled(false);
	}
}

void
EditorView::MessageReceived(BMessage* message)
{
	//PRINT(("EditorView::MessageReceived(BMessage*)\n"));

	switch (message->what) {
		case LIST_SELECTION_MSG:
			ListSelectionChanged(message);
			break;
		case APPLY_MSG:
			Apply();
			break;
		case MSG_ARTIST_CB:
			SetEnabled(artistCheckBox, artistTextControl);
			break;
		case MSG_ALBUM_CB:
			SetEnabled(albumCheckBox, albumTextControl);
			break;
		case MSG_TITLE_CB:
			SetEnabled(titleCheckBox, titleTextControl);
			break;
		case MSG_YEAR_CB:
			SetEnabled(yearCheckBox, yearTextControl);
			break;
		case MSG_COMMENT_CB:
			SetEnabled(commentCheckBox, commentTextControl);
			break;
		case MSG_TRACK_CB:
			SetEnabled(trackCheckBox, trackTextControl);
			break;
		case MSG_GENRE_CB:
			SetEnabled(genreCheckBox, genreMenuField);
			SetEnabled(genreCheckBox, genreTextControl);
			break;
		case GENRE_SEL_CHNGD:
			genreTextControl->SetText("");
			break;
		default:
			BBox::MessageReceived(message);
	}
}
