
#include <fstream>
#include <winsock2.h>
#include <windows.h> 
#include <tchar.h>
#include <commctrl.h>

#include "../res/resource.h"
#include "GemSDK.h"
#include "hsmssettingDlg.h"
#include "msgstructDlg.h" 
#include "logger.h"
#include "FTPClient.h"

//#include "vld.h"

using namespace std;

DWORD WINAPI EquipmentLoopThread(LPVOID lParam);
extern int ParseStreamFunction(string& sfstr);

enum { HSMS_TYPE_COMMUNICATION = 0, SECS_TYPE_COMMUNICATION = 1 };

volatile static int s_equipment_loop	= 0;
volatile static int s_play_loop			= 0;

char s_dirPath[MAX_PATH];
string s_ExePath = "";

static int s_dtype = HSMS_TYPE_COMMUNICATION;

string ExePath() 
{
	char buffer[MAX_PATH];
	GetModuleFileName( NULL, buffer, MAX_PATH );
	string::size_type pos = string( buffer ).find_last_of( "\\/" );
	return string( buffer ).substr( 0, pos);
}

void CreateMainMenu(HWND hDlg)
{
	//file menu
    HMENU hMenu		= CreateMenu();
	HMENU hFileMenu	= CreatePopupMenu(); 
	HMENU hEditMenu = CreatePopupMenu();
	HMENU hViewMenu = CreatePopupMenu();
	HMENU hShowMenu = CreatePopupMenu();

    AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT)hFileMenu, "檔案");
	AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT)hEditMenu, "編輯");
	AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT)hViewMenu, "檢視");
	AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT)hShowMenu, "說明");

	AppendMenu(hFileMenu, MF_STRING, IDC_COMMUNICATE_OPEN, "開啟通訊");
	AppendMenu(hFileMenu, MF_STRING, IDC_COMMUNICATE_CLOSE, "關閉通訊");
	AppendMenu(hFileMenu, MF_STRING, IDC_XML_READ, "讀取XML檔案");
	InsertMenu(hFileMenu, 3, MF_SEPARATOR | MF_BYPOSITION, 0, NULL);
	AppendMenu(hFileMenu, MF_STRING, IDC_DIALOG_OVER, "結束");
	
	AppendMenu(hEditMenu, MF_STRING, IDC_MSG_CLEAR, "全部清除");

	AppendMenu(hViewMenu, MF_STRING, IDC_SECS_SETTING, "設定SECS-I");
	AppendMenu(hViewMenu, MF_STRING, IDC_HSMS_SETTING, "設定HSMS");
	AppendMenu(hViewMenu, MF_UNCHECKED, IDC_SECS_TYPE, "SECS-I通訊模式");
	AppendMenu(hViewMenu, MF_CHECKED, IDC_HSMS_TYPE, "HSMS通訊模式");

	AppendMenu(hShowMenu, MF_STRING, IDC_ABOUT_EAP, "關於EAP Emulator");

    SetMenu(hDlg, hMenu);

	//init menu item
	EnableMenuItem(GetSubMenu(GetMenu(hDlg), 0), IDC_COMMUNICATE_OPEN, MF_BYCOMMAND | MF_ENABLED);
	EnableMenuItem(GetSubMenu(GetMenu(hDlg), 0), IDC_COMMUNICATE_CLOSE, MF_DISABLED | MF_GRAYED);
}

string GetFileName(HWND hDlg)
{
	OPENFILENAME	ofn; 
	char			szFile[260] = "C:/AlphaProject";

	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner	= hDlg;
	ofn.lpstrFile	= szFile;

	// Set lpstrFile[0] to '\0' so that GetOpenFileName does not
	// use the contents of szFile to initialize itself.
	ofn.lpstrFile[0]	= '\0';
	ofn.nMaxFile		= sizeof(szFile);
	ofn.lpstrFilter		= "*.xml\0*.xml\0";
	ofn.nFilterIndex	= 1;
	ofn.lpstrFileTitle	= NULL;
	ofn.nMaxFileTitle	= 0;
	ofn.lpstrInitialDir = szFile;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&ofn)==TRUE)
		return szFile;

	return "";
}

void initSxFxList_RecoreList(HWND hDlg)
{
	HWND		hList;
	LVBKIMAGE	plvbki = {0};
	LVCOLUMN	LvCol;
	InitCommonControls();

	/*
	** init sxfy list
	*/
	hList = GetDlgItem(hDlg, IDC_SXFX_LIST); 

	memset(&plvbki, 0, sizeof(plvbki));
	plvbki.ulFlags			= LVBKIF_SOURCE_URL;
	plvbki.xOffsetPercent	= 40;
	plvbki.yOffsetPercent	= 15;
	OleInitialize(0);

	SendMessage(hList, LVM_SETTEXTCOLOR, 0, RGB(255, 255, 0));
    SendMessage(hList, LVM_SETBKCOLOR, 0, RGB(0, 0, 0));
	SendMessage(hList, LVM_SETTEXTBKCOLOR, 0, (LPARAM)CLR_NONE);
	//SendMessage(hList, LVM_SETBKIMAGE, 0, (LPARAM)(LPLVBKIMAGE)&plvbki);
    SendMessage(hList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT); // Set style
	
	
    memset(&LvCol, 0, sizeof(LvCol)); // Reset Coluom
	LvCol.mask		= LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;	// Type of mask
	
	LvCol.cx		= 70;										// width between each coloum
	LvCol.pszText	= _T("SxFy");								// First Header
	SendMessage(hList, LVM_INSERTCOLUMN, 0, (LPARAM)&LvCol);	// Insert/Show the coloum

	LvCol.cx		= 170;
	LvCol.pszText	= _T("Name");     
    SendMessage(hList, LVM_INSERTCOLUMN, 1, (LPARAM)&LvCol);


	/*
	** init record list
	*/
	hList = GetDlgItem(hDlg, IDC_RECORD_LIST); 

	memset(&plvbki, 0, sizeof(plvbki));
	plvbki.ulFlags			= LVBKIF_SOURCE_URL;
	plvbki.xOffsetPercent	= 40;
	plvbki.yOffsetPercent	= 15;
	OleInitialize(0);

	SendMessage(hList, LVM_SETTEXTCOLOR, 0, RGB(255, 255, 0));
    SendMessage(hList, LVM_SETBKCOLOR, 0, RGB(0, 0, 0));
	SendMessage(hList, LVM_SETTEXTBKCOLOR, 0, (LPARAM)CLR_NONE);
	//SendMessage(hList, LVM_SETBKIMAGE, 0, (LPARAM)(LPLVBKIMAGE)&plvbki);
    SendMessage(hList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT); // Set style*/
	
    memset(&LvCol, 0, sizeof(LvCol)); // Reset Coluom
	LvCol.mask		= LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;	
	
	LvCol.cx		= 120;									
	LvCol.pszText	= _T("Time");								
	SendMessage(hList, LVM_INSERTCOLUMN, 0, (LPARAM)&LvCol);

	LvCol.cx		= 70;									
	LvCol.pszText	= _T("SxFy");								
	SendMessage(hList, LVM_INSERTCOLUMN, 1, (LPARAM)&LvCol);	

	LvCol.cx		= 170;
	LvCol.pszText	= _T("Name");     
    SendMessage(hList, LVM_INSERTCOLUMN, 2, (LPARAM)&LvCol);

	LvCol.cx		= 70;
	LvCol.pszText	= _T("R/S");     
    SendMessage(hList, LVM_INSERTCOLUMN, 3, (LPARAM)&LvCol);
}

void refreshSxFyList(HWND hDlg, TiXmlElement* node)
{
	TiXmlElement*	child;
	HWND			hList;
	LVITEM			LvItem;
	string			sfStr;
	string			nameStr;
	int				iItem;
	int				index;
	UINT			sfnum;

	nameStr = node->Value();
	if(nameStr.compare("Values"))
		return;

	/*
	** refresh sxfy list
	*/
	hList = GetDlgItem(hDlg, IDC_SXFX_LIST);
	SendMessage(hList,LVM_DELETEALLITEMS,0,0);

	for (child = node->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
	{
		nameStr = child->Value();

		index = nameStr.find("_");
		if(index == -1)
			sfStr = nameStr;
		else
			sfStr = nameStr.substr(0, index);

		sfnum = ParseStreamFunction(sfStr);
		if((sfnum & 0xFFFF0000) != 0)
			continue;

		if(s_dtype == HSMS_TYPE_COMMUNICATION)
		{
			GemHsmsCall::Get()->makeAtoms(nameStr, child);
		}
		else
		{
			GemSerialCall::Get()->makeAtoms(nameStr, child);
		}

		iItem = SendMessage(hList, LVM_GETITEMCOUNT, 0, 0);
		memset(&LvItem, 0, sizeof(LvItem));						// Reset Item Struct

		// Setting properties Of Items:
		LvItem.mask			= LVIF_TEXT;						// Text Style
		LvItem.cchTextMax	= 256;								// Max size of test
		LvItem.iItem		= iItem;							// choose item  
		LvItem.iSubItem		= 0;								// Put in first coluom
		LvItem.pszText		= (PSTR)sfStr.c_str();				// Text to display (can be from a char variable) (Items)
		SendMessage(hList, LVM_INSERTITEM, 0, (LPARAM)&LvItem); // Send to the Listview

		LvItem.pszText	= (PSTR)nameStr.c_str();
		LvItem.iSubItem = 1;
		SendMessage(hList, LVM_SETITEM, 0, (LPARAM)&LvItem); // Enter text to SubItems
	} 

	//clear record list
	hList = GetDlgItem(hDlg, IDC_RECORD_LIST);
	SendMessage(hList,LVM_DELETEALLITEMS,0,0);
}

void refreshMsgList(HWND hDlg, TCHAR *newText)
{
	HWND hEdit = GetDlgItem(hDlg, IDC_MSG_EDIT);

	// get new length to determine buffer size
	int outLength = GetWindowTextLength(hEdit) + lstrlen(newText) + 1;

	// create buffer to hold current and new text
	TCHAR* buf = (TCHAR*)GlobalAlloc(GPTR, outLength * sizeof(TCHAR));

	if (!buf) 
		return;

	// get existing text from edit control and put into buffer
	GetWindowText(hEdit, buf, outLength);

	// append the newText to the buffer
	_tcscat_s(buf, outLength, newText);

	// Set the text in the edit control
	SetWindowText(hEdit, buf);
	SendMessage(hEdit, EM_LINESCROLL, 0, (LPARAM)buf);
	//SendMessage(hEdit, EM_SETSEL, 0, MAKELONG(0xffff,0xffff));

	// free the buffer
	GlobalFree(buf);
}

void openCommincation(HWND hDlg)
{
	char	output[256]	= {0};
	HICON	hIcon;
	string	filepath = s_ExePath + "/HsmsSetting.txt";
	string	ip;
	string	line;
	int		port;
	int		index = 0;
	int		deviceId;

	ifstream readfile(filepath.c_str());

	while(getline(readfile, line))
	{
		switch(index)
		{
		case 1:
			ip = line;
			break;

		case 2:
			port = atoi(line.c_str());
			break;

		case 5:
			deviceId = atoi(line.c_str());
			break;
		}
		
		index++;
	}
	readfile.close();

	s_equipment_loop = 1;

	if(s_dtype == HSMS_TYPE_COMMUNICATION)
	{
		GemHsmsCall::Get()->Connect(ip, port, HSMS_MODE_ACTIVE, deviceId, hDlg);
	
		CreateThread(NULL, 0, EquipmentLoopThread, hDlg, 0, NULL);

		refreshMsgList(hDlg, "EAP Emulator v1.4 2016/09/20 HSMS Communication Started!\r\n");
		Logger::Get()->write("EAP Emulator v1.4 2016/09/20 HSMS Communication Started!");
	}
	else
	{
		if(GemSerialCall::Get()->Open(254, deviceId, hDlg) == BEE_SUCCESS)
		{
			refreshMsgList(hDlg, "EAP Emulator v1.4 2016/09/20 SECS-I Communication Started!\r\n");
			Logger::Get()->write("EAP Emulator v1.4 2016/09/20 SECS-I Communication Started!");
		}
		else
		{
			MessageBox(NULL, "通訊埠開啟失敗", "SECS Emulator", MB_ICONEXCLAMATION); 
			return;
		}
	}

	hIcon = (HICON)LoadImage(GetModuleHandle(0), MAKEINTRESOURCE(IDI_GREENCIRCLE_ICON), IMAGE_ICON, 32, 32, 0);
	SendMessage(GetDlgItem(hDlg, IDC_STATIC2), STM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)hIcon);

	EnableMenuItem(GetSubMenu(GetMenu(hDlg), 0), IDC_COMMUNICATE_OPEN, MF_DISABLED | MF_GRAYED);
	EnableMenuItem(GetSubMenu(GetMenu(hDlg), 0), IDC_COMMUNICATE_CLOSE, MF_BYCOMMAND | MF_ENABLED);

	EnableMenuItem(GetSubMenu(GetMenu(hDlg), 2), IDC_SECS_TYPE, MF_DISABLED | MF_GRAYED);
	EnableMenuItem(GetSubMenu(GetMenu(hDlg), 2), IDC_HSMS_TYPE, MF_DISABLED | MF_GRAYED);
}

void closeCommincation(HWND hDlg)
{
	char	output[256] = {0};
	HICON	hIcon;
	
	if(s_equipment_loop == 0)
		return;

	s_equipment_loop = 0;
	if(s_dtype == HSMS_TYPE_COMMUNICATION)
	{
		while( s_play_loop )
		{
			::Sleep(200);
		}

		GemHsmsCall::Get()->Disconnect();

		refreshMsgList(hDlg, "HSMS Communication Disconnected!\r\n");
		Logger::Get()->write("HSMS Communication Disconnected!");
	}
	else
	{
		GemSerialCall::Get()->Close();
		refreshMsgList(hDlg, "SECS-I Communication Disconnected!\r\n");
		Logger::Get()->write("SECS-I Communication Disconnected!");
	}

	hIcon = (HICON)LoadImage(GetModuleHandle(0), MAKEINTRESOURCE(IDI_REDCIRCLE_ICON), IMAGE_ICON, 32, 32, 0);
	SendMessage(GetDlgItem(hDlg, IDC_STATIC2), STM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)hIcon);

	EnableMenuItem(GetSubMenu(GetMenu(hDlg), 0), IDC_COMMUNICATE_OPEN, MF_BYCOMMAND | MF_ENABLED);
	EnableMenuItem(GetSubMenu(GetMenu(hDlg), 0), IDC_COMMUNICATE_CLOSE, MF_DISABLED | MF_GRAYED);

	EnableMenuItem(GetSubMenu(GetMenu(hDlg), 2), IDC_SECS_TYPE, MF_BYCOMMAND | MF_ENABLED);
	EnableMenuItem(GetSubMenu(GetMenu(hDlg), 2), IDC_HSMS_TYPE, MF_BYCOMMAND | MF_ENABLED);
}

void addRecordItem(HWND hDlg, string sxfy, string name, string mode)
{
	HWND	hList;
	LVITEM	LvItem;
	int		iItem;
	char	text[99999]	= {0}; 
	char    timebuf[80]	= {0};

	/*
	** add a recode item
	*/
	hList = GetDlgItem(hDlg, IDC_RECORD_LIST);

	iItem = SendMessage(hList, LVM_GETITEMCOUNT, 0, 0);
	memset(&LvItem, 0, sizeof(LvItem));	

	time_t now = time(0);
	struct tm  tstruct;
	tstruct = *localtime(&now);
	strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %X", &tstruct);
	//strftime(timebuf, sizeof(timebuf), "%X", &tstruct);

	LvItem.mask			= LVIF_TEXT;						
	LvItem.cchTextMax	= 256;								
	LvItem.iItem		= iItem;					
	LvItem.iSubItem		= 0;								
	LvItem.pszText		= timebuf;				
	SendMessage(hList, LVM_INSERTITEM, 0, (LPARAM)&LvItem);

	LvItem.pszText	= (PSTR)sxfy.c_str();
	LvItem.iSubItem = 1;
	SendMessage(hList, LVM_SETITEM, 0, (LPARAM)&LvItem);	// Enter text to SubItems

	LvItem.pszText	= (PSTR)name.c_str();
	LvItem.iSubItem = 2;
	SendMessage(hList, LVM_SETITEM, 0, (LPARAM)&LvItem);
	LvItem.pszText	= (LPSTR)mode.c_str();
	LvItem.iSubItem = 3;
	SendMessage(hList, LVM_SETITEM, 0, (LPARAM)&LvItem);
}

void sendSxFyMsg(HWND hDlg)
{
	HWND	hList;
	LVITEM	LvItem;
	int		iSelected;
	char	text[99999] = {0}; 
	string	sxfyStr;
	string	nameStr;

	hList		= GetDlgItem(hDlg, IDC_SXFX_LIST);
	iSelected	= SendMessage(hList, LVM_GETNEXTITEM, -1, LVNI_FOCUSED);

	if(iSelected == -1)
		return;

	memset(&LvItem, 0, sizeof(LvItem));
    LvItem.mask			= LVIF_TEXT;
    LvItem.iSubItem		= 0;
    LvItem.pszText		= text;
    LvItem.cchTextMax	= 256;
    LvItem.iItem		= iSelected;

	SendMessage(hList, LVM_GETITEMTEXT, iSelected, (LPARAM)&LvItem);
	sxfyStr = text;

	LvItem.iSubItem	= 1;
	SendMessage(hList, LVM_GETITEMTEXT, iSelected, (LPARAM)&LvItem);
	nameStr = text;

	if(s_dtype == HSMS_TYPE_COMMUNICATION)
	{
		GemHsmsCall::Get()->HSMS_SXFX(sxfyStr, nameStr);
	}
	else
	{
		GemSerialCall::Get()->SECS_SXFX(sxfyStr, nameStr);
	}
}

void editSxFyMsg(HWND hDlg)
{
}

void deleteSxFyMsg(HWND hDlg)
{
	HWND	hList;
	int		iSelected;

	hList		= GetDlgItem(hDlg, IDC_SXFX_LIST);
	iSelected	= SendMessage(hList, LVM_GETNEXTITEM, -1, LVNI_FOCUSED);

	if(iSelected == -1)
		return;

	SendMessage(hList, LVM_DELETEITEM, iSelected,0);
}

void initHsmsSetting()
{
	string filepath = s_ExePath + "\\HsmsSetting.txt";

	//ifstream readfile("C:/AlphaProject/EAPEmulator/HsmsSetting.txt");
	ifstream readfile(filepath.c_str());
	if(!readfile)
	{
		ofstream outfile(filepath.c_str()/*, ios::out*/);
		//ofstream outfile("C:/AlphaProject/EAPEmulator/HsmsSetting.txt"/*, ios::out*/);
		outfile << "1\n";
		outfile << "127.0.0.1\n5000\n";
		outfile << "127.0.0.1\n5000\n";
		outfile << "0\n";
		outfile.close();
	}
	readfile.close();
}


DWORD WINAPI EquipmentLoopThread(LPVOID lParam)
{
	HWND	hDlg = (HWND)lParam;

	bool eqpIsConnectCheck	= false;
	char timebuf[80]		= {0};
	char output[99999]		= {0}; 
	time_t nowtime			= time(0);
	time_t oldtime			= time(0);

	s_play_loop = 1;

	while(s_equipment_loop)
	{
		nowtime = time(0);
		struct tm  tstruct;
		tstruct = *localtime(&nowtime);

		strftime(timebuf, sizeof(timebuf), "%X", &tstruct);

		if(GemHsmsCall::Get()->IsConnected() && !eqpIsConnectCheck)
		{
			eqpIsConnectCheck = true;

			sprintf(output, "%s: Equipment Connected.\r\n", timebuf);
			refreshMsgList(hDlg, output);
			Logger::Get()->write(output);
		}
		
		if(!GemHsmsCall::Get()->IsConnected() && eqpIsConnectCheck)
		{
			eqpIsConnectCheck = false;

			s_equipment_loop = 0;

			GemHsmsCall::Get()->Disconnect();

			EnableMenuItem(GetSubMenu(GetMenu(hDlg), 0), IDC_COMMUNICATE_OPEN, MF_BYCOMMAND | MF_ENABLED);
			EnableMenuItem(GetSubMenu(GetMenu(hDlg), 0), IDC_COMMUNICATE_CLOSE, MF_DISABLED | MF_GRAYED);

			refreshMsgList(hDlg, "HSMS Communication Disconnected!\r\n");
			Logger::Get()->write("HSMS Communication Disconnected!\n");
		}

		::Sleep(10);
	}

	s_play_loop = 0;

	return 0;
}
	
BOOL CALLBACK DlgProc(HWND hDlg, UINT Message, WPARAM wParam, LPARAM lParam)
{		
	HDC			hdc;
	PAINTSTRUCT ps;

	switch(Message)
	{
		case WM_KEYDOWN:
         switch (wParam)
         {
          case VK_RETURN:
			  printf("");
              break;  
         }

		case WM_INITDIALOG:
			{
				//set logo
				HICON hIcon;
				hIcon = (HICON) LoadImage(GetModuleHandle(0), MAKEINTRESOURCE(IDI_ICON), IMAGE_ICON, 32, 32, 0);
				SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
				
				//HBITMAP bmp = (HBITMAP)LoadImage(NULL, "C:/AlphaProject/res/template-logo-2.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
				//SendMessage(GetDlgItem(hDlg, IDC_STATIC4), STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)bmp);

				//init list control
				initSxFxList_RecoreList(hDlg);

				//init message list
				HFONT hFont;
				hFont = CreateFont(0,10,0,0,0,0,0,0,0,0,0,0,0,TEXT("Courier New"));
				SendMessage(GetDlgItem(hDlg, IDC_MSG_EDIT), WM_SETFONT, (WPARAM)hFont, 0);

				//init connection icon
				hIcon = (HICON) LoadImage(GetModuleHandle(0), MAKEINTRESOURCE(IDI_REDCIRCLE_ICON), IMAGE_ICON, 32, 32, 0);
				SendMessage(GetDlgItem(hDlg, IDC_STATIC2), STM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)hIcon);

				//SendMessage(GetDlgItem(hDlg, IDC_MSG_EDIT), EM_SETSEL, 0, 0);
			}
			break;

		/*case WM_CTLCOLORSTATIC:
			{
				HDC hEdit = (HDC)wParam;
				SetTextColor(hEdit, RGB(0, 0, 0));
				SetBkColor(hEdit, RGB(255, 255, 255));

				// Do not return a brush created by CreateSolidBrush(...) because you'll get a memory leak
				return (INT_PTR)GetStockObject( WHITE_BRUSH );
			}
			break;*/

		case WM_NOTIFY:
			{
				switch(LOWORD(wParam))
				{
				case IDC_SXFX_LIST:
					{
						if(((LPNMHDR)lParam)->code == NM_DBLCLK)
						{
							int iSelected = SendMessage(GetDlgItem(hDlg, IDC_SXFX_LIST), LVM_GETNEXTITEM, -1, LVNI_FOCUSED);

							if(iSelected == -1)
								break;

							DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_MSG_STRUCT_DIALOG), hDlg, msgDlgProc);
						}	

						if(((LPNMHDR)lParam)->code == NM_CLICK)
						{
						}

						if(((LPNMHDR)lParam)->code == NM_RCLICK)
						{
							HMENU hpopMenu;
							POINT cursor;
							
							hpopMenu = CreatePopupMenu();
							GetCursorPos(&cursor);

							InsertMenu(hpopMenu, 0, MF_BYCOMMAND | MF_STRING | MF_ENABLED, IDC_SXFX_SEND, "Send");
							/*if(GemHsmsCall::Get()->IsConnected())
							{
								InsertMenu(hpopMenu, 0, MF_BYCOMMAND | MF_STRING | MF_ENABLED, IDC_SXFX_SEND, "Send");
							}
							else
							{
								InsertMenu(hpopMenu, 0, MF_GRAYED | MF_STRING | MF_DISABLED, IDC_SXFX_SEND, "Send");
							}*/

							//InsertMenu(hpopMenu, 0, MF_BYCOMMAND | MF_STRING | MF_ENABLED, IDC_SXFX_EDIT, "Edit");
							InsertMenu(hpopMenu, 0, MF_BYCOMMAND | MF_STRING | MF_ENABLED, IDC_SXFX_DELETE, "Delete");

							TrackPopupMenu(hpopMenu, TPM_TOPALIGN | TPM_LEFTALIGN, cursor.x, cursor.y, 0, hDlg, NULL);
						}

						if(((LPNMHDR)lParam)->code == LVN_BEGINLABELEDIT)
						{
						}
				
						if(((LPNMHDR)lParam)->code == LVN_ENDLABELEDIT)
						{
		                   
						}
					}
					break;

				case IDC_RECORD_LIST:
					if(((LPNMHDR)lParam)->code == NM_RCLICK)
					{
						printf("");
					}
					break;
				}
			}
			break;

		case WM_COMMAND:
				//notification
			 if((HIWORD(wParam) == EN_CHANGE) &&  (LOWORD(wParam) == IDC_MSG_EDIT))
			 {
				 HWND hEdit = GetDlgItem(hDlg, IDC_MSG_EDIT);

				 int outLength = GetWindowTextLength(hEdit);
				 
				 if(outLength >= 14)
				 {
					 TCHAR* buf = (TCHAR*)GlobalAlloc(GPTR, outLength * sizeof(TCHAR));
					 GetWindowText(hEdit, buf, outLength);
					 Logger::Get()->write(buf);
					 SetWindowText(hEdit, "");
				 }

				 //反白
				 //SendMessage(hText,EM_SETSEL,0,lstrlen(TEXT("Test")));
				 //SetFocus(hText);
			 }

			switch(LOWORD(wParam))
			{

			/*case IDC_MSG_EDIT:
				if(GetFocus()) 
					SetDlgItemText(hDlg,IDC_MSG_EDIT,"Enter PRessed");
				break;*/

			case IDC_BUTTON1:
				{
				}
				break;

			case IDC_COMMUNICATE_OPEN:
				{
					make_msgtype();
					openCommincation(hDlg);
				}
				break;

			case IDC_COMMUNICATE_CLOSE:
				{
					closeCommincation(hDlg);
					close_msgtype();
				}
				break;

			case IDC_XML_READ:
				{
					TiXmlDocument	doc;
					string			path;
					string			filename;
					char			output[99999];

					path = GetFileName(hDlg);
					if(path != "")
					{
						doc.LoadFile(path.c_str());
						refreshSxFyList(hDlg, doc.FirstChildElement());

						filename = path.substr(path.find_last_of("/\\")+1, path.length());
			
						sprintf(output, "Read SECS-II Messages from %s\r\n", filename.c_str());
						refreshMsgList(hDlg, output);

						//sprintf(output, "Read SECS-II Messages from %s", filename.c_str());
						Logger::Get()->write(output);
					}
				}
				break;

			case IDC_SXFX_SEND:
				{
					sendSxFyMsg(hDlg);
				}
				break;

			case IDC_SXFX_EDIT:
				break;

			case IDC_SXFX_DELETE:
				{
					deleteSxFyMsg(hDlg);
				}
				break;

			case IDC_MSG_CLEAR:
				{
					SetWindowText(GetDlgItem(hDlg, IDC_MSG_EDIT), "");
				}
				break;

			case IDC_SECS_SETTING:
				{
				}	
				break;

			case IDC_HSMS_SETTING:
				{
					DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_HSMS_SETTING_DIALOG), hDlg, hsmsDlgProc);
				}	
				break;

			case IDC_SECS_TYPE:
				s_dtype = SECS_TYPE_COMMUNICATION;
				CheckMenuItem(GetSubMenu(GetMenu(hDlg), 2), IDC_SECS_TYPE, MF_CHECKED);
				CheckMenuItem(GetSubMenu(GetMenu(hDlg), 2), IDC_HSMS_TYPE, MF_UNCHECKED);
				break;

			case IDC_HSMS_TYPE:
				s_dtype = HSMS_TYPE_COMMUNICATION;
				CheckMenuItem(GetSubMenu(GetMenu(hDlg), 2), IDC_HSMS_TYPE, MF_CHECKED);
				CheckMenuItem(GetSubMenu(GetMenu(hDlg), 2), IDC_SECS_TYPE, MF_UNCHECKED);
				break;

			case IDC_ABOUT_EAP:
				break;

			case IDC_DIALOG_OVER:
				PostMessage(hDlg, WM_QUIT, 0, 0);
				break;
			}
			break;

		case WM_PAINT:
			hdc = BeginPaint(hDlg, &ps);
			// TODO: Add any drawing code here...

			EndPaint(hDlg, &ps);
			break;

		case WM_SIZE:
			break;
		
		case WM_DESTROY:		
			PostMessage(hDlg, WM_QUIT, 0, 0);
			break;
		
		case WM_CLOSE:
			PostMessage(hDlg, WM_QUIT, 0, 0);
			break;
		
		default:
			return FALSE;
	}
	
	return TRUE;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	if(FindWindow(NULL, "EAPEmulator"))
	{
		return 0;
	}

	HWND	hDlg;
	HWND	hAct;
	MSG		msg; 

	//open GUI windows
	hDlg = CreateDialogParam(hInstance, MAKEINTRESOURCE(IDD_MAIN), 0, DlgProc, 0);
	//SetWindowPos(hDlg, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
    CreateMainMenu(hDlg);
    ShowWindow(hDlg, nCmdShow);
	UpdateWindow(hDlg);

	s_ExePath = ExePath();

	memset(&msg,0,sizeof(MSG));

	initHsmsSetting();

	while ( msg.message != WM_QUIT )
	{
		GetMessage(&msg, NULL, 0, 0);

		if( IsDialogMessage(hDlg, &msg) )
		{
			hAct = GetActiveWindow();

			if( hAct == hDlg )
				UpdateWindow(hDlg);
		}
		else
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		} 
	}
	
	closeCommincation(hDlg);
	close_msgtype();

	Logger::Free();
	GemHsmsCall::Free();
	GemSerialCall::Free();

	return (int)msg.wParam;
}