//
//  CmdLineRegistry.hpp
//  InsteonServer
//
//  Created by Vincent Moscaritolo on 4/6/21.
//

#ifndef CmdLineRegistry_hpp
#define CmdLineRegistry_hpp
 
#include <stdio.h>
#include <string>
#include <functional>
#include <vector>
#include <map>
 
#include "CommonDefs.hpp"
#include "CmdLineMgr.hpp"
#include "ServerCmdQueue.hpp"


class CmdLineRegistry {
 
public:
	
	constexpr static string_view CMD_WELCOME 		= "welcome";

	static CmdLineRegistry *shared() {
			if (!sharedInstance)
				sharedInstance = new CmdLineRegistry;
			return sharedInstance;
		}

	static CmdLineRegistry *sharedInstance;

	CmdLineRegistry();
	~CmdLineRegistry();
	
 	typedef std::function< bool (stringvector commandLine,
										  CmdLineMgr* mgr,
										  boolCallback_t	cb)> cmdHandler_t;
// 
//	void registerCommand(const string name,
//								cmdHandler_t cb = NULL);

	void registerCommand(string_view name,
								cmdHandler_t cb = NULL);

	void removeCommand(const string name);
	
	vector<string> registeredCommands();
	
	vector<string> matchesForCmd( const string cmd);

	cmdHandler_t handlerForCmd( const string cmd);

	string helpForCmd( stringvector params);
	
protected:

	map<const string, cmdHandler_t> _commandMap;

};

#endif /* CmdLineRegistry_hpp */
