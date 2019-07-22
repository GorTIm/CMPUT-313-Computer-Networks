#include <cnet.h>
#include <string.h>

#define	ISOLATED		(1<<30)
#define	MAXTTL			3

typedef struct {
    int		from;
    int		x;
    int		y;
    int		hops;
    int		ttl;
} PACKET;

static	CnetTimerID	itimer;
static	CnetPosition	me;
static	int		hops_away	= ISOLATED;
static	double		strongest	= -1000.0;

/* ----------------------------------------------------------------------- */

static EVENT_HANDLER(isolated)
{
    if(hops_away == ISOLATED)
	fprintf(stdout, "%2d: hey, fellas, over here!\n", nodeinfo.nodenumber);
}

static void broadcast(int hops_away, int ttl)
{
    if(ttl > 0) {
	PACKET		pkt;
	size_t		len;

	pkt.from	= nodeinfo.nodenumber;
	pkt.x		= me.x;
	pkt.y		= me.y;
	pkt.hops	= hops_away;
	pkt.ttl		= ttl;
	len		= sizeof(pkt);
	CNET_write_physical_reliable(1, (char *)&pkt, &len);
    }
}

static EVENT_HANDLER(start0)
{
    broadcast(0, MAXTTL);
}

static EVENT_HANDLER(listening)
{
    PACKET	pkt;
    size_t	len	= sizeof(pkt);
    int		link;

    CHECK(CNET_read_physical(&link, (char *)&pkt, &len));

    if(pkt.hops <= hops_away) {
	if(pkt.hops == hops_away) {
	    double	rx_signal;

	    CHECK(CNET_wlan_arrival(link, &rx_signal, NULL));
	    if(rx_signal <= strongest)
		goto reTx;
	    strongest	= rx_signal;
	}
	hops_away	= pkt.hops;
	CNET_stop_timer(itimer);

	TCLTK("$map delete w%d", nodeinfo.nodenumber);
	TCLTK("$map create line %d %d %d %d -fill blue -width 2 -tags w%d",
		me.x, me.y, pkt.x, pkt.y, nodeinfo.nodenumber);
    }
reTx:
    broadcast(pkt.hops+1, pkt.ttl-1);
}

/* ----------------------------------------------------------------------- */

EVENT_HANDLER(reboot_node)
{
    CnetPosition	bounds;

    CNET_check_version(CNET_VERSION);
    CNET_get_position(&me, &bounds);

    if(nodeinfo.nodenumber == 0) {
	me.x	=  bounds.x / 2;
	me.y	=  bounds.y / 2;
	CNET_set_position(me);

	hops_away	= 0;
	strongest	= 10000.0;
	CHECK(CNET_set_handler(EV_TIMER1, start0, 0));
	CNET_start_timer(EV_TIMER1, 10000, 0);
    }
    else {
	CHECK(CNET_set_handler(EV_TIMER1, isolated, 0));
	itimer = CNET_start_timer(EV_TIMER1, 3000000, 0);
    }

    CHECK(CNET_set_handler(EV_PHYSICALREADY, listening, 0));
}
