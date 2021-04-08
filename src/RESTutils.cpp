//
//  RESTutils.cpp
//  InsteonServer
//
//  Created by Vincent Moscaritolo on 4/7/21.
//

#include "RESTutils.hpp"

#include <sstream>

// MARK: - RESTserver utility function
using namespace std;

namespace rest{

string IPAddrString(sockaddr_storage addr){
	std::ostringstream oss;
	
	//Print incoming connection
	if (addr.ss_family == AF_INET){				//IPv4
		
		struct sockaddr_in * v4Addr = (sockaddr_in*) &addr;
		char ip_as_string[INET_ADDRSTRLEN];
		inet_ntop(addr.ss_family,&((struct sockaddr_in *)&addr)->sin_addr,ip_as_string, INET_ADDRSTRLEN);
		oss << "IPv4: " << ip_as_string << ":" << to_string(v4Addr->sin_port);
		
	} else if(addr.ss_family == AF_INET6){ 		//IPv6
		
		struct sockaddr_in6 * v6Addr = (sockaddr_in6*) &addr;
		char ip_as_string[INET6_ADDRSTRLEN];
		inet_ntop(addr.ss_family,&((struct sockaddr_in6 *)&addr)->sin6_addr,ip_as_string, INET6_ADDRSTRLEN);
		oss << "IPv6: " << ip_as_string  << ":" << to_string(v6Addr->sin6_port);
	}

	return oss.str();
}

	void makeStatusJSON(json &j,
							  httpStatusCodes_t code,
							  string message ,
							  string detail ,
							  string cmdLine,
							  string help) {
		
		bool sucess = (code < 400 );
		
		j[kREST_success] = sucess;
		
		if(!sucess){
			j[kREST_error] [kREST_errorCode] = code;
			
			if(!message.empty()){
				j[kREST_error] [kREST_errorMessage] = message;
			}
			if(!detail.empty()){
				j[kREST_error] [kREST_errorDetail] = detail;
			}
			if(!help.empty()){
				j[kREST_error] [kREST_errorHelp] = help;
			}
			if(!cmdLine.empty()){
				j[kREST_error] [kREST_errorCmdLine] = cmdLine;
			}
		}
	}


	bool didSucceed(json j){
		bool success = false;
		
		if(j.contains(kREST_success)
			&& j.at(kREST_success).is_boolean()){
			success = j.at(kREST_success);
		}
		
		return success;
	}

	string errorMessage( json j){
		string msg;
		
		if(j.contains(kREST_error)
			&& j.at(kREST_error).is_structured()){
			auto jErr = j.at(kREST_error);
			
			if(jErr.contains(kREST_errorCmdLine)
				&& jErr.at(kREST_errorCmdLine).is_string()){
				string message =  jErr.at(kREST_errorCmdLine);
				msg += message;
			}
			else if(jErr.contains(kREST_errorDetail)
					  && jErr.at(kREST_errorDetail).is_string()){
				string message =  jErr.at(kREST_errorMessage);
				msg += message;
			}
			else if(jErr.contains(kREST_errorMessage)
					  && jErr.at(kREST_errorMessage).is_string()){
				string message =  jErr.at(kREST_errorMessage);
				msg += message;
			}
			else {
				msg += "Unknown error";
			}
		}
		
		return msg;
	}
 }
