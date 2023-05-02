#INCLUDE "vfp2c.h"

CD (FULLPATH(JUSTPATH(SYS(16))))

IF TYPE('_WIN64') == 'L' AND _WIN64
SET LIBRARY TO vfp2c64.fll ADDITIVE
ELSE
SET LIBRARY TO vfp2c32.fll ADDITIVE
ENDIF

LOCAL lcPath
m.lcPath = 'C:\Projects\*'

*!*	?ADIREX('AdirExCallback', m.lcPath, FILE_ATTRIBUTE_DIRECTORY, ADIREX_DEST_CALLBACK + ADIREX_RECURSE + ADIREX_FULLPATH)
*!*	?ADIREX('AdirExCallback2', m.lcPath, FILE_ATTRIBUTE_DIRECTORY, ADIREX_DEST_CALLBACK + ADIREX_RECURSE + ADIREX_FULLPATH + ADIREX_FILTER_NONE, 0, 'filename, creationtime , filesize')
*!*	?ADIREX('AdirExCallback3', m.lcPath, 0, ADIREX_DEST_CALLBACK + ADIREX_RECURSE + ADIREX_FULLPATH, 0, 'filesize,creationtime, filename')

*!*	*!*	CREATE CURSOR cFiles (filename M, creationtime T, filesize N(20,0), fileattribs I)
USE IN SELECT('cFiles')

LOCAL lnTime
&& lnTime = SECONDS()
? ADIREX('cFiles', m.lcPath, 0, ADIREX_DEST_CURSOR + ADIREX_RECURSE + ADIREX_FULLPATH, 0, 'filename,creationtime,accesstime,filesize')
&& ? SECONDS() - lnTime

SELECT cFiles 

FUNCTION AdirExCallback(cFileName,cDosFileName,tCreationTime,tLastAccessTime,tLastWriteTime,nFileSize,nFileAttributes)
*!*	?cFilename,tCreationTime,tLastAccessTime,tLastWriteTime
ENDFUNC

FUNCTION AdirExCallback2(cFileName,tCreationTime,nFileSize)
*!*	?cFilename,tCreationTime,nFileSize
ENDFUNC

FUNCTION AdirExCallback3(nFileSize, tCreationTime,cFileName)
*!*	?cFilename,tCreationTime,nFileSize
ENDFUNC