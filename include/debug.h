#ifndef DEBUG_H
#define DEBUG_H

extern int DEBUG_LEVEL;

#define DBG(level) if(DEBUG_LEVEL >= (level))

#endif
