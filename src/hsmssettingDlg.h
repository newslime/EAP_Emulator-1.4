#ifndef HSMS_SETTING_DLG_H
#define HSMS_SETTING_DLG_H

BOOL CALLBACK hsmsDlgProc(HWND hHsmsDlg, UINT Message, WPARAM wParam, LPARAM lParam);

void getHsmsSetting(HWND hHsmsDlg);
void saveHsmsSetting(HWND hHsmsDlg);
void hsmsDlgPositon(HWND hHsmsDlg);

#endif