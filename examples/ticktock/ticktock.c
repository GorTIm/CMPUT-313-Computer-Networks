#include <cnet.h>

/*  This is a simple cnet protocol source file which demonstrates the use
    of timer events in cnet.

    The function reboot_node() will first be called for each node.
    reboot_node() informs cnet that the node is interested in the EV_TIMER1
    event and that cnet should call timeouts() each time EV_TIMER1 occurs.
    Finally, reboot_node() requests that EV_TIMER1 occurs in 1000000us (1sec).

    When EV_TIMER1 occurs, cnet will call timeouts(), which will print
    either tick or tock. As each timer event occurs only once, timeouts()
    must finally reschedule EV_TIMER1 to occur again in another 1000000us.
 */

static EVENT_HANDLER(timeouts)
{
    static int which = 0;

    ++which;
    printf("%3d.\t%s\n", which, ((which%2) == 1) ? "tick" : "\ttock");

//  RESCHEDULE EV_TIMER1 TO OCCUR AGAIN IN 1SEC
    CNET_start_timer(EV_TIMER1, 1000000, 0);
}

EVENT_HANDLER(reboot_node)
{
//  INDICATE THAT WE ARE INTERESTED IN THE EV_TIMER1 EVENT
    CHECK(CNET_set_handler(EV_TIMER1, timeouts, 0));

//  REQUEST THAT EV_TIMER1 OCCUR IN 1SEC, IGNORING THE RETURN VALUE
    CNET_start_timer(EV_TIMER1, 1000000, 0);
}
