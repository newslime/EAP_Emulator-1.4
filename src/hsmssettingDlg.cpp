
#include <fstream>
#include <string>
#include <windows.h>

#include "../res/resource.h"
#include "hsmssettingDlg.h"

using namespace std;

extern string s_ExePath;

BOOL CALLBACK hsmsDlgProc(HWND hHsmsDlg, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch(Message)
	{
		case WM_KEYDOWN:
			MessageBox(hHsmsDlg, "KEY", "KEY", MB_OK | MB_ICONINFORMATION);
			break;

		case WM_INITDIALOG:
			{
				hsmsDlgPositon(hHsmsDlg);
				getHsmsSetting(hHsmsDlg);
				return TRUE;
			}
			break;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
			case IDOK:
				saveHsmsSetting(hHsmsDlg);
				break;

			case IDCANCEL:
				EndDialog(hHsmsDlg, 0);
				break;

			case IDC_RADIO_PASSIVE:
				break;

			case IDC_RADIO_ACTIVE:
				break;
			}
			break;
		
		case WM_CLOSE:
			EndDialog(hHsmsDlg, 0);
			break;
		
		default:
			return FALSE;
	}

	return 0;
}

void hsmsDlgPositon(HWND hHsmsDlg)
{
	HWND hwndOwner;
	RECT rc, rcDlg, rcOwner; 

	// Get the owner window and dialog box rectangles. 
	if ((hwndOwner = GetParent(hHsmsDlg)) == NULL) 
	{
		hwndOwner = GetDesktopWindow(); 
	}

	//set the position to center
	GetWindowRect(hwndOwner, &rcOwner); 
	GetWindowRect(hHsmsDlg, &rcDlg); 
	CopyRect(&rc, &rcOwner); 

	OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top); 
	OffsetRect(&rc, -rc.left, -rc.top); 
	OffsetRect(&rc, -rcDlg.right, -rcDlg.bottom); 

	SetWindowPos(hHsmsDlg, 
				 HWND_TOP, 
				 rcOwner.left + (rc.right / 2), 
				 rcOwner.top + (rc.bottom / 2), 
				 0, 0,          // Ignores size arguments. 
				 SWP_NOSIZE);
}

void getHsmsSetting(HWND hHsmsDlg)
{
	//set role, ip, port;
	string	filepath = s_ExePath + "/HsmsSetting.txt";
	string	line;
	int		index = 0;

	//ifstream readfile("C:/AlphaProject/EAPEmulator/HsmsSetting.txt");
	ifstream readfile(filepath.c_str());

	while(getline(readfile, line))
	{
		switch(index)
		{
		case 0:
			if(atoi(line.c_str()) == 1)
				SendMessage(GetDlgItem(hHsmsDlg, IDC_RADIO_ACTIVE), BM_SETCHECK, BST_CHECKED, 1);
			else
				SendMessage(GetDlgItem(hHsmsDlg, IDC_RADIO_PASSIVE), BM_SETCHECK, BST_CHECKED, 1);
			break;

		case 1:
			SetDlgItemText(hHsmsDlg, IDC_IPADDRESS_REMOTE, line.c_str());
			break;

		case 2:
			SetDlgItemText(hHsmsDlg, IDC_PORT_REMOTE, line.c_str());
			break;

		case 3:
			SetDlgItemText(hHsmsDlg, IDC_IPADDRESS_LOCAL, line.c_str());
			break;

		case 4:
			SetDlgItemText(hHsmsDlg, IDC_PORT_LOCAL, line.c_str());
			break;

		case 5:
			SetDlgItemText(hHsmsDlg, IDC_DEVICE_ID, line.c_str());
			break;
		}
		
		index++;
	}
}

void saveHsmsSetting(HWND hHsmsDlg)
{
	string	filepath = s_ExePath + "/HsmsSetting.txt";
	char	role;
	char	remoteIp[255];    
	char	remotePort[255]; 
	char	localIp[255]; 
	char	localPort[255]; 
	char	deviceId[255]; 

	if(IsDlgButtonChecked(hHsmsDlg, IDC_RADIO_ACTIVE))
	{
		role = '1';
	}
	else
	{
		role = '0';
	}

	GetDlgItemText(hHsmsDlg, IDC_IPADDRESS_REMOTE, (LPSTR)&remoteIp, 255);
	GetDlgItemText(hHsmsDlg, IDC_PORT_REMOTE, (LPSTR)&remotePort, 255);
	GetDlgItemText(hHsmsDlg, IDC_IPADDRESS_LOCAL, (LPSTR)&localIp, 255);
	GetDlgItemText(hHsmsDlg, IDC_PORT_LOCAL, (LPSTR)&localPort, 255);
	GetDlgItemText(hHsmsDlg, IDC_DEVICE_ID, (LPSTR)&deviceId, 255);

	ofstream outfile(filepath.c_str());

	outfile << role << endl;
	outfile << remoteIp << endl;
	outfile << remotePort << endl;
	outfile << localIp << endl;
	outfile << localPort << endl;
	outfile << deviceId << endl;

	EndDialog(hHsmsDlg, 0);
}