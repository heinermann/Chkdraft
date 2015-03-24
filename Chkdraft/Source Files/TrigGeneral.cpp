#include "TrigGeneral.h"
#include "Chkdraft.h"

enum ID {
	GROUP_COMMENT = ID_FIRST,
	GROUP_NOTES,
	GROUP_FLAGS,
	EDIT_COMMENT,
	EDIT_NOTES,

	CHECK_EXTENDED_COMMENT_STRING,
	CHECK_EXTENDED_NOTES_STRING,
	CHECK_EXTENDED_COMMENT_ACTION,
	CHECK_EXTENDED_NOTES_ACTION,
	BUTTON_DELETE_COMMENT,
	BUTTON_DELETE_NOTES,

	CHECK_PRESERVE_TRIGGER,
	CHECK_DISABLED,
	CHECK_IGNORE_CONDITIONS_ONCE,
	CHECK_IGNORE_WAITSKIP_ONCE,
	CHECK_IGNORE_MISCACTIONS_ONCE,
	CHECK_IGNORE_DEFEAT_DRAW,
	CHECK_IS_PAUSED,
	TEXT_RAW_FLAGS,
	EDIT_RAW_FLAGS
};

TrigGeneralWindow::TrigGeneralWindow() : trigIndex(0), refreshing(true)
{

}

bool TrigGeneralWindow::CreateThis(HWND hParent, u32 windowId)
{
	if ( getHandle() != NULL )
		return SetParent(hParent);

	RECT rcCli;
	if ( GetWindowRect(hParent, &rcCli) &&
		 ClassWindow::RegisterWindowClass(NULL, NULL, NULL, NULL, NULL, "TrigMeta", NULL, false) &&
		 ClassWindow::CreateClassWindow(NULL, "TrigMeta", WS_CHILD,
			5, 62, rcCli.right-rcCli.left, rcCli.bottom-rcCli.top,
			hParent, (HMENU)windowId) )
	{
		CreateSubWindows(getHandle());
		return true;
	}
	else
		return false;
}

bool TrigGeneralWindow::DestroyThis()
{
	return false;
}

void TrigGeneralWindow::RefreshWindow(u32 trigIndex)
{
	refreshing = true;
	this->trigIndex = trigIndex;
	Trigger* trigger;
	if ( chkd.maps.curr->getTrigger(trigger, trigIndex) )
	{
		editRawFlags.SetEditBinaryNum<s32>(trigger->internalData);
		checkPreservedFlag.SetCheck(trigger->preserveTriggerFlagged());
		checkDisabled.SetCheck(trigger->disabled());
		checkIgnoreConditionsOnce.SetCheck(trigger->ignoreConditionsOnce());
		checkIgnoreWaitSkipOnce.SetCheck(trigger->ignoreWaitSkipOnce());
		checkIgnoreManyActions.SetCheck(trigger->ignoreMiscActionsOnce());
		checkIgnoreDefeatDraw.SetCheck(trigger->ignoreDefeatDraw());
		checkFlagPaused.SetCheck(trigger->flagPaused());
	}
	refreshing = false;
}

void TrigGeneralWindow::DoSize()
{
	RECT rect;
	if ( getClientRect(rect) )
	{
		long cliWidth = rect.right-rect.left,
			 cliHeight = rect.bottom-rect.top,
			 oneThird = cliHeight/3,
			 twoThirds = cliHeight*2/3,
			 padding = 4,
			 groupTopPadding = padding+10,
			 groupLeft = 0,
			 groupTop = 0,
			 groupWidth = 0,
			 groupHeight = 0;

		groupLeft = padding;
		groupTop = padding;
		groupWidth = cliWidth-padding*2;
		groupHeight = oneThird-padding*2;
		groupComment.SetPos(groupLeft, groupTop, groupWidth, groupHeight);
		checkExtendedCommentString.MoveTo(groupLeft+groupWidth-checkExtendedCommentString.Width()-padding, groupTop+groupTopPadding);
		checkExtendedCommentAction.MoveTo(checkExtendedCommentString.Left(), checkExtendedCommentString.Bottom()+1);
		buttonDeleteComment.MoveTo(checkExtendedCommentString.Left(), checkExtendedCommentAction.Bottom()+1);
		editComment.SetPos(groupLeft+padding, groupTop+groupTopPadding,
			checkExtendedCommentString.Left()-padding*2-groupLeft, groupHeight-groupTopPadding-padding);

		groupLeft = padding;
		groupTop = oneThird+padding;
		groupWidth = cliWidth-padding*2;
		groupHeight = oneThird-padding*2;
		groupNotes.SetPos(groupLeft, groupTop, groupWidth, groupHeight);
		checkExtendedNotesString.MoveTo(groupLeft+groupWidth-checkExtendedNotesString.Width()-padding, groupTop+groupTopPadding);
		checkExtendedNotesAction.MoveTo(checkExtendedNotesString.Left(), checkExtendedNotesString.Bottom()+1);
		buttonDeleteNotes.MoveTo(checkExtendedNotesString.Left(), checkExtendedNotesAction.Bottom()+1);
		editNotes.SetPos(groupLeft+padding, groupTop+groupTopPadding,
			checkExtendedNotesString.Left()-padding*2-groupLeft, groupHeight-groupTopPadding-padding);

		groupLeft = padding;
		groupTop = twoThirds+padding;
		groupWidth = cliWidth-padding*2;
		groupHeight = oneThird-padding*2;
		groupExecutionFlags.SetPos(groupLeft, groupTop, groupWidth, groupHeight);
		checkPreservedFlag.MoveTo(groupLeft+padding, groupTop+groupTopPadding);
		checkDisabled.MoveTo(groupLeft+padding, checkPreservedFlag.Bottom()+1);
		checkIgnoreConditionsOnce.MoveTo(groupLeft+padding, checkDisabled.Bottom()+1);
		checkIgnoreWaitSkipOnce.MoveTo(checkPreservedFlag.Right()+padding, groupTop+groupTopPadding);
		checkIgnoreManyActions.MoveTo(checkIgnoreWaitSkipOnce.Left(), checkIgnoreWaitSkipOnce.Bottom()+1);
		checkIgnoreDefeatDraw.MoveTo(checkIgnoreWaitSkipOnce.Left(), checkIgnoreManyActions.Bottom()+1);
		checkFlagPaused.MoveTo(checkIgnoreWaitSkipOnce.Left(), checkIgnoreDefeatDraw.Bottom()+1);

		textRawFlags.MoveTo(checkFlagPaused.Right()+padding*4, groupTop+groupTopPadding);
		editRawFlags.MoveTo(textRawFlags.Left(), textRawFlags.Bottom()+1);
	}
}

void TrigGeneralWindow::CreateSubWindows(HWND hWnd)
{
	groupComment.CreateThis(hWnd, 5, 5, 75, 20, "Comment: ", GROUP_COMMENT);
	editComment.CreateThis(hWnd, 85, 5, 100, 40, true, EDIT_COMMENT);
	checkExtendedCommentString.CreateThis(hWnd, 5, 5, 120, 20, false, "Use Extended String", CHECK_EXTENDED_COMMENT_STRING);
	checkExtendedCommentAction.CreateThis(hWnd, 5, 5, 120, 20, false, "Use Extended Action", CHECK_EXTENDED_COMMENT_ACTION);
	buttonDeleteComment.CreateThis(hWnd, 5, 5, 75, 23, "Delete", BUTTON_DELETE_COMMENT);

	groupNotes.CreateThis(hWnd, 5, 50, 75, 20, "Notes: ", GROUP_NOTES);
	editNotes.CreateThis(hWnd, 85, 50, 100, 80, true, EDIT_NOTES);
	checkExtendedNotesString.CreateThis(hWnd, 5, 5, 120, 20, false, "Use Extended String", CHECK_EXTENDED_NOTES_STRING);
	checkExtendedNotesAction.CreateThis(hWnd, 5, 5, 120, 20, false, "Use Extended Action", CHECK_EXTENDED_NOTES_ACTION);
	buttonDeleteNotes.CreateThis(hWnd, 5, 5, 75, 23, "Delete", BUTTON_DELETE_NOTES);

	groupExecutionFlags.CreateThis(hWnd, 5, 100, 75, 20, "Execution Flags: ", GROUP_FLAGS);
	checkPreservedFlag.CreateThis(hWnd, 5, 5, 150, 20, false, "Preserve Trigger", CHECK_PRESERVE_TRIGGER);
	checkDisabled.CreateThis(hWnd, 5, 5, 150, 20, false, "Disabled", CHECK_DISABLED);
	checkIgnoreConditionsOnce.CreateThis(hWnd, 5, 5, 150, 20, false, "Ignore Conditions Once", CHECK_IGNORE_CONDITIONS_ONCE);
	checkIgnoreWaitSkipOnce.CreateThis(hWnd, 5, 5, 150, 20, false, "Ignore Wait Skip Once", CHECK_IGNORE_WAITSKIP_ONCE);
	checkIgnoreManyActions.CreateThis(hWnd, 5, 5, 150, 20, false, "Ignore Misc Actions Once", CHECK_IGNORE_MISCACTIONS_ONCE);
	checkIgnoreDefeatDraw.CreateThis(hWnd, 5, 5, 150, 20, false, "Ignore Defeat/Draw", CHECK_IGNORE_DEFEAT_DRAW);
	checkFlagPaused.CreateThis(hWnd, 5, 5, 150, 20, false, "Flag Paused", CHECK_IS_PAUSED);

	textRawFlags.CreateThis(hWnd, 5, 5, 150, 13, "Raw Flags:", TEXT_RAW_FLAGS);
	editRawFlags.CreateThis(hWnd, 5, 5, 200, 20, false, EDIT_RAW_FLAGS);

	// TEMP //
	editComment.DisableThis();
	checkExtendedCommentString.DisableThis();
	checkExtendedCommentAction.DisableThis();
	buttonDeleteComment.DisableThis();
	editNotes.DisableThis();
	checkExtendedNotesString.DisableThis();
	checkExtendedNotesAction.DisableThis();
	buttonDeleteNotes.DisableThis();
	//////////
}

void TrigGeneralWindow::OnLeave()
{
	ParseRawFlagsText();
}

void TrigGeneralWindow::SetPreserveTrigger(bool preserve)
{
	Trigger* trigger;
	if ( chkd.maps.curr->getTrigger(trigger, trigIndex) )
	{
		trigger->setPreserveTriggerFlagged(preserve);
		RefreshWindow(trigIndex);
	}
}

void TrigGeneralWindow::SetDisabledTrigger(bool disabled)
{
	Trigger* trigger;
	if ( chkd.maps.curr->getTrigger(trigger, trigIndex) )
	{
		trigger->setDisabled(disabled);
		RefreshWindow(trigIndex);
	}
}

void TrigGeneralWindow::SetIgnoreConditionsOnce(bool ignoreConditionsOnce)
{
	Trigger* trigger;
	if ( chkd.maps.curr->getTrigger(trigger, trigIndex) )
	{
		trigger->setIgnoreConditionsOnce(ignoreConditionsOnce);
		RefreshWindow(trigIndex);
	}
}

void TrigGeneralWindow::SetIgnoreWaitSkipOnce(bool ignoreWaitSkipOnce)
{
	Trigger* trigger;
	if ( chkd.maps.curr->getTrigger(trigger, trigIndex) )
	{
		trigger->setIgnoreWaitSkipOnce(ignoreWaitSkipOnce);
		RefreshWindow(trigIndex);
	}
}

void TrigGeneralWindow::SetIgnoreMiscActionsOnce(bool ignoreMiscActionsOnce)
{
	Trigger* trigger;
	if ( chkd.maps.curr->getTrigger(trigger, trigIndex) )
	{
		trigger->setIgnoreMiscActionsOnce(ignoreMiscActionsOnce);
		RefreshWindow(trigIndex);
	}
}

void TrigGeneralWindow::SetIgnoreDefeatDraw(bool ignoreDefeatDraw)
{
	Trigger* trigger;
	if ( chkd.maps.curr->getTrigger(trigger, trigIndex) )
	{
		trigger->setIgnoreDefeatDraw(ignoreDefeatDraw);
		RefreshWindow(trigIndex);
	}
}

void TrigGeneralWindow::SetPausedTrigger(bool paused)
{
	Trigger* trigger;
	if ( chkd.maps.curr->getTrigger(trigger, trigIndex) )
	{
		trigger->setFlagPaused(paused);
		RefreshWindow(trigIndex);
	}
}

void TrigGeneralWindow::ParseRawFlagsText()
{
	Trigger* trigger;
	if ( chkd.maps.curr->getTrigger(trigger, trigIndex) )
	{
		editRawFlags.GetEditBinaryNum(trigger->internalData);
		RefreshWindow(trigIndex);
	}		
}

LRESULT TrigGeneralWindow::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch ( msg )
	{
		case WM_SHOWWINDOW:
			if ( wParam == FALSE )
				OnLeave();
			else if ( wParam == TRUE )
				RefreshWindow(trigIndex);
			break;

		case WM_COMMAND:
			if ( !refreshing )
			{
				switch ( LOWORD(wParam) )
				{
					case EDIT_RAW_FLAGS:
						if ( HIWORD(wParam) == EN_KILLFOCUS )
							ParseRawFlagsText();
						break;
				}
				switch ( HIWORD(wParam) )
				{
					case BN_CLICKED:
						switch ( LOWORD(wParam) )
						{
							case CHECK_PRESERVE_TRIGGER:		SetPreserveTrigger(checkPreservedFlag.isChecked());				break;
							case CHECK_DISABLED:				SetDisabledTrigger(checkDisabled.isChecked());					break;
							case CHECK_IGNORE_CONDITIONS_ONCE:	SetIgnoreConditionsOnce(checkIgnoreConditionsOnce.isChecked()); break;
							case CHECK_IGNORE_WAITSKIP_ONCE:	SetIgnoreWaitSkipOnce(checkIgnoreWaitSkipOnce.isChecked());		break;
							case CHECK_IGNORE_MISCACTIONS_ONCE:	SetIgnoreMiscActionsOnce(checkIgnoreManyActions.isChecked());	break;
							case CHECK_IGNORE_DEFEAT_DRAW:		SetIgnoreDefeatDraw(checkIgnoreDefeatDraw.isChecked());			break;
							case CHECK_IS_PAUSED:				SetPausedTrigger(checkFlagPaused.isChecked());					break;
						}
						break;
				}
			}
			return DefWindowProc(hWnd, msg, wParam, lParam);
			break;

		case WM_CLOSE:
			OnLeave();
			return DefWindowProc(hWnd, msg, wParam, lParam);
			break;

		default:
			return DefWindowProc(hWnd, msg, wParam, lParam);
			break;
	}
	return 0;
}