//
//  RESTServerConnection.cpp
//  InsteonServer
//
//  Created by Vincent Moscaritolo on 3/13/21.
//

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>

#include <arpa/inet.h>
#include <netinet/in.h>
#include "yuarel.h"
#include "json.hpp"

#include "RESTServerConnection.hpp"
 #include "TCPClientInfo.hpp"

using namespace nlohmann;
using namespace std;
using namespace rest;

[[clang::no_destroy]] const string HTTPHEADER_JSON  		= "Content-Type: application/json\r\n";
[[clang::no_destroy]] const string HTTPHEADER_KEEPALIVE  = "Connection: keep-alive\r\n";
[[clang::no_destroy]] const string HTTPHEADER_CLOSE  		= "Connection: close\r\n";
[[clang::no_destroy]] const string HTTPHEADER_CRLF  		= "\r\n";


// MARK: - http_parser_settings callbacks

struct RESTServerConnection::Private
{
	
	static int on_message_begin(http_parser* _parser) {
		
		RESTServerConnection * rs = 	(RESTServerConnection*)_parser->data;
		
		if(rs->_url){
			free(rs->_url);
			rs->_url = NULL;
		}
		
		if(rs->_body){
			free(rs->_body);
			rs->_body = NULL;
		}
		
		if(rs->_currentHeader){
			free(rs->_currentHeader);
			rs->_currentHeader = NULL;
		}
		
		rs->_headers.clear();
		
		//	printf("\n***MESSAGE BEGIN***");
		return 0;
	}
	
	static int on_message_complete(http_parser* _parser) {
		RESTServerConnection * rs = 	(RESTServerConnection*)_parser->data;
		
		struct yuarel url;
		
		string errorReply;
 
		rs->processHeaders();
	
		// update the headers
		rs->_info.setHeaders(rs->_headers);
		
		yuarel_parse(&url, rs->_url);
		
		//	printf("%u %s",_parser->method, url.path);
		
		switch  (_parser->method) {
				
			case HTTP_GET:
				errorReply = rs->httpHeaderForStatusCode(STATUS_INVALID_METHOD);
				break;
				
			case HTTP_PUT:
				if(strcmp(url.path, "command") == 0){
					if(!rs->_body) {
						errorReply = rs->httpHeaderForStatusCode(STATUS_INVALID_BODY);
						break;
					}
					string str =  string(rs->_body);
					auto j = json::parse(str);
					
					if(rs->validateRequestCredentials(j)) {
						rs->queueServerCommand(j, [=] (json rp, httpStatusCodes_t code){
							
							string body;
							if(rp.size())
								body = rp.dump();
							
							string header = rs->httpHeaderForStatusCode(code);
							header+= HTTPHEADER_JSON;
							header += HTTPHEADER_CLOSE;
							
							header+= rs->httpHeaderForContentLength(body.size());
							header += HTTPHEADER_CRLF;
							string reply = header + body;
							
							rs->sendString(reply);
						});
						
					}
					else {
						errorReply = rs->httpHeaderForStatusCode(STATUS_ACCESS_DENIED);
						
					}
					
				}
				else {
					errorReply = rs->httpHeaderForStatusCode(STATUS_INVALID_BODY);
				}
				
				break;
				
			default:
				errorReply = rs->httpHeaderForStatusCode(STATUS_INVALID_METHOD);
				break;
		}
		
		if(errorReply.size())
		{
			errorReply += rs->httpHeaderForContentLength(0) + HTTPHEADER_CRLF;
			rs->sendString(errorReply);
		}
		
		if(rs->_url){
			free(rs->_url);
			rs->_url = NULL;
		}
		
		if(rs->_body){
			free(rs->_body);
			rs->_body = NULL;
		}
		
		if(rs->_currentHeader){
			free(rs->_currentHeader);
			rs->_currentHeader = NULL;
		}
		
		rs->_headers.clear();
		
		return 0;
	}
	
	static int on_url(http_parser* _parser, const char* at, size_t length) {
		RESTServerConnection * rs = 	(RESTServerConnection*)_parser->data;
		
		rs->_url = strndup(at,length);
		
		//		Serial_Printf(&Serial,"Url: %.*s", (int)length, at);
		return 0;
	}
	
	static int on_body(http_parser* _parser, const char* at, size_t length) {
		RESTServerConnection * rs = 	(RESTServerConnection*)_parser->data;
		
		if(!rs->_body) {
			rs->_body = strndup(at,length);
		} else {
			rs->_body = (char*) realloc(rs->_body, strlen(rs->_body) + length + 1);
			rs->_body = strncat(rs->_body,at,length);
		}
		return 0;
	}
	
	/* Not used */
	static int on_headers_complete(http_parser* _parser) {
		RESTServerConnection * rs = 	(RESTServerConnection*)_parser->data;
		
		if(rs->_currentHeader){
			free(rs->_currentHeader);
			rs->_currentHeader = NULL;
		}
		
//		for (auto& t : rs->_headers){
//			printf("%s = %s\n", t.first.c_str(), t.second.c_str());
//		}

		return 0;
	}
	
	static int on_header_field(http_parser* _parser, const char* at, size_t length) {
		RESTServerConnection * rs = 	(RESTServerConnection*)_parser->data;
		
		if(rs->_currentHeader){
			free(rs->_currentHeader);
			rs->_currentHeader = NULL;
		}
		rs->_currentHeader = strndup(at,length);
		
		return 0;
	}
	
	static int on_header_value(http_parser* _parser, const char* at, size_t length) {
		RESTServerConnection * rs = 	(RESTServerConnection*)_parser->data;
		
		if(	rs->_currentHeader) {
			rs->_headers[string(rs->_currentHeader)] = string(at,length);
			
		}
		rs->_currentHeader = strndup(at,length);//
		
		return 0;
	}
	
};


// MARK: - RESTServerConnection

RESTServerConnection::RESTServerConnection()
		:TCPServerConnection(TCPClientInfo::CLIENT_REST, "REST"){
//	cout<<"Constructing RESTServerConnection \n";
	
	memset(&_hpCB, 0, sizeof(http_parser_settings));
	_hpCB.on_message_begin 		= Private::on_message_begin;
	_hpCB.on_url 					= Private::on_url;
	_hpCB.on_body 					= Private::on_body;
	_hpCB.on_message_complete   = Private::on_message_complete;
	_hpCB.on_header_field 			= Private::on_header_field;
	_hpCB.on_header_value 			= Private::on_header_value;
	_hpCB.on_headers_complete 	= Private::on_headers_complete;

	_url = NULL;
	_body = NULL;
	_isOpen = false;
	_currentHeader = NULL;
	_headers.clear();
	
 
};

RESTServerConnection::~RESTServerConnection() {
//	cout<<"Destructing RESTServerConnection \n";
}

 void RESTServerConnection::didOpen() {

	 if(_url){
		 free(_url);
		 _url = NULL;
	 }
	 
	 if(_body){
		 free(_body);
		 _body = NULL;
	 }
	 
	 if(_currentHeader){
		 free(_currentHeader);
		 _currentHeader = NULL;
	 }
	 _headers.clear();
	 
	 _parser.data = (void*)this;
	 http_parser_init(&_parser, HTTP_REQUEST);
	 _isOpen = true;

};

void RESTServerConnection::willClose() {
	
	  if(_url){
		  free(_url);
		  _url = NULL;
	  }
	  
	  if(_body){
		  free(_body);
		  _body = NULL;
	  }
	
	if(_currentHeader){
		free(_currentHeader);
		_currentHeader = NULL;
	}
	_headers.clear();

	_isOpen = false;
};


void RESTServerConnection::didRecvData(const void *buffer, size_t length){

	size_t nparsed = http_parser_execute(&_parser,
													 &_hpCB,
													 (const char*) buffer, length);
	
	if(nparsed != length){
		// flag problem
	}
	
//
//	auto str = ::convertToString( (char*)buffer,  length);
//	str.erase(remove(str.begin(), str.end(), '\r'), str.end()); //remove cr from string
//	str.erase(remove(str.begin(), str.end(), '\n'), str.end()); //remove cr from string
//
//	cout << "REST RCV \n" << str << endl;
}



string RESTServerConnection::httpHeaderForStatusCode(httpStatusCodes_t  code){
	
	string header =  "HTTP/1.1 ";
	
	switch (code) {
		case STATUS_OK: header+= "200 - OK";
			break;
			
		case STATUS_NO_CONTENT: header+= "204 - No Content";
			break;
			
		case STATUS_ACCESS_DENIED: header+= "401 - Access denied";
			break;
			
		case STATUS_BAD_REQUEST: header+= "400 - Bad Request";
			break;
			
		case STATUS_INVALID_BODY: header+= "400.6 - Invalid Request Body";
			break;
			
		case STATUS_INVALID_METHOD: header+= "405 - Method Not Allowed";
			break;

		case STATUS_NOT_IMPLEMENTED: header+= "501 - Not Implemented";
			break;

		case STATUS_UNAVAILABLE: header+= "503 - Service unavailable";
			break;


			
		case STATUS_INTERNAL_ERROR:;
		default:
			header+= "500 - Internal server error";
			break;
	}
	header += "\r\n";
	return header;
}
 
string RESTServerConnection::httpHeaderForContentLength(size_t length){
	string header =  "Content-Length: " + to_string(length) + "\r\n";
	return header;
}

// MARK:  - RESTServerConnection security

bool RESTServerConnection::validateRequestCredentials(json k){
	
	// here is where you check your headers..
	// see https://github.com/System-Glitch/SHA256
	if(_headers.count("Authorization"))
		return false;
		
	return true;
}

void RESTServerConnection::processHeaders(){
	
	if(_headers.count("User-Agent")){
		string agent = _headers["User-Agent"];
		_info.setUserAgent(agent);
	}

}



// MARK: - TCPServerConnection functions
 
void RESTServerConnection::sendString(const string str){
	sendData(str.c_str(), str.size());
}
 
void RESTServerConnection::closeConnection(){
	close();
}

bool RESTServerConnection::isConnected(){
	return _isOpen;
}


