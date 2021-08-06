//
//  TelnetServerConnection.cpp
//  InsteonServer
//
//  Created by Vincent Moscaritolo on 3/13/21.
//

#include "TelnetServerConnection.hpp"
#include <iostream>

#include <arpa/inet.h>
#include <netinet/in.h>

#include "LogMgr.hpp"

using namespace std;

struct TelnetServerConnection::Private
{
	
	typedef struct __attribute__((__packed__)){
		short	 width;
		short	 height;
	}  termSize_t ;
	
	
	static void telnet_event_handler(telnet_t *telnet, telnet_event_t *ev, void *arg) {
		
		(void)telnet;
		if(!arg) return;
		
		TelnetServerConnection * ts = (TelnetServerConnection *) arg;
	 
		switch (ev->type) {
				
			case TELNET_EV_TTYPE:
				if(ev->ttype.cmd == TELNET_TTYPE_IS){
					string termType = string(ev->ttype.name);
 					transform(termType.begin(), termType.end(), termType.begin(), ::tolower);

					if (termType.find("color") != std::string::npos) {
						ts->_info.setHasColor(true);
					}
					ts->_cmdLineMgr.setTermType(termType);
				}
				break;
				
			case TELNET_EV_DATA:
				ts->_cmdLineMgr.processChars((uint8_t*)ev->data.buffer, ev->data.size);
				break;
				
			case TELNET_EV_SEND:
				ts->sendData(ev->data.buffer, ev->data.size);
				break;
				
			case TELNET_EV_WARNING:
//				printf( "WARNING: %s\n", ev->error.msg);
				break;
				
			case TELNET_EV_ERROR:
//				printf(  "ERROR: %s\n", ev->error.msg);
				break;
				
			case TELNET_EV_SUBNEGOTIATION:
				if(ev->sub.telopt == TELNET_TELOPT_NAWS){
					termSize_t termSize;
					
					if(ev->sub.size ==  sizeof(termSize_t)){
						uint8_t* p = (uint8_t*)&termSize;
						p[0]  = ev->sub.buffer[1];
						p[1]  = ev->sub.buffer[0];
						p[2]  = ev->sub.buffer[3];
						p[3]  = ev->sub.buffer[2];
						
						ts->_cmdLineMgr.setTermSize(termSize.width, termSize.height);
					}
				}
				break;
				
			default:
				break;
		}
		
		if(ts->_closing)
			ts->close();
	}
	
};

TelnetServerConnection::TelnetServerConnection()
			:TCPServerConnection(TCPClientInfo::CLIENT_TERMINAL, "telnet"),_cmdLineMgr(this){
	
	_closing = false;
 
	static const telnet_telopt_t RP_telopts[] = {
		
		// 		{TELNET_TELOPT_LINEMODE,		TELNET_WILL, TELNET_DO },
		// 		{ TELNET_TELOPT_ECHO,      TELNET_WILL, TELNET_DO },
		// 		{ TELNET_FLAG_NVT_EOL,     	TELNET_WILL, TELNET_DO },
//		{ TELNET_TELOPT_TTYPE,     TELNET_WONT, TELNET_DONT },
		{ TELNET_TELOPT_COMPRESS2, TELNET_WONT, TELNET_DONT   },
		{ TELNET_TELOPT_ZMP,       TELNET_WONT, TELNET_DONT   },
		{ TELNET_TELOPT_MSSP,      TELNET_WONT, TELNET_DONT   },
		{ TELNET_TELOPT_BINARY,    TELNET_WONT, TELNET_DONT   },
		{ TELNET_TELOPT_NAWS,      TELNET_WILL, TELNET_DO },
 		{ TELNET_TELOPT_TTYPE,      TELNET_WONT, TELNET_DO },
		{ -1, 0, 0 }
	};
	
	_tn =  telnet_init(RP_telopts,
							 Private::telnet_event_handler,  0, this);
};


TelnetServerConnection::~TelnetServerConnection() {
	
 	_cmdLineMgr.didDisconnect();
 
	if(_tn) {
		telnet_free(_tn);
		_tn = NULL;
	}
}


void TelnetServerConnection::didRecvData(const void *buffer, size_t length){
	telnet_recv(_tn, (const char *) buffer,length);
}

void TelnetServerConnection::willClose() {
	cout<<"TelnetServerConnection Closed:" << to_string(_id) << endl;
	
	
	if(_tn) {
		telnet_free(_tn);
		_tn = NULL;
	}
};

void TelnetServerConnection::didOpen() {
	
	telnet_negotiate(_tn, TELNET_WILL, TELNET_FLAG_NVT_EOL);
	telnet_negotiate(_tn, TELNET_DO, 	TELNET_TELOPT_LINEMODE);
	telnet_negotiate(_tn, TELNET_WILL, TELNET_TELOPT_ECHO);
	telnet_negotiate(_tn, TELNET_DO, 	TELNET_TELOPT_NAWS);
 	telnet_negotiate(_tn, TELNET_DO, 	TELNET_TELOPT_TTYPE);
 
 	telnet_ttype_send(_tn);
	
	_cmdLineMgr.didConnect();
	
	string remoteStr = _info.remoteAddrString();
	LOGT_INFO("TELNET OPEN from %s\n", remoteStr.c_str());
};


// MARK: - TCPServerConnection functions

void TelnetServerConnection::sendString(const string str){
	telnet_send(_tn, (const char *) str.c_str(), str.size());
}

void TelnetServerConnection::closeConnection(){
	_closing = true;
	
	string remoteStr = _info.remoteAddrString();
	LOGT_INFO("TELNET CLOSE from %s\n", remoteStr.c_str());

}

bool TelnetServerConnection::isConnected(){
	return _closing == false;
}
