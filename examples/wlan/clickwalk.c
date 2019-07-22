#include <cnet.h>
#include <string.h>
#include <stdlib.h>

#define	DELTA	20

static EVENT_HANDLER(click)
{
    CnetPosition	posnow;
    int			dx=0, dy=0;

    CHECK(CNET_get_position(&posnow, NULL));
    switch ((int)ev) {
	case EV_DEBUG1 :	dx = -DELTA;	break;
	case EV_DEBUG2 :	dy = -DELTA;	break;
	case EV_DEBUG3 :	dy =  DELTA;	break;
	case EV_DEBUG4 :	dx =  DELTA;	break;
    }
    posnow.x	+= dx;
    posnow.y	+= dy;
    CHECK(CNET_set_position(posnow));
}

/* ----------------------------------------------------------------------- */

void init_clickwalk(void)
{
    if(nodeinfo.nodenumber == 0)
      fprintf(stdout, "open any node and use its debug buttons to naviagte\n");
    CHECK(CNET_set_debug_string( EV_DEBUG1, "left"));
    CHECK(CNET_set_debug_string( EV_DEBUG2, "up"));
    CHECK(CNET_set_debug_string( EV_DEBUG3, "down"));
    CHECK(CNET_set_debug_string( EV_DEBUG4, "right"));

    CHECK(CNET_set_handler(EV_DEBUG1, click, 0));
    CHECK(CNET_set_handler(EV_DEBUG2, click, 0));
    CHECK(CNET_set_handler(EV_DEBUG3, click, 0));
    CHECK(CNET_set_handler(EV_DEBUG4, click, 0));
}
