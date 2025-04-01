#ifndef DEBUG_HPP
#define DEBUG_HPP


#include "config.hpp"

// #define DEBUG
#ifdef DEBUG 

#define DEBUG_STMT(stmt) stmt

#else

#define DEBUG_STMT(stmt) {}

#endif 

#endif
