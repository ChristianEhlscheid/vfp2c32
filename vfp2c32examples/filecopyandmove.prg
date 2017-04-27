#INCLUDE vfp2c.h

CD (FULLPATH(JUSTPATH(SYS(16))))
SET LIBRARY TO vfp2c32.fll ADDITIVE
INITVFP2C32(VFP2C_INIT_ALL)

LOCAL lcSourceFile, lcDestFile

lcSourceFile = GETOPENFILENAME(0,"All Files" + CHR(0) + "*.*","","C:","Choose a file to copy/move",OFN_EX_NOPLACESBAR)
IF VARTYPE(lcSourceFile) = 'N'
	IF lcSourceFile = -1
		? "Dialog box aborted."	
	ELSE
		? "Error in dialog box."	
	ENDIF
	RETURN
ENDIF

lcDestFile = GETSAVEFILENAME(0,"All Files" + CHR(0) + "*.*","","C:","Choose a destination",OFN_EX_NOPLACESBAR)
IF VARTYPE(lcSourceFile) = 'N'
	IF lcSourceFile = -1
		? "Dialog box aborted."	
	ELSE
		? "Error in dialog box."	
	ENDIF
	RETURN
ENDIF

PUBLIC loForm
DO FORM frmfileprogress NAME loForm LINKED
SET ESCAPE ON
ON ESCAPE loForm.Abort()

CopyFileEx(lcSourceFile,lcDestFile,'loForm.Progress')
&& MoveFileEx(lcSourceFile ,lcDestFile,'loForm.Progress')

SET ESCAPE OFF
ON ESCAPE
RELEASE loForm



