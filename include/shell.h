#ifndef SHELL_H
#define SHELL_H

#include <linux/limits.h>
#include <sys/types.h>

#define LOG_SIZE 15
#define MAX_JOBS 128

typedef enum { JOB_NONE = 0, JOB_RUNNING = 1, JOB_STOPPED = 2 } job_state;

typedef struct bg_job {
    pid_t pid;
    pid_t pgid;
    int id;
    char cmd[1024];
    int active;
    job_state state;
} bg_job;

typedef struct shell_state {
    char home[PATH_MAX];
    char prev[PATH_MAX];
    char log[LOG_SIZE][1024];
    int log_count;
    bg_job jobs[MAX_JOBS];
    int next_job_id;
} shell_state;

// void shell_exec_line(shell_state *st, const char *line);

#endif
