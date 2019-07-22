#include <cnet.h>
#include <cnetsupport.h>
#include <string.h>
#include <stdlib.h>

#define	EV_MOBILITY		EV_TIMER9

#define	USEC_PER_STEP		1000000
#define	MARGIN			20
#define	CLOSE_ENOUGH		100

//  ALL COORDINATES IN METRES
typedef struct {
    double		x;
    double		y;
} POINT;

typedef struct {
    POINT		now;		// current location
    bool		paused;		// is this node paused?
    POINT		dest;		// known destination
    double		dx;		// steps to reach destination
    double		dy;

    CnetPosition	maparea;	// max. dimensions of map
    double		walkspeed;	// metres per second
    CnetTime		pausetime;	// microseconds
    CnetRandom		mt;		// random state per node
} WALK;

static	WALK		*walk		= NULL;
static	CnetPosition	*positions	= NULL;

// -----------------------------------------------------------------------

static void random_point(POINT *new)
{
    new->x = CNET_nextrand(walk->mt) % (walk->maparea.x-2*MARGIN) + MARGIN;
    new->y = CNET_nextrand(walk->mt) % (walk->maparea.y-2*MARGIN) + MARGIN;
}

static EVENT_HANDLER(mobility)
{
//  FINISHED PAUSING, BEGIN MOVING TO A NEW DESTINATION
    if(walk->paused) {
	POINT		newdest;	//  CHOOSE A NEW RANDOM DESTINATION
	do {
	    random_point(&newdest);
	} while((int)newdest.x == (int)walk->now.x &&
		(int)newdest.y == (int)walk->now.y);
	walk->dest	= newdest;

//  DETERMINE EACH STEP'S dx AND dy TOWARDS NEW DESTINATION
	double dx	= (walk->dest.x - walk->now.x);
	double dy	= (walk->dest.y - walk->now.y);
	double metres	= sqrt(dx*dx + dy*dy);
	double nsteps	= (metres / walk->walkspeed)*1000000 / USEC_PER_STEP;

	walk->dx	= dx / nsteps;
	walk->dy	= dy / nsteps;

	CNET_start_timer(EV_MOBILITY, USEC_PER_STEP, 0);
	walk->paused	= false;
    }

//  STILL MOVING, MOVE TOWARDS OUR DESTINATION
    else {
	walk->now.x	+= walk->dx;
	walk->now.y	+= walk->dy;

	CnetPosition	newpos = { walk->now.x, walk->now.y, 0 };
	CHECK(CNET_set_position(newpos));
	positions[nodeinfo.nodenumber]	= newpos;

//  ARE WE THERE YET?  ARE WE THERE YET?  ARE WE THERE YET?
	double dx	= (walk->dest.x - walk->now.x);
	double dy	= (walk->dest.y - walk->now.y);

	if((dx*dx + dy*dy) <= CLOSE_ENOUGH) {
	    walk->paused	= true;
	    CNET_start_timer(EV_MOBILITY, walk->pausetime, 0);	// pause
	}
	else
	    CNET_start_timer(EV_MOBILITY, USEC_PER_STEP, 0);	// keep walking
    }
}

void init_mobility(double walkspeed_metres_per_sec, int pausetime_secs)
{
//  ALLOCATE AND INITIALIZE A NEW WALK STRUCTURE
    walk		= calloc(1, sizeof(WALK));
    walk->walkspeed	= walkspeed_metres_per_sec;
    walk->pausetime	= pausetime_secs * 1000000;
    walk->paused	= true;
    CHECK(CNET_get_position(NULL, &walk->maparea));
    walk->mt		=
		CNET_newrand(nodeinfo.time_of_day.sec + nodeinfo.nodenumber);

    positions	= CNET_shmem2("p", NNODES*sizeof(CnetPosition));

//  CHOOSE OUR RANDOM STARTING POINT ON THE MAP
    random_point(&walk->now);
    CnetPosition	newpos = { walk->now.x, walk->now.y, 0 };
    CHECK(CNET_set_position(newpos));
    positions[nodeinfo.nodenumber]	= newpos;

//  AND START MOVING
    CHECK(CNET_set_handler(EV_MOBILITY, mobility, 0));
    CNET_start_timer(EV_MOBILITY, USEC_PER_STEP, 0);
}
