//
//  CmdLineHelp.hpp
//  FOOServer
//
//  Created by Vincent Moscaritolo on 5/13/21.
//

#ifndef CmdLineHelp_hpp
#define CmdLineHelp_hpp
#include <string>
#include <map>
#include <utility>

using namespace std;

#include <stdio.h>

class CmdLineHelp {
	
public:
	
	static CmdLineHelp *shared() {
		if (!sharedInstance)
			sharedInstance = new CmdLineHelp;
		return sharedInstance;
	}
	
	static CmdLineHelp *sharedInstance;
	
	CmdLineHelp();
	~CmdLineHelp();
	
	bool setHelpFile(const string path );

	string helpForCmd( string cmd);

private:
	
	bool loadHelpFile();
	string helpForTopic(string topic);

	string _helpFilePath;
	
	std::map < std::string, std::pair<size_t,size_t>> _topics;
};

#endif /* CmdLineHelp_hpp */
