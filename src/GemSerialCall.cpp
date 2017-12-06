
#include "GemSDK.h"
#include "logger.h"

static GemSerialCall* s_GemSerialCall_Instance = NULL;

static int s_nest;

extern int	ParseStreamFunction(string& sfstr);
extern void refreshMsgList(HWND hDlg, TCHAR *newText);
extern void addRecordItem(HWND hDlg, string sxfy, string name, string mode);

GemSerialCall* GemSerialCall::Get()
{
	if (!s_GemSerialCall_Instance)
		s_GemSerialCall_Instance = new GemSerialCall();
	
	return s_GemSerialCall_Instance;
}

void GemSerialCall::Free()
{
	delete s_GemSerialCall_Instance;
	s_GemSerialCall_Instance = NULL;
}

GemSerialCall::GemSerialCall()
{
	m_gemsecs_device = NULL;
}

GemSerialCall::~GemSerialCall()
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

int GemSerialCall::Open(int port, int deviceId, void* tag)
{
	m_tag = tag;

	int result;
	DeviceParameter*	config;
	CommParameter*	parm;

	ACE::init();
		
	config				= new CommParameter();
	m_gemsecs_device	= new GemDevice(this);
	m_gemsecs_sender	= new GemSender(this, m_gemsecs_device);
	m_gemsecs_receiver	= new GemReceiver(this, m_gemsecs_device);

	parm = (CommParameter*)config;
	parm->m_dtype		= DRIVER_SERIAL;
	parm->m_baudrate	= 9600;
	parm->m_databit		= 8;
	parm->m_stopbit		= 1;
	parm->m_slave		= 1; // 0=equipment, 1=slave (host)
	parm->m_deviceId	= deviceId;
	sprintf(parm->m_port, "\\\\.\\COM%d", port);

	sprintf(parm->m_xmlname, "./xml/secs.xml");

	result = m_gemsecs_device->initialize(parm, m_gemsecs_sender, m_gemsecs_receiver);
	
	if(result == BEE_SUCCESS)
		m_gemsecs_device->open();

	delete config;

	return result;
}

void GemSerialCall::Close()
{
	if(m_gemsecs_device)
	{
		delete m_gemsecs_device;
		m_gemsecs_device = NULL;

		ACE::fini();
	}
}

/*
**-- SECS-I send function
*/
void GemSerialCall::SECS_SXFX(string sxfy, string name, UINT transactionNum)
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
			//Logger::Get()->write(name + " Illegal SECS-I Format.\n");
			MessageBox(NULL, "Illegal SECS-I Format", "SECS-I", MB_ICONEXCLAMATION); 
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

	if(msg_body->isSecondary())
		msg_body->transNum(transactionNum);

	if(m_gemsecs_sender)
	{
		t_id = m_gemsecs_sender->send(msg_body);

		if(msg_body->isPrimary())
		{
			transactionNum = MAKE_TRANSACTION_IDENTIFIER(1, t_id);
			msg_body->transNum(transactionNum);
		}

		outputMessage(msg_body, "Send");
	}

	delete msg_body;
}

void GemSerialCall::SECS_Send(BS2Message* msg)
{
	if(m_gemsecs_sender)
		m_gemsecs_sender->send(msg);
}


/*
**-- make sxfy atoms(values)
*/
int GemSerialCall::makeAtoms(string name, TiXmlElement* node)
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

bool GemSerialCall::getSxFyAtoms(string name, BS2Atoms& atom)
{
	map<string, BS2Atoms>::iterator itr;

	itr = m_sxfyAtomsMap.find(name);

	if(itr == m_sxfyAtomsMap.end())
		return false;

	atom = itr->second;
	
	return true;
}

int GemSerialCall::listElements(TiXmlElement* node, BS2List* listParent)
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

BS2Atom* GemSerialCall::valueElement(TiXmlElement* node)
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
**-- SECS-I response callback
*/
int GemSerialCall::SECS_Response(BS2MessageInfo* binfo)
{
	BS2Message*		revMsg		= binfo->getMessage();
	BS2Message*		msg_body;
	BS2ListItem*	rootlist;
	BS2ListItem*	paramlist;
	BS2Item*		item_value;
	char			sxfy[32]	= {0}; 
	BS2Atoms		atoms;
	UINT			snum;
	UINT			fnum;

	outputMessage(revMsg, "Receive");

	snum = STREAMCODE(revMsg->sf());
	fnum = FUNCCODE(revMsg->sf());

	if(revMsg->isPrimary() && (snum != 9 && fnum != 7))
	{
		//response
		sprintf(sxfy, "S%dF%d", snum, fnum+1);
		if(GemSerialCall::Get()->getSxFyAtoms(sxfy, atoms))
		{
			GemSerialCall::Get()->SECS_SXFX(sxfy, sxfy, revMsg->transNum());
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
			}
			else if(snum == 1 && fnum == 1)
			{
				msg_body = BS2Message::factory(SFCODE(1, 2));
				rootlist = new BS2ListItem();
				msg_body->add(rootlist);
			}
			else
			{
				msg_body = BS2Message::response(revMsg);
			}

			GemSerialCall::Get()->SECS_Send(msg_body);
			GemSerialCall::Get()->outputMessage(msg_body, "Send");
			delete msg_body;
		}

		return 1;
	}
	else if(revMsg->isSecondary())
	{
		if(snum == 1 && fnum == 12)
		{
			vector<b_value>	retval;
			vector<b_value>	retval2;
			vector<b_value>	retval3;

			revMsg->getValue("SVID", retval);
			revMsg->getValue("SVNAME", retval2);
			revMsg->getValue("UNITS", retval3);

			Logger::Get()->write("S1F12:");
			Logger::Get()->write("<----------SVID---------->");
			for(int i = 0; i < retval.size(); i++) 
			{
				string name = retval2[i].toString();
				string svid = retval[i].toString();
				string uint = retval3[i].toString();

				string value = "<" + svid + " " + name + " " + uint + ">";

				Logger::Get()->write(value);
			}
		}
		else if(snum == 7 && fnum == 6)
		{
			/*atom = revMsg->getAtom("PPID");
			m_ppid = atom->toString();

			atom = revMsg->getAtom("PPBODY");
			atom->getStreamData(m_ppbody);

			Logger::Get()->write(atom->toString());

			vector<string> strings;
			istringstream f(atom->toString());
			string s;
			while (getline(f, s, ' ')) 
			{
				cout << s << endl;
				strings.push_back(s);
			}*/
		}
	}

	return 0;
}

void GemSerialCall::outputMessage(BS2Message* msg, string mode)
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
			GemSerialCall::Get()->parseItemList(text, log, (BS2ListItem *)item);
		}
		else
		{
			GemSerialCall::Get()->parseValue(text, log, item->atom());
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
	sprintf(output, "%s %s(%d).\n%s.", mode.c_str(), msg->name().c_str(), msg->transNum(), log.c_str());
	Logger::Get()->write(output);

	addRecordItem(hDlg, msg->name(), msg->name(), mode);
}


/*
**-- Output send and receive
*/
bool GemSerialCall::parseAtomList(BS2Itemtypes* listStruct, BS2List* listAtom, BS2ListItem* rootlist)
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
			//Logger::Get()->write("Illegal SECS-I Format.\n");
			MessageBox(NULL, "Illegal SECS-I Format", "SECS-I", MB_ICONEXCLAMATION); 
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

void GemSerialCall::parseItemList(string& text, string& log, BS2ListItem* listItem)
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
			GemSerialCall::Get()->parseItemList(text, log, (BS2ListItem *)item);
			s_nest -= 2;
		}
		else
		{
			s_nest += 2;
			GemSerialCall::Get()->parseValue(text, log, item->atom());
			s_nest -= 2;
		}

		itr++;
	}

	sprintf(tmp, "%s</L>\r\n", space.c_str());
	text = text + tmp;

	sprintf(tmp, "%s</L>\n", space.c_str());
	log = log + tmp;
}

void GemSerialCall::parseValue(string& text, string& log, BS2Atom* atom)
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
		sprintf(tmp1, "%s<B>0x%02X</B>\r\n", space.c_str(), atom->getInt());
		sprintf(tmp2, "%s<B>0x%02X</B>\n", space.c_str(), atom->getInt());
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

void GemSerialCall::EraseAtoms_Recursive(BS2List* list_atom)
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