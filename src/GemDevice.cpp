
#include "GemSDK.h"

GemDevice::GemDevice(void* parent_class)
{
	m_parent_class	= parent_class;
}

GemDevice::~GemDevice()
{
	this->close(-1);
}

int GemDevice::open(void *)
{
	int ret;
	
	ret = BS2Device::open();

	return ret;
}

int GemDevice::close(int exit_status)
{
	int ret;
	
	ret = BS2Device::close(exit_status);

	return ret;
}

int GemDevice::parse(void* data, int size)
{
	int						result;
	BS2BlockHeader			header;
	BS2Message*				msg_body;
	BS2TransactionInfo*		trinfo;
	BS2TransactionManager*	trmgr		= getTransactionManager();
	BS2MessageInfo*			binfo_get	= NULL;

	if(this->isSocket())
	{
		result = m_receiver->getEventInfo(binfo_get);

		if( result == BEE_SUCCESS )
		{
			msg_body = binfo_get->getMessage();

			if( msg_body )
			{
				if(GemHsmsCall::Get()->Hsms_Response(binfo_get))
				{
					//do some thing
				}
				else
				{
					msg_body->get(&header);

					trinfo	= trmgr->buffer(&header, TRANSACTION_RECV_PRIMARY);
					if( trinfo )
						trmgr->remove(trinfo);
				}
			}
		}
		else
		{
	#if defined(_WIN32)
			::Sleep(10);
	#else
			usleep(10000);
	#endif
		}
	}
	else
	{
		if(this->getDriver()->isOpened())
		{
			result = m_receiver->getEventInfo(binfo_get);

			if( result == BEE_SUCCESS )
			{
				msg_body = binfo_get->getMessage();

				if( msg_body )
				{
					if(GemSerialCall::Get()->SECS_Response(binfo_get))
					{
						//do some thing
					}
					else
					{
						msg_body->get(&header);

						trinfo	= trmgr->buffer(&header, TRANSACTION_RECV_PRIMARY);
						if( trinfo )
							trmgr->remove(trinfo);
					}
				}
			}
		}
	}
	
	result = BS2Device::parse(data, size); //nothing to do

	return result;
}

int GemDevice::put(ACE_Message_Block* mb, ACE_Time_Value* tv)
{
	int ret;
	
	ret = BS2Device::put(mb, tv);

	return ret;
}

int GemDevice::connected()
{
	int ret;
	
	ret = BS2Device::connected();

	return ret;
}

int GemDevice::disconnected()
{
	int ret;
	
	ret = BS2Device::disconnected();

	return ret;
}

int GemDevice::svc(void)
{
	int ret;
	
	ret = BS2Device::svc();

	return ret;
}