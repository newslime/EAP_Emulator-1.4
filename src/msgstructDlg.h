#ifndef MSG_STRUCT_DLG_H
#define MSG_STRUCT_DLG_H

BOOL CALLBACK msgDlgProc(HWND hMsgDlg, UINT Message, WPARAM wParam, LPARAM lParam);

void msgDlPositon(HWND hMsgDlg);
void outputList(HWND hMsgDlg, BS2List* listAtom, int& nest, int& index);
void outputValue(HWND hMsgDlg, BS2Atom* atom, int& nest, int& index);

#endif