#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "become_daemon.h"

int become_daemon()
{
	// Fork to become orphaned and be adopted by the init process.
	switch (fork()) {
	case -1:
		perror("Call to fork() failed");
		return -1;
	case 0:
		break; // this is the child process
	default:
		_exit(EXIT_SUCCESS); // parent process terminates
	}

	// Lose the controlling terminal by creating a new session.
	if (setsid() == -1) {
		return -2;
	}

	// Fork again to lose the leader status in the new session.
	// This makes the process unable to acquire a controlling terminal.
	switch (fork()) {
	case -1:
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
	chdir("/");

	// Close all file descriptors.
	// closefrom() available from glibc 2.34
	closefrom(0);

	// Point stdin, stdout and stderr to /dev/null
	// Since all file descriptors are now closed, open() will reuse
	// file descriptor 0, which is by default stdin.
	if (open("/dev/null", O_RDWR) != STDIN_FILENO) {
		return -4;
	}
	// stdout by default is file descriptor 1.
	if (dup(STDIN_FILENO) != STDOUT_FILENO) {
		return -5;
	}
	// stderr by default is file descriptor 2.
	if (dup(STDIN_FILENO) != STDERR_FILENO) {
		return -6;
	}

	return 0;
}
