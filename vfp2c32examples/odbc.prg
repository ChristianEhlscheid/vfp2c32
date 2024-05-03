#INCLUDE vfp2c.h

CD (FULLPATH(JUSTPATH(SYS(16))))

IF TYPE('_WIN64') = 'L' AND _WIN64
SET LIBRARY TO vfp2c64.fll ADDITIVE
ELSE
SET LIBRARY TO vfp2c32.fll ADDITIVE
ENDIF

PUBLIC lnCon, lnSqlHandle, lnRet, lcParm, laInfo[1], laInfo2[1], laTables[16], laError[1], lcSqlCommand, xj
m.lnCon = -1
m.lnSqlHandle = -1
m.laTables[1] = 'actor'
m.laTables[2] = 'address'
m.laTables[3] = 'category'
m.laTables[4] = 'city'
m.laTables[5] = 'country'
m.laTables[6] = 'customer'
m.laTables[7] = 'film'
m.laTables[8] = 'film_actor'
m.laTables[9] = 'film_category'
m.laTables[10] = 'film_text'
m.laTables[11] = 'inventory'
m.laTables[12] = 'language'
m.laTables[13] = 'payment'
m.laTables[14] = 'rental'
m.laTables[15] = 'staff'
m.laTables[16] = 'store'

SET MULTILOCKS ON && needed for CursorAdapter.Attach
SET DELETED ON
SET DATE GERMAN
SET CENTURY ON
SET HOURS TO 24
_SCREEN.Cls()

CURSORSETPROP("MapBinary", 1, 0)
CURSORSETPROP("MapVarchar", 1, 0)

CLOSE DATABASES ALL

*!*	TRY
*!*	IF TYPE('_WIN64') = 'L' AND _WIN64
*!*		m.lnCon = SQLSTRINGCONNECT('Driver={MySQL ODBC 8.1 ANSI Driver};Server=localhost;User=root;Pwd=pwd#4#mysql;Options=67108864',.F.)
*!*	ELSE
*!*	*!*		m.lnCon = SQLSTRINGCONNECT('Driver={MySQL ODBC 5.3 ANSI Driver};Server=172.28.160.1;User=vmuser;Pwd=pwd#4#mysql;Options=67108864;SSLMODE=DISABLED',.F.)
*!*		m.lnCon = SQLSTRINGCONNECT('Driver={MySQL ODBC 8.0 ANSI Driver};Server=localhost;User=root;Pwd=pwd#4#mysql;Options=67108864',.F.)
*!*	ENDIF

*!*		IF m.lnCon = -1
*!*			THROW
*!*		ENDIF

*!*		m.lnRet = SQLEXECEX(m.lnCon,'USE sakila')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF
*!*		
	&& reusing cursors
*!*		? 1
*!*		FOR m.xj = 10 TO ALEN(m.laTables, 1)
*!*			m.lnRet = SQLEXECEX(m.lnCon, 'SELECT * FROM ' + m.laTables[m.xj], m.laTables[m.xj])
*!*			IF m.lnRet = -1
*!*				THROW 
*!*			ENDIF

*!*			m.lnRet = SQLEXECEX(m.lnCon, 'SELECT * FROM ' + m.laTables[m.xj], m.laTables[m.xj], 'laInfo', SQLEXECEX_REUSE_CURSOR)
*!*			IF m.lnRet = -1
*!*				THROW 
*!*			ENDIF

*!*			m.lnRet = SQLEXECEX(m.lnCon, 'SELECT * FROM ' + m.laTables[m.xj], m.laTables[m.xj], 'laInfo', SQLEXECEX_APPEND_CURSOR)
*!*			IF m.lnRet = -1
*!*				THROW 
*!*			ENDIF

*!*			IF !INLIST(m.laTables[m.xj], 'film_text', 'staff')
*!*				m.lnRet = SQLEXECEX(m.lnCon, 'SELECT * FROM ' + m.laTables[m.xj], m.laTables[m.xj], '', 0, '', '', '', 0, '', 0, 64)
*!*				IF m.lnRet = -1
*!*					? m.laTables[m.xj]
*!*					THROW 
*!*				ENDIF
*!*			ENDIF
*!*					
*!*		ENDFOR

*!*		&& update statement
*!*		? 2
*!*		LOCAL lnActorId, lcFirstName, lcLastName, ldLastUpdate
*!*		m.lnActorId = 20
*!*		m.lcFirstName = 'JOHNNY' + SYS(2015)
*!*		m.lnRet = SQLEXECEX(m.lnCon, 'UPDATE actor SET first_name = ?{lcFirstName} WHERE actor_id = ?{lnActorId}', '', 'laInfo') 
*!*		IF m.lnRet = -1
*!*			THROW 
*!*		ENDIF
*!*		DISPLAY MEMORY LIKE laInfo	

*!*		&& fetch to variables
*!*		? 3	
*!*		m.lcParm = 20
*!*		m.lnRet = SQLEXECEX(m.lnCon, 'SELECT * FROM actor WHERE actor_id = ?{lcParm}', 'lnActorId,lcFirstName,lcLastName,ldLastUpdate', '', SQLEXECEX_DEST_VARIABLE) 
*!*		? lnActorId, lcFirstName, lcLastName, ldLastUpdate

*!*		&& prepared statements
*!*		? 4	
*!*		m.lnSqlHandle = SQLPREPAREEX(m.lnCon, 'SELECT * FROM actor WHERE first_name LIKE ?{lcParm}', 'actor_1', 'laInfo', SQLEXECEX_APPEND_CURSOR) 
*!*		IF m.lnSqlHandle = -1
*!*			THROW 
*!*		ENDIF
*!*			
*!*		m.lcParm = 'A%'
*!*		m.lnRet = SQLEXECEX(m.lnSqlHandle)
*!*		IF lnRet = -1
*!*			THROW 
*!*		ENDIF
*!*		
*!*		m.lcParm = 'B%'
*!*		m.lnRet = SQLEXECEX(m.lnSqlHandle)
*!*		IF lnRet = -1
*!*			THROW 
*!*		ENDIF
*!*				
*!*		m.lnRet = SQLCANCELEX(m.lnSqlHandle)
*!*		IF m.lnRet = -1
*!*			THROW 
*!*		ENDIF
*!*		m.lnSqlHandle = -1
*!*			
*!*		&& custom cursorschema
*!*		? 5	
*!*		m.lcParm = 1
*!*		m.lnRet = SQLEXECEX(m.lnCon, 'SELECT * FROM actor WHERE actor_id = ?{lcParm}', 'actor_2', '', 0, 'actor_id N(6), first_name V(50), last_name V(50), last_update C(20)') 
*!*		IF m.lnRet = -1
*!*			THROW 
*!*		ENDIF

*!*		? 6
*!*		m.lnRet = SQLEXECEX(m.lnCon, 'SELECT * FROM typetest', 'v_types_sqlexecex_off') 
*!*		IF m.lnRet = -1
*!*			THROW 
*!*		ENDIF
*!*		
*!*		m.lnRet = SQLEXEC(m.lnCon, 'SELECT * FROM typetest', 'v_types_sqlexec_off') 
*!*		IF m.lnRet = -1
*!*			THROW 
*!*		ENDIF	
*!*		
*!*		? 7
*!*		m.lnRet = SQLEXECEX(m.lnCon, 'SELECT * FROM typetest', 'v_types_sqlexecex_on') 
*!*		IF m.lnRet = -1
*!*			THROW 
*!*		ENDIF
*!*		
*!*		m.lnRet = SQLEXEC(m.lnCon, 'SELECT * FROM typetest', 'v_types_sqlexec_on') 
*!*		IF m.lnRet = -1
*!*			THROW 
*!*		ENDIF	
	
*!*		? 8
*!*		CREATE CURSOR c_update (actor_id I, first_name V(50) NULL, last_name V(50) NULL, now T)
*!*		FOR m.xj = 1 TO 5
*!*			INSERT INTO c_update VALUES(xj, SYS(2015), SYS(2015), DATETIME() + xj)
*!*		ENDFOR
*!*		GO 1
*!*		REPLACE first_name WITH null 
*!*		GO 3
*!*		REPLACE first_name WITH null 
*!*		GO 5
*!*		REPLACE first_name WITH null 
*!*		GO TOP
*!*		m.lnRet = SQLEXECEX(m.lnCon, 'UPDATE sakila.actor SET first_name = ?{first_name}, last_name = ?{last_name}, last_update = ?{now} WHERE actor_id = ?{actor_id}', '', 'laInfo', SQLEXECEX_STORE_INFO + SQLEXECEX_PARAMETER_ARRAY, '', 'c_update', '', 0, 'laInfo2')
*!*		DISPLAY MEMORY LIKE laInfo		
*!*		DISPLAY MEMORY LIKE laInfo2		
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF		
*!*		
*!*		m.lnRet = SQLEXECEX(m.lnCon, 'SELECT * FROM sakila.film_category', 'c_actor', 'laInfo', SQLEXECEX_STORE_INFO, '', '', '', 0, '', 0, 64)
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF				
*!*		DISPLAY MEMORY LIKE laInfo		
*!*		DISPLAY MEMORY LIKE laInfo2		

*!*	CATCH TO loError
*!*		? m.loError.ErrorNo, m.loError.Message
*!*		IF m.loError.ErrorNo = E_USERERROR OR m.loError.ErrorNo = 2071
*!*			AERROREX('laError')
*!*		ELSE
*!*			AERROR(laError)
*!*		ENDIF
*!*		DISPLAY MEMORY LIKE laError
*!*	FINALLY
*!*		IF m.lnSqlHandle != -1
*!*			SQLCANCELEX(m.lnSqlHandle)
*!*		ENDIF
*!*		IF lnCon != -1
*!*	*!*			SQLDISCONNECT(lnCon)
*!*		ENDIF
*!*	ENDTRY

*!*	RETURN

TRY

	m.lnCon = SQLSTRINGCONNECT('Driver={ODBC Driver 18 for SQL Server};Server=DESKTOP-O83K0AE\SQLEXPRESS;UID=sa;PWD=iYV0eLDKmiQZ4RS6CRhs;Encrypt=no;',.F.)
*!*		&& m.lnCon = SQLSTRINGCONNECT('Driver={SQL Server Native Client 11.0};Server=DESKTOP-O83K0AE\SQLEXPRESS;UID=sa;PWD=iYV0eLDKmiQZ4RS6CRhs',.F.)
*!*		&& m.lnCon = SQLSTRINGCONNECT('Driver={ODBC Driver 13 for SQL Server};Server=MYULTRABOOK\SQLEXPRESS;UID=sa;PWD=pwd#4#sql',.F.)
*!*		&& m.lnCon = SQLSTRINGCONNECT('Driver={ODBC Driver 17 for SQL Server};Server=MYULTRABOOK\SQLEXPRESS;UID=sa;PWD=pwd#4#sql',.F.)
*!*		&& m.lnCon = SQLSTRINGCONNECT('Driver={ODBC Driver 18 for SQL Server};Server=MYULTRABOOK\SQLEXPRESS;UID=sa;PWD=pwd#4#sql;Encrypt=no;',.F.)
*!*		IF m.lnCon = -1
*!*			THROW
*!*		ENDIF

	m.lnRet = SQLEXECEX(m.lnCon,'USE sakila')
	IF m.lnRet = -1
		THROW
	ENDIF

*!*		? 13
*!*		m.lcParm = '2023-01-02 08:03:24.12345'
*!*		m.lnRet = SQLEXECEX(m.lnCon, 'UPDATE sakila.typetest SET c_datetime2 = ?{m.lcParm}', '', '', 0, '') && ,'1 SQL_TIMESTAMP(7)')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF
*!*		
*!*		m.lnRet = SQLEXECEX(m.lnCon, 'SELECT * FROM sakila.typetest', 'c_typetest', 'laInfo')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF

*!*		m.lnRet = SQLTablesEx(m.lnCon, 'sakila', 'sakila', '%', 'TABLE TYPE', 'c_tabletypes', 0, SQL_SS_NAME_SCOPE_TABLE_TYPE)
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF

	m.lnRet = SQLTablesEx(m.lnCon, .NULL., .NULL., .NULL., .NULL., 'c_tables', 0)
	IF m.lnRet = -1
		THROW
	ENDIF
	
	m.lnRet = SQLTablesEx(m.lnCon, .NULL., .NULL., .NULL., 'TABLE', 'c_tables2', 0)
	IF m.lnRet = -1
		THROW
	ENDIF
	
	m.lnRet = SQLTablesEx(m.lnCon, '%', "", "", .NULL., 'c_tables3', 0)
	IF m.lnRet = -1
		THROW
	ENDIF

*!*	m.lnRet = SQLColumnsEx(m.lnCon, 'sakila', 'sakila', c_tabletypes.table_name, '%', 'c_tabletypecolumns', 0, SQL_SS_NAME_SCOPE_TABLE_TYPE)
*!*	IF m.lnRet = -1
*!*		THROW
*!*	ENDIF

*!*		? 1
*!*		m.lnRet = SQLPrimaryKeysEx(m.lnCon, 'sakila', 'sakila', 'film_text', 'c_primkeys')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF
*!*		
*!*		? 2
*!*		m.lnRet = SQLForeignKeysEx(m.lnCon, 'sakila', 'sakila', 'film', 'sakila', 'sakila', 'film_actor', 'c_fkeys')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF

*!*		? 3
*!*		m.lnRet = SQLProceduresEx(m.lnCon, 'sakila', 'sakila', 'SP_OutputParameterStreaming1', 'c_procs')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF

*!*		? 4
*!*		m.lnRet = SQLProcedureColumnsEx(m.lnCon, 'sakila', 'sakila', 'SP_OutputParameterStreaming1', '*', 'c_proccols')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF
*!*		
*!*		? 5 
*!*		m.lnRet = SQLTablePrivilegesEx(m.lnCon, 'sakila', 'sakila', 'actor', 'c_tableprivs')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF

*!*		? 6
*!*		m.lnRet = SQLColumnPrivilegesEx(m.lnCon, 'sakila', 'sakila', 'actor', 'actor_id', 'c_colprivs')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF

*!*		? 7
*!*		m.lnRet = SQLSpecialColumnsEx(m.lnCon, 'sakila', 'sakila', 'actor', 1, 0, .T., 'c_speccols')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF

*!*		? 8
*!*		m.lnRet = SQLStatisticsEx(m.lnCon, 'sakila', 'sakila', 'actor', 'actor_id', 1, .T., 'c_stats')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF
*!*		
*!*		? 9
*!*		m.lnRet = SQLGetTypeInfoEx(m.lnCon, 0, 'c_typeinfo')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF	

*!*	IF .T.

*!*		? 10
*!*		m.lnRet = SQLTablesEx(m.lnCon, 'sakila', 'sakila', '%', 'TABLE TYPE', 'c_tabletypes', 0, SQL_SS_NAME_SCOPE_TABLE_TYPE)
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF

*!*		? 11
*!*		SELECT c_tabletypes
*!*		SCAN
*!*			IF c_tabletypes.table_name == "testtabletype4" OR c_tabletypes.table_name == "testtabletype14"

*!*				m.lnRet = SQLRegisterTableType(m.lnCon, c_tabletypes.table_cat, c_tabletypes.table_schem, c_tabletypes.table_name, SQLREGISTERTABLETYPE_SCHEMA)
*!*				IF m.lnRet = -1
*!*					THROW
*!*				ENDIF
*!*			
*!*			ENDIF
*!*		ENDSCAN

*!*		m.lcParm = 1
*!*		
*!*		SELECT c_tabletypes	
*!*		LOCATE FOR c_tabletypes.table_name == 'testtabletype'
*!*		? c_tabletypes.table_name, "c_tvp"
*!*		m.lnRet = SQLCreateTableTypeCursor('', c_tabletypes.table_schem, c_tabletypes.table_name, 'c_tvp')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF
*!*		APPEND BLANK IN c_tvp
*!*		REPLACE test_id WITH 1, test_char WITH 'test', test_nchar WITH 'test', test_binary WITH CREATEBINARY('a'), ;
*!*			test_varchar WITH 'test', test_nvarchar WITH 'test', test_varbinary WITH CREATEBINARY('test'), ;
*!*			test_varcharmax WITH 'test', test_nvarcharmax WITH 'test', test_varbinarymax WITH CREATEBINARY('test'), ;
*!*			test_date WITH '2023-11-07', test_datetime WITH '2023-11-07 07:00:00', test_datetime2 WITH '2023-11-07 07:00:00.1234567', ;
*!*			test_time WITH '07:00:00', test_time2 WITH '07:00:00.1234567', test_num1 WITH 23.34, test_num2 WITH 123456789.12345, ;
*!*			test_dec1 WITH 23.34, test_dec2 WITH 123456789.12345, test_text WITH 'test', test_ntext WITH 'test', ;
*!*			test_image WITH CREATEBINARY('test') IN c_tvp
*!*		m.lnRet = SQLEXECEX(m.lnCon, '{call sakila.SP_TVPTest(?{m.lcParm}, ?{"c_tvp"})}', 'c_tvpresult1', '', 0, '', '2 SQL_SS_TABLE "sakila"."testtabletype", 2 1 SQL_CA_SS_COLUMN_COMPUTED SQL_CA_SS_COLUMN_SORT_ORDER SQL_SS_ASCENDING_ORDER SQL_CA_SS_COLUMN_IN_UNIQUE_KEY SQL_CA_SS_COLUMN_SORT_ORDINAL 1')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF

*!*		SELECT c_tabletypes
*!*		LOCATE FOR c_tabletypes.table_name == 'testtabletype2'
*!*		? c_tabletypes.table_name, "c_tvp2"
*!*		m.lnRet = SQLCreateTableTypeCursor('', c_tabletypes.table_schem, c_tabletypes.table_name, 'c_tvp2')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF
*!*		APPEND BLANK IN c_tvp2
*!*		REPLACE test_char WITH 'test', test_datetime2 WITH '2023-11-07 07:00:00.1234567', ;
*!*			test_time2 WITH '07:00:00.1234567' IN c_tvp2	
*!*		m.lnRet = SQLEXECEX(m.lnCon, '{call sakila.SP_TVPTest2(?{m.lcParm}, ?{"c_tvp2"})}', 'c_tvpresult2', '', 0, '', '2 SQL_SS_TABLE "sakila"."testtabletype2"')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF

*!*		SELECT c_tabletypes
*!*		LOCATE FOR c_tabletypes.table_name == 'testtabletype3'
*!*		? c_tabletypes.table_name, "c_tvp3"
*!*		m.lnRet = SQLCreateTableTypeCursor('', c_tabletypes.table_schem, c_tabletypes.table_name, 'c_tvp3')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF
*!*		APPEND BLANK IN c_tvp3
*!*		REPLACE test_id WITH 2, test_char WITH 'test', test_nchar WITH 'test', test_binary WITH CREATEBINARY('a'), ;
*!*			test_varchar WITH 'test', test_nvarchar WITH 'test', test_varbinary WITH CREATEBINARY('test') IN c_tvp3
*!*		m.lnRet = SQLEXECEX(m.lnCon, '{call sakila.SP_TVPTest3(?{m.lcParm}, ?{"c_tvp3"})}', 'c_tvpresult3', '', 0, '', '2 SQL_SS_TABLE "sakila"."testtabletype3"')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF
*!*		
*!*		SELECT c_tabletypes
*!*		LOCATE FOR c_tabletypes.table_name == 'testtabletype4'
*!*		? c_tabletypes.table_name, "c_tvp4"
*!*		m.lnRet = SQLCreateTableTypeCursor('', c_tabletypes.table_schem, c_tabletypes.table_name, 'c_tvp4')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF
*!*		APPEND BLANK IN c_tvp4
*!*		REPLACE test_id WITH 1, test_date WITH DATE(2023,11,7), test_datetime WITH '2023-11-07 07:00:00', test_datetime2 WITH '2023-11-07 07:00:00.1234567', ;
*!*			test_time WITH '07:00:00', test_time2 WITH '07:00:00.1234567' IN c_tvp4
*!*		m.lnRet = SQLEXECEX(m.lnCon, '{call sakila.SP_TVPTest4(?{m.lcParm}, ?{"c_tvp4"})}', 'c_tvpresult4', '', 0, '', '2 SQL_SS_TABLE "sakila"."testtabletype4"')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF		
*!*		? c_tabletypes.table_name, "c_tvp4 - done"
*!*		SELECT c_tabletypes
*!*		LOCATE FOR c_tabletypes.table_name == 'testtabletype5'
*!*		? c_tabletypes.table_name, "c_tvp5"
*!*		m.lnRet = SQLCreateTableTypeCursor('', c_tabletypes.table_schem, c_tabletypes.table_name, 'c_tvp5')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF
*!*		APPEND BLANK IN c_tvp5
*!*		REPLACE test_id WITH 1, test_num1 WITH 23.34, test_num2 WITH 123456789.12345, ;
*!*			test_dec1 WITH 23.34, test_dec2 WITH 123456789.12345 IN c_tvp5	
*!*		m.lnRet = SQLEXECEX(m.lnCon, '{call sakila.SP_TVPTest5(?{m.lcParm}, ?{"c_tvp5"})}', 'c_tvpresult5', '', 0, '', '2 SQL_SS_TABLE "sakila"."testtabletype5"')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF		

*!*		SELECT c_tabletypes
*!*		LOCATE FOR c_tabletypes.table_name == 'testtabletype6'
*!*		? c_tabletypes.table_name, "c_tvp6"
*!*		m.lnRet = SQLCreateTableTypeCursor('', c_tabletypes.table_schem, c_tabletypes.table_name, 'c_tvp6')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF
*!*		APPEND BLANK IN c_tvp6
*!*		REPLACE test_id WITH 1,test_varcharmax WITH 'test', test_nvarcharmax WITH 'test', test_varbinarymax WITH CREATEBINARY('test') IN c_tvp6	
*!*		m.lnRet = SQLEXECEX(m.lnCon, '{call sakila.SP_TVPTest6(?{m.lcParm}, ?{"c_tvp6"})}', 'c_tvpresult6', '', 0, '', '2 SQL_SS_TABLE "sakila"."testtabletype6"')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF		
*!*		
*!*		SELECT c_tabletypes
*!*		LOCATE FOR c_tabletypes.table_name == 'testtabletype7'
*!*		? c_tabletypes.table_name, "c_tvp7"
*!*		m.lnRet = SQLCreateTableTypeCursor('', c_tabletypes.table_schem, c_tabletypes.table_name, 'c_tvp7')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF
*!*		APPEND BLANK IN c_tvp7
*!*		REPLACE test_id WITH 1 IN c_tvp7
*!*		m.lnRet = SQLEXECEX(m.lnCon, '{call sakila.SP_TVPTest7(?{m.lcParm}, ?{"c_tvp7"})}', 'c_tvpresult7', '', 0, '', '2 SQL_SS_TABLE "sakila"."testtabletype7"')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF		
*!*		
*!*		SELECT c_tabletypes
*!*		LOCATE FOR c_tabletypes.table_name == 'testtabletype8'
*!*		? c_tabletypes.table_name, "c_tvp8"
*!*		m.lnRet = SQLCreateTableTypeCursor('', c_tabletypes.table_schem, c_tabletypes.table_name, 'c_tvp8')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF
*!*		APPEND BLANK IN c_tvp8
*!*		REPLACE test_id WITH 1, test_char WITH 'test' IN c_tvp8
*!*		m.lnRet = SQLEXECEX(m.lnCon, '{call sakila.SP_TVPTest8(?{m.lcParm}, ?{"c_tvp8"})}', 'c_tvpresult8', '', 0, '', '2 SQL_SS_TABLE "sakila"."testtabletype8"')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF			
*!*		
*!*		SELECT c_tabletypes
*!*		LOCATE FOR c_tabletypes.table_name == 'testtabletype9'
*!*		? c_tabletypes.table_name, "c_tvp9"
*!*		m.lnRet = SQLCreateTableTypeCursor('', c_tabletypes.table_schem, c_tabletypes.table_name, 'c_tvp9')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF
*!*		APPEND BLANK IN c_tvp9
*!*		REPLACE test_id WITH 1, test_char WITH 'test', test_nchar WITH 'test' IN c_tvp9
*!*		m.lnRet = SQLEXECEX(m.lnCon, '{call sakila.SP_TVPTest9(?{m.lcParm}, ?{"c_tvp9"})}', 'c_tvpresult9', '', 0, '', '2 SQL_SS_TABLE "sakila"."testtabletype9"')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF			
*!*		
*!*		SELECT c_tabletypes
*!*		LOCATE FOR c_tabletypes.table_name == 'testtabletype10'
*!*		? c_tabletypes.table_name, "c_tvp10"
*!*		m.lnRet = SQLCreateTableTypeCursor('', c_tabletypes.table_schem, c_tabletypes.table_name, 'c_tvp10')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF
*!*		APPEND BLANK IN c_tvp10
*!*		REPLACE test_id WITH 1, test_char WITH 'test', test_nchar WITH 'test', test_binary WITH CREATEBINARY('abcdef') IN c_tvp10
*!*		m.lnRet = SQLEXECEX(m.lnCon, '{call sakila.SP_TVPTest10(?{m.lcParm}, ?{"c_tvp10"})}', 'c_tvpresult10', '', 0, '', '2 SQL_SS_TABLE "sakila"."testtabletype10"')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF
*!*		
*!*		SELECT c_tabletypes
*!*		LOCATE FOR c_tabletypes.table_name == 'testtabletype11'
*!*		? c_tabletypes.table_name, "c_tvp11"
*!*		m.lnRet = SQLCreateTableTypeCursor('', c_tabletypes.table_schem, c_tabletypes.table_name, 'c_tvp11')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF
*!*		APPEND BLANK IN c_tvp11
*!*		REPLACE test_id WITH 1, test_char WITH 'test', test_nchar WITH 'test', test_binary WITH CREATEBINARY('abcdef'), ;
*!*			test_varchar WITH '13423452345235', test_nvarchar WITH 'akjsdfjaösdjaskdjfölkajs' IN c_tvp11
*!*		m.lnRet = SQLEXECEX(m.lnCon, '{call sakila.SP_TVPTest11(?{m.lcParm}, ?{"c_tvp11"})}', 'c_tvpresult11', '', 0, '', '2 SQL_SS_TABLE "sakila"."testtabletype11"')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF

*!*		SELECT c_tabletypes	
*!*		LOCATE FOR c_tabletypes.table_name == 'testtabletype12'
*!*		? c_tabletypes.table_name, "c_tvp12"
*!*		m.lnRet = SQLCreateTableTypeCursor('', c_tabletypes.table_schem, c_tabletypes.table_name, 'c_tvp12')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF
*!*		APPEND BLANK IN c_tvp12
*!*		REPLACE test_id WITH 1, test_varcharmax WITH 'test', test_nvarcharmax WITH 'test', test_varbinarymax WITH CREATEBINARY('test') IN c_tvp12
*!*		m.lnRet = SQLEXECEX(m.lnCon, '{call sakila.SP_TVPTest12(?{m.lcParm}, ?{"c_tvp12"})}', 'c_tvpresult12', '', 0, '', '2 SQL_SS_TABLE "sakila"."testtabletype12"')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF

*!*		SELECT c_tabletypes	
*!*		LOCATE FOR c_tabletypes.table_name == 'testtabletype13'
*!*		? c_tabletypes.table_name, "c_tvp13"
*!*		m.lnRet = SQLCreateTableTypeCursor('', c_tabletypes.table_schem, c_tabletypes.table_name, 'c_tvp13')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF
*!*		FOR m.xj = 1 TO 1000
*!*			APPEND BLANK IN c_tvp13
*!*			REPLACE test_id WITH xj, test_text WITH 'test' + ALLTRIM(STR(m.xj)), test_ntext WITH 'test' + ALLTRIM(STR(m.xj)), ;
*!*			test_image WITH CREATEBINARY('test' + ALLTRIM(STR(m.xj))) IN c_tvp13
*!*		ENDFOR	
*!*		m.lnRet = SQLEXECEX(m.lnCon, '{call sakila.SP_TVPTest13(?{m.lcParm}, ?{"c_tvp13"})}', 'c_tvpresult13', '', 0, '', '2 SQL_SS_TABLE "sakila"."testtabletype13"')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF

*!*		GO TOP IN c_tvp13
*!*		GO TOP IN c_tvpresult13	
*!*		FOR m.xj = 1 TO 1000
*!*			IF !(c_tvp13.test_text == c_tvpresult13.test_text)
*!*				? "1 different", RECNO()
*!*			ENDIF
*!*			IF !(c_tvp13.test_image == c_tvpresult13.test_image)
*!*				? "2 different", RECNO()
*!*			ENDIF
*!*			IF !(c_tvp13.test_ntext == c_tvpresult13.test_ntext)
*!*				? "3 different", RECNO()
*!*			ENDIF
*!*			SKIP IN c_tvp13
*!*			SKIP IN c_tvpresult13
*!*		ENDFOR

*!*		SELECT c_tabletypes	
*!*		LOCATE FOR c_tabletypes.table_name == 'testtabletype14'
*!*		? c_tabletypes.table_name, "c_tvp14"
*!*		m.lnRet = SQLCreateTableTypeCursor('', c_tabletypes.table_schem, c_tabletypes.table_name, 'c_tvp14')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF
*!*		FOR m.xj = 1 TO 1000
*!*			APPEND BLANK IN c_tvp14
*!*			REPLACE test_id WITH xj, test_money WITH NTOM(NTOM(xj + 0.5)), test_smallmoney WITH (NTOM(xj + 0.5)) IN c_tvp14
*!*		ENDFOR	
*!*		m.lnRet = SQLEXECEX(m.lnCon, '{call sakila.SP_TVPTest14(?{m.lcParm}, ?{"c_tvp14"})}', 'c_tvpresult13', '', 0, '', '2 SQL_SS_TABLE "sakila"."testtabletype14"')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF

*!*	ENDIF
	
*!*		? 1
*!*		? SQLEXEC(m.lnCon, 'UPDATE film_text SET description = ?FILETOSTR("iptypes.prg"), description2 = ?FILETOSTR("iptypes.prg"), bin_col = ?FILETOSTR("iptypes.prg") WHERE film_id = 1')
*!*		m.lnRet = SQLEXECEX(m.lnCon, 'SELECT description, description2, bin_col FROM film_text WHERE film_id = 1', 'test')
*!*		LOCAL lnId, lcDesc, lcDesc2, lcDesc3, lcFile, laInfo[1,2]
*!*		m.lcFile = FILETOSTR("iptypes.prg")
*!*		m.lnId = 1
*!*		m.lcDesc = ''
*!*		? 2
*!*		m.lnRet = SQLEXECEX(m.lnCon, '{call sakila.SP_OutputParameterStreaming1(?{m.lnId}, ?${m.lcDesc})}', '', '', SQLEXECEX_CALLBACK_PROGRESS, '', '', 'SqlCallback')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF		
*!*		? m.lcDesc == m.lcFile
*!*		? 3
*!*		m.lcDesc2 = CREATEBINARY('')
*!*		m.lnRet = SQLEXECEX(m.lnCon, '{call sakila.SP_OutputParameterStreaming2(?{m.lnId}, ?${m.lcDesc2})}')
*!*		? 4
*!*		m.lcDesc3 = ''
*!*		m.lnRet = SQLEXECEX(m.lnCon, '{call sakila.SP_OutputParameterStreaming3(?{m.lnId}, ?${m.lcDesc3})}')
*!*		? "Desc", m.lcDesc == m.lcFile
*!*		? "Desc2", m.lcDesc2 == m.lcFile
*!*		? "Desc3", m.lcDesc3 == m.lcFile
*!*		? LEN(m.lcDesc), LEN(m.lcDesc2), LEN(m.lcDesc3), LEN(m.lcFile)

*!*		? 5
*!*		m.lcDesc = ''
*!*		m.lnRet = SQLEXECEX(m.lnCon, '{call sakila.SP_InputOutputParameterStreaming(?{m.lnId}, ?{m.lcFile}, ?${m.lcDesc})}')
*!*		? "Desc", m.lcDesc == m.lcFile

*!*		? 6
*!*		m.lcDesc = ''
*!*		m.lnRet = SQLEXECEX(m.lnCon, '{call sakila.SP_InputOutputParameterStreamingResult(?{m.lnId}, ?{m.lcFile}, ?${m.lcDesc})}', 'c_result', 'laInfo', SQLEXECEX_CALLBACK_PROGRESS, '', '', 'SqlCallback')
*!*		? "Desc", m.lcDesc == m.lcFile

*!*		? 7
*!*		m.lcDesc = ''
*!*		m.lnRet = SQLEXECEX(m.lnCon, '{call sakila.SP_InputOutputParameterStreamingResult2(?{m.lnId}, ?{m.lcFile}, ?${m.lcDesc})}')
*!*		? "Desc", m.lcDesc == m.lcFile
*!*		
*!*		
*!*		CREATE CURSOR testcursor(testfield M, testfield2 M NOCPTRANS NULL, testfield3 C(254) NULL, testfield4 W NULL, testfield5 G NULL)
*!*		APPEND BLANK
*!*		REPLACE testfield WITH m.lcFile, testfield2 WITH .NULL., testfield3 WITH '', testfield4 WITH .NULL., testfield5 WITH .NULL.
*!*		
*!*		? 8
*!*		m.lnRet = SQLEXECEX(m.lnCon, '{call sakila.SP_InputOutputParameterStreamingResult3(?{m.lnId}, ?{testcursor.testfield}, ?${testcursor.testfield2})}', '', 'laInfo', SQLEXECEX_STORE_INFO)
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF	
*!*		? testcursor.testfield == testcursor.testfield2	

*!*		REPLACE testfield WITH m.lcFile, testfield2 WITH ''
*!*		? 9
*!*		m.lnRet = SQLEXECEX(m.lnCon, '{call sakila.SP_InputOutputParameterStreamingResult3(?{m.lnId}, ?{testcursor.testfield}, ?${testcursor.testfield2})}', '', 'laInfo', SQLEXECEX_STORE_INFO, '', '3 SQL_CHAR')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF	
*!*		? testcursor.testfield == testcursor.testfield2
*!*		
*!*		? 10
*!*		m.lnRet = SQLEXECEX(m.lnCon, '{call sakila.SP_InputOutputParameterStreamingResult3(?{m.lnId}, ?{testcursor.testfield}, ?${testcursor.testfield2})}', '', 'laInfo', SQLEXECEX_STORE_INFO)
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF	
*!*		? testcursor.testfield == testcursor.testfield2
*!*		
*!*		? 11
*!*		m.lcDesc = .null.	
*!*		m.lnRet = SQLEXECEX(m.lnCon, '{call sakila.SP_InputOutputParameterStreamingResult3(?{m.lnId}, ?{m.lcFile}, ?${m.lcDesc})}', '', 'laInfo', SQLEXECEX_STORE_INFO)
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF		
*!*		? "Desc", m.lcDesc == m.lcFile
*!*		DISPLAY MEMORY LIKE laInfo
	

*!*		LOCAL lnId, ldNow	
*!*		m.lnId = 1
*!*		m.ldNow = DATETIME()	
*!*		m.lnRet = SQLEXECEX(m.lnCon, '{call sakila.NoCountTest1(?{m.lnId}, ?{m.ldNow})}', '', 'laInfo', SQLEXECEX_STORE_INFO)
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF		
*!*		DISPLAY MEMORY LIKE laInfo	
*!*		
*!*		m.lnRet = SQLEXECEX(m.lnCon, '{call sakila.NoCountTest2(?{m.lnId}, ?{m.ldNow})}', '', 'laInfo', SQLEXECEX_STORE_INFO)
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF		
*!*		DISPLAY MEMORY LIKE laInfo	
	
*!*		m.lnRet = SQLEXECEX(m.lnCon, '{call sakila.NoCountTest3(?{m.lnId}, ?{m.ldNow})}', '', 'laInfo', SQLEXECEX_STORE_INFO)
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF
*!*		DISPLAY MEMORY LIKE laInfo			
	

*!*		m.lnRet = SQLEXECEX(m.lnCon, 'SET NOCOUNT OFF;' + CHR(10) + ;
*!*			'UPDATE sakila.actor SET last_update = ?{m.ldNow} WHERE actor_id = ?{m.lnId};' + CHR(10) + ;
*!*			'UPDATE sakila.actor SET last_update = ?{m.ldNow} WHERE actor_id = ?{m.lnId+1};' + CHR(10) + ;
*!*			'UPDATE sakila.actor SET last_update = ?{m.ldNow} WHERE actor_id = ?{m.lnId+2};', '', 'laInfo', SQLEXECEX_STORE_INFO)
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF		
*!*		DISPLAY MEMORY LIKE laInfo	

*!*		CREATE CURSOR c_update (actor_id I, now T)
*!*		FOR m.xj = 1 TO 5
*!*			INSERT INTO c_update VALUES(xj, DATETIME() + xj)
*!*		ENDFOR
*!*		m.lnRet = SQLEXECEX(m.lnCon, 'UPDATE sakila.actor SET last_update = ?{now} WHERE actor_id = ?{actor_id}', '', 'laInfo', SQLEXECEX_STORE_INFO + SQLEXECEX_PARAMETER_ARRAY, '', 'c_update')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF		
*!*		DISPLAY MEMORY LIKE laInfo		
*!*		
*!*		m.lnRet = SQLEXECEX(m.lnCon, 'SELECT * FROM sakila.actor WHERE actor_id <= 5', 'c_actor', 'laInfo')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF			
	
*!*		? 12
*!*		m.lnSqlHandle = SQLPREPAREEX(m.lnCon, 'SELECT * FROM INFORMATION_SCHEMA.TABLES', 'tabs1', 'laInfo', SQLEXECEX_APPEND_CURSOR + SQLEXECEX_PRESERVE_RECNO)
*!*		IF m.lnSqlHandle = -1
*!*			THROW
*!*		ENDIF
*!*		m.lnRet = SQLEXECEX(m.lnSqlHandle)
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF
*!*		IF m.lnRet = 2
*!*			AERROREX('laError') 
*!*			? "Warnings:"
*!*			DISPLAY MEMORY LIKE laError
*!*		ENDIF
*!*		DISPLAY MEMORY LIKE laInfo
*!*		GO 2 IN tabs1
*!*		m.lnRet = SQLEXECEX(m.lnSqlHandle)
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF
*!*		IF m.lnRet = 2
*!*			AERROREX('laError') 
*!*			? "Warnings:"
*!*			DISPLAY MEMORY LIKE laError
*!*		ENDIF
*!*		DISPLAY MEMORY LIKE laInfo	
*!*		m.lnRet = SQLCANCELEX(m.lnSqlHandle)
*!*		IF m.lnRet = -1
*!*			THROW 
*!*		ENDIF
*!*		m.lnSqlHandle = -1	
	
*!*		? 12
*!*		m.lnSqlHandle = SQLPREPAREEX(m.lnCon, ;
*!*			'SELECT * FROM INFORMATION_SCHEMA.TABLES' + CHR(10) + ;
*!*			'SELECT * FROM INFORMATION_SCHEMA.COLUMNS' + CHR(10) + ;
*!*			'SELECT * FROM INFORMATION_SCHEMA.TABLES' + CHR(10) + ;
*!*			'SELECT * FROM INFORMATION_SCHEMA.COLUMNS' + CHR(10) + ;
*!*			'SELECT * FROM INFORMATION_SCHEMA.TABLES' + CHR(10) + ;
*!*			'SELECT * FROM INFORMATION_SCHEMA.COLUMNS', ;
*!*			'tabs1,cols1,tabs2,cols2,tabs3,cols3', ;
*!*			'laInfo', SQLEXECEX_APPEND_CURSOR + SQLEXECEX_PRESERVE_RECNO + SQLEXECEX_CALLBACK_PROGRESS + SQLEXECEX_CALLBACK_INFO, 'table_name V(10)', '', 'SqlCallback')
*!*		IF m.lnSqlHandle = -1
*!*			THROW
*!*		ENDIF
*!*		
*!*		m.lnRet = SQLEXECEX(m.lnSqlHandle)
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF
*!*		IF m.lnRet = 2
*!*			AERROREX('laError') 
*!*			? "Warnings:"
*!*			DISPLAY MEMORY LIKE laError
*!*		ENDIF
*!*		DISPLAY MEMORY LIKE laInfo
*!*		GO 2 IN tabs1
*!*		GO 10 IN cols1
*!*		m.lnRet = SQLEXECEX(m.lnSqlHandle)
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF
*!*		IF m.lnRet = 2
*!*			AERROREX('laError') 
*!*			? "Warnings:"
*!*			DISPLAY MEMORY LIKE laError
*!*		ENDIF
*!*		DISPLAY MEMORY LIKE laInfo	
*!*		m.lnRet = SQLCANCELEX(m.lnSqlHandle)
*!*		IF m.lnRet = -1
*!*			THROW 
*!*		ENDIF
*!*		m.lnSqlHandle = -1	
*!*		? RECNO('tabs1'), RECNO('cols2')

*!*		m.lnRet = SQLEXEC(m.lnCon, 'SELECT * FROM sakila.typetest', 'c_types')
*!*		IF m.lnRet = -1
*!*			AERROR(laError)
*!*			DISPLAY MEMORY LIKE laError
*!*		ENDIF
*!*		? 14
*!*		m.lnRet = SQLEXECEX(m.lnCon, 'SELECT * FROM sakila.typetest', 'c_types2')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF
*!*		? 14
*!*		m.lnRet = SQLEXECEX(m.lnCon, 'SELECT c_varbinarymax, c_image, c_timestamp, c_geography, c_geometry, c_hierarchyid FROM sakila.typetest', 'c_types2')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF
*!*		? 15	
*!*		m.lnRet = SQLEXEC(m.lnCon, 'SELECT * FROM sakila.namestest', 'c_names')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF
*!*		? 16	
*!*		m.lnRet = SQLEXECEX(m.lnCon, 'SELECT * FROM sakila.namestest', 'c_names2')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF
*!*		SET DELETED ON
*!*		CREATE CURSOR c_update (actor_id I, film_id I, first_name V(50) NULL, last_name V(50) NULL, now T)
*!*		FOR m.xj = 1 TO 5
*!*			INSERT INTO c_update VALUES(xj, xj, SYS(2015), SYS(2015), DATETIME() + xj)
*!*		ENDFOR
*!*		GO 1
*!*		REPLACE first_name WITH null 
*!*		GO 3
*!*		DELETE
*!*		GO 5
*!*		REPLACE first_name WITH null 
*!*		GO TOP
*!*		m.lnRet = SQLEXECEX(m.lnCon, 'UPDATE sakila.actor SET first_name = ?{first_name}, last_name = ?{last_name}, last_update = ?{now} WHERE actor_id = ?{actor_id}', '', 'laInfo', SQLEXECEX_STORE_INFO + SQLEXECEX_PARAMETER_ARRAY + SQLEXECEX_PARAMETER_ARRAY_RESULT_CURSOR, '', 'c_update', '', 0, 'c_updateresult')
*!*		DISPLAY MEMORY LIKE laInfo		
*!*		DISPLAY MEMORY LIKE laInfo2		
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF
*!*		
*!*		m.lnRet = SQLEXECEX(m.lnCon, 'SELECT * FROM sakila.actor WHERE actor_id = ?{actor_id}', 'c_actor', 'laInfo', SQLEXECEX_STORE_INFO + SQLEXECEX_PARAMETER_ARRAY, '', 'c_update', '', 0, 'laInfo2')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF				
*!*		DISPLAY MEMORY LIKE laInfo		
*!*		DISPLAY MEMORY LIKE laInfo2		

*!*	IF .F.
*!*		CREATE CURSOR c_tmp (test I)

*!*		CREATE CURSOR c_update (id_test I, c_timestamp Q(8), c_varchar50 V(50) NULL, c_nvarchar50 V(50), c_money Y NULL, c_smallmoney Y NULL, ;
*!*			c_time C(10), c_time7 C(16), c_datetime T NULL, c_datetime2 C(27) NULL)
*!*		FOR m.xj = 1 TO 50
*!*			INSERT INTO c_update (id_test, c_varchar50, c_nvarchar50, c_money, c_smallmoney, c_time, c_time7, c_datetime, c_datetime2) ;
*!*			VALUES(xj, 'test_' + SYS(2015), 'testn_' + SYS(2015), NTOM(xj + 0.5), NTOM(xj + 0.5), ;
*!*			'00:00:00', '00:00:00.1234567', DATETIME() + xj, '2023-12-01 07:00:00.1234567')
*!*		ENDFOR
	
*!*		GO 1
*!*		REPLACE c_datetime2 WITH '2023-13-13 00:00:00'
*!*		GO 2
*!*		REPLACE c_datetime2 WITH '2023-13-13 00:00:00'
*!*		GO 2
*!*		REPLACE c_datetime2 WITH '2023-13-13 00:00:00'
*!*		GO 4
*!*		REPLACE c_datetime2 WITH '2023-13-13 00:00:00'
*!*		GO 9
*!*		REPLACE c_datetime2 WITH '2023-13-13 00:00:00'
*!*		GO 10
*!*		REPLACE c_datetime2 WITH '2023-13-13 00:00:00'
*!*		FOR m.xj = 1 TO RECCOUNT('c_update')
*!*			IF MOD(xj, 300) == 0
*!*				GO xj
*!*				REPLACE c_datetime2 WITH '2023-13-13 00:00:00'
*!*			ENDIF
*!*		ENDFOR
*!*		GO 10
*!*		REPLACE c_datetime2 WITH '2023-13-13 00:00:00'
*!*		GO 1
*!*			
*!*		SELECT c_tmp
*!*		FOR m.xj = 1 TO 10000
*!*			IF MOD(m.xj, 25) == 0
*!*				GO m.xj
*!*				IF MOD(m.xj, 50) == 0
*!*					DELETE
*!*				ELSE
*!*					
*!*				ENDIF
*!*			ENDIF
*!*		ENDFOR
*!*		LOCATE
	
*!*		m.lnRet = SQLEXECEX(m.lnCon, 'INSERT INTO sakila.typetest ' + ;
*!*		'(c_varchar50, c_nvarchar50, c_money, c_smallmoney, c_time, c_time7, c_datetime, c_datetime2) VALUES ' + ;
*!*		'(?{c_varchar50}, ?{c_nvarchar50}, ?{c_money}, ?{c_smallmoney}, ?{c_time}, ?{c_time7}, ?{c_datetime}, ?{c_datetime2})', ;
*!*		'', 'laInfo', SQLEXECEX_STORE_INFO + SQLEXECEX_PARAMETER_ARRAY + SQLEXECEX_PARAMETER_ARRAY_RESULT_CURSOR, '', ;
*!*		'c_update', '', 0, 'c_updateresult')
*!*		DISPLAY MEMORY LIKE laInfo		
*!*		DISPLAY MEMORY LIKE laInfo2		
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF
*!*		? m.lnRet
*!*		SET STEP ON

*!*	TEXT TO lcSqlCommand NOSHOW TEXTMERGE PRETEXT 7
*!*		INSERT INTO sakila.typetest (c_varchar50, c_nvarchar50, c_money, c_smallmoney, c_time, c_time7, c_datetime, c_datetime2)
*!*		OUTPUT INSERTED.id_test, INSERTED.c_timestamp
*!*		VALUES (?{ALLTRIM(c_update.c_varchar50)}, ?{LEFT(c_update.c_nvarchar50,40)}, ?{c_money}, ?{c_smallmoney}, ?{c_time}, ?{c_time7}, ?{c_datetime}, ?{c_datetime2})
*!*	ENDTEXT

*!*		m.lnRet = SQLEXECEX(m.lnCon, m.lcSqlCommand, '', 'laInfo', ;
*!*		SQLEXECEX_STORE_INFO + SQLEXECEX_PARAMETER_ARRAY + SQLEXECEX_PARAMETER_ARRAY_RESUME_ON_ERROR + SQLEXECEX_PARAMETER_ARRAY_MERGE_OUTPUT, '', 'c_update 1 C(50), 2 C(40)', ;
*!*		'', 0, 'c_updateresult',32)
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF
*!*		
*!*		m.lnRet = SQLEXECEX(m.lnCon, 'SELECT * FROM sakila.typetest', 'c_testtypes', 'laInfo')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF
*!*	ENDIF

*!*		LOCAL lnFrom, lnTo
*!*		m.lnFrom = 13
*!*		m.lnTo = 17
*!*		LOCAL lnType, lnPrecision, lnScale
*!*		? 1
*!*		m.lnRet = SQLEXECEX(m.lnCon, 'SELECT id_test, c_variant FROM sakila.typetest WHERE id_test BETWEEN ?{m.lnFrom} AND ?{m.lnTo}', 'c_testtypes', 'laInfo')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF
*!*		SCANs
*!*			? c_testtypes.id_test, SQLVariantToValue(@c_testtypes.c_variant, @m.lnType, @m.lnPrecision, @m.lnScale), m.lnType, m.lnPrecision, m.lnScale
*!*		ENDSCAN	
*!*		? 2
*!*		SCAN
*!*			m.lnRet = SQLEXECEX(m.lnCon, 'UPDATE sakila.typetest SET c_variant2 = ?{c_testtypes.c_variant} WHERE id_test = ?{c_testtypes.id_test}', '', '', 0, '', '1 SQL_SS_VARIANT')
*!*			IF m.lnRet = -1
*!*				THROW
*!*			ENDIF
*!*		ENDSCAN	

*!*		? 3
*!*		m.lnRet = SQLEXECEX(m.lnCon, 'SELECT id_test, c_variant, c_variant2 FROM sakila.typetest WHERE id_test BETWEEN ?{m.lnFrom} AND ?{m.lnTo}', 'c_testtypes', 'laInfo')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF	
*!*		SCAN
*!*			? c_testtypes.id_test, SQLVariantToValue(@c_testtypes.c_variant, @m.lnType, @m.lnPrecision, @m.lnScale), m.lnType, m.lnPrecision, m.lnScale
*!*			? c_testtypes.id_test, SQLVariantToValue(@c_testtypes.c_variant2, @m.lnType, @m.lnPrecision, @m.lnScale), m.lnType, m.lnPrecision, m.lnScale
*!*		ENDSCAN	
	
*!*	INSERT INTO [sakila].[typetest] ([id_test], [c_variant]) VALUES (1,1);
*!*	INSERT INTO [sakila].[typetest] ([id_test], [c_variant]) VALUES (2,1.2);
*!*	INSERT INTO [sakila].[typetest] ([id_test], [c_variant]) VALUES (3,CAST(1 AS numeric(10,0)));
*!*	INSERT INTO [sakila].[typetest] ([id_test], [c_variant]) VALUES (4,SYSDATETIME());
*!*	INSERT INTO [sakila].[typetest] ([id_test], [c_variant]) VALUES (5,SYSDATETIMEOFFSET());
*!*	INSERT INTO [sakila].[typetest] ([id_test], [c_variant]) VALUES (6,'Test');
*!*	INSERT INTO [sakila].[typetest] ([id_test], [c_variant]) VALUES (7,CAST('Test' AS char(10)));
*!*	INSERT INTO [sakila].[typetest] ([id_test], [c_variant]) VALUES (8,CAST('Test' AS nchar(8)));
*!*	INSERT INTO [sakila].[typetest] ([id_test], [c_variant]) VALUES (9,CAST('Test' AS binary(8)));
*!*	INSERT INTO [sakila].[typetest] ([id_test], [c_variant]) VALUES (10,CAST('Test' AS varchar(10)));
*!*	INSERT INTO [sakila].[typetest] ([id_test], [c_variant]) VALUES (11,CAST('Test' AS nvarchar(10)));
*!*	INSERT INTO [sakila].[typetest] ([id_test], [c_variant]) VALUES (12,CAST(1.1234 AS money)));
*!*	INSERT INTO [sakila].[typetest] ([id_test], [c_variant]) VALUES (13,CAST(1.1234 AS smallmoney)));
*!*	INSERT INTO [sakila].[typetest] ([id_test], [c_variant]) VALUES (12,CAST(1.1234 AS money));
*!*	INSERT INTO [sakila].[typetest] ([id_test], [c_variant]) VALUES (13,CAST(1.1234 AS smallmoney));
*!*	INSERT INTO [sakila].[typetest] ([id_test], [c_variant]) VALUES (14,CAST(1.1234 AS decimal(10,4)));
*!*	INSERT INTO [sakila].[typetest] ([id_test], [c_variant]) VALUES (15,CAST(1.1 AS decimal(15,4)));
*!*	INSERT INTO [sakila].[typetest] ([id_test], [c_variant]) VALUES (17,CURRENT_TRANSACTION_ID());

*!*		m.lnRet = SQLEXECEX(m.lnCon, 'SELECT c_datetime, INTERVAL  FROM sakila.typetest WHERE id_test > 17', '', 'laInfo')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF		

*!*		m.lnRet = SQLEXECEX(m.lnCon, 'DELETE FROM sakila.typetest WHERE id_test > 17', '', 'laInfo')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF			
	
*!*		m.lnRet = SQLEXECEX(m.lnCon, ;
*!*		'SELECT * FROM sakila.actor WHERE actor_id = ?{actor_id};' + CHR(10) + ;
*!*		'SELECT * FROM sakila.film WHERE film_id = ?{film_id};' + CHR(10) + ;
*!*		'UPDATE sakila.actor SET last_update = ?{now}, first_name = ?{first_name} WHERE actor_id = ?{actor_id};', ;
*!*		'c_actor,c_film', 'laInfo', SQLEXECEX_STORE_INFO + SQLEXECEX_PARAMETER_ARRAY, '', 'c_update', '', 0, 'laInfo2')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF				
*!*		DISPLAY MEMORY LIKE laInfo		
*!*		DISPLAY MEMORY LIKE laInfo2		

*!*		m.lnRet = SQLEXECEX(m.lnCon, "UPDATE sakila.typetest SET c_numeric38_37 = '1.23456789012312345' WHERE id_test = 2", '', 'laInfo')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF				
*!*		DISPLAY MEMORY LIKE laInfo		
*!*			
*!*		m.lnRet = SQLEXECEX(m.lnCon, "UPDATE sakila.typetest SET c_money = ?{NTOM(-1.1234)} WHERE id_test = 3", '', 'laInfo')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF				
*!*		DISPLAY MEMORY LIKE laInfo		

*!*		m.lnRet = SQLEXEC(m.lnCon, 'SELECT id_test, c_money FROM sakila.typetest', 'c_money1', 'laInfo')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF				

*!*		m.lnRet = SQLEXECEX(m.lnCon, 'SELECT id_test, c_money FROM sakila.typetest', 'c_money2', 'laInfo')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF				
*!*		DISPLAY MEMORY LIKE laInfo		
	
CATCH TO loError
	IF m.loError.ErrorNo = E_USERERROR OR m.loError.ErrorNo = 2071
		AERROREX('laError')  
	ELSE
		AERROR(laError)
	ENDIF
	DISPLAY MEMORY LIKE laError
FINALLY
	IF m.lnSqlHandle != -1
		SQLCANCELEX(m.lnSqlHandle)
	ENDIF
	IF lnCon != -1
		SQLDISCONNECT(lnCon)
	ENDIF
ENDTRY

RETURN

*!*	TRY
*!*		m.lnCon = SQLSTRINGCONNECT('Driver={Oracle in OraDB21Home1};DBQ=DESKTOP-O83K0AE/xepdb1;UID=SAKILA;PWD=pwd#4#mysql;',.F.)
*!*		IF m.lnCon = -1
*!*			AERROR(laError)
*!*			DISPLAY MEMORY LIKE laError	
*!*			THROW -1
*!*		ENDIF

*!*	*!*		m.lnRet = SQLEXECEX(m.lnCon,'USE sakila')
*!*	*!*		IF m.lnRet = -1
*!*	*!*			THROW
*!*	*!*		ENDIF

*!*		CREATE CURSOR c_tmp (test I)

*!*		CREATE CURSOR c_update (id_test I, c_timestamp Q(8), c_varchar50 V(50) NULL, c_nvarchar50 V(50), c_time C(10), c_datetime T NULL)
*!*		FOR m.xj = 1 TO 10
*!*			INSERT INTO c_update (id_test, c_varchar50, c_nvarchar50, c_time, c_datetime) ;
*!*			VALUES(xj, 'test_' + SYS(2015), 'testn_' + SYS(2015), '00:00:00', DATETIME() + xj)
*!*		ENDFOR
	
*!*		GO 1
*!*		REPLACE c_datetime2 WITH '2023-13-13 00:00:00'
*!*		GO 2
*!*		REPLACE c_datetime2 WITH '2023-13-13 00:00:00'
*!*		GO 2
*!*		REPLACE c_datetime2 WITH '2023-13-13 00:00:00'
*!*		GO 4
*!*		REPLACE c_datetime2 WITH '2023-13-13 00:00:00'
*!*		GO 9
*!*		REPLACE c_datetime2 WITH '2023-13-13 00:00:00'
*!*		GO 10
*!*		REPLACE c_datetime2 WITH '2023-13-13 00:00:00'
*!*		FOR m.xj = 1 TO RECCOUNT('c_update')
*!*			IF MOD(xj, 300) == 0
*!*				GO xj
*!*				REPLACE c_datetime2 WITH '2023-13-13 00:00:00'
*!*			ENDIF
*!*		ENDFOR
*!*		GO 10
*!*		REPLACE c_datetime2 WITH '2023-13-13 00:00:00'
*!*		GO 1
*!*			
*!*		SELECT c_tmp
*!*		FOR m.xj = 1 TO 10000
*!*			IF MOD(m.xj, 25) == 0
*!*				GO m.xj
*!*				IF MOD(m.xj, 50) == 0
*!*					DELETE
*!*				ELSE
*!*					
*!*				ENDIF
*!*			ENDIF
*!*		ENDFOR
*!*		LOCATE
	
*!*		m.lnRet = SQLEXECEX(m.lnCon, 'INSERT INTO sakila.typetest ' + ;
*!*		'(c_varchar50, c_nvarchar50, c_money, c_smallmoney, c_time, c_time7, c_datetime, c_datetime2) VALUES ' + ;
*!*		'(?{c_varchar50}, ?{c_nvarchar50}, ?{c_money}, ?{c_smallmoney}, ?{c_time}, ?{c_time7}, ?{c_datetime}, ?{c_datetime2})', ;
*!*		'', 'laInfo', SQLEXECEX_STORE_INFO + SQLEXECEX_PARAMETER_ARRAY + SQLEXECEX_PARAMETER_ARRAY_RESULT_CURSOR, '', ;
*!*		'c_update', '', 0, 'c_updateresult')
*!*		DISPLAY MEMORY LIKE laInfo		
*!*		DISPLAY MEMORY LIKE laInfo2		
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF
*!*		? m.lnRet
*!*		SET STEP ON

*!*	TEXT TO lcSqlCommand NOSHOW TEXTMERGE PRETEXT 7
*!*		INSERT INTO SAKILA.typetest (c_varchar50, c_nvarchar50, c_datetime)
*!*		VALUES (?{ALLTRIM(c_update.c_varchar50)}, ?{LEFT(c_update.c_nvarchar50,40)}, ?{c_datetime})
*!*	ENDTEXT
*!*	&& 	RETURNING id_test INTO ?&{id_test}

*!*		m.lnRet = SQLEXECEX(m.lnCon, m.lcSqlCommand, '', 'laInfo', ;
*!*		SQLEXECEX_STORE_INFO + SQLEXECEX_PARAMETER_ARRAY + SQLEXECEX_PARAMETER_ARRAY_RESUME_ON_ERROR, '', 'c_update 1 C(50), 2 C(40)', ;
*!*		'', 0, 'c_updateresult')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF
*!*		
*!*		m.lnRet = SQLEXECEX(m.lnCon, 'SELECT * FROM sakila.typetest', 'c_testtypes', 'laInfo')
*!*		IF m.lnRet = -1
*!*			THROW
*!*		ENDIF

*!*	CATCH TO loError
*!*		DO CASE
*!*			CASE m.loError.ErrorNo = E_USERERROR OR m.loError.ErrorNo = 2071 AND VARTYPE(m.loError.UserValue) != 'N'
*!*				AERROREX('laError')
*!*				DISPLAY MEMORY LIKE laError
*!*			CASE VARTYPE(m.loError.UserValue) != 'N' OR m.loError.UserValue != -1
*!*				AERROR(laError)
*!*				DISPLAY MEMORY LIKE laError
*!*		ENDCASE
*!*	FINALLY
*!*		IF m.lnSqlHandle != -1
*!*			SQLCANCELEX(m.lnSqlHandle)
*!*		ENDIF
*!*		IF lnCon != -1
*!*			SQLDISCONNECT(lnCon)
*!*		ENDIF
*!*	ENDTRY

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

&& TODO - INPUT/INPUT_OUTPUT paramtypes for parameter array's

DEFINE CLASS SQL_INTERVAL_STRUCT AS Exception

	Address = 0
	SizeOf = 28
	Name = "SQL_INTERVAL_STRUCT"
	&& structure fields
	interval_type = .F.
	interval_sign = .F.
	year = .F.
	month = .F.
	day = .F.
	hour = .F.
	minute = .F.
	second = .F.
	fraction = .F.

	PROCEDURE interval_type_Access()
		RETURN ReadInt(THIS.Address)
	ENDPROC

	PROCEDURE interval_type_Assign(lnNewVal)
		WriteInt(THIS.Address, m.lnNewVal)
	ENDPROC

	PROCEDURE interval_sign_Access()
		RETURN ReadShort(THIS.Address + 4)
	ENDPROC

	PROCEDURE interval_sign_Assign(lnNewVal)
		WriteShort(THIS.Address + 4, m.lnNewVal)
	ENDPROC
	
	PROCEDURE year_Access()
		RETURN ReadUInt(THIS.Address + 8)
	ENDPROC

	PROCEDURE month_Access()
		RETURN ReadUInt(THIS.Address + 12)
	ENDPROC	
	
	PROCEDURE day_Access()
		RETURN ReadUInt(THIS.Address)
	ENDPROC

	PROCEDURE day_Assign(lnNewVal)
		WriteUInt(THIS.Address, m.lnNewVal)
	ENDPROC

	PROCEDURE hour_Access()
		RETURN ReadUInt(THIS.Address + 4)
	ENDPROC

	PROCEDURE hour_Assign(lnNewVal)
		WriteUInt(THIS.Address + 4, m.lnNewVal)
	ENDPROC

	PROCEDURE minute_Access()
		RETURN ReadUInt(THIS.Address + 8)
	ENDPROC

	PROCEDURE minute_Assign(lnNewVal)
		WriteUInt(THIS.Address + 8, m.lnNewVal)
	ENDPROC

	PROCEDURE second_Access()
		RETURN ReadUInt(THIS.Address + 12)
	ENDPROC

	PROCEDURE second_Assign(lnNewVal)
		WriteUInt(THIS.Address + 12, m.lnNewVal)
	ENDPROC

	PROCEDURE fraction_Access()
		RETURN ReadUInt(THIS.Address + 16)
	ENDPROC

	PROCEDURE fraction_Assign(lnNewVal)
		WriteUInt(THIS.Address + 16, m.lnNewVal)
	ENDPROC	
	
ENDDEFINE
