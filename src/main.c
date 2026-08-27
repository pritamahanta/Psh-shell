#include "../include/main.h"
#include "../include/runner.h"
#include "../include/parser.h"
#include "../include/signals.h"
#include "../include/jobs.h"
#include "../include/prompt.h"  
#include "../include/shell.h"
#include "../include/history.h"
#include "../include/input.h"

#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<signal.h>

shell_state global_shell_state ;

static void kill_all(shell_state *st) {
    input_disable_raw();
    pid_t g = signals_get_fg_pgid();
    if (g > 0) kill(-g, SIGKILL);

    for (int i = 0; i < MAX_JOBS; i++) {
        if (st -> jobs[i].active ) {
            kill(-st -> jobs[i].pgid, SIGKILL);
        }
    }
}

void shell_loop() {
    // printf("Welcome to Psh shell!\n") ;

    char *line = NULL;
    // size_t cap = 0 ;

    for(;;) {
        

        // update each jobs state before showing prompt 
        jobs_check(&global_shell_state) ;
        show_prompt(&global_shell_state);
        fflush(stdout) ;

        char *raw = input_read_line(&global_shell_state);
        if (!raw) {
            printf("logout\n");
            kill_all(&global_shell_state);
            break;
        }
        if (!line) line = malloc(2048);
        strncpy(line, raw, 2047);
        line[2047] = '\0';
        // ssize_t n = strlen(line);

        // again job check as a job might have chanaged it's state while user typing 
        jobs_check(&global_shell_state) ;
        if(line[0] == '\0') continue ;

        history_add_if_needed(&global_shell_state, line) ;


        // converting && to ; 
        char norm[2048];
        norm_and_and(line, norm, sizeof(norm)) ;    
        
        if(!parse_shell_cmd(norm)) {
            fprintf(stderr, "Syntax error\n");
            continue ;
        } 
        run_sequence(norm) ;
        jobs_check(&global_shell_state) ;
    }
    free(line) ;
}

int main() {
    // printf("Starting Psh shell...\n") ;
    atexit(input_disable_raw); 
    init_prompt(&global_shell_state);

    global_shell_state.prev[0] = '\0';
    global_shell_state.log_count = 0;

    jobs_init(&global_shell_state);
    signals_init() ;
    history_load(&global_shell_state); 


    // A background process tries to read from terminal -> kernel will suspend that process 
    signal(SIGTTOU, SIG_IGN);

    // A background process tries to write to terminal -> kernel will suspend that process 
    signal(SIGTTIN, SIG_IGN);

    // creates its own process group
    pid_t shell_pgid = getpid();
    
    // Ensures the shell becomes a process group leader 
    // it isolates from any process group created by the shell
    // shell becomes it's own leader
    setpgid(shell_pgid, shell_pgid);


    // shell becomes the owner of terminal / foreground
    tcsetpgrp(STDIN_FILENO, shell_pgid);
    shell_loop();

    return 0;
}


// Everything is a file.