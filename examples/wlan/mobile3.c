#include <cnet.h>
#include <string.h>

#include "newswalk.h"

#define	TALK_FREQUENCY	1000000		// talk every 1 second
#define	EV_TALKING	EV_TIMER8

// -----------------------------------------------------------------------

#define	BROADCAST	(-1)

static EVENT_HANDLER(talking)
{
    if(am_walking() == false) {
	int	buf[2];
	size_t	len;

	buf[0]	= BROADCAST;
	buf[1]	= nodeinfo.nodenumber;
	len	= sizeof(buf);

	CHECK(CNET_set_wlancolour(1, "red"));
	CHECK(CNET_write_physical(1, (char *)buf, &len));
    }
    CNET_start_timer(EV_TALKING, TALK_FREQUENCY, data);
}

static EVENT_HANDLER(listening)
{
    int		buf[1024];
    size_t	len	= sizeof(buf);
    int		link;

    CHECK(CNET_read_physical(&link, (char *)buf, &len));
    if(buf[0] == BROADCAST) {
	buf[0]	= buf[1];
	len	= 2*sizeof(int);
	CHECK(CNET_set_wlancolour(link, "blue"));
	CHECK(CNET_write_physical(link, (char *)buf, &len));
    }
}

// -----------------------------------------------------------------------

EVENT_HANDLER(reboot_node)
{
    CNET_check_version(CNET_VERSION);
    CNET_srand(nodeinfo.time_of_day.sec + nodeinfo.nodenumber);

    CHECK(CNET_set_handler(EV_TALKING, talking, 0));
    CNET_start_timer(EV_TALKING, TALK_FREQUENCY, 0);
    CHECK(CNET_set_handler(EV_PHYSICALREADY, listening, 0));

    init_newswalk(EV_TIMER9);
    start_walking();
}
