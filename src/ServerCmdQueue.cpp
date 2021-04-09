//
//  ServerCmdQueue.cpp
//  InsteonServer
//
//  Created by Vincent Moscaritolo on 3/19/21.
//

#include <iostream>
#include <iomanip>
#include <regex>
#include <iterator>

#include "json.hpp"
#include "Utils.hpp"

#include "ServerCmdQueue.hpp"
#include "TCPClientInfo.hpp"

using namespace nlohmann;
using namespace std;
 

// MARK: - ServerCmdQueue

ServerCmdQueue *ServerCmdQueue::sharedInstance = 0;

ServerCmdQueue::ServerCmdQueue() {
	_cmdHandlers.clear();
};

ServerCmdQueue::~ServerCmdQueue() {
	_cmdHandlers.clear();
};

bool ServerCmdQueue::registerCommand(	string_view cmd,
												 cmdHandler_t handler ) {
	if(handler)
		_cmdHandlers.insert( {cmd, handler} );

	return true;
}

ServerCmdQueue::cmdHandler_t ServerCmdQueue::handlerForCommand(string cmd){
	
	cmdHandler_t  handler = NULL;
	  
	auto it = _cmdHandlers.find(cmd);
	if(it != _cmdHandlers.end()){
			handler = it->second;
	}
	
	return handler;
}



// MARK: - command management and processing

void ServerCmdQueue::queueCommand(	json request,
											 TCPClientInfo cInfo,
											 cmdCallback_t completion ) {
	
	using namespace rest;

 
	
	if(request.contains(kREST_command)
		&& request.at(kREST_command).is_string()) {
		string cmd = request.at(kREST_command);
		
		cmdHandler_t func = handlerForCommand(cmd);
		if(func) {
	 		(func)(this, request, cInfo, completion);
		}
		else {
			json reply;
			makeStatusJSON(reply,
								STATUS_NOT_IMPLEMENTED,
								"Not Implemented",
								"Lazy Programmer",
								"Lazy programmer hasn't written code for this yet." );

			(completion) (reply, STATUS_NOT_IMPLEMENTED);

		}
	}
	else {	// bad command
		json reply ;
		makeStatusJSON(reply,
							STATUS_BAD_REQUEST,
							"No Command is specified"
							);
		(completion) (reply, STATUS_BAD_REQUEST);
	}
}
 
