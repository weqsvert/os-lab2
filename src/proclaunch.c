#include "proclaunch.h"

#ifdef _WIN32
    #include <windows.h>
    #include <string.h>
    #include <stdlib.h>
    #include <stdio.h>

    static char* build_cmdline(const char *const argv[]) {
        size_t total_len = 0;
        for (int i = 0; argv[i] != NULL; i++) {
            total_len += strlen(argv[i]) + 3;
        }
        
        char *cmd = malloc(total_len + 1);
        if (!cmd) return NULL;
        cmd[0] = '\0';
        
        for (int i = 0; argv[i] != NULL; i++) {
            if (i > 0) strcat(cmd, " ");
            
            int has_space = strchr(argv[i], ' ') != NULL;
            int has_quote = strchr(argv[i], '"') != NULL;
            
            if (has_space || has_quote) {
                strcat(cmd, "\"");
                const char *p = argv[i];
                while (*p) {
                    if (*p == '"') strcat(cmd, "\\\"");
                    else { char tmp[2] = {*p, '\0'}; strcat(cmd, tmp); }
                    p++;
                }
                strcat(cmd, "\"");
            } else {
                strcat(cmd, argv[i]);
            }
        }
        return cmd;
    }

    int pl_start(const char *const argv[], pl_process *proc) {
        char *cmdline = build_cmdline(argv);
        if (!cmdline) return -1;

        STARTUPINFOA si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        if (!CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
            free(cmdline);
            return -1;
        }

        free(cmdline);
        CloseHandle(pi.hThread);
        proc->handle = (intptr_t)pi.hProcess;
        proc->pid = (long)pi.dwProcessId;
        return 0;
    }

    int pl_wait(pl_process *proc, int *exit_code) {
        HANDLE h = (HANDLE)proc->handle;
        WaitForSingleObject(h, INFINITE);
        
        DWORD code;
        if (!GetExitCodeProcess(h, &code)) {
            return -1;
        }
        CloseHandle(h);
        *exit_code = (int)code;
        return 0;
    }

    int pl_poll(pl_process *proc, int *exit_code) {
        HANDLE h = (HANDLE)proc->handle;
        DWORD wait_res = WaitForSingleObject(h, 0);
        
        if (wait_res == WAIT_TIMEOUT) {
            return 0;
        }
        
        DWORD code;
        if (!GetExitCodeProcess(h, &code)) {
            return -1;
        }
        CloseHandle(h);
        *exit_code = (int)code;
        return 1;
    }
#else
    #include <unistd.h>
    #include <sys/wait.h>
    #include <errno.h>
    #include <fcntl.h>

    int pl_start(const char *const argv[], pl_process *proc) {
        int pipefd[2];
        if (pipe(pipefd) == -1) return -1;

        pid_t pid = fork();
        if (pid < 0) {
            close(pipefd[0]);
            close(pipefd[1]);
            return -1;
        }
        
        if (pid == 0) {
            close(pipefd[0]);
            fcntl(pipefd[1], F_SETFD, FD_CLOEXEC);
            execvp(argv[0], (char *const *)argv);
            int err = errno;
            write(pipefd[1], &err, sizeof(err));
            _exit(127); 
        }
        
        close(pipefd[1]);
        int exec_errno = 0;
        ssize_t n = read(pipefd[0], &exec_errno, sizeof(exec_errno));
        close(pipefd[0]);
        
        if (n > 0) {
            int status;
            waitpid(pid, &status, 0); 
            return -1;
        }
        
        proc->pid = pid;
        return 0;
    }

    int pl_wait(pl_process *proc, int *exit_code) {
        int status;
        pid_t w;
        do {
            w = waitpid(proc->pid, &status, 0);
        } while (w == -1 && errno == EINTR);

        if (w == -1) return -1;

        if (WIFEXITED(status)) *exit_code = WEXITSTATUS(status);
        else if (WIFSIGNALED(status)) *exit_code = 128 + WTERMSIG(status);
        else *exit_code = -1;
        return 0;
    }

    int pl_poll(pl_process *proc, int *exit_code) {
        int status;
        pid_t w = waitpid(proc->pid, &status, WNOHANG);
        if (w == 0) return 0;
        if (w == -1) return -1;
        
        if (WIFEXITED(status)) *exit_code = WEXITSTATUS(status);
        else if (WIFSIGNALED(status)) *exit_code = 128 + WTERMSIG(status);
        else *exit_code = -1;
        return 1;
    }
#endif