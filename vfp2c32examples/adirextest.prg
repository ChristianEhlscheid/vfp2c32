#INCLUDE "vfp2c.h"

CD (FULLPATH(JUSTPATH(SYS(16))))

IF TYPE('_WIN64') == 'L' AND _WIN64
SET LIBRARY TO vfp2c64d.fll ADDITIVE
ELSE
SET LIBRARY TO vfp2c32d.fll ADDITIVE
ENDIF

LOCAL lcPath
m.lcPath = 'C:\Projects\imagezip\*'

? ADIREX('AdirExCallback', m.lcPath, FILE_ATTRIBUTE_DIRECTORY, ADIREX_DEST_CALLBACK + ADIREX_FULLPATH)
*!*	? ADIREX('AdirExCallback2', m.lcPath, FILE_ATTRIBUTE_DIRECTORY,ADIREX_DEST_CALLBACK + ADIREX_RECURSE + ADIREX_FULLPATH + ADIREX_FILTER_NONE, 0, 'filename, creationtime, filesize, cfileattribs')
*!*	? ADIREX('AdirExCallback3', m.lcPath, 'd', ADIREX_DEST_CALLBACK + ADIREX_RECURSE + ADIREX_FULLPATH, 0, 'filesize,creationtime, filename,fileattribs')

*!*	*!*	CREATE CURSOR cFiles (filename M, creationtime T, filesize N(20,0), fileattribs I)
USE IN SELECT('cFiles')

LOCAL lnTime
&& lnTime = SECONDS()
? ADIREX('cFiles', m.lcPath, 0, ADIREX_DEST_CURSOR + ADIREX_RECURSE + ADIREX_FULLPATH + ADIREX_STRING_FILEATTRIBUTES, 0, 'Filename,Creationtime,Accesstime,Filesize,Fileattribs,cfileattribs')
BROWSE
*!*	AERROREX('laError')
*!*	DISPLAY MEMORY LIKE laError
&& ? SECONDS() - lnTime
&& SELECT cFiles 

FUNCTION AdirExCallback(cFileName,cDosFileName,tCreationTime,tLastAccessTime,tLastWriteTime,nFileSize,nFileAttributes)
?cFilename,cDosFileName,tCreationTime,tLastAccessTime,tLastWriteTime,nFileSize,nFileAttributes
ENDFUNC

FUNCTION AdirExCallback2(cFileName,tCreationTime,nFileSize,cFileAttribs)
?cFilename,tCreationTime,nFileSize,cFileAttribs
ENDFUNC

FUNCTION AdirExCallback3(nFileSize,tCreationTime,cFileName,nFileattribs)
?cFilename,tCreationTime,nFileSize,nFileAttribs
ENDFUNC