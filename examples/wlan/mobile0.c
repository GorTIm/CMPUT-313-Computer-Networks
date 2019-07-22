#include <cnet.h>
#include <string.h>

#include "newswalk.h"

#define	TALK_FREQUENCY	1000000		// talk every 1 second
#define	EV_TALKING	EV_TIMER8
#define	EV_WALKING	EV_TIMER9


// -----------------------------------------------------------------------

static EVENT_HANDLER(talking)
{
    if(am_walking() == false) {
	size_t len = sizeof(int);

	printf("%2d: transmitting\n", nodeinfo.nodenumber);
	CHECK(
	  CNET_write_physical_reliable(1, (char *)&nodeinfo.nodenumber, &len));
    }
    CNET_start_timer(EV_TALKING, TALK_FREQUENCY, data);
}

static EVENT_HANDLER(listening)
{
    char	buf[1024];
    size_t	len	= sizeof(buf);
    int		link, from;
    double	rx_signal, rx_angle;

    CHECK(CNET_read_physical(&link, buf, &len));
    memcpy(&from, buf, sizeof(int));
    CHECK(CNET_wlan_arrival(link, &rx_signal, &rx_angle));
    printf("\t\t%2d: received from %2d (%.3fdBm @%.3f)\n",
		    nodeinfo.nodenumber, from, rx_signal, rx_angle);
}

// -----------------------------------------------------------------------

EVENT_HANDLER(reboot_node)
{
    CNET_check_version(CNET_VERSION);
    CNET_srand(nodeinfo.time_of_day.sec + nodeinfo.nodenumber);

    CHECK(CNET_set_handler(EV_TALKING, talking, 0));
    CNET_start_timer(EV_TALKING, TALK_FREQUENCY, 0);

    CHECK(CNET_set_handler(EV_PHYSICALREADY, listening, 0));

    init_newswalk(EV_WALKING);
    start_walking();
}
