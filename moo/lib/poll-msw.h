#ifndef _POLL_MSW_H_
#define _POLL_MSW_H_

#include "moo.h"

/* Event types that can be polled for.  These bits may be set in `events'
   to indicate the interesting event types; they will appear in `revents'
   to indicate the status of the file descriptor.  */
#define POLLIN          0x001           /* There is data to read.  */
#define POLLPRI         0x002           /* There is urgent data to read.  */
#define POLLOUT         0x004           /* Writing now will not block.  */

/* Event types always implicitly polled for.  These bits need not be set in
   `events', but they will appear in `revents' to indicate the status of
   the file descriptor.  */
#define POLLERR         0x008           /* Error condition.  */
#define POLLHUP         0x010           /* Hung up.  */
#define POLLNVAL        0x020           /* Invalid polling request.  */

/* Data structure describing a polling request.  */
struct pollfd 
{
	int fd;                     /* File descriptor to poll.  */
	short int events;           /* Types of events poller cares about.  */
	short int revents;          /* Types of events that actually occurred.  */
};

/* Poll the file descriptors described by the NFDS structures starting at
   FDS.  If TIMEOUT is nonzero and not -1, allow TIMEOUT milliseconds for
   an event to occur; if TIMEOUT is -1, block until an event occurs.
   Returns the number of file descriptors with events, zero if timed out,
   or -1 for errors.  */

typedef unsigned long nfds_t;

#if defined(__cplusplus)
extern "C" {
#endif

int poll (struct pollfd *pfd, nfds_t nfd, int timeout);

#if defined(__cplusplus)
}
#endif

#endif
