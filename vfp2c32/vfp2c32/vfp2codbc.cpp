#include <windows.h>
#include <math.h>
#include <stdio.h>

#include "pro_ext.h"
#include "vfp2c32.h"
#include "vfpmacros.h"
#include "vfp2cutil.h"
#include "vfp2codbc.h"
#include "vfp2ccppapi.h"
#include "vfpmacros.h"

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
			strncpy(pError->aErrorFunction, pFunction, VFP2C_ERROR_FUNCTION_LEN);
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
		strncpy(pError->aErrorFunction, pFunction, VFP2C_ERROR_FUNCTION_LEN);
		tls.ErrorCount++;
		pError++;
	}
}

int _stdcall VFP2C_Init_Odbc()
{
	if (hODBCEnvHandle)
		return 0;

	int nErrorNo;
	FoxValue vODBCHandle;

	if (nErrorNo = _Evaluate(vODBCHandle, "INT(VAL(SYS(3053)))"))
	{
		SaveCustomErrorEx("SYS(3053)", "Cannot retrieve ODBC environment handle.", nErrorNo);
		return nErrorNo;
	}

	hODBCEnvHandle = reinterpret_cast<SQLHENV>(vODBCHandle->ev_long);
	return 0;
}

void _fastcall CreateSQLDataSource(ParamBlk *parm)
{
try
{
	FoxString pDsn(vp1,2);
	FoxString pDriver(vp2);

	BOOL bRetVal;
	UINT nDSNType = vp3.ev_long & ODBC_SYSTEM_DSN ? ODBC_ADD_SYS_DSN : ODBC_ADD_DSN;

	if (!(bRetVal = SQLConfigDataSource(0,nDSNType,pDriver,pDsn)))
		SaveODBCInstallerError("SQLConfigDataSource");

	Return(bRetVal == TRUE);
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall DeleteSQLDataSource(ParamBlk *parm)
{
try
{
	FoxString pDsn(vp1);
	FoxString pDriver(vp2,2);

	BOOL bRetVal;
	UINT nDSNType = vp3.ev_long == ODBC_SYSTEM_DSN ? ODBC_REMOVE_SYS_DSN : ODBC_REMOVE_DSN;
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

void _fastcall ChangeSQLDataSource(ParamBlk *parm)
{
try
{
	FoxString pDsn(vp1,2);
	FoxString pDriver(vp2,2);

	BOOL bRetVal;
	UINT nDSNType = vp3.ev_long == ODBC_SYSTEM_DSN ? ODBC_CONFIG_SYS_DSN : ODBC_CONFIG_DSN;

	if (!(bRetVal = SQLConfigDataSource(0,nDSNType,pDriver,pDsn)))
		SaveODBCInstallerError("SQLConfigDataSource");

	Return(bRetVal == TRUE);
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall ASQLDataSources(ParamBlk *parm)
{
try
{
	int nErrorNo = VFP2C_Init_Odbc();
	if (nErrorNo)
		throw nErrorNo;

	FoxArray pArray(vp1,1,2);
	FoxString pDatasource(SQL_MAX_DSN_LENGTH+1);
	FoxString pDescription(ODBC_MAX_DESCRIPTION_LEN);
	SQLRETURN nApiRet;
	SQLSMALLINT nDsnLen, nDescLen;
	SQLUSMALLINT nFilter = SQL_FETCH_FIRST;
	unsigned int nRow;

	if (PCount() == 2)
	{
		if (vp2.ev_long == ODBC_USER_DSN)
			nFilter = SQL_FETCH_FIRST_USER;
		else if (vp2.ev_long == ODBC_SYSTEM_DSN)
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

void _fastcall ASQLDrivers(ParamBlk *parm)
{
try
{
	int nErrorNo = VFP2C_Init_Odbc();
	if (nErrorNo)
		throw nErrorNo;

	FoxArray pArray(vp1,1,2);
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

void _fastcall SQLGetPropEx(ParamBlk *parm)
{
try
{
	FoxString pCursor(parm,1);
	FoxString pConAttribute(vp2);
	FoxReference pRef(rp3);
	FoxString pBuffer;
	FoxValue vConHandle;

	char aEvalFunc[VFP2C_MAX_FUNCTIONBUFFER];
	int nRetVal = 1;
	bool bEvalHandle = true;
	SQLHDBC hConHandle = 0;
	SQLRETURN nApiRet;

	if (Vartype(vp1) == 'I' || Vartype(vp1) == 'N')
	{
		DWORD nHandle = Vartype(vp1) == 'I' ? vp1.ev_long : static_cast<DWORD>(vp1.ev_real);
		if (nHandle)
			sprintfex(aEvalFunc,"INT(SQLGETPROP(%U,'ODBChdbc'))",nHandle);
		else
			bEvalHandle = false;
	}
	else if (Vartype(vp1) == 'C')
		sprintfex(aEvalFunc,"INT(SQLGETPROP(CURSORGETPROP('ConnectHandle','%S'),'ODBChdbc'))",(char*)pCursor);
	else
		throw E_INVALIDPARAMS;

	if (bEvalHandle)
	{
		vConHandle.Evaluate(aEvalFunc);
		hConHandle = reinterpret_cast<SQLHDBC>(vConHandle->ev_long);
	}

	if (pConAttribute.ICompare("TRACE"))
	{
		BOOL bTrace;
		nApiRet = SQLGetConnectAttr(hConHandle, SQL_ATTR_TRACE, &bTrace,0,0);
		if (nApiRet != SQL_ERROR)
			pRef = bTrace > 0;
	}
	else if (pConAttribute.ICompare("TRACEFILE"))
	{
		SQLINTEGER nLen;
		pBuffer.Size(MAX_PATH);
		nApiRet = SQLGetConnectAttr(hConHandle,SQL_ATTR_TRACEFILE,pBuffer,MAX_PATH,&nLen);
		if (nApiRet != SQL_ERROR)
			pRef = pBuffer.Len(nLen);
	}
	else if (pConAttribute.ICompare("CONNECTED"))
	{
		BOOL bConnected;
		nApiRet = SQLGetConnectAttr(hConHandle,SQL_ATTR_CONNECTION_DEAD,&bConnected,0,0);
		if (nApiRet != SQL_ERROR)
			pRef = (bConnected == 0); // negate result, since we want to return if we're connected, but we query if the connection is dead ..
	}
	else if (pConAttribute.ICompare("ISOLATIONLEVEL"))
	{
		int nIsolation;
		nApiRet = SQLGetConnectAttr(hConHandle,SQL_ATTR_TXN_ISOLATION,&nIsolation,0,0);
		if (nApiRet != SQL_ERROR)				
			pRef = nIsolation;
	}
	else if (pConAttribute.ICompare("PERFDATA"))
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

void _fastcall SQLSetPropEx(ParamBlk *parm)
{
try
{
	FoxString pCursor(parm,1);
	FoxString pConAttribute(vp2);
	FoxString pProperty(parm,3);
	FoxValue vConHandle;

	char aEvalFunc[VFP2C_MAX_FUNCTIONBUFFER];
	int nRetVal = 1;
	bool bEvalHandle = true;
	SQLHDBC hConHandle = 0;
	SQLRETURN nApiRet;

	if (Vartype(vp1) == 'I' || Vartype(vp1) == 'N')
	{
		DWORD nHandle = Vartype(vp1) == 'I' ? vp1.ev_long : static_cast<DWORD>(vp1.ev_real);
		if (nHandle)
	        sprintfex(aEvalFunc, "INT(SQLGETPROP(%U,'ODBChdbc'))", vp1.ev_long);
		else
			bEvalHandle = false;
	}
	else if (Vartype(vp1) == 'C')
		sprintfex(aEvalFunc, "INT(SQLGETPROP(CURSORGETPROP('ConnectHandle','%S'),'ODBChdbc'))", (char*)pCursor);
	else
		throw E_INVALIDPARAMS;

	if (bEvalHandle)
	{
		vConHandle.Evaluate(aEvalFunc);
		hConHandle = reinterpret_cast<SQLHDBC>(vConHandle->ev_long);
	}

	if (pConAttribute.ICompare("TRACE"))
	{
		if (Vartype(vp3) != 'L')
			throw E_INVALIDPARAMS;
		
		nApiRet = SQLSetConnectAttr(hConHandle,SQL_ATTR_TRACE,(SQLPOINTER)vp3.ev_length,SQL_IS_UINTEGER);
	}
	else if (pConAttribute.ICompare("TRACEFILE"))
	{
		if (Vartype(vp3) != 'C')
			throw E_INVALIDPARAMS;
		
		nApiRet = SQLSetConnectAttr(hConHandle,SQL_ATTR_TRACEFILE,pProperty,pProperty.Len());
	}
	else if (pConAttribute.ICompare("PERFDATA"))
	{
		if (Vartype(vp3) != 'L')
			throw E_INVALIDPARAMS;
		
		vp3.ev_length = vp3.ev_length ? SQL_PERF_START : SQL_PERF_STOP;
		nApiRet = SQLSetConnectAttr(hConHandle,SQL_COPT_SS_PERF_DATA,(SQLPOINTER)vp3.ev_length,SQL_IS_INTEGER);
	}
	else if (pConAttribute.ICompare("PERFDATAFILE"))
	{
		if (Vartype(vp3) != 'C')
			throw E_INVALIDPARAMS;
		
		nApiRet = SQLSetConnectAttr(hConHandle,SQL_COPT_SS_PERF_DATA_LOG,pProperty,SQL_NTS);
	}
	else if (pConAttribute.ICompare("PERFDATALOG"))
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

void _fastcall SQLExecEx(ParamBlk *parm)
{
	// 1. setup
	// 2. parse & rewrite SQL for parameter markers (replace ?varName with ?)
	// 3. SQLBindParamter() - Bind input/output parameters
	// 4. SQLExecDirect() - Execute SQL statement
	// 4.1 if SQLExecDirect returns SQL_NEED_DATA we need to send long parameter data with SQLParamData/SQLPutData
	// 5. SQLNumResultCols() - find out number of columns in result, if no resultset was generated goto step 9.3
	// 6. SQLGetMetaData() - get meta data for columns
	// 6.1 SQLParseCursorSchema - if a custom cursorschema is passed, parse the schema
	// 7. SQLPrepareColumnBindings - based on the meta data and the cursorschema desired create Value buffers for the columns
	// 8. SQLBindColumnEx() - bind all columns that are fixed width to the Value buffers
	// 9. SQLFetchToCursor/SQLFetchToVariables - loop over SQLFetch() until no more data is returned
	// 10 STORE number of fetched / updated / deleted / inserted rows into array
	// 11. SQLMoreResults - check if more result sets are available - if so move to step 5
	// 12. cleanup

	SQLINTEGER nSQLLen;
	SQLRETURN nApiRet;
	LPSQLSTATEMENT pStmt = 0;
	DoubleValue vRowCount(0.0, 0);
	BOOL bAbort = FALSE;
	bool prepared = PCount() == 1;
	int nErrorNo = VFP2C_Init_Odbc();
	if (nErrorNo)
		goto ErrorOut;

	if (prepared)
	{
		pStmt = (LPSQLSTATEMENT)vp1.ev_long;
		pStmt->nResultset = 0;

	// if statement contained parameters parse them out
		if (pStmt->nNoOfParms)
		{
			nApiRet = SQLFreeStmt(pStmt->hStmt, SQL_RESET_PARAMS);
			if (nApiRet == SQL_ERROR)
			{
				SafeODBCStmtError("SQLFreeStmt", pStmt->hStmt);
				goto ErrorOut;
			}

			// 2.5 - evaluate parameters
			if (nErrorNo = SQLEvaluateParams(pStmt))
				goto ErrorOut;

			// 3 - bind parameters in ODBC driver
			if ((nApiRet = SQLBindParameterEx(pStmt)) == SQL_ERROR)
				goto ErrorOut;
		}
	}
	else
	{
		// 1
		pStmt = SQLAllocStatement(parm,&nErrorNo, false);
		if (nErrorNo)
			goto ErrorOut;

		// 2 - check statement for parameters
		if (!(pStmt->nFlags & SQLEXECEX_NATIVE_SQL))
			pStmt->nNoOfParms = SQLNumParamsEx(pStmt->pSQLInput);

		// if statement contained parameters parse them out
		if (pStmt->nNoOfParms)
		{
			pStmt->pSQLSend = (char*)malloc(Len(vp2)+1);
			if (!pStmt->pSQLSend)
			{
				nErrorNo = E_INSUFMEMORY;
				goto ErrorOut;
			}

			pStmt->pParamData = (LPSQLPARAMDATA)malloc(pStmt->nNoOfParms * sizeof(SQLPARAMDATA));
			if (!pStmt->pParamData)
			{
				nErrorNo = E_INSUFMEMORY;
				goto ErrorOut;
			}
			ZeroMemory(pStmt->pParamData,pStmt->nNoOfParms * sizeof(SQLPARAMDATA));

			// 2.3 - thats a meaningful function name isn't it? :)
			if (nErrorNo = SQLExtractParamsAndRewriteStatement(pStmt,&nSQLLen))
				goto ErrorOut;

			// 2.4 - parse parameterschema
			if (nErrorNo = SQLParseParamSchema(pStmt))
				goto ErrorOut;

			// 2.5 - evaluate parameters
			if (nErrorNo = SQLEvaluateParams(pStmt))
				goto ErrorOut;

			// 3 - bind parameters in ODBC driver
			if ((nApiRet = SQLBindParameterEx(pStmt)) == SQL_ERROR)
				goto ErrorOut;

		}
		else // no parameters in SQL statement .. just send it as it is ..
		{
			pStmt->pSQLSend = pStmt->pSQLInput;
			nSQLLen = Len(vp2);
		}
	}

	// 4.
	if (prepared)
		nApiRet = SQLExecute(pStmt->hStmt);
	else
		nApiRet = SQLExecDirect(pStmt->hStmt,(SQLCHAR*)pStmt->pSQLSend,nSQLLen);

	if (nApiRet == SQL_ERROR)
	{
		SafeODBCStmtError("SQLExecDirect", pStmt->hStmt);
		goto ErrorOut;
	}
	// 4.1
	else if (nApiRet == SQL_NEED_DATA)
	{
		nApiRet = SQLPutDataEx(pStmt->hStmt);
		if (nApiRet == SQL_ERROR)
			goto ErrorOut;
	}
	else if (nApiRet == SQL_SUCCESS_WITH_INFO)
		SQLInfoCallbackOrStore(pStmt);

	// 5.
SQLResultSetProcessing:

	pStmt->nResultset++;	

	// get no of columns in resultset (if 0 the SQL statement didn't produce a resultset)
	if ((nApiRet = SQLNumResultCols(pStmt->hStmt,&pStmt->nNoOfCols)) == SQL_ERROR)
	{
		SafeODBCStmtError("SQLNumResultCols", pStmt->hStmt);
		goto ErrorOut;
	}

	if (pStmt->nNoOfCols)
	{
		// we've got a resultset, allocate space to store metadata for each column
		pStmt->pColumnData = (LPSQLCOLUMNDATA)malloc(pStmt->nNoOfCols * sizeof(SQLCOLUMNDATA));
		if (!pStmt->pColumnData)
		{
			nErrorNo = E_INSUFMEMORY;
			goto ErrorOut;
		}
		ZeroMemory(pStmt->pColumnData,pStmt->nNoOfCols * sizeof(SQLCOLUMNDATA));
		// read total rows found (if not supported it'll be -1/0 depending on driver in use)
		SQLRowCount(pStmt->hStmt,&pStmt->nRowsTotal);
	}
	else
	{
		// no resultset ..(UPDATE, DELETE, INSERT or other statement ...)
		if (pStmt->pArrayName)
		{
			SQLLEN nRowCount;
			nApiRet = SQLRowCount(pStmt->hStmt,&nRowCount);
			if (nApiRet == SQL_ERROR)
			{
				SafeODBCStmtError("SQLRowCount", pStmt->hStmt);
				goto ErrorOut;
			}

			// if row is greater than 0 the array has to be redimensioned
			if (pStmt->lArrayLoc.l_sub1++)
			{
				if (nErrorNo = Dimension(pStmt->pArrayName,pStmt->lArrayLoc.l_sub1,2))
					goto ErrorOut;
			}
			
			pStmt->lArrayLoc.l_sub2 = 1;
			pStmt->vCursorName.ev_length = 0;
			if (nErrorNo = _Store(&pStmt->lArrayLoc, &pStmt->vCursorName))
				goto ErrorOut;

			vRowCount.ev_real = (double)nRowCount;
			pStmt->lArrayLoc.l_sub2 = 2;
			if (nErrorNo = _Store(&pStmt->lArrayLoc, &vRowCount))
				goto ErrorOut;
		}
		goto SQLResultSetChecking;
	}

	// 6 
	if ((nApiRet = SQLGetMetaData(pStmt)) != SQL_SUCCESS)
		goto ErrorOut;

	if (pStmt->nFlags & (SQLEXECEX_DEST_CURSOR | SQLEXECEX_REUSE_CURSOR))
	{
		/* if a cursorname is not passed for the resultset
		   generate a default cursorname */
		if (!pStmt->pCursorNames || !GetWordNumN(pStmt->pCursorName, pStmt->pCursorNames,',',pStmt->nResultset,VFP2C_VFP_MAX_CURSOR_NAME))
		{
			if (pStmt->nResultset == 1)
				strcpy(pStmt->pCursorName,"sqlresult");
			else
				sprintfex(pStmt->pCursorName,"sqlresult%I",pStmt->nResultset);
		}
		else
			Alltrim(pStmt->pCursorName);
	}

	// 6.1
	if (pStmt->nFlags & SQLEXECEX_REUSE_CURSOR)
	{
		if (nErrorNo = SQLParseCursorSchemaEx(pStmt, pStmt->pCursorName))
			goto ErrorOut;
	}
	else
	{
		if (nErrorNo = SQLParseCursorSchema(pStmt))
			goto ErrorOut;

		if (pStmt->nFlags & SQLEXECEX_DEST_VARIABLE)
		{
			if (nErrorNo = SQLBindVariableLocators(pStmt))
				goto ErrorOut;
		}
	}

	// 7
	if (nErrorNo = SQLPrepareColumnBindings(pStmt))
		goto ErrorOut;

	// 8
	if ((nApiRet = SQLBindColumnsEx(pStmt)) == SQL_ERROR)
		goto ErrorOut;

	// if result destination are variables
	if (pStmt->nFlags & SQLEXECEX_DEST_VARIABLE)
	{
		if (nErrorNo = SQLBindVariableLocatorsEx(pStmt))
			goto ErrorOut;

		if (nErrorNo = SQLFetchToVariables(pStmt))
			goto ErrorOut;
	}
	else
	{
		// else destination is a cursor ...
		if (pStmt->nFlags & SQLEXECEX_REUSE_CURSOR)
		{
			if (nErrorNo = Zap(pStmt->pCursorName))
				goto ErrorOut;
		}
		else
		{
			if (nErrorNo = SQLCreateCursor(pStmt,pStmt->pCursorName))
				goto ErrorOut;
		}
	
		if (nErrorNo = SQLBindFieldLocators(pStmt,pStmt->pCursorName))
			goto ErrorOut;

		// 9.
		if (nErrorNo = SQLFetchToCursor(pStmt,&bAbort))
			goto ErrorOut;

		if (pStmt->pArrayName)
		{
			if (pStmt->lArrayLoc.l_sub1++)
			{
				if (nErrorNo = Dimension(pStmt->pArrayName,pStmt->lArrayLoc.l_sub1, 2))
					goto ErrorOut;
			}

			pStmt->vCursorName.ev_length = strnlen(pStmt->pCursorName,VFP2C_VFP_MAX_CURSOR_NAME);
			pStmt->lArrayLoc.l_sub2 = 1;
			if (nErrorNo = _Store(&pStmt->lArrayLoc, &pStmt->vCursorName))
				goto ErrorOut;

			vRowCount.ev_real = (double)pStmt->nRowsFetched;
			pStmt->lArrayLoc.l_sub2 = 2;
			if (nErrorNo = _Store(&pStmt->lArrayLoc, &vRowCount))
				goto ErrorOut;
		}

		GoTop(pStmt->pColumnData->lField);
	}

	// callback if there is more info (from TSQL PRINT or RAISERROR statements)
	if (nErrorNo = SQLInfoCallbackOrStore(pStmt))
		goto ErrorOut;

	// unbind column buffers from the statement
	nApiRet = SQLFreeStmt(pStmt->hStmt,SQL_UNBIND);
	if (nApiRet == SQL_ERROR)
	{
		SafeODBCStmtError("SQLFreeStmt", pStmt->hStmt);
		goto ErrorOut;
	}
	// release column buffer's
	SQLFreeColumnBuffers(pStmt);

	// 10.
SQLResultSetChecking:

	nApiRet = SQLMoreResults(pStmt->hStmt);
	if (nApiRet == SQL_ERROR)
	{
		SafeODBCStmtError("SQLMoreResult", pStmt->hStmt);
		goto ErrorOut;
	}
	else if (nApiRet == SQL_SUCCESS_WITH_INFO)
	{
		if (nErrorNo = SQLInfoCallbackOrStore(pStmt))
			goto ErrorOut;
	}

	if (nApiRet != SQL_NO_DATA)
		goto SQLResultSetProcessing;

	// save output parameters
	if (nErrorNo = SQLSaveOutputParameters(pStmt))
		goto ErrorOut;

	Return(pStmt->nResultset);
	// we're finished .. clean up everything ...
	if (prepared)
		SQLFreeParameterEx(pStmt);
	else
		SQLReleaseStatement(parm, pStmt, false);
	return;

	ErrorOut:
		if (!prepared)
			SQLReleaseStatement(parm, pStmt, false);

		if (nErrorNo == E_APIERROR)
			nErrorNo = 0;

		if (nErrorNo)
			RaiseError(nErrorNo);
		else if (!bAbort)
			Return(-1);
		else
			Return(-2);
}

void _fastcall SQLPrepareEx(ParamBlk *parm)
{
	SQLRETURN nApiRet;
	LPSQLSTATEMENT pStmt = 0;
	int nErrorNo = VFP2C_Init_Odbc();
	if (nErrorNo)
		goto ErrorOut;

	// 1
	pStmt = SQLAllocStatement(parm, &nErrorNo, true);
	if (nErrorNo)
		goto ErrorOut;

	// 2 - check statement for parameters
	if (!(pStmt->nFlags & SQLEXECEX_NATIVE_SQL))
		pStmt->nNoOfParms = SQLNumParamsEx(pStmt->pSQLInput);
	
	// if statement contained parameters parse them out
	if (pStmt->nNoOfParms)
	{
		pStmt->pSQLSend = (char*)malloc(vp2.ev_length+1);
		if (!pStmt->pSQLSend)
		{
			nErrorNo = E_INSUFMEMORY;
			goto ErrorOut;
		}

		pStmt->pParamData = (LPSQLPARAMDATA)malloc(pStmt->nNoOfParms * sizeof(SQLPARAMDATA));
		if (!pStmt->pParamData)
		{
			nErrorNo = E_INSUFMEMORY;
			goto ErrorOut;
		}
		ZeroMemory(pStmt->pParamData,pStmt->nNoOfParms * sizeof(SQLPARAMDATA));

		// 2.3 - thats a meaningful function name isn't it? :)
		if (nErrorNo = SQLExtractParamsAndRewriteStatement(pStmt,&pStmt->nSQLLen))
			goto ErrorOut;

		// 2.4 - parse parameterschema
		if (nErrorNo = SQLParseParamSchema(pStmt))
			goto ErrorOut;
	}
	else // no parameters in SQL statement .. just send it as it is ..
	{
		pStmt->pSQLSend = pStmt->pSQLInput;
		pStmt->nSQLLen = vp2.ev_length;
	}

	// 4.
	nApiRet = SQLPrepare(pStmt->hStmt, (SQLCHAR*)pStmt->pSQLSend, pStmt->nSQLLen);
	if (nApiRet == SQL_ERROR)
	{
		SafeODBCStmtError("SQLPrepare", pStmt->hStmt);
		goto ErrorOut;
	}

	// we're finished .. clean up everything ...
	SQLReleaseStatement(parm, pStmt, true);
	Return(pStmt);
	return;

	ErrorOut:
		SQLReleaseStatement(parm, pStmt, false);

		if (nErrorNo == E_APIERROR)
			nErrorNo = 0;

		if (nErrorNo)
			RaiseError(nErrorNo);
		else
			Return(-1);
}

void _fastcall SQLCancelEx(ParamBlk *parm)
{
	LPSQLSTATEMENT pStmt = (LPSQLSTATEMENT)vp1.ev_long;
	if (pStmt)
	{
		SQLReleaseStatement(parm, pStmt, false);
	}
}

LPSQLSTATEMENT _stdcall SQLAllocStatement(ParamBlk *parm, int *nErrorNo, bool prepared)
{
	LPSQLSTATEMENT pStmt;
	char *pCallbackFunc;
	Value vConHandle = {'0'};
	Value vMapVarchar = {'0'};
	char aBuffer[VFP2C_MAX_FUNCTIONBUFFER];

	pStmt = (LPSQLSTATEMENT)malloc(sizeof(SQLSTATEMENT));
	if (!pStmt)
	{
		*nErrorNo = E_INSUFMEMORY;
		return 0;
	}
	ZeroMemory(pStmt,sizeof(SQLSTATEMENT));

	pStmt->bPrepared = prepared;

	if (!AllocHandleEx(pStmt->vCursorName, VFP2C_VFP_MAX_CURSOR_NAME))
	{
		*nErrorNo = E_INSUFMEMORY;
		return pStmt;
	}
	LockHandle(pStmt->vCursorName);
	pStmt->vCursorName.ev_type = 'C';
	pStmt->pCursorName = HandleToPtr(pStmt->vCursorName);

	if (PCount() >= 5 && vp5.ev_long)
	{
		pStmt->nFlags = vp5.ev_long;
		if (!(pStmt->nFlags & (SQLEXECEX_DEST_CURSOR | SQLEXECEX_DEST_VARIABLE | SQLEXECEX_REUSE_CURSOR)))
			pStmt->nFlags |= SQLEXECEX_DEST_CURSOR;
	}
	else
		pStmt->nFlags = SQLEXECEX_DEST_CURSOR | SQLEXECEX_CALLBACK_PROGRESS | SQLEXECEX_CALLBACK_INFO;

	if ((pStmt->nFlags & (SQLEXECEX_CALLBACK_INFO | SQLEXECEX_STORE_INFO)) == (SQLEXECEX_CALLBACK_INFO | SQLEXECEX_STORE_INFO))
	{
		*nErrorNo = E_INVALIDPARAMS;
		return pStmt;
	}

	pStmt->vGetDataBuffer.ev_type = 'C';
	if (!AllocHandleEx(pStmt->vGetDataBuffer,VFP2C_ODBC_MAX_BUFFER))
	{
		*nErrorNo = E_INSUFMEMORY;
		return pStmt;
	}
	LockHandle(pStmt->vGetDataBuffer);
	pStmt->pGetDataBuffer = HandleToPtr(pStmt->vGetDataBuffer);

	if (!NullTerminateValue(vp2))
	{
		*nErrorNo = E_INSUFMEMORY;
		return pStmt;
	}
	LockHandle(vp2);
	pStmt->pSQLInput = HandleToPtr(vp2);

	if (CheckOptionalParameterLen(parm,3))
	{
		if (!NullTerminateValue(vp3))
		{
			*nErrorNo = E_INSUFMEMORY;
			return pStmt;
		}
		LockHandle(vp3);
		pStmt->pCursorNames = HandleToPtr(vp3);
		if (prepared)
			pStmt->pCursorNames = strdup(pStmt->pCursorNames);
	}
	
	if (CheckOptionalParameterLen(parm,4))
	{
		if (!NullTerminateValue(vp4))
		{
			*nErrorNo = E_INSUFMEMORY;
			return pStmt;
		}
		LockHandle(vp4);
		pStmt->pArrayName = HandleToPtr(vp4);
		if (prepared)
			pStmt->pArrayName = strdup(pStmt->pArrayName);
	}
	else if (pStmt->nFlags & SQLEXECEX_STORE_INFO)
	{
		*nErrorNo = E_INVALIDPARAMS;
		return pStmt;
	}

	if (CheckOptionalParameterLen(parm,6))
	{
		if (!NullTerminateValue(vp6))
		{
			*nErrorNo = E_INSUFMEMORY;
			return pStmt;
		}
		LockHandle(vp6);
		pStmt->pCursorSchema = HandleToPtr(vp6);
		if (prepared)
			pStmt->pCursorSchema = strdup(pStmt->pCursorSchema);
	}

	if (CheckOptionalParameterLen(parm,7))
	{
		if (!NullTerminateValue(vp7))
		{
			*nErrorNo = E_INSUFMEMORY;
			return pStmt;
		}
		LockHandle(vp7);
		pStmt->pParamSchema = HandleToPtr(vp7);
		if (prepared)
			pStmt->pParamSchema = strdup(pStmt->pParamSchema);
	}

	if (CheckOptionalParameterLen(parm,8))
	{
		if (Len(vp8) > VFP2C_MAX_CALLBACKFUNCTION)
		{
			*nErrorNo = E_INVALIDPARAMS;
			return pStmt;
		}
		if (!(pStmt->nFlags & (SQLEXECEX_CALLBACK_INFO | SQLEXECEX_CALLBACK_PROGRESS)))
		{
			*nErrorNo = E_INVALIDPARAMS;
			return pStmt;
		}
		if (!NullTerminateValue(vp8))
		{
			*nErrorNo = E_INSUFMEMORY;
			return pStmt;
		}
        pCallbackFunc = HandleToPtr(vp8);

		if (pStmt->nFlags & SQLEXECEX_CALLBACK_INFO)
		{
			sprintfex(aBuffer,"__VFP2C_ODBC_PARM_%U",pStmt);
			pStmt->nInfoNTI = _NewVar(aBuffer,&pStmt->lInfoParm,NV_PRIVATE);
			if (pStmt->nInfoNTI < 0)
			{
				*nErrorNo = -pStmt->nInfoNTI;
				return pStmt;
			}
			
			pStmt->pCallbackCmdInfo = (char*)malloc(VFP2C_MAX_CALLBACKBUFFER);
			if (!pStmt->pCallbackCmdInfo)
			{
				*nErrorNo = E_INSUFMEMORY;
				return pStmt;
			}
			// build command for SQLInfoCallback
			strcpy(pStmt->pCallbackCmdInfo,pCallbackFunc);
			strcat(pStmt->pCallbackCmdInfo,"(-1,");
			strcat(pStmt->pCallbackCmdInfo,aBuffer);
			strcat(pStmt->pCallbackCmdInfo,")");
		}

		if (pStmt->nFlags & SQLEXECEX_CALLBACK_PROGRESS)
		{
			pStmt->pCallbackCmd = (char*)malloc(VFP2C_MAX_CALLBACKFUNCTION);
			pStmt->pCallbackBuffer = (char*)malloc(VFP2C_MAX_CALLBACKBUFFER);
			if (!pStmt->pCallbackCmd || !pStmt->pCallbackBuffer)
			{
				*nErrorNo = E_INSUFMEMORY;
				return pStmt;
			}
			strcpy(pStmt->pCallbackCmd,pCallbackFunc);
			strcat(pStmt->pCallbackCmd,"(%I,%I,%I)");
		}
	}

	if (PCount() >= 9 && vp9.ev_long)
		pStmt->nCallbackInterval = vp9.ev_long;
	else
		pStmt->nCallbackInterval = 100;

	if (pStmt->pArrayName)
	{
		if (*nErrorNo = DimensionEx(pStmt->pArrayName,&pStmt->lArrayLoc,1,2))
			return pStmt;
	}

	// 1.1 - build command to evaluate connection handle
    sprintfex(aBuffer,"INT(SQLGETPROP(%U,'ODBChdbc'))", vp1.ev_long);

	// 1.2 - evaluate connection handle
	if (*nErrorNo = _Evaluate(&vConHandle, aBuffer))
		return pStmt;
	else
		pStmt->hConn = (SQLHDBC)vConHandle.ev_long;


	if (CFoxVersion::MajorVersion() >= 9) 
	{
		if (*nErrorNo = _Evaluate(&vMapVarchar, "CURSORGETPROP('MapVarchar', 0)"))
			return pStmt;
		pStmt->bMapVarchar = vMapVarchar.ev_length > 0;
	}
	else
		pStmt->bMapVarchar = false;

	// 1.3 - allocate statement handle on connection
	if (SQLAllocHandle(SQL_HANDLE_STMT,pStmt->hConn,&pStmt->hStmt) == SQL_ERROR)
	{
		SafeODBCDbcError("SQLAllocHandle", pStmt->hConn);
		*nErrorNo = E_APIERROR;
		return pStmt;
	}

	return pStmt;
}

void _stdcall SQLReleaseStatement(ParamBlk *parm, LPSQLSTATEMENT pStmt, bool prepared)
{
	if (!pStmt)
		return;

	if (prepared) {
		UnlockHandle(vp2);
		if (pStmt->pCursorNames)
			UnlockHandle(vp3);
		if (pStmt->pArrayName)
			UnlockHandle(vp4);
		if (pStmt->pCursorSchema)
			UnlockHandle(vp6);
		if (pStmt->pParamSchema)
			UnlockHandle(vp7);
		return;
	}

	UnlockFreeHandle(pStmt->vCursorName);
	SQLFreeParameterEx(pStmt);
	SQLFreeColumnBuffers(pStmt);
	UnlockFreeHandle(pStmt->vGetDataBuffer);

	if (pStmt->pParamData)
		free(pStmt->pParamData);

	if (pStmt->pSQLInput)
	{
		if (!pStmt->bPrepared)
			UnlockHandle(vp2);
		if (pStmt->pSQLSend != pStmt->pSQLInput)
			free(pStmt->pSQLSend);
	}

	if (!pStmt->bPrepared)
	{
		if (pStmt->pCursorNames)
			UnlockHandle(vp3);
		if (pStmt->pArrayName)
			UnlockHandle(vp4);
		if (pStmt->pCursorSchema)
			UnlockHandle(vp6);
		if (pStmt->pParamSchema)
			UnlockHandle(vp7);
	} 
	else
	{
		if (pStmt->pCursorNames)
			free(pStmt->pCursorNames);
		if (pStmt->pArrayName)
			free(pStmt->pArrayName);
		if (pStmt->pCursorSchema)
			free(pStmt->pCursorSchema);
		if (pStmt->pParamSchema)
			free(pStmt->pParamSchema);
	}

	if (pStmt->pCallbackCmd)
		free(pStmt->pCallbackCmd);
	if (pStmt->pCallbackCmdInfo)
		free(pStmt->pCallbackCmdInfo);
	if (pStmt->pCallbackBuffer)
		free(pStmt->pCallbackBuffer);
	if (pStmt->nInfoNTI)
		_Release(pStmt->nInfoNTI);

	if (pStmt->hStmt)
		SQLFreeHandle(SQL_HANDLE_STMT,pStmt->hStmt);

	free(pStmt);
}

void _stdcall SQLFreeColumnBuffers(LPSQLSTATEMENT pStmt)
{
	if (pStmt->pColumnData)
	{
		LPSQLCOLUMNDATA lpCS = pStmt->pColumnData;
		int count = pStmt->nNoOfCols;
		while(count--)
		{
			// if the handle is valid and the buffer isn't our 
			// general purpose buffer for long data we need to free it
			if (ValidHandle(lpCS->vData) && lpCS->pData != pStmt->pGetDataBuffer)
			{
				UnlockHandle(lpCS->vData);
				FreeHandle(lpCS->vData);
			}
			lpCS++;
		}
		free(pStmt->pColumnData);
		pStmt->pColumnData = 0;
	}
}

void _stdcall SQLFreeParameterEx(LPSQLSTATEMENT pStmt)
{
	if (pStmt->pParamData)
	{
		LPSQLPARAMDATA lpParms = pStmt->pParamData;
		int count = pStmt->nNoOfParms;
		while (count--)
		{
			if (Vartype(lpParms->vParmValue) == 'C')
			{
				UnlockFreeHandle(lpParms->vParmValue);
				lpParms->vParmValue.SetNull();
			}
			lpParms++;
		}
	}
}

SQLRETURN _stdcall SQLGetMetaData(LPSQLSTATEMENT pStmt)
{
	SQLRETURN nApiRet = SQL_SUCCESS;
	LPSQLCOLUMNDATA lpCS = pStmt->pColumnData;
	SQLSMALLINT xj;
	int nUnnamedCol = 0;

	for (xj = 1; xj <= pStmt->nNoOfCols; xj++)
	{

		nApiRet = SQLDescribeCol(pStmt->hStmt,xj,lpCS->aColName,VFP2C_ODBC_MAX_COLUMN_NAME,
								&lpCS->nNameLen,&lpCS->nSQLType,&lpCS->nSize,
								&lpCS->nScale,&lpCS->bNullable);
		if (nApiRet == SQL_ERROR)
		{
			SafeODBCStmtError("SQLDescribeCol", pStmt->hStmt);
			return nApiRet;
		}

		switch (lpCS->nSQLType)
		{
			case SQL_INTEGER:
			case SQL_SMALLINT:
			case SQL_TINYINT:
				// if integer type check sign
				nApiRet = SQLColAttribute(pStmt->hStmt,xj,SQL_DESC_UNSIGNED,0,0,0,&lpCS->bUnsigned);
				if (nApiRet == SQL_ERROR)
				{
					SafeODBCStmtError("SQLColAttribute", pStmt->hStmt);
					return nApiRet;
				}
				break;

			case SQL_NUMERIC:
			case SQL_DECIMAL:
				// if numeric or decimal check for money datatype, add 2 to the size, if NUMERIC or DECIMAL since FoxPro N datatype defines size including scale and decimal point
				nApiRet = SQLColAttribute(pStmt->hStmt,xj,SQL_COLUMN_MONEY,0,0,0,&lpCS->bMoney);
				if (nApiRet == SQL_ERROR)
				{
					SafeODBCStmtError("SQLColAttribute", pStmt->hStmt);
					return nApiRet;
				}
				if (!lpCS->bMoney)
				{
					lpCS->nSize += 2;
					if (lpCS->nSize > 20)
						lpCS->nSize = 20;
					if (lpCS->nScale > 16)
						lpCS->nScale = 16;
				}
				break;

			case SQL_REAL:
			case SQL_FLOAT:
			case SQL_DOUBLE:
				lpCS->nSize = 20;
				lpCS->nScale = 16;
				break;

			case SQL_CHAR:
			case SQL_VARCHAR:
			case SQL_BINARY:
			case SQL_VARBINARY:
			case SQL_WCHAR:
			case SQL_WVARCHAR:
				if (pStmt->bMapVarchar == false)
				{
					if (lpCS->nSQLType == SQL_VARCHAR)
						lpCS->nSQLType = SQL_CHAR;
					else if (lpCS->nSQLType == SQL_WVARCHAR)
						lpCS->nSQLType = SQL_WCHAR;
				}
				// force character/binary fields of len 0 to Memo/Blob
				if (lpCS->nSize == 0)
					lpCS->nSize = VFP2C_VFP_MAX_CHARCOLUMN + 1;
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
				nApiRet = SQLColAttribute(pStmt->hStmt,xj,SQL_DESC_DISPLAY_SIZE,0,0,0,&lpCS->nDisplaySize);
				if (nApiRet == SQL_ERROR)
				{
					SafeODBCStmtError("SQLColAttribute", pStmt->hStmt);
					return nApiRet;
				}
				// if no displaysize is returned, force to a memo field
				if (!lpCS->nDisplaySize)
					lpCS->nDisplaySize = VFP2C_VFP_MAX_CHARCOLUMN + 1;
		}

		// empty column name (from an expression for example)
		if (lpCS->aColName[0] == '\0')
		{
			if (nUnnamedCol)
				sprintfex((char*)lpCS->aColName,"expr%I",nUnnamedCol);
			else
				strcpy((char*)lpCS->aColName,"expr");
			
			nUnnamedCol++;
		}
		else
			SQLFixColumnName((char*)lpCS->aColName);

		lpCS++;	
	}

	return nApiRet;
}

SQLRETURN _stdcall SQLBindColumnsEx(LPSQLSTATEMENT pStmt)
{
	SQLRETURN nApiRet = SQL_SUCCESS;
	LPSQLCOLUMNDATA lpCS = pStmt->pColumnData;
	int xj = pStmt->nNoOfCols;

	while (xj--)
	{
		if (lpCS->bBindColumn)
		{
			nApiRet = SQLBindCol(pStmt->hStmt,lpCS->nColNo, lpCS->nCType, lpCS->pData, 
				lpCS->nBufferSize, &lpCS->nIndicator);
			if (nApiRet == SQL_ERROR)
			{
				SafeODBCStmtError("SQLBindCol", pStmt->hStmt);
				return nApiRet;
			}
		}
		lpCS++;
	}

	return nApiRet;
}

int _stdcall SQLPrepareColumnBindings(LPSQLSTATEMENT pStmt)
{
	SQLRETURN nApiRet;
	SQLUINTEGER bGetDataExt;
	BOOL bUnicodeConversion = VFP2CTls::Tls().SqlUnicodeConversion;
	LPSQLCOLUMNDATA lpCS = pStmt->pColumnData;
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
		return E_APIERROR;
	}

	while (nColNo++ < pStmt->nNoOfCols)
	{

	lpCS->nColNo = nColNo;
	lpCS->hStmt = pStmt->hStmt;
	lpCS->vNull.SetNull();

	switch (lpCS->nSQLType)
	{
		// character data
		case SQL_CHAR:
			lpCS->vData.SetString();
			lpCS->nCType = SQL_C_CHAR;
			if (lpCS->bCustomSchema)
			{
				if (lpCS->aVFPType == 'M' || lpCS->aVFPType == 'W')
				{
					lpCS->vData.ev_handle = pStmt->vGetDataBuffer.ev_handle;
					lpCS->pData = pStmt->pGetDataBuffer;
					lpCS->nBufferSize = VFP2C_ODBC_MAX_BUFFER;
					lpCS->bBindColumn = FALSE;
					bBindCol = bGetDataExt;
					if (!IsVariableRef(lpCS->lField))
						lpCS->pStore = SQLStoreMemoChar;
					else
						lpCS->pStore = SQLStoreMemoCharVar;
				}
				else
				{
					if (!AllocHandleEx(lpCS->vData,lpCS->nSize + 1))
						return E_INSUFMEMORY;
					LockHandle(lpCS->vData);
					lpCS->pData = HandleToPtr(lpCS->vData);
					lpCS->nBufferSize = lpCS->nSize + 1;
					lpCS->bBindColumn = bBindCol;
					if (!IsVariableRef(lpCS->lField))
						lpCS->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpCS->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
			}
			else
			{
				if (lpCS->nSize <= VFP2C_VFP_MAX_CHARCOLUMN)
				{
					if (!AllocHandleEx(lpCS->vData,lpCS->nSize + 1))
						return E_INSUFMEMORY;
					LockHandle(lpCS->vData);
					lpCS->pData = HandleToPtr(lpCS->vData);
					lpCS->nBufferSize = lpCS->nSize + 1;
					lpCS->aVFPType = 'C';
					lpCS->bBindColumn = bBindCol;
					if (!IsVariableRef(lpCS->lField))
						lpCS->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpCS->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
				else
				{
					lpCS->vData.ev_handle = pStmt->vGetDataBuffer.ev_handle;
					lpCS->pData = pStmt->pGetDataBuffer;
					lpCS->aVFPType = 'M';
					lpCS->nBufferSize = VFP2C_ODBC_MAX_BUFFER;
					lpCS->bBindColumn = FALSE;
					bBindCol = bGetDataExt;
					if (!IsVariableRef(lpCS->lField))
						lpCS->pStore = SQLStoreMemoChar;
					else
						lpCS->pStore = SQLStoreMemoCharVar;
				}
			}
			break;

		case SQL_BINARY:
			lpCS->vData.SetString();
			lpCS->nCType = SQL_C_BINARY;
			if (lpCS->bCustomSchema)
			{
				if (lpCS->aVFPType == 'M' || lpCS->aVFPType == 'W')
				{
					lpCS->vData.ev_handle = pStmt->vGetDataBuffer.ev_handle;
					lpCS->pData = pStmt->pGetDataBuffer;
					lpCS->nBufferSize = VFP2C_ODBC_MAX_BUFFER;
					lpCS->bBindColumn = FALSE;
					bBindCol = bGetDataExt;
					if (!IsVariableRef(lpCS->lField))
						lpCS->pStore = SQLStoreMemoBinary;
					else
						lpCS->pStore = SQLStoreMemoBinaryVar;
				}
				else
				{
					if (!AllocHandleEx(lpCS->vData,lpCS->nSize))
						return E_INSUFMEMORY;
					LockHandle(lpCS->vData);
					lpCS->pData = HandleToPtr(lpCS->vData);
					lpCS->nBufferSize = lpCS->nSize;
					lpCS->bBindColumn = bBindCol;
					if (!IsVariableRef(lpCS->lField))
						lpCS->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpCS->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
			}
			else
			{
				if (lpCS->nSize <= VFP2C_VFP_MAX_CHARCOLUMN)
				{
					if (!AllocHandleEx(lpCS->vData,lpCS->nSize))
						return E_INSUFMEMORY;
					LockHandle(lpCS->vData);
					lpCS->pData = HandleToPtr(lpCS->vData);
					lpCS->nBufferSize = lpCS->nSize;
					if (CFoxVersion::MajorVersion() >= 9)
						lpCS->aVFPType = 'Q';
					else
					{
						lpCS->aVFPType = 'C';
						lpCS->bBinary = TRUE;
					}
					lpCS->bBindColumn = bBindCol;
					if (!IsVariableRef(lpCS->lField))
						lpCS->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpCS->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
				else
				{
					lpCS->vData.ev_handle = pStmt->vGetDataBuffer.ev_handle;
					lpCS->pData = pStmt->pGetDataBuffer;
					if (CFoxVersion::MajorVersion() >= 9)
						lpCS->aVFPType = 'W';
					else
					{
						lpCS->aVFPType = 'M';
						lpCS->bBinary = TRUE;
					}
					lpCS->nBufferSize = VFP2C_ODBC_MAX_BUFFER;
					lpCS->bBindColumn = FALSE;
					bBindCol = bGetDataExt;
					if (!IsVariableRef(lpCS->lField))
						lpCS->pStore = SQLStoreMemoBinary;
					else
						lpCS->pStore = SQLStoreMemoBinaryVar;
				}
			}
			break;

		case SQL_VARCHAR:
			lpCS->vData.SetString();
			lpCS->nCType = SQL_C_CHAR;
			if (lpCS->bCustomSchema)
			{
				if (lpCS->aVFPType == 'M' || lpCS->aVFPType == 'W')
				{
					lpCS->vData.ev_handle = pStmt->vGetDataBuffer.ev_handle;
					lpCS->pData = pStmt->pGetDataBuffer;
					lpCS->nBufferSize = VFP2C_ODBC_MAX_BUFFER;
					lpCS->bBindColumn = FALSE;
					bBindCol = bGetDataExt;
					if (!IsVariableRef(lpCS->lField))
						lpCS->pStore = SQLStoreMemoChar;
					else
						lpCS->pStore = SQLStoreMemoCharVar;
				}
				else
				{
					if (!AllocHandleEx(lpCS->vData,lpCS->nSize + 1))
						return E_INSUFMEMORY;
					LockHandle(lpCS->vData);
					lpCS->pData = HandleToPtr(lpCS->vData);
					lpCS->nBufferSize = lpCS->nSize + 1;
					lpCS->bBindColumn = bBindCol;
					if (!IsVariableRef(lpCS->lField))
						lpCS->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpCS->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
			}
			else
			{
				if (lpCS->nSize <= VFP2C_VFP_MAX_CHARCOLUMN)
				{
					if (!AllocHandleEx(lpCS->vData,lpCS->nSize + 1))
						return E_INSUFMEMORY;
					LockHandle(lpCS->vData);
					lpCS->pData = HandleToPtr(lpCS->vData);
					lpCS->nBufferSize = lpCS->nSize + 1;
					if (CFoxVersion::MajorVersion() >= 9)
						lpCS->aVFPType = 'V';
					else
						lpCS->aVFPType = 'C';
					lpCS->bBindColumn = bBindCol;
					if (!IsVariableRef(lpCS->lField))
						lpCS->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpCS->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
				else
				{
					lpCS->vData.ev_handle = pStmt->vGetDataBuffer.ev_handle;
					lpCS->pData = pStmt->pGetDataBuffer;
					lpCS->nBufferSize = VFP2C_ODBC_MAX_BUFFER;
					lpCS->aVFPType = 'M';
					lpCS->bBindColumn = FALSE;
					if (!IsVariableRef(lpCS->lField))
						lpCS->pStore = SQLStoreMemoChar;
					else
						lpCS->pStore = SQLStoreMemoCharVar;
					bBindCol = bGetDataExt;					
				}
			}
			break;

		case SQL_VARBINARY:
			lpCS->vData.SetString();
			lpCS->nCType = SQL_C_BINARY;
			if (lpCS->bCustomSchema)
			{
				if (lpCS->aVFPType == 'M' || lpCS->aVFPType == 'W')
				{
					lpCS->vData.ev_handle = pStmt->vGetDataBuffer.ev_handle;
					lpCS->pData = pStmt->pGetDataBuffer;
					lpCS->nBufferSize = VFP2C_ODBC_MAX_BUFFER;
					lpCS->bBindColumn = FALSE;
					bBindCol = bGetDataExt;
					if (!IsVariableRef(lpCS->lField))
						lpCS->pStore = SQLStoreMemoBinary;
					else
						lpCS->pStore = SQLStoreMemoBinaryVar;
				}
				else
				{
					if (!AllocHandleEx(lpCS->vData,lpCS->nSize + 1))
						return E_INSUFMEMORY;
					LockHandle(lpCS->vData);
					lpCS->pData = HandleToPtr(lpCS->vData);
					lpCS->nBufferSize = lpCS->nSize + 1;
					lpCS->bBindColumn = bBindCol;
					if (!IsVariableRef(lpCS->lField))
						lpCS->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpCS->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
			}
			else
			{
				if (lpCS->nSize <= VFP2C_VFP_MAX_CHARCOLUMN)
				{
					if (!AllocHandleEx(lpCS->vData,lpCS->nSize))
						return E_INSUFMEMORY;
					LockHandle(lpCS->vData);
					lpCS->pData = HandleToPtr(lpCS->vData);
					lpCS->nBufferSize = lpCS->nSize;
					lpCS->aVFPType = 'C';
					lpCS->bBinary = TRUE;
					lpCS->bBindColumn = bBindCol;
					if (!IsVariableRef(lpCS->lField))
						lpCS->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpCS->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
				else
				{
					lpCS->vData.ev_handle = pStmt->vGetDataBuffer.ev_handle;
					lpCS->pData = pStmt->pGetDataBuffer;
					lpCS->nBufferSize = VFP2C_ODBC_MAX_BUFFER;
					if (CFoxVersion::MajorVersion() >= 9)
						lpCS->aVFPType = 'W';
					else
					{
						lpCS->aVFPType = 'M';
						lpCS->bBinary = TRUE;
					}
					lpCS->bBindColumn = FALSE;
					bBindCol = bGetDataExt;
					if (!IsVariableRef(lpCS->lField))
						lpCS->pStore = SQLStoreMemoBinary;
					else
						lpCS->pStore = SQLStoreMemoBinaryVar;
				}
			}
			break;

		case SQL_LONGVARCHAR:
			lpCS->vData.SetString();
			lpCS->nCType = SQL_C_CHAR;
			if (lpCS->bCustomSchema)
			{
				if (lpCS->aVFPType == 'M' || lpCS->aVFPType == 'W')
				{
					lpCS->vData.ev_handle = pStmt->vGetDataBuffer.ev_handle;
					lpCS->pData = pStmt->pGetDataBuffer;
					lpCS->nBufferSize = VFP2C_ODBC_MAX_BUFFER;
					lpCS->bBindColumn = FALSE;
					bBindCol = bGetDataExt;
					if (!IsVariableRef(lpCS->lField))
						lpCS->pStore = SQLStoreMemoChar;
					else
						lpCS->pStore = SQLStoreMemoCharVar;
				}
				else
				{
					lpCS->vData.ev_handle = pStmt->vGetDataBuffer.ev_handle;
					lpCS->pData = pStmt->pGetDataBuffer;
					lpCS->nBufferSize = VFP2C_ODBC_MAX_BUFFER;
					lpCS->bBindColumn = FALSE;
					bBindCol = bGetDataExt;
					if (!IsVariableRef(lpCS->lField))
						lpCS->pStore = SQLStoreCharByGetData;
					else
						lpCS->pStore = SQLStoreCharByGetDataVar;
				}
			}
			else
			{
				lpCS->vData.ev_handle = pStmt->vGetDataBuffer.ev_handle;
				lpCS->pData = pStmt->pGetDataBuffer;
				lpCS->nBufferSize = VFP2C_ODBC_MAX_BUFFER;
				lpCS->aVFPType = 'M';
				lpCS->bBindColumn = FALSE;
				if (!IsVariableRef(lpCS->lField))
					lpCS->pStore = SQLStoreMemoChar;
				else
					lpCS->pStore = SQLStoreMemoCharVar;
				bBindCol = bGetDataExt;
			}
			break;

		case SQL_LONGVARBINARY:
			lpCS->vData.SetString();
			lpCS->nCType = SQL_C_BINARY;
			if (lpCS->bCustomSchema)
			{
				if (lpCS->aVFPType == 'M' || lpCS->aVFPType == 'W')
				{
					lpCS->vData.ev_handle = pStmt->vGetDataBuffer.ev_handle;
					lpCS->pData = pStmt->pGetDataBuffer;
					lpCS->nBufferSize = VFP2C_ODBC_MAX_BUFFER;
					lpCS->bBindColumn = FALSE;
					bBindCol = bGetDataExt;
					if (!IsVariableRef(lpCS->lField))
						lpCS->pStore = SQLStoreMemoBinary;
					else
						lpCS->pStore = SQLStoreMemoBinaryVar;
				}
				else
				{
					if (!AllocHandleEx(lpCS->vData,lpCS->nSize))
						return E_INSUFMEMORY;
					LockHandle(lpCS->vData);
					lpCS->pData = HandleToPtr(lpCS->vData);
					lpCS->nBufferSize = lpCS->nSize;
					lpCS->bBindColumn = bBindCol;
					if (!IsVariableRef(lpCS->lField))
						lpCS->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpCS->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
			}
			else
			{
				lpCS->vData.ev_handle = pStmt->vGetDataBuffer.ev_handle;
				lpCS->pData = pStmt->pGetDataBuffer;
				lpCS->nBufferSize = VFP2C_ODBC_MAX_BUFFER;
				if (CFoxVersion::MajorVersion() >= 9)
					lpCS->aVFPType = 'W';
				else
				{
					lpCS->aVFPType = 'M';
					lpCS->bBinary = TRUE;
				}
				lpCS->bBindColumn = FALSE;
				bBindCol = bGetDataExt;
				if (!IsVariableRef(lpCS->lField))
					lpCS->pStore = SQLStoreMemoBinary;
				else
					lpCS->pStore = SQLStoreMemoBinaryVar;
			}
			break;

		case SQL_WCHAR:
			lpCS->vData.SetString();
			if (lpCS->bCustomSchema)
			{
				if (lpCS->aVFPType == 'M' || lpCS->aVFPType == 'W')
				{
					lpCS->vData.ev_handle = pStmt->vGetDataBuffer.ev_handle;
					lpCS->pData = pStmt->pGetDataBuffer;
					lpCS->nBufferSize = VFP2C_ODBC_MAX_BUFFER;
					lpCS->bBindColumn = FALSE;
					bBindCol = bGetDataExt;
					if (lpCS->aVFPType == 'W' || lpCS->bBinary)
					{
						lpCS->nCType = SQL_C_WCHAR;
						if (!IsVariableRef(lpCS->lField))
							lpCS->pStore = SQLStoreMemoWChar;
						else
							lpCS->pStore = SQLStoreMemoWCharVar;
					}
					else
					{
						lpCS->nCType = SQL_C_CHAR;
						if (!IsVariableRef(lpCS->lField))
							lpCS->pStore = SQLStoreMemoChar;
						else
							lpCS->pStore = SQLStoreMemoCharVar;
					}
				}
				else
				{
					if (!AllocHandleEx(lpCS->vData,lpCS->nSize + 2))
						return E_INSUFMEMORY;
					LockHandle(lpCS->vData);
					lpCS->pData = HandleToPtr(lpCS->vData);
					lpCS->nBufferSize = lpCS->nSize + 2;
					if (lpCS->aVFPType == 'Q' || lpCS->bBinary)
						lpCS->nCType = SQL_C_WCHAR;
					else
						lpCS->nCType = SQL_C_CHAR;
					lpCS->bBindColumn = bBindCol;
					if (!IsVariableRef(lpCS->lField))
						lpCS->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpCS->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
			}
			else
			{
				if (lpCS->nSize <= VFP2C_VFP_MAX_CHARCOLUMN)
				{
					if (!AllocHandleEx(lpCS->vData, lpCS->nSize + 1))
						return E_INSUFMEMORY;
					LockHandle(lpCS->vData);
					lpCS->pData = HandleToPtr(lpCS->vData);
					lpCS->nBufferSize = lpCS->nSize + 1;
					lpCS->nCType = bUnicodeConversion ? SQL_C_CHAR : SQL_C_WCHAR;
					lpCS->aVFPType = 'C';
					lpCS->bBindColumn = bBindCol;
					if (!IsVariableRef(lpCS->lField))
						lpCS->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpCS->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
				else
				{
					lpCS->vData.ev_handle = pStmt->vGetDataBuffer.ev_handle;
					lpCS->pData = pStmt->pGetDataBuffer;
					lpCS->nBufferSize = VFP2C_ODBC_MAX_BUFFER;
					lpCS->nCType = bUnicodeConversion ? SQL_C_CHAR : SQL_C_WCHAR;
					lpCS->aVFPType = 'M';
					lpCS->bBindColumn = FALSE;
					if (!IsVariableRef(lpCS->lField))
						lpCS->pStore = bUnicodeConversion ? SQLStoreMemoChar : SQLStoreMemoWChar;
					else
						lpCS->pStore = bUnicodeConversion ? SQLStoreMemoCharVar : SQLStoreMemoWCharVar;
					bBindCol = bGetDataExt;
				}
			}
			break;

		case SQL_WVARCHAR:
			lpCS->vData.SetString();
			if (lpCS->bCustomSchema)
			{
				if (lpCS->aVFPType == 'M' || lpCS->aVFPType == 'W')
				{
					lpCS->vData.ev_handle = pStmt->vGetDataBuffer.ev_handle;
					lpCS->pData = pStmt->pGetDataBuffer;
					lpCS->nBufferSize = VFP2C_ODBC_MAX_BUFFER;
					lpCS->bBindColumn = FALSE;
					bBindCol = bGetDataExt;
					if (lpCS->aVFPType == 'W' || lpCS->bBinary)
					{
						lpCS->nCType = SQL_C_WCHAR;
						if (!IsVariableRef(lpCS->lField))
							lpCS->pStore = SQLStoreMemoWChar;
						else
							lpCS->pStore = SQLStoreMemoWCharVar;
					}
					else
					{
						lpCS->nCType = SQL_C_CHAR;
						if (!IsVariableRef(lpCS->lField))
							lpCS->pStore = SQLStoreMemoChar;
						else
							lpCS->pStore = SQLStoreMemoCharVar;
					}
				}
				else
				{
					if (!AllocHandleEx(lpCS->vData,lpCS->nSize + 2))
						return E_INSUFMEMORY;
					LockHandle(lpCS->vData);
					lpCS->pData = HandleToPtr(lpCS->vData);
					lpCS->nBufferSize = lpCS->nSize + 2;

					if (lpCS->aVFPType == 'Q' || lpCS->bBinary)
						lpCS->nCType = SQL_C_WCHAR;
					else
						lpCS->nCType = SQL_C_CHAR;
					lpCS->bBindColumn = bBindCol;
					if (!IsVariableRef(lpCS->lField))
						lpCS->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpCS->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
			}
			else
			{
				if (lpCS->nSize <= VFP2C_VFP_MAX_CHARCOLUMN)
				{
					if (!AllocHandleEx(lpCS->vData,lpCS->nSize + 1))
						return E_INSUFMEMORY;
					LockHandle(lpCS->vData);
					lpCS->pData = HandleToPtr(lpCS->vData);
					lpCS->nBufferSize = lpCS->nSize + 1;
					lpCS->nCType = bUnicodeConversion ? SQL_C_CHAR : SQL_C_WCHAR;
					lpCS->aVFPType = 'C';
					lpCS->bBindColumn = bBindCol;
					if (!IsVariableRef(lpCS->lField))
						lpCS->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpCS->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
					bBindCol = bGetDataExt;
				}
				else
				{
					lpCS->vData.ev_handle = pStmt->vGetDataBuffer.ev_handle;
					lpCS->pData = pStmt->pGetDataBuffer;
					lpCS->nBufferSize = VFP2C_ODBC_MAX_BUFFER;
					lpCS->nCType = bUnicodeConversion ? SQL_C_CHAR : SQL_C_WCHAR;
					lpCS->aVFPType = 'M';
					lpCS->bBindColumn = FALSE;
					if (bUnicodeConversion)
					{
						if (!IsVariableRef(lpCS->lField))
							lpCS->pStore = SQLStoreMemoChar;
						else
							lpCS->pStore = SQLStoreMemoCharVar;
					}
					else
					{
						if (!IsVariableRef(lpCS->lField))
							lpCS->pStore = SQLStoreMemoWChar;
						else
							lpCS->pStore = SQLStoreMemoWCharVar;
					}
					bBindCol = bGetDataExt;
				}
			}
			break;

		case SQL_WLONGVARCHAR:
			lpCS->vData.SetString();
			if (lpCS->bCustomSchema)
			{
				if (lpCS->aVFPType == 'M' || lpCS->aVFPType == 'W')
				{
					lpCS->vData.ev_handle = pStmt->vGetDataBuffer.ev_handle;
					lpCS->pData = pStmt->pGetDataBuffer;
					lpCS->nBufferSize = VFP2C_ODBC_MAX_BUFFER;
					lpCS->bBindColumn = FALSE;
					bBindCol = bGetDataExt;
					if (lpCS->aVFPType == 'W' || lpCS->bBinary)
					{
						lpCS->nCType = SQL_C_WCHAR;
						if (!IsVariableRef(lpCS->lField))
							lpCS->pStore = SQLStoreMemoWChar;
						else
							lpCS->pStore = SQLStoreMemoWCharVar;
					}
					else
					{
						lpCS->nCType = SQL_C_CHAR;
						if (!IsVariableRef(lpCS->lField))
							lpCS->pStore = SQLStoreMemoChar;
						else
							lpCS->pStore = SQLStoreMemoCharVar;
					}
				}
				else
				{
					if (!AllocHandleEx(lpCS->vData,lpCS->nSize + 2))
						return E_INSUFMEMORY;
					LockHandle(lpCS->vData);
					lpCS->pData = HandleToPtr(lpCS->vData);
					lpCS->nBufferSize = lpCS->nSize + 2;
					if (lpCS->aVFPType == 'Q' || lpCS->bBinary)
						lpCS->nCType = SQL_C_WCHAR;
					else
						lpCS->nCType = SQL_C_CHAR;
					lpCS->bBindColumn = bBindCol;
					bBindCol = bGetDataExt;	
					if (!IsVariableRef(lpCS->lField))
						lpCS->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpCS->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
			}
			else
			{
				lpCS->vData.ev_handle = pStmt->vGetDataBuffer.ev_handle;
				lpCS->pData = pStmt->pGetDataBuffer;
				lpCS->nBufferSize = VFP2C_ODBC_MAX_BUFFER;
				lpCS->nCType = bUnicodeConversion ? SQL_C_CHAR : SQL_C_WCHAR;
				lpCS->aVFPType = 'M';
				lpCS->bBindColumn = FALSE;
				bBindCol = bGetDataExt;
				if (bUnicodeConversion)
				{
					if (!IsVariableRef(lpCS->lField))
						lpCS->pStore = SQLStoreMemoChar;
					else
						lpCS->pStore = SQLStoreMemoCharVar;
				}
				else
				{
					if (!IsVariableRef(lpCS->lField))
						lpCS->pStore = SQLStoreMemoWChar;
					else
						lpCS->pStore = SQLStoreMemoWCharVar;
				}
			}
			break;

		// bit (logical) data
		case SQL_BIT:
			if (lpCS->bCustomSchema)
			{
				if (lpCS->aVFPType == 'C' || lpCS->aVFPType == 'V')
				{
					lpCS->vData.SetString();
					if (!AllocHandleEx(lpCS->vData,lpCS->nSize+1))
						return E_INSUFMEMORY;
					LockHandle(lpCS->vData);
					lpCS->pData = HandleToPtr(lpCS->vData);
					lpCS->nBufferSize = lpCS->nSize + 1;
					lpCS->nCType = SQL_C_CHAR;
					lpCS->bBindColumn = bBindCol;
					if (!IsVariableRef(lpCS->lField))
						lpCS->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpCS->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
					break;
				}
			}

			lpCS->vData.SetLogical();
			lpCS->pData = &lpCS->vData.ev_length;
			lpCS->nCType = SQL_C_ULONG;
			lpCS->aVFPType = 'L';
			lpCS->bBindColumn = bBindCol;
			if (!IsVariableRef(lpCS->lField))
				lpCS->pStore = bBindCol ? SQLStoreByBinding : SQLStoreByGetData;
			else
				lpCS->pStore = bBindCol ? SQLStoreByBindingVar : SQLStoreByGetDataVar;
			break;

		// integral numeric types
		case SQL_TINYINT:
		case SQL_SMALLINT:
		case SQL_INTEGER:
			if (lpCS->bCustomSchema)
			{
				if (lpCS->aVFPType == 'C' || lpCS->aVFPType == 'V' || lpCS->aVFPType == 'Q')
				{
					lpCS->vData.SetString();
					if (!AllocHandleEx(lpCS->vData,lpCS->nSize + 1))
						return E_INSUFMEMORY;
					LockHandle(lpCS->vData);
					lpCS->pData = HandleToPtr(lpCS->vData);
					lpCS->nBufferSize = lpCS->nSize + 1;
					lpCS->nCType = SQL_C_CHAR;
					if (!IsVariableRef(lpCS->lField))
						lpCS->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpCS->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
				else if (lpCS->aVFPType == 'N' || lpCS->aVFPType == 'F' || lpCS->aVFPType == 'B')
				{
					lpCS->vData.SetNumeric(lpCS->nSize,lpCS->nScale);
					lpCS->pData = &lpCS->vData.ev_real;
					lpCS->nCType = SQL_C_DOUBLE;
					if (!IsVariableRef(lpCS->lField))
						lpCS->pStore = bBindCol ? SQLStoreByBinding : SQLStoreByGetData;
					else
						lpCS->pStore = bBindCol ? SQLStoreByBindingVar : SQLStoreByGetDataVar;
				}
				else if (lpCS->aVFPType == 'I')
				{
					lpCS->vData.SetInt();
					lpCS->pData = &lpCS->vData.ev_long;
					lpCS->nCType = SQL_C_SLONG;
					if (!IsVariableRef(lpCS->lField))
						lpCS->pStore = bBindCol ? SQLStoreByBinding : SQLStoreByGetData;
					else
						lpCS->pStore = bBindCol ? SQLStoreByBindingVar : SQLStoreByGetDataVar;
				}
				if (lpCS->aVFPType == 'L')
				{
					lpCS->vData.SetLogical();
					lpCS->pData = &lpCS->vData.ev_length;
					lpCS->nCType = SQL_C_SLONG;
					if (!IsVariableRef(lpCS->lField))
						lpCS->pStore = bBindCol ? SQLStoreByBinding : SQLStoreByGetData;
					else
						lpCS->pStore = bBindCol ? SQLStoreByBindingVar : SQLStoreByGetDataVar;
				}
				lpCS->bBindColumn = bBindCol;
			}
			else
			{
				if (lpCS->nSQLType == SQL_INTEGER && lpCS->bUnsigned)
				{
					lpCS->vData.SetUInt();
					lpCS->nCType = SQL_C_DOUBLE;
					lpCS->pData = &lpCS->vData.ev_real;
					lpCS->aVFPType = 'N';
					lpCS->nSize = 10;
					lpCS->nScale = 0;
				}
				else
				{
					lpCS->vData.SetInt();
					lpCS->nCType = SQL_C_SLONG;
					lpCS->pData = &lpCS->vData.ev_long;
					lpCS->aVFPType = 'I';
				}
				lpCS->bBindColumn = bBindCol;
				if (!IsVariableRef(lpCS->lField))
					lpCS->pStore = bBindCol ? SQLStoreByBinding : SQLStoreByGetData;
				else
					lpCS->pStore = bBindCol ? SQLStoreByBindingVar : SQLStoreByGetDataVar;
			}
			break;
		
		case SQL_BIGINT:
			if (lpCS->bCustomSchema)
			{
				if (lpCS->aVFPType == 'C' || lpCS->aVFPType == 'V' || lpCS->aVFPType == 'Q')
				{
					lpCS->vData.SetString();
					if (!AllocHandleEx(lpCS->vData,VFP2C_ODBC_MAX_BIGINT_LITERAL+1))
							return E_INSUFMEMORY;
					LockHandle(lpCS->vData);
					lpCS->pData = HandleToPtr(lpCS->vData);
					lpCS->nBufferSize = VFP2C_ODBC_MAX_BIGINT_LITERAL+1;
					lpCS->nCType = SQL_C_CHAR;
					lpCS->bBindColumn = bBindCol;
					if (!IsVariableRef(lpCS->lField))
						lpCS->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpCS->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
			}
			else
			{
				lpCS->vData.SetString();
				if (!AllocHandleEx(lpCS->vData,VFP2C_ODBC_MAX_BIGINT_LITERAL+1))
					return E_INSUFMEMORY;
				LockHandle(lpCS->vData);
				lpCS->pData = HandleToPtr(lpCS->vData);
				lpCS->nSize = VFP2C_ODBC_MAX_BIGINT_LITERAL;
				lpCS->nBufferSize = VFP2C_ODBC_MAX_BIGINT_LITERAL+1;
				lpCS->aVFPType = 'C';
				lpCS->nCType = SQL_C_CHAR;
				lpCS->bBindColumn = bBindCol;
				if (!IsVariableRef(lpCS->lField))
					lpCS->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
				else
					lpCS->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
			}
			break;
		
		case SQL_NUMERIC:
		case SQL_DECIMAL:
			if (lpCS->bCustomSchema)
			{
					if (lpCS->aVFPType == 'C' || lpCS->aVFPType == 'V')
					{
						lpCS->vData.SetString();
						if (!AllocHandleEx(lpCS->vData,lpCS->nSize + 1))
								return E_INSUFMEMORY;
						LockHandle(lpCS->vData);
						lpCS->pData = HandleToPtr(lpCS->vData);
						lpCS->nBufferSize = lpCS->nSize + 1;
						lpCS->nCType = SQL_C_CHAR;
						lpCS->bBindColumn = bBindCol;
						if (!IsVariableRef(lpCS->lField))
							lpCS->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
						else
							lpCS->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
					}
					else if (lpCS->aVFPType == 'I')
					{
						lpCS->vData.SetInt();
						lpCS->pData = &lpCS->vData.ev_long;
						lpCS->nCType = SQL_C_LONG;
						lpCS->bBindColumn = bBindCol;
						if (!IsVariableRef(lpCS->lField))
							lpCS->pStore = bBindCol ? SQLStoreByBinding : SQLStoreByGetData;
						else
							lpCS->pStore = bBindCol ? SQLStoreByBindingVar : SQLStoreByGetDataVar;
					}
					else if (lpCS->aVFPType == 'N' || lpCS->aVFPType == 'B' || lpCS->aVFPType == 'F')
					{
						lpCS->vData.SetNumeric(lpCS->aVFPType == 'N' ? (short)lpCS->nSize : 20, lpCS->nScale);
						lpCS->pData = &lpCS->vData.ev_real;
						lpCS->nCType = SQL_DOUBLE;
						lpCS->bBindColumn = bBindCol;
						if (!IsVariableRef(lpCS->lField))
							lpCS->pStore = bBindCol ? SQLStoreByBinding : SQLStoreByGetData;
						else
							lpCS->pStore = bBindCol ? SQLStoreByBindingVar : SQLStoreByGetDataVar;
					}
					else if (lpCS->aVFPType == 'Y')
					{
						lpCS->vData.SetCurrency();
						lpCS->pData = lpCS->aNumeric;
						lpCS->nSize = VFP2C_VFP_CURRENCY_PRECISION;
						lpCS->nScale = VFP2C_VFP_CURRENCY_SCALE;
						lpCS->nBufferSize = VFP2C_ODBC_MAX_CURRENCY_LITERAL;
						lpCS->nCType = SQL_C_CHAR;
						lpCS->aVFPType = 'Y';
						lpCS->bBindColumn = bBindCol;
						if (!IsVariableRef(lpCS->lField))
							lpCS->pStore = bBindCol ? SQLStoreCurrencyByBinding : SQLStoreCurrencyByGetData;
						else
							lpCS->pStore = bBindCol ? SQLStoreCurrencyByBindingVar : SQLStoreCurrencyByGetDataVar;
					}
			}
			else
			{
				if (!lpCS->bMoney)
				{
					lpCS->vData.SetNumeric(lpCS->nSize, lpCS->nScale);
					lpCS->pData = &lpCS->vData.ev_real;
					lpCS->nCType = SQL_C_DOUBLE;
					lpCS->aVFPType = 'N';
					lpCS->bBindColumn = bBindCol;
					if (!IsVariableRef(lpCS->lField))
						lpCS->pStore = bBindCol ? SQLStoreByBinding : SQLStoreByGetData;
					else
						lpCS->pStore = bBindCol ? SQLStoreByBindingVar : SQLStoreByGetDataVar;
				}
				else
				{
					lpCS->vData.SetCurrency();
					lpCS->pData = lpCS->aNumeric;
					lpCS->nSize = VFP2C_VFP_CURRENCY_PRECISION;
					lpCS->nScale = VFP2C_VFP_CURRENCY_SCALE;
					lpCS->nBufferSize = VFP2C_ODBC_MAX_CURRENCY_LITERAL;
					lpCS->nCType = SQL_C_CHAR;
					lpCS->aVFPType = 'Y';
					lpCS->bBindColumn = bBindCol;
					if (!IsVariableRef(lpCS->lField))
						lpCS->pStore = bBindCol ? SQLStoreCurrencyByBinding : SQLStoreCurrencyByGetData;
					else
						lpCS->pStore = bBindCol ? SQLStoreCurrencyByBindingVar : SQLStoreCurrencyByGetDataVar;
				}
			}
			break;

		case SQL_REAL:
		case SQL_FLOAT:
		case SQL_DOUBLE:
			if (lpCS->bCustomSchema)
			{
				if (lpCS->aVFPType == 'C' || lpCS->aVFPType == 'V' || lpCS->aVFPType == 'Q')
				{
					lpCS->vData.SetString();
					if (!AllocHandleEx(lpCS->vData,lpCS->nSize + 1))
						return E_INSUFMEMORY;
					LockHandle(lpCS->vData);
					lpCS->pData = HandleToPtr(lpCS->vData);
					lpCS->nBufferSize = lpCS->nSize + 1;
					lpCS->nCType = SQL_C_CHAR;
					lpCS->bBindColumn = bBindCol;
					if (!IsVariableRef(lpCS->lField))
						lpCS->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpCS->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
				else if (lpCS->aVFPType == 'N' || lpCS->aVFPType == 'B' || lpCS->aVFPType == 'F')
				{
					lpCS->vData.SetNumeric(lpCS->nSize, lpCS->nScale);
					lpCS->pData = &lpCS->vData.ev_real;
					lpCS->nCType = SQL_C_DOUBLE;
					lpCS->bBindColumn = bBindCol;
					if (!IsVariableRef(lpCS->lField))
						lpCS->pStore = bBindCol ? SQLStoreByBinding : SQLStoreByGetData;
					else
						lpCS->pStore = bBindCol ? SQLStoreByBindingVar : SQLStoreByGetDataVar;
				}
			}
			else
			{
				lpCS->vData.SetDouble();
				lpCS->pData = &lpCS->vData.ev_real;
				lpCS->nCType = SQL_C_DOUBLE;
				lpCS->aVFPType = 'B';
				lpCS->bBindColumn = bBindCol;
				if (!IsVariableRef(lpCS->lField))
					lpCS->pStore = bBindCol ? SQLStoreByBinding : SQLStoreByGetData;
				else
					lpCS->pStore = bBindCol ? SQLStoreByBindingVar : SQLStoreByGetDataVar;
			}
			break;

		// this is only a date not a datetime ..	
		case SQL_DATE:
			if (lpCS->bCustomSchema)
			{
				if ((lpCS->aVFPType == 'C' || lpCS->aVFPType == 'V') && !lpCS->bBinary)
				{
					if (!AllocHandleEx(lpCS->vData,lpCS->nSize + 1))
						return E_INSUFMEMORY;
					LockHandle(lpCS->vData);
					lpCS->pData = HandleToPtr(lpCS->vData);
					lpCS->nBufferSize = lpCS->nSize + 1;
					lpCS->nCType = SQL_C_CHAR;
					if (!IsVariableRef(lpCS->lField))
						lpCS->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpCS->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
				else if (lpCS->aVFPType == 'Q' ||
					((lpCS->aVFPType == 'C' || lpCS->aVFPType == 'V') && lpCS->bBinary))
				{
					if (!AllocHandleEx(lpCS->vData,sizeof(SQL_TIMESTAMP_STRUCT)))
						return E_INSUFMEMORY;
					LockHandle(lpCS->vData);
					lpCS->pData = HandleToPtr(lpCS->vData);
					lpCS->nBufferSize = sizeof(SQL_TIMESTAMP_STRUCT);
					lpCS->nCType = SQL_C_TIMESTAMP;
					if (!IsVariableRef(lpCS->lField))
						lpCS->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpCS->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
				else if (lpCS->aVFPType == 'T' || lpCS->aVFPType == 'D')
				{
					if (lpCS->aVFPType == 'T')
						lpCS->vData.SetDateTime();
					else
						lpCS->vData.SetDate();
					lpCS->pData = &lpCS->sDateTime;
					lpCS->nCType = SQL_DATE;
					if (!IsVariableRef(lpCS->lField))
						lpCS->pStore = bBindCol ? SQLStoreDateByBinding : SQLStoreDateByGetData;
					else
						lpCS->pStore = bBindCol ? SQLStoreDateByBindingVar : SQLStoreDateByGetDataVar;
				}
				lpCS->bBindColumn = bBindCol;
			}
			else
			{
				lpCS->vData.SetDate();
				lpCS->pData = &lpCS->sDateTime;
				lpCS->nCType = SQL_C_DATE;
				lpCS->aVFPType = 'D';
				lpCS->bBindColumn = bBindCol;
				if (!IsVariableRef(lpCS->lField))
					lpCS->pStore = bBindCol ? SQLStoreDateByBinding : SQLStoreDateByGetData;
				else
					lpCS->pStore = bBindCol ? SQLStoreDateByBindingVar : SQLStoreDateByGetDataVar;
			}
			break;
		
		// this is a datetime
		case SQL_TIMESTAMP:
			if (lpCS->bCustomSchema)
			{
				if (lpCS->aVFPType == 'D')
				{
					lpCS->vData.SetDate();
					lpCS->pData = &lpCS->sDateTime;
					lpCS->nCType = SQL_C_TIMESTAMP;
					if (!IsVariableRef(lpCS->lField))
						lpCS->pStore = bBindCol ? SQLStoreDateByBinding : SQLStoreDateByGetData;
					else
						lpCS->pStore = bBindCol ? SQLStoreDateByBindingVar : SQLStoreDateByGetDataVar;
				}
				else if (lpCS->aVFPType == 'T')
				{
					lpCS->vData.SetDateTime();
					lpCS->pData = &lpCS->sDateTime;
					lpCS->nCType = SQL_C_TIMESTAMP;
					if (!IsVariableRef(lpCS->lField))
						lpCS->pStore = bBindCol ? SQLStoreDateTimeByBinding : SQLStoreDateTimeByGetData;
					else
						lpCS->pStore = bBindCol ? SQLStoreDateTimeByBindingVar : SQLStoreDateTimeByGetDataVar;
				}
				else if (lpCS->aVFPType == 'Q' || 
					((lpCS->aVFPType == 'C' || lpCS->aVFPType == 'V') && lpCS->bBinary))
				{
					lpCS->vData.SetString();
					if (!AllocHandleEx(lpCS->vData,sizeof(SQL_TIMESTAMP_STRUCT)))
						return E_INSUFMEMORY;
					LockHandle(lpCS->vData);
					lpCS->pData = HandleToPtr(lpCS->vData);
					lpCS->nSize = lpCS->nBufferSize = sizeof(SQL_TIMESTAMP_STRUCT);
					lpCS->nCType = SQL_C_TIMESTAMP;
					if (!IsVariableRef(lpCS->lField))
						lpCS->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpCS->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
				else if (lpCS->aVFPType == 'C' || lpCS->aVFPType == 'V')
				{
					lpCS->vData.SetString();
					if (!AllocHandleEx(lpCS->vData,lpCS->nSize + 1))
						return E_INSUFMEMORY;
					LockHandle(lpCS->vData);
					lpCS->pData = HandleToPtr(lpCS->vData);
					lpCS->nBufferSize = lpCS->nSize + 1;
					lpCS->nCType = SQL_C_CHAR;
					if (!IsVariableRef(lpCS->lField))
						lpCS->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpCS->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
				lpCS->bBindColumn = bBindCol;
			}
			else
			{
				lpCS->vData.SetDateTime();
				lpCS->pData = &lpCS->sDateTime;
				lpCS->nCType = SQL_C_TIMESTAMP;
				lpCS->aVFPType = 'T';
				lpCS->bBindColumn = bBindCol;
				if (!IsVariableRef(lpCS->lField))
					lpCS->pStore = bBindCol ? SQLStoreDateTimeByBinding : SQLStoreDateTimeByGetData;
				else
					lpCS->pStore = bBindCol ? SQLStoreDateTimeByBindingVar : SQLStoreDateTimeByGetDataVar;
			}
			break;

		case SQL_TIME:
			lpCS->vData.SetString();
			if (!AllocHandleEx(lpCS->vData,lpCS->nSize+1))
				return E_INSUFMEMORY;
			LockHandle(lpCS->vData);
			lpCS->pData = HandleToPtr(lpCS->vData);
			lpCS->nBufferSize = lpCS->nSize + 1;
			lpCS->nCType = SQL_C_CHAR;
			lpCS->aVFPType = 'C';
			lpCS->bBindColumn = bBindCol;
			if (!IsVariableRef(lpCS->lField))
				lpCS->pStore = bBindCol ? SQLStoreByBinding : SQLStoreByGetData;
			else
				lpCS->pStore = bBindCol ? SQLStoreByBindingVar : SQLStoreByGetDataVar;
			break;

		// GUID
		case SQL_GUID:
			if (lpCS->bCustomSchema)
			{
				if (lpCS->aVFPType == 'Q')
				{
					lpCS->vData.SetString();
					if (!AllocHandleEx(lpCS->vData,lpCS->nSize + 1))
						return E_INSUFMEMORY;
					LockHandle(lpCS->vData);
					lpCS->pData = HandleToPtr(lpCS->vData);
					lpCS->vData.ev_length = lpCS->nSize;
					lpCS->nBufferSize = lpCS->nSize + 1;
					lpCS->nCType = SQL_C_GUID;
					lpCS->aVFPType = 'C';
					lpCS->bBindColumn = bBindCol;
					if (!IsVariableRef(lpCS->lField))
						lpCS->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpCS->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
					break;
				}
			}

			lpCS->vData.SetString();
			if (!AllocHandleEx(lpCS->vData,VFP2C_ODBC_MAX_GUID_LITERAL + 1))
				return E_INSUFMEMORY;
			LockHandle(lpCS->vData);
			lpCS->pData = HandleToPtr(lpCS->vData);
			lpCS->vData.ev_length = VFP2C_ODBC_MAX_GUID_LITERAL;
			lpCS->nBufferSize = VFP2C_ODBC_MAX_GUID_LITERAL + 1;
			lpCS->nCType = SQL_C_CHAR;
			lpCS->aVFPType = 'C';
			lpCS->bBindColumn = bBindCol;
			if (!IsVariableRef(lpCS->lField))
				lpCS->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
			else
				lpCS->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
			break;
		
		default:
			if (lpCS->bCustomSchema)
			{
				if (lpCS->aVFPType == 'C' || lpCS->aVFPType == 'V' || lpCS->aVFPType == 'Q')
				{
					lpCS->vData.SetString();
					lpCS->vData.ev_handle = pStmt->vGetDataBuffer.ev_handle;
					lpCS->pData = pStmt->pGetDataBuffer;
					lpCS->nBufferSize = VFP2C_ODBC_MAX_BUFFER;
					lpCS->nCType = SQL_C_CHAR;
					lpCS->bBindColumn = FALSE;
					bBindCol = bGetDataExt;
					if (!IsVariableRef(lpCS->lField))
						lpCS->pStore = SQLStoreCharByGetData;
					else
						lpCS->pStore = SQLStoreCharByGetDataVar;
				}
				else if (lpCS->aVFPType == 'M' || lpCS->aVFPType == 'W')
				{
					lpCS->vData.SetString();
					lpCS->vData.ev_handle = pStmt->vGetDataBuffer.ev_handle;
					lpCS->pData = pStmt->pGetDataBuffer;
					lpCS->nBufferSize = VFP2C_ODBC_MAX_BUFFER;
					lpCS->bBindColumn = FALSE;
					bBindCol = bGetDataExt;
					if (lpCS->bBinary || lpCS->aVFPType == 'W')
					{
						lpCS->nCType = SQL_C_BINARY;
						if (!IsVariableRef(lpCS->lField))
							lpCS->pStore = SQLStoreMemoBinary;
						else
							lpCS->pStore = SQLStoreMemoBinaryVar;
					}
					else
					{
						lpCS->nCType = SQL_C_CHAR;
						if (!IsVariableRef(lpCS->lField))
							lpCS->pStore = SQLStoreMemoChar;
						else
							lpCS->pStore = SQLStoreMemoCharVar;
					}
				}
				else if (lpCS->aVFPType == 'I')
				{
					lpCS->vData.SetInt();
					lpCS->pData = &lpCS->vData.ev_long;
					lpCS->nCType = SQL_C_SLONG;
					lpCS->bBindColumn = bBindCol;
					if (!IsVariableRef(lpCS->lField))
						lpCS->pStore = bBindCol ? SQLStoreByBinding : SQLStoreByGetData;
					else
						lpCS->pStore = bBindCol ? SQLStoreByBindingVar : SQLStoreByGetDataVar;
				}
				else if (lpCS->aVFPType == 'B' || lpCS->aVFPType == 'N' || lpCS->aVFPType == 'F')
				{
					lpCS->vData.SetNumeric(lpCS->nSize, lpCS->nScale);
					lpCS->pData = &lpCS->vData.ev_real;
					lpCS->nCType = SQL_C_DOUBLE;
					lpCS->bBindColumn = bBindCol;
					if (!IsVariableRef(lpCS->lField))
						lpCS->pStore = bBindCol ? SQLStoreByBinding : SQLStoreByGetData;
					else
						lpCS->pStore = bBindCol ? SQLStoreByBindingVar : SQLStoreByGetDataVar;
				}
				else if (lpCS->aVFPType == 'Y')
				{
					lpCS->vData.SetCurrency();
					lpCS->pData = lpCS->aNumeric;
					lpCS->nSize = VFP2C_VFP_CURRENCY_PRECISION;
					lpCS->nScale = VFP2C_VFP_CURRENCY_SCALE;
					lpCS->nBufferSize = VFP2C_ODBC_MAX_CURRENCY_LITERAL;
					lpCS->nCType = SQL_C_CHAR;
					lpCS->bBindColumn = bBindCol;
					if (!IsVariableRef(lpCS->lField))
						lpCS->pStore = bBindCol ? SQLStoreCurrencyByBinding : SQLStoreCurrencyByGetData;
					else
						lpCS->pStore = bBindCol ? SQLStoreCurrencyByBindingVar : SQLStoreCurrencyByGetDataVar;
				}
			}
			else
			{
				if (lpCS->nDisplaySize > VFP2C_VFP_MAX_CHARCOLUMN)
				{
					lpCS->vData.SetString();
					lpCS->vData.ev_handle = pStmt->vGetDataBuffer.ev_handle;
					lpCS->pData = pStmt->pGetDataBuffer;
					lpCS->nBufferSize = VFP2C_ODBC_MAX_BUFFER;
					lpCS->nCType = SQL_C_CHAR;
					lpCS->aVFPType = 'M';
					lpCS->bBindColumn = FALSE;
					if (!IsVariableRef(lpCS->lField))
						lpCS->pStore = SQLStoreMemoChar;
					else
						lpCS->pStore = SQLStoreMemoCharVar;
					bBindCol = bGetDataExt;
				}
				else
				{
					lpCS->vData.SetString();
					if (!AllocHandleEx(lpCS->vData,lpCS->nDisplaySize + 1))
						return E_INSUFMEMORY;
					LockHandle(lpCS->vData);
					lpCS->pData = HandleToPtr(lpCS->vData);
					lpCS->nSize = lpCS->nDisplaySize;
					lpCS->nBufferSize = lpCS->nDisplaySize + 1;
					lpCS->nCType = SQL_C_CHAR;
					lpCS->aVFPType = 'C';
					lpCS->bBindColumn = bBindCol;
					if (!IsVariableRef(lpCS->lField))
						lpCS->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpCS->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
			}
	} // switch(lpCS-nType)

	lpCS++;

	} // while(nNumberOfCols--)
	
	return 0;
}

BOOL _stdcall SQLTypeConvertable(SQLSMALLINT nSQLType, char aVFPType)
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

int _stdcall SQLParseCursorSchema(LPSQLSTATEMENT pStmt)
{
	LPSQLCOLUMNDATA lpCS;
	char *pSchema;
	char aColName[VFP2C_ODBC_MAX_COLUMN_NAME];

	if (!pStmt->pCursorSchema)
		return 0;

	pSchema = AtEx(pStmt->pCursorSchema,'|',pStmt->nResultset-1);

	if (pSchema)
	{
		do
		{
			// try to match an identifier (fieldname)
			if (!match_identifier(&pSchema,aColName,VFP2C_ODBC_MAX_COLUMN_NAME))
			{
				// if no normal identifier found, try to match a quoted (enclosed in ") identifier
				if (match_quoted_identifier(&pSchema,aColName,VFP2C_ODBC_MAX_COLUMN_NAME))
					SQLFixColumnName(aColName);
				else
				{
					SaveCustomError("SQLExecEx","Cursorschema contained invalid data near '%10S'.", pSchema);
					return E_APIERROR;
				}
			}

			// get pointer to SQLCOLUMNDATA struct for the specified column
			lpCS = SQLFindColumn(pStmt,aColName);
			if (!lpCS)
			{
				SaveCustomError("SQLExecEx","Column '%S' not contained in resultset but specified in cursorschema.", aColName);
				return E_APIERROR;
			}

			// match the VFP datatype .. upper or lowercase ..
			if (!match_one_chr(&pSchema,"CcVvQqWwMmNnBbFfIiYyDdTtLl",(char*)&lpCS->aVFPType))
			{
				SaveCustomError("SQLExecEx", "Datatype in cursorschema for column '%S' is invalid.", lpCS->aColName);
				return E_APIERROR;
			}

			// convert to uppercase so we don't have to deal with case anymore ..
			lpCS->aVFPType = ToUpper(lpCS->aVFPType);

			// check if column is convertable to specified type
			if (!SQLTypeConvertable(lpCS->nSQLType,lpCS->aVFPType))
			{
				SaveCustomError("SQLExecEx", "Datatype conversion for column '%S' to VFP type '%s' not supported.", aColName, lpCS->aVFPType);
				return E_APIERROR;
			}

			if (lpCS->aVFPType == 'C' || lpCS->aVFPType == 'V' || lpCS->aVFPType == 'Q')
			{
				if (!match_chr(&pSchema,'('))
				{
					SaveCustomError("SQLExecEx", "Invalid cursorschema for column '%S', expected '('.", aColName);
					return E_APIERROR;
				}
				if (!match_int(&pSchema,(int*)&lpCS->nSize))
				{
					SaveCustomError("SQLExecEx", "Invalid cursorschema for column '%S', expected column size.", aColName);
					return E_APIERROR;
				}
				if (!match_chr(&pSchema,')'))
				{
					SaveCustomError("SQLExecEx", "Invalid cursorschema for column '%S', expected ')'.", aColName);
					return E_APIERROR;
				}
				// set binary flag for varbinary type
				if (lpCS->aVFPType == 'Q')
					lpCS->bBinary = TRUE;
			}
			else if (lpCS->aVFPType == 'N' || lpCS->aVFPType == 'F')
			{
				if (match_chr(&pSchema,'('))
				{
					if (!match_int(&pSchema,(int*)&lpCS->nSize))
					{
						SaveCustomError("SQLExecEx", "Invalid cursorschema for column '%S', expected column precision.", aColName);
						return E_APIERROR;
					}

					if (match_chr(&pSchema,','))
					{
						if (!match_short(&pSchema,&lpCS->nScale))
						{
							SaveCustomError("SQLExecEx","Invalid cursorschema for column '%S', expected column scale.", aColName);
							return E_APIERROR;
						}
					}
					else
						lpCS->nScale = 0;

					if (!match_chr(&pSchema,')'))
					{
						SaveCustomError("SQLExecEx", "Invalid cursorschema for column '%S', expected ')'.", aColName);
						return E_APIERROR;
					}
				}
				// set default size
				else
				{
					lpCS->nSize = 10;
					lpCS->nScale = 0;
				}
			}
			else if (lpCS->aVFPType == 'B')
			{
				if (match_chr(&pSchema,'('))
				{
					if (!match_short(&pSchema,&lpCS->nScale))
					{
						SaveCustomError("SQLExecEx", "Invalid cursorschema for column '%S', expected column size.", aColName);
						return E_APIERROR;
					}
					if (!match_chr(&pSchema,')'))
					{
						SaveCustomError("SQLExecEx","Invalid cursorschema for column '%S', expected ')'.", aColName);
						return E_APIERROR;
					}
					lpCS->nSize = VFP2C_VFP_DOUBLE_PRECISION;
				}
				else
				{
					lpCS->nSize = 8;
					lpCS->nScale = 2;
				}
			}
			// set binary flag for blob datatype
			else if (lpCS->aVFPType == 'W')
				lpCS->bBinary = TRUE;
			
			if (match_istr(&pSchema,"NULL"))
				lpCS->bNullable = TRUE;

			if (match_istr(&pSchema,"NOCPTRANS"))
				lpCS->bBinary = TRUE;

			lpCS->bCustomSchema = TRUE;

		} while (match_chr(&pSchema,','));	// do while there are new ',' 

		if (!match_chr(&pSchema,'\0')) // if not at end of string the cursor schema is invalid
		{
			SaveCustomError("SQLExecEx","Cursorschema contained invalid data near '%20S'.",pSchema);
			return E_APIERROR;
		}
	}

	return 0;
}

int _stdcall SQLParseCursorSchemaEx(LPSQLSTATEMENT pStmt, char *pCursor)
{
	LPSQLCOLUMNDATA lpCS;
	Value vValue = {'0'};
	char *pValue;
	Locator lArrayLoc;
	int nErrorNo, nColumns;
	char aArrayName[VFP2C_MAX_FUNCTIONBUFFER];
	char aExeBuffer[VFP2C_MAX_FUNCTIONBUFFER];
		
	// create unique array name .. since pStmt is a dynamically allocated pointer it's value is
	// always unique .. so we can use it to build a variable name ..
	sprintfex(aArrayName,"__VFP2C_ODBC_ARRAY_%U",pStmt);
	sprintfex(aExeBuffer,"AFIELDS(%S,'%S')",aArrayName,pCursor);

	if (nErrorNo = _Execute(aExeBuffer))
		return nErrorNo;

	if (nErrorNo = FindFoxVar(aArrayName,&lArrayLoc))
		goto ErrorOut;

	ResetArrayLocator(lArrayLoc, 2);
	nColumns = _ALen(lArrayLoc.l_NTI, AL_SUBSCRIPT1);
	
	while (nColumns--)
	{
		lArrayLoc.l_sub1++;
		
		// fieldname
		lArrayLoc.l_sub2 = 1;
		if (nErrorNo = _Load(&lArrayLoc, &vValue))
			goto ErrorOut;
		
		if (!NullTerminateValue(vValue))
		{
			nErrorNo = E_INSUFMEMORY;
			goto ErrorOut;
		}
		pValue = HandleToPtr(vValue);

		lpCS = SQLFindColumn(pStmt,pValue);
		if (!lpCS)
		{
			FreeHandle(vValue);
			SaveCustomError("SQLExecEx","Column '%S' not contained in resultset but exists in cursor.",pValue);
			nErrorNo = E_APIERROR;
			goto ErrorOut;
		}
		FreeHandle(vValue);

		// fieldtype
		lArrayLoc.l_sub2 = 2;
		if (nErrorNo = _Load(&lArrayLoc, &vValue))
			goto ErrorOut;
		
		pValue = HandleToPtr(vValue);
		lpCS->aVFPType = *pValue;
		FreeHandle(vValue);

		// precision/width
		lArrayLoc.l_sub2 = 3;
		if (nErrorNo = _Load(&lArrayLoc, &vValue))
			goto ErrorOut;
		lpCS->nSize = (SQLUINTEGER)vValue.ev_long;

		// scale
		lArrayLoc.l_sub2 = 4;
		if (nErrorNo = _Load(&lArrayLoc, &vValue))
			goto ErrorOut;
		lpCS->nScale = (SQLSMALLINT)vValue.ev_long;

		// binary
		lArrayLoc.l_sub2 = 6;
		if (nErrorNo = _Load(&lArrayLoc, &vValue))
			goto ErrorOut;
		lpCS->bBinary = (SQLSMALLINT)vValue.ev_length;

		lpCS->bCustomSchema = TRUE;
	}

	sprintfex(aExeBuffer, "RELEASE %S", aArrayName);
	nErrorNo = _Execute(aExeBuffer);
	return nErrorNo;

	ErrorOut:
		sprintfex(aExeBuffer,"RELEASE %S",aArrayName);
		_Execute(aExeBuffer);
		return nErrorNo;
}

int _stdcall SQLBindVariableLocators(LPSQLSTATEMENT pStmt)
{
	LPSQLCOLUMNDATA lpCS = pStmt->pColumnData;
	int nErrorNo, xj;
	char *pSchema;
	char aCursorOrVar[VFP2C_VFP_MAX_CURSOR_NAME];
	char aField[VFP2C_VFP_MAX_COLUMN_NAME];
	
	pSchema = AtEx(pStmt->pCursorNames,'|',pStmt->nResultset-1);
	if (!pSchema)
	{
		SaveCustomError("SQLExecEx", "No variable list specified for resultset '%I'", pStmt->nResultset);
		return E_APIERROR;
	}
	xj = pStmt->nNoOfCols;

	do 
	{
		// fill the reference to the specified variable or fieldname
		if (match_identifier(&pSchema,aCursorOrVar,VFP2C_VFP_MAX_CURSOR_NAME))
		{
			if (match_chr(&pSchema,'.'))
			{
				if (match_identifier(&pSchema,aField,VFP2C_VFP_MAX_COLUMN_NAME))
				{
					if (nErrorNo = FindFoxFieldC(aField,&lpCS->lField,aCursorOrVar))
						return nErrorNo;
				}
				else
				{
					SaveCustomError("SQLExecEx","Variable list contained invalid data near '%10S', expected columnname.", pSchema);
					return E_APIERROR;
				}
			}
			else
			{
				if (nErrorNo = FindFoxVar(aCursorOrVar,&lpCS->lField))
					return nErrorNo;
			}
		}
		else
		{
			SaveCustomError("SQLExecEx","Variable list contained invalid data near '%10S', expected columnname.", pSchema);
			return E_APIERROR;
		}

		lpCS++;

		if (!--xj)
			break;

	} while (match_chr(&pSchema,','));

	if (!match_chr(&pSchema,'\0'))
	{
		if (!xj)
		{
			SaveCustomError("SQLExecEx", "Variable list contained invalid data near '%10S'.", pSchema);
			return E_APIERROR;
		}
		else
		{
			SaveCustomError("SQLExecEx", "Variable list contained more variables than columns present in resultset.");
			return E_APIERROR;
		}
	}
	else if (xj)
	{
		SaveCustomError("SQLExecEx", "Variable list contained less variables than columns present in resultset.");
		return E_APIERROR;
	}

	return 0;
}

int _stdcall SQLBindVariableLocatorsEx(LPSQLSTATEMENT pStmt)
{
	LPSQLCOLUMNDATA lpCS = pStmt->pColumnData;
	int nColNo = pStmt->nNoOfCols, nErrorNo;
	
	while (nColNo--)
	{
		if (!IsVariableRef(lpCS->lField) && (lpCS->aVFPType == 'M' || lpCS->aVFPType == 'W'))
		{
			if (nErrorNo = MemoChan(lpCS->lField.l_where,&lpCS->hMemoFile))
				return nErrorNo;
		}
		lpCS++;
	}
	return 0;
}

LPSQLCOLUMNDATA _stdcall SQLFindColumn(LPSQLSTATEMENT pStmt, char *pColName)
{
	LPSQLCOLUMNDATA lpCS = pStmt->pColumnData;
	int xj;

	if (!lpCS)
		return 0;

	for (xj = 1; xj <= pStmt->nNoOfCols; xj++)
	{
		if (StrIEqual((char*)lpCS->aColName,pColName))
			return lpCS;

		lpCS++;
	}

	return 0;
}

void _stdcall SQLFixColumnName(char *pCol)
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

int _stdcall SQLCreateCursor(LPSQLSTATEMENT pStmt, char *pCursorName)
{
	LPSQLCOLUMNDATA lpCS = pStmt->pColumnData;
	Locator lArrayLoc;
	NTI nVarNti;
	int nErrorNo, nColNo;
	char *pChar;
	StringValue vChar;
	IntValue vNumeric;
	LogicalValue vLogical;
	char aArrayName[VFP2C_MAX_FUNCTIONBUFFER];
	char aExeBuffer[VFP2C_MAX_FUNCTIONBUFFER];
		
	// create unique array name .. since pStmt is a dynamically allocated pointer it's value is
	// always unique .. so we can use it to build a variable name ..
	sprintfex(aArrayName,"__VFP2C_ODBC_ARRAY_%U",pStmt);

	lArrayLoc.l_subs = 2; // 2 dimensional array
	lArrayLoc.l_sub1 = pStmt->nNoOfCols; // rows = no of columns
	lArrayLoc.l_sub2 = 6; // we only need the first 6 dimensions of the AFIELDS array

	nVarNti = _NewVar(aArrayName,&lArrayLoc,NV_PRIVATE);
	if (nVarNti < 0)
	{
		nErrorNo = -nVarNti;
		goto ErrorOut;
	}

	ResetArrayLocator(lArrayLoc, 2);

	if (!AllocHandleEx(vChar,VFP2C_ODBC_MAX_COLUMN_NAME))
	{
		nErrorNo = E_INSUFMEMORY;
		goto ErrorOut;
	}
	LockHandle(vChar);
	pChar = HandleToPtr(vChar);

	for (nColNo = 1; nColNo <= pStmt->nNoOfCols; nColNo++)
	{
		if ((lpCS->aVFPType == 'C' || lpCS->aVFPType == 'V' || lpCS->aVFPType == 'Q') && lpCS->nSize > 254)
		{
			SaveCustomErrorEx("SqlExecEx", "Invalid cursor schema for field '%S': length '%I'", 0, lpCS->aColName, lpCS->nSize);
			nErrorNo = E_APIERROR;
			goto ErrorOut;
		}
		else if ((lpCS->aVFPType == 'N' || lpCS->aVFPType == 'F') && (lpCS->nSize > 20 || lpCS->nScale > 19))
		{
			SaveCustomErrorEx("SqlExecEx", "Invalid cursor schema for field '%S': length '%I', scale: '%I'", 0, lpCS->aColName, lpCS->nSize, lpCS->nScale);
			nErrorNo = E_APIERROR;
			goto ErrorOut;
		}
		else if (lpCS->aVFPType == 'B' && lpCS->nSize > 18)
		{
			SaveCustomErrorEx("SqlExecEx", "Invalid cursor schema for field '%S': length '%I', scale: '%I'", 0, lpCS->aColName, lpCS->nSize, lpCS->nScale);
			nErrorNo = E_APIERROR;
			goto ErrorOut;
		}

		lArrayLoc.l_sub1++;

		// store fieldname
		lArrayLoc.l_sub2 = 1;
		vChar.ev_length = strcpyex(pChar,(char*)lpCS->aColName);
		if (nErrorNo = _Store(&lArrayLoc, &vChar))
			goto ErrorOut;

		// store fieldtype
		lArrayLoc.l_sub2 = 2;
		vChar.ev_length = 1;
		*pChar = lpCS->aVFPType;
		if (nErrorNo = _Store(&lArrayLoc, &vChar))
			goto ErrorOut;

		// store field width/precision
		lArrayLoc.l_sub2 = 3;
		// if the VFP datatype is none of the below ones .. the field width is unneccesary
		if (lpCS->aVFPType == 'C' || lpCS->aVFPType == 'V' || lpCS->aVFPType == 'Q' ||
			lpCS->aVFPType == 'F' || lpCS->aVFPType == 'N')
			vNumeric.ev_long = lpCS->nSize;
		else
			vNumeric.ev_long = 0;

		if (nErrorNo = _Store(&lArrayLoc, &vNumeric))
			goto ErrorOut;

		// store field scale
		lArrayLoc.l_sub2 = 4;
		// if the VFP datatype if none of the below ones .. the field scale is unneccesary
		if (lpCS->aVFPType == 'N' || lpCS->aVFPType == 'F')
			vNumeric.ev_long = lpCS->nScale;
		else
			vNumeric.ev_long = 0;

		if (nErrorNo = _Store(&lArrayLoc, &vNumeric))
			goto ErrorOut;

		// store if field is nullable
		lArrayLoc.l_sub2 = 5;
		vLogical.ev_length = lpCS->bNullable;
		if (nErrorNo = _Store(&lArrayLoc, &vLogical))
			goto ErrorOut;

		// store if field is binary (NOCPTRANS)
		lArrayLoc.l_sub2 = 6;
		vLogical.ev_length = lpCS->bBinary;
		if (nErrorNo = _Store(&lArrayLoc, &vLogical))
			goto ErrorOut;

		lpCS++;
	}

	sprintfex(aExeBuffer,"CREATE CURSOR %S FROM ARRAY %S",pCursorName,aArrayName);
	if (nErrorNo = _Execute(aExeBuffer))
		goto ErrorOut;

	UnlockFreeHandle(vChar);
	_Release(nVarNti);
	return 0;	

	ErrorOut:
		if (nVarNti > 0)
			_Release(nVarNti);
		UnlockFreeHandle(vChar);
		return nErrorNo;
}

int _stdcall SQLBindFieldLocators(LPSQLSTATEMENT pStmt, char *pCursorName)
{
	LPSQLCOLUMNDATA lpCS = pStmt->pColumnData;
	Value vWorkArea = {'0'};
	int nErrorNo, nColNo = 0;
	char aBuffer[256];

	sprintfex(aBuffer, "SELECT('%S')", pCursorName);
	if (nErrorNo = _Evaluate(&vWorkArea, aBuffer))
		return nErrorNo;

	while (nColNo++ < pStmt->nNoOfCols)
	{
		if (nErrorNo = FindFoxField((char*)lpCS->aColName,&lpCS->lField,vWorkArea.ev_long))
			return nErrorNo;

		if (lpCS->aVFPType == 'M' || lpCS->aVFPType == 'W')
		{
			lpCS->hMemoFile = _MemoChan(lpCS->lField.l_where);
			if (lpCS->hMemoFile == -1)
				return E_FIELDNOTFOUND;
		}

		lpCS++;
	}
	
	return 0;
}

int _stdcall SQLNumParamsEx(char *pSQL)
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
	return nParms;
}

int _stdcall SQLExtractParamsAndRewriteStatement(LPSQLSTATEMENT pStmt, SQLINTEGER *nLen)
{
	LPSQLPARAMDATA lpPS = pStmt->pParamData;
	char *pParmExpr, *pSQLIn, *pSQLOut;
	int nParamNo = 1, nParmLen;

	pSQLIn = pStmt->pSQLInput;
	pSQLOut = pStmt->pSQLSend;

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
				SaveCustomError("SQLExecEx", "Parameter expression '%20S...' exceeds limit of 512 characters", lpPS->aParmExpr);
				return E_APIERROR;
			}
			*pParmExpr = '\0'; // nullterminate parameter expression

			pSQLIn++; // skip over "}"

			lpPS++; // inc to next parameter
		}
		else 
			*pSQLOut++ = *pSQLIn++;
	}
	
	*pSQLOut = '\0'; // nullterminate Sql output
	*nLen = pSQLOut - pStmt->pSQLSend; // store length of rewritten SQL statement
	return 0;
}

int _stdcall SQLParseParamSchema(LPSQLSTATEMENT pStmt)
{
	LPSQLPARAMDATA lpPS;
	int nParmNo;
	char *pSchema;
	char aSQLType[VFP2C_ODBC_MAX_SQLTYPE];

	if (!pStmt->pParamSchema)
		return 0;

	pSchema = pStmt->pParamSchema;

	do 
	{
		if (!match_int(&pSchema,&nParmNo))
		{
			SaveCustomError("SQLExecEx", "Parameter schema contained invalid data near '%10S', expected parameter number.", pSchema);
			return E_APIERROR;
		}
		if (nParmNo < 1 || nParmNo > pStmt->nNoOfParms)
		{
			SaveCustomError("SQLExecEx", "Parameter schema contained invalid data, parameter number '%I' out of bounds.", nParmNo);
			return E_APIERROR;
		}
		// set pointer to parameter number X
		lpPS = pStmt->pParamData + (nParmNo - 1);

		lpPS->bNamed = match_quoted_identifier(&pSchema,lpPS->aParmName,VFP2C_ODBC_MAX_PARAMETER_NAME);

		if (match_identifier(&pSchema,aSQLType,VFP2C_ODBC_MAX_SQLTYPE))
		{
			if (StrEqual(aSQLType,"SQL_WCHAR"))
			{
				lpPS->nSQLType = SQL_WCHAR;
				lpPS->nCType = SQL_C_WCHAR;
			}
			else if (StrEqual(aSQLType,"SQL_BINARY"))
			{
				lpPS->nSQLType = SQL_BINARY;
				lpPS->nCType = SQL_C_BINARY;
			}
			else if (StrEqual(aSQLType,"SQL_CHAR"))
			{
				lpPS->nSQLType = SQL_CHAR;
				lpPS->nCType = SQL_C_CHAR;
			}
			else if (StrEqual(aSQLType,"SQL_DATE"))
			{
				lpPS->nSQLType = SQL_DATE;
				lpPS->nCType = SQL_C_DATE;
			}
			else if (StrEqual(aSQLType,"SQL_TIMESTAMP"))
			{
				lpPS->nSQLType = SQL_TIMESTAMP;
				lpPS->nCType = SQL_C_TIMESTAMP;
				if (match_chr(&pSchema,'('))
				{
					if (!match_int(&pSchema,(int*)&lpPS->nPrecision))
					{
						SaveCustomError("SQLExecEx", "Parameter schema contained invalid data near '%20S', expected precision.", pSchema);
						return E_APIERROR;
					}
					if (!match_chr(&pSchema,')'))
					{
						SaveCustomError("SQLExecEx", "Parameter schema contained invalid data near '%20S', expected ')'.", pSchema);
						return E_APIERROR;
					}
					lpPS->nSize = lpPS->nPrecision + 20;
				}
			}
			else if (StrEqual(aSQLType,"SQL_BIGINT"))
			{
				lpPS->nSQLType = SQL_BIGINT;
				lpPS->nCType = SQL_C_CHAR;
			}
			else if (StrEqual(aSQLType,"SQL_INTEGER"))
			{
				lpPS->nSQLType = SQL_INTEGER;
				lpPS->nCType = SQL_C_LONG;
			}
			else if (StrEqual(aSQLType,"SQL_SMALLINT"))
			{
				lpPS->nSQLType = SQL_SMALLINT;
				lpPS->nCType = SQL_C_LONG;
			}
			else if (StrEqual(aSQLType,"SQL_DOUBLE"))
			{
				lpPS->nSQLType = SQL_DOUBLE;
				lpPS->nCType = SQL_C_DOUBLE;
			}
			else if (StrEqual(aSQLType,"SQL_FLOAT"))
			{
				lpPS->nSQLType = SQL_FLOAT;
				lpPS->nCType = SQL_C_DOUBLE;
			}
			else if (StrEqual(aSQLType,"SQL_REAL"))
			{
				lpPS->nSQLType = SQL_REAL;
				lpPS->nCType = SQL_C_DOUBLE;
			}
			else if (StrEqual(aSQLType,"SQL_NUMERIC"))
			{
				lpPS->nSQLType = SQL_NUMERIC;
				lpPS->nCType = SQL_C_CHAR;
			}
			else if (StrEqual(aSQLType,"SQL_DECIMAL"))
			{
				lpPS->nSQLType = SQL_DECIMAL;
				lpPS->nCType = SQL_C_CHAR;
			}
			else
			{
				SaveCustomError("SQLExecEx", "Parameter schema contained invalid data '%S', expected SQL type.", aSQLType);
				return E_APIERROR;
			}

			lpPS->bCustomSchema = TRUE;
		}
		// if no type identified and also no parameter name was specified the parameter schema is invalid
		else if (!lpPS->bNamed) 
		{
			SaveCustomError("SQLExecEx", "Parameter schema contained invalid data near '%20S', expected SQL type.", pSchema);
			return E_APIERROR;
		}
	
	} while (match_chr(&pSchema,','));
	
	if (!match_chr(&pSchema,'\0'))
	{
		SaveCustomError("SQLExecEx", "Parameter schema contained invalid data near '%20S'.", pSchema);
		return E_APIERROR;
	}

	return 0;
}

int _stdcall SQLEvaluateParams(LPSQLSTATEMENT pStmt)
{
	LPSQLPARAMDATA lpPS = pStmt->pParamData;
	int nErrorNo, xj;

	for (xj = 1; xj <= pStmt->nNoOfParms; xj++)
	{
		if (lpPS->nParmDirection == SQL_PARAM_INPUT_OUTPUT || lpPS->nParmDirection == SQL_PARAM_OUTPUT)
		{
			if (nErrorNo = FindFoxVarOrFieldEx(lpPS->aParmExpr,&lpPS->lVarOrField))
				return nErrorNo;
			
			pStmt->bOutputParams = TRUE;
		}
	
		if (nErrorNo = _Evaluate(&lpPS->vParmValue, lpPS->aParmExpr))
			return nErrorNo;

		switch (lpPS->vParmValue.ev_type)
		{
			case 'C':
				if (lpPS->nParmDirection == SQL_PARAM_INPUT_OUTPUT || lpPS->nParmDirection == SQL_PARAM_OUTPUT)
				{
					if (!lpPS->bCustomSchema || lpPS->nSQLType == SQL_BINARY || lpPS->nSQLType == SQL_CHAR ||
						lpPS->nSQLType == SQL_WCHAR)
					{
						if (Len(lpPS->vParmValue) < VFP2C_ODBC_MAX_BUFFER)
						{
							if (!SetHandleSize(lpPS->vParmValue,VFP2C_ODBC_MAX_BUFFER))
								return E_INSUFMEMORY;
						}
						lpPS->nBufferSize = VFP2C_ODBC_MAX_BUFFER;
						lpPS->nSize = Len(lpPS->vParmValue);
					}
				}
				else
					lpPS->nSize = lpPS->nBufferSize = Len(lpPS->vParmValue);

				LockHandle(lpPS->vParmValue);
				lpPS->pParmData = HandleToPtr(lpPS->vParmValue);

				if (lpPS->bCustomSchema)
				{
					if (lpPS->nSQLType != SQL_BINARY && lpPS->nSQLType != SQL_WCHAR && 
						lpPS->nSQLType != SQL_TIMESTAMP)
					{
						SaveCustomError("SQLExecEx", "Invalid datatype conversion specified for parameter '%S'.", lpPS->aParmExpr);
						return E_APIERROR;
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
					lpPS->nBufferSize = VFP2C_ODBC_MAX_BUFFER+1;
					if (!AllocHandleEx(lpPS->vParmValue,max(lpPS->nSize,VFP2C_ODBC_MAX_BUFFER+1)))
						return E_INSUFMEMORY;
				}
				else
				{
					if (!AllocHandleEx(lpPS->vParmValue,lpPS->nSize))
						return E_INSUFMEMORY;
				}

				LockHandle(lpPS->vParmValue);
				lpPS->pParmData = HandleToPtr(lpPS->vParmValue);

				if (nErrorNo = GetMemoContent(&lpPS->vParmValue,(char*)lpPS->pParmData))
					return nErrorNo;

				if (lpPS->bCustomSchema)
				{
					if (lpPS->nSQLType != SQL_BINARY && lpPS->nSQLType != SQL_WCHAR && 
						lpPS->nSQLType != SQL_CHAR)
					{
						SaveCustomError("SQLExecEx", "Invalid datatype conversion specified for parameter '%S'.", lpPS->aParmExpr);
						return E_APIERROR;
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
						return E_APIERROR;			
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
						return E_APIERROR;			
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
						return E_APIERROR;
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
						if(lpPS->nSQLType == SQL_TIMESTAMP)
							DateTimeToTimestamp_Struct(&lpPS->vParmValue,&lpPS->sDateTime);
						else
							DateToTimestamp_Struct(&lpPS->vParmValue,&lpPS->sDateTime);

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
						DateTimeToTimestamp_Struct(&lpPS->vParmValue,&lpPS->sDateTime);

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
						return E_APIERROR;
					}

					if (lpPS->vParmValue.ev_real == 0.0)
						lpPS->nIndicator = SQL_NULL_DATA;
					else
						DateToTimestamp_Struct(&lpPS->vParmValue,&lpPS->sDateTime);

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
						DateToTimestamp_Struct(&lpPS->vParmValue,&lpPS->sDateTime);

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
				CurrencyToNumericLiteral(&lpPS->vParmValue,lpPS->aNumeric);
				lpPS->pParmData = lpPS->aNumeric;
				lpPS->nSize = VFP2C_VFP_CURRENCY_PRECISION;
				lpPS->nScale = VFP2C_VFP_CURRENCY_SCALE;
                lpPS->nBufferSize = VFP2C_ODBC_MAX_CURRENCY_LITERAL+1;
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
								if (!AllocHandleEx(lpPS->vParmValue,VFP2C_ODBC_MAX_BUFFER))
									return E_INSUFMEMORY;
								lpPS->nBufferSize = VFP2C_ODBC_MAX_BUFFER;
								LockHandle(lpPS->vParmValue);
								lpPS->pParmData = HandleToPtr(lpPS->vParmValue);
								break;

							case SQL_DATE:
							case SQL_TIMESTAMP:
								lpPS->vParmValue.ev_type = lpPS->nSQLType == SQL_DATE ? 'D' : 'T';
								lpPS->pParmData = &lpPS->sDateTime;
								lpPS->nBufferSize = sizeof(SQL_TIMESTAMP_STRUCT);
								break;

							case SQL_BIGINT:
								lpPS->vParmValue.SetString();
								if (!AllocHandleEx(lpPS->vParmValue,VFP2C_ODBC_MAX_BIGINT_LITERAL+1))
									return E_INSUFMEMORY;
								LockHandle(lpPS->vParmValue);
								lpPS->pParmData = HandleToPtr(lpPS->vParmValue);
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
	return 0;
}

SQLRETURN _stdcall SQLBindParameterEx(LPSQLSTATEMENT pStmt)
{
	SQLRETURN nApiRet;
	SQLHDESC hDesc;
	LPSQLPARAMDATA lpPS = pStmt->pParamData;
	int xj;

	for (xj = 1; xj <= pStmt->nNoOfParms; xj++)
	{
			nApiRet = SQLBindParameter(pStmt->hStmt, lpPS->nParmNo, lpPS->nParmDirection, lpPS->nCType,
				lpPS->nSQLType, lpPS->nSize, lpPS->nScale, 
				lpPS->bPutData ? lpPS : lpPS->pParmData, lpPS->nBufferSize,
				&lpPS->nIndicator);
			if (nApiRet == SQL_ERROR)
			{	
				SafeODBCStmtError("SQLBindParameter", pStmt->hStmt);
				return nApiRet;
			}

			// if the SQL type is SQL_TIMESTAMP and precision unequals 0, set precision
			if (lpPS->nSQLType == SQL_TIMESTAMP && lpPS->nPrecision)
			{
				// get descriptor handle for application parameter descriptor's
				nApiRet = SQLGetStmtAttr(pStmt->hStmt,SQL_ATTR_APP_PARAM_DESC,&hDesc,0,0);
				if (nApiRet == SQL_ERROR)
				{
					SafeODBCStmtError("SQLGetStmtAttr", pStmt->hStmt);
					return nApiRet;
				}
				// set descriptor
				nApiRet = SQLSetDescRec(hDesc,lpPS->nParmNo,lpPS->nSQLType,0,0,
					(SQLSMALLINT)lpPS->nPrecision,lpPS->nScale,lpPS->pParmData,&lpPS->nIndicator,&lpPS->nIndicator);
				if (nApiRet == SQL_ERROR)
				{
					SafeODBCStmtError("SQLSetDescRec", pStmt->hStmt);
					return nApiRet;
				}
				// get descriptor handle for implementation descriptor's
				nApiRet = SQLGetStmtAttr(pStmt->hStmt,SQL_ATTR_IMP_PARAM_DESC,&hDesc,0,0);
				if (nApiRet == SQL_ERROR)
				{
					SafeODBCStmtError("SQLGetStmtAttr", pStmt->hStmt);
					return nApiRet;
				}
				// set descriptor 
				nApiRet = SQLSetDescRec(hDesc,lpPS->nParmNo,lpPS->nSQLType,0,0,
					(SQLSMALLINT)lpPS->nPrecision,lpPS->nScale,lpPS->pParmData,&lpPS->nIndicator,&lpPS->nIndicator);
				if (nApiRet == SQL_ERROR)
				{
					SafeODBCStmtError("SQLSetDescRec", pStmt->hStmt);
					return nApiRet;
				}
			}

			// if we have a named parameter, set additional descriptor information
			if (lpPS->bNamed)
			{
				// get descriptor handle for implementation descriptor's
				nApiRet = SQLGetStmtAttr(pStmt->hStmt,SQL_ATTR_IMP_PARAM_DESC,&hDesc,0,0);
				if (nApiRet == SQL_ERROR)
				{
					SafeODBCStmtError("SQLGetStmtAttr", pStmt->hStmt);
					return nApiRet;
				}
				nApiRet = SQLSetDescField(hDesc,lpPS->nParmNo,SQL_DESC_NAME,lpPS->aParmName,SQL_NTS);
				if (nApiRet == SQL_ERROR)
				{
					SafeODBCStmtError("SQLSetDescField", pStmt->hStmt);
					return nApiRet;
				}
				SQLSetDescField(hDesc,lpPS->nParmNo,SQL_DESC_UNNAMED,SQL_NAMED,0);
			}

			lpPS++;
	}

	return nApiRet;
}

SQLRETURN _stdcall SQLPutDataEx(SQLHSTMT hStmt)
{
	SQLRETURN nApiRet;
	LPSQLPARAMDATA lpParm;
	char* lpData;
	SQLINTEGER nBufferSize, nDataSize;

	nApiRet = SQLParamData(hStmt,(SQLPOINTER*)&lpParm);
	
	while(nApiRet == SQL_NEED_DATA)
	{
		lpData = (char*)lpParm->pParmData;
		nDataSize = lpParm->nSize;
		
		SQLSendData:
			nBufferSize = nDataSize > VFP2C_ODBC_MAX_BUFFER ? VFP2C_ODBC_MAX_BUFFER : nDataSize;
			nApiRet = SQLPutData(hStmt,lpData,nBufferSize);
			if (nApiRet == SQL_ERROR)
			{
				SafeODBCStmtError("SQLPutData", hStmt);
				return nApiRet;
			}
			
			nDataSize -= nBufferSize;
			lpData += nBufferSize;

			if (nDataSize > 0)
				goto SQLSendData;

			nApiRet = SQLParamData(hStmt,(SQLPOINTER*)&lpParm);
	}
	if (nApiRet == SQL_ERROR)
		SafeODBCStmtError("SQLParamData", hStmt);

	return nApiRet;
}

int _stdcall SQLSaveOutputParameters(LPSQLSTATEMENT pStmt)
{
	LPSQLPARAMDATA lpPS;
	Value vNull = {'0'};
	int nErrorNo, xj;

	if (!pStmt->bOutputParams)
		return 0;

	lpPS = pStmt->pParamData;

	for (xj = 1; xj <= pStmt->nNoOfParms; xj++)
	{
		if (lpPS->nParmDirection == SQL_PARAM_INPUT_OUTPUT || lpPS->nParmDirection == SQL_PARAM_OUTPUT)
		{
			if (lpPS->nIndicator == SQL_NULL_DATA)
			{
				if (nErrorNo = StoreEx(&lpPS->lVarOrField, &vNull))
					return nErrorNo;

				lpPS++;
				continue;
			}

			switch (lpPS->vParmValue.ev_type)
			{

				case 'L':
					if (nErrorNo = StoreEx(&lpPS->lVarOrField, &lpPS->vParmValue))
						return nErrorNo;
					break;
				
				case 'I':
				case 'N':
					if (nErrorNo = StoreEx(&lpPS->lVarOrField, &lpPS->vParmValue))
						return nErrorNo;
					break;

				case 'D':
					Timestamp_StructToDate(&lpPS->sDateTime,&lpPS->vParmValue);
					if (nErrorNo = StoreEx(&lpPS->lVarOrField, &lpPS->vParmValue))
						return nErrorNo;
					break;

				case 'T':
					Timestamp_StructToDateTime(&lpPS->sDateTime,&lpPS->vParmValue);
					if (nErrorNo = StoreEx(&lpPS->lVarOrField, &lpPS->vParmValue))
						return nErrorNo;
					break;

				case 'C':
				case 'V':
				case 'Q':
				case 'M':
				case 'W':
					lpPS->vParmValue.ev_length = lpPS->nIndicator;
					if (nErrorNo = StoreEx(&lpPS->lVarOrField, &lpPS->vParmValue))
						return nErrorNo;
					break;

				case 'Y':
					NumericLiteralToCurrency((SQLCHAR*)lpPS->pParmData,&lpPS->vParmValue);
					if (nErrorNo = StoreEx(&lpPS->lVarOrField, &lpPS->vParmValue))
						return nErrorNo;
					break;

				default:
					return E_APIERROR;
			}
		}

		lpPS++;
	}

	return nErrorNo;
}

int _stdcall SQLProgressCallback(LPSQLSTATEMENT pStmt, int nRowsFetched, BOOL *bAbort)
{
	Value vCallRet = {'0'};
	int nErrorNo;
	bool bAbortFlag;

	sprintfex(pStmt->pCallbackBuffer,pStmt->pCallbackCmd,pStmt->nResultset,nRowsFetched,pStmt->nRowsTotal);
	if (nErrorNo = _Evaluate(&vCallRet, pStmt->pCallbackBuffer))
		return nErrorNo;

	if (Vartype(vCallRet) == 'L')
		bAbortFlag = !vCallRet.ev_length;
	else if (Vartype(vCallRet) == 'I')
		bAbortFlag = vCallRet.ev_long == 0;
	else if (Vartype(vCallRet) == 'N')
		bAbortFlag = vCallRet.ev_real == 0.0;
	else
	{
		bAbortFlag = false;
		ReleaseValue(vCallRet);
	}

	if (bAbortFlag)
	{
		*bAbort = TRUE;
		return E_APIERROR;
	}

	return 0;
}

int _stdcall SQLInfoCallbackOrStore(LPSQLSTATEMENT pStmt)
{
	SQLRETURN nApiRet;
	SQLSMALLINT nMsgLen, nError = 1;
	SQLINTEGER nNativeError;
	IntValue vResultset;
	int nErrorNo;
	SQLCHAR aSQLState[16];

	if ((pStmt->nFlags & SQLEXECEX_STORE_INFO) || ((pStmt->nFlags & SQLEXECEX_CALLBACK_INFO) && pStmt->pCallbackCmdInfo))
	{

	nApiRet = SQLGetDiagRec(SQL_HANDLE_STMT,pStmt->hStmt,nError++,aSQLState,&nNativeError,
		(SQLCHAR*)pStmt->pGetDataBuffer,VFP2C_ODBC_MAX_BUFFER,&nMsgLen);

	while (nApiRet == SQL_SUCCESS)
	{
		if (pStmt->nFlags & SQLEXECEX_STORE_INFO)
		{
			if (pStmt->lArrayLoc.l_sub1++)
			{
				if (nErrorNo = Dimension(pStmt->pArrayName, pStmt->lArrayLoc.l_sub1, 2))
					return nErrorNo;
			}

			pStmt->lArrayLoc.l_sub2 = 1;
			pStmt->vGetDataBuffer.ev_length = SQLExtractInfo((char*)pStmt->pGetDataBuffer,nMsgLen);
			if (nErrorNo = _Store(&pStmt->lArrayLoc, &pStmt->vGetDataBuffer))
				return nErrorNo;

		}
		else if (pStmt->pCallbackCmdInfo)
		{
			pStmt->vGetDataBuffer.ev_length = SQLExtractInfo((char*)pStmt->pGetDataBuffer,nMsgLen);
			if (nErrorNo = _Store(&pStmt->lInfoParm, &pStmt->vGetDataBuffer))
				return nErrorNo;

			if (nErrorNo = _Execute(pStmt->pCallbackCmdInfo))
				return nErrorNo;
		}
		nApiRet = SQLGetDiagRec(SQL_HANDLE_STMT,pStmt->hStmt,nError++,aSQLState,&nNativeError,
			(SQLCHAR*)pStmt->pGetDataBuffer,VFP2C_ODBC_MAX_BUFFER,&nMsgLen);
	}

	} // if (pStmt->nFlags & SQLEXECEX_STORE_INFO) ..
	return 0;
}

unsigned int _stdcall SQLExtractInfo(char *pMessage, unsigned int nMsgLen)
{
	char *pStart = pMessage;
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
		memmove(pStart,pMessage,nLength); // move the content to the start of the string ..
	}

	return nLength;
}

int _stdcall SQLFetchToCursor(LPSQLSTATEMENT pStmt, BOOL *bAborted)
{
	SQLRETURN nApiRet;
	int nRowsFetched = 0, nErrorNo;

	if (pStmt->pCallbackCmd)
	{
		// callback once before first row is fetched
		if (nErrorNo = SQLProgressCallback(pStmt,nRowsFetched,bAborted))
			return nErrorNo;

		nApiRet = SQLFetch(pStmt->hStmt);
		while (nApiRet == SQL_SUCCESS || nApiRet == SQL_SUCCESS_WITH_INFO)
		{
			if (nErrorNo = SQLStoreToCursor(pStmt))
				return nErrorNo;
		
			nRowsFetched++;
			// callback for each Nth row 
			if (nRowsFetched % pStmt->nCallbackInterval == 0)
			{
				if (nErrorNo = SQLProgressCallback(pStmt,nRowsFetched,bAborted))
					return nErrorNo;
			}
			nApiRet = SQLFetch(pStmt->hStmt);
		}
		if (nApiRet != SQL_ERROR)
		{
			// callback once after last row is fetched
			if (nErrorNo = SQLProgressCallback(pStmt,nRowsFetched,bAborted))
				return nErrorNo;
		}
	}
	else
	{
		nApiRet = SQLFetch(pStmt->hStmt);
		while (nApiRet == SQL_SUCCESS || nApiRet == SQL_SUCCESS_WITH_INFO)
		{
			if (nErrorNo = SQLStoreToCursor(pStmt))
				return nErrorNo;
			nRowsFetched++;
			nApiRet = SQLFetch(pStmt->hStmt);
		}
	}

	pStmt->nRowsFetched = nRowsFetched;

	if (nApiRet == SQL_ERROR)
	{
		SafeODBCStmtError("SQLFetch", pStmt->hStmt);
		return E_APIERROR;
	}
	return 0;
}

int _stdcall SQLStoreToCursor(LPSQLSTATEMENT pStmt)
{
	LPSQLCOLUMNDATA lpCS = pStmt->pColumnData;
	int nRetVal, nCol = pStmt->nNoOfCols;
	
	if (nRetVal = Append(lpCS->lField))
		return nRetVal;

	while (nCol--)
	{
		if (nRetVal = lpCS->pStore(lpCS))
			return nRetVal;
		lpCS++;
	}
	return 0;
 }

int _stdcall SQLFetchToVariables(LPSQLSTATEMENT pStmt)
{
	SQLRETURN nApiRet;
	int nRowsFetched = 0, nErrorNo;

	nApiRet = SQLFetch(pStmt->hStmt);
	while (nApiRet == SQL_SUCCESS || nApiRet == SQL_SUCCESS_WITH_INFO)
	{
		if (!nRowsFetched)
		{
			if (nErrorNo = SQLStoreToVariables(pStmt))
				return nErrorNo;
		}
		nRowsFetched++;
		nApiRet = SQLFetch(pStmt->hStmt);
	}
	pStmt->nRowsFetched = nRowsFetched;
	if (nApiRet == SQL_ERROR)
	{
		SafeODBCStmtError("SQLFetch", pStmt->hStmt);
		return E_APIERROR;
	}
	return 0;
}

int _stdcall SQLStoreToVariables(LPSQLSTATEMENT pStmt)
{
	LPSQLCOLUMNDATA lpCS = pStmt->pColumnData;
	int nRetVal, nCol = pStmt->nNoOfCols;
	
	while (nCol--)
	{
		if (nRetVal = lpCS->pStore(lpCS))
			return nRetVal;
		lpCS++;
	}
	return 0;
}

int _stdcall SQLStoreByBinding(LPSQLCOLUMNDATA lpCS)
{
	if (lpCS->nIndicator == SQL_NULL_DATA)
		return _DBReplace(&lpCS->lField, &lpCS->vNull);
	else
		return _DBReplace(&lpCS->lField, &lpCS->vData);
}

int _stdcall SQLStoreByBindingVar(LPSQLCOLUMNDATA lpCS)
{
	if (lpCS->nIndicator == SQL_NULL_DATA)
		return _Store(&lpCS->lField, &lpCS->vNull);
	else
		return _Store(&lpCS->lField, &lpCS->vData);
}

int _stdcall SQLStoreByGetData(LPSQLCOLUMNDATA lpCS)
{
	SQLRETURN nApiRet;
	nApiRet = SQLGetData(lpCS->hStmt,lpCS->nColNo,lpCS->nCType,lpCS->pData,lpCS->nBufferSize,&lpCS->nIndicator);
	if (nApiRet == SQL_SUCCESS || nApiRet == SQL_SUCCESS_WITH_INFO)
	{
		if (lpCS->nIndicator == SQL_NULL_DATA)
			return _DBReplace(&lpCS->lField, &lpCS->vNull);
		else
			return _DBReplace(&lpCS->lField, &lpCS->vData);
	}
	SafeODBCStmtError("SQLGetData", lpCS->hStmt);
	return E_APIERROR;
}

int _stdcall SQLStoreByGetDataVar(LPSQLCOLUMNDATA lpCS)
{
	SQLRETURN nApiRet;
	nApiRet = SQLGetData(lpCS->hStmt,lpCS->nColNo,lpCS->nCType,lpCS->pData,lpCS->nBufferSize,&lpCS->nIndicator);
	if (nApiRet == SQL_SUCCESS || nApiRet == SQL_SUCCESS_WITH_INFO)
	{
		if (lpCS->nIndicator == SQL_NULL_DATA)
			return _Store(&lpCS->lField, &lpCS->vNull);
		else
			return _Store(&lpCS->lField, &lpCS->vData);
	}
	SafeODBCStmtError("SQLGetData", lpCS->hStmt);
	return E_APIERROR;
}

int _stdcall SQLStoreCharByBinding(LPSQLCOLUMNDATA lpCS)
{
	if (lpCS->nIndicator == SQL_NULL_DATA)
		return _DBReplace(&lpCS->lField, &lpCS->vNull);
	else
	{
		lpCS->vData.ev_length = lpCS->nIndicator;
		return _DBReplace(&lpCS->lField, &lpCS->vData);
	}
}

int _stdcall SQLStoreCharByBindingVar(LPSQLCOLUMNDATA lpCS)
{
	if (lpCS->nIndicator == SQL_NULL_DATA)
		return _Store(&lpCS->lField, &lpCS->vNull);
	else
	{
		lpCS->vData.ev_length = lpCS->nIndicator;
		return _Store(&lpCS->lField, &lpCS->vData);
	}
}

int _stdcall SQLStoreCharByGetData(LPSQLCOLUMNDATA lpCS)
{
	SQLRETURN nApiRet;
	nApiRet = SQLGetData(lpCS->hStmt,lpCS->nColNo,lpCS->nCType,lpCS->pData,lpCS->nBufferSize,&lpCS->nIndicator);
	if (nApiRet == SQL_SUCCESS || nApiRet == SQL_SUCCESS_WITH_INFO)
	{
		if (lpCS->nIndicator == SQL_NULL_DATA)
			return _DBReplace(&lpCS->lField, &lpCS->vNull);
		else
		{
			lpCS->vData.ev_length = lpCS->nIndicator;
			return _DBReplace(&lpCS->lField, &lpCS->vData);
		}
	 }
	SafeODBCStmtError("SQLGetData", lpCS->hStmt);
	return E_APIERROR;
}

int _stdcall SQLStoreCharByGetDataVar(LPSQLCOLUMNDATA lpCS)
{
	SQLRETURN nApiRet;
	nApiRet = SQLGetData(lpCS->hStmt,lpCS->nColNo,lpCS->nCType,lpCS->pData,lpCS->nBufferSize,&lpCS->nIndicator);
	if (nApiRet == SQL_SUCCESS || nApiRet == SQL_SUCCESS_WITH_INFO)
	{
		if (lpCS->nIndicator == SQL_NULL_DATA)
			return _Store(&lpCS->lField, &lpCS->vNull);
		else
		{
			lpCS->vData.ev_length = lpCS->nIndicator;
			return _Store(&lpCS->lField, &lpCS->vData);
		}
	}
	SafeODBCStmtError("SQLGetData", lpCS->hStmt);
	return E_APIERROR;
}

int _stdcall SQLStoreDateByBinding(LPSQLCOLUMNDATA lpCS)
{
	if (lpCS->nIndicator == SQL_NULL_DATA)
		return _DBReplace(&lpCS->lField, &lpCS->vNull);
	else
	{
		Timestamp_StructToDate(&lpCS->sDateTime,&lpCS->vData);
		return _DBReplace(&lpCS->lField, &lpCS->vData);
	}
}

int _stdcall SQLStoreDateByBindingVar(LPSQLCOLUMNDATA lpCS)
{
	if (lpCS->nIndicator == SQL_NULL_DATA)
		return _Store(&lpCS->lField, &lpCS->vNull);
	else
	{
		Timestamp_StructToDate(&lpCS->sDateTime,&lpCS->vData);
		return _Store(&lpCS->lField, &lpCS->vData);
	}
}

int _stdcall SQLStoreDateByGetData(LPSQLCOLUMNDATA lpCS)
{
	SQLRETURN nApiRet;
	
	nApiRet = SQLGetData(lpCS->hStmt,lpCS->nColNo,lpCS->nCType,lpCS->pData,lpCS->nBufferSize,&lpCS->nIndicator);
	if (nApiRet == SQL_SUCCESS || nApiRet == SQL_SUCCESS_WITH_INFO)
	{
		if (lpCS->nIndicator == SQL_NULL_DATA)
			return _DBReplace(&lpCS->lField, &lpCS->vNull);
		else
		{
			Timestamp_StructToDate(&lpCS->sDateTime,&lpCS->vData);
			return _DBReplace(&lpCS->lField, &lpCS->vData);
		}
	}
	SafeODBCStmtError("SQLGetData", lpCS->hStmt);
	return E_APIERROR;
}

int _stdcall SQLStoreDateByGetDataVar(LPSQLCOLUMNDATA lpCS)
{
	SQLRETURN nApiRet;
	
	nApiRet = SQLGetData(lpCS->hStmt,lpCS->nColNo,lpCS->nCType,lpCS->pData,lpCS->nBufferSize,&lpCS->nIndicator);
	if (nApiRet == SQL_SUCCESS || nApiRet == SQL_SUCCESS_WITH_INFO)
	{
		if (lpCS->nIndicator == SQL_NULL_DATA)
			return _Store(&lpCS->lField, &lpCS->vNull);
		else
		{
			Timestamp_StructToDate(&lpCS->sDateTime,&lpCS->vData);
			return _Store(&lpCS->lField, &lpCS->vData);
		}
	}
	SafeODBCStmtError("SQLGetData", lpCS->hStmt);
	return E_APIERROR;
}

int _stdcall SQLStoreDateTimeByBinding(LPSQLCOLUMNDATA lpCS)
{
	if (lpCS->nIndicator == SQL_NULL_DATA)
		return _DBReplace(&lpCS->lField, &lpCS->vNull);
	else
	{
		Timestamp_StructToDateTime(&lpCS->sDateTime,&lpCS->vData);
		return _DBReplace(&lpCS->lField, &lpCS->vData);
	}
}

int _stdcall SQLStoreDateTimeByBindingVar(LPSQLCOLUMNDATA lpCS)
{
	if (lpCS->nIndicator == SQL_NULL_DATA)
		return _Store(&lpCS->lField, &lpCS->vNull);
	else
	{
		Timestamp_StructToDateTime(&lpCS->sDateTime,&lpCS->vData);
		return _Store(&lpCS->lField, &lpCS->vData);
	}
}

int _stdcall SQLStoreDateTimeByGetData(LPSQLCOLUMNDATA lpCS)
{
	SQLRETURN nApiRet;
	
	nApiRet = SQLGetData(lpCS->hStmt,lpCS->nColNo,lpCS->nCType,lpCS->pData,lpCS->nBufferSize,&lpCS->nIndicator);
	if (nApiRet == SQL_SUCCESS || nApiRet == SQL_SUCCESS_WITH_INFO)
	{
		if (lpCS->nIndicator == SQL_NULL_DATA)
			return _DBReplace(&lpCS->lField, &lpCS->vNull);
		else
		{
			Timestamp_StructToDateTime(&lpCS->sDateTime,&lpCS->vData);
			return _DBReplace(&lpCS->lField, &lpCS->vData);
		}
	}
	SafeODBCStmtError("SQLGetData", lpCS->hStmt);
	return E_APIERROR;
}

int _stdcall SQLStoreDateTimeByGetDataVar(LPSQLCOLUMNDATA lpCS)
{
	SQLRETURN nApiRet;
	
	nApiRet = SQLGetData(lpCS->hStmt,lpCS->nColNo,lpCS->nCType,lpCS->pData,lpCS->nBufferSize,&lpCS->nIndicator);
	if (nApiRet == SQL_SUCCESS || nApiRet == SQL_SUCCESS_WITH_INFO)
	{
		if (lpCS->nIndicator == SQL_NULL_DATA)
			return _Store(&lpCS->lField, &lpCS->vNull);
		else
		{
			Timestamp_StructToDateTime(&lpCS->sDateTime,&lpCS->vData);
			return _Store(&lpCS->lField, &lpCS->vData);
		}
	}
	SafeODBCStmtError("SQLGetData", lpCS->hStmt);
	return E_APIERROR;
}

int _stdcall SQLStoreCurrencyByBinding(LPSQLCOLUMNDATA lpCS)
{
	if (lpCS->nIndicator == SQL_NULL_DATA)
		return _DBReplace(&lpCS->lField, &lpCS->vNull);
	else
	{
		NumericLiteralToCurrency(lpCS->aNumeric,&lpCS->vData);
		return _DBReplace(&lpCS->lField, &lpCS->vData);
	}
}

int _stdcall SQLStoreCurrencyByBindingVar(LPSQLCOLUMNDATA lpCS)
{
	if (lpCS->nIndicator == SQL_NULL_DATA)
		return _Store(&lpCS->lField, &lpCS->vNull);
	else
	{
		NumericLiteralToCurrency(lpCS->aNumeric,&lpCS->vData);
		return _Store(&lpCS->lField, &lpCS->vData);
	}
}

int _stdcall SQLStoreCurrencyByGetData(LPSQLCOLUMNDATA lpCS)
{
	SQLRETURN nApiRet;
	nApiRet = SQLGetData(lpCS->hStmt,lpCS->nColNo,lpCS->nCType,lpCS->aNumeric,lpCS->nBufferSize,&lpCS->nIndicator);
	if (nApiRet == SQL_SUCCESS || nApiRet == SQL_SUCCESS_WITH_INFO)
	{
		if (lpCS->nIndicator == SQL_NULL_DATA)
			return _DBReplace(&lpCS->lField, &lpCS->vNull);
		else
		{
			NumericLiteralToCurrency(lpCS->aNumeric,&lpCS->vData);
			return _DBReplace(&lpCS->lField, &lpCS->vData);
		}
	}
	SafeODBCStmtError("SQLGetData", lpCS->hStmt);
	return E_APIERROR;
}

int _stdcall SQLStoreCurrencyByGetDataVar(LPSQLCOLUMNDATA lpCS)
{
	SQLRETURN nApiRet;
	nApiRet = SQLGetData(lpCS->hStmt,lpCS->nColNo,lpCS->nCType,lpCS->aNumeric,lpCS->nBufferSize,&lpCS->nIndicator);
	if (nApiRet == SQL_SUCCESS || nApiRet == SQL_SUCCESS_WITH_INFO)
	{
		if (lpCS->nIndicator == SQL_NULL_DATA)
			return _Store(&lpCS->lField, &lpCS->vNull);
		else
		{
			NumericLiteralToCurrency(lpCS->aNumeric,&lpCS->vData);
			return _Store(&lpCS->lField, &lpCS->vData);
		}
	}
	SafeODBCStmtError("SQLGetData", lpCS->hStmt);
	return E_APIERROR;
}

int _stdcall SQLStoreMemoChar(LPSQLCOLUMNDATA lpCS)
{
	SQLRETURN nApiRet;
	long nLoc;
	int nErrorNo;

	nApiRet = SQLGetData(lpCS->hStmt,lpCS->nColNo,lpCS->nCType,lpCS->pData,VFP2C_ODBC_MAX_BUFFER,&lpCS->nIndicator);
	if (nApiRet == SQL_SUCCESS)
	{
		if (lpCS->nIndicator != SQL_NULL_DATA)
			return ReplaceMemoEx(&lpCS->lField,(char*)lpCS->pData,lpCS->nIndicator,lpCS->hMemoFile);
		else if (lpCS->bNullable)
			return _DBReplace(&lpCS->lField, &lpCS->vNull);
	}
	else if (nApiRet == SQL_SUCCESS_WITH_INFO)
	{
		// result is greater than buffer, allocate space for the memo field, nIndicator contains length of data
		if (nErrorNo = AllocMemo(&lpCS->lField,lpCS->nIndicator,&nLoc))
			return nErrorNo;

		// append the data from the buffer
		if (nErrorNo = AppendMemo((char*)lpCS->pData,VFP2C_ODBC_MAX_BUFFER-1,lpCS->hMemoFile,&nLoc))
			return nErrorNo;

		do
		{
			nApiRet = SQLGetData(lpCS->hStmt,lpCS->nColNo,lpCS->nCType,lpCS->pData,VFP2C_ODBC_MAX_BUFFER,&lpCS->nIndicator);
			if (nApiRet == SQL_SUCCESS || nApiRet == SQL_SUCCESS_WITH_INFO)
			{
				if (nErrorNo = AppendMemo((char*)lpCS->pData,VFP2C_ODBC_MAX_BUFFER-1,lpCS->hMemoFile,&nLoc))
					return nErrorNo;
			}
		} while (nApiRet == SQL_SUCCESS_WITH_INFO);

		if (nApiRet == SQL_ERROR)
		{
			SafeODBCStmtError("SQLGetData", lpCS->hStmt);
			return E_APIERROR;
		}
	}
	else
	{
		SafeODBCStmtError("SQLGetData", lpCS->hStmt);
		return E_APIERROR;
	}

	return 0;
}

int _stdcall SQLStoreMemoCharVar(LPSQLCOLUMNDATA lpCS)
{
	SQLRETURN nApiRet;
	StringValue vData;
	char *pData;
	int nRetVal;

	if (!AllocHandleEx(vData,VFP2C_ODBC_MAX_BUFFER))
		return E_INSUFMEMORY;
	
	LockHandle(vData);
	pData = HandleToPtr(vData);

	nApiRet = SQLGetData(lpCS->hStmt,lpCS->nColNo,lpCS->nCType,pData,VFP2C_ODBC_MAX_BUFFER,&lpCS->nIndicator);
	if (nApiRet == SQL_SUCCESS)
	{
		if (lpCS->nIndicator == SQL_NULL_DATA)
			nRetVal = _Store(&lpCS->lField, &lpCS->vNull);
		else
		{
			vData.ev_length = lpCS->nIndicator;
			nRetVal = _Store(&lpCS->lField, &vData);
		}
		UnlockFreeHandle(vData);
		return nRetVal;
	}
	else if (nApiRet == SQL_SUCCESS_WITH_INFO)
	{
		vData.ev_length = lpCS->nIndicator;
		UnlockHandle(vData);
		if (!SetHandleSize(vData,lpCS->nIndicator+1))
		{
			FreeHandle(vData);
			return E_INSUFMEMORY;
		}
		LockHandle(vData);
		pData = HandleToPtr(vData);
		pData += VFP2C_ODBC_MAX_BUFFER-1;

		do 
		{
			nApiRet = SQLGetData(lpCS->hStmt,lpCS->nColNo,lpCS->nCType,pData,VFP2C_ODBC_MAX_BUFFER,&lpCS->nIndicator);
			if (nApiRet == SQL_SUCCESS_WITH_INFO)
				pData += VFP2C_ODBC_MAX_BUFFER-1;
		} while (nApiRet == SQL_SUCCESS_WITH_INFO);
		
		if (nApiRet == SQL_SUCCESS)
			nRetVal = _Store(&lpCS->lField, &vData);
		else
		{
			SafeODBCStmtError("SQLGetData", lpCS->hStmt);
			nRetVal = E_APIERROR;
		}
		UnlockFreeHandle(vData);
		return nRetVal;
	}
	else
	{
		SafeODBCStmtError("SQLGetData", lpCS->hStmt);
		UnlockFreeHandle(vData);
		return E_APIERROR;
	}
}

int _stdcall SQLStoreMemoWChar(LPSQLCOLUMNDATA lpCS)
{
	SQLRETURN nApiRet;
	long nLoc;
	int nErrorNo;

	nApiRet = SQLGetData(lpCS->hStmt,lpCS->nColNo,lpCS->nCType,lpCS->pData,VFP2C_ODBC_MAX_BUFFER,&lpCS->nIndicator);
	if (nApiRet == SQL_SUCCESS)
	{
		if (lpCS->nIndicator != SQL_NULL_DATA)
			return ReplaceMemoEx(&lpCS->lField,(char*)lpCS->pData,lpCS->nIndicator,lpCS->hMemoFile);
		else if (lpCS->bNullable)
			return _DBReplace(&lpCS->lField, &lpCS->vNull);
	}
	else if (nApiRet == SQL_SUCCESS_WITH_INFO)
	{
		// result is greater than buffer, allocate space for the memo field, nIndicator contains length of data
		if (nErrorNo = AllocMemo(&lpCS->lField,lpCS->nIndicator,&nLoc))
			return nErrorNo;

		// append the data from the buffer
		if (nErrorNo = AppendMemo((char*)lpCS->pData,VFP2C_ODBC_MAX_BUFFER-2,lpCS->hMemoFile,&nLoc))
			return nErrorNo;

		do 
		{
			nApiRet = SQLGetData(lpCS->hStmt,lpCS->nColNo,lpCS->nCType,lpCS->pData,VFP2C_ODBC_MAX_BUFFER,&lpCS->nIndicator);
			if (nApiRet == SQL_SUCCESS_WITH_INFO || nApiRet == SQL_SUCCESS)
			{
				if (nErrorNo = AppendMemo((char*)lpCS->pData,VFP2C_ODBC_MAX_BUFFER-2,lpCS->hMemoFile,&nLoc))
					return nErrorNo;
			}
		} while (nApiRet == SQL_SUCCESS_WITH_INFO);

		if (nApiRet == SQL_ERROR)
		{
			SafeODBCStmtError("SQLGetData", lpCS->hStmt);
			return E_APIERROR;
		}
	}
	else
	{
		SafeODBCStmtError("SQLGetData", lpCS->hStmt);
		return E_APIERROR;
	}
	return 0;
}

int _stdcall SQLStoreMemoWCharVar(LPSQLCOLUMNDATA lpCS)
{
	SQLRETURN nApiRet;
	StringValue vData;
	char *pData;
	int nRetVal;

	if (!AllocHandleEx(vData,VFP2C_ODBC_MAX_BUFFER))
		return E_INSUFMEMORY;
	
	LockHandle(vData);
	pData = HandleToPtr(vData);

	nApiRet = SQLGetData(lpCS->hStmt,lpCS->nColNo,lpCS->nCType,pData,VFP2C_ODBC_MAX_BUFFER,&lpCS->nIndicator);
	if (nApiRet == SQL_SUCCESS)
	{
		if (lpCS->nIndicator == SQL_NULL_DATA)
			nRetVal = _Store(&lpCS->lField, &lpCS->vNull);
		else
		{
			vData.ev_length = lpCS->nIndicator;
			nRetVal = _Store(&lpCS->lField, &vData);
		}
		UnlockFreeHandle(vData);
		return nRetVal;
	}
	else if (nApiRet == SQL_SUCCESS_WITH_INFO)
	{
		vData.ev_length = lpCS->nIndicator;
		UnlockHandle(vData);
		if (!SetHandleSize(vData,lpCS->nIndicator+2))
		{
			FreeHandle(vData);
			return E_INSUFMEMORY;
		}
		LockHandle(vData);
		pData = HandleToPtr(vData);
		pData += VFP2C_ODBC_MAX_BUFFER-2;

		do 
		{
			nApiRet = SQLGetData(lpCS->hStmt,lpCS->nColNo,lpCS->nCType,pData,VFP2C_ODBC_MAX_BUFFER,&lpCS->nIndicator);
			if (nApiRet == SQL_SUCCESS_WITH_INFO)
				pData += VFP2C_ODBC_MAX_BUFFER-2;
		} while (nApiRet == SQL_SUCCESS_WITH_INFO);

		if (nApiRet == SQL_SUCCESS)
			nRetVal = _Store(&lpCS->lField, &vData);
		else
		{
			SafeODBCStmtError("SQLGetData", lpCS->hStmt);
			nRetVal = E_APIERROR;
		}
		UnlockFreeHandle(vData);
		return nRetVal;
	}
	else
	{
		SafeODBCStmtError("SQLGetData", lpCS->hStmt);
		UnlockFreeHandle(vData);
		return E_APIERROR;
	}
}

int _stdcall SQLStoreMemoBinary(LPSQLCOLUMNDATA lpCS)
{
	SQLRETURN nApiRet;
	long nLoc;
	int nErrorNo;

	nApiRet = SQLGetData(lpCS->hStmt,lpCS->nColNo,lpCS->nCType,lpCS->pData,VFP2C_ODBC_MAX_BUFFER,&lpCS->nIndicator);
	
	if (nApiRet == SQL_SUCCESS)
	{
		if (lpCS->nIndicator != SQL_NULL_DATA)
			return ReplaceMemoEx(&lpCS->lField,(char*)lpCS->pData,lpCS->nIndicator,lpCS->hMemoFile);
		else if (lpCS->bNullable)
			return _DBReplace(&lpCS->lField, &lpCS->vNull);
	}
	else if (nApiRet == SQL_SUCCESS_WITH_INFO)
	{
		// allocate space for the memo field, nIncicator contains length of full data
		if (nErrorNo = AllocMemo(&lpCS->lField,lpCS->nIndicator,&nLoc))
			return nErrorNo;

		// append the data from the buffer
		if (nErrorNo = AppendMemo((char*)lpCS->pData,VFP2C_ODBC_MAX_BUFFER,lpCS->hMemoFile,&nLoc))
			return nErrorNo;

		do 
		{
			nApiRet = SQLGetData(lpCS->hStmt,lpCS->nColNo,lpCS->nCType,lpCS->pData,VFP2C_ODBC_MAX_BUFFER,&lpCS->nIndicator);
			if (nApiRet == SQL_SUCCESS || nApiRet == SQL_SUCCESS_WITH_INFO)
			{
				if (nErrorNo = AppendMemo((char*)lpCS->pData,VFP2C_ODBC_MAX_BUFFER,lpCS->hMemoFile,&nLoc))
					return nErrorNo;
			}
		} while (nApiRet == SQL_SUCCESS_WITH_INFO);

		if (nApiRet == SQL_ERROR)
		{
			SafeODBCStmtError("SQLGetData", lpCS->hStmt);
			return E_APIERROR;
		}
	}
	else
	{
		SafeODBCStmtError("SQLGetData", lpCS->hStmt);
		return E_APIERROR;
	}

	return 0;
}

int _stdcall SQLStoreMemoBinaryVar(LPSQLCOLUMNDATA lpCS)
{
	SQLRETURN nApiRet;
	StringValue vData;
	char *pData;
	int nRetVal;

	if (!AllocHandleEx(vData,VFP2C_ODBC_MAX_BUFFER))
		return E_INSUFMEMORY;
	
	LockHandle(vData);
	pData = HandleToPtr(vData);

	nApiRet = SQLGetData(lpCS->hStmt,lpCS->nColNo,lpCS->nCType,pData,VFP2C_ODBC_MAX_BUFFER,&lpCS->nIndicator);
	if (nApiRet == SQL_SUCCESS)
	{
		if (lpCS->nIndicator == SQL_NULL_DATA)
			nRetVal = _Store(&lpCS->lField, &lpCS->vNull);
		else
		{
			vData.ev_length = lpCS->nIndicator;
			nRetVal = _Store(&lpCS->lField, &vData);
		}
		UnlockFreeHandle(vData);
		return nRetVal;
	}
	else if (nApiRet == SQL_SUCCESS_WITH_INFO)
	{
		vData.ev_length = lpCS->nIndicator;
		UnlockHandle(vData);
		if (!SetHandleSize(vData,lpCS->nIndicator+2))
		{
			FreeHandle(vData);
			return E_INSUFMEMORY;
		}
		LockHandle(vData);
		pData = HandleToPtr(vData);
		pData += VFP2C_ODBC_MAX_BUFFER;

		do 
		{
			nApiRet = SQLGetData(lpCS->hStmt,lpCS->nColNo,lpCS->nCType,pData,VFP2C_ODBC_MAX_BUFFER,&lpCS->nIndicator);
			if (nApiRet == SQL_SUCCESS_WITH_INFO)
				pData += VFP2C_ODBC_MAX_BUFFER;
		} while (nApiRet == SQL_SUCCESS_WITH_INFO);

		if (nApiRet == SQL_SUCCESS)
			nRetVal = _Store(&lpCS->lField, &vData);
		else
		{
			SafeODBCStmtError("SQLGetData", lpCS->hStmt);
			nRetVal = E_APIERROR;
		}
		UnlockFreeHandle(vData);
		return nRetVal;
	}
	else
	{
		SafeODBCStmtError("SQLGetData", lpCS->hStmt);
		UnlockFreeHandle(vData);
		return E_APIERROR;
	}
}

void _stdcall Timestamp_StructToDateTime(SQL_TIMESTAMP_STRUCT *pTime, Value *pDateTime)
{
	int lnA, lnY, lnM, lnJDay;
	lnA = (14 - pTime->month) / 12;
	lnY = pTime->year + 4800 - lnA;
	lnM = pTime->month + 12 * lnA - 3;
	lnJDay = pTime->day + (153 * lnM + 2) / 5 + lnY * 365 + lnY / 4 - lnY / 100 + lnY / 400 - 32045;
	pDateTime->ev_real = ((double)lnJDay) + (((double)pTime->hour) * 3600 + pTime->minute * 60 + pTime->second) / 86400;
}

void _stdcall DateTimeToTimestamp_Struct(Value *pDateTime, SQL_TIMESTAMP_STRUCT *pTime)
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

void _stdcall Timestamp_StructToDate(SQL_TIMESTAMP_STRUCT *pTime, Value *pDateTime)
{
	int lnA, lnY, lnM, lnJDay;
	lnA = (14 - pTime->month) / 12;
	lnY = pTime->year + 4800 - lnA;
	lnM = pTime->month + 12 * lnA - 3;
	lnJDay = pTime->day + (153 * lnM + 2) / 5 + lnY * 365 + lnY / 4 - lnY / 100 + lnY / 400 - 32045;
	pDateTime->ev_real = ((double)lnJDay);
}

void _stdcall DateToTimestamp_Struct(Value *pDateTime, SQL_TIMESTAMP_STRUCT *pTime)
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

void _stdcall Numeric_StructToCurrency(SQL_NUMERIC_STRUCT *lpNum, Value *pValue)
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

void _stdcall CurrencyToNumeric_Struct(Value *pValue, SQL_NUMERIC_STRUCT *lpNum)
{
	__int64 *pLow = (__int64*)&lpNum->val[0], *pHigh = (__int64*)&lpNum->val[9];
	lpNum->sign = pValue->ev_currency.QuadPart < 0 ? 2 : 1;
	lpNum->precision = VFP2C_VFP_CURRENCY_PRECISION;
	lpNum->scale = VFP2C_VFP_CURRENCY_SCALE;
	*pLow = pValue->ev_currency.QuadPart;
	*pHigh = 0;
}

void _stdcall CurrencyToNumericLiteral(Value *pValue, SQLCHAR *pLiteral)
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

void _stdcall NumericLiteralToCurrency(SQLCHAR *pLiteral, Value *pValue)
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

/*
void _fastcall TableUpdateEx(ParamBlk *parm)
{
	V_VALUE(vNextRec); V_VALUE(vFieldState); V_VALUE(vDeleted); V_VALUE(vTableUpd);
	char *pSQL = 0;
	char *pCursor, *pTable, *pKeyFields = 0, *pColumns = 0, *pFieldState;
	int nErrorNo, nFlags, nParmCount;
	char aExeBuffer[VFP2C_MAX_FUNCTIONBUFFER];

    nFlags = vp2.ev_long;
	if (!(nFlags & (TABLEUPDATEEX_CURRENT_ROW | TABLEUPDATEEX_ALL_ROWS)))
		nFlags |= TABLEUPDATEEX_CURRENT_ROW;
	if (!(nFlags & (TABLEUPDATEEX_KEY_ONLY | TABLEUPDATEEX_KEY_AND_MODIFIED)))
		nFlags |= TABLEUPDATEEX_KEY_AND_MODIFIED;

	if (!NullTerminateHandle(vp3) || !NullTerminateHandle(vp4) || !NullTerminateHandle(vp5))
		RaiseError(E_INSUFMEMORY);

	LockHandle(vp3);
	LockHandle(vp4);
	LockHandle(vp5);
	pCursor = HandleToPtr(vp3);
	pTable = HandleToPtr(vp4);
	pKeyFields = HandleToPtr(vp5);

	if (VALID_STRING_EX(6))
	{
		if (!NullTerminateHandle(vp6))
		{
			nErrorNo = E_INSUFMEMORY;
			goto ErrorOut;
		}
		LockHandle(vp6);
		pColumns = HandleToPtr(vp6);
	}

	pSQL = malloc(VFP2C_ODBC_MAX_SQLSTATEMENT);
	if (!pSQL)
	{
		nErrorNo = E_INSUFMEMORY;
		goto ErrorOut;
	}

ModifiedCheck:
	if (nFlags & TABLEUPDATEEX_ALL_ROWS)
	{
		sprintfex(aExeBuffer,"GETNEXTMODIFIED(%I,'%S')",vNextRec.ev_long,pCursor);
		if (nErrorNo = EVALUATE(vNextRec,aExeBuffer))
			goto ErrorOut;

		if (vNextRec.ev_long == 0)
			goto Finish;

		sprintfex(aExeBuffer,"GO %I IN %S",vNextRec.ev_long,pCursor);
		if (nErrorNo = EXECUTE(aExeBuffer))
			goto ErrorOut;
	}

	sprintfex(aExeBuffer,"GETFLDSTATE(-1,'%S')+CHR(0)",pCursor);
	if (nErrorNo = EVALUATE(vFieldState,aExeBuffer))
		goto ErrorOut;

	pFieldState = HandleToPtr(vFieldState);

	if (*pFieldState == '1')
	{
		nParmCount = str_charcount(pFieldState,'2');
		if (nParmCount)
		{
			if (nFlags & TABLEUPDATEEX_KEY_AND_MODIFIED)
				nParmCount *= 2;
			nParmCount += GetWordCount(pKeyFields,',');

			nErrorNo = SQLCreateUpdateStmt(pSQL,pCursor,pTable,pKeyFields,pColumns,nFlags);
		}
	}
	else if (*pFieldState == '2')
	{
		sprintfex(aExeBuffer,"DELETED('%S')",pCursor);
		if (nErrorNo = EVALUATE(vDeleted,aExeBuffer))
			goto ErrorOut;

		if (vDeleted.ev_length)
		{
			nParmCount = GetWordCount(pKeyFields,',');
			nErrorNo = SQLCreateDeleteStmt(pSQL,pTable,pKeyFields);
		}
		else
			nParmCount = 0;
	}
	else if (*pFieldState == '3')
	{
		nParmCount = str_charcount(pFieldState,'4');
		if (nParmCount)
			nErrorNo = SQLCreateInsertStmt(pSQL,pCursor,pTable,pKeyFields,pColumns);
	}
	else
		nParmCount = 0;

	if (nErrorNo)
		goto ErrorOut;

	sprintfex(aExeBuffer,"TABLEUPDATE(0,.F.,'%S')",pCursor);
	if (nErrorNo = EVALUATE(vTableUpd,aExeBuffer))
		goto ErrorOut;

	if (!vTableUpd.ev_length)
	{
		SaveCustomError("TableUpdateEx","TABLEUPDATE didn't succeed.");
		goto ErrorOut;
	}

	if (nFlags & TABLEUPDATEEX_ALL_ROWS)
		goto ModifiedCheck;

	Finish:
		UnlockHandle(vp3);
		UnlockHandle(vp4);
		UnlockHandle(vp5);
		if (VALID_STRING_EX(6))
			UnlockHandle(vp6);
		free(pSQL);
		return;

	ErrorOut:
		UnlockHandle(vp3);
		UnlockHandle(vp4);
		UnlockHandle(vp5);
		if (VALID_STRING_EX(6))
			UnlockHandle(vp6);
		if (pSQL)
			free(pSQL);
		RaiseError(nErrorNo);
}

int _stdcall SQLCreateDeleteStmt(char *pSQL, char *pTable, char *pKeyFields)
{
	BOOL bAnd = FALSE;
	char aFieldName[VFP2C_VFP_MAX_COLUMN_NAME];
	char aColumnName[VFP2C_ODBC_MAX_FIELD_NAME];

	pSQL = str_append(pSQL,"DELETE FROM ");
	pSQL = str_append(pSQL,pTable);
	pSQL = str_append(pSQL," WHERE ");

	do
	{
		if (bAnd)
			pSQL = str_append(pSQL," AND ");

		if (!match_identifier(&pKeyFields,aFieldName,VFP2C_VFP_MAX_COLUMN_NAME))
		{
			SaveCustomError("TableUpdateEx", "Keyfieldlist contained invalid data near '%20S', expected fieldname.", pKeyFields);
			return -1;
		}
		
		if (match_dotted_identifier(&pKeyFields,aColumnName,VFP2C_ODBC_MAX_FIELD_NAME))
			pSQL = str_append(pSQL,aColumnName);
		else if (match_quoted_identifier_ex(&pKeyFields,aColumnName,VFP2C_ODBC_MAX_FIELD_NAME))
			pSQL = str_append(pSQL,aColumnName);
		// assume vfp fieldname is the same as backend column name
		else
			pSQL = str_append(pSQL,aFieldName);

		pSQL = str_append(pSQL,"=?");
		bAnd = TRUE;

	} while (match_chr(&pKeyFields,','));
	
	return 0;
}

int _stdcall SQLCreateInsertStmt(char *pSQL, char *pCursor, char *pTable, char *pKeyFields, char *pColumns)
{
	BOOL bComma = FALSE, bAnd = FALSE;
	int nErrorNo, nFields = 0;
	V_VALUE(vFieldState);
	char aCommand[VFP2C_MAX_FUNCTIONBUFFER];
	char aExeBuffer[VFP2C_MAX_FUNCTIONBUFFER];
	char aFieldName[VFP2C_VFP_MAX_COLUMN_NAME];
	char aColumnName[VFP2C_ODBC_MAX_FIELD_NAME];

	sprintfex(aCommand,"GETFLDSTATE('%%S','%S')",pCursor);

	pSQL = str_append(pSQL,"INSERT INTO ");
	pSQL = str_append(pSQL,pTable);
	pSQL = str_append(pSQL," (");
	
	do
	{
		if (!match_identifier(&pColumns,aFieldName,VFP2C_VFP_MAX_COLUMN_NAME))
		{
			SaveCustomError("TableUpdateEx", "Columnlist contained invalid data near '%20S', expected fieldname.", pColumns);
			return -1;
		}

		sprintfex(aExeBuffer,aCommand,aFieldName);
		if (nErrorNo = EVALUATE(vFieldState,aExeBuffer))
			return nErrorNo;

		if (vFieldState.ev_long == 4)
		{
			if (bComma)
				pSQL = str_append(pSQL,",");

			if (match_dotted_identifier(&pColumns,aColumnName,VFP2C_ODBC_MAX_FIELD_NAME))
				pSQL = str_append(pSQL,aColumnName);
			else if (match_quoted_identifier_ex(&pColumns,aColumnName,VFP2C_ODBC_MAX_FIELD_NAME))
				pSQL = str_append(pSQL,aColumnName);
			else
				pSQL = str_append(pSQL,aFieldName);

			bComma = TRUE;
			nFields++;
		}

	} while (match_chr(&pColumns,','));

	pSQL = str_append(pSQL,") VALUES (");

	bComma = FALSE;
	while (nFields--)
	{
		if (bComma)
		{
			*pSQL++ = ',';
			*pSQL++ = '?';
		}
		else 
		{
			*pSQL++ = '?';
			bComma = TRUE;
		}
	}
	pSQL = str_append(pSQL,")");

	return 0;
}

int _stdcall SQLCreateUpdateStmt(char *pSQL, char *pCursor, char *pTable, char *pKeyFields, char *pColumns, int nFlags)
{
	BOOL bAnd = FALSE;
	char aFieldName[VFP2C_VFP_MAX_COLUMN_NAME];
	char aColumnName[VFP2C_ODBC_MAX_FIELD_NAME];

	pSQL = str_append(pSQL," WHERE ");
	do
	{
		if (bAnd)
			pSQL = str_append(pSQL," AND ");

		if (!match_identifier(&pKeyFields,aFieldName,VFP2C_VFP_MAX_COLUMN_NAME))
		{
			SaveCustomError("TableUpdateEx", "Keyfieldlist contained invalid data near '%20S', expected fieldname.", pColumns);
			return -1;
		}

		if (match_dotted_identifier(&pKeyFields,aColumnName,VFP2C_ODBC_MAX_FIELD_NAME))
			pSQL = str_append(pSQL,aColumnName);
		else if (match_quoted_identifier_ex(&pKeyFields,aColumnName,VFP2C_ODBC_MAX_FIELD_NAME))
			pSQL = str_append(pSQL,aColumnName);
		else
			pSQL = str_append(pSQL,aFieldName);

		pSQL = str_append(pSQL,"=?");
		bAnd = TRUE;

	} while (match_chr(&pKeyFields,','));

	return 0;
}
*/