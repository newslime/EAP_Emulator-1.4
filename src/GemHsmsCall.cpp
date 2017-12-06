
#include <fstream>
#include <map>

#include "GemSDK.h"
#include "logger.h"

static GemHsmsCall* s_GemHsmsCall_Instance = NULL;

static int s_nest;

extern int	ParseStreamFunction(string& sfstr);
extern void refreshMsgList(HWND hDlg, TCHAR *newText);
extern void addRecordItem(HWND hDlg, string sxfy, string name, string mode);

GemHsmsCall* GemHsmsCall::Get()
{
	if (!s_GemHsmsCall_Instance)
		s_GemHsmsCall_Instance = new GemHsmsCall();
	
	return s_GemHsmsCall_Instance;
}

void GemHsmsCall::Free()
{
	delete s_GemHsmsCall_Instance;
	s_GemHsmsCall_Instance = NULL;
}

GemHsmsCall::GemHsmsCall()
{
	m_gemhsms_device = NULL;
}

GemHsmsCall::~GemHsmsCall()
{
	map<string, BS2Atoms>::iterator itr;
	BS2Atoms						atoms;
	BS2Atom*						atom;
	unsigned int					atomq;
	unsigned int					size;

	itr		= m_sxfyAtomsMap.begin();
	size	= m_sxfyAtomsMap.size();
	
	for(int i = 0; i < size; i++)
	{
		atoms = itr->second;
		atomq = atoms.size();

		for(unsigned int j = 0; j < atomq; j++)
		{
			atom = reinterpret_cast<BS2Atom *>(atoms[j]);

			if(atom->isList())
			{
				BS2List* list_atom = (BS2List*)atom;
				EraseAtoms_Recursive(list_atom);
				delete list_atom;
			}
			else if(atom->isArray())
			{
				//nothing
			}
			else
			{
				delete atom;
			}
		}

		m_sxfyAtomsMap.erase(itr);
		itr = m_sxfyAtomsMap.begin();
	}

	m_sxfyAtomsMap.clear();
}

void GemHsmsCall::Connect(string hostname, int port, int mode, int deviceId, void* tag)
{
	m_tag = tag;

	DeviceParameter*	config;
	SocketParameter*	parm;

	ACE::init();
		
	config				= new SocketParameter();
	m_gemhsms_device	= new GemDevice(this);
	m_gemhsms_sender	= new GemSender(this, m_gemhsms_device);
	m_gemhsms_receiver	= new GemReceiver(this, m_gemhsms_device);
	
	parm = (SocketParameter*)config;
	parm->m_mode		= mode;
	parm->m_slave		= 1;
	parm->m_dtype		= DRIVER_SOCKET;
	parm->m_port		= port;
	parm->m_deviceId	= deviceId;

	if(mode == HSMS_MODE_ACTIVE)
		sprintf(parm->m_hostname, hostname.c_str());

	sprintf(parm->m_xmlname, "./xml/secs.xml");

	if(m_gemhsms_device->initialize(parm, m_gemhsms_sender, m_gemhsms_receiver) == BEE_SUCCESS)
		m_gemhsms_device->open();
	
	delete config;
}

void GemHsmsCall::Disconnect()
{
	if(m_gemhsms_device)
	{
		delete m_gemhsms_device;
		m_gemhsms_device = NULL;

		ACE::fini();
	}
}

bool GemHsmsCall::IsConnected()
{
	if(m_gemhsms_device)
	{
		int status = ((BS2Socket*)(m_gemhsms_device)->getDriver())->getStatus();
		
		if(status == 2)
			return true;
	}

	return false;
}

/*
**-- Hsms send Hsms send function
*/
void GemHsmsCall::HSMS_SXFX(string sxfy, string name, UINT transactionNum)
{
	//item type and atom
	BS2Atoms		atoms;
	BS2Atom*		atom;
	BS2MessageType* msgtype;
	BS2ItemType*	itemtype;
	size_t			itemCount;
	string			itemName;
	bool			retval;
	UINT			sfnum;
	UINT			t_id;

	//send message
	BS2Message*		msg_body; 
	BS2ListItem*	rootlist; 
	BS2Item*		item_value;

	map<string, BS2Atoms>::iterator itr;

	if(sxfy.compare("S7F3") == 0)
	{
		int length		= 0;
		char* ppbody	= NULL;// = (BYTE*) malloc (sizeof(char)*size);

		ifstream readfile("C:\\myfile.bin", ios::binary);
		if(readfile)
		{
			readfile.seekg (0, ios::end);
			length = readfile.tellg();
			readfile.seekg (0, ios::beg);

			//ppbody = (BYTE*) malloc (sizeof(char)*length);
			ppbody = new char [length];
			readfile.read (ppbody, length);
			readfile.close();
		}

		msg_body = BS2Message::factory(SFCODE(7, 3));
		rootlist = new BS2ListItem();

		item_value	= BS2Item::factory(_TX("PPID"), new BS2Ascii("WBF2497"));
		rootlist->add(item_value);

		item_value	= BS2Item::factory(_TX("PPBODY"), new BS2Binary((BYTE*)ppbody, length));
		rootlist->add(item_value);

		msg_body->add(rootlist);
		free(ppbody);
	}
	else
	{
		itr = m_sxfyAtomsMap.find(name);
		if(itr == m_sxfyAtomsMap.end())
			return;

		atoms		= itr->second;
		sfnum		= ParseStreamFunction(sxfy);
		msgtype		= BS2MessageDictionary::instance()->search(sfnum);
		msg_body	= BS2Message::factory(sfnum);
		itemCount	= 0;

		for(int i = 0; i < atoms.size(); i++)
		{
			atom		= reinterpret_cast<BS2Atom *>(atoms[i]);
			itemtype	= msgtype->body().at(itemCount);

			if (!(itemtype->availFormat(atom->format() & ~ATOM_ARRAY)))
			{
				//Logger::Get()->write(name + " Illegal HSMS Format.\n");
				MessageBox(NULL, "Illegal HSMS Format", "HSMS", MB_ICONEXCLAMATION ); 
				return;
			}
			
			if(atom->isList() && itemtype->isList())
			{
				rootlist = new BS2ListItem();
				
				retval = parseAtomList((BS2Itemtypes*)itemtype, (BS2List*)atom, rootlist);
				
				if(!retval) //list error
					return;

				msg_body->add(rootlist);
			}
			else
			{
				itemName	= itemtype->itemName();
				item_value	= BS2Item::factory(_TX(itemName.c_str()), BS2Atom::factory(atom));
				msg_body->add(item_value);
			}

			itemCount++;
			if (itemCount >= msgtype->body().size())
				break; 
		}
	}

	if(msg_body->isSecondary())
		msg_body->transNum(transactionNum);

	if(m_gemhsms_sender)
	{
		t_id = m_gemhsms_sender->send(msg_body);

		if(msg_body->isPrimary())
		{
			transactionNum = MAKE_TRANSACTION_IDENTIFIER(1, t_id);
			msg_body->transNum(transactionNum);
		}

		outputMessage(msg_body, "Send");
	}

	delete msg_body;
}

void GemHsmsCall::Hsms_Send(BS2Message* msg)
{
	if(m_gemhsms_sender)
		m_gemhsms_sender->send(msg);
}


/*
**-- make sxfy atoms(values)
*/
int GemHsmsCall::makeAtoms(string name, TiXmlElement* node)
{
	BS2Atoms		atoms;
	BS2List*		listAtom;
	BS2Atom*		atom = 0;
	TiXmlElement*	child;
	string			nameStr;
	int				nodeType;
	int				result = 0;

	nodeType = node->Type();
	switch (nodeType)
    {
	case TiXmlNode::ELEMENT:
        {
			for (child = node->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
			{
				nameStr = child->Value();

				if(!nameStr.compare("L"))
				{
					listAtom = new BS2List;
					atoms.push_back(listAtom);
					result = listElements(child, listAtom);

					if(result < 0)
						return result;
				}
				else
				{
					atom = valueElement(child);

					if(atom == NULL)
						return -1;

					atoms.push_back(atom);
				}
			}
		}
		break;

	default:
		break;
	}

	m_sxfyAtomsMap[name] = atoms;
	return result;
}

bool GemHsmsCall::getSxFyAtoms(string name, BS2Atoms& atom)
{
	map<string, BS2Atoms>::iterator itr;

	itr = m_sxfyAtomsMap.find(name);

	if(itr == m_sxfyAtomsMap.end())
		return false;

	atom = itr->second;
	
	return true;
}

int GemHsmsCall::listElements(TiXmlElement* node, BS2List* listParent)
{
	BS2Atom*		atom;
	BS2List*		listChild;
	TiXmlElement*	child; 
	string			nameStr;
	int				nodeType;
	int				result = 0;

	nodeType = node->Type();
	switch (nodeType)
    {
	case TiXmlNode::ELEMENT:
        {
			for (child = node->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
			{
				nameStr = child->Value();

				if(!nameStr.compare("L"))
				{
					listChild = new BS2List;
					listParent->getList().push_back((BS2Atom *)listChild);
					result = listElements(child, listChild);

					if(result < 0)
						return result;
				}
				else
				{
					atom = valueElement(child);

					if(atom == NULL)
						return -1;

					listParent->getList().push_back(atom);
				}
			}
		}
		break;

	default:
		break;
	}

	return result;
}

BS2Atom* GemHsmsCall::valueElement(TiXmlElement* node)
{
	BS2Atom* atom = NULL;

	string attr;
	string value;

	attr = node->Value();
	if(!node->GetText())
		value = "";
	else
		value = node->GetText();

	if(!attr.compare("A"))
	{
		atom = new BS2Ascii(value);
	}
	else if(!attr.compare("B"))
	{
		int index = value.find("x");
		atom = new BS2Binary((BYTE)atoi(value.substr(index+1, value.length()).c_str()));
	}
	else if(!attr.compare("Boolean"))
	{
		int index = value.find("x");

		if(atoi(value.substr(index+1, value.length()).c_str()))
			atom = new BS2Boolean(true);
		else
			atom = new BS2Boolean(false);
	}
	else if(!attr.compare("F4"))
	{
		atom = new BS2Float4((float)atof(value.c_str()));
	}
	else if(!attr.compare("F8"))
	{
		atom = new BS2Float8((float)atof(value.c_str()));
	}
	else if(!attr.compare("I1"))
	{
		atom = new BS2Int1(atoi(value.c_str()));
	}
	else if(!attr.compare("I2"))
	{
		atom = new BS2Int2(atoi(value.c_str()));
	}
	else if(!attr.compare("I4"))
	{
		atom = new BS2Int4(atoi(value.c_str()));
	}
	else if(!attr.compare("I8"))
	{
		atom = new BS2Int8(atoi(value.c_str()));
	}
	else if(!attr.compare("U1"))
	{
		atom = new BS2UInt1(atoi(value.c_str()));
	}
	else if(!attr.compare("U2"))
	{
		atom = new BS2UInt2(atoi(value.c_str()));
	}
	else if(!attr.compare("U4"))
	{
		atom = new BS2UInt4(atoi(value.c_str()));
	}
	else if(!attr.compare("U8"))
	{
		atom = new BS2UInt8(atoi(value.c_str()));
	}

	return atom;
}


/*
**-- Hsms response callback
*/
int GemHsmsCall::Hsms_Response(BS2MessageInfo* binfo)
{
	BS2Message*		revMsg		= binfo->getMessage();
	BS2Message*		msg_body;
	BS2ListItem*	rootlist;
	BS2ListItem*	paramlist;
	BS2Item*		item_value;
	char			sxfy[32]	= {0}; 
	BS2Atoms		atoms;
	BS2Atom*		atom;
	UINT			snum;
	UINT			fnum;

	outputMessage(revMsg, "Receive");

	snum = STREAMCODE(revMsg->sf());
	fnum = FUNCCODE(revMsg->sf());

	if(revMsg->isPrimary() && (snum != 9 && fnum != 7))
	{
		//response
		sprintf(sxfy, "S%dF%d", snum, fnum+1);
		if(GemHsmsCall::Get()->getSxFyAtoms(sxfy, atoms))
		{
			GemHsmsCall::Get()->HSMS_SXFX(sxfy, sxfy, revMsg->transNum());
		}
		else
		{
			if(snum == 1 && fnum == 13)
			{
				msg_body = BS2Message::factory(SFCODE(1, 14));
				rootlist	= new BS2ListItem();
				paramlist	= new BS2ListItem();

				item_value = BS2Item::factory(_TX("COMMACK"),new BS2Binary((BYTE)0x00));

				rootlist->add(item_value);
				rootlist->add(paramlist);
				msg_body->add(rootlist);
				msg_body->transNum(binfo->getTransactionID());
			}
			else
			{
				msg_body = BS2Message::response(revMsg);
			}

			GemHsmsCall::Get()->Hsms_Send(msg_body);
			GemHsmsCall::Get()->outputMessage(msg_body, "Send");
			delete msg_body;
		}

		return 1;
	}
	else if(revMsg->isSecondary())
	{
		if(snum == 7 && fnum == 6)
		{
			BYTE*	ppbody;
			string	ppid;

			atom = revMsg->getAtom("PPID");
			ppid = atom->toString();

			atom = revMsg->getAtom("PPBODY");
			int length = atom->size();
			
			ppbody = (BYTE *)malloc(length);
			atom->getStreamData(ppbody);
			
			string filename = "C:\\" + ppid;
			std::ofstream file(filename.c_str(), std::ios::binary);
			file.write((char*)ppbody, length);
			file.close();

			free (ppbody);
		}
	}

	return 0;
}

void GemHsmsCall::outputMessage(BS2Message* msg, string mode)
{
	HWND hDlg = (HWND)m_tag;

	BS2Item*					item;
	BS2ItemList					items	= msg->items();
	list<BS2Item*>::iterator	itr		= items.begin();
	BS2Atoms					atoms;
	
	string		text			= msg->name() + "\r\n";
	string		log				= msg->name() + "\n";
	char		timebuf[80]		= {0};
	char		output[99999]	= {0}; 
	time_t		now;
	struct tm	tstruct;

	s_nest = 0;

	for(int i = 0; i < items.size(); i++)
	{
		item = *itr;

		if(item->isList())
		{
			GemHsmsCall::Get()->parseItemList(text, log, (BS2ListItem *)item);
		}
		else
		{
			GemHsmsCall::Get()->parseValue(text, log, item->atom());
		}

		itr++;
	}

	now		= time(0);
	tstruct = *localtime(&now);

	strftime(timebuf, sizeof(timebuf), "%X", &tstruct);

	//for UI
	sprintf(output, "%s: %s %s(%d).\r\n%s.\r\n", timebuf, mode.c_str(), msg->name().c_str(), msg->transNum(), text.c_str());
	refreshMsgList(hDlg, output);

	//for log
	//sprintf(output, "%s %s(%d).\n%s.", mode.c_str(), msg->name().c_str(), msg->transNum(), log.c_str());
	Logger::Get()->write(output);

	addRecordItem(hDlg, msg->name(), msg->name(), mode);
}


/*
**-- Output send and receive
*/
bool GemHsmsCall::parseAtomList(BS2Itemtypes* listStruct, BS2List* listAtom, BS2ListItem* rootlist)
{
	BS2ListItem*	plist; 
	BS2Item*		item_value;
	BS2Atom*		atom;
	BS2ItemType*	itemtype;
	size_t			itemCount = 0;
	unsigned int	atomq;
	bool			retval;
	string			itemName;
	BS2Atoms&		atoms = listAtom->getList();

	atomq = listAtom->getList().size();

	if ((listStruct->size() > 1) && (listStruct->size() != atomq))
    {   // mismatch size of list
        if (atomq == 0)
		{
            return true;          // no item in list
		}
        else
		{
            return false;	
		}
    }

	for (unsigned int i = 0; i < atomq; i++)
	{
		atom		= reinterpret_cast<BS2Atom *>(atoms[i]);
		itemtype	= listStruct->at(itemCount);

		if (!(itemtype->availFormat(atom->format() & ~ATOM_ARRAY)))
		{
			//Logger::Get()->write("Illegal HSMS Format.\n");
			MessageBox(NULL, "Illegal HSMS Format", "HSMS", MB_ICONEXCLAMATION); 
            return false;
		}

		if(atom->isList() && itemtype->isList())
		{
			plist = new BS2ListItem();

			retval = parseAtomList((BS2Itemtypes*)itemtype, (BS2List*)atom, plist);

			if(!retval) //list error
				return false;

			rootlist->add(plist);
		}
		else
		{
			itemName	= itemtype->itemName();
			item_value	= BS2Item::factory(_TX(itemName.c_str()), BS2Atom::factory(atom));
			rootlist->add(item_value);
		}

		itemCount++;

		if (itemCount == listStruct->size())
			itemCount = 0;
	}

	return true;
}

void GemHsmsCall::parseItemList(string& text, string& log, BS2ListItem* listItem)
{
	char	tmp[4096];

	if(listItem->memberq() <= 0)
	{
		string	space(s_nest,' ');
		sprintf(tmp, "%s<L>[0]\r\n%s</L>\r\n", space.c_str(), space.c_str());
		text = text + tmp;

		sprintf(tmp, "%s<L>[0]\n%s</L>\n", space.c_str(), space.c_str());
		log = log + tmp;
		return;
	}

	BS2Item*					item;
	BS2ItemList					items	= listItem->items();
	list<BS2Item*>::iterator	itr		= items.begin(); 
	unsigned int				size	= items.size();

	string	space(s_nest,' ');
	sprintf(tmp, "%s<L>[%d]\r\n", space.c_str(), size);
	text = text + tmp;

	sprintf(tmp, "%s<L>[%d]\n", space.c_str(), size);
	log = log + tmp;

	for (unsigned int i = 0; i < size; i++)
	{
		item = *itr;

		if(item->isList())
		{
			s_nest += 2;
			GemHsmsCall::Get()->parseItemList(text, log, (BS2ListItem *)item);
			s_nest -= 2;
		}
		else
		{
			s_nest += 2;
			GemHsmsCall::Get()->parseValue(text, log, item->atom());
			s_nest -= 2;
		}

		itr++;
	}

	sprintf(tmp, "%s</L>\r\n", space.c_str());
	text = text + tmp;

	sprintf(tmp, "%s</L>\n", space.c_str());
	log = log + tmp;
}

void GemHsmsCall::parseValue(string& text, string& log, BS2Atom* atom)
{
	char	tmp1[4096];
	char	tmp2[4096];
	char    value[64];
	string  content = "";
	string	space(s_nest,' ');

	if(atom->isAscii() && !atom->isArray())
	{
		sprintf(tmp1, "%s<A>%s</A>\r\n", space.c_str(), atom->toString().c_str());
		sprintf(tmp2, "%s<A>%s</A>\n", space.c_str(), atom->toString().c_str());
	}
	else if(atom->isBinary() && !atom->isArray())
	{
		if(atom->size() > 1)
		{
			sprintf(tmp1, "%s<B>%d</B>\r\n", space.c_str(), atom->size());
			sprintf(tmp2, "%s<B>%d</B>\n", space.c_str(), atom->size());
		}
		else
		{
			sprintf(tmp1, "%s<B>0x%02X</B>\r\n", space.c_str(), atom->getInt());
			sprintf(tmp2, "%s<B>0x%02X</B>\n", space.c_str(), atom->getInt());
		}
	}
	else if(atom->isBoolean()/* && !atom->isArray()*/)
	{
		if(atom->getBool())
		{
			sprintf(tmp1, "%s<Boolean>0x01</Boolean>\r\n", space.c_str());
			sprintf(tmp2, "%s<Boolean>0x01</Boolean>\n", space.c_str());
		}
		else
		{
			sprintf(tmp1, "%s<Boolean>0x00</Boolean>\r\n", space.c_str());
			sprintf(tmp2, "%s<Boolean>0x00</Boolean>\n", space.c_str());
		}
	}
	else if(atom->isFloat4())
	{
		if(atom->isFloat4Array())
		{
			sprintf(tmp1, "%s<F4>Not Support</F4>\r\n", space.c_str());
			sprintf(tmp2, "%s<F4>Not Support</F4>\n", space.c_str());
		}
		else
		{
			sprintf(tmp1, "%s<F4>%f</F4>\r\n", space.c_str(), atom->getfloat());
			sprintf(tmp2, "%s<F4>%f</F4>\n", space.c_str(), atom->getfloat());
		}
	}
	else if(atom->isFloat8())
	{
		if(atom->isFloat8Array())
		{
			sprintf(tmp1, "%s<F8>Not Support</F8>\r\n", space.c_str());
			sprintf(tmp2, "%s<F8>Not Support</F8>\n", space.c_str());
		}
		else
		{
			sprintf(tmp1, "%s<F8>%f</F8>\r\n", space.c_str(), atom->getfloat());
			sprintf(tmp2, "%s<F8>%f</F8>\n", space.c_str(), atom->getfloat());
		}
	}
	else if(atom->isInt1())
	{
		if(atom->isInt1Array())
		{
			for(int i = 0; i < atom->size(); i++)
			{
				sprintf(value, "%d", atom->getIntAt(i));
				content.append(value);
				if(i != atom->size() - 1)
					content.append(" ");
			}
		}
		else
		{
			sprintf(value, "%d", atom->getInt());
			content.append(value);
		}

		sprintf(tmp1, "%s<I1>%s</I1>\r\n", space.c_str(), content.c_str());
		sprintf(tmp2, "%s<I1>%s</I1>\n", space.c_str(), content.c_str());
	}
	else if(atom->isInt2())
	{
		if(atom->isInt2Array())
		{
			for(int i = 0; i < atom->size(); i++)
			{
				sprintf(value, "%d", atom->getIntAt(i));
				content.append(value);
				if(i != atom->size() - 1)
					content.append(" ");
			}
		}
		else
		{
			sprintf(value, "%d", atom->getInt());
			content.append(value);
		}

		sprintf(tmp1, "%s<I2>%s</I2>\r\n", space.c_str(), content.c_str());
		sprintf(tmp2, "%s<I2>%s</I2>\n", space.c_str(), content.c_str());
	}
	else if(atom->isInt4())
	{
		if(atom->isInt4Array())
		{
			for(int i = 0; i < atom->size(); i++)
			{
				sprintf(value, "%d", atom->getIntAt(i));
				content.append(value);
				if(i != atom->size() - 1)
					content.append(" ");
			}
		}
		else
		{
			sprintf(value, "%d", atom->getInt());
			content.append(value);
		}

		sprintf(tmp1, "%s<I4>%s</I4>\r\n", space.c_str(), content.c_str());
		sprintf(tmp2, "%s<I4>%s</I4>\n", space.c_str(), content.c_str());
	}
	else if(atom->isInt8())
	{
		if(atom->isInt8Array())
		{
			for(int i = 0; i < atom->size(); i++)
			{
				sprintf(value, "%d", atom->getIntAt(i));
				content.append(value);
				if(i != atom->size() - 1)
					content.append(" ");
			}
		}
		else
		{
			sprintf(value, "%d", atom->getInt());
			content.append(value);
		}

		sprintf(tmp1, "%s<I8>%s</I8>\r\n", space.c_str(), content.c_str());
		sprintf(tmp2, "%s<I8>%s</I8>\n", space.c_str(), content.c_str());
	}
	else if(atom->isUInt1())
	{
		if(atom->isUInt1Array())
		{
			for(int i = 0; i < atom->size(); i++)
			{
				sprintf(value, "%d", atom->getIntAt(i));
				content.append(value);
				if(i != atom->size() - 1)
					content.append(" ");
			}
		}
		else
		{
			sprintf(value, "%d", atom->getInt());
			content.append(value);
		}

		sprintf(tmp1, "%s<U1>%s</U1>\r\n", space.c_str(), content.c_str());
		sprintf(tmp2, "%s<U1>%s</U1>\n", space.c_str(), content.c_str());
	}
	else if(atom->isUInt2())
	{
		if(atom->isUInt2Array())
		{
			for(int i = 0; i < atom->size(); i++)
			{
				sprintf(value, "%d", atom->getIntAt(i));
				content.append(value);
				if(i != atom->size() - 1)
					content.append(" ");
			}
		}
		else
		{
			sprintf(value, "%d", atom->getInt());
			content.append(value);
		}

		sprintf(tmp1, "%s<U2>%s</U2>\r\n", space.c_str(), content.c_str());
		sprintf(tmp2, "%s<U2>%s</U2>\n", space.c_str(), content.c_str());
	}
	else if(atom->isUInt4())
	{
		if(atom->isUInt4Array())
		{
			for(int i = 0; i < atom->size(); i++)
			{
				sprintf(value, "%d", atom->getIntAt(i));
				content.append(value);
				if(i != atom->size() - 1)
					content.append(" ");
			}
		}
		else
		{
			sprintf(value, "%d", atom->getInt());
			content.append(value);
		}

		sprintf(tmp1, "%s<U4>%s</U4>\r\n", space.c_str(), content.c_str());
		sprintf(tmp2, "%s<U4>%s</U4>\n", space.c_str(), content.c_str());
	}
	else if(atom->isUInt8())
	{
		if(atom->isUInt8Array())
		{
			for(int i = 0; i < atom->size(); i++)
			{
				sprintf(value, "%d", atom->getIntAt(i));
				content.append(value);
				if(i != atom->size() - 1)
					content.append(" ");
			}
		}
		else
		{
			sprintf(value, "%d", atom->getInt());
			content.append(value);
		}

		sprintf(tmp1, "%s<U8>%s</U8>\r\n", space.c_str(), content.c_str());
		sprintf(tmp2, "%s<U8>%s</U8>\n", space.c_str(), content.c_str());
	}
	/*else
	{
		sprintf(tmp1, "%s<>not support</>\r\n", space.c_str());
		sprintf(tmp2, "%s<>not support</>\n", space.c_str());
	}*/

	text	= text + tmp1;
	log		= log + tmp2;
}

void GemHsmsCall::EraseAtoms_Recursive(BS2List* list_atom)
{
	unsigned int					i, atomq;
	BS2Atoms						atoms;
	BS2Atom*						atom;

	if(list_atom->size() > 0)
	{
		atoms	= list_atom->getList();
		atomq	= atoms.size();
		
		for(i = 0; i < atomq; i++)
		{
			atom = reinterpret_cast<BS2Atom *>(atoms[i]);

			if(atom->isList())
			{
				BS2List* list_atom = (BS2List*)atom;
				EraseAtoms_Recursive(list_atom);
				delete list_atom;
			}
			else if(atom->isArray())
			{
				//nothing
			}
			else
			{
				delete atom;
			}
		}
	}
}

//Read Binary File
/*int length		= 0;
char* ppbody	= NULL;// = (BYTE*) malloc (sizeof(char)*size);

ifstream readfile("C:\\myfile.bin", ios::binary);
if(readfile)
{
	readfile.seekg (0, ios::end);
	length = readfile.tellg();
	readfile.seekg (0, ios::beg);

	//ppbody = (BYTE*) malloc (sizeof(char)*length);
	ppbody = new char [length];
	readfile.read (ppbody, length);
	readfile.close();
}

msg_body = BS2Message::factory(SFCODE(7, 3));
rootlist = new BS2ListItem();

item_value	= BS2Item::factory(_TX("PPID"), new BS2Ascii("WBF2497"));
rootlist->add(item_value);

item_value	= BS2Item::factory(_TX("PPBODY"), new BS2Binary((BYTE*)ppbody, length));
rootlist->add(item_value);

msg_body->add(rootlist);
free(ppbody);*/


//Write Binary File
/*BYTE*	ppbody;
string	ppid;

atom = revMsg->getAtom("PPID");
ppid = atom->toString();

atom = revMsg->getAtom("PPBODY");
int length = atom->size();

ppbody = (BYTE *)malloc(length);
atom->getStreamData(ppbody);

string filename = "C:\\" + ppid;
std::ofstream file(filename.c_str(), std::ios::binary);
file.write((char*)ppbody, length);
file.close();

free (ppbody);
*/