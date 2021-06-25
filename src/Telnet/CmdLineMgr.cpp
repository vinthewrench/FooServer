//
//  CmdLineMgr.cpp
//  InsteonServer
//
//  Created by Vincent Moscaritolo on 3/16/21.
//

#include "CmdLineMgr.hpp"
#include "CmdLineHelp.hpp"

#include <iostream>
#include <regex>
#include <sstream>
#include <iostream>
#include <iomanip>
#include "Utils.hpp"

#include <netinet/in.h>
#include <arpa/inet.h>

#include "TCPServer.hpp"
#include "CmdLineRegistry.hpp"

using namespace std;
using namespace nlohmann;
using namespace rest;


CmdLineMgr::CmdLineMgr(TCPServerConnection* server): _cmdLineBuffer(this) {
	_server = server;
 	_cmdLineBuffer.clear();
 
 }

CmdLineMgr::~CmdLineMgr(){
	
}


void CmdLineMgr::didConnect(){
	registerBuiltInCommands();
	doWelcome();
	_cmdLineBuffer.didConnect();
}

void CmdLineMgr::didDisconnect(){
	_cmdLineBuffer.didDisconnect();
 }

void CmdLineMgr::processChars(uint8_t* data, size_t len){
	_cmdLineBuffer.processChars(data, len);
}


// this can change when te user changes screen sizes
void CmdLineMgr::setTermSize(uint16_t	 width, uint16_t height){
	_cmdLineBuffer.setTermSize(width, height);
}

 void CmdLineMgr::setTermType(string name){
	_server->_info.setUserAgent(name);
}


void  CmdLineMgr::clear(){
	_cmdLineBuffer.clear();
}

bool CmdLineMgr::processCommandLine(std::string cmdLine, boolCallback_t completion){
	
	bool shouldRecordInHistory = false;

	CmdLineRegistry* reg = CmdLineRegistry::shared();
 
	if(!_server->isConnected())
		return(false);
 
	bool hasColor = _server->_info.hasColor();
	
	vector<string> v = split<string>(cmdLine, " ");
	if(v.size() == 0)
		return(false);
		
	string cmd = v.at(0);
	std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);
 
	auto cmCb = reg->handlerForCmd(cmd);
 
	if(cmCb) {
		sendReply("\r\n");
 		shouldRecordInHistory = cmCb(v, this, completion);
	}
	else
	{
		string reply
			= string("\r\n  \x1B[2K")+ "command "
				+ ((hasColor)? "\x1B[36;1;4m" : "\"")
				+  cmd
				+ ((hasColor)? "\x1B[0m": "\"")
				+  " not found\r\n";
		
		sendReply(reply);
		clear();
		completion(true);
	}
	
	return shouldRecordInHistory;
}

void CmdLineMgr::helpForCommandLine(std::string cmdLine, boolCallback_t completion){
	
	if(!_server->isConnected())
		return;
 
	vector<string> v = split<string>(cmdLine, " ");

	doHelp(v);
	
	clear();
	completion(true);
 
}


void CmdLineMgr::registerBuiltInCommands(){
	
	CmdLineRegistry* reg = CmdLineRegistry::shared();
 
	// register any built in commands
	reg->registerCommand("history", [=] (stringvector line,
													 CmdLineMgr* mgr,
													 boolCallback_t completion ){
		mgr->_cmdLineBuffer.doCmdHistory(line);
		(completion) (true);
		return false;
	});
	
	// register any built in commands
	reg->registerCommand("whatismyip", [=] (stringvector line,
														 CmdLineMgr* mgr,
														 boolCallback_t completion ){
		mgr->doWhatIsMyIP(line);
		(completion) (true);
		return true;
	});
 
	// register any built in commands
	reg->registerCommand("finger", [=] (stringvector line,
														 CmdLineMgr* mgr,
														 boolCallback_t completion ){
		mgr->doFinger(line);
		(completion) (true);
		return true;
	});
 
	reg->registerCommand("help", [=](stringvector line,
												CmdLineMgr* mgr,
												boolCallback_t cb){
		mgr->doHelp(line);		
		(cb) (true);
		return false;
	});

	reg->registerCommand("bye", [=](stringvector line,
											  CmdLineMgr* mgr,
											  boolCallback_t cb){
		mgr->sendReply("  Well Bye...\r\n");
		(cb) (true);
		mgr->_server->closeConnection();
		return false;	// dontt record in history

	});
}


stringvector CmdLineMgr::matchesForCmd( const std::string cmd){
	
	
	CmdLineRegistry* reg = CmdLineRegistry::shared();

	vector<string>  options = reg->matchesForCmd(cmd);

//	 	vector<string>  biglist =
//		{"false","file","findrule5.28","footprint","fsck_cs","functionnfc","filebyproc.d",
//			"finger","for","fsck_exfat","functions",
//			"fcgistarter","filecoordinationd","firmwarepasswd","foreach","fsck_hfs","funzip",
//			"fddist","fileproviderctl","fixproc","format-sql","fsck_msdos","fuser",
//			"fdesetup","filtercalltree","flex","format-sql5.18","fsck_udf","fuzzy_match",
//			"fdisk","find","flex++","format-sql5.28","fstyp","fwkdp",
//			"fg","find2perl","float","from","fstyp_hfs","fwkpfv",
//			"fgrep","find2perl5.18","fmt","fs_usage","fstyp_msdos",
//			"fi","findrule","fold","fsck","fstyp_ntfs",
//			"fibreconfig","findrule5.18","fontrestore","fsck_apfs","fstyp_udf"};
//	
//		options.insert(end(options), begin(biglist), end(biglist));

	return options;
};
// MARK: - built in commands
 

void CmdLineMgr::doWelcome(){
	
	
	CmdLineRegistry* reg = CmdLineRegistry::shared();
 
	auto cmCb = reg->handlerForCmd(string(CmdLineRegistry::CMD_WELCOME));
	if(cmCb){
		cmCb({"welcome"}, this, [] (bool didSucceed){ });
	}
}

void CmdLineMgr::doWhatIsMyIP(stringvector params){
 
	string str = _server->_info.remoteAddrString();
	sendReply("  " + str + "\r\n");
}
 
void CmdLineMgr::doFinger(stringvector params){
	std::ostringstream oss;
  
	auto conns =  TCPServerMgr().shared()->getConnectionList();
	for(auto conn: conns){
		
		oss << setw(4) << conn.connID() << setw(0)  << " ";
		oss << setw(7) << conn.clientName() << setw(0)  << " ";
		
 		oss << setw(4) << conn.localPort()  << " ";
		oss << conn.remoteAddrString();
		
		string agent = conn.userAgent();
		if(!agent.empty())
			oss << " " << agent;
		
		oss << "\r\n";
	}
	
	sendReply(oss.str());
}
 
void CmdLineMgr::doHelp(stringvector params){
 
	CmdLineRegistry* reg = CmdLineRegistry::shared();

	auto helpStr = reg->helpForCmd(params);

	sendReply(helpStr);
}


// MARK: - wrappers for TCPServerConnection

void CmdLineMgr::sendReply(std::string str){
	_server->sendString(str);
}
