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
#include "TCPClientInfo.hpp"
#include "REST_URL.hpp"


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
	
	void queueRESTCommand( REST_URL url,
								TCPClientInfo cInfo,
								cmdCallback_t completion );

	typedef std::function<void(ServerCmdQueue* ref,
										REST_URL url,
										TCPClientInfo cInfo,
										cmdCallback_t completion)> nounHandler_t;
	bool registerNoun(	string_view noun,
							nounHandler_t handler = NULL );


private:
	
	map<string_view,  	nounHandler_t> _nounHandlers;
	nounHandler_t 		handlerForNoun(string noun);

};

#endif /* ServerCmdQueue_hpp */
