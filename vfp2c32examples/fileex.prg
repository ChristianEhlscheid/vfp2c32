&& extended VFP like low level file manipulation functions
&& main features/enhancements:
&& -also handles files larger than 2GB
&& -files can be created/opened for shared access (VFP native functions only allow exclusive access)
&& -additional functions for locking parts of a file (FLockFile(Ex), FUnlockFile(Ex))
&& FCreateEx and FOpenEx return the real windows file handle, you can use this handle for other API functions
&& you can also pass API handles not created with FCreateEx or FOpenEx to the functions FWriteEx, FPutsEx, FReadEx ect. ...

&& prerequisites: if you want to use F(Un)LockFileEx - InitVFP2C32 must have been called with the VFP2C_INIT_FILE flag
&& all other functions don't need any initialization

#INCLUDE vfp2c.h

CD (FULLPATH(JUSTPATH(SYS(16))))
SET LIBRARY TO vfp2c32.fll ADDITIVE
INITVFP2C32(VFP2C_INIT_FILE)

LOCAL lnHandle
&& create normal file for read/write with shared read/write/delete access for other processes
lnHandle = FCREATEEX('filetest.txt',FILE_ATTRIBUTE_NORMAL, 2, FILE_SHARE_READ+FILE_SHARE_WRITE+FILE_SHARE_DELETE)

IF lnHandle = -1
	AERROREX('laError')
	DISPLAY MEMORY LIKE laError
	RETURN
ENDIF

?FPUTSEX(lnHandle, 'Hello World')
?FWRITEEX(lnHandle, 'Hello World2')

&& no need to explain them all since they work just like their VFP counterparts

?FCLOSEEX(lnHandle)
&& FCloseEx has a special parameter value -1 which closes all currently open files 
