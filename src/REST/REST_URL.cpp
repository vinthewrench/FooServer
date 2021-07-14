//
//  REST_URL.cpp
//  FOOServer
//
//  Created by Vincent Moscaritolo on 5/4/21.
//

#include "REST_URL.hpp"

#include <iostream>
#include "yuarel.h"
#include "sha256.h"
#include "hmac.h"

using namespace nlohmann;
using namespace std;

struct REST_URL::Private
{
	
	static int on_message_begin(http_parser* _parser) {
		
		REST_URL * rs = 	(REST_URL*)_parser->data;
		
		rs->clear();
	
		//	printf("\n***MESSAGE BEGIN***");
		return 0;
	}
	

	static int on_message_complete(http_parser* _parser) {
		REST_URL * rs = 	(REST_URL*)_parser->data;
	
		struct yuarel url;
		yuarel_parse(&url, rs->_urlBytes);
		
		rs->_method = (enum http_method) _parser->method;
 
		if(rs->_calculateHash){
			SHA256 hash;
			
			if(rs->_bodyBytes
				&& strlen(rs->_bodyBytes) > 0
				&& (string(rs->_bodyBytes) != "{}")) {
				hash.add(rs->_bodyBytes, strlen(rs->_bodyBytes));
			}
			else
			{
				string empty;
				hash.add(&empty[0], empty.size());
			}
			rs->_bodyHashHex = hash.getHash();
			
			hash.reset();
		}

		if(rs->_bodyBytes)
			rs->_body = json::parse(string(rs->_bodyBytes));

		string query = url.query?string(url.query):string();
		std::transform(query.begin(), query.end(), query.begin(), ::tolower);

		constexpr  int maxparts = 10;
		char *parts[maxparts];
	 
		int count = yuarel_split_path(url.path, parts, maxparts);
		for(int i = 0; i < count; i++){
			rs->_path.push_back(string(parts[i]));
		}
	 
		constexpr  int maxqueries = 10;
		struct yuarel_param params[maxqueries];
	
		count  = yuarel_parse_query(url.query, '&', params, maxqueries);
		while (count -- > 0) {
			string  val =  (params[count].val != NULL)?string(params[count].val):"";
			rs->_queries[string(params[count].key)] = val;
		}
	
		rs->_valid = true;
		
		// free up c alloc data
		if(rs->_urlBytes){
			  free(rs->_urlBytes);
			rs->_urlBytes = NULL;
		  }
		  
		if(rs->_bodyBytes){
			free(rs->_bodyBytes);
			rs->_bodyBytes = NULL;
		}
		
		if(rs->_currentHeader){
			free(rs->_currentHeader);
			rs->_currentHeader = NULL;
		}
		
		// make callback
		if(rs->_completion){
			(rs->_completion)();
		}
	
		return 0;
	}

	
	static int on_url(http_parser* _parser, const char* at, size_t length) {
		REST_URL * rs = 	(REST_URL*)_parser->data;

		rs->_urlBytes = strndup(at,length);
		return 0;
	}
	
	static int on_body(http_parser* _parser, const char* at, size_t length) {
		REST_URL * rs = 	(REST_URL*)_parser->data;

		if(!rs->_bodyBytes) {
			rs->_bodyBytes = strndup(at,length);
		} else {
			rs->_bodyBytes = (char*) realloc(rs->_bodyBytes, strlen(rs->_bodyBytes) + length + 1);
			rs->_bodyBytes = strncat(rs->_bodyBytes,at,length);
		}
		return 0;
	}
	
	/* Not used */
	static int on_headers_complete(http_parser* _parser) {
		REST_URL * rs = 	(REST_URL*)_parser->data;

		if(rs->_currentHeader){
			free(rs->_currentHeader);
			rs->_currentHeader = NULL;
		}
		return 0;
	}
	
	static int on_header_field(http_parser* _parser, const char* at, size_t length) {
		REST_URL * rs = 	(REST_URL*)_parser->data;

		
		if(rs->_currentHeader){
			free(rs->_currentHeader);
			rs->_currentHeader = NULL;
		}
		rs->_currentHeader = strndup(at,length);
		
		return 0;
	}
	
	static int on_header_value(http_parser* _parser, const char* at, size_t length) {
		REST_URL * rs = 	(REST_URL*)_parser->data;

		if(	rs->_currentHeader) {
			rs->_headers[string(rs->_currentHeader)] = string(at,length);
			
		}
		rs->_currentHeader = strndup(at,length);//
		
		return 0;
	}
};

void REST_URL::commonInit() {
	
	memset(&_hpCB, 0, sizeof(http_parser_settings));
	_hpCB.on_message_begin 		= Private::on_message_begin;
	_hpCB.on_url 					= Private::on_url;
	_hpCB.on_body 					= Private::on_body;
	_hpCB.on_message_complete    = Private::on_message_complete;
	_hpCB.on_header_field 			= Private::on_header_field;
	_hpCB.on_header_value 			= Private::on_header_value;
	_hpCB.on_headers_complete 	= Private::on_headers_complete;

	_urlBytes = NULL;
	_bodyBytes = NULL;
 	_currentHeader = NULL;
	_headers.clear();
	_completion = NULL;
	_valid = false;
	_bodyHashHex.clear();
	
	_parser.data = (void*)this;
	http_parser_init(&_parser, HTTP_REQUEST);
}

void REST_URL::clear(){

	if(_urlBytes){
		  free(_urlBytes);
		  _urlBytes = NULL;
	  }
	  
	  if(_bodyBytes){
		  free(_bodyBytes);
		  _bodyBytes = NULL;
	  }
	_bodyHashHex.clear();

 	if(_currentHeader){
		free(_currentHeader);
		_currentHeader = NULL;
	}
	_headers.clear();
	_queries.clear();
	_path.clear();
	_method = (enum http_method) 0;
	_body.clear();
	_valid = false;
}



REST_URL::REST_URL(){
	commonInit();
}

REST_URL::REST_URL(string urlString){
	commonInit();
	processData(urlString.c_str(), urlString.size());
}

REST_URL::~REST_URL(){
	clear();
}


size_t REST_URL::processData(const void *buffer, size_t length, bool shouldHashData){
	
	_calculateHash = shouldHashData;
	size_t nparsed = http_parser_execute(&_parser,
													 &_hpCB,
													 (const char*) buffer, length);

	return nparsed;
	
}

// string 	REST_URL::calculateHMAC(string key){
//
//	string result;
//
//	string userString;
//
//	string timeString;
//	if(headers().count("X-auth-date")) {
//		timeString =  headers().at("X-auth-date");
//		// we need to roumnd this to the 5 minute interval?
//	}
//
//	// get the user/ApI KEY
//	if(headers().count("X-auth-key")) {
//		userString =  headers().at("X-auth-key");
// 	}
//
//	string  http_method = string(http_method_str(_method));
// 	string path = std::accumulate(_path.begin(), _path.end(), string("/"));
//
//	string kSigning = http_method  + "|" + path
//	+ "|" + _bodyHashHex  + "|" + timeString
//	+ "|" + userString;
//
//	string hmacStr = 	hmac<SHA256> (kSigning, key);
//	//				cout << "hmac: " << hmacStr << "\n";
//	//
//
//	return hmacStr;
//
//}
