#include "AppView.h"

#include <string.h>

#include <Alert.h>
#include <Beep.h>
#include <Bitmap.h>
#include <Button.h>
#include <Debug.h>
#include <Font.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <fs_attr.h>
#include <fs_info.h>
#include <image.h>
#include <InterfaceDefs.h>
#include <MenuBar.h>
#include <Message.h>
#include <Node.h>
#include <NodeInfo.h>
#include <NodeMonitor.h>
#include <ObjectList.h>
#include <OS.h>
#include <Path.h>
#include <Rect.h>
#include <ScrollBar.h>
#include <StatusBar.h>
#include <StringView.h>
#include <TextControl.h>
#include <String.h>
#include <Volume.h>
#include <VolumeRoster.h>

#include <ColumnListView.h>
#include <ColumnTypes.h>

#include "AEEncoder.h"
#include "AudioAttributes.h"
#include "GenreList.h"

#include "AppDefs.h"
#include "AppWindow.h"
#include "CommandConstants.h"
#include "CheckMark.h"
#include "EditorView.h"
#include "EncoderListView.h"
#include "FileNamePatternView.h"
#include "GUIStrings.h"
#include "RefRow.h"
#include "Settings.h"
#include "StatusBarFilter.h"

AppView::AppView(BRect frame)
	:
	BView(frame, "AppView", B_FOLLOW_ALL,
		B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP)
{
	PRINT(("AppView::AppView(BRect)\n"));

}

AppView::~AppView()
{
	PRINT(("AppView::~AppView()\n"));

	stop_watching(this);
}

void
AppView::InitView()
{
	PRINT(("AppView::InitView()\n"));

	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	int32 space = 6;

	BRect fileNameFrame = Bounds();
	fileNameFrame.InsetBy(space, space);
	fileNameFrame.bottom = 200;
	fileNameFrame.right = 200;
	fileNamePatternView = new FileNamePatternView(fileNameFrame);
	AddChild(fileNamePatternView);
	fileNameFrame = fileNamePatternView->Frame(); //it resizes itself

	BRect editorFrame = fileNameFrame;
	editorFrame.left = fileNameFrame.right + space;
	editorFrame.right = editorFrame.left + 365;
	editorView = new EditorView(editorFrame);
	AddChild(editorView);
	editorFrame = editorView->Frame(); //it resizes itself

	if (editorFrame.bottom > fileNameFrame.bottom) {
		float diff = editorFrame.bottom - fileNameFrame.bottom;
		fileNamePatternView->ResizeBy(0, diff);
		fileNameFrame = fileNamePatternView->Frame();
	} else if (editorFrame.bottom < fileNameFrame.bottom) {
		float diff = fileNameFrame.bottom - editorFrame.bottom;
		editorView->ResizeBy(0, diff);
		editorFrame = editorView->Frame();
	}

	BRect buttonFrame = Bounds();
	buttonFrame.InsetBy(space, space);
	// buttonFrame.bottom = buttonFrame.bottom - 2;
	buttonFrame.top = buttonFrame.bottom - 30;
	buttonFrame.left = buttonFrame.right - 70;
	encodeButton = new BButton(buttonFrame, "encodeButton", ENCODE_BTN,
							   new BMessage(ENCODE_MSG), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);

	buttonFrame.OffsetBy(-(buttonFrame.Width() + space), 0);
	cancelButton = new BButton(buttonFrame, "cancelButton", CANCEL_BTN,
							   new BMessage(CANCEL_MSG), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);

	BRect listViewFrame = Bounds();
	listViewFrame.left += space;
	listViewFrame.top = fileNameFrame.bottom + (space);
	listViewFrame.right -= (space + B_V_SCROLL_BAR_WIDTH);
	listViewFrame.bottom = buttonFrame.top - (space + B_H_SCROLL_BAR_HEIGHT);
	listView = new EncoderListView(listViewFrame);
	listView->SetSelectionMessage(new BMessage(LIST_SELECTION_MSG));

	BRect statusBarFrame = buttonFrame;
	statusBarFrame.left = Bounds().left + space;
	statusBarFrame.right = buttonFrame.left - space;
	statusBarFrame.top = statusBarFrame.top - 4;
	// statusBarFrame.bottom = statusBarFrame.bottom-4;
	BString remaining(STATUS_TRAILING_LABEL);
	remaining << 0;
	statusBar = new BStatusBar(statusBarFrame, "statusBar", STATUS_LABEL,
							   remaining.String());
	statusBar->SetResizingMode(B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM);
	statusBar->AddFilter(new StatusBarFilter());

	BRect viewFrame = Bounds();
	if (viewFrame.right < (editorFrame.right + space)) {
		float diff = (editorFrame.right + space) - viewFrame.right;
		ResizeBy(diff, 0);
	}

	AddChild(listView);
	AddChild(encodeButton);
	AddChild(cancelButton);
	AddChild(statusBar);
}

void
AppView::AttachedToWindow()
{
	PRINT(("AppView::AttachedToWindow()\n"));

	BView::AttachedToWindow();

	InitView();

	listView->SetTarget(editorView);
	encodeButton->SetTarget(this);
	cancelButton->SetTarget(this);
}

void
AppView::MessageReceived(BMessage* message)
{
	//PRINT(("AppView::MessageReceived(BMessage*)\n"));
	switch (message->what) {
		case VIEW_SHORTCUT: {
				int32 view;
				message->FindInt32("view", &view);
				PRINT(("View Shortcut: %d\n", view));
				switch (view) {
					case 0:
						fileNamePatternView->MakeFocus(true);
						break;
					case 1:
						editorView->MakeFocus(true);
						break;
					case 2:
						listView->MakeFocus(true);
						break;
				}
			}
			break;
		case ENCODE_MSG:
			Encode();
			break;
		case CANCEL_MSG:
			Cancel();
			break;
		case SELECT_ALL_MSG:
			if (listView->CountRows() > 0) {
				for (int index = 0; index < listView->CountRows(); index++){
					listView->AddToSelection(listView->RowAt(index));
				}
			}
			break;
		case DESELECT_ALL_MSG:
			listView->DeselectAll();
			break;
		case REMOVE_MSG: {
				int32 device;
				if ((message->FindInt32("device", &device) == B_OK)) {
					RemoveDeviceItemsFromList(device);
				} else {
					thread_id thread = spawn_thread(AppView::RemoveItemsFromList,
													"RemoveItems", B_NORMAL_PRIORITY, (void*)this);
					resume_thread(thread);
				}
			}
			break;
		case APPLY_ATTRIBUTE_CHANGES:
			ApplyAttributeChanges(message);
			break;
		case FILE_NAME_PATTERN_CHANGED: {
				int32 numRows = listView->CountRows();
				for (int i = 0; i < numRows; i++) {
					BRefRow* row = 
						(BRefRow*) listView->RowAt(i);
					SetSaveAsColumn(row);
					listView->InvalidateRow(row);
				}
			}
			break;
		case ENCODER_CHANGED: {
				BMessenger msgr(fileNamePatternView);
				msgr.SendMessage(message);
			}
			break;
		case B_REFS_RECEIVED:
		case B_SIMPLE_DATA: {
				BList* args = new BList();
				args->AddItem(this);
				args->AddItem(new BMessage(*message));
				thread_id thread = spawn_thread(AppView::RefsRecievedWrapper,
												"RefsReceived", B_NORMAL_PRIORITY,
												(void*)args);
				resume_thread(thread);
			}
			break;
		case COLUMN_SET_SHOWN: {
				int32 column;
				bool show;
				message->FindInt32("column", &column);
				message->FindBool("show", &show);
				listView->ColumnAt(column)->SetVisible(show);
			}
			break;
		case B_NODE_MONITOR: {
				int32 opcode;
				if (message->FindInt32("opcode", &opcode) != B_OK) {
					return;
				}

				switch (opcode) {
					case B_ENTRY_MOVED:
						UpdateItem(message);
						break;
					case B_ENTRY_REMOVED: {
							node_ref nref;

							if (message->FindInt64("node", &nref.node) != B_OK) {
								break;
							}
							if (message->FindInt32("device", &nref.device) != B_OK) {
								break;
							}
							RemoveNodeFromList(&nref);
						}
						break;
				}
			}
			break;
		default:
			BView::MessageReceived(message);
	}
}

int32
AppView::RefsRecievedWrapper(void* args)
{
	PRINT(("AppView::RefsReceivedWrapper(void*)\n"));

	BList* params = (BList*)args;
	AppView* view = (AppView*)params->ItemAt(0);
	BMessage* message = (BMessage*)params->ItemAt(1);
	view->RefsReceived(message);
	delete message;
	delete params;
	return B_OK;
}

void
AppView::RefsReceived(BMessage* message)
{
	PRINT(("AppView::RefsReceived(BMessage*)\n"));

	BRefRow* row;

	entry_ref tmp_ref;
	int numRefs = 0;
	while (message->FindRef("refs", numRefs, &tmp_ref) == B_NO_ERROR) {
		BNode node(&tmp_ref);
		if (node.IsFile()) {
			BNodeInfo info(&node);
			char type[B_MIME_TYPE_LENGTH+1];
			info.GetType(type);
			if ((strncmp(type, "audio/", 6) == 0)
					&& (strcmp(type, CDDA_MIME_TYPE) != 0)) {
				bool found = false;
				for (int i = 0; i < listView->CountRows(); i++) {
					BRefRow* row = (BRefRow*)listView->RowAt(i);
					entry_ref* ref = row->EntryRef();
					if (*ref == tmp_ref) {
						found = true;
						break;
					}
				}
				if (!found) {
					if (LockLooper()) {
						node_ref nref;
						if (node.GetNodeRef(&nref) == B_OK) {
							row = new BRefRow(&tmp_ref, &nref);
							watch_node(&nref, B_WATCH_NAME, this);
						} else {
							row = new BRefRow(&tmp_ref);
						}
						InitializeColumn(row);
						listView->AddRow(row);
						//listView->SortItems();
						UnlockLooper();
					}
				}
			}
		} else if (node.IsDirectory()) {
			BDirectory dir(&tmp_ref);
			if (dir.InitCheck() == B_OK) {
				BMessage refsMessage(B_REFS_RECEIVED);
				entry_ref ref;
				while (dir.GetNextRef(&ref) != B_ENTRY_NOT_FOUND) {
					refsMessage.AddRef("refs", &ref);
				}
				RefsReceived(&refsMessage);
			}
		} else if (node.IsSymLink()) {
			BEntry entry(&tmp_ref, true);
			if ((entry.InitCheck() == B_OK) && (entry.GetRef(&tmp_ref) == B_OK)) {
				BMessage refsMessage(B_REFS_RECEIVED);
				refsMessage.AddRef("refs", &tmp_ref);
				RefsReceived(&refsMessage);
			}
		}
		numRefs++;
	}
}

void
AppView::InitializeColumn(BRefRow* row)
{
	PRINT(("AppView::InitializeColumn(BRefRow*)\n"));
	
	BStringField* tmpField;

	entry_ref* ref = row->EntryRef();
	
	tmpField = (BStringField*)row->GetField(FILE_COLUMN_INDEX);
	tmpField->SetString(ref->name);
	row->SetField(tmpField, FILE_COLUMN_INDEX);

	BVolume volume(ref->device);
	fs_info fsinfo;
	fs_stat_dev(volume.Device(), &fsinfo);
	if (strcmp(fsinfo.fsh_name, "cdda") == 0) {
		char volume_name[B_FILE_NAME_LENGTH];
		volume.GetName(volume_name);
		if (strcmp(volume_name, "Audio CD") != 0) {
			BString artist(volume_name);
			int index = artist.FindFirst(" - ");
			artist.Truncate(index);
			BString album(volume_name);
			index += 3;
			album.Remove(index, album.Length() - index);
			artist.Trim();
			album.Trim();
			tmpField = (BStringField*)row->GetField(ARTIST_COLUMN_INDEX);
			tmpField->SetString(artist.String());
			row->SetField(tmpField, ARTIST_COLUMN_INDEX);
			tmpField = (BStringField*)row->GetField(ALBUM_COLUMN_INDEX);
			tmpField->SetString(album.String());
			row->SetField(tmpField, ALBUM_COLUMN_INDEX);
			tmpField = (BStringField*)row->GetField(TITLE_COLUMN_INDEX);
			tmpField->SetString(ref->name);
			row->SetField(tmpField, TITLE_COLUMN_INDEX);
		} else {
			tmpField = (BStringField*)row->GetField(ARTIST_COLUMN_INDEX);
			tmpField->SetString((const char*)0);
			row->SetField(tmpField, ARTIST_COLUMN_INDEX);
			tmpField = (BStringField*)row->GetField(ALBUM_COLUMN_INDEX);
			tmpField->SetString((const char*)0);
			row->SetField(tmpField, ALBUM_COLUMN_INDEX);
			tmpField = (BStringField*)row->GetField(TITLE_COLUMN_INDEX);
			tmpField->SetString((const char*)0);
			row->SetField(tmpField, TITLE_COLUMN_INDEX);
		}
	}

	if (volume.KnowsAttr()) {
		PRINT(("Volume knows attributes.\n"));

		BFile file(ref, B_READ_ONLY);
		AudioAttributes attributes(&file);
		PRINT_OBJECT(attributes);

		const char* artist = attributes.Artist();
		if (artist) {
			tmpField = (BStringField*)row->GetField(ARTIST_COLUMN_INDEX);
			tmpField->SetString(artist);
			row->SetField(tmpField, ARTIST_COLUMN_INDEX);
		}

		const char* album = attributes.Album();
		if (album) {
			tmpField = (BStringField*)row->GetField(ALBUM_COLUMN_INDEX);
			tmpField->SetString(album);
			row->SetField(tmpField, ALBUM_COLUMN_INDEX);
		}

		const char* title = attributes.Title();
		if (title) {
			tmpField = (BStringField*)row->GetField(TITLE_COLUMN_INDEX);
			tmpField->SetString(title);
			row->SetField(tmpField, TITLE_COLUMN_INDEX);
		}

		const char* year = attributes.Year();
		if (year) {
			tmpField = (BStringField*)row->GetField(YEAR_COLUMN_INDEX);
			tmpField->SetString(year);
			row->SetField(tmpField, YEAR_COLUMN_INDEX);
		}

		const char* comment = attributes.Comment();
		if (comment) {
			tmpField = (BStringField*)row->GetField(COMMENT_COLUMN_INDEX);
			tmpField->SetString(comment);
			row->SetField(tmpField, COMMENT_COLUMN_INDEX);
		}

		const char* track = attributes.Track();
		if (track) {
			tmpField = (BStringField*)row->GetField(TRACK_COLUMN_INDEX);
			tmpField->SetString(track);
			row->SetField(tmpField, TRACK_COLUMN_INDEX);
		}

		const char* genre = attributes.Genre();
		if (genre) {
			tmpField = (BStringField*)row->GetField(GENRE_COLUMN_INDEX);
			tmpField->SetString(genre);
			row->SetField(tmpField, GENRE_COLUMN_INDEX);
		}
	}

	SetSaveAsColumn(row);
}

void
AppView::SetSaveAsColumn(BRefRow* row)
{
	PRINT(("AppView::SetSaveAsColumn(BRefRow*)\n"));

	BStringField* tmpField;

	tmpField = (BStringField*)row->GetField(ARTIST_COLUMN_INDEX);
	const char* artist = tmpField->String();
	if (!artist) artist = "";
	tmpField = (BStringField*)row->GetField(ALBUM_COLUMN_INDEX);
	const char* album = tmpField->String();
	if (!album) album = "";
	tmpField = (BStringField*)row->GetField(TITLE_COLUMN_INDEX);
	const char* title = tmpField->String();
	if (!title) title = "";
	tmpField = (BStringField*)row->GetField(YEAR_COLUMN_INDEX);
	const char* year = tmpField->String();
	if (!year) year = "";
	tmpField = (BStringField*)row->GetField(COMMENT_COLUMN_INDEX);
	const char* comment = tmpField->String();
	if (!comment) comment = "";
	tmpField = (BStringField*)row->GetField(TRACK_COLUMN_INDEX);
	const char* track = tmpField->String();
	if (!track) track = "";
	tmpField = (BStringField*)row->GetField(GENRE_COLUMN_INDEX);
	const char* genre = tmpField->String();
	if (!genre) genre = "";

	AEEncoder* encoder = settings->Encoder();
	if (encoder) {
		BString fileNamePattern = encoder->GetPattern();
		fileNamePattern.ReplaceAll("%a", artist);
		fileNamePattern.ReplaceAll("%n", album);
		fileNamePattern.ReplaceAll("%t", title);
		fileNamePattern.ReplaceAll("%y", year);
		fileNamePattern.ReplaceAll("%c", comment);
		fileNamePattern.ReplaceAll("%k", track);
		fileNamePattern.ReplaceAll("%g", genre);
		tmpField = (BStringField*)row->GetField(SAVE_AS_COLUMN_INDEX);
		tmpField->SetString(fileNamePattern.String());
		row->SetField(tmpField, SAVE_AS_COLUMN_INDEX);
	}
}

void
AppView::ApplyAttributeChanges(BMessage* message)
{
	PRINT(("AppView::ApplyAttributeChanges(BMessage*)\n"));

	type_code index_type;
	int32 numSelected;
	message->GetInfo("index", &index_type, &numSelected);

	int32 index;
	BRefRow* row;
	for (int i = 0; i < numSelected; i++) {
		if (message->FindInt32("index", i, &index) != B_OK) {
			return;
		}

		row = (BRefRow*)listView->RowAt(index);
		if (!row) {
			return;
		}

		BString tmpString;
		BStringField* tmpField;

		if (message->FindString("artist", &tmpString) == B_OK) {
			tmpField = (BStringField*)row->GetField(ARTIST_COLUMN_INDEX);
			tmpField->SetString(tmpString.String());
			row->SetField(tmpField, ARTIST_COLUMN_INDEX);
		}

		if (message->FindString("album", &tmpString) == B_OK) {
			tmpField = (BStringField*)row->GetField(ALBUM_COLUMN_INDEX);
			tmpField->SetString(tmpString.String());
			row->SetField(tmpField, ALBUM_COLUMN_INDEX);
		}

		if (message->FindString("title", &tmpString) == B_OK) {
			tmpField = (BStringField*)row->GetField(TITLE_COLUMN_INDEX);
			tmpField->SetString(tmpString.String());
			row->SetField(tmpField, TITLE_COLUMN_INDEX);
		}

		if (message->FindString("year", &tmpString) == B_OK) {
			tmpField = (BStringField*)row->GetField(YEAR_COLUMN_INDEX);
			tmpField->SetString(tmpString.String());
			row->SetField(tmpField, YEAR_COLUMN_INDEX);
		}

		if (message->FindString("comment", &tmpString) == B_OK) {
			tmpField = (BStringField*)row->GetField(COMMENT_COLUMN_INDEX);
			tmpField->SetString(tmpString.String());
			row->SetField(tmpField, COMMENT_COLUMN_INDEX);
		}

		if (message->FindString("track", &tmpString) == B_OK) {
			if (tmpString.CountChars() == 1) {
				tmpString.Prepend("0");
			}
			tmpField = (BStringField*)row->GetField(TRACK_COLUMN_INDEX);
			tmpField->SetString(tmpString.String());
			row->SetField(tmpField, TRACK_COLUMN_INDEX);
		}

		if (message->FindString("genre", &tmpString) == B_OK) {
			tmpField = (BStringField*)row->GetField(GENRE_COLUMN_INDEX);
			tmpField->SetString(tmpString.String());
			row->SetField(tmpField, GENRE_COLUMN_INDEX);
		}

		SetSaveAsColumn(row);
		listView->InvalidateRow(row);
	}
}

void
AppView::RemoveNodeFromList(node_ref* ref)
{
	BRefRow* row;
	node_ref* rowRef;
	int32 numRows = listView->CountRows();
	for (int i = numRows - 1; i >= 0; i--) {
		row = (BRefRow*)listView->RowAt(i);
		if (row) {
			rowRef = row->NodeRef();
			if (*ref == *rowRef) {
				if (settings->IsEncoding()) {
					(new BAlert(0, REMOVED_TXT, OK))->Go(0);
					Cancel();
				}
				watch_node(rowRef, B_STOP_WATCHING, this);
				listView->Deselect(listView->RowAt(i));
				listView->RemoveRow(listView->RowAt(i));
				listView->Invalidate();
				delete row;
			}
		}
	}

}

void
AppView::RemoveDeviceItemsFromList(int32 device)
{
	PRINT(("AppView::RemoveDeviceItemsFromList(int32)\n"));

	bool deleted = false;
	BRefRow* row;
	entry_ref* ref;
	int32 numRows = listView->CountRows();
	for (int i = numRows - 1; i >= 0; i--) {
		row = (BRefRow*)listView->RowAt(i);
		if (row) {
			ref = row->EntryRef();
			if (device == ref->device) {
				deleted = true;
				watch_node(row->NodeRef(), B_STOP_WATCHING, this);
				listView->Deselect(listView->RowAt(i));
				listView->RemoveRow(listView->RowAt(i));
				listView->Invalidate();
				delete row;
			}
		}
	}

	if (deleted && settings->IsEncoding()) {
		(new BAlert(0, REMOVED_TXT, OK))->Go(0);
		Cancel();
	}
}

int32
AppView::RemoveItemsFromList(void* args)
{
	PRINT(("AppView::RemoveItemsFromList(void*)\n"));

	AppView* view = (AppView*)args;
	BRefRow* row;
	int32 max = view->listView->CountRows();
	for (int i = max - 1; i >= 0; i--) {
		if (view->listView->RowAt(i)->IsSelected()) {
			PRINT(("%03d\n", i));
			if (view->LockLooper()) {
				row = (BRefRow*)view->listView->RowAt(i);
				view->listView->Deselect(row);
				view->listView->RemoveRow(row);
				watch_node(row->NodeRef(), B_STOP_WATCHING, view);
				delete row;
				view->listView->Invalidate();
				view->UnlockLooper();
			}
		}
	}

	return B_OK;
}

int32
AppView::UpdateItem(BMessage* message)
{
	PRINT(("AppView::UpdateItem(BMessage*)\n"));

	ino_t node;
	ino_t dir;
	dev_t dev;
	const char* name;

	if (message->FindInt64("node", &node) != B_OK) {
		return B_ERROR;
	}

	if (message->FindInt64("to directory", &dir) != B_OK) {
		return B_ERROR;
	}

	if (message->FindInt32("device", &dev) != B_OK) {
		return B_ERROR;
	}

	if (message->FindString("name", &name) != B_OK) {
		return B_ERROR;
	}

	PRINT(("device = %d\n", dev));

	node_ref nref;
	entry_ref eref;

	nref.device = dev;
	nref.node = node;

	eref.device = dev;
	eref.directory = dir;
	eref.set_name(name);

	bool doVolume = false;
	if (dev == 1) {
		BVolumeRoster roster;
		BVolume volume;
		char volName[B_PATH_NAME_LENGTH];
		roster.Rewind();
		while (roster.GetNextVolume(&volume) == B_OK) {
			if (volume.InitCheck() != B_OK) {
				return B_ERROR;
			}
			volume.GetName(volName);
			if (strcmp(volName, name) == 0) {
				doVolume = true;
				dev = volume.Device();
				break;
			}
		}
	}

	BRefRow* row;
	int32 max = listView->CountRows();
	for (int i = 0; i < max; i++) {
		row = (BRefRow*)listView->RowAt(i);
		if (row) {
			PRINT(("item's device = %d\n", row->NodeRef()->device));
			if (row->NodeRef()->node == node) {
				row->SetNodeRef(&nref);
				row->SetEntryRef(&eref);
				if (LockLooper()) {
					InitializeColumn(row);
					//listView->SortItems();
					listView->Invalidate();
					UnlockLooper();
				}
			} else if (doVolume && (row->NodeRef()->device == dev)) {
				if (LockLooper()) {
					InitializeColumn(row);
					//listView->SortItems();
					listView->Invalidate();
					UnlockLooper();
				}
			}
		}
	}

	return B_OK;
}

void
AppView::Encode()
{
	PRINT(("AppView::Encode()\n"));

	thread_id thread = spawn_thread(AppView::EncodeThread, "_Encoder_",
									B_NORMAL_PRIORITY, (void*)this);
	resume_thread(thread);
}

int32
AppView::EncodeThread(void* args)
{
	PRINT(("AppView::EncodeThread(void*)\n"));

	settings->SetEncoding(true);
	AppView* view = (AppView*)args;
	BMenuBar* menuBar = ((AppWindow*)view->Window())->MenuBar();

	AEEncoder* encoder = settings->Encoder();
	if (!encoder || (encoder->InitCheck() != B_OK)) {
		if (encoder->InitCheck() == FSS_EXE_NOT_FOUND) {
			PRINT(("ERROR: exe not found\n"));
			BString msg("The addon could not find the required executable.");
			msg << "\n";
			msg << "Please check the addon's documentation.\n";
			view->AlertUser(msg.String());
		}
		system_beep(SYSTEM_BEEP_ENCODING_DONE);
		return B_ERROR;
	}

	encoder->InitEncoder();
	BMessenger statusBarMessenger(view->statusBar);

	if (view->LockLooper()) {
		view->editorView->SetEnabled(false);
		view->fileNamePatternView->SetEnabled(false);
		view->encodeButton->SetEnabled(false);
		menuBar->SetEnabled(false);
		view->UnlockLooper();
	}

	BObjectList<BRefRow> objList;
	BRefRow* row;
	BBitmapField* tmpBitmapField;
	BStringField* tmpStringField;
	
	int32 numRows = view->listView->CountRows();
	if (view->LockLooper()) {
		for (int i = 0; i < numRows; i++) {
			row = (BRefRow*)view->listView->RowAt(i);
			tmpBitmapField = (BBitmapField*)row->GetField(COMPLETE_COLUMN_INDEX);
			tmpBitmapField->SetBitmap((BBitmap*)NULL);
			row->SetField(tmpBitmapField, COMPLETE_COLUMN_INDEX);
			objList.AddItem(row);
			view->listView->InvalidateRow(row);
		}
		view->UnlockLooper();
	}

	for (int i = 0; i < numRows; i++) {
		row = objList.ItemAt(i);
		
		tmpStringField = (BStringField*)row->GetField(ARTIST_COLUMN_INDEX);
		const char* artist = tmpStringField->String();
		tmpStringField = (BStringField*)row->GetField(ALBUM_COLUMN_INDEX);
		const char* album = tmpStringField->String();
		tmpStringField = (BStringField*)row->GetField(TITLE_COLUMN_INDEX);
		const char* title = tmpStringField->String();
		tmpStringField = (BStringField*)row->GetField(YEAR_COLUMN_INDEX);
		const char* year = tmpStringField->String();
		tmpStringField = (BStringField*)row->GetField(COMMENT_COLUMN_INDEX);
		const char* comment = tmpStringField->String();
		tmpStringField = (BStringField*)row->GetField(TRACK_COLUMN_INDEX);
		const char* track = tmpStringField->String();
		tmpStringField = (BStringField*)row->GetField(GENRE_COLUMN_INDEX);
		const char* genre = tmpStringField->String();
		int32 genreNum;
		if (genre) {
			genreNum = GenreList::Genre(genre);
		}
		tmpStringField = (BStringField*)row->GetField(SAVE_AS_COLUMN_INDEX);
		const char* outputFile = tmpStringField->String();
		entry_ref* ref = row->EntryRef();
		BEntry entry(ref);
		if (entry.InitCheck() != B_OK) {
			PRINT(("ERROR: entry failed InitCheck()\n"));
			BString msg("Error opening file: ");
			msg << ref->name;
			view->AlertUser(msg.String());
			if (view->LockLooper()) {
				BString remaining(STATUS_TRAILING_LABEL);
				remaining << 0;
				view->statusBar->Reset(STATUS_LABEL, remaining.String());
				view->editorView->SetEnabled(true);
				view->fileNamePatternView->SetEnabled(true);
				view->encodeButton->SetEnabled(true);
				view->cancelButton->SetEnabled(true);
				menuBar->SetEnabled(true);
				view->UnlockLooper();
			}
			settings->SetEncoding(false);
			encoder->UninitEncoder();
			system_beep(SYSTEM_BEEP_ENCODING_DONE);
			return B_ERROR;
		}

		BPath path;
		entry.GetPath(&path);
		if (path.InitCheck() != B_OK) {
			PRINT(("ERROR: path failed InitCheck()\n"));
			BString msg("Error opening file: ");
			msg << ref->name;
			view->AlertUser(msg.String());
			if (view->LockLooper()) {
				BString remaining(STATUS_TRAILING_LABEL);
				remaining << 0;
				view->statusBar->Reset(STATUS_LABEL, remaining.String());
				view->editorView->SetEnabled(true);
				view->fileNamePatternView->SetEnabled(true);
				view->encodeButton->SetEnabled(true);
				view->cancelButton->SetEnabled(true);
				menuBar->SetEnabled(true);
				view->UnlockLooper();
			}
			settings->SetEncoding(false);
			encoder->UninitEncoder();
			system_beep(SYSTEM_BEEP_ENCODING_DONE);
			return B_ERROR;
		}

		BPath outputPath(outputFile);
		if (outputPath.InitCheck() != B_OK) {
			PRINT(("ERROR: outputPath failed InitCheck()\n"));
			BString msg("Error creating path for: ");
			msg << outputFile;
			view->AlertUser(msg.String());
			if (view->LockLooper()) {
				BString remaining(STATUS_TRAILING_LABEL);
				remaining << 0;
				view->statusBar->Reset(STATUS_LABEL, remaining.String());
				view->editorView->SetEnabled(true);
				view->fileNamePatternView->SetEnabled(true);
				view->encodeButton->SetEnabled(true);
				view->cancelButton->SetEnabled(true);
				menuBar->SetEnabled(true);
				view->UnlockLooper();
			}
			settings->SetEncoding(false);
			encoder->UninitEncoder();
			system_beep(SYSTEM_BEEP_ENCODING_DONE);
			return B_ERROR;
		}
		outputFile = outputPath.Path();
		BPath outputParent;
		outputPath.GetParent(&outputParent);
		if (outputParent.InitCheck() != B_OK) {
			PRINT(("ERROR: outputParent failed InitCheck()\n"));
			BString msg("Error creating path for: ");
			msg << outputFile;
			view->AlertUser(msg.String());
			if (view->LockLooper()) {
				BString remaining(STATUS_TRAILING_LABEL);
				remaining << 0;
				view->statusBar->Reset(STATUS_LABEL, remaining.String());
				view->editorView->SetEnabled(true);
				view->fileNamePatternView->SetEnabled(true);
				view->encodeButton->SetEnabled(true);
				view->cancelButton->SetEnabled(true);
				menuBar->SetEnabled(true);
				view->UnlockLooper();
			}
			settings->SetEncoding(false);
			encoder->UninitEncoder();
			system_beep(SYSTEM_BEEP_ENCODING_DONE);
			return B_ERROR;
		}
		if (create_directory(outputParent.Path(), 0777) != B_OK) {
			PRINT(("ERROR: failed to create directory\n"));
			BString msg("Error creating path for: ");
			msg << outputFile;
			view->AlertUser(msg.String());
			if (view->LockLooper()) {
				BString remaining(STATUS_TRAILING_LABEL);
				remaining << 0;
				view->statusBar->Reset(STATUS_LABEL, remaining.String());
				view->editorView->SetEnabled(true);
				view->fileNamePatternView->SetEnabled(true);
				view->encodeButton->SetEnabled(true);
				view->cancelButton->SetEnabled(true);
				menuBar->SetEnabled(true);
				view->UnlockLooper();
			}
			settings->SetEncoding(false);
			encoder->UninitEncoder();
			system_beep(SYSTEM_BEEP_ENCODING_DONE);
			return B_ERROR;
		}

		const char* inputFile = path.Path();

		if (view->LockLooper()) {
			BString encoding(STATUS_LABEL);
			encoding << path.Leaf();
			BString remaining(STATUS_TRAILING_LABEL);
			remaining << (numRows - i);
			view->statusBar->Reset(encoding.String(), remaining.String());
			view->UnlockLooper();
		}

		BMessage encodeMessage(FSS_ENCODE);
		encodeMessage.AddString("input file", inputFile);
		encodeMessage.AddString("output file", outputFile);
		encodeMessage.AddMessenger("statusBarMessenger", statusBarMessenger);
		if (artist) {
			encodeMessage.AddString("artist", artist);
		}
		if (album) {
			encodeMessage.AddString("album", album);
		}
		if (title) {
			encodeMessage.AddString("title", title);
		}
		if (year) {
			encodeMessage.AddString("year", year);
		}
		if (comment) {
			encodeMessage.AddString("comment", comment);
		}
		if (track) {
			encodeMessage.AddString("track", track);
		}
		if (genre) {
			encodeMessage.AddString("genre", genre);
			encodeMessage.AddInt32("genre number", genreNum);
		}

		PRINT(("Calling Addon: %s\n", encoder->GetName()));
		int32 retValue = encoder->Encode(&encodeMessage);
		switch (retValue) {
			case B_OK:
				PRINT(("Addon returned successfully.\n"));
				break;
			case FSS_EXE_NOT_FOUND: {
					PRINT(("ERROR: exe not found\n"));
					BString msg("The addon could not find the required executable.");
					msg << "\n";
					msg << "Please check the addon's dccumentation.\n";
					view->AlertUser(msg.String());
					if (view->LockLooper()) {
						BString remaining(STATUS_TRAILING_LABEL);
						remaining << 0;
						view->statusBar->Reset(STATUS_LABEL, remaining.String());
						view->editorView->SetEnabled(true);
						view->fileNamePatternView->SetEnabled(true);
						view->encodeButton->SetEnabled(true);
						view->cancelButton->SetEnabled(true);
						menuBar->SetEnabled(true);
						view->UnlockLooper();
					}
					settings->SetEncoding(false);
					encoder->UninitEncoder();
					system_beep(SYSTEM_BEEP_ENCODING_DONE);
				}
				return B_ERROR;
			case FSS_INPUT_NOT_SUPPORTED: {
					PRINT(("ERROR: input not supported\n"));
					BString msg("Input File: ");
					msg << ref->name;
					msg << " cannot be encoded with this encoder. Continue?";
					BAlert* alert = new BAlert("alert", msg.String(), YES, NO);
					int32 button = alert->Go();
					if (button == 1) {
						if (view->LockLooper()) {
							BString remaining(STATUS_TRAILING_LABEL);
							remaining << 0;
							view->statusBar->Reset(STATUS_LABEL, remaining.String());
							view->editorView->SetEnabled(true);
							view->fileNamePatternView->SetEnabled(true);
							view->encodeButton->SetEnabled(true);
							view->cancelButton->SetEnabled(true);
							menuBar->SetEnabled(true);
							view->UnlockLooper();
						}
						settings->SetEncoding(false);
						encoder->UninitEncoder();
						system_beep(SYSTEM_BEEP_ENCODING_DONE);
						return B_ERROR;
					} else {
						continue;
					}
				}
			case FSS_CANCEL_ENCODING: {
					PRINT(("User canceled encoding.\n"));
					if (view->LockLooper()) {
						BString remaining(STATUS_TRAILING_LABEL);
						remaining << 0;
						view->statusBar->Reset(STATUS_LABEL, remaining.String());
						view->editorView->SetEnabled(true);
						view->fileNamePatternView->SetEnabled(true);
						view->encodeButton->SetEnabled(true);
						view->cancelButton->SetEnabled(true);
						menuBar->SetEnabled(true);
						view->UnlockLooper();
					}
					settings->SetEncoding(false);
					encoder->UninitEncoder();
					BEntry outputEntry(outputFile);
					outputEntry.Remove();
					while (1) {
						BDirectory directory(outputParent.Path());
						if (directory.CountEntries() > 0) {
							break;
						}
						directory.Unset();
						BEntry dirEntry(outputParent.Path());
						dirEntry.Remove();
						dirEntry.Unset();
						outputParent.GetParent(&outputParent);
					}
					system_beep(SYSTEM_BEEP_ENCODING_DONE);
				}
				return B_OK;
			case B_ERROR: {
					PRINT(("ERROR: encoding failed\n"));
					const char* errmsg;
					encodeMessage.FindString("error", &errmsg);
					BString msg("Error encoding file: ");
					msg << ref->name;
					msg << "\n";
					msg << errmsg;
					msg << "Cannot continue.";
					view->AlertUser(msg.String());
					if (view->LockLooper()) {
						BString remaining(STATUS_TRAILING_LABEL);
						remaining << 0;
						view->statusBar->Reset(STATUS_LABEL, remaining.String());
						view->editorView->SetEnabled(true);
						view->fileNamePatternView->SetEnabled(true);
						view->encodeButton->SetEnabled(true);
						view->cancelButton->SetEnabled(true);
						menuBar->SetEnabled(true);
						view->UnlockLooper();
					}
					settings->SetEncoding(false);
					encoder->UninitEncoder();
					system_beep(SYSTEM_BEEP_ENCODING_DONE);
				}
				return B_ERROR;
		}

		if (view->LockLooper()) {
			BBitmap* checkMark = view->listView->GetCheckMark();
			tmpBitmapField = (BBitmapField*)row->GetField(COMPLETE_COLUMN_INDEX);
			tmpBitmapField->SetBitmap(checkMark);
			view->listView->InvalidateRow(row);
			view->UnlockLooper();
		}
	}

	encoder->UninitEncoder();
	settings->SetEncoding(false);

	if (view->LockLooper()) {
		BString remaining(STATUS_TRAILING_LABEL);
		remaining << 0;
		view->statusBar->Reset(STATUS_LABEL, remaining.String());
		view->editorView->SetEnabled(true);
		view->fileNamePatternView->SetEnabled(true);
		view->encodeButton->SetEnabled(true);
		view->cancelButton->SetEnabled(true);
		menuBar->SetEnabled(true);
		view->UnlockLooper();
	}

	system_beep(SYSTEM_BEEP_ENCODING_DONE);
	return B_OK;
}

void
AppView::Cancel()
{
	PRINT(("AppView::Cancel()\n"));

	if (settings->IsEncoding()) {
		thread_id encoder = find_thread("_Encoder_");
		if (!has_data(encoder)) {
			send_data(encoder, FSS_CANCEL_ENCODING, 0, 0);
		}
		cancelButton->SetEnabled(false);
	}
}

void
AppView::AlertUser(const char* message)
{
	PRINT(("AppView::AlertUser(const char*)\n"));
	BAlert* alert = new BAlert("alert", message, OK, NULL, NULL, B_WIDTH_AS_USUAL,
							   B_WARNING_ALERT);
	alert->Go();
}