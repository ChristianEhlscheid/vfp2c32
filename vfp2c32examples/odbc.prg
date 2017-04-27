#INCLUDE vfp2c.h

&& prerequisites: InitVFP2C32 must have been called with the VFP2C_INIT_ODBC flag set

CD (FULLPATH(JUSTPATH(SYS(16))))
SET LIBRARY TO vfp2c32.fll ADDITIVE
INITVFP2C32(VFP2C_INIT_ALL)

LOCAL lnCon, lValue, lnRet, laInfo[1]
lnCon = SQLCONNECT('mysql','root','*******')

lnRet = SQLEXECEX(lnCon,'USE cms')
lnRet = SQLEXECEX(lnCon,'SELECT * FROM cms_css', 'cCursor', '', 0, 'css_id I')
?lnRet
IF lnRet = -1
	AERROREX('laError')
	DISPLAY MEMORY LIKE laError
ENDIF

SQLDISCONNECT(lnCon)
RETURN


&& enable/disable ODBC tracing 
? SQLSETPROPEX(lnCon,"TRACE",.F.)

&& set ODBC trace file
? SQLSETPROPEX(lnCon,"TRACEFILE","C:\odbclog.txt")

lnRet = SQLGETPROPEX(lnCon,"TRACE",@lValue) 
? "ODBC debug tracing is" + IIF(lValue,"enabled","disabled")
IF lnRet = 2
	AERROREX('laInfo')
	DISPLAY MEMORY LIKE laInfo
	 && this is not an error state
	 && the underlying API function "SQLGetConnectAttr" can return the status code SQL_SUCCESS_WITH_INFO
	 && if it does the return value will be 2 instead of 1 and by calling
	 && AerrorEx() you can get at the information returned by "SQLGetDiagRec".
ENDIF

lnRet = SQLGETPROPEX(lnCon,"TRACEFILE",@lValue)
?"ODBC trace file is ", lValue

&& simple select 
lnRet = SQLEXECEX(lnCon,'SELECT someCol FROM yourTable WHERE someCol2 = 1','cCursor','laInfo')
IF lnRet > 0
	? "Cursor " + laInfo[1,1] + " contains " + ALLTRIM(STR(laInfo[1,2])) + " records"
ELSE
	AERROREX('laInfo')
	DISPLAY MEMORY LIKE laInfo
ENDIF

&& simple select with parameter markers
LOCAL lnPK
lnPK = 234
lnRet = SQLEXECEX(lnCon,'SELECT someCol FROM yourTable WHERE yourPK = ?{lnPK}','cCursor')
&& as you can see the only difference to SQLEXEC is that you have to enclose the parameter expression
&& in curly braces {}
&& the starting curly brace has to immediately follow the "?"
&& e.g.
&& lcSQL = "UPDATE someTable SET someCol = ?{someVar}" && is correct, but
&& lcSQL = "UPDATE someTable SET someCol = ?  {someVar}"
&& raises an error, since "{someVar}" is not removed from the SQL statement
&& before it is send to the DB backend 

&& simple update with parameter markers
LOCAL lcNewValue
lcNewValue = "someValue"
lnRet = SQLEXECEX(lnCon,'UPDATE yourTable SET someCol = ?{lcNewValue} WHERE yourPK = ?{lnPK}','','laInfo')
IF lnRet > 0
	? ALLTRIM(STR(laInfo[1,2])) + " rows updated"
ELSE
	AERROREX('laInfo')
	DISPLAY MEMORY LIKE laInfo
ENDIF


&& ----------------------------------- &&
&& USING the cursor & parameter schema &&
&& ----------------------------------- &&

&& truncating datetime to date
lnRet = SQLEXECEX(lnCon,'SELECT someDateTime FROM yourTable WHERE yourPK = ?{lnPK}','cCursor','',0,'someDateTime D NULL')

&& retrieve timestamp in binary form (to include milliseconds)
&& this will store a SQL_TIMESTAMP_STRUCT struct into the field
lnRet = SQLEXECEX(lnCon,'SELECT someTimeStamp FROM yourTable WHERE yourPK = ?{lnPK}','cCursor','',0,'someTimeStamp Q(16)')

&& updating timestamp retrieved in binary form
lnRet = SQLEXECEX(lnCon,'UPDATE yourTable SET someDateTime = ?{cCursor.someDateTime} WHERE yourPK = {lnPK}','','laInfo',0,'','1 SQL_TIMESTAMP(3)')

&& using the cursorschema to force retrieval of untranslated unicode text
lnRet = SQLEXECEX(lnCon,'SELECT someUnicodeCol FROM yourTable WHERE yourPK = ?{lnPK}','cCursor','',0,'someUnicodeCol C(200) NOCPTRANS')
&& or
lnRet = SQLEXECEX(lnCon,'SELECT someUnicodeCol FROM yourTable WHERE yourPK = ?{lnPK}','cCursor','',0,'someUnicodeCol Q(200)')
&& or
lnRet = SQLEXECEX(lnCon,'SELECT someUnicodeCol FROM yourTable WHERE yourPK = ?{lnPK}','cCursor','',0,'someUnicodeCol M NOCPTRANS')

&& updating some unicode column
&& if data is in ANSI format
LOCAL lcValue 
lcValue = "someansitext"
lnRet = SQLEXECEX(lnCon,'UPDATE yourTable SET someUnicodeCol = ?{lcValue} WHERE yourPK = {lnPK}','','laInfo')
&& as you see nothing special here ... the default 

&& if data is already in unicode format you have to specify this in the parameter schema (SQL_WCHAR stands for unicode)
lcValue = STRCONV("sometext",5)
lnRet = SQLEXECEX(lnCon,'UPDATE yourTable SET someUnicodeCol = ?{lcValue} WHERE yourPK = {lnPK}','','laInfo',0,'','1 SQL_WCHAR')

&& if you send a batch of several SQL statements or call a stored procedure that creates more than one resultset/cursor
&& you can specify a cursorschema for each, seperate them by a "|" character e.g.
LOCAL lcSQL 
lcSQL = "SELECT someCol FROM table1 WHERE someCol < 50" + CHR(13) + ;
		"SELECT someCol2 FROM table2 WHERE pk = ?{lnPK}"
lnRet = SQLEXECEX(lnCon,lcSQL,'cursor1,cursor2','laInfo',0,'someCol I | someCol2 C(200) NOCPTRANS')


&& using named parameters (only for stored procedures)
*!* T-SQL
*!*	CREATE PROCEDURE someProcedure AS (@lnPK INT = 0, @description VARCHAR(300) = 'Default',
*!*	@lnID INT = 0, @message VARCHAR(400) = 'Default')
*!*	AS
*!*	BEGIN
*!*	 .....
*!*	END
LOCAL lcDesc, lcMes
lcDesc = 'Hello'
lcMes = 'World'
lnRet = SQLEXECEX(lnCon,'{ CALL someProcedure(?{lcDesc},?{lcMes}) }','','laInfo',0,'','1 "@description",2 "@message"')


&& -------------------------------- &&
&& USING the callback functionality &&
&& -------------------------------- &&

lnRet = SQLEXECEX(lnCon,'{ CALL someLongRunningProcedureWithPRINTStatements(?{lnPK}) }','cCursor','laInfo',SQLEXECEX_CALLBACK_INFO,'','','SQLCallback')
?SQLEXECEX(lnCon,'SELECT mychar2 FROM testtab','cCursor','laInfo',0x10,'myChar C(60)','','','SQLCallback',100)
SQLEXECEX(lnCon,'SELECT mychar2 FROM testtab','cCursor','laInfo',0x010,'','','SQLCallback',100)

&& -------------------------------- &&
&& some nFlags settings				&&
&& -------------------------------- &&

&& store result into variables instead of a cursor (useful for aggregate functions)
LOCAL lnSum
lnRet = SQLEXECEX(lnCon,'SELECT SUM(someCol) FROM yourTable WHERE someCondition','lnSum','',SQLEXECEX_DEST_VARIABLE)
?lnSum

&& store result of several SQL command send in a batch into variables
&& seperate the individual variable/field lists with a "|" character
LOCAL lnSum, lnSum2
lcSQL = 'SELECT SUM(someCol) FROM yourTable WHERE someCondition' + CHR(13) + ;
		'SELECT SUM(someCol) FROM yourTable2 WHERE someCondition'
lnRet = SQLEXECEX(lnCon,lcSQL,'lnSum | lnSum2','',SQLEXECEX_DEST_VARIABLE)
?lnSum, lnSum2


&& the SQLEXECEX_DEST_VARIABLE flag can also be used to refresh some fields in a cursor
&& just specify the fully qualified name (cursor.fieldname)
CREATE CURSOR yourCursor (someCol C(20))
APPEND BLANK
lnRet = SQLEXECEX(lnCon,'SELECT someCol FROM yourTable WHERE yourPK = ?{lnPK}','yourCursor.someCol','',SQLEXECEX_DEST_VARIABLE)
&& this only works for a single row .. the fields of the current record are updated

&& store result into an existing cursor, you may use this to avoid the grid unbind behaviour (grid goes blank if the underlying cursor is recreated)
&& the specified cursor is ZAP'ed instead of being recreated with CREATE CURSOR
lnRet = SQLEXECEX(lnCon,'SELECT someCol FROM yourTable WHERE yourCondition','yourCursor','',SQLEXECEX_REUSE_CURSOR)

&& pass some weird SQL statement that refuses to be parsed correctly with SQLEXEC cause of an
&& embedded "?" for example
lnRet = SQLEXECEX(lnCon,'SELECT someCol FROM yourTable WHERE someCol2 = some?regular?expression','cCursor','',SQLEXECEX_NATIVE_SQL)
&& the statement is not parsed for parameters, it's passed on to SQLExecDirect (the underlying ODBC function)
&& without any modification



SQLDISCONNECT(lnCon)

FUNCTION SQLCallback(lnSet,lnRow,lnRowCount)
	IF lnSet = -1
		? lnRow
	ELSE
		IF lnRowCount <= 0
			&& if ODBC driver don't support providing the number of matching rows in SQLRowCount function
			? "Fetching resultset " + ALLTRIM(STR(lnSet)) + ", " + ALLTRIM(STR(lnRow)) + " rows fetched"		
		ELSE
			? "Fetching resultset " + ALLTRIM(STR(lnSet)) + ", " + ALLTRIM(STR(lnRow)) + " rows from " + ALLTRIM(STR(lnRowCount)) + " fetched"
		ENDIF
	ENDIF
ENDFUNC
