#include <cnet.h>
#include <stdlib.h>

#define	MOVE_FREQUENCY	200000		// move every 1/5 second
#define	WALK_SPEED	5		// metres per MOVE_FREQUENCY
#define	CLOSE_ENOUGH	(3*WALK_SPEED)	// close enough to destination?
#define	MIN_PAUSE	5		// in seconds
#define	MAX_PAUSE	30		// in seconds

static	CnetEvent	EV_MOVING	= EV_NULL;
static	CnetTimerID	tid		= NULLTIMER;
static	bool		paused		= true;

static EVENT_HANDLER(newswalk)
{
    static CnetPosition	dest	= {0, 0, 0};
    static int		dx	= 0;
    static int		dy	= 0;

    CnetPosition	now, max;
    CnetTime		movenext	= MOVE_FREQUENCY;

    CHECK(CNET_get_position(&now, &max));

    if(paused) {
	paused	= false;
//  BEGIN MOVING ALONG THE X-AXIS
	if(CNET_rand() % 2 == 0) {
	    dest.x	= CNET_rand() % ((int)max.x - 2*CLOSE_ENOUGH)+CLOSE_ENOUGH;
	    dest.y	= now.y;
	    dest.z	= 0;
	    dx		= CNET_rand() % WALK_SPEED + 1;
	    if(dest.x < now.x)
		dx	= -dx;
	    dy		= 0;
	}
//  OR ALONG THE Y-AXIS
	else {
	    dest.x	= now.x;
	    dest.y	= CNET_rand() % ((int)max.y - 2*CLOSE_ENOUGH)+CLOSE_ENOUGH;
	    dest.z	= 0;
	    dy		= CNET_rand() % WALK_SPEED + 1;
	    if(dest.y < now.y)
		dy	= -dy;
	    dx		= 0;
	}
	now.x	+= dx;
	now.y	+= dy;
	CHECK(CNET_set_position(now));
    }
    else {
//  WE ARE CURRENTLY MOVING
	int	rx	= (dest.x - now.x);
	int	ry	= (dest.y - now.y);

//  ARE WE CLOSE ENOUGH TO OUR WAYPOINT YET?
	if((rx*rx + ry*ry) <= CLOSE_ENOUGH*CLOSE_ENOUGH) {
	    paused	= true;
	    movenext	= (CNET_rand() % (MAX_PAUSE-MIN_PAUSE) + MIN_PAUSE) *
				1000000;
	}
	else {
//  NOT THERE YET, SO JUST KEEP WALKING
	    now.x	+= dx;
	    now.y	+= dy;
	    CHECK(CNET_set_position(now));
	}
    }

//  RESCHEDULE THIS MOVEMENT EVENT
    tid	= CNET_start_timer(EV_MOVING, movenext, data);
}

// -----------------------------------------------------------------------

void init_newswalk(CnetEvent ev)
{
    if(nodeinfo.nodenumber == 0)
	fprintf(stdout, "making a random walk along the N, S, E, W axes\n");
    EV_MOVING	= ev;
    CHECK(CNET_set_handler(EV_MOVING, newswalk, 0));
}

void start_walking(CnetEvent ev)
{
    CNET_stop_timer(tid);
    tid	= CNET_start_timer(EV_MOVING, MOVE_FREQUENCY, (CnetData)0);
}

void stop_walking(void)
{
    CNET_stop_timer(tid);
    paused = true;
}

bool am_walking(void)
{
    return(paused == false);
}
