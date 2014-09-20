#include "Chkdraft.h"

void Chkdraft::OnLoadTest()
{/*
	// Standard Map Open
	maps.NewMap(128, 128, 0, 0, 0);
	maps.FocusActive();
	maps.curr->Scroll(SCROLL_X|SCROLL_Y);
	// End Standard Map Open

	ShowWindow(getHandle(), SW_MAXIMIZE);
	OpenMapSettings(ID_SCENARIO_SOUNDEDITOR); //*/
}

Chkdraft::Chkdraft() : editFocused(false)
{

}

int Chkdraft::Run(LPSTR lpCmdLine, int nCmdShow)
{
	if ( !CreateThis() )
		return 1;

	this->OnLoadTest();

	scData.Load();
	InitCommonControls();
	ShowWindow(getHandle(), nCmdShow);
    UpdateWindow(getHandle());
	ParseCmdLine(lpCmdLine);

	MSG msg;
	while ( GetMessage(&msg, NULL, 0, 0) > 0 )
	{
		DlgKeyListener(msg.hwnd, msg.message, msg.wParam, msg.lParam);

		if ( IsDialogMessage(currDialog, &msg) == FALSE &&
			 TranslateMDISysAccel(maps.getHandle(), &msg) == FALSE )
		{
			TranslateMessage(&msg);
			KeyListener(msg.hwnd, msg.message, msg.wParam, msg.lParam);
			DispatchMessage(&msg);
		}
	}
	return msg.wParam;
}

bool Chkdraft::CreateThis()
{
	if ( !ClassWindow::WindowClassIsRegistered("wcChkdraft") )
	{
		DWORD classStyle = NULL;
		HICON hIcon = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_PROGRAM_ICON), IMAGE_ICON, 32, 32, 0);
		HICON hIconSmall = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_PROGRAM_ICON), IMAGE_ICON, 16, 16, 0);
		HCURSOR hCursor = LoadCursor(NULL, IDC_ARROW);
		HBRUSH hBackground = (HBRUSH)(COLOR_APPWORKSPACE+1);
		LPCTSTR menu = MAKEINTRESOURCE(IDR_MAIN_MENU);
		LPCTSTR wcName = "wcChkdraft";
		if ( !ClassWindow::RegisterWindowClass(classStyle, hIcon, hCursor, hBackground, menu, wcName, hIconSmall, false) )
		{
			DestroyIcon(hIcon);
			DestroyCursor(hCursor);
			DestroyIcon(hIconSmall);
		}
	}

	DWORD exStyle = NULL;
	LPCSTR windowName = "Chkdraft 1.0 (Alpha 1.0.1)";
	DWORD style = WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN;
	int windowX = CW_USEDEFAULT,
		windowY = CW_USEDEFAULT,
		windowWidth = CW_USEDEFAULT,
		windowHeight = CW_USEDEFAULT;
	HMENU id = NULL;

	return ClassWindow::CreateClassWindow(exStyle, windowName, style, windowX, windowY, windowWidth, windowHeight, NULL, id) &&
		   Chkdraft::CreateSubWindows();
}

bool Chkdraft::ChangesLocked(u16 mapId)
{
	GuiMap* map = maps.GetMap(mapId);
	return map != nullptr && map->changesLocked();
}

bool Chkdraft::EditFocused()
{
	return editFocused;
}

void Chkdraft::SetCurrDialog(HWND hDialog)
{
	if ( currDialog != hDialog )
		currDialog = hDialog;
}

void Chkdraft::SetEditFocused(bool editFocused)
{
	this->editFocused = editFocused;
}

bool Chkdraft::DlgKeyListener(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch ( msg )
	{
		case WM_KEYDOWN:
			{
				switch ( wParam )
				{
					case VK_RETURN:
						if ( GetParent(hWnd) == unitWindow.getHandle() )
						{
							unitWindow.DestroyThis();
							return true;
						}
						else if ( GetParent(hWnd) == locationWindow.getHandle() )
						{
							locationWindow.DestroyThis();
							return true;
						}
						break;
					case VK_DELETE:
						if ( GetParent(hWnd) == unitWindow.getHandle() )
						{
							SendMessage(unitWindow.getHandle(), WM_COMMAND, MAKEWPARAM(IDC_BUTTON_DELETE, 0), NULL);
							return true;
						}
						break;
					/*case 'Z': case 'Y': case 'X': case 'C': case 'V':
						if ( GetKeyState(VK_CONTROL) & 0x8000 )
						{
							KeyListener(hWnd, msg, wParam, lParam);
							return true;
						}
						break;*/
				}
			}
			break;
		case WM_KEYUP:
			if ( wParam == VK_SPACE && maps.curr && maps.clipboard.isPasting() )
			{
				LockCursor(NULL);
				return true;
			}
			break;
	}
	return false;
}

void Chkdraft::KeyListener(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch ( msg )
	{
		case WM_KEYDOWN:
			{
				switch ( wParam )
				{
					case VK_SPACE: LockCursor(maps.curr->getHandle()); return; break;
					case VK_DELETE: maps.curr->deleteSelection(); return; break;
					case VK_ESCAPE: maps.endPaste(); return; break;
					case VK_RETURN: maps.curr->ReturnKeyPress(); return; break;
				}

				if ( GetKeyState(VK_CONTROL) & 0x8000 ) // Control is down
				{
					if ( GetKeyState(VK_SHIFT) & 0x8000 ) // Shift is down
					{
						switch ( wParam )
						{
							case 'S': maps.SaveCurr(true); return; break;
						}
					}
					else // Only control is down
					{
						switch ( wParam )
						{
							case 'A': case 'C': case 'X': case 'V':
								{
									if ( currDialog == NULL )
									{
										switch ( wParam )
										{
											case 'A': maps.curr->selectAll(); return; break;
											case 'C': maps.copy(); return; break;
											case 'X': maps.cut(); return; break;
											case 'V': maps.startPaste(false); return; break;
										}
									}
								}
								break;
							case 'D': maps.ChangeLayer(LAYER_DOODADS); return; break;
							case 'E': FindLeaks(); return; break;
							case 'F': maps.ChangeLayer(LAYER_FOG); return; break;
							case 'L': maps.ChangeLayer(LAYER_LOCATIONS);return; break;
							case 'N': newMap.CreateThis(getHandle()); return; break;
							case 'O': maps.OpenMap(); return; break;
							case 'R': maps.ChangeLayer(LAYER_SPRITES); return; break;
							case 'S': maps.SaveCurr(false); return; break;
							case 'T': maps.ChangeLayer(LAYER_TERRAIN); return; break;
							case 'U': maps.ChangeLayer(LAYER_UNITS); return; break;
							case 'Y': maps.curr->redo(); return; break;
							case 'Z': maps.curr->undo(); return; break;
							case VK_OEM_PLUS: maps.ChangeZoom(true); return; break;
							case VK_OEM_MINUS: maps.ChangeZoom(false); return; break;
							case VK_F4: maps.CloseActive(); return; break;
							case VK_F6: maps.nextMdi(); return; break;
						}
					}
				}
			}
			break;

		case WM_SYSKEYDOWN: // Sent insted of WM_KEYUP when alt is pressed
			{
				switch ( wParam )
				{
					case 'U': maps.SetGrid(8, 8); return; break;
					case 'F': maps.SetGrid(16, 16); return; break;
					case 'G': maps.SetGrid(32, 32); return; break;
					case 'L': maps.SetGrid(64, 64); return; break;
					case 'E': maps.SetGrid(128, 128); return; break;
				}
			}
			break;

		case WM_KEYUP:
			{
				switch ( wParam )
				{
					case VK_SPACE:
						if ( !maps.clipboard.isPasting() )
							LockCursor(NULL);
						return; break;
				}
			}
			break;
	}

	if ( maps.curr && editFocused == false && GetActiveWindow() == getHandle() )
	{
		u8 layer = maps.curr->currLayer();
		if ( layer == LAYER_UNITS || layer == LAYER_FOG || layer == LAYER_SPRITES )
		{
			u8 newPlayer;
			switch ( wParam )
			{
				case '1': newPlayer = 0	; break;
				case '2': newPlayer = 1	; break;
				case '3': newPlayer = 2	; break;
				case '4': newPlayer = 3	; break;
				case '5': newPlayer = 4	; break;
				case '6': newPlayer = 5	; break;
				case '7': newPlayer = 6	; break;
				case '8': newPlayer = 7	; break;
				case '9': newPlayer = 8	; break;
				case '0': newPlayer = 9	; break;
				case '-': newPlayer = 10; break;
				case '=': newPlayer = 11; break;
				default	: return		; break;
			}

			mainToolbar.playerBox.SetSel(newPlayer);
			maps.ChangePlayer(newPlayer);
		}
	}
}

void Chkdraft::ParseCmdLine(LPSTR lpCmdLine)
{
	int length = strlen(lpCmdLine);
	if ( length > 1 ) // Minimum length 2
	{
		if ( lpCmdLine[0] == '\"' )
		{
			lpCmdLine[length-1] = '\0';
			maps.OpenMap(&lpCmdLine[1]);
		}
		else
			maps.OpenMap(&lpCmdLine[0]);

		maps.FocusActive();
	}
}

LRESULT Chkdraft::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch ( msg )
	{
		case WM_COMMAND:
			{
				switch ( LOWORD(wParam) )
				{
					// File
					case ID_FILE_NEW1: newMap.CreateThis(hWnd); break;
					case ID_FILE_OPEN1: maps.OpenMap(); break;
					case ID_FILE_CLOSE1: maps.CloseActive(); break;
					case ID_FILE_SAVE1: maps.SaveCurr(false); break;
					case ID_FILE_SAVEAS1: maps.SaveCurr(true); break;
					case ID_FILE_QUIT1: PostQuitMessage(0); break;

					// Edit
					case ID_EDIT_UNDO1: maps.curr->undo(); break;
					case ID_EDIT_REDO1: maps.curr->redo(); break;
					case ID_EDIT_CUT1: maps.cut(); break;
					case ID_EDIT_COPY1: maps.copy(); break;
					case ID_EDIT_PASTE1: maps.startPaste(true); break;
					case ID_EDIT_SELECTALL: maps.curr->selectAll(); break;
					case ID_EDIT_DELETE: maps.curr->deleteSelection(); break;
					case ID_EDIT_PROPERTIES: maps.properties(); break;

					// View
						// Grid
						case ID_GRID_ULTRAFINE: maps.SetGrid(8, 8); break;
						case ID_GRID_FINE: maps.SetGrid(16, 16); break;
						case ID_GRID_NORMAL: maps.SetGrid(32, 32); break;
						case ID_GRID_LARGE: maps.SetGrid(64, 64); break;
						case ID_GRID_EXTRALARGE: maps.SetGrid(128, 128); break;
						case ID_GRID_DISABLED: maps.SetGrid(0, 0); break;
							// Color
							case ID_COLOR_BLACK: maps.SetGridColor(0, 0, 0); break;
							case ID_COLOR_GREY: maps.SetGridColor(72, 72, 88); break;
							case ID_COLOR_WHITE: maps.SetGridColor(255, 255, 255); break;
							case ID_COLOR_GREEN: maps.SetGridColor(16, 252, 24); break;
							case ID_COLOR_RED: maps.SetGridColor(244, 4, 4); break;
							case ID_COLOR_BLUE: maps.SetGridColor(36, 36, 252); break;
							case ID_COLOR_OTHER: break;
						// Zoom
						case ID_ZOOM_400: maps.curr->setZoom(zooms[0]); break;
						case ID_ZOOM_300: maps.curr->setZoom(zooms[1]); break;
						case ID_ZOOM_200: maps.curr->setZoom(zooms[2]); break;
						case ID_ZOOM_150: maps.curr->setZoom(zooms[3]); break;
						case ID_ZOOM_100: maps.curr->setZoom(zooms[4]); break;
						case ID_ZOOM_66:  maps.curr->setZoom(zooms[5]); break;
						case ID_ZOOM_50:  maps.curr->setZoom(zooms[6]); break;
						case ID_ZOOM_33:  maps.curr->setZoom(zooms[7]); break;
						case ID_ZOOM_25:  maps.curr->setZoom(zooms[8]); break;
						case ID_ZOOM_10:  maps.curr->setZoom(zooms[9]); break;
						// Terrain
						case ID_TERRAIN_DISPLAYTILEELEVATIONS: maps.curr->ToggleDisplayElevations(); break;
						case ID_TERRAIN_DISPLAYTILEVALUES: maps.curr->ToggleTileNumSource(false); break;
						case ID_TERRAIN_DISPLAYTILEVALUESMTXM: maps.curr->ToggleTileNumSource(true); break;

					// Editor
							// Units
							case ID_UNITS_UNITSSNAPTOGRID: maps.curr->ToggleUnitSnap(); break;
							case ID_UNITS_ALLOWSTACK: maps.curr->ToggleUnitStack(); break;

							// Locations
							case ID_LOCATIONS_SNAPTOTILE: maps.curr->SetLocationSnap(SNAP_LOCATION_TO_TILE); break;
							case ID_LOCATIONS_SNAPTOACTIVEGRID: maps.curr->SetLocationSnap(SNAP_LOCATION_TO_GRID); break;
							case ID_LOCATIONS_NOSNAP: maps.curr->SetLocationSnap(NO_LOCATION_SNAP); break;
							case ID_LOCATIONS_LOCKANYWHERE: maps.curr->ToggleLockAnywhere(); break;
							case ID_LOCATIONS_CLIPNAMES: maps.curr->ToggleLocationNameClip(); break;

					// Scenario
							case ID_SCENARIO_DESCRIPTION: case ID_SCENARIO_FORCES: case ID_SCENARIO_UNITSETTINGS:
							case ID_SCENARIO_UPGRADESETTINGS: case ID_SCENARIO_TECHSETTINGS: case ID_SCENARIO_STRINGS:
							case ID_SCENARIO_SOUNDEDITOR: OpenMapSettings(LOWORD(wParam)); break;

					// Tools
					case ID_TRIGGERS_TRIGGEREDITOR: textTrigWindow.CreateThis(getHandle()); break;

					// Windows
					case ID_WINDOWS_CASCADE: maps.cascade(); break;
					case ID_WINDOWS_TILEHORIZONTALLY: maps.tileHorizontally(); break;
					case ID_WINDOWS_TILEVERTICALLY: maps.tileVertically(); break;
					case ID_WINDOW_CLOSE: maps.destroyActive(); break;

					// Help
					case ID_HELP_STARCRAFT_WIKI: OpenWebPage("http://www.staredit.net/starcraft/Main_Page"); break;
					case ID_HELP_SUPPORT_FORUM: OpenWebPage("http://www.staredit.net/forums/"); break;
					case ID_HELP_CHKDRAFTTHREAD: OpenWebPage("http://www.staredit.net/topic/15514/"); break;

					default:
						switch ( HIWORD(wParam) )
						{
							case CBN_SETFOCUS: editFocused = true; break;
							case CBN_KILLFOCUS: editFocused = false; break;
							case CBN_EDITCHANGE: ComboEditChanged((HWND)lParam, LOWORD(wParam)); break;
							case CBN_SELCHANGE: ComboSelChanged((HWND)lParam, LOWORD(wParam)); break;
							default: return DefFrameProc(hWnd, maps.getHandle(), msg, wParam, lParam); break;
						}
						break;
				}
			}
			break;

		case WM_MENUCHAR:
			return MAKELPARAM(0, MNC_CLOSE);
			break;

		case WM_SIZE:
		case WM_SIZING:
			SizeSubWindows();
			break;

		case WM_DROPFILES:
			char fileName[MAX_PATH];
			DragQueryFile((HDROP)wParam, 0, fileName, MAX_PATH);
			DragFinish((HDROP)wParam);
			UseDragFile(fileName);
			break;

		case WM_SHOWWINDOW:
			{
				LRESULT result = DefWindowProc(hWnd, msg, wParam, lParam);
				if ( wParam == TRUE )
					ShowWindow(currDialog, SW_SHOW);

				return result;
			}
			break;

		case WM_SYSCOMMAND:
			if ( wParam == SC_RESTORE )
			{
				LRESULT result = DefFrameProc(hWnd, maps.getHandle(), msg, wParam, lParam);
				RestoreDialogs();
				return result;
			}
			else if ( wParam == SC_MINIMIZE )
				MinimizeDialogs();
			return DefFrameProc(hWnd, maps.getHandle(), msg, wParam, lParam);
			break;

		case WM_DESTROY:
			PostQuitMessage(0);
			break;

		default:
			if ( msg == WM_COPYDATA || (msg >= PLUGIN_MSG_START && msg <= PLUGIN_MSG_END) )
				return CallWindowProc(PluginProc, hWnd, msg, wParam, lParam);
			else
				return DefFrameProc(hWnd, maps.getHandle(), msg, wParam, lParam);
			break;
	}
	return 0;
}

bool Chkdraft::CreateSubWindows()
{
	HWND hWnd = getHandle();
	if ( hWnd != NULL )
	{
		DragAcceptFiles(hWnd, TRUE);
		int statusWidths[] = { 130, 205, 350, 450, 600, -1 };

		return mainMenu.FindThis(hWnd) &&
			   mainToolbar.CreateThis(hWnd) &&
			   statusBar.CreateThis( sizeof(statusWidths)/sizeof(int), statusWidths, NULL,
									 WS_CHILD|WS_VISIBLE|SBARS_SIZEGRIP, hWnd, (HMENU)IDR_MAIN_STATUS ) &&
			   maps.CreateMdiFrame( GetSubMenu(GetMenu(hWnd), 6), ID_MDI_FIRSTCHILD, NULL, NULL,
									WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN|WS_VSCROLL|WS_HSCROLL,
									0, 0, 0, 0, hWnd, (HMENU)IDR_MAIN_PLOT ) &&
			   mainPlot.CreateThis(hWnd);
	}
	else
		return false;
}

void Chkdraft::MinimizeDialogs()
{
	ShowWindow(unitWindow.getHandle(), SW_HIDE);
	ShowWindow(locationWindow.getHandle(), SW_HIDE);
	ShowWindow(terrainPalWindow.getHandle(), SW_HIDE);
	ShowWindow(mapSettingsWindow.getHandle(), SW_HIDE);
}

void Chkdraft::RestoreDialogs()
{
	ShowWindow(unitWindow.getHandle(), SW_SHOW);
	ShowWindow(locationWindow.getHandle(), SW_SHOW);
	ShowWindow(terrainPalWindow.getHandle(), SW_SHOW);
	ShowWindow(mapSettingsWindow.getHandle(), SW_SHOW);
}

void Chkdraft::SizeSubWindows()
{
	RECT rcMain, rcTool, rcStatus, rcLeftBar;
				
	mainToolbar.AutoSize();
	statusBar.AutoSize();

	// Get the size of the client area, toolbar, status bar, and left bar
	GetClientRect(getHandle(), &rcMain);
	GetWindowRect(mainToolbar.getHandle(), &rcTool);
	GetWindowRect(statusBar.getHandle(), &rcStatus);
	GetWindowRect(mainPlot.leftBar.getHandle(), &rcLeftBar);

	int border = GetSystemMetrics(SM_CXSIZEFRAME)-1;

	// Fit plot to the area between the toolbar and statusbar
	SetWindowPos( mainPlot.getHandle(), NULL, 0, rcTool.bottom-rcTool.top,
				  rcMain.right-rcMain.left, rcMain.bottom-rcMain.top+rcTool.top-rcTool.bottom-rcStatus.bottom+rcStatus.top, SWP_NOZORDER );

	// Fit left bar to the area between the toolbar and statusbar without changing width
	SetWindowPos( mainPlot.leftBar.getHandle(), NULL, -border, -border,
				  rcLeftBar.right-rcLeftBar.left, rcStatus.top-rcTool.bottom+border*2, SWP_NOZORDER );

	// Fit the map MDI window to the area right of the left bar and between the toolbar and statusbar
	SetWindowPos( maps.getHandle(), NULL, rcLeftBar.right-rcLeftBar.left-border-2, rcTool.bottom-rcTool.top,
				  rcMain.right-rcMain.left-rcLeftBar.right+rcLeftBar.left+border+2, rcStatus.top-rcTool.bottom+2, SWP_NOZORDER );

	RedrawWindow(statusBar.getHandle(), NULL, NULL, RDW_INVALIDATE);
}

void Chkdraft::UseDragFile(const char* fileName)
{
	maps.OpenMap(fileName);
	maps.FocusActive();
}

void Chkdraft::OpenMapSettings(u16 menuId)
{
	if ( mapSettingsWindow.getHandle() == NULL )
		mapSettingsWindow.CreateThis(getHandle());
									
	if ( mapSettingsWindow.getHandle() != NULL )
	{
		switch ( menuId )
		{
			case ID_SCENARIO_DESCRIPTION: mapSettingsWindow.ChangeTab(ID_TAB_MAPPROPERTIES); break;
			case ID_SCENARIO_FORCES: mapSettingsWindow.ChangeTab(ID_TAB_FORCES); break;
			case ID_SCENARIO_UNITSETTINGS: mapSettingsWindow.ChangeTab(ID_TAB_UNITSETTINGS); break;
			case ID_SCENARIO_UPGRADESETTINGS: mapSettingsWindow.ChangeTab(ID_TAB_UPGRADESETTINGS); break;
			case ID_SCENARIO_TECHSETTINGS: mapSettingsWindow.ChangeTab(ID_TAB_TECHSETTINGS); break;
			case ID_SCENARIO_STRINGS: mapSettingsWindow.ChangeTab(ID_TAB_STRINGEDITOR); break;
			case ID_SCENARIO_SOUNDEDITOR: mapSettingsWindow.ChangeTab(ID_TAB_WAVEDITOR); break;
		}
		ShowWindow(mapSettingsWindow.getHandle(), SW_SHOW);
	}
}

void Chkdraft::OpenWebPage(const char* address)
{
	ShellExecute(NULL, "open", address, NULL, NULL, SW_SHOWNORMAL);
}

void Chkdraft::ComboEditChanged(HWND hCombo, u16 comboId)
{
	if ( comboId == ID_COMBOBOX_PLAYER )
	{
		u8 newPlayer;
		if ( GetPlayerNum(hCombo, newPlayer) )
			maps.ChangePlayer(newPlayer);
	}
}

void Chkdraft::ComboSelChanged(HWND hCombo, u16 comboId)
{
	int itemIndex = SendMessage(hCombo, CB_GETCURSEL, 0, 0);
	switch ( comboId )
	{
		case ID_COMBOBOX_LAYER: maps.ChangeLayer(itemIndex); break;
		case ID_COMBOBOX_PLAYER: maps.ChangePlayer(itemIndex); break;
		case ID_COMBOBOX_ZOOM:
			if ( itemIndex >= 0 && itemIndex < 10 )
				maps.curr->setZoom(zooms[itemIndex]);
			break;
		case ID_COMBOBOX_TERRAIN:
			if ( itemIndex == 3 )
			{
				terrainPalWindow.CreateThis(getHandle());
				ShowWindow(terrainPalWindow.getHandle(), SW_SHOW);
			}
			break;
	}
}