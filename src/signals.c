#include "../include/posix_lib.h"
#include "../include/signals.h"
#include "../include/shell.h"
#include "../include/jobs.h"

#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>

static volatile sig_atomic_t fg_pgid = -1;
static char fg_name[256] = {0};
extern shell_state global_shell_state;

static volatile sig_atomic_t pending_tstp = 0;
static volatile sig_atomic_t pending_tstp_pgid = -1;

static void on_sigint(int sig) {
    (void)sig;
    pid_t pg = fg_pgid;
    if (pg > 0) {
        kill(-pg, SIGINT); 
    }
}

static void on_sigtstp(int sig) {
    (void)sig;
    pid_t pg = fg_pgid;
    if (pg > 0) {
        kill(-pg, SIGTSTP);          
        pending_tstp_pgid = pg;      
        pending_tstp = 1;           
    }
}
/*
struct sigaction {
    void (*sa_handler)(int); // pointer to the signal handling fn 
    // sigset_t sa_mask; 
    int sa_flags; // set of signals that will be temporarily blocked while the handler is running.
    void (*sa_sigaction)(int, siginfo_t *, void *);
};
*/

void signals_init() {
    struct sigaction sa_int, sa_tstp;

    memset(&sa_int, 0, sizeof(sa_int));
    memset(&sa_tstp, 0, sizeof(sa_tstp));

    sa_int.sa_handler = on_sigint;
    sa_tstp.sa_handler = on_sigtstp;

    sigemptyset(&sa_int.sa_mask);
    sigemptyset(&sa_tstp.sa_mask);

    sa_int.sa_flags = 0;
    sa_tstp.sa_flags = 0;

    sigaction(SIGINT, &sa_int, NULL);
    sigaction(SIGTSTP, &sa_tstp, NULL);

    signal(SIGQUIT, SIG_IGN);
}

void sig_init() {
    // these are custom signal handlers declaration 
    struct sigaction sig_int, sig_tstp, sig_quit; 

    // these might contain garbage values so let's clean them 

    memset(&sig_int, 0, sizeof(sig_int)) ;
    memset(&sig_tstp, 0, sizeof(sig_tstp)) ;
    memset(&sig_quit, 0, sizeof(sig_quit)) ;

}

void signals_set_fg_pgid(pid_t pgid, const char *cmd) {
    fg_pgid = pgid;
    if (pgid < 0) {
        fg_name[0] = '\0';
        return;
    }
    if (cmd) {
        strncpy(fg_name, cmd, sizeof(fg_name) - 1);
        fg_name[sizeof(fg_name) - 1] = '\0';
    } else {
        fg_name[0] = '\0';
    }
}

pid_t signals_get_fg_pgid(void) { return fg_pgid; }
const char *signals_get_fg_name(void) { return fg_name; }

void signals_handle_pending(void) {
    if (!pending_tstp) return;

    pid_t pg = (pid_t) pending_tstp_pgid;
    pending_tstp = 0;
    pending_tstp_pgid = -1;

    if (pg > 0) {
        int id = jobs_add(&global_shell_state, pg, fg_name);
        if (id > 0) {
            for (int i = 0; i < MAX_JOBS; i++) {
                if (global_shell_state.jobs[i].active && global_shell_state.jobs[i].id == id) {
                    global_shell_state.jobs[i].state = JOB_STOPPED;
                    global_shell_state.jobs[i].pid = pg;
                    break;
                }
            }
            printf("[%d] Stopped %s with pid %d\n", id, fg_name, (int) pg);
            fflush(stdout);
        }
    }
}
