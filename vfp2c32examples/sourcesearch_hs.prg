#INCLUDE "vfp2c.h"
#INCLUDE "windows.h"
#INCLUDE "hyperscan.h"

CD (FULLPATH(JUSTPATH(SYS(16))))

IF TYPE('_WIN64') == 'L' AND _WIN64
	IF !("VFP2C64.FLL" $ SET("Library"))
		SET LIBRARY TO vfp2c64.fll ADDITIVE
	ENDIF
ELSE
	IF !("VFP2C32.FLL" $ SET("Library"))
		SET LIBRARY TO vfp2c32.fll ADDITIVE
	ENDIF
ENDIF

_SCREEN.Cls()


&& SET COVERAGE TO (ADDBS(FULLPATH(JUSTPATH(SYS(16)))) + 'coverage.txt')

LOCAL xj, lcPath
*!*	m.lcPath = 'C:\syncapi.h'
*!*	m.lcPath = 'C:\*'
m.lcPath = 'C:\Projects\*.php'
*!*	m.lcPath = "C:\Projects\*.scx|*.vcx"
*!*	m.lcPath = 'C:\Projects\websites\geve4-online_chris\testfile._php'
*!*	m.lcPath = 'C:\Projects\websites\tcr\administrator\components\com_config\config.class.php'
*!*	m.lcPath = 'C:\Projects\websites\rtv\plugins\mobile\scientia\scientia\TeraWurflExceptions\TeraWurflExceptions.php'
*!*	m.lcPath = 'C:\Projects\git\joomla-platform\libraries\phpmailer\pop3.php'
*!*	m.lcPath = 'C:\Projects\websites\*.php'
*!*	m.lcPath = 'C:\OpenComposite\*'
*!*	m.lcPath = 'C:\windows'
USE IN SELECT('curFiles1')
USE IN SELECT('curMatches1')
USE IN SELECT('curMatches2')
USE IN SELECT('curMatches3')
USE IN SELECT('curMatches4')

? TestAdirEx('AdirEx', m.lcPath, 'curFiles1', BITOR(ADIREX_DEST_CURSOR, ADIREX_RECURSE,  ADIREX_FULLPATH))
*!*	? TestScan1('curFiles1', 'curMatches1', 'fileid')
*!*	? TestScan2('curFiles1', 'curMatches2', 'fileid')
*!*	? TestScan3('curFiles1', 'curMatches3', 'fileid')
*!*	? TestScan4('curFiles1', 'curMatches4', 'fileid')
*!*	? TestScan5('curFiles1', 'curMatches1', 'fileid I NOT NULL')
*!*	? TestScan6('curFiles1', 'curMatches1', 'fileid')
? TestScan8('curFiles1', 'curMatches1', 'fileid I NULL')

RETURN

*!*	FOR m.xj = 1 TO 3
*!*		INKEY(0.25,'H')
*!*		USE IN SELECT('curFiles1')	
*!*		? TestAdirEx('AdirEx NoDos', m.lcPath, 'curFiles1', BITOR(ADIREX_DEST_CURSOR, ADIREX_RECURSE, ADIREX_NODOSNAME, ADIREX_FILEID, ADIREX_PARENTID))
*!*		INKEY(0.25,'H')

*!*	*!*		SELECT parentid FROM curfiles1 WHERE BITAND(curfiles1.fileattribs, FILE_ATTRIBUTE_REPARSE_POINT + FILE_ATTRIBUTE_DIRECTORY) = 0 GROUP BY 1 INTO CURSOR curids
*!*	*!*		SELECT fileid FROM curfiles1 WHERE fileid JOIN 
*!*			
*!*	*!*		DELETE curFiles1 FROM (SELECT fileid curFiles1 AS cf1 WHERE cf1.parentid = curfiles1.fileid AND BITAND(cf1.fileattribs, FILE_ATTRIBUTE_DIRECTORY) = 0

*!*		INKEY(0.25,'H')
*!*		USE IN SELECT('curFiles2')		
*!*		? TestAdirEx('AdirEx', m.lcPath, 'curFiles2', BITOR(ADIREX_DEST_CURSOR, ADIREX_RECURSE, ADIREX_NODOSNAME, ADIREX_FILEID, ADIREX_PARENTID))
*!*		INKEY(0.5,'H')
*!*	ENDFOR	

? CompareCursors('curfiles1', 'curfiles1')
RETURN

*!*	ENDFOR
&&  AUTOINC NEXTVALUE 1 STEP 1 DEFAULT GETAUTOINCVALUE(0)
*!*	USE IN SELECT('curfiles')
*!*	CREATE CURSOR curfiles(fileid I, filename M, writetime T, filesize N(10,0), fileattribs I) 

*!*	FOR m.xj = 1 TO 5
*!*	PUBLIC laFiles[1,1]
*!*	TestAdirEx('AdirEx - normal', m.lcPath, 'laFiles', ADIREX_RECURSE)
*!*	?ALEN(laFiles,1)
*!*	?ALEN(laFiles,2)
*!*	TestAdirEx('AdirEx - NoDosName', m.lcPath, 'laFiles', ADIREX_RECURSE + ADIREX_NODOSNAME)
*!*	?ALEN(laFiles,1)
*!*	?ALEN(laFiles,2)
*!*	TestAdirEx('AdirEx - NoDosName,FileId', m.lcPath, 'laFiles', ADIREX_RECURSE + ADIREX_NODOSNAME + ADIREX_FILEID)
*!*	?ALEN(laFiles,1)
*!*	?ALEN(laFiles,2)
*!*	TestAdirEx('AdirEx - NoDosName,FileId,ParentId', m.lcPath, 'laFiles', ADIREX_RECURSE + ADIREX_NODOSNAME + ADIREX_FILEID + ADIREX_PARENTID)
*!*	?ALEN(laFiles,1)
*!*	?ALEN(laFiles,2)

PUBLIC lnCounter 
LOCAL laThreads[1], lnThreadCount
*!*	TestAdirEx('AdirEx CB - normal', m.lcPath, 'AdirExCallback1', ADIREX_DEST_CALLBACK + ADIREX_RECURSE)
*!*	TestAdirEx('AdirEx CB - NoDosName', m.lcPath, 'AdirExCallback2', ADIREX_DEST_CALLBACK +  ADIREX_RECURSE + ADIREX_NODOSNAME)
*!*	TestAdirEx('AdirEx CB - NoDosName,FileId', m.lcPath, 'AdirExCallback3', ADIREX_DEST_CALLBACK +  ADIREX_RECURSE + ADIREX_NODOSNAME + ADIREX_FILEID)
*!*	TestAdirEx('AdirEx CB - NoDosName,FileId,ParentId', m.lcPath, 'AdirExCallback4', ADIREX_DEST_CALLBACK + ADIREX_RECURSE + ADIREX_NODOSNAME + ADIREX_FILEID + ADIREX_PARENTID)
*!*	FOR m.xj = 1 TO 100
*!*		m.lnCounter = 1
*!*		TestAdirEx('AdirEx CB - normal', m.lcPath, 'AdirExCallback5', ADIREX_DEST_CALLBACK + ADIREX_MULTITHREADED +  ADIREX_RECURSE)
*!*		m.lnThreadCount = APROCESSTHREADS("laThreads",_VFP.ProcessId)
*!*		? m.lnThreadCount
*!*	ENDFOR
*!*	TestAdirEx('AdirEx', m.lcPath, 'curfiles1', ADIREX_DEST_CURSOR + ADIREX_RECURSE + ADIREX_NO_DOSNAME)
*!*	TestAdirEx('AdirEx', m.lcPath, 'curfiles2', ADIREX_DEST_CURSOR + ADIREX_RECURSE + ADIREX_MULTITHREADED + ADIREX_NO_DOSNAME)
*!*	TestAdirEx('AdirEx MT - NtQueryDirectory', m.lcPath, 'curFiles4', ADIREX_FULLPATH + ADIREX_RECURSE + ADIREX_MULTITHREADED + ADIREX_NO_DOSNAME)
*!*	ENDFOR
*!*	RETURN
RETURN

INKEY(1)

*!*	? CompareCursors('curFiles1', 'curFiles2')
*!*	? CompareCursors('curFiles2', 'curFiles3')
*!*	? CompareCursors('curFiles3', 'curFiles4')
*!*	SET TALK ON
*!*	LOCAL xj
*!*	FOR m.xj = 1 TO 10
*!*	TestScan2('curFiles', 'curMatches2', 'fileid')
*!*	TestScan3('curFiles', 'curMatches3', 'fileid')
*!*	TestScan4('curFiles', 'curMatches4', 'fileid')
*!*	ENDFOR
RETURN


*!*	? "C:\", 1197155
*!*	DirStat(m.lcPath)
RETURN

FUNCTION DirStat
	LPARAMETERS lcDir
	LOCAL lnTotalFileCount, lnTotalDirCount, lnTotalReparseCount, lnRet, lnFileCount, ;
		lnReparseCount, lnDirCount, lnCursorCounter, lcCursor
	m.lnTotalFileCount = 0
	m.lnTotalDirCount = 0
	m.lnTotalReparseCount = 0
	m.lnCursorCounter = 2
	TestAdirEx('AdirEx', ADDBS(m.lcDir) + '*', 'curFiles1', ADIREX_DEST_CURSOR + ADIREX_FULLPATH)
	CREATE CURSOR fileresults (dirname C(254), ccursor C(30), filecount I, reparsecount I, dircount I)
	SELECT curFiles1
	SCAN FOR BITAND(curFiles1.fileattribs, FILE_ATTRIBUTE_DIRECTORY) > 0
		m.lcCursor = 'curFiles' + ALLTRIM(STR(m.lnCursorCounter))
		USE IN SELECT(m.lcCursor)
		m.lnRet = TestAdirEx('AdirEx', ADDBS(curFiles1.filename) + '*', m.lcCursor, ADIREX_DEST_CURSOR + ADIREX_FULLPATH + ADIREX_RECURSE + ADIREX_MULTITHREADED)
		SELECT &lcCursor
		COUNT FOR BITAND(&lcCursor..fileattribs, FILE_ATTRIBUTE_DIRECTORY) > 0 TO lnDirCount
		COUNT FOR BITAND(&lcCursor..fileattribs, FILE_ATTRIBUTE_DIRECTORY) == 0 TO lnFileCount
		COUNT FOR BITAND(&lcCursor..fileattribs, FILE_ATTRIBUTE_REPARSE_POINT) > 0 TO lnReparseCount
		INSERT INTO fileresults VALUES(curFiles1.filename, m.lcCursor, m.lnFileCount, m.lnReparseCount, m.lnDirCount)
		m.lnTotalFileCount = m.lnTotalFileCount + m.lnFileCount
		m.lnTotalDirCount = m.lnTotalDirCount + m.lnDirCount
		m.lnTotalReparseCount = m.lnTotalReparseCount + m.lnReparseCount
		m.lnCursorCounter = m.lnCursorCounter+ 1
	ENDSCAN
	INSERT INTO fileresults VALUES("TOTAL", "", m.lnTotalFileCount, m.lnTotalReparseCount, m.lnTotalDirCount)
	SELECT fileresults 
	BROWSE NOWAIT
ENDFUNC
&& TestAdirEx('AdirEx MT NoDos- NtQueryDirectory', m.lcPath, 'curFiles4', ADIREX_DEST_CURSOR + ADIREX_RECURSE + ADIREX_NODOSNAME + ADIREX_MULTITHREADED)	

FUNCTION TestAdirEx
	LPARAMETERS lcTest, lcPath, lcDest, lnFlags
	LOCAL lnStart, lnEnd, lnCount
	&& USE IN SELECT(m.lcCursor)
	m.lnStart = SECONDS()
	TRY
		m.lnCount = ADIREX(m.lcDest, m.lcPath, 0, m.lnFlags, 0, 'fileid,filename,filesize,fileattribs')
	CATCH TO loError
		? m.lcPath, m.loError.Message
		m.lnCount = 0
	ENDTRY
	m.lnEnd = SECONDS()
	? m.lcTest, m.lnEnd - m.lnStart
	RETURN m.lnCount
ENDFUNC

FUNCTION AdirExCallback1
	LPARAMETER cFileName,cDosFileName,tCreationTime,tLastAccessTime,tLastWriteTime,nFileSize,nFileAttributes
	? "AdirExCallback1", PCOUNT()
	RETURN .F.
ENDFUNC

FUNCTION AdirExCallback2
	LPARAMETER cFileName,tCreationTime,tLastAccessTime,tLastWriteTime,nFileSize,nFileAttributes
	? "AdirExCallback2", PCOUNT()
	RETURN .F.
ENDFUNC

FUNCTION AdirExCallback3
	LPARAMETER nFileId, cFileName,tCreationTime,tLastAccessTime,tLastWriteTime,nFileSize,nFileAttributes
	? "AdirExCallback3", PCOUNT()
	RETURN .F.
ENDFUNC

FUNCTION AdirExCallback4
	LPARAMETER nFileId, nParentId, cFileName,tCreationTime,tLastAccessTime,tLastWriteTime,nFileSize,nFileAttributes
	? "AdirExCallback4", PCOUNT()
	RETURN .F.
ENDFUNC

FUNCTION AdirExCallback5
	LPARAMETER nFileId, nParentId, cFileName,tCreationTime,tLastAccessTime,tLastWriteTime,nFileSize,nFileAttributes
*!*		? "AdirExCallback5", PCOUNT()
	m.lnCounter = m.lnCounter + 1
	IF m.lnCounter = 10000
		? "Aborting callback"
		RETURN .F.
	ENDIF
	RETURN .T.
ENDFUNC

FUNCTION TestScan1
	LPARAMETERS lcCursor, lcResultCursor, lcContextField
	LOCAL lnStart, lnEnd, lnRet
	m.lnStart = SECONDS()
	m.lnRet = hyperscan_scan_files('(public|private|protected)\s+function\s+([_a-zA-Z0-9]+)\s*\(', 0, ;
		HS_FLAG_MULTILINE, HS_MODE_SOM_HORIZON_SMALL, m.lcCursor, m.lcResultCursor, m.lcContextField)
	IF m.lnRet != 0
		? "Error hyperscan_scan_files", m.lnRet
		RETURN
	ENDIF 
	m.lnEnd = SECONDS()
	? "Scan", m.lnEnd - m.lnStart
ENDFUNC

FUNCTION TestScan2
	LPARAMETERS lcCursor, lcResultCursor, lcContextField
	LOCAL lnStart, lnEnd, lnRet
	m.lnStart = SECONDS()
	m.lnRet = hyperscan_scan_files('(public|private|protected)\s+function\s+([_a-zA-Z0-9]+)\s*\(', 0, ;
		HS_FLAG_MULTILINE, HS_MODE_SOM_HORIZON_SMALL + HS_MODE_EX_LINENO, m.lcCursor, m.lcResultCursor, m.lcContextField)
	IF m.lnRet != 0
		? "Error hyperscan_scan_files", m.lnRet
		RETURN
	ENDIF 
	m.lnEnd = SECONDS()
	? "Scan", m.lnEnd - m.lnStart
ENDFUNC

FUNCTION TestScan3
	LPARAMETERS lcCursor, lcResultCursor, lcContextField
	LOCAL lnStart, lnEnd, lnRet
	m.lnStart = SECONDS()
	m.lnRet = hyperscan_scan_files('(public|private|protected)\s+function\s+([_a-zA-Z0-9]+)\s*\(', 0, 
		HS_FLAG_MULTILINE, HS_MODE_SOM_HORIZON_SMALL, ;
		m.lcCursor, m.lcResultCursor, m.lcContextField)
	IF m.lnRet != 0
		? "Error hyperscan_scan_files", m.lnRet
		RETURN
	ENDIF 
	m.lnEnd = SECONDS()
	? "Scan", m.lnEnd - m.lnStart
ENDFUNC

FUNCTION TestScan4
	LPARAMETERS lcCursor, lcResultCursor, lcContextField
	LOCAL lnStart, lnEnd, lnRet
	m.lnStart = SECONDS()
	m.lnRet = hyperscan_scan_files('(public|private|protected)\s+function\s+([_a-zA-Z0-9]+)\s*\(', 0, ;
		HS_FLAG_MULTILINE, HS_MODE_SOM_HORIZON_SMALL + HS_MODE_EX_LINENO, m.lcCursor, m.lcResultCursor, m.lcContextField)

	IF m.lnRet != 0
		? "Error hyperscan_scan_files", m.lnRet
		RETURN
	ENDIF 
	m.lnEnd = SECONDS()
	? "Scan", m.lnEnd - m.lnStart
ENDFUNC

FUNCTION TestScan5
	LPARAMETERS lcCursor, lcResultCursor, lcContextField
	LOCAL lnStart, lnEnd, lnRet, lnDB, lnScratch, lnMode, loExt
	LOCAL laRegs[3]
	LOCAL laExprExt[3]
	
	m.lnStart = SECONDS()
	m.lnDb = 0
	m.lnScratch = 0
	m.lnRet = 0
	
*!*		m.lnMode = HS_MODE_STREAM + HS_MODE_SOM_HORIZON_SMALL + HS_MODE_EX_LINENO + HS_MODE_EX_MATCH_ID
	m.lnMode = HS_MODE_BLOCK + HS_MODE_EX_LINENO + HS_MODE_EX_MATCH_ID
	m.loExt = CREATEOBJECT('Empty')
	ADDPROPERTY(m.loExt, 'min_offset', 10)
	ADDPROPERTY(m.loExt, 'max_offset', 2000)	

	m.laExprExt[1] = .F.
	m.laExprExt[2] = .F.
	m.laExprExt[3] = m.loExt

	m.laRegs[1] = '\bnamespace\s+([\\_a-zA-Z0-9]+)\b'
	m.laRegs[2] = '\bclass\s+([_a-zA-Z0-9]+)\b'
	m.laRegs[3] = '\b(public|private|protected)\s+function\s+([_a-zA-Z0-9]+)\b'

	
	TRY
		m.lnRet = hyperscan_compile(@m.laRegs, @m.lnDB, 0, m.lnMode, 'laExprExt')
		IF m.lnRet != HS_SUCCESS
			THROW m.lnRet
		ENDIF

		m.lnRet = hyperscan_alloc_scratch(m.lnDB, @m.lnScratch)
		IF m.lnRet != HS_SUCCESS
			THROW m.lnRet
		ENDIF 

		m.lnRet = hyperscan_scan_files(m.lnDB, m.lnScratch, m.lnMode, m.lcCursor, m.lcResultCursor, m.lcContextField, (32 * 1024))
		IF m.lnRet != HS_SUCCESS
			THROW m.lnRet
		ENDIF 
		
	CATCH TO loError

		? "Error", m.lnRet
		IF m.lnRet != 0
			AERROREX("laError")
			DISPLAY MEMORY LIKE laError
		ELSE
			? loError.LineNo, loError.ErrorNo, loError.UserValue, loError.Message
		ENDIF

	FINALLY

		IF m.lnScratch != 0
			m.lnRet = hyperscan_free_scratch(m.lnScratch)
		ENDIF

		IF m.lnDB != 0
			m.lnRet = hyperscan_free_database(m.lnDB)
		ENDIF

	ENDTRY

	m.lnEnd = SECONDS()
	? "Scan", m.lnEnd - m.lnStart	
ENDFUNC

FUNCTION TestScan6
	LPARAMETERS lcCursor, lcResultCursor, lcContextField
	LOCAL lnStart, lnEnd, lnRet, lnDb, lnScratch, lnMode, lnWorkArea, laRegs[2], laFlags[2], lcFile, lcFileExt
	m.lnStart = SECONDS()
	m.lnDb = 0
	m.lnScratch = 0

	m.lnMode = HS_MODE_STREAM + HS_MODE_SOM_HORIZON_SMALL + HS_MODE_EX_LINENO + HS_MODE_EX_MATCH_ID
	m.laRegs[1] = '^PROCEDURE\s+([\\_a-zA-Z0-9]+)\b'
	m.laRegs[2] = '^ENDPROC\b'
	
	m.laFlags[1] = HS_FLAG_MULTILINE
	m.laFlags[2] = HS_FLAG_MULTILINE

	TRY
		m.lnRet = hyperscan_compile(@m.laRegs, @m.lnDb, @m.laFlags, m.lnMode)

		IF m.lnRet != 0
			THROW m.lnRet
		ENDIF 

		m.lnRet = hyperscan_alloc_scratch(m.lnDB, @m.lnScratch)
		IF m.lnRet != 0
			THROW m.lnRet
		ENDIF

		CREATE CURSOR (lcResultCursor) (fileid I NOT NULL, match_id N(10,0) NOT NULL, match_from N(20,0) NOT NULL, match_to N(20,0) NOT NULL, ;
			 match_lineno N(20,0) NOT NULL, match_expr M NOT NULL)
		m.lnWorkArea = SELECT(0)

		SELECT (lcCursor)
		SCAN
			m.lcFile = EVALUATE(m.lcCursor + '.filename')
			m.lcFileExt = JUSTEXT(m.lcFile)
			? m.lcFile
			USE (m.lcFile) IN 0 SHARED ALIAS vfp2c_search
			DO CASE
				CASE m.lcFileExt == 'vcx' OR m.lcFileExt == 'scx'
					m.lnRet = hyperscan_scan_cursor(m.lnDB, m.lnScratch, m.lnMode, ;
						'vfp2c_search', 'methods', m.lnWorkArea, 'fileid I NOT NULL', EVALUATE(m.lcCursor + '.fileid'))
				CASE m.lcFileExt == 'frx'
				CASE m.lcFileExt == 'lbx'
				CASE m.lcFileExt == 'mnx'
			ENDCASE
			USE IN SELECT('vfp2c_search')
		ENDSCAN

	CATCH TO loError
	
		? "Error", m.lnRet
		IF m.lnRet != 0
			AERROREX("laError")
			DISPLAY MEMORY LIKE laError
		ELSE
			? loError.LineNo, loError.ErrorNo, loError.Message
		ENDIF

	FINALLY

		IF m.lnScratch != 0
			m.lnRet = hyperscan_free_scratch(m.lnScratch)
		ENDIF
		IF m.lnDB != 0
			m.lnRet = hyperscan_free_database(m.lnDB)
		ENDIF

	ENDTRY	


	m.lnEnd = SECONDS()	
	? "Scan", m.lnEnd - m.lnStart
ENDFUNC

FUNCTION TestScan7
	LPARAMETERS lcCursor, lcResultCursor, lcContextField
	LOCAL lnStart, lnEnd, lnRet, lnDb, lnScratch, lnMode, lnWorkArea, laRegs[3], laFlags[3], lcFile, lcFileExt
	m.lnDb = 0
	m.lnScratch = 0

	m.lnMode = HS_MODE_STREAM + HS_MODE_SOM_HORIZON_MEDIUM + HS_MODE_EX_LINENO + HS_MODE_EX_MATCH_ID

	m.laRegs[1] = '^namespace\s+([\\_a-zA-Z0-9]+)\b'
	m.laRegs[2] = '^class\s+([_a-zA-Z0-9]+)\b'
	m.laRegs[3] = '^(public|private|protected)\s+function\s+([_a-zA-Z0-9]+)\b'
	
	m.laFlags[1] = HS_FLAG_MULTILINE
	m.laFlags[2] = HS_FLAG_MULTILINE
	m.laFlags[3] = HS_FLAG_MULTILINE	

	TRY
		m.lnRet = hyperscan_compile(@m.laRegs, @m.lnDb, @m.laFlags, m.lnMode)

		IF m.lnRet != 0
			THROW m.lnRet
		ENDIF 

		m.lnRet = hyperscan_alloc_scratch(m.lnDB, @m.lnScratch)
		IF m.lnRet != 0
			THROW m.lnRet
		ENDIF

		CREATE CURSOR filecontent (fileid I NOT NULL, content MEMO NOT NULL)
		SELECT (lcCursor)
		SCAN
			m.lcFile = EVALUATE(m.lcCursor + '.filename')
			INSERT INTO filecontent VALUES(EVALUATE(m.lcCursor + '.fileid'), FILETOSTR(m.lcFile))
		ENDSCAN
		GO TOP 
		
		CREATE CURSOR (lcResultCursor) (fileid I NOT NULL, match_id N(10,0) NOT NULL, match_from N(20,0) NOT NULL, match_to N(20,0) NOT NULL, ;
			 match_lineno N(20,0) NOT NULL, match_expr M NOT NULL)
		m.lnWorkArea = SELECT(0)

		m.lnStart = SECONDS()

		m.lnRet = hyperscan_scan_cursor(m.lnDB, m.lnScratch, 0, m.lnMode, ;
				'filecontent', 'content', m.lnWorkArea, 'fileid', EVALUATE(m.lcCursor + '.fileid'))

	CATCH TO loError
	
		? "Error", m.lnRet
		IF m.lnRet != 0
			AERROREX("laError")
			DISPLAY MEMORY LIKE laError
		ELSE
			? loError.LineNo, loError.ErrorNo, loError.Message
		ENDIF

	FINALLY

		IF m.lnScratch != 0
			m.lnRet = hyperscan_free_scratch(m.lnScratch)
		ENDIF
		IF m.lnDB != 0
			m.lnRet = hyperscan_free_database(m.lnDB)
		ENDIF

	ENDTRY	

	m.lnEnd = SECONDS()	
	? "Scan", m.lnEnd - m.lnStart
ENDFUNC

FUNCTION TestScan8
	LPARAMETERS lcCursor, lcResultCursor, lcContextField
	LOCAL lnStart, lnEnd, lnRet, lnDB, lnScratch, lnMode, loPlatform
	LOCAL laRegs[3], laFlags[3]
	
	m.lnStart = SECONDS()
	m.lnDb = 0
	m.lnScratch = 0
	m.lnRet = 0
	
*!*		m.lnMode = HS_MODE_STREAM + HS_MODE_SOM_HORIZON_SMALL + HS_MODE_EX_SKIP_OVERLAPPED_EXPR
*!*		m.lnMode = HS_MODE_BLOCK + HS_MODE_EX_SKIP_OVERLAPPED_EXPR
*!*		m.lcRegs = 'a{16000}'
	m.lnMode = HS_MODE_STREAM + HS_MODE_SOM_HORIZON_MEDIUM + HS_MODE_EX_LINENO + HS_MODE_EX_MATCH_ID

	m.laRegs[1] = '^namespace\s+([\\_a-zA-Z0-9]+)\b'
	m.laRegs[2] = '^class\s+([_a-zA-Z0-9]+)\b'
	m.laRegs[3] = '^(public|private|protected)\s+function\s+([_a-zA-Z0-9]+)\b'
	
	m.laFlags[1] = HS_FLAG_MULTILINE
	m.laFlags[2] = HS_FLAG_MULTILINE
	m.laFlags[3] = HS_FLAG_MULTILINE
	
	TRY
	
		m.lnRet = hyperscan_platform(@m.loPlatform)
		IF m.lnRet != HS_SUCCESS
			THROW m.lnRet
		ENDIF
	
		m.lnRet = hyperscan_compile(@m.laRegs, @m.lnDB, @m.laFlags, m.lnMode, .NULL., m.loPlatform)
		IF m.lnRet != HS_SUCCESS
			THROW m.lnRet
		ENDIF

		m.lnRet = hyperscan_alloc_scratch(m.lnDB, @m.lnScratch)
		IF m.lnRet != HS_SUCCESS
			THROW m.lnRet
		ENDIF 

		m.lnRet = hyperscan_scan_files(m.lnDB, m.lnScratch, m.lnMode, m.lcCursor, m.lcResultCursor, m.lcContextField)
		IF m.lnRet != HS_SUCCESS
			THROW m.lnRet
		ENDIF 
		
	CATCH TO loError

		? "Error", m.lnRet
		IF m.lnRet != 0
			AERROREX("laError")
			DISPLAY MEMORY LIKE laError
		ELSE
			? loError.LineNo, loError.ErrorNo, loError.UserValue, loError.Message
		ENDIF

	FINALLY

		IF m.lnScratch != 0
			m.lnRet = hyperscan_free_scratch(m.lnScratch)
		ENDIF

		IF m.lnDB != 0
			m.lnRet = hyperscan_free_database(m.lnDB)
		ENDIF

	ENDTRY

	m.lnEnd = SECONDS()
	? "Scan", m.lnEnd - m.lnStart	
ENDFUNC

FUNCTION CompareCursors(lcCursor1, lcCursor2)
	LOCAL lbEqual
	m.lbEqual = .T.
	GO TOP IN (m.lcCursor2)
	SELECT (m.lcCursor1)
	SCAN
		IF !(EVALUATE(m.lcCursor1 + '.filename') == EVALUATE(m.lcCursor2 + '.filename')) OR ;
			EVALUATE(m.lcCursor1 + '.creationtime') != EVALUATE(m.lcCursor2 + '.creationtime') OR ;
			EVALUATE(m.lcCursor1 + '.filesize') != EVALUATE(m.lcCursor2 + '.filesize') OR ;
			EVALUATE(m.lcCursor1 + '.fileattribs') != EVALUATE(m.lcCursor2 + '.fileattribs')

			? RECNO(m.lcCursor1), EVALUATE(m.lcCursor1 + '.filename'), EVALUATE(m.lcCursor2 + '.filename')
			? EVALUATE(m.lcCursor1 + '.filesize'), EVALUATE(m.lcCursor2 + '.filesize')
			? EVALUATE(m.lcCursor1 + '.fileattribs'), EVALUATE(m.lcCursor2 + '.fileattribs')
			m.lbEqual = .F.
			RETURN .F.
		ENDIF
		SKIP 1 IN (m.lcCursor2)
	ENDSCAN
	RETURN m.lbEqual
ENDFUNC