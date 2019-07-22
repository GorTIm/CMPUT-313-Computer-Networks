//  This is a sample cnet topology file for "stopandwait.c".
//  For the first time we now see frame loss in the physical layer.

compile = "stopandwait.c"

probframeloss = 2

host perth {
    x=100, y=50
    address     = 302
    messagerate = 1000ms

    link to melbourne
}

host melbourne {
    x=400, y=50
    address     = 351
    messagerate = 4000ms

    link to perth
}
