	@	/0001
INIT	IO	/02	;Get Data from device
	MM	IADDR	;Save first byte of initial address
	MM	START
	IO	/02
	MM	IADDR2	;Save second byte of initial address
	MM	START2
	IO	/02
	MM	SIZE	;Save size
LOOP	IO	/02	;Read byte from	file
	CN	/02	;Activate Indirect Mode
	MM	IADDR	;Indirect move to current address
	LD	IADDR2	;Get current address
	+	ONE	;Sum 1
	MM	IADDR2	;Put it back
	LD	SIZE	;Get size
	-	ONE	;Subtract 1
	MM	SIZE	;Put it back
	JZ	END	;Finish	if size = 0
	JP	LOOP
END	CN	/02
	JP	START
IADDR	K	/00
IADDR2	K	/00
SIZE	K	/00
ONE	K	/01
START	K	/00
START2	K	/00
	#	INIT
