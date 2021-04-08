//
//  CommonDefs.h
//  InsteonServer
//
//  Created by Vincent Moscaritolo on 3/19/21.
//

#ifndef CommonDefs_h
#define CommonDefs_h

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <functional>
#include <vector>
#include <string>
 

typedef std::function<void(bool didSucceed)> boolCallback_t;
typedef std::function<void()> voidCallback_t;
typedef std::vector<std::string> stringvector;


#endif /* CommonDefs_h */
