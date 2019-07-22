#include <cnet.h>
#include <cnetsupport.h>
#include <unistd.h>

#define	NODE_SPACING		50
#define	TRANSMIT_PERIOD		1000000
#define	SENSE_PERIOD		50000

typedef struct {
    char	payload[64];
} WLAN_FRAME;


/* ----------------------------------------------------------------------- */

static EVENT_HANDLER(sense)
{
    int got = CNET_carrier_sense(1);

    if(got != 0)
	fprintf(stderr, "sense: @%lldusec %d\n", nodeinfo.time_in_usec, got);
    CNET_start_timer(ev, SENSE_PERIOD, 0);
}

static EVENT_HANDLER(transmit)
{
    WLAN_FRAME	frame;
    size_t	len	= sizeof(frame);

    CHECK(CNET_write_physical(1, (char *)&frame, &len));
    CNET_start_timer(ev, TRANSMIT_PERIOD+CNET_rand()%1000, 0);
}

static EVENT_HANDLER(listening)
{
    WLAN_FRAME	frame;
    size_t	len	= sizeof(frame);
    int		link;

    CHECK(CNET_read_physical(&link, (char *)&frame, &len));
}

EVENT_HANDLER(reboot_node)
{
    CnetPosition	pos;

//  ENSURE THAT WE'RE USING THE CORRECT VERSION OF cnet
    CNET_check_version(CNET_VERSION);
    CNET_srand(getpid() + nodeinfo.nodenumber);

//  POSITION ALL NODES IN A ROW
    pos.x	 = (nodeinfo.nodenumber+1) * NODE_SPACING;
    pos.y	 = 50;
    pos.z	 = 0;
    CHECK(CNET_set_position(pos));

    CHECK(CNET_set_handler(EV_TIMER1, transmit, 0));
    CNET_start_timer(EV_TIMER1, TRANSMIT_PERIOD+CNET_rand()%1000, 0);

    CHECK(CNET_set_handler(EV_TIMER2, sense, 0));
    CNET_start_timer(EV_TIMER2, SENSE_PERIOD, 0);

    CHECK(CNET_set_handler(EV_PHYSICALREADY, listening, 0));
}
