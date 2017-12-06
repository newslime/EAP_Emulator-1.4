
#ifndef GEM_HSMSCALL_H
#define GEM_HSMSCALL_H

class GemHsmsCall
{
public:

	static GemHsmsCall*	Get();
	static void			Free();

    GemHsmsCall();
    ~GemHsmsCall();

	void Connect(string hostname, int port, int mode, int deviceId, void* tag);
	void Disconnect();
	bool IsConnected();

	/*
	**-- make sxfy atoms(values)
	*/
	int makeAtoms(string name, TiXmlElement* node);
	bool getSxFyAtoms(string name, BS2Atoms& atoms);

	/*
	**-- Hsms send function
	*/
	void HSMS_SXFX(string sxfy, string sxfyName, UINT transactionNum = 0);
	void Hsms_Send(BS2Message* msg);

	/*
	**-- Hsms response
	*/
	int Hsms_Response(BS2MessageInfo* binfo);

protected:
	int	listElements(TiXmlElement* node, BS2List* listParent);
	BS2Atom* valueElement(TiXmlElement* node);

	/*
	**-- Output send and receive
	*/
	bool parseAtomList(BS2Itemtypes* listStruct, BS2List* listAtom, BS2ListItem* rootlist);

	void parseItemList(string& text, string& log, BS2ListItem* listItem);
	void parseValue(string& text, string& log, BS2Atom* atom);

	void EraseAtoms_Recursive(BS2List* list_atom);

	void outputMessage(BS2Message* msg, string mode);

	map<string, BS2Atoms>	m_sxfyAtomsMap;
	wxCriticalSection		m_mutex_gemdriver;
	void*					m_tag;

	GemDevice*		m_gemhsms_device;
	GemSender*		m_gemhsms_sender;
	GemReceiver*	m_gemhsms_receiver;
};

#endif // GEM_HSMSCALL_H
