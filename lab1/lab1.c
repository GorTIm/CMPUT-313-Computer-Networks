#include <cnet.h>
#include <assert.h>
#include <string.h>

// This file is based on "protocol.c"
// ------------------------------

//CMPUT313 LAB1
//NAME:Weihao Han
//ID:1446915

typedef enum { DL_HELLO, DL_HELLO_ACK } FRAMEKIND;

typedef struct {
  char         data[MAX_NODENAME_LEN];
} MSG;



typedef struct {
  FRAMEKIND    kind;
  CnetAddr     srcAddr;
  CnetTime     time_send;
  MSG          msg;
  char         nodename[MAX_NODENAME_LEN];
  CnetAddr	   address;


} FRAME;

#define MAX_DEGREE 32 //the maximum number of neighbour

#define MAX_NUM_MSG 6 //the number of message to be send to find neighbour info



//static int node_num = 0;



//define a struct to store  the required information
struct neighbour {
    char	nodename[MAX_NODENAME_LEN];
    CnetAddr	address;

    CnetNodeType nodetype;
    
};

//create a  array to contain info
struct neighbour neighbour_array[MAX_DEGREE+1];

int          rec_msg=0;
int          trans_msg=0;





// --------------------
// This function is defined in 'protocol.c' but is not used here.
// 
static EVENT_HANDLER(application_ready)
{
    CnetAddr	destaddr;
    char	buffer[MAX_MESSAGE_SIZE];
    size_t	length;

    length = sizeof(buffer);
    CNET_read_application(&destaddr, buffer, &length);
    printf("\tI have a message of %4d bytes for address %d\n",
			    length, (int)destaddr);
}
// ------------------------------

static EVENT_HANDLER(button_pressed)
{
    printf("\n Node name       : %s\n",	nodeinfo.nodename);
    printf(" Node number     : %d\n",	nodeinfo.nodenumber);
    printf(" Node address    : %d\n",	nodeinfo.address);
    printf(" Number of links : %d\n\n",	nodeinfo.nlinks);

    
    switch(nodeinfo.nodetype){
        case 0:
          printf(" Node type       : HOST\n" );
          break;
        case 1:
        printf(" Node type       : ROUTER\n" );
        break;
        case 2:
        printf(" Node type       : MOBILE\n" );
        break;
        case 3:
        printf(" Node type       : ACCESSPOINT\n" );
        break;


    }
    
    printf(" This node has received %d messages, and transmitted %d messages \n",rec_msg,trans_msg);

    

    for (int a=1;a<nodeinfo.nlinks+1;a++){
        printf(" link %d ==> %s , %d \n",a,neighbour_array[a].nodename,neighbour_array[a].address);
        }
    printf("\n");




}
// ------------------------------
static EVENT_HANDLER(physical_ready)
   // Recall: the macro expands to the following function declaration:
   // static void physical_ready(CnetEvent ev, CnetTimerID timer, CnetData data)
{

  int    link;
  size_t len;
  CnetTime  delta;
  FRAME  f;

  len= sizeof(FRAME);
  CHECK ( CNET_read_physical (&link, (char *) &f, &len) );

  switch (f.kind) {
  case DL_HELLO:
      //assert ( link == 1 );
    

      f.kind= DL_HELLO_ACK;
      //store the info
      f.srcAddr= nodeinfo.address;
      rec_msg+=1;



      // echo on the same link
      len= sizeof(f);
      
      // testing
      //printf("\n %s \n",nei.nodename );
      //printf("\n %d %d \n",f.from_node,link);

      memcpy(f.nodename,nodeinfo.nodename , sizeof(nodeinfo.nodename));
      f.address=nodeinfo.address;

      


      
      CHECK( CNET_write_physical(link, (char *) &f, &len) );
      break;

  case DL_HELLO_ACK:
      //assert ( f.srcAddr == nodeinfo.address );
      rec_msg+=1;
      memcpy(neighbour_array[link].nodename, f.nodename, sizeof(f.nodename));
      neighbour_array[link].address=f.address;




      delta= nodeinfo.time_in_usec - f.time_send;     // turnaround time
      printf("debug: delta= %lld \n", delta);




      break;
  } 
}
// ------------------------------
static EVENT_HANDLER(timer1_send_hello)
   // Recall: the macro expands to the following function declaration:
   // static void timer1_send_hello (CnetEvent ev, CnetTimerID timer, CnetData data)
{
  int   link = 1;
 
  size_t  len;
  FRAME f;

  f.kind= DL_HELLO;
  f.srcAddr= nodeinfo.address;
  f.time_send=  nodeinfo.time_in_usec;    // get current time


  len= sizeof(f);

  

  for(int a=1;a<nodeinfo.nlinks+1;a++){
      trans_msg+=1;
      link=a;
      CHECK( CNET_write_physical(link, (char *) &f, &len) );
        
      }
CNET_start_timer (EV_TIMER1, 10000000, 0);   // send continuous hello  


}

// ------------------------------
EVENT_HANDLER(reboot_node)
{
//  Interested in hearing about:
//    - the Physical Layer has a frame to deliver
//    - timer 1
//

    CNET_set_handler(EV_APPLICATIONREADY, application_ready, 0);

    CNET_set_handler(EV_DEBUG0, button_pressed, 0);
    CNET_set_debug_string(EV_DEBUG0, "Node Info");

    CNET_set_handler(EV_PHYSICALREADY, physical_ready, 0);
    CNET_set_handler(EV_TIMER1, timer1_send_hello, 0);

    // Request EV_TIMER1 in 1 sec, ignore return value
    CNET_start_timer (EV_TIMER1, 1000000, 0);

    if (nodeinfo.nodetype == NT_HOST) { CNET_enable_application(ALLNODES); }
    
}
