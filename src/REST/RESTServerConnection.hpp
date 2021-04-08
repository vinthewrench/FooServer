//
//  RESTServerConnection.hpp
//  InsteonServer
//
//  Created by Vincent Moscaritolo on 3/13/21.
//

#ifndef RESTServerConnection_hpp
#define RESTServerConnection_hpp

#include <map>
#include <string>

#include "TCPServer.hpp"
#include "http_parser.h"

class RESTServerConnection : public TCPServerConnection {
	
public:

	RESTServerConnection();
	~RESTServerConnection();

	virtual void didOpen();
	
	virtual void willClose();
	virtual void didRecvData(const void *buffer, size_t length);

	// TCPServerConnection passthrough
	
	virtual void sendString(const std::string);
	virtual void closeConnection() ;
	virtual bool isConnected();
 
private:
	std::string httpHeaderForStatusCode(httpStatusCodes_t code);
	std::string httpHeaderForContentLength(size_t length);

	bool validateRequestCredentials(json k);

	void processHeaders();

	http_parser 				_parser;
	http_parser_settings 	_hpCB;

	char* 					_url;		// must dealloc this
	char* 					_body;		// must dealloc this
	char*					_currentHeader;
	
	map<string, string> _headers;
	
	bool					_isOpen;
	struct Private;
};

#endif /* RESTServerConnection_hpp */
