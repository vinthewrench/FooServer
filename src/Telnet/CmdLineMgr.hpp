//
//  CmdLineMgr.hpp
//  InsteonServer
//
//  Created by Vincent Moscaritolo on 3/16/21.
//

#ifndef CmdLineMgr_hpp
#define CmdLineMgr_hpp

#include <netinet/in.h>

#include <stdio.h>
#include <string>
#include <functional>
#include <vector>
#include <map>
 
#include "CommonDefs.hpp"
#include "TCPServer.hpp"
#include "CmdLineBuffer.hpp"

class CmdLineMgr:  public CmdLineBufferManager {

public:
	CmdLineMgr(TCPServerConnection* server);
	~CmdLineMgr();

	// calls from TelnetServerConnection
	void didConnect();
	void didDisconnect();
	void processChars(uint8_t* data, size_t len);
	void setTermSize(uint16_t	 width, uint16_t height);
	void setTermType(string name);

	void clear();

	// calls from CmdLineBufferManager
	virtual bool processCommandLine(std::string cmdLine, boolCallback_t cb);
	virtual stringvector matchesForCmd(const std::string cmd);
	
	//convenience wrappers to TCPServerConnection
	void sendReply(std::string reply);

private:

	void registerBuiltInCommands();
	// built in commands
	void doWhatIsMyIP(stringvector params);
	void doFinger(stringvector params);

	TCPServerConnection*	_server;
	CmdLineBuffer 			_cmdLineBuffer;
};

#endif /* CmdLineMgr_hpp */
