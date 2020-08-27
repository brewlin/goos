#ifndef GOOS_LOG_H
#define GOOS_LOG_H

#include <errno.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>
#include "config.h"

#define LOG_BUFFER_SIZE 1024
#define LOG_DATE_STRLEN  64

#define DEBUG_MSG_SIZE 512
#define TRACE_MSG_SIZE 512
#define WARN_MSG_SIZE 512
#define ERROR_MSG_SIZE 512

extern char debug[DEBUG_MSG_SIZE];
extern char trace[TRACE_MSG_SIZE];
extern char warn[WARN_MSG_SIZE];
extern char error[ERROR_MSG_SIZE];
#ifdef GO_DEBUG
    #define Debug(str, ...)                                                         \
        snprintf(debug, DEBUG_MSG_SIZE, "%s: " str " in %s on line %d.", __func__, ##__VA_ARGS__, __FILE__, __LINE__); \
        Log::put(LOG_DEBUG_D, debug);

    #define Trace(str, ...)                                                         \
        snprintf(trace, TRACE_MSG_SIZE, "%s: " str " in %s on line %d.", __func__, ##__VA_ARGS__, __FILE__, __LINE__); \
        Log::put(LOG_TRACE_D, trace);

    #define Warn(str, ...)                                                         \
        snprintf(error, ERROR_MSG_SIZE, "%s: " str " in %s on line %d.", __func__, ##__VA_ARGS__, __FILE__, __LINE__); \
        Log::put(LOG_WARNING_D, error);

    #define Error(str, ...)                                                         \
        {\
        snprintf(error, ERROR_MSG_SIZE, "%s: " str " in %s on line %d.", __func__, ##__VA_ARGS__, __FILE__, __LINE__); \
        Log::put(LOG_ERROR_D, error); \
        exit(-1);\
        }
#else
    #define Debug(str,...)
    #define Trace(str,...)
    #define Warn(str,...)
    #define Error(str,...)
#endif


enum Log_define{
    LOG_DEBUG_D,
    LOG_TRACE_D,
    LOG_INFO_D,
    LOG_NOTICE_D,
    LOG_WARNING_D,
    LOG_ERROR_D,
};

class Log{
public:
    static void put(int level,char *cnt);
};


#endif //GOOS_LOG_H
