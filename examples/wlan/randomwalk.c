#include <cnet.h>

#define	MOVE_FREQUENCY	200000		// move every 1/5 second
#define	WALK_SPEED	3		// metres per MOVE_FREQUENCY
#define	CLOSE_ENOUGH	(3*WALK_SPEED)	// close enough to destination?
#define	MIN_PAUSE	5		// in seconds
#define	MAX_PAUSE	30		// in seconds

static	CnetEvent	EV_MOVING	= EV_NULL;
static	CnetTimerID	tid		= NULLTIMER;
static	bool		paused		= true;

static EVENT_HANDLER(randomwalk)
{
    static CnetPosition	dest	= {0, 0, 0};
    static int		dx	= 0;
    static int		dy	= 0;
    static int		speed	= 0;

    CnetPosition	posnow, bounds;
    CnetTime		movenext	= MOVE_FREQUENCY;

    CHECK(CNET_get_position(&posnow, &bounds));

    if(paused) {
	paused	= false;
	dest.x	= CNET_rand() % ((int)bounds.x - 2*CLOSE_ENOUGH)+CLOSE_ENOUGH;
	dest.y	= CNET_rand() % ((int)bounds.y - 2*CLOSE_ENOUGH)+CLOSE_ENOUGH;
	dest.z	= 0;

	speed	= CNET_rand() % WALK_SPEED + 1;
	dx	= (dest.x - posnow.x) < 0 ? -speed : speed;
	dy	= (dest.y - posnow.y) < 0 ? -speed : speed;

	posnow.x	+= dx;
	posnow.y	+= dy;
	CHECK(CNET_set_position(posnow));
    }
    else {
//  WE ARE CURRENTLY MOVING
	int	rx	= (dest.x - posnow.x);
	int	ry	= (dest.y - posnow.y);

//  ARE WE CLOSE ENOUGH TO OUR WAYPOINT YET?
	if((rx*rx + ry*ry) <= CLOSE_ENOUGH*CLOSE_ENOUGH) {
	    paused	= true;
	    movenext	= (CNET_rand() % (MAX_PAUSE-MIN_PAUSE) + MIN_PAUSE) *
				(CnetTime)1000000;
	}
	else {
//  NOT THERE YET, SO JUST KEEP WALKING
	    dx	= (dest.x - posnow.x) < 0 ? -speed : speed;
	    dy	= (dest.y - posnow.y) < 0 ? -speed : speed;
	    posnow.x	+= dx;
	    posnow.y	+= dy;
	    CHECK(CNET_set_position(posnow));
	}
    }

//  RESCHEDULE THIS MOVEMENT EVENT
    tid	= CNET_start_timer(EV_MOVING, movenext, data);
}

// -----------------------------------------------------------------------

void init_randomwalk(CnetEvent ev)
{
    if(nodeinfo.nodenumber == 0)
	fprintf(stdout, "making a random walk in any direction\n");
    EV_MOVING	= ev;
    CHECK(CNET_set_handler(EV_MOVING, randomwalk, 0));
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
