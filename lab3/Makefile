# ------------------------------------------------------------
# An example (incomplete) Makefile for CMPUT 313
#
# Usage:  make run       // run cnet on topology file 'tFile'
#         make tar       // creat a 'tar.gz' archive of 'allFiles'
#         make scp       // send '$(target).tar.gz' by scp to 'scpDir'
#         make clean     // remove unneeded files
# ------------------------------------------------------------

target=		submit
allFiles=       Makefile lab3.c LAN-5 LAN-10 LAN-15 LAN-20
scpDir=		username@labmachine.cs.ualberta.ca:directory

# ------------------------------------------------------------

tFile= LAN-20

run:
	cnet -WTs -Q -o out.%n -e 30sec $(tFile)
#	cnet -WTs -Q -o out.%n -e 30sec -f 10sec $(tFile)


tar:
	touch $(target).tar.gz
	mv $(target).tar.gz  x$(target).tar.gz
	tar -cvf $(target).tar $(allFiles)
	gzip $(target).tar

scp:
	scp $(target).tar.gz $(scpDir)

clean:
	rm *~ out.* *.o *.cnet
