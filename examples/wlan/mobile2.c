#include <cnet.h>
#include "clickwalk.h"
#include <string.h>
#include <stdlib.h>

static EVENT_HANDLER(physicalready)
{
    char	buf[1024];
    size_t	len	= sizeof(buf);
    int		link, from;
    double	rx_signal, rx_angle;

    CHECK(CNET_read_physical(&link, buf, &len));
    memcpy(&from, buf, sizeof(from));
    CHECK(CNET_wlan_arrival(link, &rx_signal, &rx_angle));
    fprintf(stdout,"\t\t%2d: received from %2d (%.2fdBm @%.2f)\n",
			nodeinfo.nodenumber, from, rx_signal, rx_angle);
}

static EVENT_HANDLER(senddata)
{
    size_t	len	= sizeof(int);

    fprintf(stdout,"%d: transmitting\n", nodeinfo.nodenumber);
    CHECK(CNET_write_physical_reliable(1, (char *)&nodeinfo.nodenumber, &len));
}

/* ----------------------------------------------------------------------- */

EVENT_HANDLER(reboot_node)
{
    CHECK(CNET_set_handler(EV_PHYSICALREADY, physicalready, 0));

    CHECK(CNET_set_debug_string( EV_DEBUG0, "transmit"));
    CHECK(CNET_set_handler(EV_DEBUG0, senddata, 0));

    init_clickwalk();
}
