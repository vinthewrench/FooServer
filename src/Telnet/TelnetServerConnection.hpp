//
//  TelnetServerConnection.hpp
//  InsteonServer
//
//  Created by Vincent Moscaritolo on 3/13/21.
//

#ifndef TelnetServerConnection_hpp
#define TelnetServerConnection_hpp

#include "libtelnet.h"

#include "TCPServer.hpp"
#include "CmdLineMgr.hpp"

class TelnetServerConnection : public TCPServerConnection {
	
public:
 
	TelnetServerConnection();  
	~TelnetServerConnection();
 
	virtual void didOpen();
	virtual void willClose();
	virtual void didRecvData(const void *buffer, size_t length);
 
	// TCPServerConnection override
	virtual void sendString(const std::string);
	virtual void closeConnection();
	virtual bool isConnected();
 
	private:
		telnet_t*	 		_tn;
		CmdLineMgr 		_cmdLineMgr;
 		bool 				_closing;

	struct Private;

};

#endif /* TelnetServerConnection_hpp */
