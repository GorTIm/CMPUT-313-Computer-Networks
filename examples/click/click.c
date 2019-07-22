#include <cnet.h>
#include <ctype.h>
#include <string.h>

/*  This simple cnet "protocol" demonstrates the use of cnet's
    debugging buttons, the Physical Layer, and checksums.

    The supplied function reboot_node() is first called for each node.
    reboot_node() informs cnet that the node is interested in the EV_DEBUG0,
    EV_DEBUG1 and EV_PHYSICALREADY events.
    Strings are placed on the first and second debug buttons, corresponding
    to the two interesting EV_DEBUG? events.

    When EV_DEBUG0 is raised, cnet calls send_reliable(), which will format a
    message in a frame and transmit that frame to the other node via link 1.
    As the message will be assumed to be a C string (ending in a '\0'
    byte), it is important to send the '\0' byte too (hence, strlen()+1 ).
    Similarly, when EV_DEBUG1 occurs, send_unreliable() is called.

    When EV_PHYSICALREADY occurs, cnet calls frame_arrived(),
    to read the frame from the Physical Layer and report its contents.
    If the frame was corrupted in transmit, any non-alphanumeric bytes
    are printed as question marks.
 */

#define	MAX_FRAME	64

static char	*fruit[] = { "apple", "pineapple", "banana", "tomato", "plum" };
#define	NFRUITS	(sizeof(fruit) / sizeof(fruit[0]))

static int	seqno    = 0;

// -----------------------------------------------------------------------

static void describe_frame(char *frame, size_t length)
{
    char	byte;
    int		n;

    printf("[\"");
    for(n=0 ; n<length-1 ; ++n) {
	byte = frame[n];
	if(isalnum(byte) || byte == ' ')
	    putchar(byte);
	else
	    putchar('?');
    }
    printf("\", ");
    printf("checksum=%5d]\n",CNET_IP_checksum((unsigned short *)frame,length));
}

static EVENT_HANDLER(send_reliable)
{
    char	frame[MAX_FRAME];
    size_t	length;

    sprintf(frame, "message %d is %s", ++seqno, fruit[CNET_rand()%NFRUITS]);
    length	= strlen(frame) + 1;	// we need to send '\0' byte too

    printf("sending %u bytes on link %d: ", length, 1);
    describe_frame(frame, length);
    CHECK(CNET_write_physical_reliable(1, frame, &length));
}

static EVENT_HANDLER(send_unreliable)
{
    char	frame[MAX_FRAME];
    size_t	length;

    sprintf(frame, "message %d is %s", ++seqno, fruit[CNET_rand()%NFRUITS]);
    length	= strlen(frame) + 1;	// we need to send '\0' byte too

    printf("sending %u bytes on link %d: ", length, 1);
    describe_frame(frame, length);
    CHECK(CNET_write_physical(1, frame, &length));
}

static EVENT_HANDLER(frame_arrived)
{
    char	frame[MAX_FRAME];
    size_t	length;
    int			link;

    length	= sizeof(frame);
    CHECK(CNET_read_physical(&link, frame, &length));
    printf("received %u bytes on link %d: ", length, link);
    describe_frame(frame, length);
}

EVENT_HANDLER(reboot_node)
{
//  INDICATE OUR INTEREST in EV_PHYSICALREADY, EV_DEBUG0 and EV_DEBUG1 EVENTS
    CHECK(CNET_set_handler(EV_PHYSICALREADY,	frame_arrived, 0));

    CHECK(CNET_set_handler(EV_DEBUG0,		send_reliable, 0));
    CHECK(CNET_set_debug_string(EV_DEBUG0,	"Reliable"));

    CHECK(CNET_set_handler(EV_DEBUG1,		send_unreliable, 0));
    CHECK(CNET_set_debug_string(EV_DEBUG1,	"Unreliable"));
}
