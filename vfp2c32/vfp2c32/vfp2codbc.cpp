#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <sql.h>

#if !defined(_WIN64)
#include "pro_ext.h"
#else
#include "pro_ext64.h"
#endif
#include "vfp2c32.h"
#include "vfp2cutil.h"
#include "vfp2codbc.h"
#include "vfp2ccppapi.h"

static SQLHENV hODBCEnvHandle = 0;

void _stdcall SaveODBCError(char *pFunction, SQLHANDLE hHandle, SQLSMALLINT nHandleType)
{
	VFP2CTls& tls = VFP2CTls::Tls();
	LPVFP2CERROR pError = tls.ErrorInfo;

	SQLRETURN nApiRet = SQL_SUCCESS;
	SQLSMALLINT nErrorLength, nError = 1;
	int nErrorNum = 0;

	do 
	{
		nApiRet = SQLGetDiagRec(nHandleType,hHandle, nError++,
			(SQLCHAR*)pError->aSqlState,
			(SQLINTEGER*)&pError->nErrorNo,
			(SQLCHAR*)pError->aErrorMessage,
			VFP2C_ERROR_MESSAGE_LEN,
			&nErrorLength);

		if(nApiRet == SQL_SUCCESS)
		{
			pError->nErrorType = VFP2C_ERRORTYPE_ODBC;
			strncpyex(pError->aErrorFunction, pFunction, VFP2C_ERROR_FUNCTION_LEN);
			nErrorNum++;
			pError++;
		}
	} while (nApiRet == SQL_SUCCESS);

	tls.ErrorCount = nErrorNum - 1;
}

void _stdcall SaveODBCInstallerError(char *pFunction)
{
	VFP2CTls& tls = VFP2CTls::Tls();
	LPVFP2CERROR pError = tls.ErrorInfo;
	WORD nErrorLength, nError = 1;
	tls.ErrorCount = 0;

	while (nError <= 8 && 
		SQLInstallerError(nError++, &pError->nErrorNo, pError->aErrorMessage, VFP2C_ERROR_MESSAGE_LEN, &nErrorLength) == SQL_SUCCESS)
	{
		pError->nErrorType = VFP2C_ERRORTYPE_WIN32; // we don't need a SQLState field here 
		strncpyex(pError->aErrorFunction, pFunction, VFP2C_ERROR_FUNCTION_LEN);
		tls.ErrorCount++;
		pError++;
	}
}

int _stdcall VFP2C_Init_Odbc()
{
	if (hODBCEnvHandle)
		return 0;

	int nErrorNo;
	ValueEx vODBCHandle;

	if (nErrorNo = _Evaluate(vODBCHandle, "VAL(SYS(3053))"))
	{
		SaveCustomErrorEx("SYS(3053)", "Cannot retrieve ODBC environment handle.", nErrorNo);
		return nErrorNo;
	}

	hODBCEnvHandle = vODBCHandle.DynamicPtr<SQLHENV>();
	return 0;
}

void _fastcall CreateSQLDataSource(ParamBlkEx& parm)
{
try
{
	FoxString pDsn(parm(1),2);
	FoxString pDriver(parm(2));

	BOOL bRetVal;
	UINT nDSNType = parm(3)->ev_long & ODBC_SYSTEM_DSN ? ODBC_ADD_SYS_DSN : ODBC_ADD_DSN;

	if (!(bRetVal = SQLConfigDataSource(0,nDSNType,pDriver,pDsn)))
		SaveODBCInstallerError("SQLConfigDataSource");

	Return(bRetVal == TRUE);
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall DeleteSQLDataSource(ParamBlkEx& parm)
{
try
{
	FoxString pDsn(parm(1));
	FoxString pDriver(parm(2),2);

	BOOL bRetVal;
	UINT nDSNType = parm(3)->ev_long == ODBC_SYSTEM_DSN ? ODBC_REMOVE_SYS_DSN : ODBC_REMOVE_DSN;
	char aDSNName[SQL_MAX_DSN_LENGTH_EX] = "DSN=";

	if (pDsn.Len() > SQL_MAX_DSN_LENGTH)
		throw E_INVALIDPARAMS;
	
	strcat(aDSNName,pDsn);

	if (!(bRetVal = SQLConfigDataSource(0,nDSNType,pDriver,aDSNName)))
		SaveODBCInstallerError("SQLConfigDataSource");

	Return(bRetVal == TRUE);
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall ChangeSQLDataSource(ParamBlkEx& parm)
{
try
{
	FoxString pDsn(parm(1),2);
	FoxString pDriver(parm(2),2);

	BOOL bRetVal;
	UINT nDSNType = parm(3)->ev_long == ODBC_SYSTEM_DSN ? ODBC_CONFIG_SYS_DSN : ODBC_CONFIG_DSN;

	if (!(bRetVal = SQLConfigDataSource(0,nDSNType,pDriver,pDsn)))
		SaveODBCInstallerError("SQLConfigDataSource");

	Return(bRetVal == TRUE);
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall ASQLDataSources(ParamBlkEx& parm)
{
try
{
	int nErrorNo = VFP2C_Init_Odbc();
	if (nErrorNo)
		throw nErrorNo;

	FoxArray pArray(parm(1),1,2);
	FoxString pDatasource(SQL_MAX_DSN_LENGTH+1);
	FoxString pDescription(ODBC_MAX_DESCRIPTION_LEN);
	SQLRETURN nApiRet;
	SQLSMALLINT nDsnLen, nDescLen;
	SQLUSMALLINT nFilter = SQL_FETCH_FIRST;
	unsigned int nRow;

	if (parm.PCount() == 2)
	{
		if (parm(2)->ev_long == ODBC_USER_DSN)
			nFilter = SQL_FETCH_FIRST_USER;
		else if (parm(2)->ev_long == ODBC_SYSTEM_DSN)
			nFilter = SQL_FETCH_FIRST_SYSTEM;
		else
			throw E_INVALIDPARAMS;
	}
	
	nApiRet = SQLDataSources(hODBCEnvHandle,nFilter,pDatasource,SQL_MAX_DSN_LENGTH+1,&nDsnLen,
				pDescription,ODBC_MAX_DESCRIPTION_LEN,&nDescLen);

	pArray.AutoGrow(8);
	while (nApiRet == SQL_SUCCESS)
	{
		nRow = pArray.Grow();
		pArray(nRow,1) = pDatasource.Len(nDsnLen);
		pArray(nRow,2) = pDescription.Len(nDescLen);
		
		nApiRet = SQLDataSources(hODBCEnvHandle,SQL_FETCH_NEXT,pDatasource,SQL_MAX_DSN_LENGTH+1,&nDsnLen,
				pDescription,ODBC_MAX_DESCRIPTION_LEN,&nDescLen);
	}

	if (nApiRet != SQL_NO_DATA)
	{
		SaveODBCError("SQLDataSources", hODBCEnvHandle, SQL_HANDLE_ENV);
		throw E_APIERROR;
	}

	pArray.ReturnRows();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall ASQLDrivers(ParamBlkEx& parm)
{
try
{
	int nErrorNo = VFP2C_Init_Odbc();
	if (nErrorNo)
		throw nErrorNo;

	FoxArray pArray(parm(1),1,2);
	FoxString pDriver(ODBC_MAX_DRIVER_LEN);
	FoxString pAttributes(ODBC_MAX_ATTRIBUTES_LEN);
	SQLRETURN nApiRet;
	SQLSMALLINT nDriverLen, nAttribLen;
	unsigned int nRow;
	
	nApiRet = SQLDrivers(hODBCEnvHandle,SQL_FETCH_FIRST,pDriver,ODBC_MAX_DRIVER_LEN,&nDriverLen,
						pAttributes,ODBC_MAX_ATTRIBUTES_LEN,&nAttribLen);

	pArray.AutoGrow(32);
	while (nApiRet == SQL_SUCCESS)
	{
		nRow = pArray.Grow();
		pArray(nRow,1) = pDriver.Len(nDriverLen);
		pArray(nRow,2) = pAttributes.Len(--nAttribLen);
		nApiRet = SQLDrivers(hODBCEnvHandle,SQL_FETCH_NEXT,pDriver,ODBC_MAX_DRIVER_LEN,&nDriverLen,
						pAttributes,ODBC_MAX_ATTRIBUTES_LEN,&nAttribLen);
	}

	if (nApiRet != SQL_NO_DATA)
	{
		SaveODBCError("SQLDrivers", hODBCEnvHandle, SQL_HANDLE_ENV);
		throw E_APIERROR;
	}

	pArray.ReturnRows();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall SQLGetPropEx(ParamBlkEx& parm)
{
try
{
	FoxString pCursor(parm,1);
	FoxString pConAttribute(parm(2));
	LocatorEx& pRef = parm(3);
	FoxString pBuffer;
	ValueEx vConHandle;
	CStrBuilder<VFP2C_MAX_FUNCTIONBUFFER> pExeBuffer;
	int nRetVal = 1;
	bool bEvalHandle = true;
	SQLHDBC hConHandle = 0;
	SQLRETURN nApiRet;

	if (parm(1)->Vartype() == 'I' || parm(1)->Vartype() == 'N')
	{
		UINT_PTR nHandle = parm(1)->DynamicPtr<UINT_PTR>();
		if (nHandle)
			pExeBuffer.Format("SQLGETPROP(%P,'ODBChdbc')", nHandle);
		else
			bEvalHandle = false;
	}
	else if (parm(1)->Vartype() == 'C')
	{
		CStringView pCursorView = pCursor;
		pExeBuffer.Format("SQLGETPROP(CURSORGETPROP('ConnectHandle','%V'),'ODBChdbc')", &pCursorView);
	}
	else
		throw E_INVALIDPARAMS;

	if (bEvalHandle)
	{
		Evaluate(vConHandle, pExeBuffer);
		hConHandle = vConHandle.DynamicPtr<SQLHDBC>();
	}

	if (pConAttribute.ICompare("trace"))
	{
		BOOL bTrace;
		nApiRet = SQLGetConnectAttr(hConHandle, SQL_ATTR_TRACE, &bTrace,0,0);
		if (nApiRet != SQL_ERROR)
			pRef = bTrace > 0;
	}
	else if (pConAttribute.ICompare("tracefile"))
	{
		SQLINTEGER nLen;
		pBuffer.Size(MAX_PATH);
		nApiRet = SQLGetConnectAttr(hConHandle,SQL_ATTR_TRACEFILE,pBuffer.Ptr<SQLPOINTER>(),MAX_PATH,&nLen);
		if (nApiRet != SQL_ERROR)
			pRef = pBuffer.Len(nLen);
	}
	else if (pConAttribute.ICompare("connected"))
	{
		BOOL bConnected;
		nApiRet = SQLGetConnectAttr(hConHandle,SQL_ATTR_CONNECTION_DEAD,&bConnected,0,0);
		if (nApiRet != SQL_ERROR)
			pRef = (bConnected == 0); // negate result, since we want to return if we're connected, but we query if the connection is dead ..
	}
	else if (pConAttribute.ICompare("isolationlevel"))
	{
		int nIsolation;
		nApiRet = SQLGetConnectAttr(hConHandle,SQL_ATTR_TXN_ISOLATION,&nIsolation,0,0);
		if (nApiRet != SQL_ERROR)				
			pRef = nIsolation;
	}
	else if (pConAttribute.ICompare("perfdata"))
	{
		if (!hConHandle)
			throw E_INVALIDPARAMS;

		DWORD nPerfData;
		nApiRet = SQLGetConnectAttr(hConHandle,SQL_COPT_SS_PERF_DATA,&nPerfData,0,0);
		if (nApiRet != SQL_ERROR)
			pRef = nPerfData;
	}
	else
		throw E_INVALIDPARAMS;

	if (nApiRet == SQL_ERROR)
	{
		SafeODBCDbcError("SQLGetConnectAttr", hConHandle);
		throw E_APIERROR;
	}
	else if (nApiRet == SQL_SUCCESS_WITH_INFO)
	{
		SafeODBCDbcError("SQLGetConnectAttr", hConHandle);
		nRetVal = 2;
	}

	Return(nRetVal);
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall SQLSetPropEx(ParamBlkEx& parm)
{
try
{
	FoxString pCursor(parm,1);
	FoxString pConAttribute(parm(2));
	FoxString pProperty(parm,3);
	ValueEx vConHandle;
	CStrBuilder<VFP2C_MAX_FUNCTIONBUFFER> pExeBuffer;
	int nRetVal = 1;
	bool bEvalHandle = true;
	SQLHDBC hConHandle = 0;
	SQLRETURN nApiRet;

	if (parm(1)->Vartype() == 'I' || parm(1)->Vartype() == 'N')
	{
		DWORD nHandle = parm(1)->Vartype() == 'I' ? parm(1)->ev_long : static_cast<DWORD>(parm(1)->ev_real);
		if (nHandle)
			pExeBuffer.Format("INT(SQLGETPROP(%U,'ODBChdbc'))", parm(1)->ev_long);
		else
			bEvalHandle = false;
	}
	else if (parm(1)->Vartype() == 'C')
	{
		CStringView pCursorView = pCursor;
		pExeBuffer.Format("INT(SQLGETPROP(CURSORGETPROP('ConnectHandle','%V'),'ODBChdbc'))", &pCursorView);
	}
	else
		throw E_INVALIDPARAMS;

	if (bEvalHandle)
	{
		Evaluate(vConHandle, pExeBuffer);
		hConHandle = vConHandle.DynamicPtr<SQLHDBC>();
	}

	if (pConAttribute.ICompare("trace"))
	{
		if (parm(3)->Vartype() != 'L')
			throw E_INVALIDPARAMS;
		SQLPOINTER pParam = parm(3)->ev_length ? (SQLPOINTER)1 : 0;
		nApiRet = SQLSetConnectAttr(hConHandle,SQL_ATTR_TRACE, pParam, SQL_IS_UINTEGER);
	}
	else if (pConAttribute.ICompare("tracefile"))
	{
		if (parm(3)->Vartype() != 'C')
			throw E_INVALIDPARAMS;
		
		nApiRet = SQLSetConnectAttr(hConHandle,SQL_ATTR_TRACEFILE,pProperty.Ptr<SQLPOINTER>(),pProperty.Len());
	}
	else if (pConAttribute.ICompare("perfdata"))
	{
		if (parm(3)->Vartype() != 'L')
			throw E_INVALIDPARAMS;
		
		SQLPOINTER pParam = parm(3)->ev_length ? (SQLPOINTER)SQL_PERF_START : (SQLPOINTER)SQL_PERF_STOP;
		nApiRet = SQLSetConnectAttr(hConHandle,SQL_COPT_SS_PERF_DATA, pParam, SQL_IS_INTEGER);
	}
	else if (pConAttribute.ICompare("perfdatafile"))
	{
		if (parm(3)->Vartype() != 'C')
			throw E_INVALIDPARAMS;
		
		nApiRet = SQLSetConnectAttr(hConHandle,SQL_COPT_SS_PERF_DATA_LOG,pProperty.Ptr<SQLPOINTER>(),SQL_NTS);
	}
	else if (pConAttribute.ICompare("perfdatalog"))
	{
		nApiRet = SQLSetConnectAttr(hConHandle, SQL_COPT_SS_PERF_DATA_LOG_NOW, NULL, 0);
	}
	else
		throw E_INVALIDPARAMS;

	if (nApiRet == SQL_ERROR)
	{
		SafeODBCDbcError("SQLSetConnectAttr", hConHandle);
		throw E_APIERROR;
	}
	else if (nApiRet == SQL_SUCCESS_WITH_INFO)
	{
		SafeODBCDbcError("SQLSetConnectAttr", hConHandle);
		nRetVal = 2;
	}

	Return(nRetVal);
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall SQLExecEx(ParamBlkEx& parm)
{
	// 1. setup
	// 2. parse & rewrite SQL for parameter markers (replace ?{varName} with ?)
	// 3. SQLBindParamter() - Bind input/output parameters
	// 4. SQLExecDirect() - Execute SQL statement
	// 4.1 if SQLExecDirect returns SQL_NEED_DATA we need to send long parameter data with SQLParamData/SQLPutData
	// 5. SQLNumResultCols() - find out number of columns in result, if no resultset was generated goto step 9.3
	// 6. SQLGetMetaData() - get meta data for columns
	// 6.1 SQLParseCursorSchema - if a custom cursorschema is passed, parse the schema
	// 7. SQLPrepareColumnBindings - based on the meta data and the cursorschema desired create Value buffers for the columns
	// 8. SQLBindColumnEx() - bind all columns that are fixed width to the Value buffers
	// 9. SQLFetchToCursor/SQLFetchToVariables - loop over SQLFetch() until no more data is returned
	// 10 store number of fetched / updated / deleted / inserted rows into array
	// 11. SQLMoreResults - check if more result sets are available - if so move to step 5
	// 12. cleanup

	SqlStatement* pStmt = 0;
	bool prepared = parm.PCount() == 1;
	BOOL bAbort = FALSE;

	try
	{
		SqlResultSet* pResultSet;
		SQLCHAR* pSqlCommand;
		SQLINTEGER nSQLLen;
		SQLRETURN nApiRet;
		ValueEx vRowCount;
		vRowCount.SetDouble(0, 0);
		int nErrorNo = VFP2C_Init_Odbc();
		if (nErrorNo)
			throw nErrorNo;

		if (prepared)
		{
			if (parm(1)->Vartype() == 'I' || parm(1)->Vartype() == 'N')
				pStmt = parm(1)->DynamicPtr<SqlStatement*>();
			else
			{
				SaveCustomError("SqlExecEx", "Invalid parameter nHandle, should be of type I or N");
				throw E_INVALIDPARAMS;
			}
			pStmt->nResultset = 0;

			// if statement contained parameters 
			if (pStmt->nNoOfParms)
			{
				nApiRet = SQLFreeStmt(pStmt->hStmt, SQL_RESET_PARAMS);
				if (nApiRet == SQL_ERROR)
				{
					SafeODBCStmtError("SQLFreeStmt - SQL_RESET_PARAMS", pStmt->hStmt);
					throw E_APIERROR;
				}

				// 2.5 - evaluate parameters
				pStmt->EvaluateParams();

				// 3 - bind parameters in ODBC driver
				pStmt->BindParameters();
			}

			if (pStmt->bExecutedOnce && pStmt->pInfoArray)
			{
				pStmt->pInfoArray.Dimension(1, 2);
				pStmt->pInfoArray.Reset();
			}
		}
		else
		{
			// 1
			pStmt = SQLAllocStatement(parm, false);

			// 2 - check statement for parameters
			if (!(pStmt->nFlags & SQLEXECEX_NATIVE_SQL))
				pStmt->NumParamsEx(pStmt->pSQLInput);

			// if statement contained parameters parse them out
			if (pStmt->nNoOfParms)
			{
				pStmt->pParamData = new SqlParameter[pStmt->nNoOfParms];
				if (!pStmt->pParamData)
					throw E_INSUFMEMORY;

				// 2.3
				pStmt->ExtractParamsAndRewriteStatement();
				pSqlCommand = pStmt->pSQLSend.Ptr<SQLCHAR*>();
				nSQLLen = pStmt->pSQLSend.Len();

				// 2.4 - parse parameterschema
				pStmt->ParseParamSchema();

				// 2.5 - evaluate parameters
				pStmt->EvaluateParams();

				// 3 - bind parameters in ODBC driver
				pStmt->BindParameters();
			}
			else // no parameters in SQL statement .. just send it as it is ..
			{
				pSqlCommand = pStmt->pSQLInput.Ptr<SQLCHAR*>();
				nSQLLen = pStmt->pSQLInput.Len();
			}
		}

		// 4.
		if (prepared)
			nApiRet = SQLExecute(pStmt->hStmt);
		else
			nApiRet = SQLExecDirect(pStmt->hStmt, pSqlCommand, nSQLLen);

		// 4.1
		if (nApiRet == SQL_ERROR)
		{
			SafeODBCStmtError(prepared ? "SQLExecute" : "SQLExecDirect", pStmt->hStmt);
			throw E_APIERROR;
		}
		else if (nApiRet == SQL_NEED_DATA)
		{
			pStmt->PutData();
		}
		else if (nApiRet == SQL_SUCCESS_WITH_INFO)
			pStmt->InfoCallbackOrStore();

		// 5.
	SQLResultSetProcessing:

		SQLSMALLINT nNoOfCols;
		// get no of columns in resultset (if 0 the SQL statement didn't produce a resultset)
		if ((nApiRet = SQLNumResultCols(pStmt->hStmt, &nNoOfCols)) == SQL_ERROR)
		{
			SafeODBCStmtError("SQLNumResultCols", pStmt->hStmt);
			throw E_APIERROR;
		}

		if (nNoOfCols)
		{
			pResultSet = pStmt->AddResultSet();
			pResultSet->AllocateColumns(nNoOfCols);

			// read total rows found (if not supported it'll be -1/0 depending on driver in use)
			if (pStmt->nFlags & SQLEXECEX_CALLBACK_PROGRESS)
				SQLRowCount(pStmt->hStmt, &pStmt->nRowsTotal);
		}
		else
		{
			// no resultset ..(UPDATE, DELETE, INSERT or other statement ...)
			if (pStmt->pInfoArray)
			{
				SQLLEN nRowCount;
				nApiRet = SQLRowCount(pStmt->hStmt, &nRowCount);
				if (nApiRet == SQL_ERROR)
				{
					SafeODBCStmtError("SQLRowCount", pStmt->hStmt);
					throw E_APIERROR;
				}

				int nIndex = pStmt->pInfoArray.Grow();
				pStmt->pInfoArray(nIndex, 1) = pStmt->pGetDataBuffer.Len(0);

				vRowCount.ev_real = (double)nRowCount;
				pStmt->pInfoArray(nIndex, 2) = vRowCount;
			}
			goto SQLResultSetChecking;
		}

		// 6
		pResultSet->GetMetaData();

		if (pStmt->nFlags & SQLEXECEX_DEST_CURSOR)
		{
			if (!pStmt->bExecutedOnce)
			{
				/* if a cursorname is not passed for the resultset generate a default cursorname */
				if (pStmt->pCursorNames.Len() == 0)
				{
					CStrBuilder<16> pCursor;
					if (pStmt->nResultset == 1)
						pCursor = "sqlresult";
					else
						pCursor.Format("sqlresult%I", pStmt->nResultset);

					pResultSet->pCursorName = pCursor;
				}
				else
					pResultSet->pCursorName = pStmt->pCursorNames.GetWordNum(pStmt->nResultset, ',').Alltrim();
			}

			// 6.1
			if (pStmt->nFlags & SQLEXECEX_REUSE_CURSOR)
			{
				pResultSet->nWorkArea = Select(pResultSet->pCursorName);
				if (pResultSet->nWorkArea)
					pResultSet->ParseCursorSchemaEx();
				else
					pResultSet->ParseCursorSchema();
			}
			else
				pResultSet->ParseCursorSchema();
				
			// 7
			pResultSet->PrepareColumnBindings();

			// 8
			pResultSet->BindColumns();

			int nRecNo = 0;
			if (pResultSet->nWorkArea && (pStmt->nFlags & SQLEXECEX_REUSE_CURSOR))
			{
				if ((pStmt->nFlags & SQLEXECEX_APPEND_CURSOR) == 0)
				{
					AutoOnOffSetting pSetting("SAFETY", false);
					if (nErrorNo = Zap(pResultSet->pCursorName))
						throw nErrorNo;
				}
				if ((pStmt->nFlags & SQLEXECEX_PRESERVE_RECNO) && !Bof(pResultSet->nWorkArea) && !Eof(pResultSet->nWorkArea))
					nRecNo = RecNo(pResultSet->nWorkArea);
			}
			else
			{
				pResultSet->CreateCursor();
			}

			pResultSet->BindFieldLocators();

			// 9.
			pResultSet->FetchToCursor(&bAbort);

			if (nRecNo && (pStmt->nFlags & SQLEXECEX_PRESERVE_RECNO))
				Go(nRecNo, pResultSet->nWorkArea);
			else
				GoTop(pResultSet->nWorkArea);

			if (pStmt->pInfoArray)
			{
				int nIndex = pStmt->pInfoArray.Grow();
				pStmt->pInfoArray(nIndex, 1) = pResultSet->pCursorName;

				vRowCount.ev_real = (double)pStmt->nRowsFetched;
				pStmt->pInfoArray(nIndex, 2) = vRowCount;
			}
		}
		else if (pStmt->nFlags & SQLEXECEX_DEST_VARIABLE)
		{
			pResultSet->BindVariableLocators();
			pResultSet->BindVariableLocatorsEx();

			// 7
			pResultSet->PrepareColumnBindings();

			// 8
			pResultSet->BindColumns();

			// 9.
			pResultSet->FetchToVariables();

			if (pStmt->pInfoArray)
			{
				int nIndex = pStmt->pInfoArray.Grow();
				pStmt->pInfoArray(nIndex, 1) = pStmt->pGetDataBuffer.Len(0);

				vRowCount.ev_real = (double)pStmt->nRowsFetched;
				pStmt->pInfoArray(nIndex, 2) = vRowCount;
			}
		}

		// callback if there is more info (from TSQL PRINT or RAISERROR statements)
		pStmt->InfoCallbackOrStore();

		// unbind column buffers from the statement
		nApiRet = SQLFreeStmt(pStmt->hStmt, SQL_UNBIND);
		if (nApiRet == SQL_ERROR)
		{
			SafeODBCStmtError("SQLFreeStmt - SQL_UNBIND", pStmt->hStmt);
			throw E_APIERROR;
		}
		
		// 10.
	SQLResultSetChecking:

		nApiRet = SQLMoreResults(pStmt->hStmt);
		if (nApiRet == SQL_ERROR)
		{
			SafeODBCStmtError("SQLMoreResult", pStmt->hStmt);
			throw E_APIERROR;
		}
		else if (nApiRet == SQL_SUCCESS_WITH_INFO)
		{
			pStmt->InfoCallbackOrStore();
		}

		if (nApiRet != SQL_NO_DATA)
			goto SQLResultSetProcessing;

		// save output parameters
		pStmt->SaveOutputParameters();

		// we're finished .. clean up everything ...
		Return(pStmt->nResultset);
		if (prepared)
		{
			pStmt->FreeParameters();
			pStmt->bExecutedOnce = true;
		}
		else
		{
			delete pStmt;
		}
	}
	catch (int nErrorNo)
	{
		if (!prepared)
			delete pStmt;
		if (nErrorNo == E_APIERROR)
			nErrorNo = 0;

		if (nErrorNo)
			RaiseError(nErrorNo);
		else if (!bAbort)
			Return(-1);
		else
			Return(-2);
	}
}

void _fastcall SQLPrepareEx(ParamBlkEx& parm)
{
	SQLRETURN nApiRet;
	SqlStatement* pStmt = 0;
	SQLCHAR* pSqlCommand;
	SQLINTEGER nSqlLen;

	try
	{
		int nErrorNo = VFP2C_Init_Odbc();
		if (nErrorNo)
			throw nErrorNo;

		// 1
		pStmt = SQLAllocStatement(parm, true);

		// 2 - check statement for parameters
		if (!(pStmt->nFlags & SQLEXECEX_NATIVE_SQL))
			pStmt->NumParamsEx(pStmt->pSQLInput);

		// if statement contained parameters parse them out
		if (pStmt->nNoOfParms)
		{
			pStmt->pParamData = new SqlParameter[pStmt->nNoOfParms];
			if (!pStmt->pParamData)
				throw E_INSUFMEMORY;

			pStmt->ExtractParamsAndRewriteStatement();
			pSqlCommand = pStmt->pSQLSend.Ptr<SQLCHAR*>();
			nSqlLen = pStmt->pSQLSend.Len();

			// 2.4 - parse parameterschema
			pStmt->ParseParamSchema();
		}
		else // no parameters in SQL statement .. just send it as it is ..
		{
			pSqlCommand = pStmt->pSQLInput.Ptr<SQLCHAR*>();
			nSqlLen = pStmt->pSQLInput.Len();
		}

		// 4.
		nApiRet = SQLPrepare(pStmt->hStmt, pSqlCommand, nSqlLen);
		if (nApiRet == SQL_ERROR)
		{
			SafeODBCStmtError("SQLPrepare", pStmt->hStmt);
			throw E_APIERROR;
		}

		// we're finished .. clean up everything ...
		// SQLReleaseStatement(parm, pStmt, true);
		Return(pStmt);
	}
	catch (int nErrorNo)
	{
		if (pStmt)
			delete pStmt;
		if (nErrorNo == E_APIERROR)
			nErrorNo = 0;

		if (nErrorNo)
			RaiseError(nErrorNo);
		else
			Return(-1);
	}
}

void _fastcall SQLCancelEx(ParamBlkEx& parm)
{
	SqlStatement* pStmt = parm(1)->Ptr<SqlStatement*>();
	int retval = 1;
	try
	{
		if (pStmt)
		{
			delete pStmt;
		}
		Return(1);
	}
	catch (int nError)
	{
		if (nError != E_APIERROR)
			RaiseError(nError);
		else
			Return(-1);
	}
}

#pragma warning(disable : 4290)
SqlStatement* _fastcall SQLAllocStatement(ParamBlkEx& parm, bool prepared) throw(int)
{
	SqlStatement* pStmt = 0;
try
{
	ValueEx vConHandle;
	ValueEx vMapVarchar;
	CStrBuilder<VFP2C_MAX_FUNCTIONBUFFER> pBuffer;
	vConHandle = 0;
	vMapVarchar = 0;

	pStmt = new SqlStatement();
	if (!pStmt)
		throw E_INSUFMEMORY;

	pStmt->bPrepared = prepared;

	if (parm.PCount() >= 5 && parm(5)->ev_long)
	{
		pStmt->nFlags = parm(5)->ev_long;
		if ((pStmt->nFlags & SQLEXECEX_APPEND_CURSOR))
			pStmt->nFlags |= SQLEXECEX_REUSE_CURSOR;
		if ((pStmt->nFlags & SQLEXECEX_REUSE_CURSOR))
			pStmt->nFlags |= SQLEXECEX_DEST_CURSOR;
	}
	else
		pStmt->nFlags = SQLEXECEX_DEST_CURSOR | SQLEXECEX_CALLBACK_PROGRESS | SQLEXECEX_CALLBACK_INFO;

	if ((pStmt->nFlags & SQLEXECEX_DEST_VARIABLE) && (pStmt->nFlags & (SQLEXECEX_DEST_CURSOR | SQLEXECEX_REUSE_CURSOR | SQLEXECEX_APPEND_CURSOR | SQLEXECEX_PRESERVE_RECNO)))
	{
		SaveCustomError("SqlExecEx", "Invalid nFlags parameter, SQLEXECEX_DEST_VARIABLE in combination with SQLEXECEX_DEST_CURSOR, SQLEXECEX_REUSE_CURSOR, SQLEXECEX_APPEND_CURSOR or SQLEXECEX_PRESERVE_RECNO.");
		throw E_INVALIDPARAMS;
	}

	if ((pStmt->nFlags & SQLEXECEX_PRESERVE_RECNO) && (pStmt->nFlags & SQLEXECEX_APPEND_CURSOR) == 0)
	{
		SaveCustomError("SqlExecEx", "Invalid nFlags parameter, SQLEXECEX_PRESERVE_RECNO requires SQLEXECEX_APPEND_CURSOR.");
		throw E_INVALIDPARAMS;
	}

	if ((pStmt->nFlags & (SQLEXECEX_CALLBACK_INFO | SQLEXECEX_STORE_INFO)) == (SQLEXECEX_CALLBACK_INFO | SQLEXECEX_STORE_INFO))
	{
		SaveCustomError("SqlExecEx", "Invalid nFlags parameter, SQLEXECEX_CALLBACK_INFO and SQLEXECEX_STORE_INFO or mutually exclusive.");
		throw E_INVALIDPARAMS;
	}

	pStmt->pSQLInput.Attach(parm(2));

	if (parm.CheckOptionalParameterLen(3))
	{
		pStmt->pCursorNames.Attach(parm(3));
		if (prepared)
			pStmt->pCursorNames.DetachParameter();
	}

	if (parm.CheckOptionalParameterLen(4))
	{
		pStmt->pInfoArray.Dimension(parm(4), 1, 2);
	}
	else if (pStmt->nFlags & SQLEXECEX_STORE_INFO)
	{
		SaveCustomError("SqlExecEx", "SQLEXECEX_STORE_INFO requires the passing of an array name in parameter cArray.");
		throw E_INVALIDPARAMS;
	}

	if (parm.CheckOptionalParameterLen(6))
	{
		pStmt->pCursorSchema.Attach(parm(6));
		if (prepared)
			pStmt->pCursorSchema.DetachParameter();
	}

	if (parm.CheckOptionalParameterLen(7))
	{
		pStmt->pParamSchema.Attach(parm(7));
		if (prepared)
			pStmt->pParamSchema.DetachParameter();
	}

	if (parm.CheckOptionalParameterLen(8))
	{
		if (parm(8)->Len() > VFP2C_MAX_CALLBACKFUNCTION)
		{
			SaveCustomError("SqlExecEx", "Callback function length is greater than maximum length of 1024.");
			throw E_INVALIDPARAMS;
		}
		if (!(pStmt->nFlags & (SQLEXECEX_CALLBACK_INFO | SQLEXECEX_CALLBACK_PROGRESS)))
		{
			SaveCustomError("SqlExecEx", "Callback function passed, but the nFlags parameter does not specify SQLEXECEX_CALLBACK_INFO or SQLEXECEX_CALLBACK_PROGRESS.");
			throw E_INVALIDPARAMS;
		}
		FoxString pCallback(parm(8));
		if (pStmt->nFlags & SQLEXECEX_CALLBACK_INFO)
		{
			pStmt->pCallback.SetCallback(pCallback);
		}
	}
	else
	{
		pStmt->nFlags &= ~(SQLEXECEX_CALLBACK_INFO | SQLEXECEX_CALLBACK_PROGRESS);
	}

	if (parm.PCount() >= 9 && parm(9)->ev_long)
		pStmt->nCallbackInterval = parm(9)->ev_long;
	else
		pStmt->nCallbackInterval = 100;

	// 1.1 - build command to evaluate connection handle
	pBuffer.Format("SQLGETPROP(%U,'ODBChdbc')", parm(1)->ev_long);

	// 1.2 - evaluate connection handle
	Evaluate(vConHandle, pBuffer);
	pStmt->hConn = vConHandle.DynamicPtr<SQLHDBC>();

	if (CFoxVersion::MajorVersion() >= 9) 
	{
		Evaluate(vMapVarchar, "CURSORGETPROP('MapVarchar', 0)");
		pStmt->bMapVarchar = vMapVarchar.ev_length > 0;
	}
	else
		pStmt->bMapVarchar = false;

	// 1.3 - allocate statement handle on connection
	if (SQLAllocHandle(SQL_HANDLE_STMT,pStmt->hConn,&pStmt->hStmt) == SQL_ERROR)
	{
		SafeODBCDbcError("SQLAllocHandle", pStmt->hConn);
		throw E_APIERROR;
	}
	return pStmt;
}
catch (int nErrorNo)
{
	if (pStmt)
		delete pStmt;
	throw nErrorNo;
}
}
#pragma warning(default : 4290)

SqlStatement::SqlStatement() {

	pParamData = 0;
	hStmt = 0;
	hConn = 0;
	nNoOfParms = 0;
	bOutputParams = FALSE;
	nResultset = 0;
	nCallbackInterval = 100;
	nRowsTotal = 0;
	nRowsFetched = 0;
	nFlags = 0;
	bMapVarchar = false;
	bPrepared = false;
	bExecutedOnce = false;
}

SqlStatement::~SqlStatement()
{
	FreeParameters();
	pResultSets.RemoveAll();

	if (pParamData)
		delete[] pParamData;

	if (hStmt)
	{
		SQLRETURN nApiRet = SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
		if (nApiRet != 0)
		{
			SafeODBCStmtError("SQLFreeHandle", hStmt);
			throw E_APIERROR;
		}
	}
}

void SqlStatement::NumParamsEx(char* pSQL)
{
	int nParms = 0;
	while (*pSQL)
	{
		if (*pSQL == '"') // skip over strings
		{
			while (*pSQL && *pSQL++ != '"');
		}
		else if (*pSQL == '\'') // skip over strings
		{
			while (*pSQL && *pSQL++ != '\'');
		}
		else if (*pSQL == '?' && (pSQL[1] == '{' || (pSQL[1] == '@' && pSQL[2] == '{')))
		{
			while (*pSQL)
			{
				if (*pSQL == '\\' && pSQL[1] == '}')
					pSQL += 2;
				else if (*pSQL++ == '}')
					break;
			}
			nParms++;
		}
		else
			pSQL++;
	}
	nNoOfParms = nParms;
}

SqlResultSet* SqlStatement::AddResultSet()
{
	SqlResultSet* pResultSet;
	if (pResultSets.GetCount() > nResultset)
	{
		pResultSet = pResultSets.GetAt(nResultset);
	}
	else
	{ 
		pResultSet = new SqlResultSet(this);
		if (pResultSet == 0)
			throw E_INSUFMEMORY;
		pResultSets.Add(CAutoPtr<SqlResultSet>(pResultSet));
	}
	nResultset++;
	return pResultSet;
}

void SqlStatement::FreeParameters()
{
	if (pParamData)
	{
		SqlParameter* lpParms = pParamData;
		int count = nNoOfParms;
		while (count--)
		{
			if (lpParms->vParmValue.Vartype() == 'C')
			{
				lpParms->vParmValue.UnlockHandle();
				lpParms->vParmValue.FreeHandle();
				lpParms->vParmValue.SetNull();
			}
			lpParms++;
		}
	}
}

void SqlStatement::ExtractParamsAndRewriteStatement()
{
	SqlParameter* lpPS = pParamData;
	char* pParmExpr, * pSQLIn, *pSQLOut, *pSqlOutStart;
	int nParamNo = 1, nParmLen;

	pSQLSend.Size(pSQLInput.Size());
	pSQLIn = pSQLInput;
	pSQLOut = pSqlOutStart = pSQLSend;

	while (*pSQLIn)
	{
		if (*pSQLIn == '"') // skip over strings
		{
			*pSQLOut++ = *pSQLIn++;
			while (*pSQLIn && *pSQLIn != '"') *pSQLOut++ = *pSQLIn++;
			*pSQLOut++ = *pSQLIn++;
		}
		else if (*pSQLIn == '\'') // skip over strings
		{
			*pSQLOut++ = *pSQLIn++;
			while (*pSQLIn && *pSQLIn != '\'') *pSQLOut++ = *pSQLIn++;
			*pSQLOut++ = *pSQLIn++;
		}
		else if (*pSQLIn == '?' && (pSQLIn[1] == '{' || (pSQLIn[1] == '@' && pSQLIn[2] == '{')))
		{
			lpPS->nParmNo = nParamNo++;

			*pSQLOut++ = *pSQLIn; // write "?" 

			if (pSQLIn[1] == '@') // input/output parameter?
			{
				lpPS->nParmDirection = SQL_PARAM_INPUT_OUTPUT;
				pSQLIn += 3; // skip over "?@{"
			}
			else
			{
				lpPS->nParmDirection = SQL_PARAM_INPUT;
				pSQLIn += 2; // skip over "?{"
			}

			nParmLen = VFP2C_ODBC_MAX_PARAMETER_EXPR;
			pParmExpr = lpPS->aParmExpr;

			while (*pSQLIn && nParmLen)
			{
				if (*pSQLIn == '\\' && pSQLIn[1] == '}')
				{
					*pParmExpr++ = '}';
					pSQLIn += 2;
				}
				else if (*pSQLIn == '}')
					break;
				else
					*pParmExpr++ = *pSQLIn++;

				nParmLen--;
			}
			if (!nParmLen)
			{
				SaveCustomError("SQLExecEx", "Parameter expression '%20S...' exceeds limit of 256 characters", lpPS->aParmExpr);
				throw E_APIERROR;
			}
			*pParmExpr = '\0'; // nullterminate parameter expression

			pSQLIn++; // skip over "}"

			lpPS++; // inc to next parameter
		}
		else
			*pSQLOut++ = *pSQLIn++;
	}

	*pSQLOut = '\0'; // nullterminate Sql output
	pSQLSend.Len(pSQLOut - pSqlOutStart); // store length of rewritten SQL statement
}

void SqlStatement::ParseParamSchema()
{
	SqlParameter* lpPS;
	int nParmNo;
	char* pSchema;
	char aSQLType[VFP2C_ODBC_MAX_SQLTYPE];

	if (!pParamSchema.Len())
		return;

	pSchema = pParamSchema;

	do
	{
		if (!match_int(&pSchema, &nParmNo))
		{
			SaveCustomError("SQLExecEx", "Parameter schema contained invalid data near '%10S', expected parameter number.", pSchema);
			throw E_APIERROR;
		}
		if (nParmNo < 1 || nParmNo > nNoOfParms)
		{
			SaveCustomError("SQLExecEx", "Parameter schema contained invalid data, parameter number '%I' out of bounds.", nParmNo);
			throw E_APIERROR;
		}
		// set pointer to parameter number X
		lpPS = pParamData + (nParmNo - 1);

		lpPS->bNamed = match_quoted_identifier(&pSchema, lpPS->aParmName, VFP2C_ODBC_MAX_PARAMETER_NAME);

		if (match_identifier(&pSchema, aSQLType, VFP2C_ODBC_MAX_SQLTYPE))
		{
			if (StrEqual(aSQLType, "SQL_WCHAR"))
			{
				lpPS->nSQLType = SQL_WCHAR;
				lpPS->nCType = SQL_C_WCHAR;
			}
			else if (StrEqual(aSQLType, "SQL_BINARY"))
			{
				lpPS->nSQLType = SQL_BINARY;
				lpPS->nCType = SQL_C_BINARY;
			}
			else if (StrEqual(aSQLType, "SQL_CHAR"))
			{
				lpPS->nSQLType = SQL_CHAR;
				lpPS->nCType = SQL_C_CHAR;
			}
			else if (StrEqual(aSQLType, "SQL_DATE"))
			{
				lpPS->nSQLType = SQL_DATE;
				lpPS->nCType = SQL_C_DATE;
			}
			else if (StrEqual(aSQLType, "SQL_TIMESTAMP"))
			{
				lpPS->nSQLType = SQL_TIMESTAMP;
				lpPS->nCType = SQL_C_TIMESTAMP;
				if (match_chr(&pSchema, '('))
				{
					if (!match_int(&pSchema, (int*)&lpPS->nPrecision))
					{
						SaveCustomError("SQLExecEx", "Parameter schema contained invalid data near '%20S', expected precision.", pSchema);
						throw E_APIERROR;
					}
					if (!match_chr(&pSchema, ')'))
					{
						SaveCustomError("SQLExecEx", "Parameter schema contained invalid data near '%20S', expected ')'.", pSchema);
						throw E_APIERROR;
					}
					lpPS->nSize = lpPS->nPrecision + 20;
				}
			}
			else if (StrEqual(aSQLType, "SQL_BIGINT"))
			{
				lpPS->nSQLType = SQL_BIGINT;
				lpPS->nCType = SQL_C_CHAR;
			}
			else if (StrEqual(aSQLType, "SQL_INTEGER"))
			{
				lpPS->nSQLType = SQL_INTEGER;
				lpPS->nCType = SQL_C_LONG;
			}
			else if (StrEqual(aSQLType, "SQL_SMALLINT"))
			{
				lpPS->nSQLType = SQL_SMALLINT;
				lpPS->nCType = SQL_C_LONG;
			}
			else if (StrEqual(aSQLType, "SQL_DOUBLE"))
			{
				lpPS->nSQLType = SQL_DOUBLE;
				lpPS->nCType = SQL_C_DOUBLE;
			}
			else if (StrEqual(aSQLType, "SQL_FLOAT"))
			{
				lpPS->nSQLType = SQL_FLOAT;
				lpPS->nCType = SQL_C_DOUBLE;
			}
			else if (StrEqual(aSQLType, "SQL_REAL"))
			{
				lpPS->nSQLType = SQL_REAL;
				lpPS->nCType = SQL_C_DOUBLE;
			}
			else if (StrEqual(aSQLType, "SQL_NUMERIC"))
			{
				lpPS->nSQLType = SQL_NUMERIC;
				lpPS->nCType = SQL_C_CHAR;
			}
			else if (StrEqual(aSQLType, "SQL_DECIMAL"))
			{
				lpPS->nSQLType = SQL_DECIMAL;
				lpPS->nCType = SQL_C_CHAR;
			}
			else
			{
				SaveCustomError("SQLExecEx", "Parameter schema contained invalid data '%S', expected SQL type.", aSQLType);
				throw E_APIERROR;
			}

			lpPS->bCustomSchema = TRUE;
		}
		// if no type identified and also no parameter name was specified the parameter schema is invalid
		else if (!lpPS->bNamed)
		{
			SaveCustomError("SQLExecEx", "Parameter schema contained invalid data near '%20S', expected SQL type.", pSchema);
			throw E_APIERROR;
		}

	} while (match_chr(&pSchema, ','));

	if (!match_chr(&pSchema, '\0'))
	{
		SaveCustomError("SQLExecEx", "Parameter schema contained invalid data near '%20S'.", pSchema);
		throw E_APIERROR;
	}
}

void SqlStatement::EvaluateParams()
{
	SqlParameter* lpPS = pParamData;
	int nErrorNo, xj;

	for (xj = 1; xj <= nNoOfParms; xj++)
	{
		if (lpPS->nParmDirection == SQL_PARAM_INPUT_OUTPUT || lpPS->nParmDirection == SQL_PARAM_OUTPUT)
		{
			if (nErrorNo = FindFoxVarOrFieldEx(lpPS->aParmExpr, &lpPS->lVarOrField))
				throw nErrorNo;

			bOutputParams = TRUE;
		}

		Evaluate(lpPS->vParmValue, lpPS->aParmExpr);

		switch (lpPS->vParmValue.ev_type)
		{
		case 'C':
			if (lpPS->nParmDirection == SQL_PARAM_INPUT_OUTPUT || lpPS->nParmDirection == SQL_PARAM_OUTPUT)
			{
				if (!lpPS->bCustomSchema || lpPS->nSQLType == SQL_BINARY || lpPS->nSQLType == SQL_CHAR ||
					lpPS->nSQLType == SQL_WCHAR)
				{
					if (lpPS->vParmValue.Len() < VFP2C_ODBC_MAX_BUFFER)
					{
						if (!lpPS->vParmValue.SetHandleSize(VFP2C_ODBC_MAX_BUFFER))
							throw E_INSUFMEMORY;
					}
					lpPS->nBufferSize = VFP2C_ODBC_MAX_BUFFER;
					lpPS->nSize = lpPS->vParmValue.Len();
				}
			}
			else
				lpPS->nSize = lpPS->nBufferSize = lpPS->vParmValue.Len();

			lpPS->vParmValue.LockHandle();
			lpPS->pParmData = lpPS->vParmValue.HandleToPtr();

			if (lpPS->bCustomSchema)
			{
				if (lpPS->nSQLType != SQL_BINARY && lpPS->nSQLType != SQL_WCHAR &&
					lpPS->nSQLType != SQL_TIMESTAMP)
				{
					SaveCustomError("SQLExecEx", "Invalid datatype conversion specified for parameter '%S'.", lpPS->aParmExpr);
					throw E_APIERROR;
				}

				if (lpPS->nSQLType == SQL_BINARY)
					lpPS->nSQLType = SQL_LONGVARBINARY;
				else if (lpPS->nSQLType == SQL_WCHAR)
					lpPS->nSQLType = SQL_WLONGVARCHAR;
				else if (lpPS->nSQLType == SQL_CHAR)
					lpPS->nSQLType = SQL_LONGVARCHAR;

				lpPS->nIndicator = SQL_LEN_DATA_AT_EXEC((int)lpPS->nSize);
				lpPS->bPutData = TRUE;
			}
			else
			{
				lpPS->nCType = lpPS->vParmValue.ev_width ? SQL_C_BINARY : SQL_C_CHAR;

				if (lpPS->nSize < VFP2C_ODBC_MAX_CHAR_LEN)
				{
					lpPS->nSQLType = lpPS->vParmValue.ev_width ? SQL_BINARY : SQL_CHAR;
					lpPS->nIndicator = lpPS->nSize;
				}
				else
				{
					lpPS->nSQLType = lpPS->vParmValue.ev_width ? SQL_LONGVARBINARY : SQL_LONGVARCHAR;
					lpPS->nIndicator = SQL_LEN_DATA_AT_EXEC((int)lpPS->nSize);
					lpPS->bPutData = TRUE;
				}
			}
			break;

		case 'M':
		case 'W':
			lpPS->nSize = lpPS->vParmValue.ev_long;
			if (lpPS->nParmDirection == SQL_PARAM_INPUT_OUTPUT || lpPS->nParmDirection == SQL_PARAM_OUTPUT)
			{
				lpPS->nBufferSize = VFP2C_ODBC_MAX_BUFFER + 1;
				if (!lpPS->vParmValue.AllocHandle(max(lpPS->nSize, VFP2C_ODBC_MAX_BUFFER + 1)))
					throw E_INSUFMEMORY;
			}
			else
			{
				if (!lpPS->vParmValue.AllocHandle(lpPS->nSize))
					throw E_INSUFMEMORY;
			}

			lpPS->vParmValue.LockHandle();
			lpPS->pParmData = lpPS->vParmValue.HandleToPtr();

			if (nErrorNo = GetMemoContent(lpPS->vParmValue, (char*)lpPS->pParmData))
				throw nErrorNo;

			if (lpPS->bCustomSchema)
			{
				if (lpPS->nSQLType != SQL_BINARY && lpPS->nSQLType != SQL_WCHAR &&
					lpPS->nSQLType != SQL_CHAR)
				{
					SaveCustomError("SQLExecEx", "Invalid datatype conversion specified for parameter '%S'.", lpPS->aParmExpr);
					throw E_APIERROR;
				}
				if (lpPS->nSQLType == SQL_BINARY)
					lpPS->nSQLType = SQL_LONGVARBINARY;
				else if (lpPS->nSQLType == SQL_WCHAR)
					lpPS->nSQLType = SQL_WLONGVARCHAR;
				else if (lpPS->nSQLType == SQL_CHAR)
					lpPS->nSQLType = SQL_LONGVARCHAR;

				lpPS->nIndicator = SQL_LEN_DATA_AT_EXEC((int)lpPS->nSize);
				lpPS->bPutData = TRUE;
			}
			else
			{
				// lpPS->vParmValue.ev_length contains either the codepage of the memo/blob or 0 if it's a blob/NOCPTRANS field
				lpPS->nCType = lpPS->vParmValue.ev_length ? SQL_C_CHAR : SQL_C_BINARY;
				lpPS->nSQLType = lpPS->vParmValue.ev_length ? SQL_LONGVARCHAR : SQL_LONGVARBINARY;
				lpPS->nIndicator = SQL_LEN_DATA_AT_EXEC((int)lpPS->nSize);
				lpPS->bPutData = TRUE;
			}
			break;

		case 'N':
			if (lpPS->bCustomSchema)
			{
				if (lpPS->nSQLType != SQL_DOUBLE && lpPS->nSQLType != SQL_FLOAT &&
					lpPS->nSQLType != SQL_REAL && lpPS->nSQLType != SQL_INTEGER &&
					lpPS->nSQLType != SQL_SMALLINT && lpPS->nSQLType != SQL_BIGINT)
				{
					SaveCustomError("SQLExecEx", "Invalid datatype conversion specified for parameter '%S'.", lpPS->aParmExpr);
					throw E_APIERROR;
				}
				lpPS->pParmData = &lpPS->vParmValue.ev_real;
				lpPS->nCType = SQL_C_DOUBLE;
			}
			else
			{
				lpPS->pParmData = &lpPS->vParmValue.ev_real;
				lpPS->nCType = SQL_C_DOUBLE;
				lpPS->nSQLType = SQL_DOUBLE;
			}
			break;

		case 'I':
			if (lpPS->bCustomSchema)
			{
				if (lpPS->nSQLType != SQL_DOUBLE && lpPS->nSQLType != SQL_FLOAT &&
					lpPS->nSQLType != SQL_REAL && lpPS->nSQLType != SQL_INTEGER &&
					lpPS->nSQLType != SQL_SMALLINT && lpPS->nSQLType != SQL_BIGINT)
				{
					SaveCustomError("SQLExecEx", "Invalid datatype conversion specified for parameter '%S'.", lpPS->aParmExpr);
					throw E_APIERROR;
				}
				lpPS->pParmData = &lpPS->vParmValue.ev_long;
				lpPS->nCType = SQL_INTEGER;
			}
			else
			{
				lpPS->pParmData = &lpPS->vParmValue.ev_long;
				lpPS->nCType = SQL_C_SLONG;
				lpPS->nSQLType = SQL_INTEGER;
			}
			break;

		case 'T':
			if (lpPS->bCustomSchema)
			{
				if (lpPS->nSQLType != SQL_DATE && lpPS->nSQLType != SQL_TIMESTAMP)
				{
					SaveCustomError("SQLExecEx", "Invalid datatype conversion specified for parameter '%S'.", lpPS->aParmExpr);
					throw E_APIERROR;
				}

				if (lpPS->vParmValue.ev_real == 0.0)
				{
					lpPS->nIndicator = SQL_NULL_DATA;
					lpPS->nCType = SQL_C_TIMESTAMP;
					lpPS->nSQLType = SQL_TIMESTAMP;
					lpPS->pParmData = &lpPS->sDateTime;
				}
				else
				{
					if (lpPS->nSQLType == SQL_TIMESTAMP)
						DateTimeToTimestamp_Struct(lpPS->vParmValue, &lpPS->sDateTime);
					else
						DateToTimestamp_Struct(lpPS->vParmValue, &lpPS->sDateTime);

					lpPS->pParmData = &lpPS->sDateTime;
					lpPS->nCType = SQL_C_TIMESTAMP;
					lpPS->nSQLType = SQL_TIMESTAMP;
					lpPS->nSize = sizeof(SQL_TIMESTAMP_STRUCT);
				}
			}
			else
			{
				if (lpPS->vParmValue.ev_real == 0.0)
					lpPS->nIndicator = SQL_NULL_DATA;
				else
					DateTimeToTimestamp_Struct(lpPS->vParmValue, &lpPS->sDateTime);

				lpPS->pParmData = &lpPS->sDateTime;
				lpPS->nCType = SQL_C_TIMESTAMP;
				lpPS->nSQLType = SQL_TIMESTAMP;
				lpPS->nSize = lpPS->nBufferSize = sizeof(SQL_TIMESTAMP_STRUCT);
			}
			break;

		case 'D':
			if (lpPS->bCustomSchema)
			{
				if (lpPS->nSQLType != SQL_DATE && lpPS->nSQLType != SQL_TIMESTAMP)
				{
					SaveCustomError("SQLExecEx", "Invalid datatype conversion specified for parameter '%S'.", lpPS->aParmExpr);
					throw E_APIERROR;
				}

				if (lpPS->vParmValue.ev_real == 0.0)
					lpPS->nIndicator = SQL_NULL_DATA;
				else
					DateToTimestamp_Struct(lpPS->vParmValue, &lpPS->sDateTime);

				lpPS->pParmData = &lpPS->sDateTime;
				lpPS->nCType = SQL_C_TIMESTAMP;
				lpPS->nSQLType = SQL_TIMESTAMP;
				lpPS->nSize = sizeof(SQL_TIMESTAMP_STRUCT);
			}
			else
			{
				if (lpPS->vParmValue.ev_real == 0.0)
					lpPS->nIndicator = SQL_NULL_DATA;
				else
					DateToTimestamp_Struct(lpPS->vParmValue, &lpPS->sDateTime);

				lpPS->pParmData = &lpPS->sDateTime;
				lpPS->nCType = SQL_C_TIMESTAMP;
				lpPS->nSQLType = SQL_TIMESTAMP;
				lpPS->nSize = sizeof(SQL_TIMESTAMP_STRUCT);
			}
			break;

		case 'L':
			lpPS->pParmData = &lpPS->vParmValue.ev_length;
			lpPS->nCType = SQL_C_SLONG;
			lpPS->nSQLType = SQL_BIT;
			break;

		case 'Y':
			CurrencyToNumericLiteral(lpPS->vParmValue, lpPS->aNumeric);
			lpPS->pParmData = lpPS->aNumeric;
			lpPS->nSize = VFP2C_VFP_CURRENCY_PRECISION;
			lpPS->nScale = VFP2C_VFP_CURRENCY_SCALE;
			lpPS->nBufferSize = VFP2C_ODBC_MAX_CURRENCY_LITERAL + 1;
			lpPS->nIndicator = SQL_NTS;
			lpPS->nCType = SQL_C_CHAR;
			lpPS->nSQLType = SQL_DECIMAL;
			break;

		case '0':
			lpPS->nIndicator = SQL_NULL_DATA;
			// set default if not specified in parameter schema
			if (!lpPS->bCustomSchema)
			{
				lpPS->nCType = SQL_C_DEFAULT;
				lpPS->nSQLType = SQL_CHAR;
			}
			else
			{
				if (lpPS->nParmDirection == SQL_PARAM_INPUT_OUTPUT || lpPS->nParmDirection == SQL_PARAM_OUTPUT)
				{
					switch (lpPS->nSQLType)
					{
					case SQL_INTEGER:
					case SQL_SMALLINT:
						lpPS->vParmValue.SetInt();
						lpPS->pParmData = &lpPS->vParmValue.ev_long;
						break;

					case SQL_DOUBLE:
					case SQL_FLOAT:
					case SQL_REAL:
						lpPS->vParmValue.SetDouble();
						lpPS->pParmData = &lpPS->vParmValue.ev_real;
						break;

					case SQL_CHAR:
					case SQL_WCHAR:
					case SQL_BINARY:
						lpPS->vParmValue.SetString();
						if (!lpPS->vParmValue.AllocHandle(VFP2C_ODBC_MAX_BUFFER))
							throw E_INSUFMEMORY;
						lpPS->nBufferSize = VFP2C_ODBC_MAX_BUFFER;
						lpPS->vParmValue.LockHandle();
						lpPS->pParmData = lpPS->vParmValue.HandleToPtr();
						break;

					case SQL_DATE:
					case SQL_TIMESTAMP:
						lpPS->vParmValue.ev_type = lpPS->nSQLType == SQL_DATE ? 'D' : 'T';
						lpPS->pParmData = &lpPS->sDateTime;
						lpPS->nBufferSize = sizeof(SQL_TIMESTAMP_STRUCT);
						break;

					case SQL_BIGINT:
						lpPS->vParmValue.SetString();
						if (!lpPS->vParmValue.AllocHandle(VFP2C_ODBC_MAX_BIGINT_LITERAL + 1))
							throw E_INSUFMEMORY;
						lpPS->vParmValue.LockHandle();
						lpPS->pParmData = lpPS->vParmValue.HandleToPtr();
						lpPS->nBufferSize = VFP2C_ODBC_MAX_BIGINT_LITERAL;
						break;

					case SQL_NUMERIC:
					case SQL_DECIMAL:
						lpPS->vParmValue.SetCurrency();
						lpPS->pParmData = &lpPS->aNumeric;
						lpPS->nBufferSize = VFP2C_ODBC_MAX_CURRENCY_LITERAL;
						break;
					}
				}
			}
		}

		lpPS++;
	}
}

void SqlStatement::BindParameters()
{
	SQLRETURN nApiRet = SQL_SUCCESS;
	SQLHDESC hDesc;
	SqlParameter* lpPS = pParamData;
	for (int xj = 1; xj <= nNoOfParms; xj++)
	{
		nApiRet = SQLBindParameter(hStmt, lpPS->nParmNo, lpPS->nParmDirection, lpPS->nCType,
			lpPS->nSQLType, lpPS->nSize, lpPS->nScale,
			lpPS->bPutData ? lpPS : lpPS->pParmData, lpPS->nBufferSize,
			&lpPS->nIndicator);
		if (nApiRet == SQL_ERROR)
		{
			SafeODBCStmtError("SQLBindParameter", hStmt);
			throw E_APIERROR;
		}

		// if the SQL type is SQL_TIMESTAMP and precision unequals 0, set precision
		if (lpPS->nSQLType == SQL_TIMESTAMP && lpPS->nPrecision)
		{
			// get descriptor handle for application parameter descriptor's
			nApiRet = SQLGetStmtAttr(hStmt, SQL_ATTR_APP_PARAM_DESC, &hDesc, 0, 0);
			if (nApiRet == SQL_ERROR)
			{
				SafeODBCStmtError("SQLGetStmtAttr", hStmt);
				throw E_APIERROR;
			}
			// set descriptor
			nApiRet = SQLSetDescRec(hDesc, lpPS->nParmNo, lpPS->nSQLType, 0, 0,
				(SQLSMALLINT)lpPS->nPrecision, lpPS->nScale, lpPS->pParmData, &lpPS->nIndicator, &lpPS->nIndicator);
			if (nApiRet == SQL_ERROR)
			{
				SafeODBCStmtError("SQLSetDescRec", hStmt);
				throw E_APIERROR;
			}
			// get descriptor handle for implementation descriptor's
			nApiRet = SQLGetStmtAttr(hStmt, SQL_ATTR_IMP_PARAM_DESC, &hDesc, 0, 0);
			if (nApiRet == SQL_ERROR)
			{
				SafeODBCStmtError("SQLGetStmtAttr", hStmt);
				throw E_APIERROR;
			}
			// set descriptor 
			nApiRet = SQLSetDescRec(hDesc, lpPS->nParmNo, lpPS->nSQLType, 0, 0,
				(SQLSMALLINT)lpPS->nPrecision, lpPS->nScale, lpPS->pParmData, &lpPS->nIndicator, &lpPS->nIndicator);
			if (nApiRet == SQL_ERROR)
			{
				SafeODBCStmtError("SQLSetDescRec", hStmt);
				throw E_APIERROR;
			}
		}

		// if we have a named parameter, set additional descriptor information
		if (lpPS->bNamed)
		{
			// get descriptor handle for implementation descriptor's
			nApiRet = SQLGetStmtAttr(hStmt, SQL_ATTR_IMP_PARAM_DESC, &hDesc, 0, 0);
			if (nApiRet == SQL_ERROR)
			{
				SafeODBCStmtError("SQLGetStmtAttr", hStmt);
				throw E_APIERROR;
			}
			nApiRet = SQLSetDescField(hDesc, lpPS->nParmNo, SQL_DESC_NAME, lpPS->aParmName, SQL_NTS);
			if (nApiRet == SQL_ERROR)
			{
				SafeODBCStmtError("SQLSetDescField", hStmt);
				throw E_APIERROR;
			}
			nApiRet = SQLSetDescField(hDesc, lpPS->nParmNo, SQL_DESC_UNNAMED, SQL_NAMED, 0);
			if (nApiRet == SQL_ERROR)
			{
				SafeODBCStmtError("SQLSetDescField", hStmt);
				throw E_APIERROR;
			}
		}
		lpPS++;
	}
}

void SqlStatement::PutData()
{
	SQLRETURN nApiRet;
	SqlParameter* lpParm;
	char* lpData;
	SQLINTEGER nBufferSize, nDataSize;

	nApiRet = SQLParamData(hStmt, (SQLPOINTER*)&lpParm);

	while (nApiRet == SQL_NEED_DATA)
	{
		lpData = (char*)lpParm->pParmData;
		nDataSize = lpParm->nSize;

	SQLSendData:
		nBufferSize = nDataSize > VFP2C_ODBC_MAX_BUFFER ? VFP2C_ODBC_MAX_BUFFER : nDataSize;
		nApiRet = SQLPutData(hStmt, lpData, nBufferSize);
		if (nApiRet == SQL_ERROR)
		{
			SafeODBCStmtError("SQLPutData", hStmt);
			throw E_APIERROR;
		}

		nDataSize -= nBufferSize;
		lpData += nBufferSize;

		if (nDataSize > 0)
			goto SQLSendData;

		nApiRet = SQLParamData(hStmt, (SQLPOINTER*)&lpParm);
	}
	if (nApiRet == SQL_ERROR)
	{
		SafeODBCStmtError("SQLParamData", hStmt);
		throw E_APIERROR;
	}
}

void SqlStatement::SaveOutputParameters()
{
	if (!bOutputParams)
		return;

	SqlParameter* lpPS;
	Value vNull = { '0' };

	lpPS = pParamData;

	for (int xj = 1; xj <= nNoOfParms; xj++)
	{
		if (lpPS->nParmDirection == SQL_PARAM_INPUT_OUTPUT || lpPS->nParmDirection == SQL_PARAM_OUTPUT)
		{
			if (lpPS->nIndicator == SQL_NULL_DATA)
			{
				lpPS->lVarOrField = vNull;
				lpPS++;
				continue;
			}

			switch (lpPS->vParmValue.ev_type)
			{

			case 'L':
			case 'I':
			case 'N':
				lpPS->lVarOrField = lpPS->vParmValue;
				break;

			case 'D':
				Timestamp_StructToDate(&lpPS->sDateTime, lpPS->vParmValue);
				lpPS->lVarOrField = lpPS->vParmValue;
				break;

			case 'T':
				Timestamp_StructToDateTime(&lpPS->sDateTime, lpPS->vParmValue);
				lpPS->lVarOrField = lpPS->vParmValue;
				break;

			case 'C':
			case 'V':
			case 'Q':
			case 'M':
			case 'W':
				lpPS->vParmValue.ev_length = lpPS->nIndicator;
				lpPS->lVarOrField = lpPS->vParmValue;
				break;

			case 'Y':
				NumericLiteralToCurrency((SQLCHAR*)lpPS->pParmData, lpPS->vParmValue);
				lpPS->lVarOrField = lpPS->vParmValue;
				break;

			default:
				throw E_APIERROR;
			}
		}

		lpPS++;
	}
}

void SqlStatement::ProgressCallback(int nRowsFetched, BOOL* bAbort)
{
	ValueEx vCallRet;
	vCallRet = 0;
	int nErrorNo;
	bool bAbortFlag;

	if (nErrorNo = pCallback.Evaluate(vCallRet, nResultset, nRowsFetched, nRowsTotal))
		throw nErrorNo;

	if (vCallRet.Vartype() == 'L')
		bAbortFlag = !vCallRet.ev_length;
	else if (vCallRet.Vartype() == 'I')
		bAbortFlag = vCallRet.ev_long == 0;
	else if (vCallRet.Vartype() == 'N')
		bAbortFlag = static_cast<DWORD>(vCallRet.ev_real) == 0;
	else
	{
		bAbortFlag = false;
		vCallRet.Release();
	}

	if (bAbortFlag)
	{
		*bAbort = TRUE;
		throw E_APIERROR;
	}
}

void SqlStatement::InfoCallbackOrStore()
{
	SQLRETURN nApiRet;
	SQLSMALLINT nMsgLen, nError = 1;
	SQLINTEGER nNativeError;
	SQLCHAR aSQLState[16];

	if ((nFlags & SQLEXECEX_STORE_INFO) || (nFlags & SQLEXECEX_CALLBACK_INFO))
	{
		pGetDataBuffer.Size(VFP2C_ODBC_MAX_BUFFER);
		nApiRet = SQLGetDiagRec(SQL_HANDLE_STMT, hStmt, nError++, aSQLState, &nNativeError,
			pGetDataBuffer.Ptr<SQLCHAR*>(), pGetDataBuffer.Size(), &nMsgLen);

		while (nApiRet == SQL_SUCCESS)
		{
			pGetDataBuffer.Len(SQLExtractInfo(pGetDataBuffer, nMsgLen));
			if (nFlags & SQLEXECEX_STORE_INFO)
			{
				int nIndex = pInfoArray.Grow();
				pInfoArray(nIndex, 1) = pGetDataBuffer;
			}
			else if ((nFlags & SQLEXECEX_CALLBACK_INFO))
			{
				CStringView pInfo = pGetDataBuffer;
				pCallback.Execute(-1, pInfo);
			}
			nApiRet = SQLGetDiagRec(SQL_HANDLE_STMT, hStmt, nError++, aSQLState, &nNativeError,
				pGetDataBuffer.Ptr<SQLCHAR*>(), VFP2C_ODBC_MAX_BUFFER, &nMsgLen);
		}
	}
}


SqlResultSet::SqlResultSet(SqlStatement* stmt)
{
	pStmt = stmt;
	pColumnData = 0;
	nNoOfCols = 0;
	nWorkArea = 0;
}

SqlResultSet::~SqlResultSet()
{
	if (pColumnData)
	{
		SqlColumn* lpSC = pColumnData;
		int count = nNoOfCols;
		SQLPOINTER pGetDataPtr = pStmt->pGetDataBuffer.Ptr<SQLPOINTER>();
		while (count--)
		{
			// if the handle is valid and the buffer isn't our 
			// general purpose buffer for long data we need to free it
			if (lpSC->vData.Vartype() == 'C' && lpSC->vData.ValidHandle() && lpSC->pData != pGetDataPtr)
			{
				lpSC->vData.UnlockHandle();
				lpSC->vData.FreeHandle();
			}
			lpSC++;
		}
		delete[] pColumnData;
		pColumnData = 0;
	}
}

void SqlResultSet::AllocateColumns(SQLSMALLINT columncount)
{
	// we've got a resultset, allocate space to store metadata for each column
	if (pColumnData == 0 || nNoOfCols != columncount)
	{
		if (pColumnData)
		{
			delete[] pColumnData;
			pColumnData = 0;
		}
		nNoOfCols = columncount;
		pColumnData = new SqlColumn[nNoOfCols];
		if (!pColumnData)
			throw E_INSUFMEMORY;
	}
}

void SqlResultSet::GetMetaData()
{
	if (pStmt->bExecutedOnce)
		return;

	SQLRETURN nApiRet = SQL_SUCCESS;
	SqlColumn* lpSC = pColumnData;
	SQLSMALLINT xj;
	int nUnnamedCol = 0;

	for (xj = 1; xj <= nNoOfCols; xj++)
	{

		nApiRet = SQLDescribeCol(pStmt->hStmt,xj,lpSC->aColName,VFP2C_ODBC_MAX_COLUMN_NAME,
								&lpSC->nNameLen,&lpSC->nSQLType,&lpSC->nSize,
								&lpSC->nScale,&lpSC->bNullable);
		if (nApiRet == SQL_ERROR)
		{
			SafeODBCStmtError("SQLDescribeCol", pStmt->hStmt);
			throw E_APIERROR;
		}

		switch (lpSC->nSQLType)
		{
			case SQL_INTEGER:
			case SQL_SMALLINT:
			case SQL_TINYINT:
				// if integer type check sign
				nApiRet = SQLColAttribute(pStmt->hStmt,xj,SQL_DESC_UNSIGNED,0,0,0,&lpSC->bUnsigned);
				if (nApiRet == SQL_ERROR)
				{
					SafeODBCStmtError("SQLColAttribute", pStmt->hStmt);
					throw E_APIERROR;
				}
				break;

			case SQL_NUMERIC:
			case SQL_DECIMAL:
				// if numeric or decimal check for money datatype, add 2 to the size, if NUMERIC or DECIMAL since FoxPro N datatype defines size including scale and decimal point
				nApiRet = SQLColAttribute(pStmt->hStmt,xj,SQL_COLUMN_MONEY,0,0,0,&lpSC->bMoney);
				if (nApiRet == SQL_ERROR)
				{
					SafeODBCStmtError("SQLColAttribute", pStmt->hStmt);
					throw E_APIERROR;
				}
				if (!lpSC->bMoney)
				{
					lpSC->nSize += 2;
					if (lpSC->nSize > 20)
						lpSC->nSize = 20;
					if (lpSC->nScale > 16)
						lpSC->nScale = 16;
				}
				break;

			case SQL_REAL:
			case SQL_FLOAT:
			case SQL_DOUBLE:
				lpSC->nSize = 20;
				lpSC->nScale = 16;
				break;

			case SQL_CHAR:
			case SQL_VARCHAR:
			case SQL_BINARY:
			case SQL_VARBINARY:
			case SQL_WCHAR:
			case SQL_WVARCHAR:
				if (pStmt->bMapVarchar == false)
				{
					if (lpSC->nSQLType == SQL_VARCHAR)
						lpSC->nSQLType = SQL_CHAR;
					else if (lpSC->nSQLType == SQL_WVARCHAR)
						lpSC->nSQLType = SQL_WCHAR;
				}
				// force character/binary fields of len 0 to Memo/Blob
				if (lpSC->nSize == 0)
					lpSC->nSize = VFP2C_VFP_MAX_CHARCOLUMN + 1;
				break;

			case SQL_LONGVARCHAR:
			case SQL_LONGVARBINARY:
			case SQL_WLONGVARCHAR:
			case SQL_DATE:
			case SQL_TIMESTAMP:
			case SQL_TIME:
			case SQL_BIGINT:
			case SQL_BIT:
			case SQL_GUID:
				break;

			default:
				nApiRet = SQLColAttribute(pStmt->hStmt,xj,SQL_DESC_DISPLAY_SIZE,0,0,0,&lpSC->nDisplaySize);
				if (nApiRet == SQL_ERROR)
				{
					SafeODBCStmtError("SQLColAttribute", pStmt->hStmt);
					throw E_APIERROR;
				}
				// if no displaysize is returned, force to a memo field
				if (!lpSC->nDisplaySize)
					lpSC->nDisplaySize = VFP2C_VFP_MAX_CHARCOLUMN + 1;
		}

		// empty column name (from an expression for example)
		if (lpSC->aColName[0] == '\0')
		{
			if (nUnnamedCol)
			{
				sprintfex((char*)lpSC->aColName, "expr%I", nUnnamedCol);
			}
			else
			{
#pragma warning(disable : 4996)
				strcpy((char*)lpSC->aColName, "expr");
#pragma warning(default : 4996)
			}

			nUnnamedCol++;
		}
		else
			FixColumnName((char*)lpSC->aColName);

		lpSC++;	
	}
}

void SqlResultSet::BindColumns()
{
	SQLRETURN nApiRet = SQL_SUCCESS;
	SqlColumn* lpSC = pColumnData;
	int xj = nNoOfCols;

	while (xj--)
	{
		if (lpSC->bBindColumn)
		{
			nApiRet = SQLBindCol(pStmt->hStmt,lpSC->nColNo, lpSC->nCType, lpSC->pData, 
				lpSC->nBufferSize, &lpSC->nIndicator);
			if (nApiRet == SQL_ERROR)
			{
				SafeODBCStmtError("SQLBindCol", pStmt->hStmt);
				throw E_APIERROR;
			}
		}
		lpSC++;
	}
}

void SqlResultSet::PrepareColumnBindings()
{
	if (pStmt->bExecutedOnce)
		return;

	SQLRETURN nApiRet;
	SQLUINTEGER bGetDataExt;
	BOOL bUnicodeConversion = VFP2CTls::Tls().SqlUnicodeConversion;
	SqlColumn* lpSC = pColumnData;
	FoxString* pGetDataBuffer = &pStmt->pGetDataBuffer;
	BOOL bBindCol = TRUE;
    int nColNo = 0;

	// does the ODBC driver support calling of SQLBindColumn for columns that are to the right
	// of columns that need to be retrieved by SQLGetData?
	// if so we can bind any fixed length column and only need to call
	// SQLGetData for non-fixed length columns
	// if no extension is supported, we need to call SQLGetData for each column
	// after the first column that doesn't support binding ..
	// all this is done mainly for performance reasons since binding columns is many times
	// faster than calling SQLGetData ..
	nApiRet = SQLGetInfo(pStmt->hConn,SQL_GETDATA_EXTENSIONS,&bGetDataExt,sizeof(SQLUINTEGER),0);
	if (nApiRet == SQL_SUCCESS)
		bGetDataExt &= SQL_GD_ANY_COLUMN;
	else
	{
		SafeODBCDbcError("SQLGetInfo", pStmt->hConn);
		throw E_APIERROR;
	}

	while (nColNo++ < nNoOfCols)
	{

	lpSC->nColNo = nColNo;
	lpSC->hStmt = pStmt->hStmt;

	switch (lpSC->nSQLType)
	{
		// character data
		case SQL_CHAR:
			lpSC->vData.SetString();
			lpSC->nCType = SQL_C_CHAR;
			if (lpSC->bCustomSchema)
			{
				if (lpSC->aVFPType == 'M' || lpSC->aVFPType == 'W')
				{
					BindGetDataBuffer(lpSC, pGetDataBuffer);
					bBindCol = bGetDataExt;
					if (!lpSC->lField.IsVariableRef())
						lpSC->pStore = SQLStoreMemoChar;
					else
						lpSC->pStore = SQLStoreMemoCharVar;
				}
				else
				{
					AllocateColumnBuffer(lpSC, lpSC->nSize + 1, bBindCol, pGetDataBuffer);
					if (!lpSC->lField.IsVariableRef())
						lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
			}
			else
			{
				if (lpSC->nSize <= VFP2C_VFP_MAX_CHARCOLUMN)
				{
					AllocateColumnBuffer(lpSC, lpSC->nSize + 1, bBindCol, pGetDataBuffer);
					lpSC->aVFPType = 'C';
					if (!lpSC->lField.IsVariableRef())
						lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
				else
				{
					BindGetDataBuffer(lpSC, pGetDataBuffer);
					bBindCol = bGetDataExt;
					lpSC->aVFPType = 'M';
					if (!lpSC->lField.IsVariableRef())
						lpSC->pStore = SQLStoreMemoChar;
					else
						lpSC->pStore = SQLStoreMemoCharVar;
				}
			}
			break;

		case SQL_BINARY:
			lpSC->vData.SetString();
			lpSC->nCType = SQL_C_BINARY;
			if (lpSC->bCustomSchema)
			{
				if (lpSC->aVFPType == 'M' || lpSC->aVFPType == 'W')
				{
					BindGetDataBuffer(lpSC, pGetDataBuffer);
					bBindCol = bGetDataExt;
					if (!lpSC->lField.IsVariableRef())
						lpSC->pStore = SQLStoreMemoBinary;
					else
						lpSC->pStore = SQLStoreMemoBinaryVar;
				}
				else
				{
					AllocateColumnBuffer(lpSC, lpSC->nSize, bBindCol, pGetDataBuffer);
					if (!lpSC->lField.IsVariableRef())
						lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
			}
			else
			{
				if (lpSC->nSize <= VFP2C_VFP_MAX_CHARCOLUMN)
				{
					AllocateColumnBuffer(lpSC, lpSC->nSize, bBindCol, pGetDataBuffer);
					if (CFoxVersion::MajorVersion() >= 9)
						lpSC->aVFPType = 'Q';
					else
					{
						lpSC->aVFPType = 'C';
						lpSC->bBinary = TRUE;
					}
					if (!lpSC->lField.IsVariableRef())
						lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
				else
				{
					BindGetDataBuffer(lpSC, pGetDataBuffer);
					bBindCol = bGetDataExt;
					if (CFoxVersion::MajorVersion() >= 9)
						lpSC->aVFPType = 'W';
					else
					{
						lpSC->aVFPType = 'M';
						lpSC->bBinary = TRUE;
					}
					if (!lpSC->lField.IsVariableRef())
						lpSC->pStore = SQLStoreMemoBinary;
					else
						lpSC->pStore = SQLStoreMemoBinaryVar;
				}
			}
			break;

		case SQL_VARCHAR:
			lpSC->vData.SetString();
			lpSC->nCType = SQL_C_CHAR;
			if (lpSC->bCustomSchema)
			{
				if (lpSC->aVFPType == 'M' || lpSC->aVFPType == 'W')
				{
					BindGetDataBuffer(lpSC, pGetDataBuffer);
					bBindCol = bGetDataExt;
					if (!lpSC->lField.IsVariableRef())
						lpSC->pStore = SQLStoreMemoChar;
					else
						lpSC->pStore = SQLStoreMemoCharVar;
				}
				else
				{
					AllocateColumnBuffer(lpSC, lpSC->nSize + 1, bBindCol, pGetDataBuffer);
					if (!lpSC->lField.IsVariableRef())
						lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
			}
			else
			{
				if (lpSC->nSize <= VFP2C_VFP_MAX_CHARCOLUMN)
				{
					AllocateColumnBuffer(lpSC, lpSC->nSize + 1, bBindCol, pGetDataBuffer);
					if (CFoxVersion::MajorVersion() >= 9)
						lpSC->aVFPType = 'V';
					else
						lpSC->aVFPType = 'C';
					if (!lpSC->lField.IsVariableRef())
						lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
				else
				{
					BindGetDataBuffer(lpSC, pGetDataBuffer);
					lpSC->aVFPType = 'M';
					if (!lpSC->lField.IsVariableRef())
						lpSC->pStore = SQLStoreMemoChar;
					else
						lpSC->pStore = SQLStoreMemoCharVar;
					bBindCol = bGetDataExt;					
				}
			}
			break;

		case SQL_VARBINARY:
			lpSC->vData.SetString();
			lpSC->nCType = SQL_C_BINARY;
			if (lpSC->bCustomSchema)
			{
				if (lpSC->aVFPType == 'M' || lpSC->aVFPType == 'W')
				{
					BindGetDataBuffer(lpSC, pGetDataBuffer);
					bBindCol = bGetDataExt;
					if (!lpSC->lField.IsVariableRef())
						lpSC->pStore = SQLStoreMemoBinary;
					else
						lpSC->pStore = SQLStoreMemoBinaryVar;
				}
				else
				{
					AllocateColumnBuffer(lpSC, lpSC->nSize + 1, bBindCol, pGetDataBuffer);
					if (!lpSC->lField.IsVariableRef())
						lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
			}
			else
			{
				if (lpSC->nSize <= VFP2C_VFP_MAX_CHARCOLUMN)
				{
					AllocateColumnBuffer(lpSC, lpSC->nSize, bBindCol, pGetDataBuffer);
					lpSC->aVFPType = 'C';
					lpSC->bBinary = TRUE;
					if (!lpSC->lField.IsVariableRef())
						lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
				else
				{
					BindGetDataBuffer(lpSC, pGetDataBuffer);
					bBindCol = bGetDataExt;
					if (CFoxVersion::MajorVersion() >= 9)
						lpSC->aVFPType = 'W';
					else
					{
						lpSC->aVFPType = 'M';
						lpSC->bBinary = TRUE;
					}
					if (!lpSC->lField.IsVariableRef())
						lpSC->pStore = SQLStoreMemoBinary;
					else
						lpSC->pStore = SQLStoreMemoBinaryVar;
				}
			}
			break;

		case SQL_LONGVARCHAR:
			lpSC->vData.SetString();
			lpSC->nCType = SQL_C_CHAR;
			if (lpSC->bCustomSchema)
			{
				if (lpSC->aVFPType == 'M' || lpSC->aVFPType == 'W')
				{
					BindGetDataBuffer(lpSC, pGetDataBuffer);
					bBindCol = bGetDataExt;
					if (!lpSC->lField.IsVariableRef())
						lpSC->pStore = SQLStoreMemoChar;
					else
						lpSC->pStore = SQLStoreMemoCharVar;
				}
				else
				{
					BindGetDataBuffer(lpSC, pGetDataBuffer);
					bBindCol = bGetDataExt;
					if (!lpSC->lField.IsVariableRef())
						lpSC->pStore = SQLStoreCharByGetData;
					else
						lpSC->pStore = SQLStoreCharByGetDataVar;
				}
			}
			else
			{
				BindGetDataBuffer(lpSC, pGetDataBuffer);
				bBindCol = bGetDataExt;
				lpSC->aVFPType = 'M';
				if (!lpSC->lField.IsVariableRef())
					lpSC->pStore = SQLStoreMemoChar;
				else
					lpSC->pStore = SQLStoreMemoCharVar;
			}
			break;

		case SQL_LONGVARBINARY:
			lpSC->vData.SetString();
			lpSC->nCType = SQL_C_BINARY;
			if (lpSC->bCustomSchema)
			{
				if (lpSC->aVFPType == 'M' || lpSC->aVFPType == 'W')
				{
					BindGetDataBuffer(lpSC, pGetDataBuffer);
					bBindCol = bGetDataExt;
					if (!lpSC->lField.IsVariableRef())
						lpSC->pStore = SQLStoreMemoBinary;
					else
						lpSC->pStore = SQLStoreMemoBinaryVar;
				}
				else
				{
					AllocateColumnBuffer(lpSC, lpSC->nSize, bBindCol, pGetDataBuffer);
					if (!lpSC->lField.IsVariableRef())
						lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
			}
			else
			{
				BindGetDataBuffer(lpSC, pGetDataBuffer);
				bBindCol = bGetDataExt;
				if (CFoxVersion::MajorVersion() >= 9)
					lpSC->aVFPType = 'W';
				else
				{
					lpSC->aVFPType = 'M';
					lpSC->bBinary = TRUE;
				}
				if (!lpSC->lField.IsVariableRef())
					lpSC->pStore = SQLStoreMemoBinary;
				else
					lpSC->pStore = SQLStoreMemoBinaryVar;
			}
			break;

		case SQL_WCHAR:
			lpSC->vData.SetString();
			if (lpSC->bCustomSchema)
			{
				if (lpSC->aVFPType == 'M' || lpSC->aVFPType == 'W')
				{
					BindGetDataBuffer(lpSC, pGetDataBuffer);
					bBindCol = bGetDataExt;
					if (lpSC->aVFPType == 'W' || lpSC->bBinary)
					{
						lpSC->nCType = SQL_C_WCHAR;
						if (!lpSC->lField.IsVariableRef())
							lpSC->pStore = SQLStoreMemoWChar;
						else
							lpSC->pStore = SQLStoreMemoWCharVar;
					}
					else
					{
						lpSC->nCType = SQL_C_CHAR;
						if (!lpSC->lField.IsVariableRef())
							lpSC->pStore = SQLStoreMemoChar;
						else
							lpSC->pStore = SQLStoreMemoCharVar;
					}
				}
				else
				{
					AllocateColumnBuffer(lpSC, lpSC->nSize + 2, bBindCol, pGetDataBuffer);
					if (lpSC->aVFPType == 'Q' || lpSC->bBinary)
						lpSC->nCType = SQL_C_WCHAR;
					else
						lpSC->nCType = SQL_C_CHAR;
					if (!lpSC->lField.IsVariableRef())
						lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
			}
			else
			{
				if (lpSC->nSize <= VFP2C_VFP_MAX_CHARCOLUMN)
				{
					AllocateColumnBuffer(lpSC, lpSC->nSize + 1, bBindCol, pGetDataBuffer);
					lpSC->nCType = bUnicodeConversion ? SQL_C_CHAR : SQL_C_WCHAR;
					lpSC->aVFPType = 'C';
					if (!lpSC->lField.IsVariableRef())
						lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
				else
				{
					BindGetDataBuffer(lpSC, pGetDataBuffer);
					bBindCol = bGetDataExt;
					lpSC->nCType = bUnicodeConversion ? SQL_C_CHAR : SQL_C_WCHAR;
					lpSC->aVFPType = 'M';
					if (!lpSC->lField.IsVariableRef())
						lpSC->pStore = bUnicodeConversion ? SQLStoreMemoChar : SQLStoreMemoWChar;
					else
						lpSC->pStore = bUnicodeConversion ? SQLStoreMemoCharVar : SQLStoreMemoWCharVar;
				}
			}
			break;

		case SQL_WVARCHAR:
			lpSC->vData.SetString();
			if (lpSC->bCustomSchema)
			{
				if (lpSC->aVFPType == 'M' || lpSC->aVFPType == 'W')
				{
					BindGetDataBuffer(lpSC, pGetDataBuffer);
					bBindCol = bGetDataExt;
					if (lpSC->aVFPType == 'W' || lpSC->bBinary)
					{
						lpSC->nCType = SQL_C_WCHAR;
						if (!lpSC->lField.IsVariableRef())
							lpSC->pStore = SQLStoreMemoWChar;
						else
							lpSC->pStore = SQLStoreMemoWCharVar;
					}
					else
					{
						lpSC->nCType = SQL_C_CHAR;
						if (!lpSC->lField.IsVariableRef())
							lpSC->pStore = SQLStoreMemoChar;
						else
							lpSC->pStore = SQLStoreMemoCharVar;
					}
				}
				else
				{
					AllocateColumnBuffer(lpSC, lpSC->nSize + 2, bBindCol, pGetDataBuffer);
					if (lpSC->aVFPType == 'Q' || lpSC->bBinary)
						lpSC->nCType = SQL_C_WCHAR;
					else
						lpSC->nCType = SQL_C_CHAR;
					if (!lpSC->lField.IsVariableRef())
						lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
			}
			else
			{
				if (lpSC->nSize <= VFP2C_VFP_MAX_CHARCOLUMN)
				{
					AllocateColumnBuffer(lpSC, lpSC->nSize + 1, bBindCol, pGetDataBuffer);
					lpSC->nCType = bUnicodeConversion ? SQL_C_CHAR : SQL_C_WCHAR;
					lpSC->aVFPType = 'C';
					if (!lpSC->lField.IsVariableRef())
						lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
				else
				{
					BindGetDataBuffer(lpSC, pGetDataBuffer);
					bBindCol = bGetDataExt;
					lpSC->nCType = bUnicodeConversion ? SQL_C_CHAR : SQL_C_WCHAR;
					lpSC->aVFPType = 'M';
					if (bUnicodeConversion)
					{
						if (!lpSC->lField.IsVariableRef())
							lpSC->pStore = SQLStoreMemoChar;
						else
							lpSC->pStore = SQLStoreMemoCharVar;
					}
					else
					{
						if (!lpSC->lField.IsVariableRef())
							lpSC->pStore = SQLStoreMemoWChar;
						else
							lpSC->pStore = SQLStoreMemoWCharVar;
					}
				}
			}
			break;

		case SQL_WLONGVARCHAR:
			lpSC->vData.SetString();
			if (lpSC->bCustomSchema)
			{
				if (lpSC->aVFPType == 'M' || lpSC->aVFPType == 'W')
				{
					BindGetDataBuffer(lpSC, pGetDataBuffer);
					bBindCol = bGetDataExt;
					if (lpSC->aVFPType == 'W' || lpSC->bBinary)
					{
						lpSC->nCType = SQL_C_WCHAR;
						if (!lpSC->lField.IsVariableRef())
							lpSC->pStore = SQLStoreMemoWChar;
						else
							lpSC->pStore = SQLStoreMemoWCharVar;
					}
					else
					{
						lpSC->nCType = SQL_C_CHAR;
						if (!lpSC->lField.IsVariableRef())
							lpSC->pStore = SQLStoreMemoChar;
						else
							lpSC->pStore = SQLStoreMemoCharVar;
					}
				}
				else
				{
					AllocateColumnBuffer(lpSC, lpSC->nSize + 2, bBindCol, pGetDataBuffer);
					if (lpSC->aVFPType == 'Q' || lpSC->bBinary)
						lpSC->nCType = SQL_C_WCHAR;
					else
						lpSC->nCType = SQL_C_CHAR;
					if (!lpSC->lField.IsVariableRef())
						lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
			}
			else
			{
				BindGetDataBuffer(lpSC, pGetDataBuffer);
				bBindCol = bGetDataExt;
				lpSC->nCType = bUnicodeConversion ? SQL_C_CHAR : SQL_C_WCHAR;
				lpSC->aVFPType = 'M';
				if (bUnicodeConversion)
				{
					if (!lpSC->lField.IsVariableRef())
						lpSC->pStore = SQLStoreMemoChar;
					else
						lpSC->pStore = SQLStoreMemoCharVar;
				}
				else
				{
					if (!lpSC->lField.IsVariableRef())
						lpSC->pStore = SQLStoreMemoWChar;
					else
						lpSC->pStore = SQLStoreMemoWCharVar;
				}
			}
			break;

		// bit (logical) data
		case SQL_BIT:
			if (lpSC->bCustomSchema)
			{
				if (lpSC->aVFPType == 'C' || lpSC->aVFPType == 'V')
				{
					lpSC->vData.SetString();
					AllocateColumnBuffer(lpSC, lpSC->nSize + 1, bBindCol, pGetDataBuffer);
					lpSC->nCType = SQL_C_CHAR;
					if (!lpSC->lField.IsVariableRef())
						lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
					break;
				}
			}

			lpSC->vData.SetLogical();
			lpSC->pData = &lpSC->vData.ev_length;
			lpSC->nCType = SQL_C_ULONG;
			lpSC->aVFPType = 'L';
			lpSC->bBindColumn = bBindCol;
			if (!lpSC->lField.IsVariableRef())
				lpSC->pStore = bBindCol ? SQLStoreByBinding : SQLStoreByGetData;
			else
				lpSC->pStore = bBindCol ? SQLStoreByBindingVar : SQLStoreByGetDataVar;
			break;

		// integral numeric types
		case SQL_TINYINT:
		case SQL_SMALLINT:
		case SQL_INTEGER:
			if (lpSC->bCustomSchema)
			{
				if (lpSC->aVFPType == 'C' || lpSC->aVFPType == 'V' || lpSC->aVFPType == 'Q')
				{
					lpSC->vData.SetString();
					AllocateColumnBuffer(lpSC, lpSC->nSize + 1, bBindCol, pGetDataBuffer);
					lpSC->nCType = SQL_C_CHAR;
					if (!lpSC->lField.IsVariableRef())
						lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
				else if (lpSC->aVFPType == 'N' || lpSC->aVFPType == 'F' || lpSC->aVFPType == 'B')
				{
					lpSC->vData.SetNumeric(lpSC->nSize,lpSC->nScale);
					lpSC->pData = &lpSC->vData.ev_real;
					lpSC->nCType = SQL_C_DOUBLE;
					if (!lpSC->lField.IsVariableRef())
						lpSC->pStore = bBindCol ? SQLStoreByBinding : SQLStoreByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreByBindingVar : SQLStoreByGetDataVar;
				}
				else if (lpSC->aVFPType == 'I')
				{
					lpSC->vData.SetInt();
					lpSC->pData = &lpSC->vData.ev_long;
					lpSC->nCType = SQL_C_SLONG;
					if (!lpSC->lField.IsVariableRef())
						lpSC->pStore = bBindCol ? SQLStoreByBinding : SQLStoreByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreByBindingVar : SQLStoreByGetDataVar;
				}
				if (lpSC->aVFPType == 'L')
				{
					lpSC->vData.SetLogical();
					lpSC->pData = &lpSC->vData.ev_length;
					lpSC->nCType = SQL_C_SLONG;
					if (!lpSC->lField.IsVariableRef())
						lpSC->pStore = bBindCol ? SQLStoreByBinding : SQLStoreByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreByBindingVar : SQLStoreByGetDataVar;
				}
				lpSC->bBindColumn = bBindCol;
			}
			else
			{
				if (lpSC->nSQLType == SQL_INTEGER && lpSC->bUnsigned)
				{
					lpSC->vData.SetUInt();
					lpSC->nCType = SQL_C_DOUBLE;
					lpSC->pData = &lpSC->vData.ev_real;
					lpSC->aVFPType = 'N';
					lpSC->nSize = 10;
					lpSC->nScale = 0;
				}
				else
				{
					lpSC->vData.SetInt();
					lpSC->nCType = SQL_C_SLONG;
					lpSC->pData = &lpSC->vData.ev_long;
					lpSC->aVFPType = 'I';
				}
				lpSC->bBindColumn = bBindCol;
				if (!lpSC->lField.IsVariableRef())
					lpSC->pStore = bBindCol ? SQLStoreByBinding : SQLStoreByGetData;
				else
					lpSC->pStore = bBindCol ? SQLStoreByBindingVar : SQLStoreByGetDataVar;
			}
			break;
		
		case SQL_BIGINT:
			if (lpSC->bCustomSchema)
			{
				if (lpSC->aVFPType == 'C' || lpSC->aVFPType == 'V' || lpSC->aVFPType == 'Q')
				{
					lpSC->vData.SetString();
					AllocateColumnBuffer(lpSC, VFP2C_ODBC_MAX_BIGINT_LITERAL + 1, bBindCol, pGetDataBuffer);
					lpSC->nCType = SQL_C_CHAR;
					if (!lpSC->lField.IsVariableRef())
						lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
			}
			else
			{
				lpSC->vData.SetString();
				AllocateColumnBuffer(lpSC, VFP2C_ODBC_MAX_BIGINT_LITERAL + 1, bBindCol, pGetDataBuffer);
				lpSC->nSize = VFP2C_ODBC_MAX_BIGINT_LITERAL;
				lpSC->aVFPType = 'C';
				lpSC->nCType = SQL_C_CHAR;
				if (!lpSC->lField.IsVariableRef())
					lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
				else
					lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
			}
			break;
		
		case SQL_NUMERIC:
		case SQL_DECIMAL:
			if (lpSC->bCustomSchema)
			{
					if (lpSC->aVFPType == 'C' || lpSC->aVFPType == 'V')
					{
						lpSC->vData.SetString();
						AllocateColumnBuffer(lpSC, lpSC->nSize + 1, bBindCol, pGetDataBuffer);
						lpSC->nCType = SQL_C_CHAR;
						if (!lpSC->lField.IsVariableRef())
							lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
						else
							lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
					}
					else if (lpSC->aVFPType == 'I')
					{
						lpSC->vData.SetInt();
						lpSC->pData = &lpSC->vData.ev_long;
						lpSC->nCType = SQL_C_LONG;
						lpSC->bBindColumn = bBindCol;
						if (!lpSC->lField.IsVariableRef())
							lpSC->pStore = bBindCol ? SQLStoreByBinding : SQLStoreByGetData;
						else
							lpSC->pStore = bBindCol ? SQLStoreByBindingVar : SQLStoreByGetDataVar;
					}
					else if (lpSC->aVFPType == 'N' || lpSC->aVFPType == 'B' || lpSC->aVFPType == 'F')
					{
						lpSC->vData.SetNumeric(lpSC->aVFPType == 'N' ? (short)lpSC->nSize : 20, lpSC->nScale);
						lpSC->pData = &lpSC->vData.ev_real;
						lpSC->nCType = SQL_DOUBLE;
						lpSC->bBindColumn = bBindCol;
						if (!lpSC->lField.IsVariableRef())
							lpSC->pStore = bBindCol ? SQLStoreByBinding : SQLStoreByGetData;
						else
							lpSC->pStore = bBindCol ? SQLStoreByBindingVar : SQLStoreByGetDataVar;
					}
					else if (lpSC->aVFPType == 'Y')
					{
						lpSC->vData.SetCurrency();
						lpSC->pData = lpSC->aNumeric;
						lpSC->nSize = VFP2C_VFP_CURRENCY_PRECISION;
						lpSC->nScale = VFP2C_VFP_CURRENCY_SCALE;
						lpSC->nBufferSize = VFP2C_ODBC_MAX_CURRENCY_LITERAL;
						lpSC->nCType = SQL_C_CHAR;
						lpSC->aVFPType = 'Y';
						lpSC->bBindColumn = bBindCol;
						if (!lpSC->lField.IsVariableRef())
							lpSC->pStore = bBindCol ? SQLStoreCurrencyByBinding : SQLStoreCurrencyByGetData;
						else
							lpSC->pStore = bBindCol ? SQLStoreCurrencyByBindingVar : SQLStoreCurrencyByGetDataVar;
					}
			}
			else
			{
				if (!lpSC->bMoney)
				{
					lpSC->vData.SetNumeric(lpSC->nSize, lpSC->nScale);
					lpSC->pData = &lpSC->vData.ev_real;
					lpSC->nCType = SQL_C_DOUBLE;
					lpSC->aVFPType = 'N';
					lpSC->bBindColumn = bBindCol;
					if (!lpSC->lField.IsVariableRef())
						lpSC->pStore = bBindCol ? SQLStoreByBinding : SQLStoreByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreByBindingVar : SQLStoreByGetDataVar;
				}
				else
				{
					lpSC->vData.SetCurrency();
					lpSC->pData = lpSC->aNumeric;
					lpSC->nSize = VFP2C_VFP_CURRENCY_PRECISION;
					lpSC->nScale = VFP2C_VFP_CURRENCY_SCALE;
					lpSC->nBufferSize = VFP2C_ODBC_MAX_CURRENCY_LITERAL;
					lpSC->nCType = SQL_C_CHAR;
					lpSC->aVFPType = 'Y';
					lpSC->bBindColumn = bBindCol;
					if (!lpSC->lField.IsVariableRef())
						lpSC->pStore = bBindCol ? SQLStoreCurrencyByBinding : SQLStoreCurrencyByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreCurrencyByBindingVar : SQLStoreCurrencyByGetDataVar;
				}
			}
			break;

		case SQL_REAL:
		case SQL_FLOAT:
		case SQL_DOUBLE:
			if (lpSC->bCustomSchema)
			{
				if (lpSC->aVFPType == 'C' || lpSC->aVFPType == 'V' || lpSC->aVFPType == 'Q')
				{
					lpSC->vData.SetString();
					AllocateColumnBuffer(lpSC, lpSC->nSize + 1, bBindCol, pGetDataBuffer);
					lpSC->nCType = SQL_C_CHAR;
					if (!lpSC->lField.IsVariableRef())
						lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
				else if (lpSC->aVFPType == 'N' || lpSC->aVFPType == 'B' || lpSC->aVFPType == 'F')
				{
					lpSC->vData.SetNumeric(lpSC->nSize, lpSC->nScale);
					lpSC->pData = &lpSC->vData.ev_real;
					lpSC->nCType = SQL_C_DOUBLE;
					lpSC->bBindColumn = bBindCol;
					if (!lpSC->lField.IsVariableRef())
						lpSC->pStore = bBindCol ? SQLStoreByBinding : SQLStoreByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreByBindingVar : SQLStoreByGetDataVar;
				}
			}
			else
			{
				lpSC->vData.SetDouble();
				lpSC->pData = &lpSC->vData.ev_real;
				lpSC->nCType = SQL_C_DOUBLE;
				lpSC->aVFPType = 'B';
				lpSC->bBindColumn = bBindCol;
				if (!lpSC->lField.IsVariableRef())
					lpSC->pStore = bBindCol ? SQLStoreByBinding : SQLStoreByGetData;
				else
					lpSC->pStore = bBindCol ? SQLStoreByBindingVar : SQLStoreByGetDataVar;
			}
			break;

		// this is only a date not a datetime ..	
		case SQL_DATE:
			if (lpSC->bCustomSchema)
			{
				if ((lpSC->aVFPType == 'C' || lpSC->aVFPType == 'V') && !lpSC->bBinary)
				{
					lpSC->vData.SetString();
					AllocateColumnBuffer(lpSC, lpSC->nSize + 1, bBindCol, pGetDataBuffer);
					lpSC->nCType = SQL_C_CHAR;
					if (!lpSC->lField.IsVariableRef())
						lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
				else if (lpSC->aVFPType == 'Q' ||
					((lpSC->aVFPType == 'C' || lpSC->aVFPType == 'V') && lpSC->bBinary))
				{
					lpSC->vData.SetString();
					AllocateColumnBuffer(lpSC, sizeof(SQL_TIMESTAMP_STRUCT), bBindCol, pGetDataBuffer);
					lpSC->nCType = SQL_C_TIMESTAMP;
					if (!lpSC->lField.IsVariableRef())
						lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
				else if (lpSC->aVFPType == 'T' || lpSC->aVFPType == 'D')
				{
					if (lpSC->aVFPType == 'T')
						lpSC->vData.SetDateTime();
					else
						lpSC->vData.SetDate();
					lpSC->pData = &lpSC->sDateTime;
					lpSC->nCType = SQL_DATE;
					if (!lpSC->lField.IsVariableRef())
						lpSC->pStore = bBindCol ? SQLStoreDateByBinding : SQLStoreDateByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreDateByBindingVar : SQLStoreDateByGetDataVar;
				}
				lpSC->bBindColumn = bBindCol;
			}
			else
			{
				lpSC->vData.SetDate();
				lpSC->pData = &lpSC->sDateTime;
				lpSC->nCType = SQL_C_DATE;
				lpSC->aVFPType = 'D';
				lpSC->bBindColumn = bBindCol;
				if (!lpSC->lField.IsVariableRef())
					lpSC->pStore = bBindCol ? SQLStoreDateByBinding : SQLStoreDateByGetData;
				else
					lpSC->pStore = bBindCol ? SQLStoreDateByBindingVar : SQLStoreDateByGetDataVar;
			}
			break;
		
		// this is a datetime
		case SQL_TIMESTAMP:
			if (lpSC->bCustomSchema)
			{
				if (lpSC->aVFPType == 'D')
				{
					lpSC->vData.SetDate();
					lpSC->pData = &lpSC->sDateTime;
					lpSC->nCType = SQL_C_TIMESTAMP;
					if (!lpSC->lField.IsVariableRef())
						lpSC->pStore = bBindCol ? SQLStoreDateByBinding : SQLStoreDateByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreDateByBindingVar : SQLStoreDateByGetDataVar;
				}
				else if (lpSC->aVFPType == 'T')
				{
					lpSC->vData.SetDateTime();
					lpSC->pData = &lpSC->sDateTime;
					lpSC->nCType = SQL_C_TIMESTAMP;
					if (!lpSC->lField.IsVariableRef())
						lpSC->pStore = bBindCol ? SQLStoreDateTimeByBinding : SQLStoreDateTimeByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreDateTimeByBindingVar : SQLStoreDateTimeByGetDataVar;
				}
				else if (lpSC->aVFPType == 'Q' || 
					((lpSC->aVFPType == 'C' || lpSC->aVFPType == 'V') && lpSC->bBinary))
				{
					lpSC->vData.SetString();
					AllocateColumnBuffer(lpSC, sizeof(SQL_TIMESTAMP_STRUCT), bBindCol, pGetDataBuffer);
					lpSC->nSize = sizeof(SQL_TIMESTAMP_STRUCT);
					lpSC->nCType = SQL_C_TIMESTAMP;
					if (!lpSC->lField.IsVariableRef())
						lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
				else if (lpSC->aVFPType == 'C' || lpSC->aVFPType == 'V')
				{
					lpSC->vData.SetString();
					AllocateColumnBuffer(lpSC, lpSC->nSize + 1, bBindCol, pGetDataBuffer);
					lpSC->nCType = SQL_C_CHAR;
					if (!lpSC->lField.IsVariableRef())
						lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
				lpSC->bBindColumn = bBindCol;
			}
			else
			{
				lpSC->vData.SetDateTime();
				lpSC->pData = &lpSC->sDateTime;
				lpSC->nCType = SQL_C_TIMESTAMP;
				lpSC->aVFPType = 'T';
				lpSC->bBindColumn = bBindCol;
				if (!lpSC->lField.IsVariableRef())
					lpSC->pStore = bBindCol ? SQLStoreDateTimeByBinding : SQLStoreDateTimeByGetData;
				else
					lpSC->pStore = bBindCol ? SQLStoreDateTimeByBindingVar : SQLStoreDateTimeByGetDataVar;
			}
			break;

		case SQL_TIME:
			lpSC->vData.SetString();
			AllocateColumnBuffer(lpSC, lpSC->nSize + 1, bBindCol, pGetDataBuffer);
			lpSC->nCType = SQL_C_CHAR;
			lpSC->aVFPType = 'C';
			if (!lpSC->lField.IsVariableRef())
				lpSC->pStore = bBindCol ? SQLStoreByBinding : SQLStoreByGetData;
			else
				lpSC->pStore = bBindCol ? SQLStoreByBindingVar : SQLStoreByGetDataVar;
			break;

		// GUID
		case SQL_GUID:
			if (lpSC->bCustomSchema)
			{
				if (lpSC->aVFPType == 'Q')
				{
					lpSC->vData.SetString();
					AllocateColumnBuffer(lpSC, lpSC->nSize + 1, bBindCol, pGetDataBuffer);
					lpSC->vData.ev_length = lpSC->nSize;
					lpSC->nCType = SQL_C_GUID;
					lpSC->aVFPType = 'C';
					if (!lpSC->lField.IsVariableRef())
						lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
					break;
				}
			}

			lpSC->vData.SetString();
			AllocateColumnBuffer(lpSC, VFP2C_ODBC_MAX_GUID_LITERAL + 1, bBindCol, pGetDataBuffer);
			lpSC->vData.ev_length = VFP2C_ODBC_MAX_GUID_LITERAL;
			lpSC->nCType = SQL_C_CHAR;
			lpSC->aVFPType = 'C';
			if (!lpSC->lField.IsVariableRef())
				lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
			else
				lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
			break;
		
		default:
			if (lpSC->bCustomSchema)
			{
				if (lpSC->aVFPType == 'C' || lpSC->aVFPType == 'V' || lpSC->aVFPType == 'Q')
				{
					lpSC->vData.SetString();
					BindGetDataBuffer(lpSC, pGetDataBuffer);
					bBindCol = bGetDataExt;
					lpSC->nCType = SQL_C_CHAR;
					if (!lpSC->lField.IsVariableRef())
						lpSC->pStore = SQLStoreCharByGetData;
					else
						lpSC->pStore = SQLStoreCharByGetDataVar;
				}
				else if (lpSC->aVFPType == 'M' || lpSC->aVFPType == 'W')
				{
					lpSC->vData.SetString();
					BindGetDataBuffer(lpSC, pGetDataBuffer);
					bBindCol = bGetDataExt;
					if (lpSC->bBinary || lpSC->aVFPType == 'W')
					{
						lpSC->nCType = SQL_C_BINARY;
						if (!lpSC->lField.IsVariableRef())
							lpSC->pStore = SQLStoreMemoBinary;
						else
							lpSC->pStore = SQLStoreMemoBinaryVar;
					}
					else
					{
						lpSC->nCType = SQL_C_CHAR;
						if (!lpSC->lField.IsVariableRef())
							lpSC->pStore = SQLStoreMemoChar;
						else
							lpSC->pStore = SQLStoreMemoCharVar;
					}
				}
				else if (lpSC->aVFPType == 'I')
				{
					lpSC->vData.SetInt();
					lpSC->pData = &lpSC->vData.ev_long;
					lpSC->nCType = SQL_C_SLONG;
					lpSC->bBindColumn = bBindCol;
					if (!lpSC->lField.IsVariableRef())
						lpSC->pStore = bBindCol ? SQLStoreByBinding : SQLStoreByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreByBindingVar : SQLStoreByGetDataVar;
				}
				else if (lpSC->aVFPType == 'B' || lpSC->aVFPType == 'N' || lpSC->aVFPType == 'F')
				{
					lpSC->vData.SetNumeric(lpSC->nSize, lpSC->nScale);
					lpSC->pData = &lpSC->vData.ev_real;
					lpSC->nCType = SQL_C_DOUBLE;
					lpSC->bBindColumn = bBindCol;
					if (!lpSC->lField.IsVariableRef())
						lpSC->pStore = bBindCol ? SQLStoreByBinding : SQLStoreByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreByBindingVar : SQLStoreByGetDataVar;
				}
				else if (lpSC->aVFPType == 'Y')
				{
					lpSC->vData.SetCurrency();
					lpSC->pData = lpSC->aNumeric;
					lpSC->nSize = VFP2C_VFP_CURRENCY_PRECISION;
					lpSC->nScale = VFP2C_VFP_CURRENCY_SCALE;
					lpSC->nBufferSize = VFP2C_ODBC_MAX_CURRENCY_LITERAL;
					lpSC->nCType = SQL_C_CHAR;
					lpSC->bBindColumn = bBindCol;
					if (!lpSC->lField.IsVariableRef())
						lpSC->pStore = bBindCol ? SQLStoreCurrencyByBinding : SQLStoreCurrencyByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreCurrencyByBindingVar : SQLStoreCurrencyByGetDataVar;
				}
			}
			else
			{
				if (lpSC->nDisplaySize > VFP2C_VFP_MAX_CHARCOLUMN)
				{
					lpSC->vData.SetString();
					BindGetDataBuffer(lpSC, pGetDataBuffer);
					bBindCol = bGetDataExt;
					lpSC->nCType = SQL_C_CHAR;
					lpSC->aVFPType = 'M';
					if (!lpSC->lField.IsVariableRef())
						lpSC->pStore = SQLStoreMemoChar;
					else
						lpSC->pStore = SQLStoreMemoCharVar;
				}
				else
				{
					lpSC->vData.SetString();
					AllocateColumnBuffer(lpSC, lpSC->nDisplaySize + 1, bBindCol, pGetDataBuffer);
					lpSC->nSize = lpSC->nDisplaySize;
					lpSC->nCType = SQL_C_CHAR;
					lpSC->aVFPType = 'C';
					if (!lpSC->lField.IsVariableRef())
						lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
			}
	} // switch(lpSC-nType)

	lpSC++;

	} // while(nNumberOfCols--)
}

void SqlResultSet::BindGetDataBuffer(SqlColumn* lpSC, FoxString* pGetDataBuffer)
{
	pGetDataBuffer->Size(VFP2C_ODBC_MAX_BUFFER);
	lpSC->vData.ev_handle = pGetDataBuffer->GetHandle();
	lpSC->pData = pGetDataBuffer->Ptr<SQLPOINTER>();
	lpSC->nBufferSize = VFP2C_ODBC_MAX_BUFFER;
	lpSC->bBindColumn = FALSE;
}

void SqlResultSet::AllocateColumnBuffer(SqlColumn* lpSC, int nLen, BOOL bBindColumn, FoxString *pGetDataBuffer)
{
	if (bBindColumn)
	{
		if (!lpSC->vData.AllocHandle(nLen))
			throw E_INSUFMEMORY;
		lpSC->vData.LockHandle();
		lpSC->pData = lpSC->vData.HandleToPtr();
		lpSC->nBufferSize = nLen;
		lpSC->bBindColumn = bBindColumn;
	}
	else
	{
		BindGetDataBuffer(lpSC, pGetDataBuffer);
	}
}

void SqlResultSet::ParseCursorSchema()
{
	if (!pStmt->pCursorSchema.Len() || pStmt->bExecutedOnce)
		return;

	SqlColumn* lpSC;
	char *pSchema;
	char aColName[VFP2C_ODBC_MAX_COLUMN_NAME];

	pSchema = AtEx(pStmt->pCursorSchema,'|', pStmt->nResultset-1);

	if (pSchema)
	{
		do
		{
			// try to match an identifier (fieldname)
			if (!match_identifier(&pSchema,aColName,VFP2C_ODBC_MAX_COLUMN_NAME))
			{
				// if no normal identifier found, try to match a quoted (enclosed in ") identifier
				if (match_quoted_identifier(&pSchema,aColName,VFP2C_ODBC_MAX_COLUMN_NAME))
					FixColumnName(aColName);
				else
				{
					SaveCustomError("SQLExecEx","Cursorschema contained invalid data near '%10S'.", pSchema);
					throw E_APIERROR;
				}
			}

			// get pointer to SQLCOLUMNDATA struct for the specified column

			lpSC = FindColumn(aColName);
			if (!lpSC)
			{
				SaveCustomError("SQLExecEx","Column '%S' not contained in resultset but specified in cursorschema.", aColName);
				throw E_APIERROR;
			}

			// match the VFP datatype .. upper or lowercase ..
			if (!match_one_chr(&pSchema,"CcVvQqWwMmNnBbFfIiYyDdTtLl",(char*)&lpSC->aVFPType))
			{
				SaveCustomError("SQLExecEx", "Datatype in cursorschema for column '%S' is invalid.", lpSC->aColName);
				throw E_APIERROR;
			}

			// convert to uppercase so we don't have to deal with case anymore ..
			lpSC->aVFPType = ToUpper(lpSC->aVFPType);

			// check if column is convertable to specified type
			if (!SQLTypeConvertible(lpSC->nSQLType,lpSC->aVFPType))
			{
				SaveCustomError("SQLExecEx", "Datatype conversion for column '%S' to VFP type '%s' not supported.", aColName, lpSC->aVFPType);
				throw E_APIERROR;
			}

			if (lpSC->aVFPType == 'C' || lpSC->aVFPType == 'V' || lpSC->aVFPType == 'Q')
			{
				if (!match_chr(&pSchema,'('))
				{
					SaveCustomError("SQLExecEx", "Invalid cursorschema for column '%S', expected '('.", aColName);
					throw E_APIERROR;
				}
				if (!match_int(&pSchema,(int*)&lpSC->nSize))
				{
					SaveCustomError("SQLExecEx", "Invalid cursorschema for column '%S', expected column size.", aColName);
					throw E_APIERROR;
				}
				if (!match_chr(&pSchema,')'))
				{
					SaveCustomError("SQLExecEx", "Invalid cursorschema for column '%S', expected ')'.", aColName);
					throw E_APIERROR;
				}
				// set binary flag for varbinary type
				if (lpSC->aVFPType == 'Q')
					lpSC->bBinary = TRUE;
			}
			else if (lpSC->aVFPType == 'N' || lpSC->aVFPType == 'F')
			{
				if (match_chr(&pSchema,'('))
				{
					if (!match_int(&pSchema,(int*)&lpSC->nSize))
					{
						SaveCustomError("SQLExecEx", "Invalid cursorschema for column '%S', expected column precision.", aColName);
						throw E_APIERROR;
					}

					if (match_chr(&pSchema,','))
					{
						if (!match_short(&pSchema,&lpSC->nScale))
						{
							SaveCustomError("SQLExecEx","Invalid cursorschema for column '%S', expected column scale.", aColName);
							throw E_APIERROR;
						}
					}
					else
						lpSC->nScale = 0;

					if (!match_chr(&pSchema,')'))
					{
						SaveCustomError("SQLExecEx", "Invalid cursorschema for column '%S', expected ')'.", aColName);
						throw E_APIERROR;
					}
				}
				// set default size
				else
				{
					lpSC->nSize = 10;
					lpSC->nScale = 0;
				}
			}
			else if (lpSC->aVFPType == 'B')
			{
				if (match_chr(&pSchema,'('))
				{
					if (!match_short(&pSchema,&lpSC->nScale))
					{
						SaveCustomError("SQLExecEx", "Invalid cursorschema for column '%S', expected column size.", aColName);
						throw E_APIERROR;
					}
					if (!match_chr(&pSchema,')'))
					{
						SaveCustomError("SQLExecEx","Invalid cursorschema for column '%S', expected ')'.", aColName);
						throw E_APIERROR;
					}
					lpSC->nSize = VFP2C_VFP_DOUBLE_PRECISION;
				}
				else
				{
					lpSC->nSize = 8;
					lpSC->nScale = 2;
				}
			}
			// set binary flag for blob datatype
			else if (lpSC->aVFPType == 'W')
				lpSC->bBinary = TRUE;
			
			if (match_istr(&pSchema,"NULL"))
				lpSC->bNullable = TRUE;

			if (match_istr(&pSchema,"NOCPTRANS"))
				lpSC->bBinary = TRUE;

			lpSC->bCustomSchema = TRUE;

		} while (match_chr(&pSchema,','));	// do while there are new ',' 

		if (!match_chr(&pSchema,'\0')) // if not at end of string the cursor schema is invalid
		{
			SaveCustomError("SQLExecEx","Cursorschema contained invalid data near '%20S'.",pSchema);
			throw E_APIERROR;
		}
	}
}

void SqlResultSet::ParseCursorSchemaEx()
{
	if (pStmt->bExecutedOnce)
		return;

	SqlColumn* lpSC;
	LocatorEx lArrayLoc;
	ValueEx vValue;
	vValue = 0;
	char* pValue;
	int nFieldCount, nRow;
	CStrBuilder<VFP2C_MAX_FUNCTIONBUFFER> pArrayName;
	CStrBuilder<VFP2C_MAX_FUNCTIONBUFFER> pExeBuffer;
	CStringView pArrayView;
	CStringView pCursorView = pCursorName;

try {
	// create unique array name .. since pStmt is a dynamically allocated pointer it's value is
	// always unique .. so we can use it to build a variable name ..
	pArrayName.Format("__VFP2C_ODBC_ARRAY_%U", this);
	pArrayView = pArrayName;
	pExeBuffer.Format("AFIELDS(%V,'%V')", &pArrayView, &pCursorView);

	Execute(pExeBuffer);
	lArrayLoc.FindVar(pArrayName);
	lArrayLoc.ResetArray(2);
	nFieldCount = lArrayLoc.ALen(AL_SUBSCRIPT1);

	while (nFieldCount--)
	{
		nRow = ++lArrayLoc.l_sub1;

		// fieldname
		vValue = lArrayLoc(nRow, 1);

		if (!vValue.NullTerminate())
			throw E_INSUFMEMORY;
		pValue = vValue.HandleToPtr();

		lpSC = FindColumn(pValue);
		if (!lpSC)
		{
			vValue.FreeHandle();
			vValue = 0;
			SaveCustomError("SQLExecEx", "Column '%S' not contained in resultset but exists in cursor.", pValue);
			throw E_APIERROR;
		}
		vValue.FreeHandle();
		vValue = 0;

		// fieldtype
		vValue = lArrayLoc(nRow, 2);
		pValue = vValue.HandleToPtr();
		lpSC->aVFPType = *pValue;
		vValue.FreeHandle();
		vValue = 0;

		// precision/width
		vValue = lArrayLoc(nRow, 3);
		lpSC->nSize = (SQLUINTEGER)vValue.ev_long;

		// scale
		vValue = lArrayLoc(nRow, 4);
		lpSC->nScale = (SQLSMALLINT)vValue.ev_long;

		// binary
		vValue = lArrayLoc(nRow, 6);
		lpSC->bBinary = (SQLSMALLINT)vValue.ev_length;

		lpSC->bCustomSchema = TRUE;
	}

	pExeBuffer.Format("RELEASE %V", &pArrayView);
	Execute(pExeBuffer);
}
catch (int nError)
{
	pExeBuffer.Format("RELEASE %V", &pArrayView);
	_Execute(pExeBuffer);
	throw nError;
}
}

void SqlResultSet::BindVariableLocators()
{
	if (pStmt->bExecutedOnce)
		return;

	SqlColumn* lpSC = pColumnData;
	int nErrorNo, xj;
	char *pSchema;
	char aCursorOrVar[VFP2C_VFP_MAX_CURSOR_NAME];
	char aField[VFP2C_VFP_MAX_COLUMN_NAME];
	
	pSchema = AtEx(pStmt->pCursorNames,'|', pStmt->nResultset-1);
	if (!pSchema)
	{
		SaveCustomError("SQLExecEx", "No variable list specified for resultset '%I'", pStmt->nResultset);
		throw E_APIERROR;
	}
	xj = nNoOfCols;

	do 
	{
		// fill the reference to the specified variable or fieldname
		if (match_identifier(&pSchema,aCursorOrVar,VFP2C_VFP_MAX_CURSOR_NAME))
		{
			if (match_chr(&pSchema,'.'))
			{
				if (match_identifier(&pSchema,aField,VFP2C_VFP_MAX_COLUMN_NAME))
				{
					if (nErrorNo = FindFoxFieldC(aField, lpSC->lField,aCursorOrVar))
						throw nErrorNo;
				}
				else
				{
					SaveCustomError("SQLExecEx","Variable list contained invalid data near '%10S', expected columnname.", pSchema);
					throw E_APIERROR;
				}
			}
			else
			{
				if (nErrorNo = FindFoxVar(aCursorOrVar, lpSC->lField))
					throw nErrorNo;
			}
		}
		else
		{
			SaveCustomError("SQLExecEx","Variable list contained invalid data near '%10S', expected columnname.", pSchema);
			throw E_APIERROR;
		}

		lpSC++;

		if (!--xj)
			break;

	} while (match_chr(&pSchema,','));

	if (!match_chr(&pSchema,'\0'))
	{
		if (!xj)
		{
			SaveCustomError("SQLExecEx", "Variable list contained invalid data near '%10S'.", pSchema);
			throw E_APIERROR;
		}
		else
		{
			SaveCustomError("SQLExecEx", "Variable list contained more variables than columns present in resultset.");
			throw E_APIERROR;
		}
	}
	else if (xj)
	{
		SaveCustomError("SQLExecEx", "Variable list contained less variables than columns present in resultset.");
		throw E_APIERROR;
	}
}

void SqlResultSet::BindVariableLocatorsEx()
{
	if (pStmt->bExecutedOnce)
		return;

	SqlColumn* lpSC = pColumnData;
	int nColNo = nNoOfCols, nErrorNo;
	
	while (nColNo--)
	{
		if (!lpSC->lField.IsVariableRef() && (lpSC->aVFPType == 'M' || lpSC->aVFPType == 'W'))
		{
			if (nErrorNo = MemoChan(lpSC->lField.l_where,&lpSC->hMemoFile))
				throw nErrorNo;
		}
		lpSC++;
	}
}

SqlColumn* SqlResultSet::FindColumn(CStringView pColName)
{
	SqlColumn* lpSC = pColumnData;
	int xj;

	if (!lpSC)
		return 0;

	for (xj = 1; xj <= nNoOfCols; xj++)
	{
		if (pColName.Len == lpSC->nNameLen && StrIEqual(pColName.Data, (char*)lpSC->aColName))
			return lpSC;

		lpSC++;
	}

	return 0;
}

void SqlResultSet::FixColumnName(char *pCol)
{
	int nMaxLen = VFP2C_VFP_MAX_COLUMN_NAME;

	if (!*pCol)
		return;

	if (!IsCharacter(*pCol) && *pCol != '_')
	{
		*pCol++ = '_';
		nMaxLen--;
	}

	while (*pCol && nMaxLen--)
	{
		if (IsCharacter(*pCol) || IsDigit(*pCol) || *pCol == '_')
			pCol++;
		else
			*pCol++ = '_';
	}
	// nullterminate if maxlen is reached .. otherwise nullterminator is just overwritten
	*pCol = '\0';
}

void SqlResultSet::CreateCursor()
{

	SqlColumn* lpSC = pColumnData;
	FoxArray pArray;
try
{
	NTI nVarNti = 0;
	int nErrorNo = 0;
	char *pChar;
	FoxString vChar;
	ValueEx vNumeric;
	ValueEx vLogical;
	vNumeric.SetInt();
	vLogical.SetLogical();
	CStrBuilder<VFP2C_MAX_FUNCTIONBUFFER> pArrayName;
	CStrBuilder<VFP2C_MAX_FUNCTIONBUFFER> pExeBuffer;

	// create unique array name .. since pStmt is a dynamically allocated pointer it's value is
	// always unique .. so we can use it to build a variable name ..
	pArrayName.Format("__VFP2C_ODBC_ARRAY_%U", this);
	pArray.Dimension(pArrayName, nNoOfCols, 6);

	vChar.Size(VFP2C_ODBC_MAX_COLUMN_NAME);
	pChar = vChar.Ptr<char*>();

	for (int nRow = 1; nRow <= nNoOfCols; nRow++)
	{
		if ((lpSC->aVFPType == 'C' || lpSC->aVFPType == 'V' || lpSC->aVFPType == 'Q') && lpSC->nSize > 254)
		{
			SaveCustomError("SqlExecEx", "Invalid cursor schema for field '%S': length %I", lpSC->aColName, lpSC->nSize);
			throw E_APIERROR;
		}
		else if ((lpSC->aVFPType == 'N' || lpSC->aVFPType == 'F') && (lpSC->nSize > 20 || lpSC->nScale > 19))
		{
			SaveCustomError("SqlExecEx", "Invalid cursor schema for field '%S': length %I, scale: %I", lpSC->aColName, lpSC->nSize, lpSC->nScale);
			throw E_APIERROR;
		}
		else if (lpSC->aVFPType == 'B' && lpSC->nSize > 18)
		{
			SaveCustomError("SqlExecEx", "Invalid cursor schema for field '%S': length %I", lpSC->aColName, lpSC->nSize);
			throw E_APIERROR;
		}

		// store fieldname
		vChar.Len(strcpyex(pChar, (char*)lpSC->aColName));
		pArray(nRow, 1) = vChar;

		// store fieldtype
		
		vChar.Len(1);
		*pChar = lpSC->aVFPType;
		pArray(nRow, 2) = vChar;

		// store field width/precision
		// if the VFP datatype is none of the below ones .. the field width is unneccesary
		if (lpSC->aVFPType == 'C' || lpSC->aVFPType == 'V' || lpSC->aVFPType == 'Q' ||
			lpSC->aVFPType == 'F' || lpSC->aVFPType == 'N')
			vNumeric.ev_long = lpSC->nSize;
		else
			vNumeric.ev_long = 0;

		pArray(nRow, 3) = vNumeric;

		// store field scale
		// if the VFP datatype if none of the below ones .. the field scale is unneccesary
		if (lpSC->aVFPType == 'N' || lpSC->aVFPType == 'F')
			vNumeric.ev_long = lpSC->nScale;
		else
			vNumeric.ev_long = 0;

		pArray(nRow, 4) = vNumeric;

		// store if field is nullable
		vLogical.ev_length = lpSC->bNullable;
		pArray(nRow, 5) = vLogical;

		// store if field is binary (NOCPTRANS)
		vLogical.ev_length = lpSC->bBinary;
		pArray(nRow, 6) = vLogical;

		lpSC++;
	}

	CStringView pCursorView = pCursorName, pArrayView = pArrayName;
	pExeBuffer.Format("CREATE CURSOR %V FROM ARRAY %V", &pCursorView, &pArrayView);
	Execute(pExeBuffer);
	pArray.Release();
}
catch (int nErrorNo)
{
	pArray.Release();
	throw nErrorNo;
}
}

void SqlResultSet::BindFieldLocators()
{
	if (pStmt->bExecutedOnce)
		return;

	SqlColumn* lpSC = pColumnData;
	int nErrorNo, nColNo = 0;

	if (nWorkArea == 0)
		nWorkArea = Select(pCursorName);

	while (nColNo++ < nNoOfCols)
	{
		if (nErrorNo = FindFoxField((char*)lpSC->aColName, lpSC->lField, nWorkArea))
			throw nErrorNo;

		if (lpSC->aVFPType == 'M' || lpSC->aVFPType == 'W')
			MemoChan(lpSC->lField.l_where, &lpSC->hMemoFile);

		lpSC++;
	}
}

void SqlResultSet::FetchToCursor(BOOL *bAborted)
{
	SQLRETURN nApiRet;
	SqlColumn* lpSC;
	int nErrorNo;
	pStmt->nRowsFetched = 0;
	
	if (pStmt->nFlags & SQLEXECEX_CALLBACK_PROGRESS)
	{
		// callback once before first row is fetched
		pStmt->ProgressCallback(pStmt->nRowsFetched, bAborted);

		nApiRet = SQLFetch(pStmt->hStmt);
		while (nApiRet == SQL_SUCCESS || nApiRet == SQL_SUCCESS_WITH_INFO)
		{
			lpSC = pColumnData;
			if (nErrorNo = Append(nWorkArea))
				throw nErrorNo;

			for (int xj = 0; xj < nNoOfCols; xj++)
			{
				if (nErrorNo = lpSC->pStore(lpSC))
					throw nErrorNo;
				
				lpSC++;
			}

			pStmt->nRowsFetched++;
			// callback for each Nth row 
			if (pStmt->nRowsFetched % pStmt->nCallbackInterval == 0)
			{
				pStmt->ProgressCallback(pStmt->nRowsFetched, bAborted);
			}
			nApiRet = SQLFetch(pStmt->hStmt);
		}
		if (nApiRet != SQL_ERROR)
		{
			// callback once after last row is fetched
			pStmt->ProgressCallback(pStmt->nRowsFetched, bAborted);
		}
	}
	else
	{
		nApiRet = SQLFetch(pStmt->hStmt);
		while (nApiRet == SQL_SUCCESS || nApiRet == SQL_SUCCESS_WITH_INFO)
		{
			lpSC = pColumnData;
			if (nErrorNo = Append(nWorkArea))
				throw nErrorNo;

			for (int xj = 0; xj < nNoOfCols; xj++)
			{
				if (nErrorNo = lpSC->pStore(lpSC))
					throw nErrorNo;
				lpSC++;
			}
			pStmt->nRowsFetched++;
			nApiRet = SQLFetch(pStmt->hStmt);
		}
	}

	if (nApiRet == SQL_ERROR)
	{
		SafeODBCStmtError("SQLFetch", pStmt->hStmt);
		throw E_APIERROR;
	}
}

void SqlResultSet::FetchToVariables()
{
	SQLRETURN nApiRet;
	SqlColumn* lpSC;
	int nErrorNo;
	pStmt->nRowsFetched = 0;
	nApiRet = SQLFetch(pStmt->hStmt);
	if (nApiRet == SQL_SUCCESS || nApiRet == SQL_SUCCESS_WITH_INFO)
	{
		lpSC = pColumnData;
		for (int xj = 0; xj < nNoOfCols; xj++)
		{
			if (nErrorNo = lpSC->pStore(lpSC))
				throw nErrorNo;
			lpSC++;
		}
		pStmt->nRowsFetched++;
	}
	else if (nApiRet == SQL_ERROR)
	{
		SafeODBCStmtError("SQLFetch", pStmt->hStmt);
		throw E_APIERROR;
	}
}

SqlColumn::SqlColumn()
{
	vData.SetNull();
	vNull.SetNull();
	pData = 0;
	nSize = 0;
	nCType = 0;
	bBinary = 0;
	bUnsigned = 0;
	bMoney = 0;
	nDisplaySize = 0;
	nBufferSize = 0;
	nIndicator = 0;
	bBindColumn = FALSE;
	bCustomSchema = FALSE;
}

SqlParameter::SqlParameter()
{
	aParmExpr[0] = '\0';
	aParmName[0] = '\0';
	vParmValue.SetNull();
	pParmData = 0;
	nParmNo = 0;
	nSize = 0;
	nBufferSize = 0;
	nIndicator = 0;
	nParmDirection = 0;
	nPrecision = 0;
	nScale = 0;
	nSQLType = 0;
	nCType = 0;
	bPutData = FALSE;
	bCustomSchema = FALSE;
	bNamed = FALSE;
}

BOOL _fastcall SQLTypeConvertible(SQLSMALLINT nSQLType, char aVFPType)
{
	switch (nSQLType)
	{
	case SQL_CHAR:
	case SQL_VARCHAR:
	case SQL_LONGVARCHAR:
	case SQL_BINARY:
	case SQL_VARBINARY:
	case SQL_LONGVARBINARY:
	case SQL_WCHAR:
	case SQL_WVARCHAR:
	case SQL_WLONGVARCHAR:
		return aVFPType == 'C' || aVFPType == 'V' || aVFPType == 'Q' ||
			aVFPType == 'M' || aVFPType == 'W';

	case SQL_TINYINT:
	case SQL_SMALLINT:
	case SQL_INTEGER:
		return aVFPType == 'C' || aVFPType == 'V' || aVFPType == 'Q' ||
			aVFPType == 'N' || aVFPType == 'F' || aVFPType == 'B' ||
			aVFPType == 'L' || aVFPType == 'I';

	case SQL_BIGINT:
		return aVFPType == 'C' || aVFPType == 'V' || aVFPType == 'Q';


	case SQL_FLOAT:
	case SQL_REAL:
	case SQL_DOUBLE:
		return aVFPType == 'N' || aVFPType == 'F' || aVFPType == 'B' ||
			aVFPType == 'C' || aVFPType == 'V' || aVFPType == 'Q';

	case SQL_DECIMAL:
	case SQL_NUMERIC:
		return aVFPType == 'C' || aVFPType == 'V' || aVFPType == 'N' ||
			aVFPType == 'B' || aVFPType == 'F' || aVFPType == 'I' ||
			aVFPType == 'Y' || aVFPType == 'L';

	case SQL_DATETIME:
	case SQL_TIMESTAMP:
		return aVFPType == 'D' || aVFPType == 'T' || aVFPType == 'C' ||
			aVFPType == 'V' || aVFPType == 'Q';

	case SQL_TIME:
		return aVFPType == 'C' || aVFPType == 'V' || aVFPType == 'Q';

	case SQL_BIT:
		return aVFPType == 'C' || aVFPType == 'V' || aVFPType == 'L';

	case SQL_GUID:
		return aVFPType == 'C' || aVFPType == 'Q';

	default:
		return TRUE;
	}
}

unsigned int _fastcall SQLExtractInfo(char* pMessage, unsigned int nMsgLen)
{
	char* pStart = pMessage;
	unsigned int nLength = 0;
	int nMaxDriverMessages = 3;

	if (!*pMessage)
		return 0;

	if (*pMessage == '[')
	{
		while (nMaxDriverMessages--)
		{
			while (*pMessage && *pMessage != ']')
				pMessage++;

			if (*pMessage == ']')
				pMessage++; // skip over ]
			else
				break;

			if (*pMessage != '[') // found start of real message
				break;
		}
	}

	if (*pMessage)
	{
		nLength = nMsgLen - (pMessage - pStart); // compute length of message
		memmove(pStart, pMessage, nLength); // move the content to the start of the string ..
	}

	return nLength;
}


int _fastcall SQLStoreByBinding(SqlColumn* lpSC)
{
	if (lpSC->nIndicator == SQL_NULL_DATA)
		return _DBReplace(lpSC->lField, lpSC->vNull);
	else
		return _DBReplace(lpSC->lField, lpSC->vData);
}

int _fastcall SQLStoreByBindingVar(SqlColumn* lpSC)
{
	if (lpSC->nIndicator == SQL_NULL_DATA)
		return _Store(lpSC->lField, lpSC->vNull);
	else
		return _Store(lpSC->lField, lpSC->vData);
}

int _fastcall SQLStoreByGetData(SqlColumn* lpSC)
{
	SQLRETURN nApiRet;
	nApiRet = SQLGetData(lpSC->hStmt,lpSC->nColNo,lpSC->nCType,lpSC->pData,lpSC->nBufferSize,&lpSC->nIndicator);
	if (nApiRet == SQL_SUCCESS || nApiRet == SQL_SUCCESS_WITH_INFO)
	{
		if (lpSC->nIndicator == SQL_NULL_DATA)
			return _DBReplace(lpSC->lField, lpSC->vNull);
		else
			return _DBReplace(lpSC->lField, lpSC->vData);
	}
	SafeODBCStmtError("SQLGetData", lpSC->hStmt);
	return E_APIERROR;
}

int _fastcall SQLStoreByGetDataVar(SqlColumn* lpSC)
{
	SQLRETURN nApiRet;
	nApiRet = SQLGetData(lpSC->hStmt,lpSC->nColNo,lpSC->nCType,lpSC->pData,lpSC->nBufferSize,&lpSC->nIndicator);
	if (nApiRet == SQL_SUCCESS || nApiRet == SQL_SUCCESS_WITH_INFO)
	{
		if (lpSC->nIndicator == SQL_NULL_DATA)
			return _Store(lpSC->lField, lpSC->vNull);
		else
			return _Store(lpSC->lField, lpSC->vData);
	}
	SafeODBCStmtError("SQLGetData", lpSC->hStmt);
	return E_APIERROR;
}

int _fastcall SQLStoreCharByBinding(SqlColumn* lpSC)
{
	if (lpSC->nIndicator == SQL_NULL_DATA)
		return _DBReplace(lpSC->lField, lpSC->vNull);
	else
	{
		lpSC->vData.ev_length = lpSC->nIndicator;
		return _DBReplace(lpSC->lField, lpSC->vData);
	}
}

int _fastcall SQLStoreCharByBindingVar(SqlColumn* lpSC)
{
	if (lpSC->nIndicator == SQL_NULL_DATA)
		return _Store(lpSC->lField, lpSC->vNull);
	else
	{
		lpSC->vData.ev_length = lpSC->nIndicator;
		return _Store(lpSC->lField, lpSC->vData);
	}
}

int _fastcall SQLStoreCharByGetData(SqlColumn* lpSC)
{
	SQLRETURN nApiRet;
	nApiRet = SQLGetData(lpSC->hStmt,lpSC->nColNo,lpSC->nCType,lpSC->pData,lpSC->nBufferSize,&lpSC->nIndicator);
	if (nApiRet == SQL_SUCCESS || nApiRet == SQL_SUCCESS_WITH_INFO)
	{
		if (lpSC->nIndicator == SQL_NULL_DATA)
			return _DBReplace(lpSC->lField, lpSC->vNull);
		else
		{
			lpSC->vData.ev_length = lpSC->nIndicator;
			return _DBReplace(lpSC->lField, lpSC->vData);
		}
	 }
	SafeODBCStmtError("SQLGetData", lpSC->hStmt);
	return E_APIERROR;
}

int _fastcall SQLStoreCharByGetDataVar(SqlColumn* lpSC)
{
	SQLRETURN nApiRet;
	nApiRet = SQLGetData(lpSC->hStmt,lpSC->nColNo,lpSC->nCType,lpSC->pData,lpSC->nBufferSize,&lpSC->nIndicator);
	if (nApiRet == SQL_SUCCESS || nApiRet == SQL_SUCCESS_WITH_INFO)
	{
		if (lpSC->nIndicator == SQL_NULL_DATA)
			return _Store(lpSC->lField, lpSC->vNull);
		else
		{
			lpSC->vData.ev_length = lpSC->nIndicator;
			return _Store(lpSC->lField, lpSC->vData);
		}
	}
	SafeODBCStmtError("SQLGetData", lpSC->hStmt);
	return E_APIERROR;
}

int _fastcall SQLStoreDateByBinding(SqlColumn* lpSC)
{
	if (lpSC->nIndicator == SQL_NULL_DATA)
		return _DBReplace(lpSC->lField, lpSC->vNull);
	else
	{
		Timestamp_StructToDate(&lpSC->sDateTime, lpSC->vData);
		return _DBReplace(lpSC->lField, lpSC->vData);
	}
}

int _fastcall SQLStoreDateByBindingVar(SqlColumn* lpSC)
{
	if (lpSC->nIndicator == SQL_NULL_DATA)
		return _Store(lpSC->lField, lpSC->vNull);
	else
	{
		Timestamp_StructToDate(&lpSC->sDateTime, lpSC->vData);
		return _Store(lpSC->lField, lpSC->vData);
	}
}

int _fastcall SQLStoreDateByGetData(SqlColumn* lpSC)
{
	SQLRETURN nApiRet;
	
	nApiRet = SQLGetData(lpSC->hStmt,lpSC->nColNo,lpSC->nCType,lpSC->pData,lpSC->nBufferSize,&lpSC->nIndicator);
	if (nApiRet == SQL_SUCCESS || nApiRet == SQL_SUCCESS_WITH_INFO)
	{
		if (lpSC->nIndicator == SQL_NULL_DATA)
			return _DBReplace(lpSC->lField, lpSC->vNull);
		else
		{
			Timestamp_StructToDate(&lpSC->sDateTime, lpSC->vData);
			return _DBReplace(lpSC->lField, lpSC->vData);
		}
	}
	SafeODBCStmtError("SQLGetData", lpSC->hStmt);
	return E_APIERROR;
}

int _fastcall SQLStoreDateByGetDataVar(SqlColumn* lpSC)
{
	SQLRETURN nApiRet;
	
	nApiRet = SQLGetData(lpSC->hStmt,lpSC->nColNo,lpSC->nCType,lpSC->pData,lpSC->nBufferSize,&lpSC->nIndicator);
	if (nApiRet == SQL_SUCCESS || nApiRet == SQL_SUCCESS_WITH_INFO)
	{
		if (lpSC->nIndicator == SQL_NULL_DATA)
			return _Store(lpSC->lField, lpSC->vNull);
		else
		{
			Timestamp_StructToDate(&lpSC->sDateTime, lpSC->vData);
			return _Store(lpSC->lField, lpSC->vData);
		}
	}
	SafeODBCStmtError("SQLGetData", lpSC->hStmt);
	return E_APIERROR;
}

int _fastcall SQLStoreDateTimeByBinding(SqlColumn* lpSC)
{
	if (lpSC->nIndicator == SQL_NULL_DATA)
		return _DBReplace(lpSC->lField, lpSC->vNull);
	else
	{
		Timestamp_StructToDateTime(&lpSC->sDateTime, lpSC->vData);
		return _DBReplace(lpSC->lField, lpSC->vData);
	}
}

int _fastcall SQLStoreDateTimeByBindingVar(SqlColumn* lpSC)
{
	if (lpSC->nIndicator == SQL_NULL_DATA)
		return _Store(lpSC->lField, lpSC->vNull);
	else
	{
		Timestamp_StructToDateTime(&lpSC->sDateTime, lpSC->vData);
		return _Store(lpSC->lField, lpSC->vData);
	}
}

int _fastcall SQLStoreDateTimeByGetData(SqlColumn* lpSC)
{
	SQLRETURN nApiRet;
	
	nApiRet = SQLGetData(lpSC->hStmt,lpSC->nColNo,lpSC->nCType,lpSC->pData,lpSC->nBufferSize,&lpSC->nIndicator);
	if (nApiRet == SQL_SUCCESS || nApiRet == SQL_SUCCESS_WITH_INFO)
	{
		if (lpSC->nIndicator == SQL_NULL_DATA)
			return _DBReplace(lpSC->lField, lpSC->vNull);
		else
		{
			Timestamp_StructToDateTime(&lpSC->sDateTime, lpSC->vData);
			return _DBReplace(lpSC->lField, lpSC->vData);
		}
	}
	SafeODBCStmtError("SQLGetData", lpSC->hStmt);
	return E_APIERROR;
}

int _fastcall SQLStoreDateTimeByGetDataVar(SqlColumn* lpSC)
{
	SQLRETURN nApiRet;
	
	nApiRet = SQLGetData(lpSC->hStmt,lpSC->nColNo,lpSC->nCType,lpSC->pData,lpSC->nBufferSize,&lpSC->nIndicator);
	if (nApiRet == SQL_SUCCESS || nApiRet == SQL_SUCCESS_WITH_INFO)
	{
		if (lpSC->nIndicator == SQL_NULL_DATA)
			return _Store(lpSC->lField, lpSC->vNull);
		else
		{
			Timestamp_StructToDateTime(&lpSC->sDateTime, lpSC->vData);
			return _Store(lpSC->lField, lpSC->vData);
		}
	}
	SafeODBCStmtError("SQLGetData", lpSC->hStmt);
	return E_APIERROR;
}

int _fastcall SQLStoreCurrencyByBinding(SqlColumn* lpSC)
{
	if (lpSC->nIndicator == SQL_NULL_DATA)
		return _DBReplace(lpSC->lField, lpSC->vNull);
	else
	{
		NumericLiteralToCurrency(lpSC->aNumeric, lpSC->vData);
		return _DBReplace(lpSC->lField, lpSC->vData);
	}
}

int _fastcall SQLStoreCurrencyByBindingVar(SqlColumn* lpSC)
{
	if (lpSC->nIndicator == SQL_NULL_DATA)
		return _Store(lpSC->lField, lpSC->vNull);
	else
	{
		NumericLiteralToCurrency(lpSC->aNumeric, lpSC->vData);
		return _Store(lpSC->lField, lpSC->vData);
	}
}

int _fastcall SQLStoreCurrencyByGetData(SqlColumn* lpSC)
{
	SQLRETURN nApiRet;
	nApiRet = SQLGetData(lpSC->hStmt,lpSC->nColNo,lpSC->nCType,lpSC->aNumeric,lpSC->nBufferSize,&lpSC->nIndicator);
	if (nApiRet == SQL_SUCCESS || nApiRet == SQL_SUCCESS_WITH_INFO)
	{
		if (lpSC->nIndicator == SQL_NULL_DATA)
			return _DBReplace(lpSC->lField, lpSC->vNull);
		else
		{
			NumericLiteralToCurrency(lpSC->aNumeric, lpSC->vData);
			return _DBReplace(lpSC->lField, lpSC->vData);
		}
	}
	SafeODBCStmtError("SQLGetData", lpSC->hStmt);
	return E_APIERROR;
}

int _fastcall SQLStoreCurrencyByGetDataVar(SqlColumn* lpSC)
{
	SQLRETURN nApiRet;
	nApiRet = SQLGetData(lpSC->hStmt,lpSC->nColNo,lpSC->nCType,lpSC->aNumeric,lpSC->nBufferSize,&lpSC->nIndicator);
	if (nApiRet == SQL_SUCCESS || nApiRet == SQL_SUCCESS_WITH_INFO)
	{
		if (lpSC->nIndicator == SQL_NULL_DATA)
			return _Store(lpSC->lField, lpSC->vNull);
		else
		{
			NumericLiteralToCurrency(lpSC->aNumeric, lpSC->vData);
			return _Store(lpSC->lField, lpSC->vData);
		}
	}
	SafeODBCStmtError("SQLGetData", lpSC->hStmt);
	return E_APIERROR;
}

int _fastcall SQLStoreMemoChar(SqlColumn* lpSC)
{
	SQLRETURN nApiRet;
	long nLoc;
	int nErrorNo;

	nApiRet = SQLGetData(lpSC->hStmt,lpSC->nColNo,lpSC->nCType,lpSC->pData,VFP2C_ODBC_MAX_BUFFER,&lpSC->nIndicator);
	if (nApiRet == SQL_SUCCESS)
	{
		if (lpSC->nIndicator != SQL_NULL_DATA)
			return ReplaceMemoEx(lpSC->lField,(char*)lpSC->pData,lpSC->nIndicator,lpSC->hMemoFile);
		else if (lpSC->bNullable)
			return _DBReplace(lpSC->lField, lpSC->vNull);
	}
	else if (nApiRet == SQL_SUCCESS_WITH_INFO)
	{
		// result is greater than buffer, allocate space for the memo field, nIndicator contains length of data
		if (nErrorNo = AllocMemo(lpSC->lField,lpSC->nIndicator,&nLoc))
			return nErrorNo;

		// append the data from the buffer
		if (nErrorNo = AppendMemo((char*)lpSC->pData,VFP2C_ODBC_MAX_BUFFER-1,lpSC->hMemoFile,&nLoc))
			return nErrorNo;

		do
		{
			nApiRet = SQLGetData(lpSC->hStmt,lpSC->nColNo,lpSC->nCType,lpSC->pData,VFP2C_ODBC_MAX_BUFFER,&lpSC->nIndicator);
			if (nApiRet == SQL_SUCCESS || nApiRet == SQL_SUCCESS_WITH_INFO)
			{
				if (nErrorNo = AppendMemo((char*)lpSC->pData,VFP2C_ODBC_MAX_BUFFER-1,lpSC->hMemoFile,&nLoc))
					return nErrorNo;
			}
		} while (nApiRet == SQL_SUCCESS_WITH_INFO);

		if (nApiRet == SQL_ERROR)
		{
			SafeODBCStmtError("SQLGetData", lpSC->hStmt);
			return E_APIERROR;
		}
	}
	else
	{
		SafeODBCStmtError("SQLGetData", lpSC->hStmt);
		return E_APIERROR;
	}

	return 0;
}

int _fastcall SQLStoreMemoCharVar(SqlColumn* lpSC)
{
	SQLRETURN nApiRet;
	ValueEx vData;
	char *pData;
	int nRetVal;

	vData.SetString();
	if (!vData.AllocHandle(VFP2C_ODBC_MAX_BUFFER))
		return E_INSUFMEMORY;
	
	vData.LockHandle();
	pData = vData.HandleToPtr();

	nApiRet = SQLGetData(lpSC->hStmt,lpSC->nColNo,lpSC->nCType,pData,VFP2C_ODBC_MAX_BUFFER,&lpSC->nIndicator);
	if (nApiRet == SQL_SUCCESS)
	{
		if (lpSC->nIndicator == SQL_NULL_DATA)
			nRetVal = _Store(lpSC->lField, lpSC->vNull);
		else
		{
			vData.ev_length = lpSC->nIndicator;
			nRetVal = _Store(lpSC->lField, vData);
		}
		vData.UnlockHandle();
		vData.FreeHandle();
		return nRetVal;
	}
	else if (nApiRet == SQL_SUCCESS_WITH_INFO)
	{
		vData.ev_length = lpSC->nIndicator;
		vData.UnlockHandle();
		if (!vData.SetHandleSize(lpSC->nIndicator+1))
		{
			vData.FreeHandle();
			return E_INSUFMEMORY;
		}
		vData.LockHandle();
		pData = vData.HandleToPtr();
		pData += VFP2C_ODBC_MAX_BUFFER-1;

		do 
		{
			nApiRet = SQLGetData(lpSC->hStmt,lpSC->nColNo,lpSC->nCType,pData,VFP2C_ODBC_MAX_BUFFER,&lpSC->nIndicator);
			if (nApiRet == SQL_SUCCESS_WITH_INFO)
				pData += VFP2C_ODBC_MAX_BUFFER-1;
		} while (nApiRet == SQL_SUCCESS_WITH_INFO);
		
		if (nApiRet == SQL_SUCCESS)
			nRetVal = _Store(lpSC->lField, vData);
		else
		{
			SafeODBCStmtError("SQLGetData", lpSC->hStmt);
			nRetVal = E_APIERROR;
		}
		vData.UnlockHandle();
		vData.FreeHandle();
		return nRetVal;
	}
	else
	{
		SafeODBCStmtError("SQLGetData", lpSC->hStmt);
		vData.UnlockHandle();
		vData.FreeHandle();
		return E_APIERROR;
	}
}

int _fastcall SQLStoreMemoWChar(SqlColumn* lpSC)
{
	SQLRETURN nApiRet;
	long nLoc;
	int nErrorNo;

	nApiRet = SQLGetData(lpSC->hStmt,lpSC->nColNo,lpSC->nCType,lpSC->pData,VFP2C_ODBC_MAX_BUFFER,&lpSC->nIndicator);
	if (nApiRet == SQL_SUCCESS)
	{
		if (lpSC->nIndicator != SQL_NULL_DATA)
			return ReplaceMemoEx(lpSC->lField,(char*)lpSC->pData,lpSC->nIndicator,lpSC->hMemoFile);
		else if (lpSC->bNullable)
			return _DBReplace(lpSC->lField, lpSC->vNull);
	}
	else if (nApiRet == SQL_SUCCESS_WITH_INFO)
	{
		// result is greater than buffer, allocate space for the memo field, nIndicator contains length of data
		if (nErrorNo = AllocMemo(lpSC->lField,lpSC->nIndicator,&nLoc))
			return nErrorNo;

		// append the data from the buffer
		if (nErrorNo = AppendMemo((char*)lpSC->pData,VFP2C_ODBC_MAX_BUFFER-2,lpSC->hMemoFile,&nLoc))
			return nErrorNo;

		do 
		{
			nApiRet = SQLGetData(lpSC->hStmt,lpSC->nColNo,lpSC->nCType,lpSC->pData,VFP2C_ODBC_MAX_BUFFER,&lpSC->nIndicator);
			if (nApiRet == SQL_SUCCESS_WITH_INFO || nApiRet == SQL_SUCCESS)
			{
				if (nErrorNo = AppendMemo((char*)lpSC->pData,VFP2C_ODBC_MAX_BUFFER-2,lpSC->hMemoFile,&nLoc))
					return nErrorNo;
			}
		} while (nApiRet == SQL_SUCCESS_WITH_INFO);

		if (nApiRet == SQL_ERROR)
		{
			SafeODBCStmtError("SQLGetData", lpSC->hStmt);
			return E_APIERROR;
		}
	}
	else
	{
		SafeODBCStmtError("SQLGetData", lpSC->hStmt);
		return E_APIERROR;
	}
	return 0;
}

int _fastcall SQLStoreMemoWCharVar(SqlColumn* lpSC)
{
	SQLRETURN nApiRet;
	ValueEx vData;
	char *pData;
	int nRetVal;

	vData.SetString();
	if (!vData.AllocHandle(VFP2C_ODBC_MAX_BUFFER))
		return E_INSUFMEMORY;
	
	vData.LockHandle();
	pData = vData.HandleToPtr();

	nApiRet = SQLGetData(lpSC->hStmt,lpSC->nColNo,lpSC->nCType,pData,VFP2C_ODBC_MAX_BUFFER,&lpSC->nIndicator);
	if (nApiRet == SQL_SUCCESS)
	{
		if (lpSC->nIndicator == SQL_NULL_DATA)
			nRetVal = _Store(lpSC->lField, lpSC->vNull);
		else
		{
			vData.ev_length = lpSC->nIndicator;
			nRetVal = _Store(lpSC->lField, vData);
		}
		vData.UnlockHandle();
		vData.FreeHandle();
		return nRetVal;
	}
	else if (nApiRet == SQL_SUCCESS_WITH_INFO)
	{
		vData.ev_length = lpSC->nIndicator;
		vData.UnlockHandle();
		if (!vData.SetHandleSize(lpSC->nIndicator+2))
		{
			vData.FreeHandle();
			return E_INSUFMEMORY;
		}
		vData.LockHandle();
		pData = vData.HandleToPtr();
		pData += VFP2C_ODBC_MAX_BUFFER-2;

		do 
		{
			nApiRet = SQLGetData(lpSC->hStmt,lpSC->nColNo,lpSC->nCType,pData,VFP2C_ODBC_MAX_BUFFER,&lpSC->nIndicator);
			if (nApiRet == SQL_SUCCESS_WITH_INFO)
				pData += VFP2C_ODBC_MAX_BUFFER-2;
		} while (nApiRet == SQL_SUCCESS_WITH_INFO);

		if (nApiRet == SQL_SUCCESS)
			nRetVal = _Store(lpSC->lField, vData);
		else
		{
			SafeODBCStmtError("SQLGetData", lpSC->hStmt);
			nRetVal = E_APIERROR;
		}
		vData.UnlockHandle();
		vData.FreeHandle();
		return nRetVal;
	}
	else
	{
		SafeODBCStmtError("SQLGetData", lpSC->hStmt);
		vData.UnlockHandle();
		vData.FreeHandle();
		return E_APIERROR;
	}
}

int _fastcall SQLStoreMemoBinary(SqlColumn* lpSC)
{
	SQLRETURN nApiRet;
	long nLoc;
	int nErrorNo;

	nApiRet = SQLGetData(lpSC->hStmt,lpSC->nColNo,lpSC->nCType,lpSC->pData,VFP2C_ODBC_MAX_BUFFER,&lpSC->nIndicator);
	
	if (nApiRet == SQL_SUCCESS)
	{
		if (lpSC->nIndicator != SQL_NULL_DATA)
			return ReplaceMemoEx(lpSC->lField,(char*)lpSC->pData,lpSC->nIndicator,lpSC->hMemoFile);
		else if (lpSC->bNullable)
			return _DBReplace(lpSC->lField, lpSC->vNull);
	}
	else if (nApiRet == SQL_SUCCESS_WITH_INFO)
	{
		// allocate space for the memo field, nIncicator contains length of full data
		if (nErrorNo = AllocMemo(lpSC->lField,lpSC->nIndicator,&nLoc))
			return nErrorNo;

		// append the data from the buffer
		if (nErrorNo = AppendMemo((char*)lpSC->pData,VFP2C_ODBC_MAX_BUFFER,lpSC->hMemoFile,&nLoc))
			return nErrorNo;

		do 
		{
			nApiRet = SQLGetData(lpSC->hStmt,lpSC->nColNo,lpSC->nCType,lpSC->pData,VFP2C_ODBC_MAX_BUFFER,&lpSC->nIndicator);
			if (nApiRet == SQL_SUCCESS || nApiRet == SQL_SUCCESS_WITH_INFO)
			{
				if (nErrorNo = AppendMemo((char*)lpSC->pData,VFP2C_ODBC_MAX_BUFFER,lpSC->hMemoFile,&nLoc))
					return nErrorNo;
			}
		} while (nApiRet == SQL_SUCCESS_WITH_INFO);

		if (nApiRet == SQL_ERROR)
		{
			SafeODBCStmtError("SQLGetData", lpSC->hStmt);
			return E_APIERROR;
		}
	}
	else
	{
		SafeODBCStmtError("SQLGetData", lpSC->hStmt);
		return E_APIERROR;
	}

	return 0;
}

int _fastcall SQLStoreMemoBinaryVar(SqlColumn* lpSC)
{
	SQLRETURN nApiRet;
	ValueEx vData;
	char *pData;
	int nRetVal;
	
	vData.SetString();
	if (!vData.AllocHandle(VFP2C_ODBC_MAX_BUFFER))
		return E_INSUFMEMORY;
	
	vData.LockHandle();
	pData = vData.HandleToPtr();

	nApiRet = SQLGetData(lpSC->hStmt,lpSC->nColNo,lpSC->nCType,pData,VFP2C_ODBC_MAX_BUFFER,&lpSC->nIndicator);
	if (nApiRet == SQL_SUCCESS)
	{
		if (lpSC->nIndicator == SQL_NULL_DATA)
			nRetVal = _Store(lpSC->lField, lpSC->vNull);
		else
		{
			vData.ev_length = lpSC->nIndicator;
			nRetVal = _Store(lpSC->lField, vData);
		}
		vData.UnlockHandle();
		vData.FreeHandle();
		return nRetVal;
	}
	else if (nApiRet == SQL_SUCCESS_WITH_INFO)
	{
		vData.ev_length = lpSC->nIndicator;
		vData.UnlockHandle();
		if (!vData.SetHandleSize(lpSC->nIndicator+2))
		{
			vData.FreeHandle();
			return E_INSUFMEMORY;
		}
		vData.LockHandle();
		pData = vData.HandleToPtr();
		pData += VFP2C_ODBC_MAX_BUFFER;

		do 
		{
			nApiRet = SQLGetData(lpSC->hStmt,lpSC->nColNo,lpSC->nCType,pData,VFP2C_ODBC_MAX_BUFFER,&lpSC->nIndicator);
			if (nApiRet == SQL_SUCCESS_WITH_INFO)
				pData += VFP2C_ODBC_MAX_BUFFER;
		} while (nApiRet == SQL_SUCCESS_WITH_INFO);

		if (nApiRet == SQL_SUCCESS)
			nRetVal = _Store(lpSC->lField, vData);
		else
		{
			SafeODBCStmtError("SQLGetData", lpSC->hStmt);
			nRetVal = E_APIERROR;
		}
		vData.UnlockHandle();
		vData.FreeHandle();
		return nRetVal;
	}
	else
	{
		SafeODBCStmtError("SQLGetData", lpSC->hStmt);
		vData.UnlockHandle();
		vData.FreeHandle();
		return E_APIERROR;
	}
}

void _fastcall Timestamp_StructToDateTime(SQL_TIMESTAMP_STRUCT *pTime, Value *pDateTime)
{
	int lnA, lnY, lnM, lnJDay;
	lnA = (14 - pTime->month) / 12;
	lnY = pTime->year + 4800 - lnA;
	lnM = pTime->month + 12 * lnA - 3;
	lnJDay = pTime->day + (153 * lnM + 2) / 5 + lnY * 365 + lnY / 4 - lnY / 100 + lnY / 400 - 32045;
	pDateTime->ev_real = ((double)lnJDay) + (((double)pTime->hour) * 3600 + pTime->minute * 60 + pTime->second) / 86400;
}

void _fastcall DateTimeToTimestamp_Struct(Value *pDateTime, SQL_TIMESTAMP_STRUCT *pTime)
{
	int lnA, lnB, lnC, lnD, lnE, lnM;
	DWORD lnDays, lnSecs;
	double dDays, dSecs;

	dSecs = modf(pDateTime->ev_real,&dDays);
	lnDays = static_cast<DWORD>(dDays);

	lnA = lnDays + 32044;
	lnB = (4 * lnA + 3) / 146097;
	lnC = lnA - (lnB * 146097) / 4;

	lnD = (4 * lnC + 3) / 1461;
	lnE = lnC - (1461 * lnD) / 4;
	lnM = (5 * lnE + 2) / 153;
	
	pTime->day = (WORD) lnE - (153 * lnM + 2) / 5 + 1;
	pTime->month = (WORD) lnM + 3 - 12 * (lnM / 10);
	pTime->year = (WORD) lnB * 100 + lnD - 4800 + lnM / 10;

	lnSecs = (int)floor(dSecs * 86400.0 + 0.1);
	pTime->hour = (WORD)lnSecs / 3600;
	lnSecs %= 3600;
	pTime->minute = (WORD)lnSecs / 60;
	lnSecs %= 60;
	pTime->second = (WORD)lnSecs;
	pTime->fraction = 0;
}

void _fastcall Timestamp_StructToDate(SQL_TIMESTAMP_STRUCT *pTime, Value *pDateTime)
{
	int lnA, lnY, lnM, lnJDay;
	lnA = (14 - pTime->month) / 12;
	lnY = pTime->year + 4800 - lnA;
	lnM = pTime->month + 12 * lnA - 3;
	lnJDay = pTime->day + (153 * lnM + 2) / 5 + lnY * 365 + lnY / 4 - lnY / 100 + lnY / 400 - 32045;
	pDateTime->ev_real = ((double)lnJDay);
}

void _fastcall DateToTimestamp_Struct(Value *pDateTime, SQL_TIMESTAMP_STRUCT *pTime)
{
	int lnA, lnB, lnC, lnD, lnE, lnM;
	DWORD lnDays;

	lnDays = static_cast<DWORD>(pDateTime->ev_real);

	lnA = lnDays + 32044;
	lnB = (4 * lnA + 3) / 146097;
	lnC = lnA - (lnB * 146097) / 4;

	lnD = (4 * lnC + 3) / 1461;
	lnE = lnC - (1461 * lnD) / 4;
	lnM = (5 * lnE + 2) / 153;
	
	pTime->day = (WORD) lnE - (153 * lnM + 2) / 5 + 1;
	pTime->month = (WORD) lnM + 3 - 12 * (lnM / 10);
	pTime->year = (WORD) lnB * 100 + lnD - 4800 + lnM / 10;
	pTime->hour = 0;
	pTime->minute = 0;
	pTime->second = 0;
	pTime->fraction = 0;
}

void _fastcall Numeric_StructToCurrency(SQL_NUMERIC_STRUCT *lpNum, Value *pValue)
{
	__int64 nNumeric = *(__int64*)lpNum->val;
	int nScale;

	if (lpNum->sign == 2)
		nNumeric = -nNumeric;
	
	nScale = VFP2C_VFP_CURRENCY_SCALE - lpNum->scale;
	if (nScale == 0)
		pValue->ev_currency.QuadPart = nNumeric;
	else if (nScale > 0)
	{
		while (nScale--)
			nNumeric *= 10;
		pValue->ev_currency.QuadPart = nNumeric;
	}
	else if (nScale < 0)
	{
		while (nScale--)
			nNumeric /= 10;
		pValue->ev_currency.QuadPart = nNumeric;
	}
}

void _fastcall CurrencyToNumeric_Struct(Value *pValue, SQL_NUMERIC_STRUCT *lpNum)
{
	__int64 *pLow = (__int64*)&lpNum->val[0], *pHigh = (__int64*)&lpNum->val[9];
	lpNum->sign = pValue->ev_currency.QuadPart < 0 ? 2 : 1;
	lpNum->precision = VFP2C_VFP_CURRENCY_PRECISION;
	lpNum->scale = VFP2C_VFP_CURRENCY_SCALE;
	*pLow = pValue->ev_currency.QuadPart;
	*pHigh = 0;
}

void _fastcall CurrencyToNumericLiteral(Value *pValue, SQLCHAR *pLiteral)
{
	__int64 nCurrency;
	unsigned int nFraction;
	char aNumeric[VFP2C_ODBC_MAX_CURRENCY_LITERAL+1];
	char *pNumeric = aNumeric;

	nCurrency = pValue->ev_currency.QuadPart;
	if (nCurrency < 0)
	{
		*pLiteral++ = '-';
		nCurrency = -nCurrency;
	}
	nFraction = (unsigned int)nCurrency % 10000;
	nCurrency /= 10000;

	if (nFraction)
	{
		do
		{
			*pNumeric++ = '0' + nFraction % 10;
			nFraction /= 10;
		} while(nFraction);
		*pNumeric++ = '.';
	}

	if (nCurrency)
	{
		unsigned __int64 nCurr = (unsigned __int64)nCurrency;
		do
		{
			*pNumeric++ = '0' + (char)(nCurr % 10);
			nCurr /= 10;
		} while (nCurr);
	}
	else
		*pNumeric++ = '0';

	while (pNumeric != aNumeric)
		*pLiteral++ = *--pNumeric;

	*pLiteral = '\0';
}

void _fastcall NumericLiteralToCurrency(SQLCHAR *pLiteral, Value *pValue)
{
	__int64 nCurrency = 0;
	unsigned int nFraction = 0;
	int nScale = VFP2C_VFP_CURRENCY_SCALE + 1;
	BOOL bNegative;
	
	while (*pLiteral == ' ') pLiteral++; // skip over whitespace

	if (*pLiteral == '-')
	{
		pLiteral++;
		bNegative = TRUE;
	}
	else if (*pLiteral == '+')
	{
		pLiteral++;
		bNegative = FALSE;
	}
	else
		bNegative = FALSE;
    
	while (*pLiteral)
	{
		if (*pLiteral == '.')
			break;

		nCurrency = nCurrency * 10 + (*pLiteral - '0');
		pLiteral++;
	}
	
	if (*pLiteral == '.')
	{
		pLiteral++; // skip over '.'
		while (*pLiteral && nScale--)
		{
			nFraction = nFraction * 10 + (*pLiteral - '0');
			pLiteral++;
		}
	}

	// there were more than 4 digits after the decimal point, round up if neccessary
	if (nScale == -1)
	{	
		if (nFraction % 10 > 5)
			nFraction = nFraction / 10 + 1;
		else
			nFraction /= 10;
	}
	else if (nScale == VFP2C_VFP_CURRENCY_SCALE + 1); // there were no decimals, do nothing
	else if (nScale > 1) // there were less then 4 digits, scale fraction
		while (nScale-- > 1) nFraction *= 10;

	nCurrency = nCurrency * 10000 + nFraction;
	pValue->ev_currency.QuadPart = bNegative ? -nCurrency : nCurrency;
}