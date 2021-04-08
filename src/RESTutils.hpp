//
//  RESTutils.hpp
//  InsteonServer
//
//  Created by Vincent Moscaritolo on 4/7/21.
//

#ifndef RESTutils_hpp
#define RESTutils_hpp

#include <stdio.h>
#include "json.hpp"
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace nlohmann;
using namespace std;

typedef enum {
  STATUS_OK 					= 200,
  STATUS_NO_CONTENT			= 204,
  STATUS_BAD_REQUEST		= 400,
  STATUS_ACCESS_DENIED		= 401,
  STATUS_INVALID_BODY		= 4006,
  STATUS_INVALID_METHOD	= 405,
  STATUS_INTERNAL_ERROR	= 500,
  STATUS_NOT_IMPLEMENTED	= 501,
}httpStatusCodes_t;

namespace rest{
 
  constexpr const char* kREST_command	  			= "cmd" ;

  constexpr const char* kREST_success		  		= "success" ;

  constexpr const char* kREST_error 			 	= "error" ;
  constexpr const char* kREST_errorCode 		 	= "code" ;
  constexpr const char* kREST_errorMessage   	= "message" ;
  constexpr const char* kREST_errorDetail  		= "detail" ;
  constexpr const char* kREST_errorCmdLine  	= "cmdDetail" ;
  constexpr const char* kREST_errorHelp  		= "help" ;

  void makeStatusJSON(json &j,
							 httpStatusCodes_t code,
							 string message = "",
							 string detail = "",
							 string cmdLine = "",
							 string help = "");

  string errorMessage(json reply);
  bool 	didSucceed(json reply);

	string IPAddrString(sockaddr_storage addr);


};

#endif /* RESTutils_hpp */
