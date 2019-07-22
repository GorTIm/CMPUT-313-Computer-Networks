#!/bin/sh
#
TOPOLOGIES="FLOODING1 FLOODING2 FLOODING3"

PNG="plot.png"

TITLE="Number of messages delivered"`date '+,  run %c'`
XAXIS="seconds"

METRIC="Messages delivered"
DURATION="1hour"
EVERY="10"

CNET="cnet"
GNUPLOT="gnuplot"

# --  nothing needs changing below this line  --------------------------

function run_simulations {
    for t in `echo $TOPOLOGIES`
    do
	rm -f stats.$t
	$CNET -qsTW -e $DURATION -f ${EVERY}s $t	| \
	grep "$METRIC"					| \
	cut -d: -f 2					| \
        awk "{ printf(\"%d %s\n\", ++i * $EVERY, \$1); }"       > stats.$t
    done
}

function run_gnuplot {
    TMP="/tmp/p$$.gp"

    rm -f $TMP
    cat << END_END	>> $TMP
set terminal png
set output "$PNG"
#
set title  "$TITLE"
set xlabel "$XAXIS"
#
set grid
set yrange [0:]

plot \\
END_END

    for t in `echo $TOPOLOGIES`
    do
	echo "\"stats.$t\" title \"$t\" with lines lw 2, \\"
    done		>> $TMP

    $GNUPLOT $TMP
    rm -f $TMP
    for t in `echo $TOPOLOGIES`; do rm -f stats.$t; done
}


run_simulations
run_gnuplot
echo "output is in $PNG"
