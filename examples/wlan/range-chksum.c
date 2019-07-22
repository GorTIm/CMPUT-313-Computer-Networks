#include <cnet.h>
#include <string.h>

#define	NODE_SPACING		50
#define	TRANSMIT_PERIOD		3000000

#define	INITIAL_POWER		0.0
#define	INC_POWER		2.0
#define	FINAL_POWER		30.0

typedef struct {
    int		src;
    int		seqno;
    char	payload[64];
} WLAN_FRAME;

/* ----------------------------------------------------------------------- */

static EVENT_HANDLER(transmit)
{
    WLAN_FRAME	frame;
    size_t	len	= sizeof(frame);
    int		link	= 1;
    static int	seqno	= 0;

    WLANINFO	wlaninfo;
    
//  SLOWLY INCREASE OUR TRANSMISSION POWER
    CHECK(CNET_get_wlaninfo(link, &wlaninfo));
    wlaninfo.tx_power_dBm	+= INC_POWER;
    CHECK(CNET_set_wlaninfo(link, &wlaninfo));

//  TRANSMIT A FRAME
    memset(&frame, 0, len);
    frame.src	= nodeinfo.nodenumber;
    frame.seqno	= seqno++ ;
    CHECK(CNET_write_physical_reliable(link, (char *)&frame, &len));

    fprintf(stdout, "\n%d: transmitting @%.2fdBm (=%.6fmW) ccitt=%d\n",
		nodeinfo.nodenumber,
		wlaninfo.tx_power_dBm,
		dBm2mW(wlaninfo.tx_power_dBm),
		CNET_ccitt((unsigned char *)&frame, len) );

//  SCHEDULE OUR NEXT TRANSMISSION
    if(wlaninfo.tx_power_dBm < FINAL_POWER)
	CNET_start_timer(EV_TIMER1, TRANSMIT_PERIOD, 0);
}

static EVENT_HANDLER(listening)
{
    WLAN_FRAME	frame;
    size_t	len	= sizeof(frame);
    int		link;
    double	rx_signal;

    CHECK(CNET_read_physical(&link, (char *)&frame, &len));
    CHECK(CNET_wlan_arrival(link, &rx_signal, NULL));

    fprintf(stdout, "\t%d: %3dm, received @%.3fdBm (=%.6fmW) ccitt=%d\n",
		nodeinfo.nodenumber, nodeinfo.nodenumber*NODE_SPACING,
		rx_signal, dBm2mW(rx_signal),
		CNET_ccitt((unsigned char *)&frame, len) );
}

EVENT_HANDLER(reboot_node)
{
    CnetPosition	pos;

//  ENSURE THAT WE'RE USING THE CORRECT VERSION OF cnet
    CNET_check_version(CNET_VERSION);

//  POSITION ALL NODES IN A ROW
    pos.x	 = (nodeinfo.nodenumber+1) * NODE_SPACING;
    pos.y	 = 50;
    pos.z	 = 0;
    CHECK(CNET_set_position(pos));

    if(nodeinfo.nodenumber == 0 || nodeinfo.nodenumber == 9) {
	WLANINFO	wlaninfo;
	int		link = 1;
    
	CHECK(CNET_get_wlaninfo(link, &wlaninfo));
	wlaninfo.tx_power_dBm	= INITIAL_POWER;
	CHECK(CNET_set_wlaninfo(link, &wlaninfo));

	CHECK(CNET_set_handler(EV_TIMER1, transmit, 0));
	CNET_start_timer(EV_TIMER1, TRANSMIT_PERIOD, 0);
    }
    CHECK(CNET_set_handler(EV_PHYSICALREADY, listening, 0));
}
