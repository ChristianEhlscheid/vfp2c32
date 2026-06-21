#INCLUDE "vfp2c.h"

CD (FULLPATH(JUSTPATH(SYS(16))))

IF TYPE('_WIN64') == 'L' AND _WIN64
SET LIBRARY TO vfp2c64.fll ADDITIVE
ELSE
SET LIBRARY TO vfp2c32.fll ADDITIVE
ENDIF

_SCREEN.Cls()

&& ---------------
&& ADIREX examples
&& ---------------
LOCAL lcPath, lnStart, lnEnd, lnFlags, lnAttribs
*!*	IF !SHBROWSEFOLDER('Select a path to search', BIF_RETURNONLYFSDIRS + BIF_NEWDIALOGSTYLE, @lcPath)
*!*		RETURN
*!*	ENDIF

m.lcPath = 'C:\Users\*.*'
*!*	m.lcPath = 'C:\Projects\imagezip\*.jpg|*.bmp|*.gif|*.jpeg|*.png'

USE IN SELECT('cFiles1')
m.lnFlags = ADIREX_DEST_CURSOR + ADIREX_INCLUDE_REPARSE_POINT + ADIREX_FULLPATH + ADIREX_RECURSE && + ADIREX_JUSTFNAME
m.lnAttribs = ''
m.lnStart = SECONDS()
? ADIREX('cFiles1', m.lcPath, m.lnAttribs, m.lnFlags, 0, 'fileid,filename,creationtime,filesize,fileattribs')
m.lnEnd = SECONDS()
? m.lnEnd - m.lnStart

*!*	LOCAL ldCreation
*!*	SELECT cFiles1
*!*	SCAN
*!*	GETFILETIMES(@cFiles1.filename, @ldCreation)
*!*	ENDSCAN

*!*	USE IN SELECT('cFiles2')
*!*	SET LIBRARY TO 
*!*	SET LIBRARY TO vfp2c32_rel.fll ADDITIVE
*!*	m.lnFlags = ADIREX_DEST_CURSOR + ADIREX_INCLUDE_REPARSE_POINT + ADIREX_FULLPATH + ADIREX_RECURSE && ADIREX_RECURSE  + ADIREX_JUSTFNAME
*!*	m.lnAttribs = ''
*!*	m.lnStart = SECONDS()
*!*	? ADIREX('cFiles2', m.lcPath, m.lnAttribs, m.lnFlags, 0, 'fileid,filename,creationtime,filesize,fileattribs')
*!*	m.lnEnd = SECONDS()
*!*	? m.lnEnd - m.lnStart

*!*	SET LIBRARY TO 

*!*	? CompareCursors('cFiles1', 'cFiles2')

*!*	SetFileAttributesEx(m.lcPath, '-R', 0, 0)
*!*	m.lnAttribs = '-R'
*!*	? ADIREX('cFiles2', m.lcPath, m.lnAttribs, m.lnFlags, 0, 'fileid,filename,filesize,fileattribs')

*!*	? ADIREX('cFiles', m.lcPath, '-A-R', ADIREX_DEST_CURSOR + ADIREX_FULLPATH, 0, 'Filename,Creationtime,Accesstime,Filesize,Fileattribs,cfileattribs')
*!*	? ADIREX('cFiles2', m.lcPath, FILE_ATTRIBUTE_ARCHIVE, ADIREX_DEST_CURSOR + ADIREX_FULLPATH + ADIREX_FILTER_NONE, 0, 'Filename,Creationtime,Accesstime,Filesize,Fileattribs,cfileattribs')
*!*	RETURN

&& enumerate all files in some directory into array laFiles
*!*	lnCount = ADIREX('laFiles',ADDBS(FULLPATH(CURDIR()))+"*.*")

*!*	&& enumerate all subdirectories in some directory into array ..
*!*	&& default filter algorithm: BITAND(nAttributes,nYourFilter) > 0 - or in words .. one of the attributes have to exist for a match
*!*	lnCount = ADIREX('laFiles',ADDBS(FULLPATH(CURDIR()))+"*.*",FILE_ATTRIBUTE_DIRECTORY)

*!*	&& enumerate all read only files into array..
*!*	lnCount = ADIREX('laFiles',ADDBS(FULLPATH(CURDIR()))+"*.*",FILE_ATTRIBUTE_READONLY)

*!*	&& enumerate all files into cursor "curFiles" with default fieldnames
*!*	lnCount = ADIREX('curFiles',ADDBS(FULLPATH(CURDIR()))+"*.*",0,ADIREX_DEST_CURSOR)
*!*	USE IN curfiles

*!*	&& enumerate all files by calling back into AdirCallback function
*!*	?ADIREX('AdirExCallback',ADDBS(FULLPATH(CURDIR()))+"*.*",0,ADIREX_DEST_CALLBACK)

*!*	&& enumerate all system and hidden files into array laFiles
*!*	&& underlying filter algorithm: BITAND(nFileAttributes,nYourFilter) > 0
*!*	lnCount = ADIREX('laFiles',ADDBS(FULLPATH(CURDIR()))+"*.*",FILE_ATTRIBUTE_SYSTEM+FILE_ATTRIBUTE_HIDDEN)

*!*	&& enumerate all files expect system or hidden into array laFiles
*!*	&& underlying filter algorithm: BITAND(nFileAtributes,nYourFilter) == 0
*!*	lnCount = ADIREX('laFiles',ADDBS(FULLPATH(CURDIR()))+"*.*",FILE_ATTRIBUTE_SYSTEM+FILE_ATTRIBUTE_HIDDEN,ADIREX_FILTER_NONE)

*!*	&& enumerate only "read only" files (exact match, no other attributes)
*!*	&& underlying filter algorithm: nFileAttributes == nYourFilter
*!*	lnCount = ADIREX('laFiles',ADDBS(FULLPATH(CURDIR()))+"*.*",FILE_ATTRIBUTE_READONLY,ADIREX_FILTER_EXACT)

*!*	m.lcPath = 'C:\Projects\libcurl\*'
*!*	? ADIREX('AdirExCallback2', m.lcPath, 0, ADIREX_DEST_CALLBACK + ADIREX_FULLPATH + ADIREX_RECURSE + ADIREX_NTQUERYDIRECTORY + ADIREX_NO_DOSNAME)
*!*	? ADIREX('AdirExCallback2', m.lcPath, 0, ADIREX_DEST_CALLBACK + ADIREX_FULLPATH + ADIREX_RECURSE + ADIREX_ASYNC + ADIREX_NO_DOSNAME)
*!*	? ADIREX('AdirExCallback2', m.lcPath, 0, ADIREX_DEST_CALLBACK + ADIREX_FULLPATH + ADIREX_RECURSE + ADIREX_MULTITHREADED + ADIREX_NO_DOSNAME)
*!*	? ADIREX('AdirExCallback2', m.lcPath, 0, ADIREX_DEST_CALLBACK + ADIREX_FULLPATH + ADIREX_NO_DOSNAME)
*!*	? ADIREX('AdirExCallback3', m.lcPath, FILE_ATTRIBUTE_DIRECTORY, ADIREX_DEST_CALLBACK + ADIREX_RECURSE + ADIREX_FULLPATH + ADIREX_FILTER_NONE, 0, 'filename, creationtime, filesize, cfileattribs')
*!*	? ADIREX('AdirExCallback4', m.lcPath, 'd', ADIREX_DEST_CALLBACK + ADIREX_RECURSE + ADIREX_FULLPATH, 0, 'filesize,creationtime, filename,fileattribs')

*!*	*!*	CREATE CURSOR cFiles (filename M, creationtime T, filesize N(20,0), fileattribs I)
*!*	USE IN SELECT('cFiles')
*!*	? ADIREX('cFiles', m.lcPath, '', ADIREX_DEST_CURSOR + ADIREX_FULLPATH + ADIREX_STRING_FILEATTRIBUTES + ADIREX_RECURSE, 0, '')
*!*	? ADIREX('cFiles', m.lcPath, '-A-R', ADIREX_DEST_CURSOR + ADIREX_FULLPATH, 0, 'Filename,Creationtime,Accesstime,Filesize,Fileattribs,cfileattribs')
*!*	? ADIREX('cFiles2', m.lcPath, FILE_ATTRIBUTE_ARCHIVE, ADIREX_DEST_CURSOR + ADIREX_FULLPATH + ADIREX_FILTER_NONE, 0, 'Filename,Creationtime,Accesstime,Filesize,Fileattribs,cfileattribs')
*!*	BROWSE
&& SELECT cFiles 

FUNCTION AdirExCallback(cFileName,cDosFileName,tCreationTime,tLastAccessTime,tLastWriteTime,nFileSize,nFileAttributes)
?cFilename,cDosFileName,tCreationTime,tLastAccessTime,tLastWriteTime,nFileSize,nFileAttributes
ENDFUNC

FUNCTION AdirExCallback2(cFileName,tCreationTime,tLastAccessTime,tLastWriteTime,nFileSize,nFileAttributes)
?cFilename,tCreationTime,tLastAccessTime,tLastWriteTime,nFileSize,nFileAttributes
RETURN .T.
ENDFUNC

FUNCTION AdirExCallback3(cFileName,tCreationTime,nFileSize,cFileAttribs)
?cFilename,tCreationTime,nFileSize,cFileAttribs
ENDFUNC

FUNCTION AdirExCallback4(nFileSize,tCreationTime,cFileName,nFileattribs)
?cFilename,tCreationTime,nFileSize,nFileAttribs
ENDFUNC

FUNCTION AdirExCallback5(nFileId,nParentId,cFileName,nFileSize,cFileAttribs)
	?nFileId,nParentId,cFilename,nFileSize,cFileAttribs
ENDFUNC

FUNCTION CompareCursors(lcCursor1, lcCursor2)
	LOCAL lbEqual
	m.lbEqual = .T.
	GO TOP IN (m.lcCursor2)
	SELECT (m.lcCursor1)
	SCAN
		IF EVALUATE(m.lcCursor1 + '.fileid') != EVALUATE(m.lcCursor2 + '.fileid') OR ;
			!(EVALUATE(m.lcCursor1 + '.filename') == EVALUATE(m.lcCursor2 + '.filename')) OR ;
			EVALUATE(m.lcCursor1 + '.creationtime') != EVALUATE(m.lcCursor2 + '.creationtime') OR ;
			EVALUATE(m.lcCursor1 + '.filesize') != EVALUATE(m.lcCursor2 + '.filesize') OR ;
			EVALUATE(m.lcCursor1 + '.fileattribs') != EVALUATE(m.lcCursor2 + '.fileattribs')

			? RECNO(m.lcCursor1), EVALUATE(m.lcCursor1 + '.fileid'), EVALUATE(m.lcCursor1 + '.filename'), EVALUATE(m.lcCursor2 + '.filename')
			? EVALUATE(m.lcCursor1 + '.filesize'), EVALUATE(m.lcCursor2 + '.filesize')
			? EVALUATE(m.lcCursor1 + '.fileattribs'), EVALUATE(m.lcCursor2 + '.fileattribs')
			m.lbEqual = .F.
			RETURN .F.
		ENDIF
		SKIP 1 IN (m.lcCursor2)
	ENDSCAN
	RETURN m.lbEqual
ENDFUNC

