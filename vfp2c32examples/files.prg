#INCLUDE vfp2c.h

LOCAL lcPath
m.lcPath = FULLPATH(JUSTPATH(SYS(16)))
CD (m.lcPath)

SET LIBRARY TO vfp2c32.fll ADDITIVE
INITVFP2C32(VFP2C_INIT_FILE)

LOCAL lnCount, lnCount2, laFiles[1], xj, xi

ADIRECTORYINFO('laDir', m.lcPath)
? "No. of files:", laDir[1]
? "No. of subdirectories:", laDir[2]
? "Size of files:", laDir[3] && in bytes

lnCount = ADRIVEINFO('laDrives')
FOR xj = 1 TO lnCount
	? "Drive:", laDrives[xj,1]
	? "Type:", laDrives[xj,2] && the same values as returned by native DRIVETYPE() function
	? "Device No.:", laDrives[xj,3]
	? "Partition No.:", laDrives[xj,4]
ENDFOR

lnCount = AVolumes('laVolumes')
FOR xj = 1 TO lnCount
	? "Volume:", laVolumes[xj]
	AVolumeInformation('laInfo', SUBSTR(laVolumes[xj], 1, LEN(laVolumes[xj])-1))
	DISPLAY MEMORY LIKE laInfo
	lnCount2 = AVolumeMountPoints('laMountPoints', laVolumes[xj])
	FOR xi = 1 TO lnCount2
		? "Mountpoint:", laMountPoints[xi]
	ENDFOR
	lnCount2 = AVolumePaths('laPaths', laVolumes[xj])
	FOR xi = 1 TO lnCount2
		? "Path:", laPaths[xi]
	ENDFOR
ENDFOR

&& ---------------
&& ADIREX examples
&& ---------------

&& enumerate all files in some directory into array lafiles
lnCount = ADIREX('laFiles',ADDBS(FULLPATH(CURDIR()))+"*.*")

&& enumerate all subdirectories in some directory into array ..
&& default filter algorithm: BITAND(nAttributes,nYourFilter) > 0 - or in words .. one of the attributes have to exist for a match
lnCount = ADIREX('laFiles',ADDBS(FULLPATH(CURDIR()))+"*.*",FILE_ATTRIBUTE_DIRECTORY)

&& enumerate all read only files into array..
lnCount = ADIREX('laFiles',ADDBS(FULLPATH(CURDIR()))+"*.*",FILE_ATTRIBUTE_READONLY)

&& enumerate all files into cursor "curFiles" with default fieldnames
lnCount = ADIREX('curFiles',ADDBS(FULLPATH(CURDIR()))+"*.*",0,ADIREX_DEST_CURSOR)
USE IN curfiles

&& emumerate all files into cursor "yourCursor" with you own names
&& fieldtypes & order must match the ones below!!
CREATE CURSOR yourCursor (thefilename C(254), thealternate C(13), ctime T, atime T, wtime T, fisize N(20,0), fattribs I)
lnCount = ADIREX('yourCursor',ADDBS(FULLPATH(CURDIR()))+"*.*",0,ADIREX_DEST_CURSOR)
BROWSE
USE IN yourCursor

&& enumerate all files by calling back into AdirCallback function
?ADIREX('AdirCallback',ADDBS(FULLPATH(CURDIR()))+"*.*",0,ADIREX_DEST_CALLBACK)


&& enumerate all system and hidden files into array laFiles
&& underlying filter algorithm: BITAND(nFileAttributes,nYourFilter) == nYourFilter
lnCount = ADIREX('laFiles',ADDBS(FULLPATH(CURDIR()))+"*.*",FILE_ATTRIBUTE_SYSTEM+FILE_ATTRIBUTE_HIDDEN,ADIREX_FILTER_ALL)

&& enumerate all files expect system or hidden into array laFiles
&& underlying filter algorithm: BITAND(nFileAtributes,nYourFilter) == 0
lnCount = ADIREX('laFiles',ADDBS(FULLPATH(CURDIR()))+"*.*",FILE_ATTRIBUTE_SYSTEM+FILE_ATTRIBUTE_HIDDEN,ADIREX_FILTER_NONE)

&& enumerate only "read only" files (exact match, no other attributes)
&& underlying filter algorithm: nFileAttributes == nYourFilter
lnCount = ADIREX('laFiles',ADDBS(FULLPATH(CURDIR()))+"*.*",FILE_ATTRIBUTE_SYSTEM+FILE_ATTRIBUTE_HIDDEN,ADIREX_FILTER_EXACT)

&& ----------------
&& CompareFileTimes - compare last modified date of two files
&& ----------------

LOCAL lnComp
lnComp = COMPAREFILETIMES("vfp2c32.fll","vfp2c32d.fll")
DO CASE
	CASE lnComp = 0
		? "times are equal"
	CASE lnComp = 1
		? "first file is newer than second file"
	CASE lnComp = 2
		? "second file is newer than first file"
ENDCASE

LOCAL laFileAttrib[1]
?AFILEATTRIBUTES('laFileAttrib',"vfp2c32.fll")
? "File Attributes: ", laFileAttrib[1]
? "File Size: ", laFileAttrib[2]
? "Creationtime: ", laFileAttrib[3]
? "LastAccesstime: ", laFileAttrib[4]
? "LastWritetime: ", laFileAttrib[5]

?AFILEATTRIBUTESEX('laFileAttrib',"vfp2c32.fll")
? "File Attributes: ", laFileAttrib[1]
? "File Size: ", laFileAttrib[2]
? "Creationtime: ", laFileAttrib[3]
? "LastAccesstime: ", laFileAttrib[4]
? "LastWritetime: ", laFileAttrib[5]
? "No of links: ", laFileAttrib[6]
? "Volume Serial Number: ", laFileAttrib[7]
? "FileIndex Low: ", laFileAttrib[8]
? "FileIndex High: ", laFileAttrib[9]

LOCAL ltCreate, ltAccess, ltWrite
?GETFILETIMES("vfp2c32.fll",@ltCreate,@ltAccess,@ltWrite)
? "File created: ", ltCreate
? "Last accessed: ", ltAccess
? "Last written to: ", ltWrite
&& only get one or two of the times
&& GETFILETIMES("vfp2c32.fll",@ltCreate)
&& GETFILETIMES("vfp2c32.fll",NULL,@ltAccess)
&& GETFILETIMES("vfp2c32.fll",NULL,NULL,@ltWrite)
&& GETFILETIMES("vfp2c32.fll",@ltCreate,NULL,@ltWrite)

&& increasing the time we got from above 1 minute
ltCreate = ltCreate + 60
ltAccess = ltAccess + 60
ltWrite = ltWrite + 60

&& setting all times
?SETFILETIMES("vfp2c32.fll",ltCreate,ltAccess,ltWrite)
&& only set creation time
&& SETFILETIMES(FULLPATH("vfp2c32.fll"),ltCreate)
&& only set access time
&& SETFILETIMES(FULLPATH("vfp2c32.fll"),NULL,ltAccess)
&& only set write time
&& SETFILETIMES(FULLPATH("vfp2c32.fll"),NULL,NULL,ltWrite)

LOCAL lnAttribs
lnAttribs = GETFILEATTRIBUTES("vfp2c32.fll")
? "File is " + IIF(BITAND(lnAttribs,FILE_ATTRIBUTE_READONLY)>0,"read only","not read only")
? "File is " + IIF(BITAND(lnAttribs,FILE_ATTRIBUTE_SYSTEM)>0,"system","not system")
? "File is " + IIF(BITAND(lnAttribs,FILE_ATTRIBUTE_ARCHIVE)>0,"archive","not archive")

&& set archive attribute
lnAttribs = BITOR(lnAttribs,FILE_ATTRIBUTE_ARCHIVE)
&& ?SETFILEATTRIBUTES("vfp2c32.fll",lnAttribs)

LOCAL lnFileSize
lnFileSize = GETFILESIZE("vfp2c32.fll")
? "File is ", lnFileSize, "bytes big"

LOCAL lcOwner,lcDomain,lnSidType
?GETFILEOWNER("vfp2c32.fll",@lcOwner,@lcDomain,@lnSidType)
&& GETFILEOWNER("vfp2c32.fll",@lcOwner)
&& GETFILEOWNER("vfp2c32.fll",@lcOwner,@lcDomain)

&& deleting a file even if it is marked readonly
TRY
	STRTOFILE('DummyFile','dummy.txt')
	SETFILEATTRIBUTES('dummy.txt',FILE_ATTRIBUTE_READONLY)
	DELETE FILE "dummy.txt" && triggers error since file is read only
CATCH TO loError
	?loError.ErrorNo, loError.Message
ENDTRY
DELETEFILEEX('dummy.txt') && this succeeds since it checks for readonly and removes attribute if needed

&& shell file operation's
&&
&& copy all files in a directory into a new directory
*!*	LOCAL lcSource, lcTarget
*!*	lcSource = FULLPATH(CURDIR())+"*.prg"
*!*	lcTarget = FULLPATH(CURDIR())+"testdirectory"
*!*	?SHCOPYFILES(lcSource,lcTarget)

*!*	&& copy several single files into a new directory
*!*	lcSource = FULLPATH(CURDIR())+"vfp2c32.fll" + CHR(0) + FULLPATH(CURDIR())+"vfp2c32d.fll" + CHR(0) + FULLPATH(CURDIR())+"vfp2cdummy.prg"
*!*	lcTarget = FULLPATH(CURDIR())+"testdirectory"
*!*	?SHCOPYFILES(lcSource,lcTarget)

*!*	&& copy all prg files in a directory into several new directories
*!*	lcSource = FULLPATH(CURDIR())+"*.prg"
*!*	lcTarget = FULLPATH(CURDIR())+"testdirectory" + CHR(0) + FULLPATH(CURDIR())+"testdirectory2" + CHR(0) + FULLPATH(CURDIR())+"testdirectory3"
*!*	?SHCOPYFILES(lcSource,lcTarget,FOF_MULTIDESTFILES)

*!*	&& copy one file into several new destinations
*!*	lcSource = FULLPATH(CURDIR())+"vfp2c32.fll"
*!*	lcTarget = FULLPATH(CURDIR())+"testdirectory" + CHR(0) + FULLPATH(CURDIR())+"testdirectory2" + CHR(0) + ;
*!*	FULLPATH(CURDIR())+"testdirectory\vfp2c32.fll.bak"
*!*	?SHCOPYFILES(lcSource,lcTarget,FOF_MULTIDESTFILES)

&& deleting a whole directory (the one we created above in SHCopyFiles)
*!*	?DELETEDIRECTORY(FULLPATH(CURDIR())+"testdirectory")

FUNCTION AdirCallback(lcFileName,lcDosFileName,tCreationTime,tAccessTime,tWriteTime, ;
				nFileSize,nFileAttributes)
		? lcFileName, nFileSize		
		RETURN .T.
		&& RETURN .F. to abort enumeration
ENDFUNC

FUNCTION FileProgress(lnReason,lnSize,lnPercent)
	WAIT WINDOW TRANSFORM(lnReason) + "," + TRANSFORM(lnSize) + "," + TRANSFORM(lnPercent) NOWAIT NOCLEAR
	RETURN .T.
ENDFUNC