#include <cnet.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
//include math header
#include <math.h>

/*
# ------------------------------------------------------------
# Copyright Notice:
#    Copyright by CMPUT 313, U. of Alberta, course instructor (E. Elmallah).
#    All rights reserved. Do not post any part on a publicly-available Web site.
#
#
# Purpose:
#     This file implements a local area network (LAN) protocol similar to
#     the Ethernet protocol. The protocol has the following features:
#     - When a transmitted frame collides, the protocol uses an exponential
#       backing off procedure to schedule a retransmission.
#     - MAX_BACKOFF determines the maximum value of the backoff counter.
#       After this number, a frame is dropped.
#     - When 'CS_FLAG= 0', the protocol relies only on the backing off
#       procedure without using carrier sensing (i.e., function
#	CNET_carrier_sense is not used). Else, when 'CS_FLAG= 1', the protocol
#	uses both carrier sensing and backing off before transmitting
#	(or re-transmitting) each frame.
#      
# Implementation remarks:
#     - Struct LANC holds the following key values for a LAN controller:
#       - lasttimer:  the timer ID of the last set timeout period
#	- link: the link number on which the LAN controller is installed
#	- f, fLen, fTime: when not idle, the controller is busy transmitting
#	     	   	  frame 'f', of length 'fLen' bytes, that takes
#			  'fTime' (usecs) to transmit on the LAN.
#			  'fTime' (in usecs) is obtained by dividing
#			  (8 * fLen) by
#			  (linkinfo[lanc.link].bandwidth/1000,000.00
#       - cs_flag: if 0, the controller does not use carrier sensing before
#	           transmitting (or re-transmitting) frame 'f'
#       - state: either LC_idle, LC_busy, or LC_collided
#       - backoff: the range of the backoff counter is [0, MAX_BACKOFF]
#	- backoff_rn: a random number in the range [0, 2^backoff]
#
#     - The program makes heavy use of the data field in CNET timers, e.g.:
#       - Before starting (or, continuing) a carrier sensing task, we
#	  start a timer with data = EP_cs. On expiry, the timer handler
#	  calls function LANC_manager with entry point= data= EP_cs.
#       - During backing off, we start a timer with data= EP_backoff.
#	  On expiry, the timer handler calls function LANC_manager with
#	  entry point= data= EP_backoff
#	- During frame transmission, we start a timer with data=
#	  EP_frame_transmit. On expiry, the timer handler calls function
#	  LANC_manager with entry point= data= EP_frame_transmit
#
#     - Function LANC_manager includes most of the protocol's logic.
#       It has the following entry points. The code for handling each
#	entry point is designed to be an independent block (no block
#	overlapping):
#	- EP_app: used by application_ready to request the transmission of
#	          a new frame
#	- EP_collision: used for invoking LANC_manager after the collision
#	                handler detects a collision while the controller
#			is not idle. 	  
#       - EP_cs:  used to begin (or continue) carrier sensing (if any)
#	          followed by frame transmission
#       - EP_backoff: used after a backing off timer expires
#	- EP_frame_transmit: used after a frame is transmitted and, the
#	                     protocol has waited for a frame transmission
#			     time without hearing a collision. The protocol
#			     assumes that the frame has been successfully
#			     transmitted.
#
#    - Variable 'lanc' (of type struct LANC) holds the current controller's
#      information. Variable 'prev-lanc' is intended to hold a 'previous'
#      copy of 'lanc'. If the two structs differ in some value of interest
#      then one can print debugging information.
#
#    - Variable 'lanc_stats' (of type struct LANC_STATS) is intended to
#      hold protocol statistics.
#
#    - Printing routines (e.g., LANC_print, and LANC_STATS_print) takes
#      a 'prefix' argument that is printed on every line to simplify using
#      'grep' to select particular lines from large output files. 
#      
#    - When transmitting a frame on a LAN, care should be taken to
#      transmit frames of length at least LAN_MINPACKET bytes each.
#      So,
#      - computing checksums,
#      - invoking CNET_write_physical, and
#      - verifying checksums
#      should be done on frames of length at least LAN_MINPACKET.
#
#      This can be done (easily) if struct 'msg' is the last field in a frame,
#      and it is padded with zeros (when the stored message is short).
#      In such cases, the frame length that should be dealt with is:
#            (fLen >= LAN_MINPACKET)? fLen: LAN_MINPACKET
#      (rather than the original length 'fLen')
#
# Remarks:
#     - When all frames use the broadcast address as the destination address
#       and the GUI is not used, CNET reports zeros in the following fields:
#	- msgs_received (in struct CnetNodeStats)
#	- rx_frames     (in struct CnetLinkStats)
#	- rx_frames_corrupted, rx_frames_collisions (in struct CnetLinkStats)
#	The stats collected by the program should be more accurate.
#	 
# Tips:
#	- function CNET_rand() returns an integer in the range [0, LONG_MAX]
#	  (LONG_MAX is defined in 'limits.h')
#     
#     IMPORTANT:
#     - Run with "-Q" since the MAC provides unreliable, connectionless
#       service (frames can be lost silently)
#
# Status:
#     - There is only one instance of the LANC struct, so the protocol
#       handles a node with one LAN controller.
#
#Acknowledgement:
#       All code from the instructor is used without any change
#
# ------------------------------------------------------------ 
*/

#define FRAME_HEADER_SIZE  (sizeof(FRAME) - sizeof(MSG))
#define FRAME_SIZE(f)      (FRAME_HEADER_SIZE + (f).msgLen) //Don't use 'f.msgLen'

#define LAN_SLOTTIME   52	   // microseconds
#define LAN_TINYSLOT    5	   // microseconds (a negligible delay)
#define MAX_BACKOFF     7          // drop a pkt after MAX_BACKOFF trials

typedef enum { LC_idle, LC_busy, LC_collided } LC_STATE;
char *LC_STATE_str[]= { "LC_idle", "LC_busy", "LC_collided" };

enum { EP_app, EP_collision, EP_cs, EP_backoff, EP_frame_transmit };
char *EP_str[] = { "EP_app", "EP_collision", "EP_cs", "EP_backoff",
     	       	    "EP_frame_transmit" };
		      	   
typedef struct { char data[MAX_MESSAGE_SIZE]; } MSG;

typedef struct {
    CnetNICaddr	 destNIC, srcNIC;
    CnetAddr     dest, src;
    size_t	 msgLen;       	// the length of the msg field only
    int          checksum;  	// if (frame length < LAN_MINPACKET)
    		 		//     compute checksum of a frame padded
				//     with zeros
				// else compute checksum over the whole frame
    MSG          msg;		// the msg field must come last to transmit
    		 		// a padded frame
} FRAME;

// a LANC struct stores all key information of a single LAN controller
//
typedef struct {
    CnetTimerID	 lasttimer;
    int		 link;
    FRAME	 f;
    size_t	 fLen;
    int		 fTime;			// frame transmission time (usec)
    int		 cs_flag;		// carrier sense flag
    LC_STATE	 state;
    int		 backoff, backoff_rn;
} LANC;

// a LANC_STATS stores some key statistics about the protocol
//
typedef struct {
    int	   tx_frames;		// number of original frames attempted
    int	   success_frames;	// number of frames we think have succeeded
    int	   dropped_frames;	// number of frames dropped
    int	   rx_frames;		// number of frames correctly received
} LANC_STATS;
 
LANC        lanc;	   // only one LAN controller is supported for now
LANC        prev_lanc;     // used for debugging
LANC_STATS  lanc_stats;    // holds protocol stats

int   CS_FLAG;             // 0 =  no carrier sensing 
// ------------------------------------------------------------
// make_frame - compose a frame with the specified parameters.
//     The function clears struct msg before storing any data
//     To initialize a frame use: make_frame(f, -1 , NULL, 0)
//

int  make_frame (FRAME *f, CnetAddr dest, MSG *msg, size_t msgLen)
{
    size_t  fLen;		// frame length

    memcpy ( f->destNIC, NICADDR_BCAST, sizeof(CnetNICaddr) );
    memcpy ( f->srcNIC,  NICADDR_ZERO,  sizeof(CnetNICaddr) );

    f->dest = dest; f->src = nodeinfo.address;
    f->msgLen = msgLen;

    memset ( &(f->msg), 0, sizeof(MSG) );
    if (msg != NULL)  memcpy( &(f->msg), msg, (int)msgLen );

    // Note. Normally, cnet does not report corrupted frames to the receiving
    // host ('-E' overrides this behaviour). So, the sender should include an
    // error detection code.

    f->checksum = 0;
    fLen = FRAME_SIZE(*f);		// header + msg
    f->checksum =  CNET_ccitt( (unsigned char *)f,
    		   	(int) (fLen >= LAN_MINPACKET)? fLen: LAN_MINPACKET);
    return (fLen);
}

// ------------------------------------------------------------
// FRAME_init - use make_frame() with suitable values to initialize a frame
//
void FRAME_init (FRAME *f) { make_frame (f, -1 , NULL, 0);  } 

void FRAME_print (char *prefix, FRAME *f) {
    char    destNIC[20], srcNIC[20];

    CNET_format_nicaddr (destNIC, f->destNIC);
    CNET_format_nicaddr (srcNIC,  f->srcNIC);

    // printf ("(destNIC= %s, srcNIC= %s) \n", destNIC, srcNIC);
    printf ("%s %s: (dest= %d, src= %d, msgLen= %d) \n",
    	     prefix, nodeinfo.nodename, f->dest, f->src, f->msgLen);
}

// ------------------------------------------------------------
// LANC_init - initialize a LAN controller installed on the specified 'link'
//
void LANC_init (int link) {

    assert (linkinfo[link].linktype == LT_LAN ||
    	    linkinfo[link].linktype == LT_WLAN );

    lanc.lasttimer= NULLTIMER;
    lanc.link= link;
    lanc.state = LC_idle; lanc.cs_flag= CS_FLAG;
    lanc.backoff = lanc.backoff_rn = 0;
    FRAME_init (& lanc.f);
    lanc.fLen= FRAME_SIZE(lanc.f); lanc.fTime= LAN_SLOTTIME;
}

// ------------------------------

void LANC_print (char *prefix) {
    CnetData  timerData;

    if (lanc.lasttimer == NULLTIMER ) { timerData= -1; }
    else { CHECK( CNET_timer_data (lanc.lasttimer, &timerData) ); }

    printf ("%s --------------------\n", prefix);
    printf ("%s %s: LAN controller (time= %s, link= %d, cs_flag= %d)\n",
    	     prefix, nodeinfo.nodename, 
	     CNET_format64 (nodeinfo.time_in_usec), lanc.link, lanc.cs_flag );
    printf ("%s %s: LC_state= %s (Backoff= %d, Backoff_rn= %d) \n",
    	     prefix, nodeinfo.nodename,
    	     LC_STATE_str[lanc.state], lanc.backoff, lanc.backoff_rn );
    printf ("%s %s: timerData= %s, frame_time= %d \n",
    	     prefix, nodeinfo.nodename,
	     (timerData >= 0)? EP_str[timerData] : "NULL",
	     lanc.fTime);
    FRAME_print (prefix, & lanc.f); printf("\n");
} 
// ------------------------------
void LANC_STATS_init () {
     lanc_stats.tx_frames = 0;
     lanc_stats.success_frames = 0;
     lanc_stats.dropped_frames = 0;
     lanc_stats.rx_frames = 0;
}

void LANC_STATS_print (char *prefix) {

   printf ("%s --------------------\n", prefix);

   printf ("%s %s: LANC statistics (time= %s)\n",
    	    prefix, nodeinfo.nodename, CNET_format64 (nodeinfo.time_in_usec));
   printf ("%s %s: tx_frames= %d, success_frames= %d, dropped_frames= %d \n",
            prefix, nodeinfo.nodename, lanc_stats.tx_frames,
	    lanc_stats.success_frames, lanc_stats.dropped_frames);
   printf ("%s %s: rx_frames= %d \n",
            prefix, nodeinfo.nodename, lanc_stats.rx_frames);

   printf("\n");
}

// ------------------------------

void LANC_start_timer (CnetTime interval, CnetData data) {
    // Avoid starting a timer with a zero value
    // 
    if (interval > 0 ) {
        lanc.lasttimer = CNET_start_timer (EV_TIMER1, interval, data);
    }
    else {
        lanc.lasttimer = CNET_start_timer (EV_TIMER1, LAN_TINYSLOT, data);
    }
}

void LANC_stop_timer () {
    if (lanc.lasttimer != NULLTIMER)  {
        CNET_stop_timer (lanc.lasttimer); lanc.lasttimer = NULLTIMER;
    }
}
// ------------------------------------------------------------
void LANC_manager_debug () {
    // print debugging information if there is a key change in
    // the lanc struct

    if ( (prev_lanc.state      != lanc.state)   ||
         (prev_lanc.backoff    != lanc.backoff) ||
	 (prev_lanc.backoff_rn != lanc.backoff_rn) ) LANC_print ("debug: ");
    prev_lanc = lanc; 
}
// ------------------------------------------------------------
void LANC_manager (int ePoint)
{
    int      retVal,expo;
    size_t   fLen;
    double   rn;


    if (ePoint==EP_app){
        assert(lanc.state==LC_idle);
        assert(lanc.f.dest!=nodeinfo.address);
        lanc.state=LC_busy;

        LANC_start_timer ( LAN_TINYSLOT,EP_cs );
        return;
    }


    if(ePoint==EP_collision){
        assert((lanc.state==LC_busy) || (lanc.state==LC_collided));
        assert(lanc.lasttimer!= NULLTIMER);
        printf("\n");
        printf("debug: LANC_manager : collision\n");
        LANC_stop_timer ();
        if(lanc.backoff>=MAX_BACKOFF){
            //drop the current frame in LANC
            lanc_stats.dropped_frames++;
            FRAME_init (& lanc.f);
            //reset the state of the LANC
            lanc.state = LC_idle;
            //re-enable the AL in all nodes
            CNET_enable_application(ALLNODES);
            return;
        }else{
            lanc.state=LC_collided;
            lanc.backoff++;
            lanc.backoff_rn=pow(2,lanc.backoff);
            rn=CNET_rand()%lanc.backoff_rn;
            retVal=rn*LAN_SLOTTIME;
            LANC_start_timer ( retVal,EP_backoff );
            return;
        }
    }


    if(ePoint==EP_cs){
        if (CS_FLAG == 1){
            expo=CNET_carrier_sense(lanc.link);
            //lanc.state==LC_busy
            if(expo==1){
                LANC_start_timer ( LAN_TINYSLOT,EP_cs);
                CNET_carrier_sense(lanc.link);
                return;
            }
        }
        if(CS_FLAG == 0||(CS_FLAG ==1&&expo!=1)){
            //adjust the length to LAN_MINPACKET if the frame is too short
            if(lanc.fLen<LAN_MINPACKET){
                lanc.fLen=LAN_MINPACKET;
            }
            fLen=lanc.fLen;
            
            CHECK(CNET_write_physical(lanc.link,&lanc.f,&fLen));
          
            LANC_start_timer (lanc.fTime,EP_frame_transmit);
            return;
        }

    }
 
    if(ePoint==EP_backoff){
        LANC_start_timer ( LAN_TINYSLOT,EP_cs);
        return;
    }

    if(ePoint==EP_frame_transmit){
        CNET_enable_application(ALLNODES);
        lanc_stats.success_frames++;
        LANC_init (lanc.link);
        return;
    }

    printf("Error: unknown entry point\n");
  
    LANC_manager_debug ();
    return;
   

}

// ------------------------------------------------------------
void NODE_print_stats (char *prefix) {

   CnetNodeStats  stats;

   CNET_get_nodestats (&stats);

   printf ("%s --------------------\n", prefix);

   printf ("%s %s: Node statistics (time= %s, nEvents= %ld, nErrors= %ld)\n",
    	    prefix, nodeinfo.nodename, CNET_format64 (nodeinfo.time_in_usec),
	     stats.nevents, stats.nerrors );
   printf ("%s %s: msgs generated= %ld, msgs received= %ld \n",
            prefix, nodeinfo.nodename,
	    stats.msgs_generated, stats.msgs_received );

   printf("\n");
}

// ------------------------------
void LINK_print_stats (char *prefix, int link) {

   CnetLinkStats  stats;

   CNET_get_linkstats (link, &stats);

   printf ("%s --------------------\n", prefix);
   printf ("%s %s: Link (= %d) statistics (time= %s)\n",
    	    prefix, nodeinfo.nodename, link, 
	    CNET_format64 (nodeinfo.time_in_usec) );
   printf ("%s %s: tx_frames= %ld, rx_frames= %ld \n",
            prefix, nodeinfo.nodename, stats.tx_frames, stats.rx_frames );
   printf ("%s %s: rx_frames_corrupted= %ld, rx_frames_collisions= %ld \n",
            prefix, nodeinfo.nodename,
	    stats.rx_frames_corrupted, stats.rx_frames_collisions );

   printf("\n");
}

// ------------------------------------------------------------

EVENT_HANDLER(application_ready)
{
    CnetAddr  destaddr;
    size_t    msgLen, fLen;
    MSG	      lastMsg;
    FRAME     lastFrame;

    msgLen  = sizeof(MSG);
    CHECK(CNET_read_application(&destaddr, &lastMsg, &msgLen));
    CNET_disable_application (ALLNODES);

    // make frame for storage
    fLen= make_frame(&lastFrame, destaddr, &lastMsg, msgLen);

    // store frame information in LAN controller (store a complete frame
    // of size 'sizeof(FRAME)' with struct msg padded with zeros)
    //
    assert (fLen <= LAN_MAXPACKET );
    lanc.fLen= fLen;
    memcpy ( &lanc.f, &lastFrame, sizeof(FRAME) );

    lanc.fTime= (int) (8.0 * lanc.fLen) /
	              (linkinfo[lanc.link].bandwidth / 1000000.00);

    // Request the LAN manager to transmit the frame (with struct msg possibly
    // padded with zeros)
    //
    assert ( lanc.state == LC_idle );
    lanc_stats.tx_frames++;    // increment number of frames attempted
    LANC_manager (EP_app);
}
// ------------------------------------------------------------
EVENT_HANDLER(physical_ready)
{
    FRAME        f;
    size_t	 msgLen, fLen;
    int          link, stored_checksum, computed_checksum;

    fLen         = sizeof(FRAME);
    CHECK(CNET_read_physical(&link, &f, &fLen));

    // Ignore if the frame is not for this node
    //
    if ( f.dest != nodeinfo.address ) return;

    // else, check the integrity of the received frame
    //
    stored_checksum    = f.checksum;
    f.checksum  = 0;
    computed_checksum= CNET_ccitt((unsigned char *)&f,
    		       (int) (fLen >= LAN_MINPACKET)? fLen: LAN_MINPACKET);

    if ( stored_checksum != computed_checksum) {
        printf("\t\tBAD checksum - frame ignored (stored= %d,computed= %d)\n",
	        stored_checksum, computed_checksum);
        return;
    }

    // else, deliver
    //
    msgLen= f.msgLen;
    lanc_stats.rx_frames++;   // increment number of received frames
    CHECK( CNET_write_application( &f.msg, &msgLen) );
    
}


// ------------------------------------------------------------

// Recall: 'EVENT_HANDLER(name)' expands to
//          void name(CnetEvent ev, CnetTimerID timer, CnetData data)

static EVENT_HANDLER(LANC_timeouts) 
{
    lanc.lasttimer= NULLTIMER;  LANC_manager (data);

    
} 

static EVENT_HANDLER(LANC_collisions) 
{
    if ( (lanc.state == LC_busy) || (lanc.state == LC_collided) ) {
        LANC_manager (EP_collision);   // inform manager
    }
    else {
        printf ("[%s] LANC_collisions: caught a LAN collision \n",
	         nodeinfo.nodename);
    }
} 

static EVENT_HANDLER(LANC_showstate)
{
    LANC_print("showstate: ");
    LANC_STATS_print ("showstate: ");
}

// ------------------------------
static EVENT_HANDLER(LANC_periodic)
{
    NODE_print_stats ("periodic: ");
    LINK_print_stats ("periodic: ", lanc.link);
    LANC_STATS_print ("periodic: ");
}

// ------------------------------
static EVENT_HANDLER(LANC_shutdown)
{
    NODE_print_stats ("shutdown: ");
    LINK_print_stats ("shutdown: ", lanc.link);
    LANC_STATS_print ("shutdown: ");
}
// ------------------------------------------------------------

EVENT_HANDLER(reboot_node)
{

    if ( (linkinfo[1].linktype != LT_LAN ) &&
    	 (linkinfo[1].linktype != LT_WLAN) ) {
	fprintf(stderr,"link 1 is neither a LAN nor a WLAN!\n");
	exit(1);
    }

    if (nodeinfo.nodetype != NT_ROUTER) {
         CHECK(CNET_set_handler( EV_APPLICATIONREADY, application_ready, 0));
    }

    CHECK(CNET_set_handler( EV_PHYSICALREADY,    physical_ready, 0));
    CHECK(CNET_set_handler( EV_TIMER1,           LANC_timeouts, 0));
    CHECK(CNET_set_handler( EV_FRAMECOLLISION,   LANC_collisions, 0));

    CHECK(CNET_set_handler( EV_SHUTDOWN,	 LANC_shutdown, 0));

    CHECK(CNET_set_handler( EV_PERIODIC,         LANC_periodic, 0));
    CHECK(CNET_set_handler( EV_DEBUG0,           LANC_showstate, 0));
    CHECK(CNET_set_debug_string( EV_DEBUG0, "LANC State"));

    CNET_enable_application(ALLNODES);

    CNET_srand (nodeinfo.time_of_day.sec + nodeinfo.nodenumber);

    // LANC specific initialization
    //

    CS_FLAG = atoi (CNET_getvar ("cs_flag"));
    assert ( CS_FLAG == 0 || CS_FLAG == 1);

    LANC_init (1);       // initialize LAN controller

    LANC_STATS_init ();  // initialize protocol's stats

    prev_lanc= lanc;	 // 'prev_lanc' is used for debugging to detect
    	       		 // changes of certain key values in the 'lanc'
}
