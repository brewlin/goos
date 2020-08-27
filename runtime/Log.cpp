#include "Log.h"
#include <thread>

char debug[DEBUG_MSG_SIZE];
char trace[TRACE_MSG_SIZE];
char warn[WARN_MSG_SIZE];
char error[ERROR_MSG_SIZE];

void Log::put(int level, char *cnt)
{
    const char *level_str;
    char date_str[LOG_DATE_STRLEN];
    char log_str[LOG_BUFFER_SIZE];
    int n;
    time_t t;
    struct tm *p;

    switch (level) {
        case LOG_DEBUG_D:
            level_str = "DEBUG";
            break;
        case LOG_NOTICE_D:
            level_str = "NOTICE";
            break;
        case LOG_ERROR_D:
            level_str = "ERROR";
            break;
        case LOG_WARNING_D:
            level_str = "WARNING";
            break;
        case LOG_TRACE_D:
            level_str = "TRACE";
            break;
        default:
            level_str = "INFO";
            break;
    }

    t = time(NULL);
    p = localtime(&t);
    snprintf(date_str, LOG_DATE_STRLEN, "%d-%02d-%02d %02d:%02d:%02d", p->tm_year + 1900, p->tm_mon + 1, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
    pthread_t pid = pthread_self();
//    n = snprintf(log_str, LOG_BUFFER_SIZE, "[%s][%ld]\t%s\t%s\n", date_str,pid, level_str, cnt);
    n = snprintf(log_str, LOG_BUFFER_SIZE, "[tid:%x]\t%s\t%s\n", pid, level_str, cnt);
    if (write(STDOUT_FILENO, log_str, n) < 0) {
        printf("write(log_fd, size=%d) failed. Error: %s[%d].\n", n, strerror(errno), errno);
    }
}