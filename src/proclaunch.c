#include "proclaunch.h"

#ifdef _WIN32
    // Windows code
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
