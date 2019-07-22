#include <cnet.h>
#include <stdlib.h>
#include <string.h>

#include "newswalk.h"

// ----------------------------------------------------------------

typedef enum    { DATA, ACK }   FRAMEKIND;

typedef struct {
    char        data[MAX_MESSAGE_SIZE];
} MSG;

typedef struct {
    FRAMEKIND   kind;      	// only ever DATA or ACK
    int         len;       	// the length of the msg field only
    int         checksum;  	// checksum of the whole frame
    int         seq;       	// only ever 0 or 1
    MSG         msg;
} FRAME;

#define FRAME_HEADER_SIZE  (sizeof(FRAME) - sizeof(MSG))
#define FRAME_SIZE(f)      (FRAME_HEADER_SIZE + f.len)


static  MSG       	*lastmsg;
static  size_t       	lastlength		= 0;
static  CnetTimerID	lasttimer		= NULLTIMER;

static  int       	ackexpected		= 0;
static	int		nextframetosend		= 0;
static	int		frameexpected		= 0;

static void transmit_frame(MSG *msg, FRAMEKIND kind, size_t length, int seqno)
{
    FRAME       f;
    int		link = 1;

    f.kind      = kind;
    f.seq       = seqno;
    f.checksum  = 0;
    f.len       = length;

    switch (kind) {
    case ACK :
	CHECK(CNET_set_wlancolour(link, "blue"));
	break;

    case DATA: {
	CnetTime	timeout;

	CHECK(CNET_set_wlancolour(link, "red"));
        memcpy(&f.msg, (char *)msg, length);

	timeout = (FRAME_SIZE(f)*(CnetTime)8000000 / linkinfo[link].bandwidth +
				linkinfo[link].propagationdelay);
	timeout	= 300000;

        lasttimer = CNET_start_timer(EV_TIMER1, 3 * timeout, 0);
	break;
      }
    }
    length      = FRAME_SIZE(f);
    f.checksum  = CNET_ccitt((unsigned char *)&f, length);
    CHECK(CNET_write_physical_reliable(link, (char *)&f, &length));
}

static EVENT_HANDLER(application_ready)
{
    CnetAddr destaddr;

    lastlength  = sizeof(MSG);
    CHECK(CNET_read_application(&destaddr, (char *)lastmsg, &lastlength));
    CNET_disable_application(ALLNODES);

    transmit_frame(lastmsg, DATA, lastlength, nextframetosend);
    nextframetosend = 1-nextframetosend;
}

static EVENT_HANDLER(physical_ready)
{
    FRAME       f;
    size_t	len;
    int         link, checksum;

    len         = sizeof(FRAME);
    CHECK(CNET_read_physical(&link, (char *)&f, &len));

    checksum    = f.checksum;
    f.checksum  = 0;
    if(CNET_ccitt((unsigned char *)&f, len) != checksum) {
        return;           // bad checksum, ignore frame
    }

    switch (f.kind) {
    case ACK :
        if(f.seq == ackexpected) {
            CNET_stop_timer(lasttimer);
            ackexpected = 1-ackexpected;
            CNET_enable_application(ALLNODES);
        }
	break;

    case DATA :
        if(f.seq == frameexpected) {
            len = f.len;
            CHECK(CNET_write_application((char *)&f.msg, &len));
            frameexpected = 1-frameexpected;
        }
        transmit_frame(NULL, ACK, 0, f.seq);
	break;
    }
}

static EVENT_HANDLER(timeouts)
{
    if(timer == lasttimer) {
        transmit_frame(lastmsg, DATA, lastlength, ackexpected);
    }
}

// ----------------------------------------------------------------

EVENT_HANDLER(reboot_node)
{
    if(nodeinfo.nodenumber > 1) {
	fprintf(stderr,"This is not a 2-node network!\n");
	exit(1);
    }

    CNET_check_version(CNET_VERSION);
    CNET_srand(nodeinfo.time_of_day.sec + nodeinfo.nodenumber);

    lastmsg	= malloc(sizeof(MSG));

    CHECK(CNET_set_handler( EV_APPLICATIONREADY, application_ready, 0));
    CHECK(CNET_set_handler( EV_PHYSICALREADY,    physical_ready, 0));
    CHECK(CNET_set_handler( EV_TIMER1,           timeouts, 0));

    if(nodeinfo.nodetype == NT_MOBILE) {
	init_newswalk(EV_TIMER7);
	start_walking();
    }

    CNET_enable_application(ALLNODES);
}
