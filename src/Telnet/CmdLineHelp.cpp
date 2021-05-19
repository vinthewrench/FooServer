//
//  CmdLineHelp.cpp
//  FOOServer
//
//  Created by Vincent Moscaritolo on 5/13/21.
//

#include <stdio.h>
#include <string.h>
#include <vector>
#include <sstream>
#include "CmdLineHelp.hpp"

#include "Utils.hpp"

CmdLineHelp *CmdLineHelp::sharedInstance = 0;

CmdLineHelp::CmdLineHelp() {
	 };

CmdLineHelp::~CmdLineHelp() {
};


bool CmdLineHelp::setHelpFile(const string path ){
	_helpFilePath = path;

	bool statusOk = false;

	statusOk = loadHelpFile();
	return statusOk;
}

typedef enum  {
	LOAD_UNKNOWN = 0,
	LOAD_TOPIC,
}loadState_t;

bool CmdLineHelp::loadHelpFile(){
	bool statusOk = false;
	
	FILE * fp;
	loadState_t state = LOAD_UNKNOWN;
	
	if(_helpFilePath.empty())
		return false;
	
	if( (fp = fopen (_helpFilePath.c_str()	, "r")) == NULL)
		return false;
	
	char buffer[256];
	char topic[33] = {0};
	fpos_t startPos = 0;
	fpos_t endPos = 0;
	
	while(fgets(buffer, sizeof(buffer), fp) != NULL) {
		
		size_t len = strlen(buffer);
		if (len == 0) continue;
		if (len == 1 && buffer[0] == '\n' ) continue;
		
		switch (state) {
			case LOAD_UNKNOWN:
				topic[0] = 0;
				startPos = 0;
				break;
 
			default:
				break;
		}
		
		char *p = buffer;
		
		// skip ws
		while(isspace(*p) && (*p !='\0')) p++;
		if(*p == '#') continue;
		
		// check for Token:
		char token[17] = {0};
		int cnt = 0;
		
		cnt = sscanf(p, "%16[^ :]: \"%32[^ \"]:\"", token,topic);
		
		if(cnt == 2
			&& (strncmp(token, "TOPIC-START", 11) == 0)
			&& strlen(topic) > 0) {
			
			fgetpos(fp, &startPos);
			state = LOAD_TOPIC;
			continue;
		}
		else if(cnt == 1
				  && (strncmp(token, "TOPIC-END", 9) == 0)){
			
			fgetpos(fp, &endPos);
			endPos -=len;  // don't include this line
			
//			printf("- %s %lld %lld = %lld\n", topic, startPos, endPos, endPos-startPos);
			_topics[string(topic)] =  make_pair(startPos, endPos);
			state = LOAD_UNKNOWN;
			continue;
		}
		else {
			continue;
		}
	};
	fclose(fp);
	
	statusOk = !_topics.empty();
	
	return statusOk;
	
}


string CmdLineHelp::helpForCmd( string cmd){

	std::ostringstream oss;

	if(cmd.empty()){
		oss << "help topics: \r\n";
	
		vector<string> items;
		for (auto& [key, value] : _topics) {
			items.push_back(key);
		}
		sort(items.begin(), items.end());
		
		for (auto key  : items) {
			oss <<  "  " << key << "\r\n";
		}
		
	}
	else {
		if(_topics.count(cmd) == 0){
			oss << "help is not available for topic \"" << cmd  << "\"\r\n";
		}
		else {
	 		oss <<  helpForTopic(cmd);

		}
	}
	 
	return oss.str();
}

string CmdLineHelp::helpForTopic( string topic){
	std::ostringstream oss;

	if(_topics.count(topic) >0) {
		auto value = _topics.at(topic);
		fpos_t startpos = value.first;
		fpos_t endpos = value.second;
		size_t len =  endpos-startpos;
		
		FILE * fp;
		if( (fp = fopen (_helpFilePath.c_str()	, "r")) != NULL) {
			fsetpos(fp, &startpos);
			char* data = (char*) malloc(len);
			fread(data, len, 1, fp);
			string str = string(data,len);
			free(data);
			str = replaceAll(str, "\n","\r\n" );
		oss << str;
			fclose(fp);
		}

//		oss << (len) << " bytes are available\r\n";

	}
	
	return oss.str();
}
 
