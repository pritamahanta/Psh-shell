#include "../include/execute.h"
#include "../include/builtins.h"
#include "../include/helpers.h"
#include "../include/signals.h"

#include<unistd.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<sys/wait.h>
#include<fcntl.h>
#include<ctype.h>

// testing
#include <time.h>

#ifndef MAX_INFILES
#  define MAX_INFILES   8
#endif

#ifndef MAX_OUTFILES
#  define MAX_OUTFILES  8
#endif


// test

#ifdef PSH_BENCH
static long long now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (long long)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}
#endif



static void expand_vars(char *cmd, char *out, size_t sz) {
    char *w = out;
    char *end = out + sz - 1;

    while (*cmd && w < end) {
        if (*cmd == '$') {
            cmd++;
            char var[128];
            int vi = 0;
            while (*cmd && (isalnum((unsigned char)*cmd) || *cmd == '_') && vi < 127)
                var[vi++] = *cmd++;
            var[vi] = '\0';

            if (vi > 0) {
                const char *val = getenv(var);
                if (val) {
                    while (*val && w < end) *w++ = *val++;
                }
            } else {
                *w++ = '$'; 
            }
        } else {
            *w++ = *cmd++;
        }
    }
    *w = '\0';
}

static void trim(char *s) {
    int n = (int)strlen(s) ;
    while(n && (s[n - 1] == ' ' || s[n - 1] == '\t')) s[--n] = '\0' ;
    while (*s == ' '|| *s == '\t') memmove(s, s + 1, strlen(s) + 1 ) ;
}

static int split_argv(char *cmd, char **argv, int max) {
    int argc = 0 ;
    
    while(*cmd && argc < max) {
        while(*cmd == ' ' || *cmd == '\t') cmd++ ;
        if(*cmd == '\0' || argc >= max - 1) break ;

        if(*cmd == '\'' || *cmd == '"') {
            char quote = *cmd++ ;
            argv[argc++] = cmd ;
            while(*cmd && *cmd != quote) cmd++ ;
            if(*cmd) *cmd++ = '\0' ;
        }
        else {
            argv[argc++] = cmd ;
            while(*cmd && *cmd != ' ' && *cmd != '\t') cmd++ ;
            if(*cmd) *cmd++ = '\0' ;
        }
    }
    argv[argc] = NULL ;
    return argc ;
}

static pid_t run_single(char *cmd, int in_fd, int out_fd, int wait_fg, int bg_detach_stdin, pid_t pg_lead) {

    char *infiles[MAX_INFILES] ; int n_in = 0 ;
    char *outfiles[MAX_OUTFILES] ; int n_out = 0 ;
    int append [MAX_OUTFILES] ;

    for(char *p = cmd ; (p = strpbrk(p, "<>") ) ; ) {
        if(*p == '<') {
            *p++ = '\0' ;
            while(*p == ' ' || *p == '\t') p++ ;
            if(*p == '\0' || n_in >= MAX_INFILES) continue ;
            infiles[n_in++] = strtok(p, " \t");
        }
        else if(*p == '>' && *(p + 1) == '>') {
            *p = '\0' ; p += 2;
            while(*p == ' ' || *p == '\t') p++ ;
            if(*p == '\0' || n_out >= MAX_OUTFILES) continue ;
            outfiles[n_out] =  strtok(p, " \t"); append[n_out++] = 1 ;
        }
        else {
            *p++ = '\0' ; 
            while(*p == ' ' || *p == '\t') p++ ;
            if(*p == '\0' || n_out >= MAX_OUTFILES) continue ;
            outfiles[n_out] =  strtok(p, " \t"); append[n_out++] = 0 ;
        }
    }
    trim(cmd) ;
    if(cmd[0] == '\0') return -1 ;

    char expanded[2048];
    expand_vars(cmd, expanded, sizeof(expanded));

    char *argv[64] ;
    int argc = split_argv(expanded, argv, 64) ;

    if (argc == 0 || argv[0] == NULL) {
        _exit(0);
    }

    // test 

    #ifdef PSH_BENCH
        long long start_ns = now_ns();  
    #endif
    //

    pid_t pid = fork() ;

    if( pid == 0 ) { 

        if(pg_lead > 0) setpgid(0, pg_lead) ;
        else setpgid(0, 0) ;

        int cur_in = -1 ;
        for(int i = 0 ; i < n_in ; i++) {
            int fd = open(infiles[i], O_RDONLY) ;
            if(cur_in >= 0) close(cur_in) ;
            if(fd < 0) {
                perror(infiles[i]) ;
                exit(1) ;
            }
            cur_in = fd ;
        }
        if(cur_in >= 0) {
            dup2(cur_in, STDIN_FILENO) ;
            close(cur_in) ;
        }
        else if(in_fd != STDIN_FILENO) {
            dup2(in_fd, STDIN_FILENO) ;
            if(bg_detach_stdin) close(in_fd) ;
        } 
        else if(!wait_fg && bg_detach_stdin) {
            int devnull = open("/dev/null", O_RDONLY) ;
            if(devnull >= 0) {
                dup2(devnull, STDIN_FILENO) ;
                close(devnull) ;
            }
        }

        int cur_out = -1 ;
        for(int i = 0 ; i < n_out ; i++) {
            int fd = open(outfiles[i], O_WRONLY | O_CREAT | (append[i] ? O_APPEND : O_TRUNC), 0666) ;
            if(fd < 0) {
                printf("Unable to create file for writing\n");
                _exit(1);
            }
            if(cur_out >= 0) close(cur_out) ;
            cur_out = fd ;
        }
        if(cur_out >= 0) {
            dup2(cur_out, STDOUT_FILENO) ;
            close(cur_out) ;
        }
        else if(out_fd != STDOUT_FILENO) {
            dup2(out_fd, STDOUT_FILENO) ;
            close(out_fd) ;
        }
        // if (strcmp(argv[0], "echo") == 0)  { command_echo(argv, NULL); _exit(0); }
        // if (strcmp(argv[0], "pwd") == 0)  { command_pwd(); _exit(0); }
        // if (strcmp(argv[0], "env") == 0)  { command_env(NULL); _exit(0); }
        // if (strcmp(argv[0], "which") == 0)  { command_which(argv, NULL); _exit(0); }
        // printf("Executing command in child: %s\n", argv[0]) ;
        execvp(argv[0], argv);
        printf("Command not found!\n");
        _exit(1);
    }
    if(pg_lead > 0) setpgid(pid, pg_lead) ;
    else setpgid(pid, pid) ;

    if(wait_fg) {
        pid_t grp = (pg_lead > 0 ? pg_lead : pid);

        
        // job test 
        #ifdef PSH_BENCH
            long long jc_setup_start = now_ns();
        #endif
        //


        setpgid(pid, grp);
        signals_set_fg_pgid(grp, argv[0] ? argv[0] : "");
        tcsetpgrp(STDIN_FILENO, grp);

        // job test
        #ifdef PSH_BENCH
            long long jc_setup_end = now_ns();
                fprintf(stderr,
                    "PSH_JC_SETUP_NS %lld\n",
                    jc_setup_end - jc_setup_start);
        #endif
        //
        
        int status;
        waitpid(pid, &status, WUNTRACED);

        // external test
        #ifdef PSH_BENCH
            long long end_ns = now_ns();
            fprintf(stderr, "PSH_EXEC_NS %lld\n", end_ns - start_ns);
            long long jc_teardown_start = now_ns();
        #endif
        // 

        if (WIFSIGNALED(status) && WTERMSIG(status) == SIGINT) {
            write(STDOUT_FILENO, "\n", 1);  // clean newline after ^C
        }
        // signals_handle_pending must come BEFORE tcsetpgrp
        signals_handle_pending();
        tcsetpgrp(STDIN_FILENO, getpgrp());
        signals_set_fg_pgid(-1, NULL);

        // job test 
        #ifdef PSH_BENCH
            long long jc_teardown_end = now_ns();

            fprintf(stderr,
            "PSH_JC_TEARDOWN_NS %lld\n",
            jc_teardown_end - jc_teardown_start);
        #endif
        //
    }
    return pid ;
}

int execute_command(char *line, int wait_fg, pid_t  *first_pid) {

    // printf("Executing command line: %s\n", line) ;
    char* parts[16] ;
    int cnt = 0 ;
    char *save_ptr ;
    char *token = strtok_r(line, "|", &save_ptr) ;

    while (token && cnt < 16) {
        parts[cnt++] = token ;
        token = strtok_r(NULL, "|", &save_ptr) ;
    }
    
    pid_t pg = -1 ;
    pid_t first = -1 ;

    if(cnt == 1) {
        pid_t p = run_single(parts[0], STDIN_FILENO, STDOUT_FILENO, wait_fg, !wait_fg, -1) ;
        if(first_pid) *first_pid = p ;
    }
    else {
        
        // {read_end, right_end}
        int fds[2] = {-1, -1} ;
        int in_fd = STDIN_FILENO ;

        for(int i = 0 ; i < cnt ; i++) {
            if(i + 1 < cnt) {
                pipe(fds) ;
            }
            pid_t p = run_single(parts[i], in_fd, (i == cnt - 1) ? STDOUT_FILENO : fds[1], 0, !wait_fg, pg);
            if(p > 0 && pg == -1) pg = p ;
            if(!i) first = p ;
            if(i + 1 < cnt ) {
                close(fds[1]) ;
                if(in_fd != STDIN_FILENO) close(in_fd) ;
                in_fd = fds[0] ;
            } 
        }
        // if (wait_fg) { tcsetpgrp(STDIN_FILENO, getpgrp()); signals_set_fg_pgid(-1,NULL); }
        if (wait_fg && pg > 0) {
            pid_t grp = pg;
            signals_set_fg_pgid(grp, parts[0]);   
            tcsetpgrp(STDIN_FILENO, grp);     

            int status = 0 ;
    
            while (waitpid(-pg, &status, WUNTRACED) > 0) {
                if (WIFSTOPPED(status)) break;      
            }
            if (WIFSIGNALED(status) && WTERMSIG(status) == SIGINT) {
                write(STDOUT_FILENO, "\n", 1);
            }
            signals_handle_pending();
            tcsetpgrp(STDIN_FILENO, getpgrp());
            signals_set_fg_pgid(-1, NULL);
        }
        if (first_pid) *first_pid = first;
    }
    return 0 ;
} 
