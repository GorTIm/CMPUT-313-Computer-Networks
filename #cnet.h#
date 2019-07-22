/*  This is the standard header file for the cnet simulator.
    DO NOT COPY this file, simply #include it into your cnet protocol files.

    When your protocols are compiled, the following preprocessor constants
    specify what features are available:

	CNET_PROVIDES_APPLICATION_LAYER
	CNET_PROVIDES_WANS
	CNET_PROVIDES_LANS
	CNET_PROVIDES_WLANS
	CNET_PROVIDES_MOBILITY
	CNET_PROVIDES_KEYBOARD

    and for the iPod:

	CNET4IPOD
	CNET4IPOD_RAND=value

    Tue Mar  1 04:42:49 AWST 2016
 */

// The cnet network simulator (v3.3.4)
// Copyright (C) 1992-onwards,  Chris.McDonald@uwa.edu.au
// Released under the GNU General Public License (GPL) version 2.

#ifndef	_CNET_H_
#define	_CNET_H_		1

#define	CNET_VERSION		"cnet v3.3.4"
#define AUTHOR_EMAIL		"Chris.McDonald@uwa.edu.au"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

// -------------- BASIC DATATYPES USED IN cnet AND ITS APIs -------------

typedef int32_t			CnetAddr;
typedef char *			CnetColour;
typedef	intptr_t		CnetData;
typedef	void *			CnetRandom;
typedef	int64_t			CnetTime;
typedef	int32_t			CnetTimerID;

typedef struct {
    double		x, y, z;	// z-coordinate is currently unused
} CnetPosition;

#define	PRIdCnetTime		PRId64

#define	ALLNODES		((CnetAddr)(-1))
#define	NULLTIMER		((CnetTimerID)0)

// -------------- NODE DEFINITIONS ---------------------------------------

typedef enum { NT_HOST=0, NT_ROUTER, NT_MOBILE, NT_ACCESSPOINT } CnetNodeType;

#if	CNET_PROVIDES_APPLICATION_LAYER
#define	HAVE_APPLICATIONLAYER()	(nodeinfo.nodetype == NT_HOST || \
				 nodeinfo.nodetype == NT_MOBILE)
#else
#define	HAVE_APPLICATIONLAYER()	false
#endif

#define	HAVE_KEYBOARD()		(nodeinfo.nodetype == NT_HOST || \
			 	 nodeinfo.nodetype == NT_MOBILE)
#define	HAVE_MOBILITY()	 	(nodeinfo.nodetype == NT_MOBILE)


#define	MAX_NODENAME_LEN	32	// including terminating '\0'

typedef	struct {
    CnetNodeType nodetype;		// one of NT_* 
    int		nodenumber;		// Ranging from 0..NNODES-1
    CnetAddr	address;		// Possibly != nodenumber
    char	nodename[MAX_NODENAME_LEN];
    int		nlinks;			// number of physical links

    CnetTime	time_in_usec;		// usecs since node's last EV_REBOOT
    struct {
	long	sec;			// seconds
	long	usec;			// microseconds
    } time_of_day;			// the node's wall-clock time

#if	CNET_PROVIDES_APPLICATION_LAYER
    CnetTime	messagerate;		// rate of AL msg generation (usecs)
    int		minmessagesize;		// min size (in bytes) of new AL msgs
    int 	maxmessagesize;		// max size (in bytes) of new AL msgs
#endif
} CnetNodeInfo;

extern	CnetNodeInfo	nodeinfo;

extern	int		NNODES;		// divulged with cnet -N, otherwise 0

// -------------- LINK DEFINITIONS ---------------------------------------

typedef enum { LT_LOOPBACK, LT_WAN, LT_LAN, LT_WLAN } CnetLinkType;

#define	LOOPBACK_LINK		0
#define	MAX_LINKNAME_LEN	16	// len including terminating NULL
#define	LEN_NICADDR		6	// in bytes

typedef	unsigned char	CnetNICaddr[LEN_NICADDR];

typedef struct {
    CnetLinkType  linktype;		// one of LT_* 
    char	  linkname[MAX_LINKNAME_LEN];
    bool	  linkup;		// true iff functioning & not severed
    CnetNICaddr	  nicaddr;		// for LT_LAN and LT_WLAN links

    int           bandwidth;		// in bits per second
    int           mtu;			// maximum transmit unit - in bytes
    CnetTime	  propagationdelay;	// in usecs, for LT_WAN and LT_LAN
    int           costperbyte;		// unitless
    int           costperframe;		// unitless
} CnetLinkInfo;

extern  CnetLinkInfo	*linkinfo; 	// indexed with 0..nodeinfo.nlinks

extern	CnetNICaddr	NICADDR_ZERO;	// The Nicaddr of all zero bits
extern	CnetNICaddr	NICADDR_BCAST;	// The Nicaddr of all one  bits


#if	CNET_PROVIDES_WANS

#define DRAWFRAME_TEXTLEN	16	// len including terminating '\0'
#define DRAWFRAME_MAXFIELDS	8

typedef struct {
    size_t	len;				// length of frame presented
    char        *frame;				// a pointer to the frame
    int		nfields;			// number of fields to draw
    CnetColour	colours[DRAWFRAME_MAXFIELDS];	// with these colours
    int         pixels[DRAWFRAME_MAXFIELDS];	// and pixels lengths
    char        text[DRAWFRAME_TEXTLEN];	// text centred on moving frame
} CnetDrawFrame;
#endif

#if	CNET_PROVIDES_LANS
#define	LAN_MINDATA	46		// in bytes
#define	LAN_MAXDATA	1500		// in bytes
#define	LAN_MINPACKET	64		// in bytes
#define	LAN_MAXPACKET	1518		// in bytes
#define	LAN_MTU		LAN_MAXPACKET
#endif

#if	CNET_PROVIDES_WLANS
#define	WLAN_MINDATA	0		// in bytes
#define	WLAN_MINPACKET	34		// in bytes

#if	defined(CNET4IPOD)
#define	WLAN_MAXDATA	1472		// in bytes
#else
#define	WLAN_MAXDATA	2312		// in bytes
#endif

#define	WLAN_MAXPACKET	2346		// in bytes

#define	WLAN_MTU	WLAN_MAXPACKET
#endif


// --------------- API FOR EVENT HANDLING --------------------------------

typedef enum {
    EV_NULL = 0,	// this event is never raised.
    EV_REBOOT,		// raised as reboot_node(EV_REBOOT, ... , ...)
    EV_SHUTDOWN,	// raised before cnet is (cleanly) terminated
    EV_PHYSICALREADY,	// raised when a frame arrives at the PL

#if	CNET_PROVIDES_APPLICATION_LAYER
    EV_APPLICATIONREADY,// raised when AL has a msg ready for delivery
#endif
#if	CNET_PROVIDES_KEYBOARD
    EV_KEYBOARDREADY,	// raised when a line entered from the keyboard
#endif

#if	CNET_PROVIDES_WANS
    EV_DRAWFRAME,	// raised when a WAN frame is to be drawn
    EV_LINKSTATE,	// raised when a WAN link changes state
#endif
#if	CNET_PROVIDES_LANS || CNET_PROVIDES_WLANS
    EV_FRAMECOLLISION,	// raised when a frame collides on LT_LAN or LT_WLAN
#endif

    EV_PERIODIC,	// raised periodically if -f option provided
    EV_UPDATEGUI,	// raised when GUI is updated if -u option provided

    EV_DEBUG0,	EV_DEBUG1,	EV_DEBUG2,	EV_DEBUG3,	EV_DEBUG4,
    EV_TIMER0,	EV_TIMER1,	EV_TIMER2,	EV_TIMER3,	EV_TIMER4,	
    EV_TIMER5,	EV_TIMER6,	EV_TIMER7,	EV_TIMER8,	EV_TIMER9
} CnetEvent;

#define	N_CNET_EVENTS	(((int)EV_TIMER9) + 1)
#define	N_CNET_DEBUGS	 5
#define	N_CNET_TIMERS	10

extern	const char	*cnet_evname[N_CNET_EVENTS]; 	// e.g. "EV_REBOOT"

#define	IS_DEBUG(ev)	((int)EV_DEBUG0<=(int)ev && (int)ev<=(int)EV_DEBUG4)
#define	IS_TIMER(ev)	((int)EV_TIMER0<=(int)ev && (int)ev<=(int)EV_TIMER9)

#define	EVENT_HANDLER(name)	\
		void name(CnetEvent ev, CnetTimerID timer, CnetData data)

	/* EVENT_HANDLER simply provides the convenience of being able to
	   declare or define an event handler and "hide" the actual
	   parameters that are always required. */

extern	int CNET_set_handler(CnetEvent ev,
			    void (*handler)(CnetEvent, CnetTimerID, CnetData),
			    CnetData data);

	/* Register handler as the void ``returning'' function to be invoked
	   when the event ev occurs. When the handler is invoked, it will
	   be passed the event, a timer and the user-specified data. */

extern	int CNET_get_handler(CnetEvent ev,
			    void (*handler)(CnetEvent, CnetTimerID, CnetData),
			    CnetData *data);

	/* Obtains the address of the void ``returning'' function which will
	   be invoked when the indicated event occurs, together with the
	   user-specified data value that will be passed to the handler. */

extern CnetEvent CNET_unused_debug_event(void);

	/* Return the first available event, EV_DEBUG*, that does not have
	   an event handler. This function does not 'reserve' the event, and
	   so will typically be followed by a call to CNET_set_handler().
	   If all EV_DEBUG* events have handlers, EV_NULL is returned.
	 */

extern	int CNET_set_debug_string(CnetEvent ev, const char *str);

	/* Changes the string on the required debug button,
	   indicated by the event EV_DEBUG0..EV_DEBUG4.
	   Passing the NULL or empty string will remove the button. */


// --------------- API FOR ERROR HANDLING --------------------------------

typedef enum {
    ER_OK = 0,		// No error
    ER_NOTSUPPORTED,	// Invalid operation for this node or link type
    ER_NOTREADY,	// Function called when service not available
    ER_TOOBUSY,		// Function is too busy/congested to handle request

    ER_BADALLOC,	// Allocation of dynamic memory failed
    ER_BADARG,		// Invalid argument passed to a function
    ER_BADEVENT,	// Invalid event passed to a function
    ER_BADLINK,		// Invalid link number passed to a function
    ER_BADNODE,        	// Invalid node passed to a function
    ER_BADSIZE,		// Indicated length is of incorrect size
    ER_BADTIMERID,	// Invalid CnetTimerID passed to a function
    ER_CORRUPTFRAME,	// Attempt to transfer a corrupt data frame
    ER_LINKDOWN,	// Attempt to transmit on a link that is down

#if	CNET_PROVIDES_APPLICATION_LAYER
    ER_BADSENDER,	// Application Layer given msg from an unknown node
    ER_BADSESSION,	// Application Layer given msg from incorrect session
    ER_DUPLICATEMSG,	// Application Layer given a duplicate msg
    ER_MISSINGMSG,	// Application Layer given msg before previous ones
    ER_NOTFORME,	// Application Layer given a msg for another node
#endif

#if	CNET_PROVIDES_MOBILITY
    ER_BADPOSITION,	// Attempt to move mobile node off the map
    ER_NOBATTERY,	// Battery exhaused on a mobile node
#endif
} CnetError;

extern	CnetError	cnet_errno;
extern	const char	*cnet_errname[];	// e.g. "ER_BADARG"
extern	const char	*cnet_errstr[];
extern	void		CNET_perror(char *prefix);

	/* Most API functions return 0 on success, and a -1 on failure.
	   The reason for any failure is reflected in the global variable
	   cnet_errno.  cnet_errno may also be used as the integer index
	   into the arrays cnet_errname[] and cnet_errstr[] to enable more
	   detailed error reporting.  The function CNET_perror prints its
	   string argument, preceeded by the last known error. */

extern	void CNET_exit(const char *filename, const char *funcnname, int lineno);

	/* Calling this function will exit (all of) cnet, and display the
	   invoking node's name, source file, function, and line where it
	   was called, together with the latest cnet_errno value. */

#define CHECK(call)	do { if((call) != 0) \
			CNET_exit(__FILE__, __func__, __LINE__);} while(false)

	/* By enveloping most calls to cnet's API routines we can get
	   an accurate and immediate report on the location (source file +
	   function name + line number + nodename) and type of each error.
	   These helpful values are passed to cnet's function CNET_exit which,
	   if able, pops up a window highlighting the file and line number
	   of the runtime error. */


// --------------- API FOR EACH NODE'S APPLICATION LAYER -----------------

#if	CNET_PROVIDES_APPLICATION_LAYER

#define	MIN_MESSAGE_SIZE	48	// ... that may be generated by AL
#define	MAX_MESSAGE_SIZE	32768	// ... that may be generated by AL

extern	int CNET_read_application(CnetAddr *dest, void *msg, size_t *len);

	/* On invocation, len will point to an integer indicating the maximum
	   number of bytes that may be copied into msg.  On return, len will
	   point to an integer now indicating the number of bytes copied into
	   msg.  The address of the required destination node is copied
	   into destaddr. */

extern	int CNET_write_application(const void *msg, size_t *len);

	/* Passes a number of bytes, pointed to by msg, ``up to'' the
	   application layer. On invocation, len must point to an integer
	   indicating the number of bytes to be taken from msg.
	   On return, len will point to an integer now indicating the number
	   of bytes accepted by the application layer. */

extern	int CNET_enable_application(CnetAddr destaddr);

	/* Permits the application layer to generate messages for the node
	   with the indicated address. Initially, msg generation for all nodes
	   is disabled and must be enabled to begin the generation of msgs.
	   If destaddr == ALLNODES, message generation will be enabled
	   for all nodes.  */

extern	int CNET_disable_application(CnetAddr destaddr);

	/* Prevents the application layer from generating new messages for the
	   node with the indicated address. This function should be called
	   when a harried node runs out of buffer space, or perhaps while
	   routing information is being gathered.  If destaddr == ALLNODES,
	   message generation will be disabled for all nodes.  */
#endif

// --------------- API FOR PER-NODE AND PER-LINK STATISTICS --------------

typedef struct {
    int64_t	nevents;
    int64_t	nerrors;

#if	CNET_PROVIDES_APPLICATION_LAYER
    int64_t	msgs_generated;
    int64_t	bytes_generated;

    int64_t	msgs_received;
    int64_t	bytes_received;
#endif
} CnetNodeStats;

typedef struct {
    int64_t	tx_frames;
    int64_t	tx_bytes;
    int64_t	tx_cost;

    int64_t	rx_frames;
    int64_t	rx_bytes;
    int64_t	rx_frames_corrupted;
    int64_t	rx_frames_collisions;
} CnetLinkStats;

extern	int CNET_get_nodestats(CnetNodeStats *stats);
extern	int CNET_get_linkstats(int link, CnetLinkStats *stats);

	/* Functions CNET_get_nodestats and CNET_get_linkstats retrieve some
	   statistics about the performance of each node and each of its
	   links.  The "sum" of all of these statistics provide the global
	   statistics maintained by cnet and reported via its GUI and -s.  */

// --------------- API FOR EACH NODE'S PHYSICAL LAYER --------------------

extern	int CNET_write_physical(int link, void *frame, size_t *len);

	/* Passes a number of bytes, pointed to by frame ``down to'' the
	   physical layer which will attempt to deliver them on the indicated
	   link.  Each node has a fixed number of links, the first available
	   link is number 1, the second is number 2, and so on.
	   On invocation, len will point to an integer indicating the number
	   of bytes to be taken from frame. On return, len will point to
	   an integer now indicating the number of bytes accepted by the
	   Physical Layer.
	   As a special case, a node may reliably transmit a frame to itself by
	   requesting the LT_LOOPBACK link (number 0).  */

extern int CNET_write_physical_reliable(int link, void *frame, size_t *len);

	/* CNET_write_physical_reliable() performs an identical function to
	   CNET_write_physical() but will never introduce any corruption or
	   loss (thus providing a reliable datalink layer). */

extern	int CNET_write_direct(CnetAddr destaddr, void *frame, size_t *len);

	/* CNET_write_direct() performs identically to
	   CNET_write_physical_reliable() although the address of the required
	   destination node must be specified (providing a reliable network
	   layer).  All directly written frames arrive on link 1 in the
	   destination.  */

extern	int CNET_read_physical(int *link, void *frame, size_t *len);

	/* Accepts the specified maximum number of bytes from the physical
	   layer, placing them in the address pointed to by frame.
	   On invocation, len will point to an integer indicating the maximum
	   number of bytes that may be copied into frame.  On return, len will
	   point to an integer now indicating the number of bytes taken from
	   the physical layer and link will point to an integer indicating on
	   which link they were received. */

#if	CNET_PROVIDES_LANS || CNET_PROVIDES_WLANS
extern	int CNET_carrier_sense(int link);

	/* Indicates if the indicated LT_LAN or LT_WLAN link is busy -
	   either because the NIC is still transmitting a frame,
	   or because we can sense other transmissions on the shared medium. */
#endif

extern	int CNET_set_nicaddr(int link, CnetNICaddr nicaddr);

	/* Sets the network interface card's addreess for the indicated link
	   Setting a nicaddr is only meaningful for LT_LAN and LT_WLAN links. */

extern	int CNET_format_nicaddr(char *buf, CnetNICaddr nicaddr);
extern	int CNET_parse_nicaddr (CnetNICaddr nicaddr, char *buf);


// --------------- API FOR WLAN LINKS AND MOBILITY -----------------------

#if	CNET_PROVIDES_WLANS

typedef enum { WLAN_SLEEP, WLAN_IDLE, WLAN_TX, WLAN_RX } WLANSTATE;

typedef enum { WLAN_TOOWEAK, WLAN_TOONOISY, WLAN_RECEIVED } WLANRESULT;

typedef struct {
    double	frequency_GHz;
    double	tx_power_dBm;
    double	tx_cable_loss_dBm;
    double	tx_antenna_gain_dBi;

    double	rx_antenna_gain_dBi;
    double	rx_cable_loss_dBm;
    double	rx_sensitivity_dBm;
    double	rx_signal_to_noise_dBm;

    double	sleep_current_mA;
    double	idle_current_mA;
    double	tx_current_mA;
    double	rx_current_mA;
} WLANINFO;

#define	dBm2mW(dBm)	pow(10.0, ((double)dBm) / 10.0)
#define	mW2dBm(mW)	(log10((double)mW) * 10.0)

typedef struct {
    int64_t	 tx_seqno;		// a unique transmission number

    int		 tx_n;			// details about the transmitter
    CnetPosition tx_pos;
    WLANINFO	*tx_info;

    int		 rx_n;			// details about the receiver
    CnetPosition rx_pos;
    WLANINFO	*rx_info;

    double	 rx_strength_dBm;	// calculated strength at receiver
} WLANSIGNAL;

extern	int CNET_set_wlan_model( WLANRESULT (*newmodel)(WLANSIGNAL *sig) );

	/* Register a function to determine the extent and success of WLAN
	   signal propagation. The function will be passed a pointer to a
	   WLANSIGNAL structure whose elements describe the transmitting and
	   receiving nodes and their WLAN links.
	   On return, the function should set the received signal strength
	   (in dBm) and return the success of the signal propagation. */

extern	int CNET_get_wlaninfo(int link, WLANINFO *wlaninfo);
extern	int CNET_set_wlaninfo(int link, WLANINFO *wlaninfo);

	/* LT_WLAN links are modelled on Orinocco PCMCIA Silver/Gold cards.
	   These functions permit transmission and reception characteristics
	   of LT_WLAN links to be examined and modified while the simulation
	   is running.  To change just one of the attributes, get() the WLAN
	   info, change the required attribute, and then set() the WLAN info.
	   All of the attributes may be set in the simulation's topology file,
	   but frequency_GHz may not be set at runtime. */

extern	int CNET_wlan_arrival(int link,double *rx_signal_dBm,double *rx_angle);

	/* CNET_wlan_arrival() provides the characteristics of the most
	   recently arrived signal on a LT_WLAN link. The received signal
	   strength, rx_signal_dBm, is measured in dBm, and the arrival angle,
	   rx_angle, is in radians (0 = 3 o'clock, pi/2 = 12 o'clock). If NULL
	   is presented as any parameter, then the attribute is not reported. */

extern	int CNET_get_wlanstate(int link, WLANSTATE *state);
extern	int CNET_set_wlanstate(int link, WLANSTATE new_state);

	/* A WLAN link may be in one of four states - SLEEP, IDLE, TRANSMIT,
	   or RECEIVE.  It is not possible to set a WLAN link's state to
	   TRANSMIT or RECEIVE - transitions to these states occur
	   automatically, from the IDLE state, whenever a link is asked to
	   transmit a signal, or when a signal arrives.
	   CNET_get_wlanstate() reports the current state, and
	   CNET_set_wlanstate() may set it to either WLAN_SLEEP or WLAN_IDLE */
#endif

// --------------- API FOR MOBILITY AND BATTERY ------------------------------

#if	CNET_PROVIDES_MOBILITY

extern	int	CNET_set_position(CnetPosition new_position);
extern	int	CNET_get_position(CnetPosition *now, CnetPosition *max);
extern	double	CNET_get_mapscale(void);
extern	int	CNET_get_mapmargin(void);

	/* Nodes of type NT_MOBILE may move by setting their new position
	   to anywhere on the main 'map' of the simulation.  cnet places
	   no significance on coordinates or distances other than in
	   assuming that one pixel (x=1) is equivalent to one metre when
	   determining signal propagation for LT_WLAN links.
	   A node may determine its current position, and the maximum size
	   of the 'map', by calling CNET_get_position().  If NULL is
	   presented as any parameter, then the attribute is not reported. */

extern	int CNET_set_wlancolour(int link, CnetColour colour);

	/* When using the graphical interface, the colour of the WLAN signal
	   may be changed from its default with CNET_set_wlancolour()  */

#define	DEFAULT_BATTERY_volts		1.5
#define	DEFAULT_BATTERY_mAH		2000.0

extern	int CNET_set_battery(double new_volts, double new_mAH);
extern	int CNET_get_battery(double *volts, double *mAH);

#endif


// --------------- API FOR TIMERS ----------------------------------------

extern CnetEvent CNET_unused_timer_event(void);

	/* Return the first available event, EV_TIMER*, that does not have
	   an event handler. This function does not 'reserve' the event, and
	   so will typically be followed by a call to CNET_set_handler().
	   If all EV_TIMER* events have handlers, EV_NULL is returned.
	 */

extern CnetTimerID CNET_start_timer(CnetEvent ev,CnetTime usecs,CnetData data);

	/* Requests that a new timer be created which will expire in the
	   indicated number of microseconds.  When the timer expires, the
	   handler for the indicated event, EV_TIMER0..EV_TIMER9, will be
	   called together with the user-specified data value.
	   A unique timerID is returned to distinguish this newly created
	   timer from all others. This timerID may later be used in
	   subsequent calls to CNET_stop_timer() or CNET_timer_data().
	   On failure, NULLTIMERID is returned.  */

extern	int CNET_stop_timer(CnetTimerID timerID);

	/* Requests that the timer identified by the timerID be stopped. */

extern	int CNET_timer_data(CnetTimerID timerID, CnetData *data);

	/* Allows the CnetData value identified by the timerID
	   to be recovered. The timer is not stopped. */


// --------------- API FOR PSEUDO-RANDOM NUMBERS -------------------------

extern	void CNET_srand(unsigned int seed);
extern	long CNET_rand(void);

	/*  Each node has its own random number generator so that the
	    frequency of requests for random numbers in one node does not
	    interfere with those of other nodes.  CNET_srand() seeds the
	    current node's random number generator and CNET_rand()
	    returns the next number in the sequence on [0..MAXLONG]. */

extern	CnetRandom	CNET_newrand(unsigned int seed);
extern	long		CNET_nextrand(CnetRandom);

	/*  Each node may employ multiple independent random sequences.
	    CNET_newrand() returns a pointer to a new sequence, and
	    CNET_nextrand() receives that pointer to return the next
	    independent random number in that sequence on [0..MAXLONG]. */


// --------------- API FOR CHECKSUM/CRC FUNCTIONS ------------------------

extern	uint16_t	CNET_ccitt(unsigned char *addr, size_t nbytes);
extern	uint16_t	CNET_crc16(unsigned char *addr, size_t nbytes);
extern	uint32_t	CNET_crc32(unsigned char *addr, size_t nbytes);
extern	int32_t		CNET_IP_checksum(unsigned short *addr, size_t nbytes);


// --------------- API FOR EVENT TRACING ---------------------------------

#define	TE_NOEVENTS		(0)
#define	TE_ALLEVENTS		(0xFFFFFFFF)
#define	TE_REBOOT		(1<<(int)EV_REBOOT)
#define	TE_SHUTDOWN		(1<<(int)EV_SHUTDOWN)
#define	TE_PHYSICALREADY	(1<<(int)EV_PHYSICALREADY)
#define	TE_PERIODIC		(1<<(int)EV_PERIODIC)

#if	CNET_PROVIDES_APPLICATION_LAYER
#define	TE_APPLICATIONREADY	(1<<(int)EV_APPLICATIONREADY)
#endif
#if	CNET_PROVIDES_KEYBOARD
#define	TE_KEYBOARDREADY	(1<<(int)EV_KEYBOARDREADY)
#endif
#if	CNET_PROVIDES_WAN
#define	TE_DRAWFRAME		(1<<(int)EV_DRAWFRAME)
#endif

#if	!defined(CNET4IPOD)
#define	TE_FRAMECOLLISION	(1<<(int)EV_FRAMECOLLISION)
#define	TE_LINKSTATE		(1<<(int)EV_LINKSTATE)
#define	TE_UPDATEGUI		(1<<(int)EV_UPDATEGUI)
#endif

#define	TE_DEBUG0		(1<<(int)EV_DEBUG0)
#define	TE_DEBUG1		(1<<(int)EV_DEBUG1)
#define	TE_DEBUG2		(1<<(int)EV_DEBUG2)
#define	TE_DEBUG3		(1<<(int)EV_DEBUG3)
#define	TE_DEBUG4		(1<<(int)EV_DEBUG4)
#define	TE_TIMER0		(1<<(int)EV_TIMER0)
#define	TE_TIMER1		(1<<(int)EV_TIMER1)
#define	TE_TIMER2		(1<<(int)EV_TIMER2)
#define	TE_TIMER3		(1<<(int)EV_TIMER3)
#define	TE_TIMER4		(1<<(int)EV_TIMER4)
#define	TE_TIMER5		(1<<(int)EV_TIMER5)
#define	TE_TIMER6		(1<<(int)EV_TIMER6)
#define	TE_TIMER7		(1<<(int)EV_TIMER7)
#define	TE_TIMER8		(1<<(int)EV_TIMER8)
#define	TE_TIMER9		(1<<(int)EV_TIMER9)

extern	int CNET_set_trace(int neweventmask);
extern	int CNET_get_trace(void);

	/* CNET_set_trace enables only certain event types to be traced;
	   multiple event types may be ORed together, for example:
	   TE_APPLICATIONREADY | TE_PHYSICALREADY.
	   Invoking cnet with the -t option, is equivalent to calling
	   CNET_set_trace(TE_ALLEVENTS).
	   CNET_get_trace returns the current node's event trace mask. */

extern	int CNET_trace(const char *, ...);

	/* When tracing is enabled in any node, CNET_trace may be called
	   to display any arbitary string or debug output, which will be
	   indented by one tab. It employs a printf-like interface. */

extern	int CNET_trace_name(void *addr, const char *name);

	/* When tracing is enabled in any node, memory addresses are, by
	   default, printed as hexadecimal values. CNET_trace_name
	   associates an address (typically that or a variable) and a string
	   (typically the name of that variable).  When tracing is enabled,
	   the given string will be printed in for its hexadecimal value. */


// --------------- API FOR INTERACTIVE INPUT AND OUTPUT ------------------

#if	CNET_PROVIDES_KEYBOARD
extern	int CNET_read_keyboard(char *line, size_t *len);

	/* Requests the specified maximum number of bytes from the keyboard
	   buffer and places them in the address pointed to by line.
	   On invocation, len will point to an integer indicating the maximum
	   number of bytes that may be copied into line. On return, line
	   will be NULL-byte terminated (and not include a newline) and the
	   integer pointed to by len will contain strlen(line)+1. */
#endif

extern	void CNET_clear(void);

	/* Clears the calling node's standard output window. */

extern	int	CNET_printf(const char *fmt, ...);
#ifdef	printf
#undef	printf
#endif
#define	printf	CNET_printf

#ifdef	puts
#undef	puts
#endif
extern	int	CNET_puts(const char *fmt);
#define	puts	CNET_puts

#ifdef	putchar
#undef	putchar
#endif
extern	int	CNET_putchar(int ch);
#define	putchar	CNET_putchar

// --------------- API FOR EVERYTHING ELSE -------------------------------

extern	int CNET_set_time_of_day(long newsec, long newusec);

	/* Changes the node's notion of the wall-clock time of day.
	   time_of_day.sec is the number of seconds since Jan. 1, 1970,
	   and may be used in a call to ctime(3c).
	   As nodeinfo should be considered a read-only structure, this is
	   the only method to set time_of_day.sec and time_of_day.usec . */

extern	void CNET_check_version(const char *protocol_is_using);

	/* This function compares the current version of the simulator
	   with the version of the <cnet.h> header file used to compile
	   the currently running protocol.  A typical invocation is:

		CNET_check_version(CNET_VERSION);

	   On success, the function returns.  On failure, an error message
	   is printed to stderr and cnet terminates. */

extern	char *CNET_format64(int64_t value);
extern	bool  CNET_set_commas(bool wanted);
extern	bool  CNET_get_commas(void);

	/*  Large (64 bit) integers, such as instances of CnetTime, may be
	    formatted with commas to make them more readable.  cnet only
	    formats large integers with commas on the GUI, only if requested,
	    and never in the statistics printed to stdout.
	    CNET_format64() formats the 64-bit integer and returns a pointer
	    to a static character buffer, whose contents are overwritten on
	    each call.  */

extern	int	CNET_set_outputfile(const char *filenm);
extern	int	CNET_set_tracefile (const char *filenm);

	/*  Request that STDOUT and tracing is mirrored to the named files.
	    Providing a filename of NULL will close/disable mirroring */

#if	!defined(CNET4IPOD)

#define	CNET_NLEDS		6
#define	CNET_LED_OFF		((CnetColour)NULL)

extern	int	CNET_set_LED(int led, CnetColour colour);

	/* Each node has a number of LEDs that may be set to quickly show a
	   node's state. Each LED may be set OFF or to a supported colour. */

extern	void	*CNET_shmem(size_t length);
extern	void	*CNET_shmem2(const char *name, size_t length);

	/* CNET_shmem() returns a pointer to a block of memory of at
	   least the requested number of bytes.  If multiple nodes each make
	   the same request, they will each receive the same pointer to the
	   same shared memory.  When first allocated, the memory is cleared.
	   This region of shared memory can be used to conveniently maintain
	   global statistics from all nodes.
	   CNET_shmem2() permits shared memory segments to be named; each
	   node simply needs to provide the correct name for each segment. */

extern	void	TCLTK(const char *fmt, ...);
extern	void	*TCLTK_interp(void);

extern	char	*CNET_getvar(const char *name);

extern	int	CNET_vflag(void);
extern	bool	CNET_Wflag(void);

#endif

#endif
