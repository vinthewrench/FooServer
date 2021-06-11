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

#include "sha256.h"
#include "hmac.h"
#include "hash.h"
#include "RESTServerConnection.hpp"
 #include "TCPClientInfo.hpp"

using namespace nlohmann;
using namespace std;
using namespace rest;

[[clang::no_destroy]] const string HTTPHEADER_JSON  		= "Content-Type: application/json\r\n";
[[clang::no_destroy]] const string HTTPHEADER_KEEPALIVE  = "Connection: keep-alive\r\n";
[[clang::no_destroy]] const string HTTPHEADER_CLOSE  		= "Connection: close\r\n";
[[clang::no_destroy]] const string HTTPHEADER_CRLF  		= "\r\n";


// MARK: - RESTServerConnection

RESTServerConnection::RESTServerConnection()
:TCPServerConnection(TCPClientInfo::CLIENT_REST, "REST"){

	_isOpen = false;

	_rURL.setCallBack([=] (){
		
		string errorReply;
		
		auto headers = _rURL.headers();
		if(headers.count("User-Agent")){
			string agent = headers["User-Agent"];
			_info.setUserAgent(agent);
		}
		
		auto body = _rURL.body();
		bool mustAuthenticate = apiSecretMustAuthenticate();

		if( mustAuthenticate == false
			|| validateRequestCredentials()){
			
			queueRESTCommand(_rURL, [=] (json rp, httpStatusCodes_t code){
				
				string body;
				if(rp.size())
					body = rp.dump();
				
				string header = httpHeaderForStatusCode(code);
				header+= HTTPHEADER_JSON;
				header += HTTPHEADER_CLOSE;
				
				header+= httpHeaderForContentLength(body.size());
				header += HTTPHEADER_CRLF;
				string reply = header + body;
				
				sendString(reply);
			});
		}
		else {
			errorReply = httpHeaderForStatusCode(STATUS_ACCESS_DENIED);
		}
		
		if(errorReply.size())
		{
			errorReply += httpHeaderForContentLength(0) + HTTPHEADER_CRLF;
			sendString(errorReply);
		}
		
	});
};

RESTServerConnection::~RESTServerConnection() {
//	cout<<"Destructing RESTServerConnection \n";
}

 void RESTServerConnection::didOpen() {
 
	 _rURL.clear();
	 _isOpen = true;
};

void RESTServerConnection::willClose() {
	
	_rURL.clear();
	_isOpen = false;
};


void RESTServerConnection::didRecvData(const void *buffer, size_t length){

  	_rURL.processData((const char*) buffer, length, true);
}



string RESTServerConnection::httpHeaderForStatusCode(httpStatusCodes_t  code){
	
	string header =  "HTTP/1.1 ";
	
	switch (code) {
		case STATUS_OK: header+= "200 - OK";
			break;
			
		case STATUS_NO_CONTENT: header+= "204 - No Content";
			break;
			
		case STATUS_NOT_MODIFIED: header+= "304 - Not Modified";
			break;
	
		case STATUS_ACCESS_DENIED: header+= "401 - Access denied";
			break;
			
		case STATUS_BAD_REQUEST: header+= "400 - Bad Request";
			break;
			
		case STATUS_NOT_FOUND: header+= "404 - Not found";
			break;
	 
		case STATUS_INVALID_BODY: header+= "400.6 - Invalid Request Body";
			break;
	
		case STATUS_CONFLICT: header+= "409 - Conflict";
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


 
bool RESTServerConnection::validateRequestCredentials(){
	
	string authString;
	string timeString;
	string authKey;
	string urlPath;
	string http_method;
	string APISecret;
	
	if(_rURL.headers().count("Authorization"))
		authString  = _rURL.headers().at("Authorization");
	
	if(_rURL.headers().count("X-auth-date"))
		timeString =  _rURL.headers().at("X-auth-date");
	
	// get the user/ApI KEY
	if(_rURL.headers().count("X-auth-key"))
		authKey =  _rURL.headers().at("X-auth-key");
	
	http_method = string(http_method_str(_rURL.method()));
	
	for(auto str : _rURL.path())
		urlPath += "/" + str;
	
	// check the time string  -   must be within a 5 minute window.
	const time_t quantum = 5;
	const time_t SECS_PER_MIN =  ((time_t)(60UL));
	
	time_t requestTime  = (time_t) atoll(timeString.c_str());
	time_t now = time(NULL);
	time_t diff = abs(requestTime - now);
	
	if(diff > SECS_PER_MIN * quantum)
		return false;
	
	string stringToSign = http_method  + "|" + urlPath
	+ "|" + _rURL.bodyHash()  + "|" + timeString
	+ "|" + authKey;
	
	if(! getAPISecret(authKey, APISecret))
		return false;
	
	string hmacStr = 	hmac<SHA256> (stringToSign, APISecret);
	
	return  authString == hmacStr;
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


