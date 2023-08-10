#INCLUDE "vfp2c.h"

CD (FULLPATH(JUSTPATH(SYS(16))))

IF TYPE('_WIN64') == 'L' AND _WIN64
SET LIBRARY TO vfp2c64.fll ADDITIVE
ELSE
SET LIBRARY TO vfp2c32.fll ADDITIVE
ENDIF

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

&& enumerate all files by calling back into AdirCallback function
?ADIREX('AdirExCallback',ADDBS(FULLPATH(CURDIR()))+"*.*",0,ADIREX_DEST_CALLBACK)

&& enumerate all system and hidden files into array laFiles
&& underlying filter algorithm: BITAND(nFileAttributes,nYourFilter) == nYourFilter
lnCount = ADIREX('laFiles',ADDBS(FULLPATH(CURDIR()))+"*.*",FILE_ATTRIBUTE_SYSTEM+FILE_ATTRIBUTE_HIDDEN,ADIREX_FILTER_ALL)

&& enumerate all files expect system or hidden into array laFiles
&& underlying filter algorithm: BITAND(nFileAtributes,nYourFilter) == 0
lnCount = ADIREX('laFiles',ADDBS(FULLPATH(CURDIR()))+"*.*",FILE_ATTRIBUTE_SYSTEM+FILE_ATTRIBUTE_HIDDEN,ADIREX_FILTER_NONE)

&& enumerate only "read only" files (exact match, no other attributes)
&& underlying filter algorithm: nFileAttributes == nYourFilter
lnCount = ADIREX('laFiles',ADDBS(FULLPATH(CURDIR()))+"*.*",FILE_ATTRIBUTE_READONLY,ADIREX_FILTER_EXACT)

*!*	? ADIREX('AdirExCallback', m.lcPath, FILE_ATTRIBUTE_DIRECTORY, ADIREX_DEST_CALLBACK + ADIREX_FULLPATH)
*!*	? ADIREX('AdirExCallback2', m.lcPath, FILE_ATTRIBUTE_DIRECTORY,ADIREX_DEST_CALLBACK + ADIREX_RECURSE + ADIREX_FULLPATH + ADIREX_FILTER_NONE, 0, 'filename, creationtime, filesize, cfileattribs')
*!*	? ADIREX('AdirExCallback3', m.lcPath, 'd', ADIREX_DEST_CALLBACK + ADIREX_RECURSE + ADIREX_FULLPATH, 0, 'filesize,creationtime, filename,fileattribs')

*!*	*!*	CREATE CURSOR cFiles (filename M, creationtime T, filesize N(20,0), fileattribs I)
USE IN SELECT('cFiles')

LOCAL lcPath
m.lcPath = 'C:\Projects\imagezip\*'
? ADIREX('cFiles', m.lcPath, '', ADIREX_DEST_CURSOR + ADIREX_FULLPATH + ADIREX_STRING_FILEATTRIBUTES + ADIREX_RECURSE, 0, '')
*!*	? ADIREX('cFiles', m.lcPath, '-A-R', ADIREX_DEST_CURSOR + ADIREX_FULLPATH, 0, 'Filename,Creationtime,Accesstime,Filesize,Fileattribs,cfileattribs')
*!*	? ADIREX('cFiles2', m.lcPath, FILE_ATTRIBUTE_ARCHIVE, ADIREX_DEST_CURSOR + ADIREX_FULLPATH + ADIREX_FILTER_NONE, 0, 'Filename,Creationtime,Accesstime,Filesize,Fileattribs,cfileattribs')
BROWSE
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