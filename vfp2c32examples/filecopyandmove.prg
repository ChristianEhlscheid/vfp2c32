#INCLUDE vfp2c.h

CD (FULLPATH(JUSTPATH(SYS(16))))
IF TYPE('_WIN64') = 'L' AND _WIN64
SET LIBRARY TO vfp2c64.fll ADDITIVE
ELSE
SET LIBRARY TO vfp2c32.fll ADDITIVE
ENDIF

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

? CopyFileEx(lcSourceFile,lcDestFile,'loForm.Progress')
&& MoveFileEx(lcSourceFile ,lcDestFile,'loForm.Progress')

SET ESCAPE OFF
ON ESCAPE
RELEASE loForm



