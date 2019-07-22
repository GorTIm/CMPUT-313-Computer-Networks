#include <cnet.h>

/*  This "protocol" doesn't do much - at the click of a button it simply
    transmits the same frame, twice, and demonstrates how LEDs may be set
    and cleared using CNET_set_LED().
 */

#define	FRAMESIZE	5000

static EVENT_HANDLER(arrived)
{
    char	frame[FRAMESIZE];
    size_t	length;
    int		link;

    length	= sizeof(frame);
    CHECK(CNET_read_physical(&link, frame, &length));
    printf("\t%d: received %zd bytes @ t=%susec\n",
	    nodeinfo.nodenumber, length, CNET_format64(nodeinfo.time_in_usec));
}

static void send_once(void)
{
    char	frame[FRAMESIZE];
    size_t	length;

    length	= sizeof(frame);
    CHECK(CNET_write_physical(1, frame, &length));
    printf("%d: sending %zd bytes @ t=%susec\n",
	    nodeinfo.nodenumber, length, CNET_format64(nodeinfo.time_in_usec));
}

static EVENT_HANDLER(send_twice)
{
    static CnetColour led_colours[] = {
	"red", "cyan", "green", "yellow"
    };
#define	N_LED_COLOURS	(sizeof(led_colours) / sizeof(led_colours[0]))

    static int l=0;		// start with first LED
    static int c=0;		// start with first colour

    send_once();
    send_once();

    CNET_set_LED(l, led_colours[c]);
    l = (l+1) % CNET_NLEDS;
    c = (c+1) % N_LED_COLOURS;
}

static EVENT_HANDLER(clear_leds)
{
    int n;

    for(n=0 ; n<CNET_NLEDS ; ++n)
	CNET_set_LED(n, CNET_LED_OFF);
}

EVENT_HANDLER(reboot_node)
{
    CHECK(CNET_set_handler(EV_PHYSICALREADY, arrived, 0));

    CHECK(CNET_set_debug_string(EV_DEBUG0, "send twice"));
    CHECK(CNET_set_handler(EV_DEBUG0, send_twice, 0));

    CHECK(CNET_set_debug_string(EV_DEBUG2, "clear LEDs"));
    CHECK(CNET_set_handler(EV_DEBUG2, clear_leds, 0));
}
