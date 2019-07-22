#include <cnet.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/*
# ------------------------------------------------------------
# Copyright Notice:
#    Copyright by CMPUT 313, U. of Alberta, course instructor (E. Elmallah).
#    All rights reserved. Do not post any part on a publicly-available Web site.
#
# Important:
#     Your submitted solution based on using parts of the code should
#     - Replace the provided documentation with your own documentation
#     - Omit the provided suggested steps
#     - Acknowledge parts of this code used in your solution
#
# Purpose:
#     This file implements a sliding window protocol for a 2-node network 
#     with rdt feature,specifically:
#     - Sender buffers "sendWin" pkts 
#     - Sender keeps one timeout period (for the oldest transmitted but not yet
#     	ack'ed pkt). On timeout, only the timed out packet is retransmitted.
#     - Receiver buffers "rcvWin" pkts (it stores out-of-order pkts if	
#        rcvWin > 1)
#     - Receiver sends cumulative ACKs; the ack number is the sequence number
#       of the last correctly received in-order pkt
#     - Each frame carries a 'seq' number and an 'ack' number; these numbers
#       count whole frames (transmitted or received). 
#     - Each node work as both sender and receiver
# 
# Implementation remarks:
#     - For a transmitted frame f:
#       - If (f.seq == -1) the frame carries no data in the 'msg' field.
#	  Else, if (f.seq >= 0) the frame carries a 'msg' whose sequence number
#	  is 'f.seq'.
#       - If (f.ack == -1) the frame carries no acknowledgement.
#	  Else if (f.ack >= 0), the frame confirms that the sender has correctly
#	  received all frames in the range [0,ack] (cumulative ACK).
#	  
#     - a frame can contain either one or both of data and ack number
#     - Each frame is initialized by FRAME_init() to be an empty frame
#     
#     - The topology file should define variables "sendWin" and "rcvWin" to make 
#     topology work           
#
# Tips:
#      update the sendBase and rcvBase correctly is very important!!! 
#	   conn information should be printed in a appropriate form
#
# Status:
#    - Currently, the protocol works only for 2-node networks
#    - There is one instance of the CONN struct, so the protocol handles
#      only one sender-receiver connection
#
# ------------------------------------------------------------ 
*/

#define FRAME_HEADER_SIZE  (sizeof(FRAME) - sizeof(MSG))
#define FRAME_SIZE(f)      (FRAME_HEADER_SIZE + (f).msgLen) //Don't use 'f.msgLen'

typedef struct {  char data[MAX_MESSAGE_SIZE]; } MSG;

typedef struct {
    CnetAddr     src,dest; 
    size_t	 msgLen;       	// the length of the msg field only
    int          checksum;  	// checksum of the whole frame
    int          seq;       	// for a data pkt, seq > 0 (else -1)
    int		 ack;		// for a valid ack, ack > 0 (else, -1)
    MSG          msg;
} FRAME;

// a CONN struct stores all key information of a single connection
//
typedef struct {
    CnetAddr     dest;
    CnetTimerID	 lasttimer;
    int          sendWin, rcvWin;	// size of sender and receiver windows
    int          sendBase, nextseqnum;	// sender variables
    int          rcvBase;		// receiver variables
    FRAME        *sendBuf, *rcvBuf;   	// sender and receiver buffers (arrays)  
} CONN;

static CONN  conn;	// only one connection is supported in this version

static CnetTime  CONN_timeout;
// ------------------------------------------------------------
// FRAME_init - initialize a frame with 'seq= -1' and 'ack= -1'. 
//

void FRAME_init (FRAME *f) {
    f->src = f->dest = -1;
    f->msgLen = 0;
    f->checksum = 0;
    f->seq = f->ack = -1;
    memset ( &(f->msg), 0, sizeof(MSG) );
}   
// ------------------------------

void FRAME_print (FRAME *f) {
    printf ("(src= %d, dest= %d, seq= %d, ack= %d, msgLen= %d) \n",
     	    f->src, f->dest, f->seq, f->ack, f->msgLen);
}

// ------------------------------
// CONN_init - initialize a connection with the specified 'sendWin' and
//     'rcvWin' buffer sizes
//
void CONN_init (int sendWin, int rcvWin) {
    int idx;

    conn.dest = -1;
    conn.lasttimer = NULLTIMER;
    conn.sendWin= sendWin; conn.rcvWin= rcvWin;  

					// start with zero sequence numbers
    conn.sendBase = conn.nextseqnum = conn.rcvBase = 0;
    
    conn.sendBuf = (FRAME *) calloc (sendWin, sizeof(FRAME)); // array of FRAMEs
    conn.rcvBuf  = (FRAME *) calloc (rcvWin,  sizeof(FRAME)); // array of FRAMEs 

    for (idx= 0; idx < sendWin; idx++) FRAME_init ( &(conn.sendBuf[idx]) );

    for (idx= 0; idx < rcvWin; idx++)  FRAME_init ( &(conn.rcvBuf[idx]) );
}
// ----------

void CONN_print () {

    int idx;
    printf("SendBuffer: \n ");
    for(idx= 0; idx < conn.sendWin; idx++){
        FRAME_print (&(conn.sendBuf[idx]));
    }
    printf(" \n");
    printf("receivBuffer: \n "); 
    for(idx= 0; idx < conn.rcvWin; idx++){
        FRAME_print (&(conn.rcvBuf[idx]));

    }
    printf("\n");
}
// ----------

void CONN_stop_timer () {
    if (conn.lasttimer != NULLTIMER)  {
        CNET_stop_timer (conn.lasttimer); conn.lasttimer = NULLTIMER;
    }
}
// ----------

void CONN_start_timer () {
    conn.lasttimer = CNET_start_timer (EV_TIMER1, CONN_timeout, 0);
}

// ----------
// make_frame - compose a frame with the specified parameters.
// Note: a frame can be composed, stored in 'sendBuf', and transmitted.
//      To re-transmit the frame at some future time, the 'ack'
//	field has to be updated, and consequently the 'checksum' field
//	has to be recomputed. So, the frame should be re-composed again.
//
static int  make_frame (FRAME *f, CnetAddr dest, int seq, int ack,
       	    	        MSG *msg, size_t msgLen)
{
    size_t  fLen;		// frame length

    f->src = nodeinfo.address;  f->dest = dest;
    f->msgLen = msgLen;
    f->seq = seq; f->ack = ack;
    if (msg != NULL)  memcpy( &(f->msg), msg, (int)msgLen );

    // Note. Normally, cnet does not report corrupted frames to the receiving
    // host ('-E' overrides this behaviour). So, the sender should include an
    // error detection code.

    f->checksum = 0;
    fLen = FRAME_SIZE(*f);
    f->checksum =  CNET_ccitt((unsigned char *)f, (int)fLen);
    return (fLen);
}

// ------------------------------------------------------------

static EVENT_HANDLER(application_ready)
{
    CnetAddr  destaddr;
    int       outLink;
    size_t    msgLen, fLen;
    MSG	      lastMsg;
    FRAME     lastFrame;
    int	      lastFrame_idx;	// index in sendBuf[] to store a frame carrying
    	      			// a new AL msg

    msgLen  = sizeof(MSG);
    CHECK(CNET_read_application(&destaddr, &lastMsg, &msgLen));

    if (conn.dest == -1) conn.dest = destaddr;	 // initialize conn.dest    
    if (conn.dest != destaddr) return;		 // ignore msgs not to conn.dest 

    lastFrame_idx = conn.nextseqnum - conn.sendBase;


    assert (lastFrame_idx < conn.sendWin);

    // Disable AL if the sendBuf becomes full after inserting a new frame
    //
    if ( (lastFrame_idx + 1) == conn.sendWin ) {
         CNET_disable_application (destaddr);
    }	  	 

    // make frame for storage
    fLen= make_frame(&lastFrame, destaddr, conn.nextseqnum, -1, &lastMsg, msgLen);
    memcpy ( &(conn.sendBuf[lastFrame_idx]), &lastFrame, fLen );

    // transmit frame
    outLink = 1;
    CHECK ( CNET_write_physical (outLink, &lastFrame, &fLen) );


    if ( conn.nextseqnum== conn.sendBase){
        CONN_stop_timer ();
        CONN_start_timer ();
        }

    conn.nextseqnum++;

}

// ------------------------------------------------------------

static EVENT_HANDLER(physical_ready)
{
    FRAME        f, fout;
    size_t	  fLen;
    int          link, stored_checksum, computed_checksum;
    int		 outLink, idx, fIdx;

    fLen         = sizeof(FRAME);
    CHECK(CNET_read_physical(&link, &f, &fLen));

    // check the integrity of the received frame
    //
    stored_checksum    = f.checksum;
    f.checksum  = 0;
    computed_checksum= CNET_ccitt((unsigned char *)&f, (int)fLen);

   
    if(computed_checksum!=stored_checksum){
        return;
    }

    if (conn.dest==-1){
        conn.dest=f.dest;
    }

    fIdx=f.seq-conn.rcvBase;
    if (f.seq>= 0   && f.dest==nodeinfo.address && fIdx < conn.rcvWin ){
        
        if (conn.rcvBuf[fIdx].src==-1){
            memcpy ( &(conn.rcvBuf[fIdx]), &f, fLen );
            
            
        }else{
            assert(conn.rcvBuf[fIdx].seq=f.seq);
        }
        //use fIdx to detect how many frame to be delivered to application
        fIdx=0;
        for (idx=0;idx<conn.rcvWin;idx++){
            if (conn.rcvBuf[idx].dest!=-1){
                CNET_write_application(&(conn.rcvBuf[idx].msg),&(conn.rcvBuf[idx].msgLen) );
                
                fIdx++;
                
                
            }
            else{
                idx=conn.rcvWin;
            }
        }
        int gap=fIdx;

        //use fIdx to detect the index of frames whose contents have been relocated 
        idx=0;
        if (conn.rcvWin>1){
            for(int i=fIdx;i<conn.rcvWin;i++){
                memcpy (&(conn.rcvBuf[idx]),&(conn.rcvBuf[i]),fLen);
                FRAME_init (&(conn.rcvBuf[i]));
                idx++;
                }

        }else{
            FRAME_init (&(conn.rcvBuf[idx]));
        }

        conn.rcvBase=conn.rcvBase+gap;
        //conn.rcvBase=conn.rcvBuf[0].seq;
        


    }


    if (f.seq>= 0){
        outLink=1;
        fLen=make_frame (&fout, f.src,-1,conn.rcvBase - 1, NULL,0 );
        fout.ack=conn.rcvBase-1;
        fout.dest=f.src;
        CHECK ( CNET_write_physical (outLink, &fout, &fLen) );
        
    }
   


    if(f.ack>=conn.sendBase){
        assert (f.ack <= conn.nextseqnum);
        int fIdx=0;
        idx=f.ack+1-conn.sendBase;
        int gap2=idx;
        if (conn.sendWin>1){
            for(int idx;idx<conn.sendWin ;idx++){
            memcpy(&(conn.sendBuf[fIdx]),&(conn.sendBuf[idx]),fLen);
            FRAME_init (&(conn.sendBuf[idx]));
            fIdx++;
            }

        }
        conn.sendBase=conn.sendBase+gap2;
        //conn.sendBase=conn.sendBuf[0].seq;
        CONN_stop_timer ();
        CONN_start_timer ();
        CNET_enable_application(ALLNODES);


    }

}
// ------------------------------------------------------------

static EVENT_HANDLER(CONN_timeouts) 
{
    int		outLink= 1;
    FRAME	f;
    size_t	fLen;

    		// a timeout occurs only if (sendBase < nextseqnum)
    assert (conn.sendBase < conn.nextseqnum);

    // Retransmit the pkt at sendBase[0]. Re-pack the pkt because the 'ack'
    // number may have changed since the time we stored the frame.
    //
    fLen= make_frame (&f, conn.sendBuf[0].dest, conn.sendBuf[0].seq,
    	              conn.rcvBase - 1, &(conn.sendBuf[0].msg),
		      conn.sendBuf[0].msgLen );
    
    

    CHECK ( CNET_write_physical (outLink, &f, &fLen)  );

    // restrat timer
    CONN_start_timer();
} 

// ------------------------------------------------------------

static EVENT_HANDLER(showstate)
{
    CONN_print();
}

// ------------------------------------------------------------

EVENT_HANDLER(reboot_node)
{
    int sendWin, rcvWin;

    //if(nodeinfo.nodenumber > 1) {
	//fprintf(stderr,"This is not a 2-node network!\n");
	//exit(1);
    //}

    if (nodeinfo.nodetype == NT_HOST) {
         CHECK(CNET_set_handler( EV_APPLICATIONREADY, application_ready, 0));
    }

    CHECK(CNET_set_handler( EV_PHYSICALREADY,    physical_ready, 0));
    CHECK(CNET_set_handler( EV_TIMER1,           CONN_timeouts, 0));

    CHECK(CNET_set_handler( EV_DEBUG0,           showstate, 0));
    CHECK(CNET_set_debug_string( EV_DEBUG0, "CONN State"));

    CNET_enable_application(ALLNODES);

    // connection specific initialization
    //
    // The timeout period follows the stop-and-wait.c protocol
    //
    CONN_timeout= sizeof(FRAME) * ( (CnetTime) 8000000 / linkinfo[1].bandwidth)
    	          + linkinfo[1].propagationdelay;
    CONN_timeout=  3 * CONN_timeout;

    sendWin = atoi (CNET_getvar ("sendWin")); assert (sendWin > 0);
    rcvWin  = atoi (CNET_getvar ("rcvWin"));  assert (rcvWin  > 0);

    CONN_init (sendWin, rcvWin);
}
