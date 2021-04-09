//
//  main.cpp
//  InsteonServer
//
//  Created by Vincent Moscaritolo on 3/12/21.
//

#include <unistd.h>			//Needed for sleep
#include "TCPServer.hpp"
#include "Telnet/TelnetServerConnection.hpp"
#include "REST/RESTServerConnection.hpp"
#include "ServerCmdQueue.hpp"
#include "Telnet/CmdLineRegistry.hpp"

#include <regex>
#include <string>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <sstream>

#include "Utils.hpp"
#include "date.h"


// MARK: -  Validators


class ServerCmdArgValidator {

public:
	ServerCmdArgValidator() {};
	virtual ~ServerCmdArgValidator() {};

	virtual bool validateArg( string_view arg)	{ return false; };
	
	virtual bool createJSONentry( string_view arg, json &j) { return false; };
	
	virtual bool containsKey(string_view key, const json &j) {
		return (j.contains(key));
	}
	
	virtual bool getStringFromJSON(string_view key, const json &j, string &result) {
		string k = string(key);
		
		if( j.contains(k) && j.at(k).is_string()){
			result = j.at(k);
			return true;
		}
		return false;
	}
 
	virtual bool getIntFromJSON(string_view key, const json &j, int &result) {
		if( j.contains(key)) {
			string k = string(key);
		
			if( j.at(k).is_string()){
				string str = j.at(k);
				
				char* p;
				long val = strtol(str.c_str(), &p, 0);
				if(*p == 0){
					result = (int)val;
					return  true;;
				}
			}
			else if( j.at(k).is_number()){
				result = j.at(k);
				return true;
			}
		}
		return  false;
	}
};

class DeviceIDArgValidator : public ServerCmdArgValidator {
public:

	virtual bool validateArg( string_view arg) {
		return regex_match(string(arg), std::regex("^[A-Fa-f0-9]{2}.[A-Fa-f0-9]{2}.[A-Fa-f0-9]{2}$"));
	};
	
	virtual bool getvalueFromJSON(string_view key, const json &j, string &result){
	string str;
	if(getStringFromJSON(key, j, str) && validateArg(str)){
		result = str;
		return true;
	}
	return false;
}

};

class DeviceLevelArgValidator : public ServerCmdArgValidator {
	
public:
		
	virtual bool validateArg(string_view arg) {
		int intValue;
		int count = std::sscanf( string(arg).c_str(), "%d", &intValue);
		
		if (count == 1 && intValue >= 0 && intValue < 256){
			return true;
		}
		return false;
	};

	virtual bool getvalueFromJSON(string_view key, const json &j, int &result){
		int intValue;
	
		if(getIntFromJSON(key, j, intValue)
			&& intValue >= 0 && intValue < 256){
			
			result = intValue;
			return true;
		}
		return false;
	}
};

class StringArgValidator : public ServerCmdArgValidator {
public:
 
	virtual bool getvalueFromJSON(string_view key, const json &j, any &result){
		string str;
		if(getStringFromJSON(key, j, str)){
			result = str;
			return true;
		}
		return false;
	}
};


// MARK: - SERVER FUNCTIONS


constexpr string_view FOO_VERSION_STRING		= "1.0.0";

constexpr string_view JSON_ARG_DEVICEID 		= "deviceID";
constexpr string_view JSON_ARG_LEVEL 			= "level";
constexpr string_view JSON_ARG_FUNCTION		= "function";
constexpr string_view JSON_ARG_DATE				= "date";
constexpr string_view JSON_ARG_VERSION			= "version";
constexpr string_view JSON_ARG_TIMESTAMP		= "timestamp";
constexpr string_view JSON_ARG_DEVICEIDS 		= "deviceIDs";

 

constexpr string_view JSON_CMD_SETLEVEL 		= "set";
constexpr string_view JSON_CMD_SHOWDEVICE 	= "show";
constexpr string_view JSON_CMD_DATE			 	= "date";
constexpr string_view JSON_CMD_PLM			 	= "plm";
constexpr string_view JSON_CMD_GROUP			= "group";
constexpr string_view JSON_CMD_VERSION			= "version";

static void SetLevelCommand( ServerCmdQueue* cmdQueue,
									 json request,  // entire request
									 TCPClientInfo cInfo,
									 ServerCmdQueue::cmdCallback_t completion){
	using namespace rest;
	json reply;
	
	DeviceIDArgValidator vDeviceID;
	DeviceLevelArgValidator vLevel;
	
	string deviceID;
	int level;
	
	if(vDeviceID.getvalueFromJSON(JSON_ARG_DEVICEID, request, deviceID)){
		if(vLevel.getvalueFromJSON(JSON_ARG_LEVEL, request, level)){
			
			printf("Set %s %d\n", deviceID.c_str(), level);
			makeStatusJSON(reply,STATUS_OK);
			(completion) (reply, STATUS_OK);
		}
		else {
			makeStatusJSON(reply, STATUS_BAD_REQUEST, "Bad argument for level" );;
			(completion) (reply, STATUS_BAD_REQUEST);
		}
	}
	else {
		makeStatusJSON(reply, STATUS_BAD_REQUEST, "Bad argument for deviceID" );;
		(completion) (reply, STATUS_BAD_REQUEST);
	}
}

static void ShowDeviceCommand( ServerCmdQueue* cmdQueue,
									 json request,  // entire request
										TCPClientInfo cInfo,
										ServerCmdQueue::cmdCallback_t completion){
	using namespace rest;
	json reply;
	
	DeviceIDArgValidator vDeviceID;
	string deviceID;
	
	if(vDeviceID.getvalueFromJSON(JSON_ARG_DEVICEID, request, deviceID)){
		
		printf("Show %s\n", deviceID.c_str() );
		
		reply[string(JSON_ARG_DEVICEID)] = deviceID;
		reply[string(JSON_ARG_LEVEL)] 		= 222;
		
		makeStatusJSON(reply,STATUS_OK);
		(completion) (reply, STATUS_OK);
	}
	else {
		makeStatusJSON(reply, STATUS_BAD_REQUEST, "Bad argument for deviceID" );;
		(completion) (reply, STATUS_BAD_REQUEST);
	}
}


static void ShowDateCommand(ServerCmdQueue* cmdQueue,
								  json request,  // entire request
									 TCPClientInfo cInfo,
								  ServerCmdQueue::cmdCallback_t completion) {
	using namespace rest;
	json reply;
	string plmFunc;
	
	using namespace date;
	using namespace std::chrono;

	printf("DATE request \n" );
	
	const auto tp = time_point_cast<seconds>(system_clock::now());
	auto rfc399 =  date::format("%FT%TZ", tp);
	
	reply["date"] =  rfc399;
 
	makeStatusJSON(reply,STATUS_OK);
 	(completion) (reply, STATUS_OK);
}

static void PlmFunctionsCommand(ServerCmdQueue* cmdQueue,
											json request,  // entire request
										  TCPClientInfo cInfo,
										  ServerCmdQueue::cmdCallback_t completion) {
	using namespace rest;
	json reply;
 
	ServerCmdArgValidator v;
	string plmFunc;
	
	if(v.getStringFromJSON(JSON_ARG_FUNCTION, request, plmFunc)){
		
		printf("PLM %s \n", plmFunc.c_str());
		
		makeStatusJSON(reply,STATUS_OK);
		(completion) (reply, STATUS_OK);
		return;
		
	}
	else {
		makeStatusJSON(reply, STATUS_BAD_REQUEST, "No argument for function" );;
		(completion) (reply, STATUS_BAD_REQUEST);
	}
 }

static void GroupFunctionsCommand(ServerCmdQueue* cmdQueue,
											json request,  // entire request
											 TCPClientInfo cInfo,
										  ServerCmdQueue::cmdCallback_t completion) {
	using namespace rest;
	json reply;
 	
	
		auto dump = request.dump(4);
		dump = replaceAll(dump, "\n","\r\n" );
		cout << dump << "\n";

	ServerCmdArgValidator v;
	DeviceIDArgValidator vDeviceID;

	string groupFunc;
	
	if(v.getStringFromJSON(JSON_ARG_FUNCTION, request, groupFunc)){
		
		printf("Group %s \n", groupFunc.c_str());
		
		string key = string(JSON_ARG_DEVICEIDS);
		
		if( request.contains(key)
			&& request.at(key).is_array()){
			
			auto deviceIDs = request.at(key);
			
			for(auto val : deviceIDs){
				if(val.is_string()){
					string deviceID = val;
					
					if(vDeviceID.validateArg(deviceID)) {
						printf("add: %s\n",deviceID.c_str());
					}
					else {
						// flag error
					}
				}
				
			}
		}
		
		makeStatusJSON(reply,STATUS_OK);
		(completion) (reply, STATUS_OK);
		return;
		
	}
	else {
		makeStatusJSON(reply, STATUS_BAD_REQUEST, "No argument for function" );;
		(completion) (reply, STATUS_BAD_REQUEST);
	}
}


static void VersionCommand(ServerCmdQueue* cmdQueue,
											json request,  // entire request
										TCPClientInfo cInfo,
										  ServerCmdQueue::cmdCallback_t completion) {
	using namespace rest;
	json reply;
	
	reply[string(JSON_ARG_VERSION)] =   FOO_VERSION_STRING;
	reply[string(JSON_ARG_TIMESTAMP)]	=  string(__DATE__) + " " + string(__TIME__);
	
	makeStatusJSON(reply,STATUS_OK);
	(completion) (reply, STATUS_OK);
}



// MARK: - COMMAND LINE FUNCTIONS

static bool SETcmdHandler( stringvector 		line,
								  CmdLineMgr* 		mgr,
								  boolCallback_t 	cb){
	
	using namespace rest;
	string errorStr;
	string command = line[0];
	string deviceID;
	string onlevel;
	
	
	DeviceIDArgValidator v1;
	DeviceLevelArgValidator v2;
	
	if(	line.size() > 1)
		deviceID = line[1];
	
	if(	line.size() > 2)
		onlevel = line[2];
	
	if(	deviceID.empty()) {
		errorStr =  "Command: \x1B[36;1;4m"  + command + "\x1B[0m expects deviceID.";
	}
	else if(command == "dim" && onlevel.empty()) {
		errorStr =  "Command: \x1B[36;1;4m"  + command + "\x1B[0m expects level.";
	}
	else if(!v1.validateArg(deviceID)){
		errorStr =  "\x1B[36;1;4m"  + deviceID + "\x1B[0m is not a valid DeviceID.";
	}
	else if( !onlevel.empty() && !v2.validateArg(onlevel)) {
		errorStr =  "\x1B[36;1;4m"  + onlevel + "\x1B[0m is not a valid device level.";
	}
	else {
		
		json request;
		request[kREST_command] =  JSON_CMD_SETLEVEL ;
		request[string(JSON_ARG_DEVICEID)] =  line[1];
		
		if(command == "turn-on") {
			request[string(JSON_ARG_LEVEL)] = 255;
		}
		else if(command == "turn-off") {
			request[string(JSON_ARG_LEVEL)] = 0;
		}
		else if(command == "dim"){
			int intValue;
			std::sscanf( string(onlevel).c_str(), "%d", &intValue);
			request[string(JSON_ARG_LEVEL)] =  intValue;
		}
		
//		auto dump = request.dump(4);
//		dump = replaceAll(dump, "\n","\r\n" );
//		cout << dump << "\n";
//
		ServerCmdQueue* cmdQueue = ServerCmdQueue::shared();
		TCPClientInfo cInfo = mgr->getClientInfo();

		cmdQueue->queueCommand(request, cInfo, [=] (json reply, httpStatusCodes_t code) {
			
			bool success = didSucceed(reply);
			
			if(success) {
				mgr->sendReply( "OK \n\r");
			}
			else {
				string error = errorMessage(reply);
				mgr->sendReply( error + "\n\r");
			}
			
			(cb) (success);
		});
		
		return true;
	}
	
	mgr->sendReply(errorStr + "\n\r");
	return false;
};

static bool DATEcmdHandler( stringvector line,
									CmdLineMgr* mgr,
									boolCallback_t	cb){
	using namespace rest;
	json request;
	request[kREST_command] =  JSON_CMD_DATE ;
	
	TCPClientInfo cInfo = mgr->getClientInfo();
	ServerCmdQueue::shared()->queueCommand(request,cInfo, [=] (json reply, httpStatusCodes_t code) {
		
		string str;
  
		if(reply.count(JSON_ARG_DATE) ) {
			string rfc399 =  reply[string(JSON_ARG_DATE)] ;
  
			using namespace date;
			using namespace std::chrono;

			std::chrono::system_clock::time_point tp;
			std::istringstream in{ rfc399 };
			in >> date::parse("%FT%TZ", tp);
			
	//		auto pdt_now = make_zoned(current_zone(), utc_now);
	//		cout << format("%a, %b %d %Y at %I:%M %p %Z\n", pdt_now);
	//
			time_t tt = system_clock::to_time_t ( tp );
			struct tm * timeinfo = localtime (&tt);
		
			str = string(asctime(timeinfo));  // asctime appends \n
			
			mgr->sendReply(str + "\r");
		}
		
		(cb) (code > 199 && code < 400);
	});
	
	//cmdQueue->queue
	
		return true;
};

static bool SHOWcmdHandler( stringvector line,
									CmdLineMgr* mgr,
									boolCallback_t	cb){
	using namespace rest;

	string errorStr;
	string command = line[0];
	string deviceID;
	string onlevel;
	
	DeviceIDArgValidator v1;
	
	if(	line.size() > 1)
		deviceID = line[1];
	
	if(	deviceID.empty()) {
		errorStr =  "Command: \x1B[36;1;4m"  + command + "\x1B[0m expects deviceID.";
	}
	else if(!v1.validateArg(deviceID)){
		errorStr =  "\x1B[36;1;4m"  + deviceID + "\x1B[0m is not a valid DeviceID.";
	}
	else {
		
		json request;
		request[kREST_command] =  JSON_CMD_SHOWDEVICE ;
		request[string(JSON_ARG_DEVICEID)] =  line[1];
		
		TCPClientInfo cInfo = mgr->getClientInfo();
		ServerCmdQueue::shared()->queueCommand(request,cInfo, [=] (json reply, httpStatusCodes_t code) {
			
			bool success = didSucceed(reply);
			
			if(success) {
				
				auto dump = reply.dump(4);
				dump = replaceAll(dump, "\n","\r\n" );
				cout << dump << "\n";
		
				if(reply.count(JSON_ARG_LEVEL)) {
					
					int level = reply.at(string(JSON_ARG_LEVEL)).get<int>();
		 			mgr->sendReply( "level: " + to_string(level) +"\n\r" );
				}
			}
			else {
				string error = errorMessage(reply);
				mgr->sendReply( error + "\n\r");
			}
			
			(cb) (success);
		});
		
		return true;
	}
	
	mgr->sendReply(errorStr + "\n\r");
	(cb)(false);
	return false;
};

static bool PLMcmdHandler( stringvector line,
								  CmdLineMgr* mgr,
								  boolCallback_t	cb){
	using namespace rest;

	
	mgr->sendReply("not written yet \n\r");
	(cb)(false);
 
	return false;
};


static bool VersionCmdHandler( stringvector line,
								  		CmdLineMgr* mgr,
										boolCallback_t	cb){
	using namespace rest;
	
	using namespace rest;
	json request;
	request[kREST_command] =  JSON_CMD_VERSION ;
	
	TCPClientInfo cInfo = mgr->getClientInfo();
	
	for (auto& t : cInfo.headers()){
		printf("%s = %s\n", t.first.c_str(), t.second.c_str());
	}

	ServerCmdQueue::shared()->queueCommand(request,cInfo,[=] (json reply, httpStatusCodes_t code) {
		
		bool success = didSucceed(reply);
		
		if(success) {
			
 			std::ostringstream oss;

			if(reply.count(JSON_ARG_VERSION) ) {
				string ver = reply[string(JSON_ARG_VERSION)];
				oss << ver << " ";
			}
	
			if(reply.count(JSON_ARG_TIMESTAMP) ) {
				string timestamp = reply[string(JSON_ARG_TIMESTAMP)];
				oss <<  timestamp;
			}
	
			oss << "\r\n";
			mgr->sendReply(oss.str());
					
		}
		else {
			string error = errorMessage(reply);
			mgr->sendReply( error + "\n\r");
		}
		
		(cb) (success);
		
	});
	
	return true;
};


static bool GroupCmdHandler( stringvector line,
								  CmdLineMgr* mgr,
								  boolCallback_t	cb){
 
	using namespace rest;
	string errorStr;

	string command = line[0];

	if(line.size() < 2){
		errorStr =  "Command: \x1B[36;1;4m"  + command + "\x1B[0m expects a function.";
 	}
	else {
		string subcommand = line[1];

		json request;
		request[kREST_command] =  JSON_CMD_GROUP ;

		if(subcommand == "add"){
			request[string(JSON_ARG_FUNCTION)] = "add";
			
			int count = 0;
			for(int idx = 2; idx < line.size(); idx++) {
				string item = line[idx];
				request[string(JSON_ARG_DEVICEIDS)][count++] = item;
			}
		
			TCPClientInfo cInfo = mgr->getClientInfo();
			ServerCmdQueue::shared()->queueCommand(request,cInfo,[=] (json reply, httpStatusCodes_t code) {

				bool success = didSucceed(reply);
				
				if(success) {
					
					auto dump = reply.dump(4);
					dump = replaceAll(dump, "\n","\r\n" );
					cout << dump << "\n";
			 
					mgr->sendReply( "OK\n\r" );
					 
				}
				else {
					string error = errorMessage(reply);
					mgr->sendReply( error + "\n\r");
				}
				
				(cb) (success);
			});
 		}
		return true;
	}
	
	mgr->sendReply(errorStr + "\n\r");
	(cb)(false);
	return false;
};




// MARK: -  register commands

static void registerCommands() {
 
	// create the server command processor
	auto cmdQueue = ServerCmdQueue::shared();
		
	// register all our server commands
	cmdQueue->registerCommand( JSON_CMD_SETLEVEL,
									  SetLevelCommand);
	
	cmdQueue->registerCommand( JSON_CMD_SHOWDEVICE,
									  ShowDeviceCommand);
	
	cmdQueue->registerCommand( JSON_CMD_DATE,
									  ShowDateCommand);
	
	cmdQueue->registerCommand( JSON_CMD_PLM,
									  PlmFunctionsCommand);
	
	cmdQueue->registerCommand( JSON_CMD_GROUP,
									  GroupFunctionsCommand);

	cmdQueue->registerCommand( JSON_CMD_VERSION,
									  VersionCommand);

	
	
	 // register command line commands
	 auto cmlR = CmdLineRegistry::shared();
	cmlR->registerCommand("turn-on",  	SETcmdHandler);
	cmlR->registerCommand("turn-off",  SETcmdHandler);
	cmlR->registerCommand("dim",  		SETcmdHandler);
	
 	cmlR->registerCommand("show",  	SHOWcmdHandler);
	cmlR->registerCommand("plm",  	PLMcmdHandler);
	cmlR->registerCommand("date",	DATEcmdHandler);

	cmlR->registerCommand("group",	GroupCmdHandler);
	cmlR->registerCommand("version",	VersionCmdHandler);

}



// MARK: - MAIN

int main(int argc, const char * argv[]) {
 

 	// create the server command processor
	auto cmdQueue = ServerCmdQueue::shared();
	
	registerCommands();

	TCPServer telnet_server(cmdQueue);
	telnet_server.begin(2020, true, [=](){
		return new TelnetServerConnection();
	});
	
	TCPServer rest_server(cmdQueue);
	rest_server.begin(8080, false, [=](){
		return new RESTServerConnection();
	});

	while(true) {
		sleep(30);
	}
	
	return 0;
}
