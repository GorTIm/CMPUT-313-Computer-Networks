#include <cnet.h>
#include <string.h>

/*  This cnet protocol demonstrates the use of cnet's keyboard event
    and the Physical Layer.

    The supplied function reboot_node() will first be called for each node.
    reboot_node() informs cnet that the node is interested in the
    EV_KEYBOARDREADY and EV_PHYSICALREADY events.

    When EV_KEYBOARDREADY is raised, cnet will call keyboard(), which will
    read the input line using CNET_read_keyboard() and then transmit the
    line as a data frame, to the other node via link 1.
    As the frame will be assumed to be a C string (ending in a '\0' byte),
    it is important to send the '\0' byte too.

    When EV_PHYSICALREADY occurs, cnet will call frame_arrived(), which
    will read the frame from the Physical Layer and print it out.
    Note the differing propagation delays in the topology file KEYBOARD.

    Note that this simple ``protocol'' will not work if there is corruption
    on the link (probframecorrupt != 0). Can you see why?
 */

#define	MAX_LINE	80
#define	MAX_FRAME	128

static void prompt(int inc)
{
    static int n=0;

    n += inc;
    printf("%s.%d> ", nodeinfo.nodename, n);
}

static EVENT_HANDLER(keyboard)
{
    char	line[MAX_LINE];
    size_t	length;

    length	= sizeof(line);
    CHECK(CNET_read_keyboard(line, &length));

    if(length > 1) {			/* not just a blank line? */
	printf("\tsending %d bytes - \"%s\"\n", length, line);
	CHECK(CNET_write_physical(1, line, &length));
	prompt(1);
    }
    else
	prompt(0);
}

static EVENT_HANDLER(frame_arrived)
{
    char	frame[MAX_FRAME];
    size_t	length;
    int		link;

    length	= sizeof(frame);
    CHECK(CNET_read_physical(&link, frame, &length));

    printf("\treceived %ld bytes on link %d - \"%s\"\n", length,link,frame);
    prompt(0);
}

EVENT_HANDLER(reboot_node)
{
/*  EXPRESS INTEREST IN EV_KEYBOARDREADY AND EV_PHYSICALREADY EVENTS */
    CHECK(CNET_set_handler( EV_KEYBOARDREADY, keyboard, 0));
    CHECK(CNET_set_handler( EV_PHYSICALREADY, frame_arrived, 0));

    prompt(1);
}
