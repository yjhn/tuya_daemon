#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <syslog.h>
#include <unistd.h>

#include "become_daemon.h"

int become_daemon()
{
	// Fork to become orphaned and be adopted by the init process.
	switch (fork()) {
	case -1:
		syslog(LOG_ERR, "Call to fork() failed: %m");
		return -1;
	case 0:
		break; // this is the child process
	default:
		_exit(EXIT_SUCCESS); // parent process terminates
	}

	// Lose the controlling terminal by creating a new session.
	if (setsid() == -1) {
		syslog(LOG_ERR, "Call to setsid() failed: %m");
		return -2;
	}

	// Fork again to lose the leader status in the new session.
	// This makes the process unable to acquire a controlling terminal.
	switch (fork()) {
	case -1:
		syslog(LOG_ERR, "Second call to fork() failed: %m");
		return -3;
	case 0:
		break; // this is the child process
	default:
		_exit(EXIT_SUCCESS);
	}

	// Clear file / directory creation mode mask.
	umask(0);

	// Change working directory to root to not interfere with
	// unmounting of unused mount points.
	if (chdir("/") == -1) {
		syslog(LOG_ERR, "Call to chdir() failed: %m");
		return -4;
	}

	int ret_val = close(STDIN_FILENO);
	if (ret_val == -1) {
		// %m in syslog is equivalent to strerror(errno)
		syslog(LOG_ERR, "Failed to close stdin: %m");
		return -5;
	}
	ret_val = close(STDOUT_FILENO);
	if (ret_val == -1) {
		syslog(LOG_ERR, "Failed to close stdout: %m");
		return -6;
	}
	ret_val = close(STDERR_FILENO);
	if (ret_val == -1) {
		syslog(LOG_ERR, "Failed to close stderr: %m");
		return -7;
	}

	// Point stdin, stdout and stderr to /dev/null
	// Since all file descriptors are now closed, open() will reuse
	// file descriptor 0, which is by default stdin.
	if (open("/dev/null", O_RDWR) != STDIN_FILENO) {
		syslog(LOG_ERR, "Failed to point stdin to /dev/null");
		return -8;
	}
	// stdout by default is file descriptor 1.
	if (dup(STDIN_FILENO) != STDOUT_FILENO) {
		syslog(LOG_ERR, "Failed to point stdout to /dev/null");
		return -9;
	}
	// stderr by default is file descriptor 2.
	if (dup(STDIN_FILENO) != STDERR_FILENO) {
		syslog(LOG_ERR, "Failed to point stderr to /dev/null");
		return -10;
	}

	return 0;
}
