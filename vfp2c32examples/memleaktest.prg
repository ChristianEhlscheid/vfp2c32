&& there are some debugging functions in the debug version (vfp2c32d.fll) that help to 
&& test your converted C structs for memory leaks
&& since the conversion is done automatically there "normally" shouldn't be leaks
&& i'm not aware of any bugs in which the vfp2c32front.exe produces code that creates memory leaks ..
&& but since the whole thing is rather complex i've implemented these debugging functions

#INCLUDE vfp2c.h
CD (FULLPATH(JUSTPATH(SYS(16))))
SET LIBRARY TO vfp2c32d.fll ADDITIVE
INITVFP2C32(VFP2C_INIT_ALL)

&& 1st step
TRACKMEM(.T.) && enable memory allocation tracking in the library

&& 2nd step, create your struct, assign values to all fields and then release it ..
LOCAL loStruct
loStruct = CREATEOBJECT('TestStruct')
loStruct.pField1 = "Hello memory leak :)"
loStruct.nField2 = 345
loStruct = .NULL.

&& 3rd step, call AMemLeaks
IF AMEMLEAKS("laLeaks") > 0
	DISPLAY MEMORY LIKE laLeaks && we just leaked some memory ... 
ENDIF

&& 4th step, disable tracking again
TRACKMEM(.F.)
&& TRACKMEM(.F.,.T.) disable allocation tracking & clear the tracking list

SET LIBRARY TO

DEFINE CLASS TestStruct AS Relation

	Address = 0
	SizeOf = 8
	Name = "TestStruct"
	&& structure fields
	pField1 = .F.
	nField2 = .F.

	PROCEDURE Init()
		THIS.Address = AllocMem(THIS.SizeOf)
		IF THIS.Address = 0
			ERROR(43)
			RETURN .F.
		ENDIF
	ENDPROC

	PROCEDURE Destroy()
		&& THIS.FreeMembers() commented out to produce a memory leak, normally this would release the memory for the "pField1" member
		FreeMem(THIS.Address)
	ENDPROC

	PROCEDURE FreeMembers()
		FreePMem(THIS.Address)
	ENDPROC

	PROCEDURE pField1_Access()
		RETURN ReadPCString(THIS.Address)
	ENDPROC

	PROCEDURE pField1_Assign(lnNewVal)
		WritePCString(THIS.Address,lnNewVal)
	ENDPROC

	PROCEDURE nFiedl2_Access()
		RETURN ReadInt(THIS.Address+4)
	ENDPROC

	PROCEDURE nFiedl2_Assign(lnNewVal)
		WriteInt(THIS.Address+4,lnNewVal)
	ENDPROC

ENDDEFINE