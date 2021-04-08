//
//  TCPClientInfo.hpp
//  InsteonServer
//
//  Created by Vincent Moscaritolo on 4/2/21.
//

#ifndef TCPClientInfo_h
#define TCPClientInfo_h
 
using namespace std;

class TCPServerConnection;
class TCPServer;

class TCPClientInfo {
	friend TCPServerConnection;
	friend TCPServer;

	public:
	
	typedef enum {
		CLIENT_REST,
		CLIENT_TERMINAL,
		CLIENT_INVALID
	}clientType_t;

	bool isTerminal() 	{return _type == CLIENT_TERMINAL;};
	bool isREST() 		{return _type == CLIENT_REST;};

	void setUserAgent(string s) { _userAgent = s;};
	const string userAgent() {return _userAgent;};
	
	void setHasColor(bool b) { _hasColor = b;};
	bool hasColor() {return _hasColor; };
	
	int localPort() {return _localPort;};
 	sockaddr_storage remoteAddr() { return _remoteAddr;};
	string remoteAddrString();
	
protected:
	TCPClientInfo(clientType_t typ) {_type = typ;};
	
	clientType_t 		_type;
	int 					_localPort;
	sockaddr_storage 	_remoteAddr;
	
	// for terminals
	bool _hasColor;
	string _userAgent;
};

#endif /* TCPClientInfo_h */
