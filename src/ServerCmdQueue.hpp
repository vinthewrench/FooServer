//
//  ServerCmdQueue.hpp
//  InsteonServer
//
//  Created by Vincent Moscaritolo on 3/19/21.
//

#ifndef ServerCmdQueue_hpp
#define ServerCmdQueue_hpp

#include <stdio.h>
#include <stdint.h>
#include <string>
#include <functional>
#include <vector>
#include <list>
#include <map>
#include <tuple>
#include <any>

#include <string_view>
 
#include "CommonDefs.hpp"
#include "RESTutils.hpp"


using namespace nlohmann;
using namespace std;

class ServerCmdQueue {

public:
	
	static ServerCmdQueue *shared() {
			if (!sharedInstance)
				sharedInstance = new ServerCmdQueue;
			return sharedInstance;
		}

	static ServerCmdQueue *sharedInstance;
	
	ServerCmdQueue();
	~ServerCmdQueue();
	
	typedef std::function<void(	json reply,
										httpStatusCodes_t code)> cmdCallback_t;
	
	void queueCommand(	json command,
							cmdCallback_t completion );

	
	typedef std::function<void(ServerCmdQueue* ref,
 										json request,
										cmdCallback_t completion)> cmdHandler_t;

	bool registerCommand(	string_view cmd,
								cmdHandler_t handler = NULL );
private:
	
	map<string_view,  	cmdHandler_t> _cmdHandlers;
	cmdHandler_t 		handlerForCommand(string cmd);
};

#endif /* ServerCmdQueue_hpp */
