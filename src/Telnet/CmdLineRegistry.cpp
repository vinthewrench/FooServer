//
//  CmdLineRegistry.cpp
//  InsteonServer
//
//  Created by Vincent Moscaritolo on 4/6/21.
//
#include <sstream>
#include <iostream>
#include <iomanip>

#include "CmdLineRegistry.hpp"
#include "CmdLineHelp.hpp"

CmdLineRegistry *CmdLineRegistry::sharedInstance = 0;

CmdLineRegistry::CmdLineRegistry(){
	_commandMap.clear();
}

CmdLineRegistry::~CmdLineRegistry(){
	_commandMap.clear();
}


void CmdLineRegistry::registerCommand(const string name,
												  cmdHandler_t cb){
	string str = name;
 	std::transform(str.begin(), str.end(), str.begin(), ::tolower);
	_commandMap[str] = cb;
}

void CmdLineRegistry::removeCommand(const string name){
	string str = name;
	std::transform(str.begin(), str.end(), str.begin(), ::tolower);
	_commandMap.erase(str);
};

vector<string> CmdLineRegistry::matchesForCmd( const string cmd){
	
	vector<string> options = registeredCommands();
	vector<string> results;
	
	string search = cmd;
	std::transform(search.begin(), search.end(), search.begin(), ::tolower);
	
	for(auto str :options){
		if(str.find(search) == 0){
			results.push_back(str);
		}
	}
	
	sort(results.begin(), results.end());
	
	return results;
}

vector<string> CmdLineRegistry::registeredCommands(){

	vector<string>  results;
	results.clear();
 
	for(auto it = _commandMap.begin();  it != _commandMap.end(); ++it) {
		results.push_back(it->first);
	};
	
	return results;
}

CmdLineRegistry::cmdHandler_t CmdLineRegistry::handlerForCmd( const string cmd){
	cmdHandler_t func = NULL;
	
	auto it = _commandMap.find(cmd);
	if(it != _commandMap.end()){
		func = it->second;
	}
	
	return func;
}

// MARK: - help

string CmdLineRegistry::helpForCmd( stringvector params){
	
	std::ostringstream oss;
		
	if(params.size() == 0){
		// do generic help
		
		auto str = CmdLineHelp::shared()->helpForCmd("");
 		oss << " ?\r\n" << str << "\r\n";
	}
	else {
		
		string cmd = params.at(0);
		std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);
		
		if(cmd == "help"){
 			auto str = CmdLineHelp::shared()->helpForCmd(params.size() >1?params.at(1):"");
			oss << "\r\n" << str << "\r\n";
		}
		else {
			auto str = CmdLineHelp::shared()->helpForCmd(cmd);
			oss << " ?\r\n" << str << "\r\n";
		}
 	}
	return oss.str();
}
