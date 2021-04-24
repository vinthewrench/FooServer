//
//  CmdLineBuffer.cpp
//  InsteonServer
//
//  Created by Vincent Moscaritolo on 3/14/21.
//


#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <string>
#include <stdio.h>
#include <limits.h>
#include <regex>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <netinet/in.h>

#include "CmdLineBuffer.hpp"
#include "Utils.hpp"


#define ALLOC_QUANTUM 4
using namespace std;

 
CmdLineBuffer::CmdLineBuffer(CmdLineBufferManager * bufMgr) {

	_clMgr = bufMgr;

	dbuff_init();
	_state = CLB_UNKNOWN;
	_history.clear();
	_historyIndex = ULONG_MAX;
	_displayingCompletionOptions = 0;

	readHistoryFile();
	
//	_isConnected = false;
}
 
CmdLineBuffer::~CmdLineBuffer(){
	
	if(_dbuf.data)
		free(_dbuf.data);
	
	_dbuf.used = 0;
	_dbuf.pos = 0;

	_dbuf.alloc = 0;
	_history.clear();

}

void  CmdLineBuffer::didConnect(){
	clear();
	_state = CLB_READY;
	sendReply(STR_PROMPT);
}

void  CmdLineBuffer::didDisconnect(){
	_state = CLB_UNKNOWN;
	clear();
 }
 

void  CmdLineBuffer::clear(){
	_dbuf.used = 0;
	_dbuf.pos = 0;
	_displayingCompletionOptions = 0;
	_state = CLB_READY;
}
 

void CmdLineBuffer::processChars(uint8_t* data, size_t len){
	
	for( size_t idx = 0; idx < len; idx++){
		
		uint8_t ch = data[idx];
		
	// last char is null,. Ignore
		if(idx == (len -1) && ch == 0) return;
		
		processChar(ch);
	}
}

void CmdLineBuffer::setTermSize(uint16_t	 width, uint16_t height){
	
	_termWidth = width;
}



//MARK: -  private

void CmdLineBuffer::processChar(uint8_t ch){
	
//	printf("%c - %02X\n", ch > CHAR_PRINTABLE?ch:' ', ch);
	
	
	switch (_state) {
			
		case CLB_READY:{
	
			if(ch != CHAR_TAB && _displayingCompletionOptions)
				removeCompletions()	;

			if(ch == CHAR_ESC ) {
				_state = CLB_ESC;
				
			} else if(ch == CHAR_DEL ) {
				maybeCopyHistoryToBuffer();
				handleDeleteChar();
				
			} else if(ch == CHAR_TAB ) {
				maybeCopyHistoryToBuffer();
				handleTabChar();
				
			} else if(ch == CHAR_CNTL_U) {
				maybeCopyHistoryToBuffer();
				handleClearLine();
				
			} else if(ch == CHAR_CNTL_A) {
				maybeCopyHistoryToBuffer();
				handleBeginingOfLine();
	
			} else if(ch == CHAR_CNTL_E) {
				maybeCopyHistoryToBuffer();
 				handleEndOfLine();

			} else if(ch == CHAR_CNTL_K) {
				maybeCopyHistoryToBuffer();
 				handleClearToEndOfLine();
				
			} else if(ch == CHAR_BS ) {
				maybeCopyHistoryToBuffer();
				handleBackArrow();
				
			}else if(ch == CHAR_CR  ) {
				processBuffer();
			}
			else if(ch > CHAR_PRINTABLE){
				maybeCopyHistoryToBuffer();
				if(_dbuf.pos < _dbuf.used) {
					append_char(ch);
					sendReply("\x1B[4h"  + string(1, ch) + "\x1B[4l");
	 			}
				else {
					append_char(ch);
					sendReply(string(1, ch));
				}
			}
		}
			break;
			
		case CLB_ESC:{
			if(ch == '[')
				_state = CLB_ESC1;
			
			else {
				printf("Unknown ESC %c - %02X\n", ch > CHAR_PRINTABLE?ch:'?', ch);
				// an error here?
				_state = CLB_READY;
			}
		}
			break;
			
		case CLB_ESC1:{
			_state = CLB_READY;
			
			switch (ch) {
				case 'A':		//  up arrow
					handleUpArrow();
					break;
					
				case 'B':	//  down arrow
					handleDownArrow();
					break;
					
				case 'C':	//  fwd arrow
					maybeCopyHistoryToBuffer();
					handleFwdArrow();
					break;
					
				case 'D':	//  back arrow
					maybeCopyHistoryToBuffer();
					handleBackArrow();
					break;
					
				default:
					// invalid
					printf("Unknown ESC [ %c - %02X\n", ch > CHAR_PRINTABLE?ch:'?', ch);
					break;
			}
			
		}
			break;
			
		default:
			break;
	}
}


void CmdLineBuffer::handleDeleteChar(){
	
	if(_dbuf.pos > 0) {
		delete_char();
		sendReply("\b\x1B[P");
	}
}

void CmdLineBuffer::handleTabChar(){
	displayCompletions();
	
}


void CmdLineBuffer::handleClearLine(){
	removeCompletions();
	clear();
	sendReply("\r\x1B[2K");
	sendReply(STR_PROMPT);
}



void CmdLineBuffer::handleBeginingOfLine(){
	_dbuf.pos = 0;
	sendReply("\r" + STR_PROMPT);
}

void CmdLineBuffer::handleEndOfLine(){
	_dbuf.pos = _dbuf.used;
	sendReply("\r\x1B[" + to_string(_dbuf.pos + 2) + "C");
}

void CmdLineBuffer::handleClearToEndOfLine(){
	_dbuf.used = _dbuf.pos;
	sendReply("\x1B[K");
}

void CmdLineBuffer::handleUpArrow(){
	
	removeCompletions();
	if(_history.size() == 0)
		return;
	
	if(_historyIndex > _history.size())
		_historyIndex = _history.size()-1;
	else if(_historyIndex > 0)
		_historyIndex--;

	displayHistoryLine();
}

void CmdLineBuffer::handleDownArrow(){

	removeCompletions();
	if(_history.size() == 0)
		return;
	
	if(_historyIndex > _history.size()){
		_historyIndex = ULONG_MAX;
		return;
	}

	_historyIndex++;
	 
	displayHistoryLine();
}

void CmdLineBuffer::handleBackArrow(){
	
	if(_dbuf.pos > 0)
	{
		_dbuf.pos--;
		sendReply("\x1B[0D");
	}
}

void CmdLineBuffer::handleFwdArrow(){
	
	if(_dbuf.pos < _dbuf.used)
	{
		_dbuf.pos++;
		sendReply("\x1B[0C");
	}

}
 

void CmdLineBuffer::processBuffer(){
	
	removeCompletions();
	
	string cmdString((char*)_dbuf.data, _dbuf.used);
	cmdString = Utils::trimEnd(cmdString);
	cmdString = Utils::trimStart(cmdString);
	
	// are we using a history command?
	if(_historyIndex  < _history.size()){
		cmdString = _history.at(_historyIndex);
	}
	_historyIndex = ULONG_MAX;
	
	// ignore empty lines
	if(cmdString.size() == 0){
		sendReply("\r\n" + STR_PROMPT);
		return;
	}
	
	clear();  // clear the buffer

	// pass it back to the CmdLineMgr
	if(	_clMgr->processCommandLine(cmdString, [=](bool didSucceed){
		// if the buffer has data from history  - dont prompt
		if(_dbuf.used == 0)
			sendReply(STR_PROMPT);
	})){
		//  storing in history if cmdcallback return true;
		storeCmdInHistory(cmdString);
	}
}

// MARK: - wrapper to CmdLineMgr

void CmdLineBuffer::sendReply(std::string str){
	_clMgr->sendReply(str);
  }

// MARK: - completion


void CmdLineBuffer::removeCompletions(){
	if(_displayingCompletionOptions){
		
		std::ostringstream oss;
		
		for(int i = 0; i < _displayingCompletionOptions; i++){
			sendReply("\n\r\x1B[2K");
		}
		// go up linecount lines
		oss << "\x1B[" << _displayingCompletionOptions << "A" << "\r\x1B[" <<  (_dbuf.pos + 2)  << "C";
		
		_displayingCompletionOptions = 0;
		
		sendReply(oss.str());
	}
}

void CmdLineBuffer::displayCompletions(){
	
	std::vector<std::string> options;
	
	if(	_displayingCompletionOptions > 0 && _dbuf.used == 0){
		removeCompletions();
		return;
	}

	removeCompletions();
	
	options = _clMgr->matchesForCmd(string((char*)_dbuf.data, _dbuf.used));
	
	if(options.size() == 1){
		// if we have only one -- put it in the buffer
		
		clear();
		dbuf_set(options.front());

		sendReply("\r\x1B[2K");
		sendReply(STR_PROMPT);
		sendReply(string((char*)_dbuf.data, _dbuf.used));
	}
	else if(options.size() > 1) {
		
		// give the user a list of completions
		std::ostringstream oss;
		 
		// can we extend the choice..
		string prefix = findCommonPrefix(options);
		
		// if we got a longer match  -- put it in the buffer
		if(prefix.length() > _dbuf.used){
			
			clear();
			dbuf_set(prefix);

			sendReply("\r\x1B[2K");
			sendReply(STR_PROMPT);
			sendReply(string((char*)_dbuf.data, _dbuf.used));
			displayCompletions();
			return;
		}
		
		oss << "\r\n";
		
		int colwidth = 18;
		int colcount = _termWidth / (colwidth + 1);
		
		int linecount = 1;
		for (auto it = options.begin(); it != options.end(); it++) {
			auto offset = std::distance(options.begin(), it);
			oss << setw(colwidth) << left << *it << setw(0) << " ";
			if((offset % colcount) == colcount-1){
				oss << "\r\n";
				linecount++;
			}
		}
		
		// go up linecount lines
		oss << "\x1B[" << linecount << "A" << "\r\x1B[" <<  (_dbuf.pos + 2)  << "C";
		
		_displayingCompletionOptions = linecount;
		
		sendReply(oss.str());
	}
}


std::string CmdLineBuffer::findCommonPrefix(std::vector<std::string> options){
	
	string prefix;
	
	string test = options.front();
	
	if(options.size() == 1)
		return test;
	
	size_t maxLen = test.size();
	size_t matchLen = 1;
	
	for(matchLen  = 1; matchLen < maxLen; matchLen++){
		bool match = true;
		
		for(auto str :options){
			if(str.find(test.c_str(), 0,matchLen) != 0){
				match = false;
				matchLen = matchLen-1;
				break;
			}
		};
		
		if(!match) break;
	}
	
	prefix = test.substr(0, matchLen);
	
	return prefix;
};

// MARK: - History file

void CmdLineBuffer::displayHistoryLine() {
	
	string cmdString;
	
	if(_historyIndex < _history.size()){
		cmdString = _history.at(_historyIndex);
	}
	else {
		cmdString = string((char*)_dbuf.data, _dbuf.used);
	}
 
	dbuf_set(cmdString);

	sendReply(string("\r\x1B[2K") + STR_PROMPT + cmdString);
}


void CmdLineBuffer::maybeCopyHistoryToBuffer(){
	// are we modifying a history line  -- if so make that our buffer
	
 	if(_historyIndex  < _history.size()){
		string cmdString = _history.at(_historyIndex);
		dbuf_set(cmdString);
		_historyIndex = ULONG_MAX;
	}
}

void CmdLineBuffer::doCmdHistory(vector<string> params){
	
	std::ostringstream oss;
	
	if(params.size() == 1){
		for (auto it = _history.begin(); it != _history.end(); it++) {
			auto offset = std::distance(_history.begin(), it);
			oss <<  setw(4) << offset << setw(0) << " " << *it << "\r\n";
		}
		
		sendReply(oss.str());
		clear();
	}
	else {
		auto param1 = params.at(1);
		
		if(param1 == "-c"){
			// clear history
			_history.clear();
			saveHistoryFile();
			clear();
		}
		else if(std::regex_match(param1, std::regex("^[-]?[0-9]+$"))){
			int offset = atoi(param1.c_str());
			if(offset < 0){
				_historyIndex = _historyIndex - offset;
			}
			else {
				_historyIndex = offset;
			}
			
			if(_historyIndex < 0)
				_historyIndex = 0;
			
			if(_historyIndex > _history.size())
				_historyIndex = _history.size() -1;
			
			displayHistoryLine();
			return;
		}
		
	}
}

void CmdLineBuffer::storeCmdInHistory(const std::string cmd) {
	
	if(_history.size() == 0){
		_history.push_back(cmd);
		saveHistoryFile();
	}
	else if(cmd != _history.back()){
		_history.push_back(cmd);
		saveHistoryFile();
	}
	
}

bool CmdLineBuffer::saveHistoryFile() {
	
	string fileName = _fileName.size() == 0?"history":_fileName;
	string path = _directoryPath + fileName;
	
	std::ofstream ofs(path);
	if (!ofs)
		return false;
 
		for(auto line :_history){
		
			ofs << line << endl;
		}
 
	ofs.close();
	return false;
}

bool CmdLineBuffer::readHistoryFile(){
 
	string fileName = _fileName.size() == 0?"history":_fileName;
	string path = _directoryPath + fileName;
	 
	std::ifstream ifs(path);
	if (!ifs)
		return false;
 
	std::vector<std::string>	history;
	history.clear();

	std::string line;
	while ( std::getline(ifs, line) )
	{
		history.push_back(line);
	}
	
	ifs.close();
	_history = history;

	return true;
}


//MARK: - buffer management

void CmdLineBuffer::dbuff_init(){
	
	_dbuf.used = 0;
	_dbuf.pos = 0;
	_dbuf.alloc = ALLOC_QUANTUM;
	_dbuf.data =  (uint8_t*) malloc(ALLOC_QUANTUM);
}
 

bool CmdLineBuffer::dbuff_cmp(const char* str){
	if(!str) return
		false;
	
	size_t len =  strlen(str);
	
	if(_dbuf.used < len)
		return false;
	
	return (memcmp(_dbuf.data, str, len) == 0);
};


bool CmdLineBuffer::dbuff_cmp_end(const char* str){
	if(!str) return
		false;
	
	size_t len =  strlen(str);
	
	if(_dbuf.used < len)
		return false;
	
	return (memcmp(_dbuf.data+_dbuf.used-len, str, len) == 0);
};


size_t CmdLineBuffer::data_size(){
  return _dbuf.used;
};



void CmdLineBuffer::dbuf_set(std::string str){
	_dbuf.pos = 0;
	_dbuf.used = 0;
	append_data((char*)str.c_str(), str.size());
}

bool CmdLineBuffer::append_data(void* data, size_t len){
  
  if(len + _dbuf.pos  >  _dbuf.alloc){
	  size_t newSize = len + _dbuf.used + ALLOC_QUANTUM;
	  
	  if( (_dbuf.data = (uint8_t*) realloc(_dbuf.data,newSize)) == NULL)
		  return false;
	  
	  _dbuf.alloc = newSize;
  }
  
	if(_dbuf.pos < _dbuf.used) {
		memmove((void*) (_dbuf.data + _dbuf.pos + len) ,
				  (_dbuf.data + _dbuf.pos),
				  _dbuf.used -_dbuf.pos);
	}
	
  memcpy((void*) (_dbuf.data + _dbuf.pos), data, len);
	
	_dbuf.pos += len;
	_dbuf.used += len;
//	if(_dbuf.pos > _dbuf.used)
//		_dbuf.used = _dbuf.pos;
	
  return true;
}

bool CmdLineBuffer::append_char(uint8_t c){
  return append_data(&c, 1);
}

void CmdLineBuffer::delete_char(){
	if(_dbuf.pos > 0){
		if(_dbuf.used > 0) {
			
			if(_dbuf.pos < _dbuf.used) {
				size_t bytesToMove = _dbuf.used - _dbuf.pos;
				memmove((void*) (_dbuf.data + _dbuf.pos-1) , (_dbuf.data + _dbuf.pos), bytesToMove);
			}
			_dbuf.pos--;
			_dbuf.used--;
		}
	}
}

