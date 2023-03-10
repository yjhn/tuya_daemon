#ifndef BECOME_DAEMON_H
#define BECOME_DAEMON_H

/*
 * Become a daemon process using the double fork method.
 * Return values:
 *  0 - success, the program is now a daemon
 * -1 - first fork() failed
 * -2 - setsid() failed
 * -3 - second fork() failed
 * -4 - failed to point stdin to /dev/null
 * -5 - failed to point stdout to /dev/null
 * -6 - failed to point stderr to /dev/null
 */
int become_daemon(void);

#endif
