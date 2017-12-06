
#include <fstream>
#include <winsock2.h>
#include <windows.h>
#include <tchar.h>
#include <commctrl.h>

#include "../res/resource.h"
#include "tinyxml/tinyxml.h"
#include "GemSDK.h"
#include "GemHsmsCall.h"
#include "msgstructDlg.h"

using namespace std;

BOOL CALLBACK msgDlgProc(HWND hMsgDlg, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch(Message)
	{
		case WM_INITDIALOG:
			{
				HWND	hList;
				LVITEM	LvItem;
				int		iSelected;
				char	text[255]	= {0}; 
				string	sxfyStr;
				string	nameStr;
				int		index	= 0;
				int		nest	= 0;

				msgDlPositon(hMsgDlg);
				
				//get sxfy name
				hList		= GetDlgItem(GetParent(hMsgDlg), IDC_SXFX_LIST);
				iSelected	= SendMessage(hList, LVM_GETNEXTITEM, -1, LVNI_FOCUSED);

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

				sxfyStr = nameStr + ":" + sxfyStr;
				SendDlgItemMessage(hMsgDlg, IDC_MSG_STRUCT, LB_INSERTSTRING, index++, (LPARAM)sxfyStr.c_str());

				//output message structure
				BS2Atoms atoms;
				BS2Atom* atom;

				if(GemHsmsCall::Get()->getSxFyAtoms(nameStr, atoms))
				{
					for(int i = 0; i < atoms.size(); i++)
					{
						atom = reinterpret_cast<BS2Atom *>(atoms[i]);

						if(atom->isList())
						{
							outputList(hMsgDlg, (BS2List*)atom, nest, index);
						}
						else
						{
							outputValue(hMsgDlg, atom, nest, index);
						}
					}
					SendDlgItemMessage(hMsgDlg, IDC_MSG_STRUCT, LB_INSERTSTRING, index++, (LPARAM)".");
				}

				// resize font
				HFONT	hFont;
				hFont = CreateFont(0,9,0,0,0,0,0,0,0,0,0,0,0,TEXT("Courier New"));
				SendMessage(GetDlgItem(hMsgDlg, IDC_MSG_STRUCT), WM_SETFONT, (WPARAM)hFont, 0);
			}
			break;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
			case IDCANCEL:
				EndDialog(hMsgDlg, 0);
				break;
			}
			break;
		
		case WM_CLOSE:
			EndDialog(hMsgDlg, 0);
			break;
		
		default:
			return FALSE;
	}

	return 0;
}

void msgDlPositon(HWND hMsgDlg)
{
	HWND hwndOwner;
	RECT rc, rcDlg, rcOwner; 

	// Get the owner window and dialog box rectangles. 
	if ((hwndOwner = GetParent(hMsgDlg)) == NULL) 
	{
		hwndOwner = GetDesktopWindow(); 
	}

	//set the position to center
	GetWindowRect(hwndOwner, &rcOwner); 
	GetWindowRect(hMsgDlg, &rcDlg); 
	CopyRect(&rc, &rcOwner); 

	OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top); 
	OffsetRect(&rc, -rc.left, -rc.top); 
	OffsetRect(&rc, -rcDlg.right, -rcDlg.bottom); 

	SetWindowPos(hMsgDlg, 
				 HWND_TOP, 
				 rcOwner.left + (rc.right / 2), 
				 rcOwner.top + (rc.bottom / 2), 
				 0, 0,          // Ignores size arguments. 
				 SWP_NOSIZE); 
}

void outputList(HWND hMsgDlg, BS2List* listAtom, int& nest, int& index)
{
	char		msg[64];
	BS2Atoms&	atoms = listAtom->getList();
	BS2Atom*	atom;

	string space(nest,' ');
	sprintf(msg, "%s<L>[%d]", space.c_str(), atoms.size());
	SendDlgItemMessage(hMsgDlg, IDC_MSG_STRUCT, LB_INSERTSTRING, index++, (LPARAM)msg);

	for (unsigned int i = 0; i < atoms.size(); i++)
	{
		atom = reinterpret_cast<BS2Atom *>(atoms[i]);
		if(atom->isList())
		{
			nest += 2;
			outputList(hMsgDlg, (BS2List*)atom, nest, index);
			nest -= 2;
		}
		else
		{
			nest += 2;
			outputValue(hMsgDlg, atom, nest, index);
			nest -= 2;
		}
	}

	sprintf(msg, "%s</L>", space.c_str());
	SendDlgItemMessage(hMsgDlg, IDC_MSG_STRUCT, LB_INSERTSTRING, index++, (LPARAM)msg);
}

void outputValue(HWND hMsgDlg, BS2Atom* atom, int& nest, int& index)
{
	char	msg[64];
	string	space(nest,' ');

	if(atom->isAscii())
	{
		sprintf(msg, "%s<A>%s</A>", space.c_str(), atom->toString().c_str());
	}
	else if(atom->isBinary())
	{
		if(atom->getInt())
			sprintf(msg, "%s<B>0x01</B>", space.c_str());
		else
			sprintf(msg, "%s<B>0x00</B>", space.c_str());
	}
	else if(atom->isBoolean())
	{
		if(atom->getBool())
			sprintf(msg, "%s<Boolean>0x01</Boolean>", space.c_str());
		else
			sprintf(msg, "%s<Boolean>0x00</Boolean>", space.c_str());
	}
	else if(atom->isFloat4())
	{
		sprintf(msg, "%s<F4>%f</F4>", space.c_str(), atom->getfloat());
	}
	else if(atom->isFloat8())
	{
		sprintf(msg, "%s<F8>%f</F8>", space.c_str(), atom->getfloat());
	}
	else if(atom->isInt1())
	{
		sprintf(msg, "%s<I1>%d</I1>", space.c_str(), atom->getInt());
	}
	else if(atom->isInt2())
	{
		sprintf(msg, "%s<I2>%d</I2>", space.c_str(), atom->getInt());
	}
	else if(atom->isInt4())
	{
		sprintf(msg, "%s<I4>%d</I4>", space.c_str(), atom->getInt());
	}
	else if(atom->isInt8())
	{
		sprintf(msg, "%s<I8>%d</I8>", space.c_str(), atom->getInt());
	}
	else if(atom->isUInt1())
	{
		sprintf(msg, "%s<U1>%d</U1>", space.c_str(), atom->getUInt());	
	}
	else if(atom->isUInt2())
	{
		sprintf(msg, "%s<U2>%d</U2>", space.c_str(), atom->getUInt());
	}
	else if(atom->isUInt4())
	{
		sprintf(msg, "%s<U4>%d</U4>", space.c_str(), atom->getUInt());
	}
	else if(atom->isUInt8())
	{
		sprintf(msg, "%s<U8>%d</U8>", space.c_str(), atom->getUInt());
	}

	SendDlgItemMessage(hMsgDlg, IDC_MSG_STRUCT, LB_INSERTSTRING, index++, (LPARAM)msg);
}