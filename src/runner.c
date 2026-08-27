#include "../include/builtins.h"
#include "../include/helpers.h"
#include "../include/runner.h"
#include "../include/execute.h"

#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/wait.h>


static int split_words(char *line, char **args) {
    int n = 0; char *tok = strtok(line, " \t");
    while (tok && n < 64) { args[n++] = tok; tok = strtok(NULL, " \t"); }
    args[n] = NULL ;
    return n;
}

void norm_and_and(const char *in, char *out, size_t sz){

    size_t j = 0;
    for(size_t i = 0; in[i] && j + 1 < sz; i++, j++) {
        if(in[i] == '&' && in[i + 1] == '&') {
            out[j] = ';';
            i += 1 ;
        } else {
            out[j] = in[i];
        }
    }
    out[j] = '\0';
}

int run_sequence(const char *line) {
    
    char buf[2048] ;
    strncpy(buf, line, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    int n = (int)strlen(buf) ;

    char *cmd[128] ;
    char bg[128] = {0} ;
    int cnt = 0 , st = 0 ;

    for(int i = 0 ; i < n; i++) {
        if(buf[i] == ';' || buf[i] == '&') {
            bg[cnt] = (buf[i] == '&') ;
            buf[i] = '\0' ;
            cmd[cnt++] = buf + st ;
            st = i + 1 ;
        }
    }
    if(st <= n) {
        cmd[cnt] = buf + st ;
        bg[cnt++] = 0 ;
    }
    for(int i = 0 ; i < cnt; i++) {
        char temp[1024] ;
        strncpy(temp, cmd[i], sizeof(temp) - 1) ;
        temp[sizeof(temp) - 1] = '\0' ;

        if(!temp[0]) continue ;
        char *s = temp ;

        for(; *s == ' ' || *s == '\t'; s++) {}
        if(*s == '\0') continue ;

        char name[1024] ;
        my_strncpy(name, s, sizeof(name) - 1) ;
        name[sizeof(name) - 1] = '\0' ;

        char *first = strtok(name, " \t|&><;") ;
        if(!first) continue ;

        int hard = !!(strchr(first, '|') || strchr(first, '>') || strchr(first, '<') || strchr(first, ';') );

        const char* built_in_commands[] = {"cd", "pwd", "echo", "env", "setenv", "unsetenv", "which", "exit"} ;
        // (void) bg ;
        // printf("Command to run: %s\n", s) ;
        if(!hard) {
            int is_builtin = 0 ;   

            for(int j = 0 ; j < 8 ; j++) {       
                if(!strcmp(first, built_in_commands[j])) {

                is_builtin = 1;

                char temp[1024]; 
                my_strncpy(temp, s, sizeof(temp) - 1 );
                temp[sizeof(temp) - 1 ] = '\0'; 

                char *args[64]; 
                int argc = split_words(temp, args);

                if (argc == 0) continue;
                // printf("Running built-in command: %s\n", args[0]) ;
                    switch(j) {
                        case 0: command_cd(args) ; break ;
                        case 1: command_pwd() ; break ;
                        case 2: command_echo(args, NULL) ; break ;
                        case 3: command_env(NULL) ; break ;
                        case 4: command_setenv(args) ; break ;
                        case 5: command_unsetenv(args) ; break ;
                        case 6: command_which(args, NULL) ; break ;
                        case 7: exit(0) ; break ;
                    }
                    break ;
                }
            }
            if(is_builtin) continue ;
        }
        // printf("Running command: %s\n", s) ;
        pid_t pid = -1 ;
        char execbuf[1024] ;
        my_strncpy(execbuf, s, sizeof(execbuf) - 1) ;
        execbuf[sizeof(execbuf) - 1] = '\0' ;

        int wait_fg = !bg[i] ;

        execute_command(execbuf, wait_fg, &pid) ;
    }
    return 1 ;
}