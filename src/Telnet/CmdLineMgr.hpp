//
//  CmdLineMgr.hpp

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
	virtual void helpForCommandLine(std::string cmdLine, boolCallback_t cb);

	
	//convenience wrappers to TCPServerConnection
	void sendReply(std::string reply);

	// used to verify
	void waitForChar( std::vector<char> chs,
						  std::function<void(bool didSucceed, char ch)> callback = NULL);;

	TCPClientInfo getClientInfo() { return _server->_info; };
	
private:

	void registerBuiltInCommands();
	// built in commands
	void doWhatIsMyIP(stringvector params);
	void doFinger(stringvector params);
	void doHelp(stringvector params);
	void doWelcome();
	
	TCPServerConnection*	_server;
	CmdLineBuffer 			_cmdLineBuffer;
};

#endif /* CmdLineMgr_hpp */
