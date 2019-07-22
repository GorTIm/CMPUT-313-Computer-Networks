#include <cnet.h>

/*  This simple example demonstrates how a node can catch a change
    in a link's state with EV_LINKSTATE.

    If a link is changed with the right mouse button, or if linkmtbf
    and linkmttr are set in the topology file, link_changed() will be
    called with its third parameter set to the number of the link whose
    state has changed.

    This example determines if, and for how long, a node is isolated 
    from the rest of the network.
 */

static EVENT_HANDLER(link_changed)
{
    static CnetTime	when	= 0;
    static int		ndown	= 0;

    int 		link	= (int)data;

/*  THE INDICATED LINK HAS JUST COME UP */
    if(linkinfo[ link ].linkup) {
	fprintf(stderr, "%s: link %d repaired\n", nodeinfo.nodename, link);
	--ndown;
	if(ndown < nodeinfo.nlinks && when != 0) {
	    fprintf(stderr, "\t%s reconnected after %susec\n",
		nodeinfo.nodename, CNET_format64(nodeinfo.time_in_usec - when));
	    when	= 0;
	}
    }
/*  THE INDICATED LINK HAS JUST GONE DOWN */
    else {
	fprintf(stderr, "%s: link %d severed\n", nodeinfo.nodename, link);
	++ndown;
	if(ndown == nodeinfo.nlinks) {
	    fprintf(stderr, "\t%s is now isolated!\n", nodeinfo.nodename);
	    when = nodeinfo.time_in_usec;
	}
    }
}

EVENT_HANDLER(reboot_node)
{
    CHECK(CNET_set_handler(EV_LINKSTATE, link_changed, 0));
}
