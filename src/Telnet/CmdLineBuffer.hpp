//
//  CmdLineBuffer.hpp

//
//  Created by Vincent Moscaritolo on 3/14/21.
//

#ifndef CmdLineBuffer_hpp
#define CmdLineBuffer_hpp

#include <stdio.h>
#include <stdint.h>
#include <string>
#include <functional>
#include <vector>
#include <netinet/in.h>
#include "CommonDefs.hpp"

class CmdLineMgr;

class  CmdLineBufferManager {
 
public:
	virtual ~CmdLineBufferManager() {};
	virtual void sendReply(const std::string) = 0;
	virtual bool processCommandLine(std::string cmdLine, boolCallback_t completion) = 0;
	virtual stringvector matchesForCmd(const std::string cmd) = 0;
	virtual void helpForCommandLine(std::string cmdLine, boolCallback_t cb) = 0;
};

 class CmdLineBuffer {
	
	 friend class CmdLineMgr;

public:
	CmdLineBuffer( CmdLineBufferManager * bufMgr);
	~CmdLineBuffer();
	
	void didConnect();
	void didDisconnect();

	void processChars(uint8_t* data, size_t len);
	void clear();
	void setTermSize(uint16_t	 width, uint16_t height);
 
	// for history file
	void 	setDirectoryPath(std::string path) { _directoryPath = path;};
	std::string getDirectoryPath() { return _directoryPath;};
	void setHistoryFileName(std::string fileName = "") {_fileName = fileName; };
	bool readHistoryFile();

	// built in commands
	void doCmdHistory(stringvector params);

 protected:
	 
	 void waitForChar( std::vector<char> chs,
							std::function<void(bool didSucceed, char ch)> callback = NULL);
	 
 private:
	
	typedef enum  {
		CLB_UNKNOWN = 0,
		CLB_READY,
		CLB_ESC,
		CLB_ESC1
	}clb_state_t;

	static const uint8_t CHAR_PRINTABLE = 0x1F;
	static const uint8_t CHAR_LF        = 0x0A;
	static const uint8_t CHAR_CR        = 0x0D;
	static const uint8_t CHAR_BS        = 0x08;
	static const uint8_t CHAR_TAB       = 0x09;
	static const uint8_t CHAR_DEL       = 0x7F;
	static const uint8_t CHAR_ESC     	 = 0x1B;
	static const uint8_t CHAR_QUESTION   = '?';

	static const uint8_t CHAR_CNTL_A     	 = 0x01;		// begining of line
	static const uint8_t CHAR_CNTL_E     	 = 0x05;		// end of line
	static const uint8_t CHAR_CNTL_K     	 = 0x0B;		// delete till end
	static const uint8_t CHAR_CNTL_U     	 = 0x15;		// delete line

	const std::string 	 STR_PROMPT = "\x1B[96m> \x1B[0m";

 	CmdLineBufferManager*		_clMgr;
	clb_state_t					_state;
	stringvector					_history;
	std::string 					_directoryPath;
	std::string 					_fileName;
	size_t							_historyIndex; 	// used for scrollback
	int								_displayingCompletionOptions;  // how man line of completion
	uint16_t	 					_termWidth = 80;
	
	void processChar(uint8_t ch);
	 
	void sendReply(std::string str);
	
	void processBuffer();

	void handleUpArrow();
	void handleDownArrow();
	void handleBackArrow();
	void handleFwdArrow();
	void handleClearLine();
	void handleDeleteChar();
	void handleTabChar();
	void handleBeginingOfLine();
	void handleEndOfLine();
	void handleClearToEndOfLine();
	void handleHelp();

	void displayHistoryLine();

	void displayCompletions();
	void removeCompletions();
	
	//bool maybeHandleCommand(const std::string);
	std::string findCommonPrefix(stringvector);
	
	bool saveHistoryFile();
	void storeCmdInHistory(const std::string);
	void maybeCopyHistoryToBuffer();
	
	void dbuff_init();
	bool dbuff_cmp(const char* str);
	bool dbuff_cmp_end(const char* str);
	
	void dbuf_set(std::string);

	bool append_data(void* data, size_t len);
	bool append_char(uint8_t c);
	void delete_char();
	size_t data_size();
	
	std::vector<char> _waitForChar;
	std::function<void(bool didSucceed, char ch)> _waitForCharCB;
	 
	struct  {
		size_t  	pos;			// cursor pos
		size_t  	used;			// actual end of buffer
		size_t  	alloc;
		uint8_t*  data;
		
	} _dbuf;
	
};

#endif /* CmdLineBuffer_hpp */
