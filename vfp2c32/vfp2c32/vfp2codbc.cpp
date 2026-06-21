#include <windows.h>
#include <cmath>
#include <sql.h>

#if !defined(_WIN64)
#include "pro_ext.h"
#else
#include "pro_ext64.h"
#endif
#include "vfp2c32.h"
#include "vfp2cutil.h"
#include "vfp2codbc.h"
#include "vfp2codbcclasses.h"
#include "vfp2ccppapi.h"

void _stdcall SaveODBCError(const char *pFunction, SQLHANDLE hHandle, SQLSMALLINT nHandleType, bool bAddError)
{
	VFP2CTls& tls = VFP2CTls::Tls();
	if (bAddError)
	{
		if (tls.ErrorCount == VFP2C_MAX_ERRORS)
			return;
		else if (tls.ErrorCount != 0)
			tls.ErrorCount++;
	}
	else
		tls.ErrorCount = 0;

	LPVFP2CERROR pError = &tls.ErrorInfo[tls.ErrorCount];
	SQLRETURN nApiRet;
	SQLSMALLINT nErrorLength, nErrorNo = 1;
	do 
	{
		nApiRet = SQLGetDiagRec(nHandleType, hHandle, nErrorNo, (SQLCHAR*)pError->aSqlState, (SQLINTEGER*)&pError->nErrorNo,
			(SQLCHAR*)pError->aErrorMessage, VFP2C_ERROR_MESSAGE_LEN, &nErrorLength);

		if(SQL_SUCCEEDED(nApiRet))
		{
			pError->nErrorType = VFP2C_ERRORTYPE_ODBC;
			strncpyex(pError->aErrorFunction, pFunction, VFP2C_ERROR_FUNCTION_LEN);
			if (nHandleType == SQL_HANDLE_STMT)
			{
				SQLGetDiagField(nHandleType, hHandle, nErrorNo, SQL_DIAG_ROW_NUMBER, (SQLPOINTER)&pError->nRowNumber, 0, 0);
				SQLGetDiagField(nHandleType, hHandle, nErrorNo, SQL_DIAG_COLUMN_NUMBER, (SQLPOINTER)&pError->nColumnNumber, 0, 0);
			}
			else
			{
				pError->nRowNumber = SQL_ROW_NUMBER_UNKNOWN;
				pError->nColumnNumber = SQL_COLUMN_NUMBER_UNKNOWN;
			}
			pError++;
			tls.ErrorCount++;
			nErrorNo++;
		}
	} while (SQL_SUCCEEDED(nApiRet) && tls.ErrorCount < VFP2C_MAX_ERRORS);
}

void _stdcall SaveODBCInstallerError(const char *pFunction)
{
	VFP2CTls& tls = VFP2CTls::Tls();
	LPVFP2CERROR pError = tls.ErrorInfo;
	WORD nErrorLength, nError = 1;
	tls.ErrorCount = 0;

	while (nError <= 8 && 
		SQLInstallerError(nError, &pError->nErrorNo, pError->aErrorMessage, VFP2C_ERROR_MESSAGE_LEN, &nErrorLength) == SQL_SUCCESS)
	{
		nError++;
		pError->nErrorType = VFP2C_ERRORTYPE_WIN32; // we don't need a SQLState field here 
		strncpyex(pError->aErrorFunction, pFunction, VFP2C_ERROR_FUNCTION_LEN);
		tls.ErrorCount++;
		pError++;
	}
}

SQLHENV _fastcall GetODBCEnvHandle()
{
	int nErrorNo;
	ValueEx vODBCHandle;
	vODBCHandle.SetNull();
	nErrorNo = __Evaluate(vODBCHandle, "VAL(SYS(3053))");
	if (nErrorNo)
	{
		SaveCustomErrorEx("SYS(3053)", "Cannot retrieve ODBC environment handle.", nErrorNo);
		throw E_APIERROR;
	}
	return vODBCHandle.DynamicPtr<SQLHENV>();
}

void _fastcall CreateSQLDataSource(ParamBlkEx& parm)
{
try
{
	FoxString pDsn(parm(1),2);
	FoxString pDriver(parm(2));

	BOOL bRetVal;
	UINT nDSNType = parm(3)->ev_long & ODBC_SYSTEM_DSN ? ODBC_ADD_SYS_DSN : ODBC_ADD_DSN;

	if (pDsn.Len() > SQL_MAX_DSN_LENGTH)
	{
		SaveCustomErrorEx("CreateSQLDataSource", "Dsn is greater than maximum of %U characters.", E_VFP_INVALIDPARAMS, SQL_MAX_DSN_LENGTH);
		throw E_APIERROR;
	}

	bRetVal = SQLConfigDataSource(0, nDSNType, pDriver.Ptr<LPCSTR>(), pDsn.Ptr<LPCSTR>());
	if (!bRetVal)
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
	{
		SaveCustomErrorEx("DeleteSQLDataSource", "Dsn is greater than maximum of %U characters.", E_VFP_INVALIDPARAMS, SQL_MAX_DSN_LENGTH);
		throw E_APIERROR;
	}
	
	strcat(aDSNName,pDsn.Ptr<LPCSTR>());

	bRetVal = SQLConfigDataSource(0, nDSNType, pDriver.Ptr<LPCSTR>(), aDSNName);
	if (!bRetVal)
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

	if (pDsn.Len() > SQL_MAX_DSN_LENGTH)
	{
		SaveCustomErrorEx("ChangeSQLDataSource", "Dsn is greater than maximum of %U characters.", E_VFP_INVALIDPARAMS, SQL_MAX_DSN_LENGTH);
		throw E_APIERROR;
	}

	bRetVal = SQLConfigDataSource(0, nDSNType, pDriver.Ptr<LPCSTR>(), pDsn.Ptr<LPCSTR>());
	if (!bRetVal)
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
	SQLHENVPARAM hEnv = GetODBCEnvHandle();
	FoxArray pArray(parm(1), 1, 2);
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
	
	nApiRet = SQLDataSources(hEnv,nFilter,pDatasource.Ptr<SQLCHAR*>(),SQL_MAX_DSN_LENGTH+1,&nDsnLen,
				pDescription.Ptr<SQLCHAR*>(),ODBC_MAX_DESCRIPTION_LEN,&nDescLen);

	pArray.AutoGrow(8);
	while (nApiRet == SQL_SUCCESS)
	{
		nRow = pArray.Grow();
		pArray(nRow,1) = pDatasource.Len(nDsnLen);
		pArray(nRow,2) = pDescription.Len(nDescLen);
		
		nApiRet = SQLDataSources(hEnv,SQL_FETCH_NEXT,pDatasource.Ptr<SQLCHAR*>(),SQL_MAX_DSN_LENGTH+1,&nDsnLen,
				pDescription.Ptr<SQLCHAR*>(),ODBC_MAX_DESCRIPTION_LEN,&nDescLen);
	}

	if (nApiRet != SQL_NO_DATA)
	{
		SaveODBCError("SQLDataSources", hEnv);
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
	SQLHENVPARAM hEnv = GetODBCEnvHandle();
	FoxArray pArray(parm(1), 1, 2);
	FoxString pDriver(ODBC_MAX_DRIVER_LEN);
	FoxString pAttributes(ODBC_MAX_ATTRIBUTES_LEN);
	SQLRETURN nApiRet;
	SQLSMALLINT nDriverLen, nAttribLen;
	unsigned int nRow;
	
	nApiRet = SQLDrivers(hEnv,SQL_FETCH_FIRST,pDriver.Ptr<SQLCHAR*>(),ODBC_MAX_DRIVER_LEN,&nDriverLen,
						pAttributes.Ptr<SQLCHAR*>(),ODBC_MAX_ATTRIBUTES_LEN,&nAttribLen);

	pArray.AutoGrow(32);
	while (nApiRet == SQL_SUCCESS)
	{
		nRow = pArray.Grow();
		pArray(nRow,1) = pDriver.Len(nDriverLen);
		pArray(nRow,2) = pAttributes.Len(--nAttribLen);
		nApiRet = SQLDrivers(hEnv,SQL_FETCH_NEXT,pDriver.Ptr<SQLCHAR*>(),ODBC_MAX_DRIVER_LEN,&nDriverLen,
						pAttributes.Ptr<SQLCHAR*>(),ODBC_MAX_ATTRIBUTES_LEN,&nAttribLen);
	}

	if (nApiRet != SQL_NO_DATA)
	{
		SaveODBCError("SQLDrivers", hEnv);
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
	SQLHDBCPARAM hConHandle;
	SQLRETURN nApiRet;

	if (parm(1)->Numeric())
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

	if (pConAttribute.ICompare(__sv("trace")))
	{
		BOOL bTrace;
		nApiRet = SQLGetConnectAttr(hConHandle, SQL_ATTR_TRACE, &bTrace,0,0);
		if (nApiRet != SQL_ERROR)
			pRef = bTrace > 0;
	}
	else if (pConAttribute.ICompare(__sv("tracefile")))
	{
		SQLINTEGER nLen;
		pBuffer.Size(MAX_PATH);
		nApiRet = SQLGetConnectAttr(hConHandle,SQL_ATTR_TRACEFILE,pBuffer.Ptr<SQLPOINTER>(),MAX_PATH,&nLen);
		if (nApiRet != SQL_ERROR)
			pRef = pBuffer.Len(nLen);
	}
	else if (pConAttribute.ICompare(__sv("connected")))
	{
		BOOL bConnected;
		nApiRet = SQLGetConnectAttr(hConHandle,SQL_ATTR_CONNECTION_DEAD,&bConnected,0,0);
		if (nApiRet != SQL_ERROR)
			pRef = (bConnected == 0); // negate result, since we want to return if we're connected, but we query if the connection is dead ..
	}
	else if (pConAttribute.ICompare(__sv("isolationlevel")))
	{
		int nIsolation;
		nApiRet = SQLGetConnectAttr(hConHandle,SQL_ATTR_TXN_ISOLATION,&nIsolation,0,0);
		if (nApiRet != SQL_ERROR)				
			pRef = nIsolation;
	}
	else if (pConAttribute.ICompare(__sv("perfdata")))
	{
		if (hConHandle == false)
			throw E_INVALIDPARAMS;

		DWORD nPerfData;
		nApiRet = SQLGetConnectAttr(hConHandle,SQL_COPT_SS_PERF_DATA,&nPerfData,0,0);
		if (nApiRet != SQL_ERROR)
			pRef = nPerfData;
	}
	else
	{
		SaveCustomErrorEx("SQLGetPropEx", "Unknown connection attribute '%128V'.", E_VFP_INVALIDPARAMS, pConAttribute.StringView());
		throw E_APIERROR;
	}

	if (nApiRet == SQL_ERROR)
	{
		SaveODBCError("SQLGetConnectAttr", hConHandle);
		throw E_APIERROR;
	}
	else if (nApiRet == SQL_SUCCESS_WITH_INFO)
	{
		SaveODBCError("SQLGetConnectAttr", hConHandle);
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
	SQLHDBCPARAM hConHandle;
	SQLRETURN nApiRet;

	if (parm(1)->Numeric())
	{
		DWORD nHandle = parm(1)->NumericCast<DWORD>();
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

	if (pConAttribute.ICompare(__sv("trace")))
	{
		if (parm(3)->Vartype() != 'L')
			throw E_INVALIDPARAMS;
		SQLPOINTER pParam = parm(3)->ev_length ? (SQLPOINTER)1 : 0;
		nApiRet = SQLSetConnectAttr(hConHandle,SQL_ATTR_TRACE, pParam, SQL_IS_UINTEGER);
	}
	else if (pConAttribute.ICompare(__sv("tracefile")))
	{
		if (parm(3)->Vartype() != 'C')
			throw E_INVALIDPARAMS;
		
		nApiRet = SQLSetConnectAttr(hConHandle,SQL_ATTR_TRACEFILE,pProperty.Ptr<SQLPOINTER>(), (SQLINTEGER)pProperty.Len());
	}
	else if (pConAttribute.ICompare(__sv("perfdata")))
	{
		if (parm(3)->Vartype() != 'L')
			throw E_INVALIDPARAMS;
		
		SQLPOINTER pParam = parm(3)->ev_length ? (SQLPOINTER)SQL_PERF_START : (SQLPOINTER)SQL_PERF_STOP;
		nApiRet = SQLSetConnectAttr(hConHandle,SQL_COPT_SS_PERF_DATA, pParam, SQL_IS_INTEGER);
	}
	else if (pConAttribute.ICompare(__sv("perfdatafile")))
	{
		if (parm(3)->Vartype() != 'C')
			throw E_INVALIDPARAMS;
		
		nApiRet = SQLSetConnectAttr(hConHandle,SQL_COPT_SS_PERF_DATA_LOG,pProperty.Ptr<SQLPOINTER>(),SQL_NTS);
	}
	else if (pConAttribute.ICompare(__sv("perfdatalog")))
	{
		nApiRet = SQLSetConnectAttr(hConHandle, SQL_COPT_SS_PERF_DATA_LOG_NOW, NULL, 0);
	}
	else
	{
		SaveCustomErrorEx("SQLSetPropEx", "Unknown connection attribute '%128V'.", E_VFP_INVALIDPARAMS, pConAttribute.StringView());
		throw E_APIERROR;
	}

	if (nApiRet == SQL_ERROR)
	{
		SaveODBCError("SQLSetConnectAttr", hConHandle);
		throw E_APIERROR;
	}
	else if (nApiRet == SQL_SUCCESS_WITH_INFO)
	{
		SaveODBCError("SQLSetConnectAttr", hConHandle);
		nRetVal = 2;
	}

	Return(nRetVal);
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

SqlStatement* _fastcall SQLAllocCatalogStatement(ParamBlkEx& parm, VFP2CTls& tls, DWORD nFlags)
{
	SqlStatement* pStmt = 0;

	try
	{
		ValueEx vConHandle;
		ValueEx vTypeSettings;
		CStrBuilder<VFP2C_MAX_FUNCTIONBUFFER> pBuffer;
		vConHandle.SetNull();
		vTypeSettings.SetNull();
		SQLRETURN nApiRet;

		pStmt = new(std::nothrow) SqlStatement();
		if (!pStmt)
			throw E_INSUFMEMORY;

		pStmt->bStoreInfoAsError = tls.SqlStoreInfoAsError;
		pStmt->bMapBinaryToGeneral = tls.SqlMapBinaryToGeneral;
		pStmt->bUnicodeConversion = tls.SqlUnicodeConversion;

		// 1.1 - build command to evaluate connection handle
		pBuffer.Format("SQLGETPROP(%U,'ODBChdbc')", parm(1)->ev_long);

		// 1.2 - evaluate connection handle
		Evaluate(vConHandle, pBuffer);
		pStmt->hConn = vConHandle.DynamicPtr<SQLHDBC>();

		if (CFoxVersion::MajorVersion() >= 9)
		{
			Evaluate(vTypeSettings, "CURSORGETPROP('UseMemoSize',0)+IIF(CURSORGETPROP('MapVarchar',0),256,0)+IIF(CURSORGETPROP('MapBinary',0),512,0)");
			pStmt->nUseMemoSize = (vTypeSettings.ev_long & 255);
			pStmt->bMapVarchar = (vTypeSettings.ev_long & 256) > 0;
			pStmt->bMapVarbinary = (vTypeSettings.ev_long & 512) > 0;
		}
		else
		{
			Evaluate(vTypeSettings, "CURSORGETPROP('UseMemoSize',0)");
			pStmt->nUseMemoSize = vTypeSettings.ev_long;
		}

		// 1.3 - allocate statement handle on connection
		nApiRet = SQLAllocHandle(SQL_HANDLE_STMT, pStmt->hConn, pStmt->hStmt);
		if (nApiRet == SQL_ERROR)
		{
			SaveODBCError("SQLAllocHandle", pStmt->hConn);
			throw E_APIERROR;
		}
		else if (nApiRet == SQL_SUCCESS_WITH_INFO && pStmt->bStoreInfoAsError)
			AddODBCError("SQLAllocHandle", pStmt->hConn);

		pStmt->nFlags = (nFlags & ~SQLCATALOG_METADATA_ID);
		SQLPOINTER nMetaDataId = (nFlags & SQLCATALOG_METADATA_ID) ? reinterpret_cast<SQLPOINTER>((INT_PTR)1) : 0;
		if (nMetaDataId)
		{
			nApiRet = SQLSetStmtAttr(pStmt->hStmt, SQL_ATTR_METADATA_ID, nMetaDataId, 0);
			if (nApiRet == SQL_ERROR)
			{
				SaveODBCError("SQLSetStmtAttr", pStmt->hConn);
				throw E_APIERROR;
			}
			else if (nApiRet == SQL_SUCCESS_WITH_INFO && pStmt->bStoreInfoAsError)
				AddODBCError("SQLSetStmtAttr", pStmt->hConn);
		}

		return pStmt;
	}
	catch (int nErrorNo)
	{
		if (pStmt)
		{
			pStmt->FreeHandle();
			delete pStmt;
		}
		throw nErrorNo;
	}
}

void _fastcall SQLRegisterTableType(ParamBlkEx& parm)
{
	SqlStatement* pStmt = 0;
	SqlTableType* pTableType = 0;

	try
	{
		VFP2CTls& tls = VFP2CTls::Tls();
		tls.ErrorCount = 0;
		FoxString pCatalog(parm(2));
		FoxString pSchema(parm(3));
		FoxString pType(parm(4));
		DWORD nFlags = parm.PCount() >= 5 ? parm(5)->ev_long : (SQLREGISTERTABLETYPE_CATALOG | SQLREGISTERTABLETYPE_SCHEMA);
		SQLRETURN nApiRet;
		SQLUSMALLINT nColumnNo = 0;
		SQLSMALLINT nSQLType, nSQLTypeCode, nScale, bNullable, nNativeType;
		SQLLEN nSQLTypeCodeInd, nDisplaySizeInd;
		SQLUINTEGER nSize, nDisplaySize;
		SQLLEN nColumnNameLen;
		SQLCHAR aColumnName[VFP2C_ODBC_MAX_COLUMN_NAME + 1] = { '\0' };
		CStringView pColumnName = aColumnName[0];

		pStmt = SQLAllocCatalogStatement(parm, tls, 0);

		nApiRet = SQLSetStmtAttr(pStmt->hStmt, SQL_SOPT_SS_NAME_SCOPE, (SQLPOINTER)SQL_SS_NAME_SCOPE_TABLE_TYPE, 0);
		if (nApiRet == SQL_ERROR)
		{
			SaveODBCError("SQLSetStmtAttr", pStmt->hConn);
			throw E_APIERROR;
		}
		else if (nApiRet == SQL_SUCCESS_WITH_INFO && pStmt->bStoreInfoAsError)
			AddODBCError("SQLSetStmtAttr", pStmt->hConn);

		nApiRet = SQLColumns(pStmt->hStmt, pCatalog.Ptr<SQLCHAR*>(), (SQLSMALLINT)pCatalog.Len(), pSchema.Ptr<SQLCHAR*>(), (SQLSMALLINT)pSchema.Len(), pType.Ptr<SQLCHAR*>(), (SQLSMALLINT)pType.Len(), 0, 0);
		if (nApiRet == SQL_ERROR)
		{
			SaveODBCError("SQLColumns", pStmt->hStmt);
			throw E_APIERROR;
		}
		else if (nApiRet == SQL_SUCCESS_WITH_INFO && pStmt->bStoreInfoAsError)
			AddODBCError("SQLColumns", pStmt->hStmt);

		CStringView pCatalogView, pSchemaView;
		if (nFlags & SQLREGISTERTABLETYPE_CATALOG)
			pCatalogView = pCatalog;
		if (nFlags & SQLREGISTERTABLETYPE_SCHEMA)
			pSchemaView =  pSchema;

		pTableType = new(std::nothrow) SqlTableType(pCatalogView, pSchemaView, pType);
		if (!pTableType)
			throw E_INSUFMEMORY;

		pTableType->bMapBinaryToGeneral = pStmt->bMapBinaryToGeneral;
		pTableType->bMapVarchar = pStmt->bMapVarchar;
		pTableType->bMapVarbinary = pStmt->bMapVarbinary;
		pTableType->nUseMemoSize = pStmt->nUseMemoSize;
		pTableType->bUnicodeConversion = pStmt->bUnicodeConversion;
		pTableType->nDefaultDecimals = pStmt->GetDefaultDecimals();

		nApiRet = SQLBindCol(pStmt->hStmt, 4, SQL_C_CHAR, aColumnName, sizeof(aColumnName), &nColumnNameLen);
		if (!SQL_SUCCEEDED(nApiRet))
		{
			SaveODBCError("SQLBindCol", pStmt->hStmt);
			throw E_APIERROR;
		}

		nApiRet = SQLBindCol(pStmt->hStmt, 5, SQL_C_SHORT, &nSQLType, 0, 0);
		if (!SQL_SUCCEEDED(nApiRet))
		{
			SaveODBCError("SQLBindCol", pStmt->hStmt);
			throw E_APIERROR;
		}

		nApiRet = SQLBindCol(pStmt->hStmt, 7, SQL_C_ULONG, &nSize, 0, 0);
		if (!SQL_SUCCEEDED(nApiRet))
		{
			SaveODBCError("SQLBindCol", pStmt->hStmt);
			throw E_APIERROR;
		}

		nApiRet = SQLBindCol(pStmt->hStmt, 9, SQL_C_SSHORT, &nScale, 0, 0);
		if (!SQL_SUCCEEDED(nApiRet))
		{
			SaveODBCError("SQLBindCol", pStmt->hStmt);
			throw E_APIERROR;
		}

		nApiRet = SQLBindCol(pStmt->hStmt, 11, SQL_C_SSHORT, &bNullable, 0, 0);
		if (!SQL_SUCCEEDED(nApiRet))
		{
			SaveODBCError("SQLBindCol", pStmt->hStmt);
			throw E_APIERROR;
		}

		nApiRet = SQLBindCol(pStmt->hStmt, 15, SQL_C_SHORT, &nSQLTypeCode, 0, &nSQLTypeCodeInd);
		if (!SQL_SUCCEEDED(nApiRet))
		{
			SaveODBCError("SQLBindCol", pStmt->hStmt);
			throw E_APIERROR;
		}

		nApiRet = SQLBindCol(pStmt->hStmt, 16, SQL_C_ULONG, &nDisplaySize, 0, &nDisplaySizeInd);
		if (!SQL_SUCCEEDED(nApiRet))
		{
			SaveODBCError("SQLBindCol", pStmt->hStmt);
			throw E_APIERROR;
		}

		nApiRet = SQLBindCol(pStmt->hStmt, 29, SQL_C_SHORT, &nNativeType, 0, 0);
		if (!SQL_SUCCEEDED(nApiRet))
		{
			SaveODBCError("SQLBindCol", pStmt->hStmt);
			throw E_APIERROR;
		}

		nApiRet = SQLFetch(pStmt->hStmt);
		while (SQL_SUCCEEDED(nApiRet))
		{
			SqlTableTypeColumn* lpSC = pTableType->pColumns.Add();
			lpSC->nColumnNo = nColumnNo++;
			pColumnName.Len = nColumnNameLen;
			lpSC->pColumnName = pColumnName;
			lpSC->pColumnName.Len(FixCursorColumnName(lpSC->pColumnName), true);
			lpSC->nSQLType = nSQLType;
			lpSC->nSQLTypeCode = (SQLSMALLINT)((nSQLTypeCodeInd == SQL_NULL_DATA) ? 0 : nSQLTypeCode);
			lpSC->nSize = nSize;
			lpSC->bNullable = bNullable;
			lpSC->nDisplaySize = (nDisplaySizeInd == SQL_NULL_DATA) ? 0 : nDisplaySize;

			switch (lpSC->nSQLType)
			{
				case SQL_CHAR:
					lpSC->nCType = SQL_C_CHAR;
					lpSC->cVFPType = lpSC->nSize > 0 && lpSC->nSize <= pTableType->nUseMemoSize ? 'C' : 'M';
					break;

				case SQL_BINARY:
					lpSC->nCType = SQL_C_BINARY;
					if (pTableType->bMapVarbinary && lpSC->nSize > 0 && lpSC->nSize <= pTableType->nUseMemoSize)
					{
						if (CFoxVersion::MajorVersion() >= 9)
							lpSC->cVFPType = 'Q';
						else
						{
							lpSC->cVFPType = 'C';
							lpSC->bVFPBinary = true;
						}
					}
					else
					{
						if (pTableType->bMapVarbinary == false)
						{
							if (lpSC->nSize == 0)
								lpSC->cVFPType = 'G';
							else
							{
								lpSC->cVFPType = 'M';
								lpSC->bVFPBinary = true;
							}
						}
						else
						{
							if (CFoxVersion::MajorVersion() >= 9)
								lpSC->cVFPType = 'W';
							else
							{
								lpSC->cVFPType = 'M';
								lpSC->bVFPBinary = true;
							}
						}
					}
					break;

				case SQL_VARCHAR:
					lpSC->nCType = SQL_C_CHAR;
					if (lpSC->nSize > 0 && lpSC->nSize <= pTableType->nUseMemoSize)
					{
						if (CFoxVersion::MajorVersion() >= 9)
							lpSC->cVFPType = pTableType->bMapVarchar ? 'V' : 'C';
						else
							lpSC->cVFPType = 'C';
					}
					else
					{
						lpSC->cVFPType = 'M';
					}
					break;

				case SQL_VARBINARY:
					lpSC->nCType = SQL_C_BINARY;
					if (pTableType->bMapVarbinary && lpSC->nSize > 0 && lpSC->nSize <= pTableType->nUseMemoSize)
					{
						if (CFoxVersion::MajorVersion() >= 9)
							lpSC->cVFPType = 'Q';
						else
						{
							lpSC->cVFPType = 'C';
							lpSC->bVFPBinary = true;
						}
					}
					else
					{
						if (CFoxVersion::MajorVersion() >= 9 && pTableType->bMapVarbinary)
							lpSC->cVFPType = 'W';
						else if (lpSC->nSize == 0 && pTableType->bMapBinaryToGeneral)
							lpSC->cVFPType = 'G';
						else
						{
							lpSC->cVFPType = 'M';
							lpSC->bVFPBinary = true;
						}
					}
					break;

				case SQL_LONGVARCHAR:
					lpSC->nCType = SQL_C_CHAR;
					lpSC->cVFPType = 'M';
					break;

				case SQL_WLONGVARCHAR:
					lpSC->nCType = pTableType->bUnicodeConversion ? SQL_C_CHAR : SQL_C_WCHAR;
					lpSC->cVFPType = 'M';
					break;

				case SQL_LONGVARBINARY:
					lpSC->nCType = SQL_C_BINARY;
					if (CFoxVersion::MajorVersion() >= 9 && pTableType->bMapVarbinary)
						lpSC->cVFPType = 'W';
					else
					{
						if (pTableType->bMapBinaryToGeneral)
						{
							lpSC->cVFPType = 'G';
						}
						else
						{
							lpSC->cVFPType = 'M';
							lpSC->bVFPBinary = TRUE;
						}
					}
					break;

				case SQL_WCHAR:
					if (pTableType->bUnicodeConversion)
					{
						lpSC->nCType = SQL_C_CHAR;
						if (lpSC->nSize > 0 && lpSC->nSize <= pTableType->nUseMemoSize)
							lpSC->cVFPType = 'C';
						else
							lpSC->cVFPType = 'M';
					}
					else
					{
						lpSC->nCType = SQL_C_WCHAR;
						if (lpSC->nDisplaySize > 0 && lpSC->nDisplaySize <= pTableType->nUseMemoSize)
							lpSC->cVFPType = 'C';
						else
							lpSC->cVFPType = 'M';
					}
					break;

				case SQL_WVARCHAR:
					if (pTableType->bUnicodeConversion)
					{
						lpSC->nCType = SQL_C_CHAR;
						if (lpSC->nSize > 0 && lpSC->nSize <= pTableType->nUseMemoSize)
							lpSC->cVFPType = pTableType->bMapVarchar ? 'V' : 'C';
						else
							lpSC->cVFPType = 'M';
					}
					else
					{
						lpSC->nCType = SQL_C_WCHAR;
						if (lpSC->nDisplaySize > 0 && lpSC->nDisplaySize <= pTableType->nUseMemoSize)
							lpSC->cVFPType = pTableType->bMapVarchar ? 'V' : 'C';
						else
							lpSC->cVFPType = 'M';
					}
					break;

				case SQL_BIT:
					lpSC->cVFPType = 'L';
					break;

				case SQL_TINYINT:
				case SQL_SMALLINT:
				case SQL_INTEGER:
					lpSC->nCType = SQL_C_SLONG;
					lpSC->cVFPType = 'I';
					break;

				case SQL_BIGINT:
					lpSC->nCType = SQL_C_CHAR;
					lpSC->nSize = VFP2C_ODBC_MAX_BIGINT_LITERAL;
					lpSC->cVFPType = 'C';
					break;

				case SQL_NUMERIC:
				case SQL_DECIMAL:
					lpSC->bMoney = nNativeType == SQLMONEYN ? 1 : 0;
					if (!lpSC->bMoney)
					{
						lpSC->nCType = SQL_C_DOUBLE;
						lpSC->cVFPType = 'N';
						lpSC->nSize += 2;
						if (lpSC->nSize > 20)
							lpSC->nSize = 20;
						if (lpSC->nScale > 16)
							lpSC->nScale = 16;
						break;
					}
					else
					{
						lpSC->nCType = SQL_C_NUMERIC;
						lpSC->cVFPType = 'Y';
					}
					break;

				case SQL_REAL:
				case SQL_FLOAT:
				case SQL_DOUBLE:
					lpSC->nCType = SQL_C_DOUBLE;
					lpSC->cVFPType = 'B';
					break;

				case SQL_DATE:
					switch (lpSC->nSQLTypeCode)
					{
						case SQL_CODE_DATE:
							lpSC->nCType = SQL_C_DATE;
							lpSC->cVFPType = 'D';
							break;
						case SQL_CODE_TIME:
							lpSC->nCType = SQL_C_TIME;
							lpSC->cVFPType = 'T'; // TODO - mapping correct?
							break;
						case SQL_CODE_TIMESTAMP:
							if (lpSC->nScale == 0)
							{
								lpSC->nCType = SQL_C_TIMESTAMP;
								lpSC->cVFPType = 'T';
							}
							else
							{
								lpSC->nCType = SQL_C_CHAR;
								lpSC->cVFPType = 'C';
							}
							break;
						default:
							break;
					}
					break;

				case SQL_TYPE_DATE:
					lpSC->nCType = SQL_C_TYPE_DATE;
					lpSC->cVFPType = 'D';
					break;

				case SQL_TIMESTAMP:
					lpSC->nCType = SQL_C_TIMESTAMP;
					lpSC->cVFPType = 'T';
					break;

				case SQL_TYPE_TIMESTAMP:
					if (lpSC->nScale == 0)
					{
						lpSC->nCType = SQL_C_TYPE_TIMESTAMP;
						lpSC->cVFPType = 'T';
					}
					else
					{
						lpSC->nCType = SQL_C_CHAR;
						lpSC->cVFPType = 'C';
					}
					break;

				case SQL_INTERVAL:
					lpSC->nCType = SQL_C_BINARY;
					if (CFoxVersion::MajorVersion() >= 9)
						lpSC->cVFPType = 'Q';
					else
					{
						lpSC->cVFPType = 'C';
						lpSC->bVFPBinary = true;
					}
					break;

				case SQL_TYPE_TIME:
				case SQL_SS_TIME2:
				case SQL_SS_TIMESTAMPOFFSET:
					lpSC->nCType = SQL_C_CHAR;
					lpSC->cVFPType = 'C';
					break;

				case SQL_GUID:
					lpSC->nSize = VFP2C_ODBC_MAX_GUID_LITERAL;
					lpSC->cVFPType = 'C';
					break;

				case SQL_SS_VARIANT:
					lpSC->nCType = SQL_C_BINARY;
					if (CFoxVersion::MajorVersion() >= 9)
						lpSC->cVFPType = 'W';
					else
					{
						lpSC->cVFPType = 'M';
						lpSC->bVFPBinary = true;
					}
					break;

				case SQL_SS_UDT:
					lpSC->nCType = SQL_C_BINARY;
					if (lpSC->nSize <= 254)
					{
						if (CFoxVersion::MajorVersion() >= 9)
							lpSC->cVFPType = 'Q';
						else
						{
							lpSC->cVFPType = 'C';
							lpSC->bVFPBinary = true;
						}
					}
					else
					{
						if (CFoxVersion::MajorVersion() >= 9)
							lpSC->cVFPType = 'W';
						else
						{
							lpSC->cVFPType = 'M';
							lpSC->bVFPBinary = true;
						}
					}
					break;

				case SQL_SS_XML:
					lpSC->nCType = SQL_C_BINARY;
					if (CFoxVersion::MajorVersion() >= 9)
						lpSC->cVFPType = 'W';
					else
					{
						lpSC->cVFPType = 'M';
						lpSC->bVFPBinary = true;
					}
					break;

				default:
					if (lpSC->nDisplaySize > 0 && lpSC->nDisplaySize <= pTableType->nUseMemoSize)
					{
						lpSC->nSize = lpSC->nDisplaySize;
						lpSC->cVFPType = 'C';
					}
					else
						lpSC->cVFPType = 'M';
					break;
			}

			nApiRet = SQLFetch(pStmt->hStmt);
		}
		if (nApiRet == SQL_ERROR)
		{
 			SaveODBCError("SQLFetch", pStmt->hStmt);
			throw E_APIERROR;
		}

		if (!pStmt->FreeHandle())
			throw E_APIERROR;
		delete pStmt;

		int nIndex = g_RegisteredTableTypes.FindKey(pTableType->Key());
		if (nIndex > 0)
		{
			SqlTableType* pOld = g_RegisteredTableTypes.GetValueAt(nIndex);
			g_RegisteredTableTypes.SetAtIndex(nIndex, pTableType->Key(), pTableType);
			delete pOld;
		}
		else
		{
			g_RegisteredTableTypes.Add(pTableType->Key(), pTableType);
		}
		Return(tls.ErrorCount == 0 ? 1 : -1);
	}
	catch (int nErrorNo)
	{
		if (pStmt)
		{
			pStmt->FreeHandle();
			delete pStmt;
		}

		if (pTableType)
			delete pTableType;

		if (nErrorNo == E_APIERROR)
			nErrorNo = 0;

		if (nErrorNo)
			RaiseError(nErrorNo);
		else
			Return(-1);
	}
}

void _fastcall SQLCreateTableTypeCursor(ParamBlkEx& parm)
{
	try
	{
		FoxString pCatalog(parm(1));
		FoxString pSchema(parm(2));
		FoxString pType(parm(3));
		FoxString pCursor(parm(4));
		FoxString vStringValue;
		FoxArray pArray;
		FoxVariable pArrayRef;
		ValueEx vNumeric;
		ValueEx vLogical;
		vNumeric.SetInt();
		vLogical.SetLogical();
		CStrBuilder<VFP2C_MAX_FUNCTIONBUFFER> pArrayName;
		CStrBuilder<VFP2C_MAX_FUNCTIONBUFFER> pExeBuffer;
		SqlTableType pTableType(pCatalog, pSchema, pType);
		SqlTableType* pFoundType = g_RegisteredTableTypes.Lookup(pTableType.Key());
		if (!pFoundType)
		{
			SaveCustomError("SQLCreateTableTypeCursor", "Table type '%S' not registered.", pTableType.Key());
			throw E_APIERROR;
		}
		// create unique array name .. since pStmt is a dynamically allocated pointer it's value is
		// always unique .. so we can use it to build a variable name ..
		pArrayName.Format("__VFP2C_ODBC_ARRAY_%U", pFoundType);
		pArray.Dimension(pArrayName, pFoundType->pColumns.GetCount(), 6);
		pArrayRef.Attach(pArrayName);

		vStringValue.Size(VFP2C_ODBC_MAX_COLUMN_NAME + 1);

		for (unsigned int nRow = 1; nRow <= pFoundType->pColumns.GetCount(); nRow++)
		{
			SqlTableTypeColumn* lpSC = pFoundType->pColumns[nRow - 1];

			if ((lpSC->cVFPType == 'C' || lpSC->cVFPType == 'V' || lpSC->cVFPType == 'Q') && lpSC->nSize > 254)
			{
				SaveCustomError("SqlExecEx", "Invalid cursor schema for field '%S': length %I", lpSC->pColumnName.Ptr<char*>(), lpSC->nSize);
				throw E_APIERROR;
			}
			else if ((lpSC->cVFPType == 'N' || lpSC->cVFPType == 'F') && (lpSC->nSize > 20 || lpSC->nScale > 19))
			{
				SaveCustomError("SqlExecEx", "Invalid cursor schema for field '%S': length %I, scale: %I", lpSC->pColumnName.Ptr<char*>(), lpSC->nSize, lpSC->nScale);
				throw E_APIERROR;
			}
			else if (lpSC->cVFPType == 'B' && lpSC->nScale > 18)
			{
				SaveCustomError("SqlExecEx", "Invalid cursor schema for field '%S': length %I", lpSC->pColumnName.Ptr<char*>(), lpSC->nScale);
				throw E_APIERROR;
			}

			// store fieldname
			vStringValue = lpSC->pColumnName.StringView();
			pArray(nRow, 1) = vStringValue;

			// store fieldtype
			vStringValue.Len(1);
			vStringValue[0] = (char)lpSC->cVFPType;
			pArray(nRow, 2) = vStringValue;

			// store field width/precision
			// if the VFP datatype is none of the below ones .. the field width is unneccesary
			if (lpSC->cVFPType == 'C' || lpSC->cVFPType == 'V' || lpSC->cVFPType == 'Q' ||
				lpSC->cVFPType == 'F' || lpSC->cVFPType == 'N')
				vNumeric.ev_long = (long)lpSC->nSize;
			else
				vNumeric.ev_long = 0;

			pArray(nRow, 3) = vNumeric;

			// store field scale
			// if the VFP datatype if none of the below ones .. the field scale is unneccesary
			if (lpSC->cVFPType == 'N' || lpSC->cVFPType == 'F' || lpSC->cVFPType == 'B')
				vNumeric.ev_long = lpSC->nScale;
			else
				vNumeric.ev_long = 0;

			pArray(nRow, 4) = vNumeric;

			// store if field is nullable
			vLogical.ev_length = lpSC->bNullable;
			pArray(nRow, 5) = vLogical;

			// store if field is binary (NOCPTRANS)
			vLogical.ev_length = lpSC->bVFPBinary ? 1 : 0;
			pArray(nRow, 6) = vLogical;
		}

		CStringView pCursorView = pCursor, pArrayView = pArrayName;
		pExeBuffer.Format("CREATE CURSOR %V FROM ARRAY %V", &pCursorView, &pArrayView);
		Execute(pExeBuffer);
		Return(1);
	}
	catch (int nErrorNo)
	{
		if (nErrorNo != E_APIERROR)
			RaiseError(nErrorNo);
		else
			Return(-1);
	}
}

void _fastcall SQLCatalogFetch(SqlStatement* pStmt, FoxString& pCursor, VFP2CTls& tls)
{
	SQLSMALLINT nNoOfCols = pStmt->GetColumnCount();
	bool bFetchAborted = false;
	int nErrorNo;

	SqlResultSet* pResultSet = pStmt->AddResultSet();
	pResultSet->AllocateColumns(nNoOfCols);

	pResultSet->GetColumnMetaData();
	pResultSet->pCursorName = pCursor;

	if (pStmt->nFlags & SQLEXECEX_REUSE_CURSOR)
		pResultSet->nWorkArea = Select(pResultSet->pCursorName);

	// 7
	pResultSet->PrepareColumnBindings();

	// 8
	pResultSet->BindColumns();

	if (pResultSet->nWorkArea && (pStmt->nFlags & SQLEXECEX_REUSE_CURSOR))
	{
		if ((pStmt->nFlags & SQLEXECEX_APPEND_CURSOR) == 0)
		{
			AutoOnOffSetting pSetting("SAFETY", false);
			nErrorNo = Zap(pResultSet->nWorkArea);
			if (nErrorNo)
				throw nErrorNo;
		}
		pResultSet->pRecNoRestore.SaveRecNo(pResultSet->nWorkArea, (pStmt->nFlags & SQLEXECEX_PRESERVE_RECNO) > 0, true);
	}
	else
	{
		pResultSet->CreateCursor();
		pResultSet->pRecNoRestore.SaveRecNo(pResultSet->nWorkArea, false, true);
	}

	pResultSet->BindFieldLocators();

	// 9.
	pResultSet->FetchToCursor(&bFetchAborted);

	if (!pStmt->FreeHandle())
		throw E_APIERROR;
	delete pStmt;
	Return(tls.ErrorCount == 0 ? 1 : -1);
}

void _fastcall SQLTablesEx(ParamBlkEx& parm)
{
	SqlStatement* pStmt = 0;
	try
	{
		VFP2CTls& tls = VFP2CTls::Tls();
		tls.ErrorCount = 0;
		FoxString pCatalog(parm, 2, FoxString::NoNullIfEmpty);
		FoxString pSchema(parm, 3, FoxString::NoNullIfEmpty);
		FoxString pTable(parm, 4, FoxString::NoNullIfEmpty);
		FoxString pTableType(parm, 5, FoxString::NoNullIfEmpty);
		FoxString pCursor(parm(6));
		DWORD nFlags = parm.PCount() >= 7 ? parm(7)->NumericCast<DWORD>() : 0;
		UINT_PTR nScope = parm.PCount() >= 8 ? parm(8)->NumericCast<UINT_PTR>() : 0;
		SQLRETURN nApiRet;

		pStmt = SQLAllocCatalogStatement(parm, tls, nFlags);

		if (nScope)
		{
			nApiRet = SQLSetStmtAttr(pStmt->hStmt, SQL_SOPT_SS_NAME_SCOPE, (SQLPOINTER)nScope, 0);
			if (nApiRet == SQL_ERROR)
			{
				SaveODBCError("SQLSetStmtAttr", pStmt->hConn);
				throw E_APIERROR;
			}
			else if (nApiRet == SQL_SUCCESS_WITH_INFO && pStmt->bStoreInfoAsError)
				AddODBCError("SQLSetStmtAttr", pStmt->hConn);
		}

		nApiRet = SQLTables(pStmt->hStmt, pCatalog.Ptr<SQLCHAR*>(), pCatalog.Len() ? SQL_NTS : 0, pSchema.Ptr<SQLCHAR*>(), pSchema.Len() ? SQL_NTS : 0,
			pTable.Ptr<SQLCHAR*>(), pTable.Len() ? SQL_NTS : 0, pTableType.Ptr<SQLCHAR*>(), pTableType.Len() ? SQL_NTS : 0);
		if (nApiRet == SQL_ERROR)
		{
			SaveODBCError("SQLTables", pStmt->hStmt);
			throw E_APIERROR;
		}
		else if (nApiRet == SQL_SUCCESS_WITH_INFO && pStmt->bStoreInfoAsError)
			AddODBCError("SQLTables", pStmt->hStmt);

		SQLCatalogFetch(pStmt, pCursor, tls);
	}
	catch (int nErrorNo)
	{
		if (pStmt)
		{
			pStmt->FreeHandle();
			delete pStmt;
		}

		if (nErrorNo != E_APIERROR)
			RaiseError(nErrorNo);
		else
			Return(-1);
	}
}

void _fastcall SQLColumnsEx(ParamBlkEx& parm)
{
	SqlStatement* pStmt = 0;
	try
	{
		VFP2CTls& tls = VFP2CTls::Tls();
		tls.ErrorCount = 0;
		FoxString pCatalog(parm(2));
		FoxString pSchema(parm(3));
		FoxString pTable(parm(4));
		FoxString pColumn(parm(5));
		FoxString pCursor(parm(6));
		DWORD nFlags = parm.PCount() >= 7 ? parm(7)->NumericCast<DWORD>() : 0;
		UINT_PTR nScope = parm.PCount() >= 8 ? parm(8)->NumericCast<UINT_PTR>() : 0;
		SQLRETURN nApiRet;

		pStmt = SQLAllocCatalogStatement(parm, tls, nFlags);

		if (nScope)
		{
			nApiRet = SQLSetStmtAttr(pStmt->hStmt, SQL_SOPT_SS_NAME_SCOPE, (SQLPOINTER)nScope, 0);
			if (nApiRet == SQL_ERROR)
			{
				SaveODBCError("SQLSetStmtAttr", pStmt->hConn);
				throw E_APIERROR;
			}
			else if (nApiRet == SQL_SUCCESS_WITH_INFO && pStmt->bStoreInfoAsError)
				AddODBCError("SQLSetStmtAttr", pStmt->hConn);
		}

		nApiRet = SQLColumns(pStmt->hStmt, pCatalog.Ptr<SQLCHAR*>(), (SQLSMALLINT)pCatalog.Len(), pSchema.Ptr<SQLCHAR*>(), (SQLSMALLINT)pSchema.Len(), pTable.Ptr<SQLCHAR*>(), (SQLSMALLINT)pTable.Len(), pColumn.Ptr<SQLCHAR*>(), (SQLSMALLINT)pColumn.Len());
		if (nApiRet == SQL_ERROR)
		{
			SaveODBCError("SQLColumns", pStmt->hStmt);
			throw E_APIERROR;
		}
		else if (nApiRet == SQL_SUCCESS_WITH_INFO && pStmt->bStoreInfoAsError)
			AddODBCError("SQLColumns", pStmt->hStmt);

		SQLCatalogFetch(pStmt, pCursor, tls);
	}
	catch (int nErrorNo)
	{
		if (pStmt)
		{
			pStmt->FreeHandle();
			delete pStmt;
		}

		if (nErrorNo != E_APIERROR)
			RaiseError(nErrorNo);
		else
			Return(-1);
	}
}

void _fastcall SQLPrimaryKeysEx(ParamBlkEx& parm)
{
	SqlStatement* pStmt = 0;
	try
	{
		VFP2CTls& tls = VFP2CTls::Tls();
		tls.ErrorCount = 0;
		FoxString pCatalog(parm(2));
		FoxString pSchema(parm(3));
		FoxString pTable(parm(4));
		FoxString pCursor(parm(5));
		DWORD nFlags = parm.PCount() >= 6 ? parm(6)->NumericCast<DWORD>() : 0;

		pStmt = SQLAllocCatalogStatement(parm, tls, nFlags);
		SQLRETURN nApiRet = SQLPrimaryKeys(pStmt->hStmt, pCatalog.Ptr<SQLCHAR*>(), (SQLSMALLINT)pCatalog.Len(), pSchema.Ptr<SQLCHAR*>(), (SQLSMALLINT)pSchema.Len(), pTable.Ptr<SQLCHAR*>(), (SQLSMALLINT)pTable.Len());
		if (nApiRet == SQL_ERROR)
		{
			SaveODBCError("SQLPrimaryKeys", pStmt->hStmt);
			throw E_APIERROR;
		}
		else if (nApiRet == SQL_SUCCESS_WITH_INFO && pStmt->bStoreInfoAsError)
			AddODBCError("SQLPrimaryKeys", pStmt->hStmt);

		SQLCatalogFetch(pStmt, pCursor, tls);
	}
	catch (int nErrorNo)
	{
		if (pStmt)
		{
			pStmt->FreeHandle();
			delete pStmt;
		}

		if (nErrorNo != E_APIERROR)
			RaiseError(nErrorNo);
		else
			Return(-1);
	}
}

void _fastcall SQLForeignKeysEx(ParamBlkEx& parm)
{
	SqlStatement* pStmt = 0;
	try
	{
		VFP2CTls& tls = VFP2CTls::Tls();
		tls.ErrorCount = 0;
		FoxString pCatalog(parm(2));
		FoxString pSchema(parm(3));
		FoxString pTable(parm(4));
		FoxString pCatalog2(parm(5));
		FoxString pSchema2(parm(6));
		FoxString pTable2(parm(7));
		FoxString pCursor(parm(8));
		DWORD nFlags = parm.PCount() >= 9 ? parm(9)->NumericCast<DWORD>() : 0;

		pStmt = SQLAllocCatalogStatement(parm, tls, nFlags);
		SQLRETURN nApiRet = SQLForeignKeys(pStmt->hStmt, pCatalog.Ptr<SQLCHAR*>(), (SQLSMALLINT)pCatalog.Len(), pSchema.Ptr<SQLCHAR*>(),
			(SQLSMALLINT)pSchema.Len(), pTable.Ptr<SQLCHAR*>(), (SQLSMALLINT)pTable.Len(), 	pCatalog2.Ptr<SQLCHAR*>(), (SQLSMALLINT)pCatalog2.Len(), 
			pSchema2.Ptr<SQLCHAR*>(), (SQLSMALLINT)pSchema2.Len(), pTable2.Ptr<SQLCHAR*>(), (SQLSMALLINT)pTable2.Len());
		if (nApiRet == SQL_ERROR)
		{
			SaveODBCError("SQLForeignKeys", pStmt->hStmt);
			throw E_APIERROR;
		}
		else if (nApiRet == SQL_SUCCESS_WITH_INFO && pStmt->bStoreInfoAsError)
			AddODBCError("SQLForeignKeys", pStmt->hStmt);

		SQLCatalogFetch(pStmt, pCursor, tls);
	}
	catch (int nErrorNo)
	{
		if (pStmt)
		{
			pStmt->FreeHandle();
			delete pStmt;
		}

		if (nErrorNo != E_APIERROR)
			RaiseError(nErrorNo);
		else
			Return(-1);
	}
}

void _fastcall SQLProceduresEx(ParamBlkEx& parm)
{
	SqlStatement* pStmt = 0;
	try
	{
		VFP2CTls& tls = VFP2CTls::Tls();
		tls.ErrorCount = 0;
		FoxString pCatalog(parm(2));
		FoxString pSchema(parm(3));
		FoxString pProcedure(parm(4));
		FoxString pCursor(parm(5));
		DWORD nFlags = parm.PCount() >= 6 ? parm(6)->NumericCast<DWORD>() : 0;

		pStmt = SQLAllocCatalogStatement(parm, tls, nFlags);
		SQLRETURN nApiRet = SQLProcedures(pStmt->hStmt, pCatalog.Ptr<SQLCHAR*>(), (SQLSMALLINT)pCatalog.Len(), pSchema.Ptr<SQLCHAR*>(), (SQLSMALLINT)pSchema.Len(),
			pProcedure.Ptr<SQLCHAR*>(), (SQLSMALLINT)pProcedure.Len());
		if (nApiRet == SQL_ERROR)
		{
			SaveODBCError("SQLProcedures", pStmt->hStmt);
			throw E_APIERROR;
		}
		else if (nApiRet == SQL_SUCCESS_WITH_INFO && pStmt->bStoreInfoAsError)
			AddODBCError("SQLProcedures", pStmt->hStmt);

		SQLCatalogFetch(pStmt, pCursor, tls);
	}
	catch (int nErrorNo)
	{
		if (pStmt)
		{
			pStmt->FreeHandle();
			delete pStmt;
		}

		if (nErrorNo != E_APIERROR)
			RaiseError(nErrorNo);
		else
			Return(-1);
	}
}

void _fastcall SQLProcedureColumnsEx(ParamBlkEx& parm)
{
	SqlStatement* pStmt = 0;
	try
	{
		VFP2CTls& tls = VFP2CTls::Tls();
		tls.ErrorCount = 0;
		FoxString pCatalog(parm(2));
		FoxString pSchema(parm(3));
		FoxString pProcedure(parm(4));
		FoxString pColumn(parm(5));
		FoxString pCursor(parm(6));
		DWORD nFlags = parm.PCount() >= 7 ? parm(7)->NumericCast<DWORD>() : 0;

		pStmt = SQLAllocCatalogStatement(parm, tls, nFlags);
		SQLRETURN nApiRet = SQLProcedureColumns(pStmt->hStmt, pCatalog.Ptr<SQLCHAR*>(), (SQLSMALLINT)pCatalog.Len(), 
			pSchema.Ptr<SQLCHAR*>(), (SQLSMALLINT)pSchema.Len(), pProcedure.Ptr<SQLCHAR*>(), (SQLSMALLINT)pProcedure.Len(),
			pColumn.Ptr<SQLCHAR*>(), (SQLSMALLINT)pColumn.Len());
		if (nApiRet == SQL_ERROR)
		{
			SaveODBCError("SQLProcedureColumns", pStmt->hStmt);
			throw E_APIERROR;
		}
		else if (nApiRet == SQL_SUCCESS_WITH_INFO && pStmt->bStoreInfoAsError)
			AddODBCError("SQLProcedureColumns", pStmt->hStmt);

		SQLCatalogFetch(pStmt, pCursor, tls);
	}
	catch (int nErrorNo)
	{
		if (pStmt)
		{
			pStmt->FreeHandle();
			delete pStmt;
		}

		if (nErrorNo != E_APIERROR)
			RaiseError(nErrorNo);
		else
			Return(-1);
	}
}

void _fastcall SQLTablePrivilegesEx(ParamBlkEx& parm)
{
	SqlStatement* pStmt = 0;
	try
	{
		VFP2CTls& tls = VFP2CTls::Tls();
		tls.ErrorCount = 0;
		FoxString pCatalog(parm(2));
		FoxString pSchema(parm(3));
		FoxString pTable(parm(4));
		FoxString pCursor(parm(5));
		DWORD nFlags = parm.PCount() >= 6 ? parm(6)->NumericCast<DWORD>() : 0;

		pStmt = SQLAllocCatalogStatement(parm, tls, nFlags);
		SQLRETURN nApiRet = SQLTablePrivileges(pStmt->hStmt, pCatalog.Ptr<SQLCHAR*>(), (SQLSMALLINT)pCatalog.Len(), 
			pSchema.Ptr<SQLCHAR*>(), (SQLSMALLINT)pSchema.Len(), pTable.Ptr<SQLCHAR*>(), (SQLSMALLINT)pTable.Len());
		if (nApiRet == SQL_ERROR)
		{
			SaveODBCError("SQLTablePrivileges", pStmt->hStmt);
			throw E_APIERROR;
		}
		else if (nApiRet == SQL_SUCCESS_WITH_INFO && pStmt->bStoreInfoAsError)
			AddODBCError("SQLTablePrivileges", pStmt->hStmt);

		SQLCatalogFetch(pStmt, pCursor, tls);
	}
	catch (int nErrorNo)
	{
		if (pStmt)
		{
			pStmt->FreeHandle();
			delete pStmt;
		}

		if (nErrorNo != E_APIERROR)
			RaiseError(nErrorNo);
		else
			Return(-1);
	}
}

void _fastcall SQLColumnPrivilegesEx(ParamBlkEx& parm)
{
	SqlStatement* pStmt = 0;
	try
	{
		VFP2CTls& tls = VFP2CTls::Tls();
		tls.ErrorCount = 0;
		FoxString pCatalog(parm(2));
		FoxString pSchema(parm(3));
		FoxString pTable(parm(4));
		FoxString pColumn(parm(5));
		FoxString pCursor(parm(6));
		DWORD nFlags = parm.PCount() >= 7 ? parm(7)->NumericCast<DWORD>() : 0;

		pStmt = SQLAllocCatalogStatement(parm, tls, nFlags);
		SQLRETURN nApiRet = SQLColumnPrivileges(pStmt->hStmt, pCatalog.Ptr<SQLCHAR*>(), (SQLSMALLINT)pCatalog.Len(), 
			pSchema.Ptr<SQLCHAR*>(), (SQLSMALLINT)pSchema.Len(), pTable.Ptr<SQLCHAR*>(), (SQLSMALLINT)pTable.Len(),
			pColumn.Ptr<SQLCHAR*>(), (SQLSMALLINT)pColumn.Len());
		if (nApiRet == SQL_ERROR)
		{
			SaveODBCError("SQLColumnPrivileges", pStmt->hStmt);
			throw E_APIERROR;
		}
		else if (nApiRet == SQL_SUCCESS_WITH_INFO && pStmt->bStoreInfoAsError)
			AddODBCError("SQLColumnPrivileges", pStmt->hStmt);

		SQLCatalogFetch(pStmt, pCursor, tls);
	}
	catch (int nErrorNo)
	{
		if (pStmt)
		{
			pStmt->FreeHandle();
			delete pStmt;
		}

		if (nErrorNo != E_APIERROR)
			RaiseError(nErrorNo);
		else
			Return(-1);
	}
}

void _fastcall SQLSpecialColumnsEx(ParamBlkEx& parm)
{
	SqlStatement* pStmt = 0;
	try
	{
		VFP2CTls& tls = VFP2CTls::Tls();
		tls.ErrorCount = 0;
		FoxString pCatalog(parm(2));
		FoxString pSchema(parm(3));
		FoxString pTable(parm(4));
		SQLUSMALLINT nIdentifier = (SQLUSMALLINT)parm(5)->ev_long;
		SQLUSMALLINT nScope = (SQLUSMALLINT)parm(6)->ev_long;
		SQLUSMALLINT nNullable = (SQLUSMALLINT)parm(7)->ev_length;
		FoxString pCursor(parm(8));
		DWORD nFlags = parm.PCount() >= 9 ? parm(9)->NumericCast<DWORD>() : 0;
		
		pStmt = SQLAllocCatalogStatement(parm, tls, nFlags);
		SQLRETURN nApiRet = SQLSpecialColumns(pStmt->hStmt, nIdentifier, pCatalog.Ptr<SQLCHAR*>(), (SQLSMALLINT)pCatalog.Len(), 
			pSchema.Ptr<SQLCHAR*>(), (SQLSMALLINT)pSchema.Len(), pTable.Ptr<SQLCHAR*>(), (SQLSMALLINT)pTable.Len(),
			nScope, nNullable);
		if (nApiRet == SQL_ERROR)
		{
			SaveODBCError("SQLColumnPrivileges", pStmt->hStmt);
			throw E_APIERROR;
		}
		else if (nApiRet == SQL_SUCCESS_WITH_INFO && pStmt->bStoreInfoAsError)
			AddODBCError("SQLColumnPrivileges", pStmt->hStmt);

		SQLCatalogFetch(pStmt, pCursor, tls);
	}
	catch (int nErrorNo)
	{
		if (pStmt)
		{
			pStmt->FreeHandle();
			delete pStmt;
		}

		if (nErrorNo != E_APIERROR)
			RaiseError(nErrorNo);
		else
			Return(-1);
	}
}

void _fastcall SQLStatisticsEx(ParamBlkEx& parm)
{
	SqlStatement* pStmt = 0;
	try
	{
		VFP2CTls& tls = VFP2CTls::Tls();
		tls.ErrorCount = 0;
		FoxString pCatalog(parm(2));
		FoxString pSchema(parm(3));
		FoxString pTable(parm(4));
		FoxString pColumn(parm(5));
		SQLUSMALLINT nUnique = (SQLUSMALLINT)parm(6)->ev_long;
		SQLUSMALLINT nReserved = (SQLUSMALLINT)parm(7)->ev_length;
		FoxString pCursor(parm(8));
		DWORD nFlags = parm.PCount() >= 9 ? parm(9)->NumericCast<DWORD>() : 0;
		
		pStmt = SQLAllocCatalogStatement(parm, tls, nFlags);
		SQLRETURN nApiRet = SQLStatistics(pStmt->hStmt, pCatalog.Ptr<SQLCHAR*>(), (SQLSMALLINT)pCatalog.Len(), 
			pSchema.Ptr<SQLCHAR*>(), (SQLSMALLINT)pSchema.Len(), pTable.Ptr<SQLCHAR*>(), (SQLSMALLINT)pTable.Len(),
			nUnique, nReserved);
		if (nApiRet == SQL_ERROR)
		{
			SaveODBCError("SQLStatistics", pStmt->hStmt);
			throw E_APIERROR;
		}
		else if (nApiRet == SQL_SUCCESS_WITH_INFO && pStmt->bStoreInfoAsError)
			AddODBCError("SQLStatistics", pStmt->hStmt);

		SQLCatalogFetch(pStmt, pCursor, tls);
	}
	catch (int nErrorNo)
	{
		if (pStmt)
		{
			pStmt->FreeHandle();
			delete pStmt;
		}

		if (nErrorNo != E_APIERROR)
			RaiseError(nErrorNo);
		else
			Return(-1);
	}
}

void _fastcall SQLGetTypeInfoEx(ParamBlkEx& parm)
{
	SqlStatement* pStmt = 0;
	try
	{
		VFP2CTls& tls = VFP2CTls::Tls();
		tls.ErrorCount = 0;
		SQLSMALLINT nType = (SQLSMALLINT)parm(2)->ev_long;
		FoxString pCursor(parm(3));
		DWORD nFlags = parm.PCount() >= 4 ? parm(4)->NumericCast<DWORD>() : 0;

		pStmt = SQLAllocCatalogStatement(parm, tls, nFlags);
		SQLRETURN nApiRet = SQLGetTypeInfo(pStmt->hStmt, nType);
		if (nApiRet == SQL_ERROR)
		{
			SaveODBCError("SQLStatistics", pStmt->hStmt);
			throw E_APIERROR;
		}
		else if (nApiRet == SQL_SUCCESS_WITH_INFO && pStmt->bStoreInfoAsError)
			AddODBCError("SQLStatistics", pStmt->hStmt);

		SQLCatalogFetch(pStmt, pCursor, tls);
	}
	catch (int nErrorNo)
	{
		if (pStmt)
		{
			pStmt->FreeHandle();
			delete pStmt;
		}

		if (nErrorNo != E_APIERROR)
			RaiseError(nErrorNo);
		else
			Return(-1);
	}
}

void _fastcall SQLExecEx(ParamBlkEx& parm)
{
	// 1. setup
	// 2. parse & rewrite SQL for parameter markers (replace ?{varName} with ?)
	// 3. BindParameters() - Bind input/output parameters
	// 4. SQLExecDirect() - Execute SQL statement
	// 4.1 if SQLExecDirect returns SQL_NEED_DATA we need to send long parameter data with SQLParamData/SQLPutData
	// 5. GetColumnCount() - find out number of columns in result, if no resultset was generated goto step 9.3
	// 6. GetColunMetaData() - get meta data for columns
	// 6.1 SQLParseCursorSchema() - if a custom cursorschema is passed, parse the schema
	// 7. PrepareColumnBindings() - based on the meta data and the cursorschema desired create Value buffers for the columns
	// 8. BindColumns() - bind all columns that are fixed width to the Value buffers
	// 9. SQLFetchToCursor/SQLFetchToVariables - loop over SQLFetch() until no more data is returned
	// 10 store number of fetched / updated / deleted / inserted rows into array
	// 11. SQLMoreResults - check if more result sets are available - if so move to step 5
	// 12. cleanup

	SqlStatement* pStmt = 0;
	bool prepared = parm.PCount() == 1;
	bool bFetchAborted = false;

	try
	{
		VFP2CTls& tls = VFP2CTls::Tls();
		tls.ErrorCount = 0;
		SQLRETURN nApiRet;
		SQLSMALLINT nNoOfCols;
		SqlResultSet* pResultSet = 0;
		ValueEx vRowCount;
		ValueEx vNoCountStatus;
		int nErrorNo;
		vRowCount.SetDouble(0, 0);
		vNoCountStatus.SetLogical();

		if (prepared)
		{
			if (parm(1)->Numeric())
				pStmt = parm(1)->DynamicPtr<SqlStatement*>();
			else
			{
				SaveCustomErrorEx("SqlExecEx", "Invalid parameter 'nHandle', should be of type I or N", E_VFP_INVALIDPARAMS);
				throw E_APIERROR;
			}

			pStmt->Reset();

			// if statement contained parameters 
			if (pStmt->pParameters.GetCount())
			{
				if ((pStmt->nFlags & SQLEXECEX_PARAMETER_ARRAY) == 0)
				{
					// 2.5 - evaluate parameters
					pStmt->EvaluateParameters();

					// 3 - bind parameters in ODBC driver
					pStmt->BindParameters();
				}
				else
				{
					pStmt->ParseParameterArraySchema();
					pStmt->BindParameterArray();
				}
			}
		}
		else
		{
			// 1
			pStmt = SQLAllocStatement(parm, false, tls);

			bool bParameters = false;
			// 2 - check statement for parameters
			if (!(pStmt->nFlags & SQLEXECEX_NATIVE_SQL))
				bParameters = pStmt->ParseParameters();

			// if statement contained parameters parse them out
			if (bParameters)
			{
				pStmt->AllocateParameters();

				// 2.3
				pStmt->ExtractParametersAndRewriteStatement();

				// 2.4 - parse parameterschema
				pStmt->ParseParameterSchema();

				if ((pStmt->nFlags & SQLEXECEX_PARAMETER_ARRAY) == 0)
				{
					// 2.5 - evaluate parameters
					pStmt->EvaluateParameters();

					// 3 - bind parameters in ODBC driver
					pStmt->BindParameters();
				}
				else
				{
					pStmt->ParseParameterArraySchema();
					pStmt->BindParameterArray();
				}
			}
		}

		nApiRet = pStmt->Exec();

SQLParameterArrayResumePoint:
		// 4.1
		if (nApiRet == SQL_NEED_DATA)
		{
			if ((pStmt->nFlags & SQLEXECEX_PARAMETER_ARRAY) == 0)
				nApiRet = pStmt->SendParameters();
			else
				nApiRet = pStmt->SendParameterArray();
		}

		if (nApiRet == SQL_PARAM_DATA_AVAILABLE)
		{
			pStmt->StreamParameters();
			goto SQLResultSetChecking;
		}
		else if (nApiRet == SQL_SUCCESS_WITH_INFO)
		{
			if (pStmt->nFlags & SQLEXECEX_PARAMETER_ARRAY)
			{
				pStmt->SaveParameterArrayErrors();
				if (pStmt->nFlags & SQLEXECEX_PARAMETER_ARRAY_RESUME_ON_ERROR)
				{
					nApiRet = pStmt->ResumeOnParameterArrayError();
					if (nApiRet != SQL_NO_DATA)
						goto SQLParameterArrayResumePoint;
				}
				goto SQLResultSetProcessing;
			}
			else if (!pStmt->InfoCallbackOrStore() && pStmt->bStoreInfoAsError)
				AddODBCError(prepared ? "SQLExecute" : "SQLExecDirect", pStmt->hStmt);
			goto SQLResultSetChecking;
		}
		else if (nApiRet == SQL_ERROR)
		{
			SaveODBCError(prepared ? "SQLExecute" : "SQLExecDirect", pStmt->hStmt);
			throw E_APIERROR;
		}

		// 5.
SQLResultSetProcessing:

		nNoOfCols = pStmt->GetColumnCount();
		if (nNoOfCols == 0)
		{
			// no resultset ..(UPDATE, DELETE, INSERT or other statement ...)
			pStmt->SaveRowCount();
			goto SQLResultSetChecking;
		}

		if (pStmt->nFlags & SQLEXECEX_PARAMETER_ARRAY_MERGE_OUTPUT)
		{
			pResultSet = pStmt->AddResultSet();
			pResultSet->nWorkArea = pStmt->pParameterArray.nWorkArea;
			pResultSet->AllocateColumns(nNoOfCols);
			pResultSet->GetColumnMetaData();
			pResultSet->PrepareColumnBindings();
			pResultSet->BindColumns();
			pResultSet->BindFieldLocators();

SQLParameterArrayMergeOutput:
			pResultSet->FetchOutputColumns(pStmt->pParameterArray);
			goto SQLResultSetChecking;
		}

		pResultSet = pStmt->AddResultSet();
		pResultSet->AllocateColumns(nNoOfCols);

		if (pStmt->nFlags & SQLEXECEX_CALLBACK_PROGRESS)
			pStmt->nRowsTotal = pStmt->GetRowCount();

		// 6
		pResultSet->GetColumnMetaData();

		if (pStmt->nFlags & SQLEXECEX_DEST_CURSOR)
		{
			pResultSet->SetCursorName();

			// 6.1
			// NOLINTNEXTLINE(bugprone-assignment-in-if-condition)
			if ((pStmt->nFlags & SQLEXECEX_REUSE_CURSOR) && (pResultSet->nWorkArea = Select(pResultSet->pCursorName)))
				pResultSet->ParseExistingCursorSchema();
			else
				pResultSet->ParseCursorSchema();
				
			// 7
			pResultSet->PrepareColumnBindings();

			// 8
			pResultSet->BindColumns();

			if (pResultSet->nWorkArea && (pStmt->nFlags & (SQLEXECEX_REUSE_CURSOR | SQLEXECEX_PARAMETER_ARRAY)))
			{
				if ((pStmt->nFlags & (SQLEXECEX_APPEND_CURSOR | SQLEXECEX_PARAMETER_ARRAY)) == 0)
				{
					AutoOnOffSetting pSetting("SAFETY", false);
					nErrorNo = Zap(pResultSet->nWorkArea);
					if (nErrorNo)
						throw nErrorNo;
				}
				pResultSet->pRecNoRestore.SaveRecNo(pResultSet->nWorkArea, (pStmt->nFlags & SQLEXECEX_PRESERVE_RECNO) > 0, true);
			}
			else
			{
				pResultSet->CreateCursor();
				pResultSet->pRecNoRestore.SaveRecNo(pResultSet->nWorkArea, (pStmt->nFlags& SQLEXECEX_PRESERVE_RECNO) > 0, true);
			}

			pResultSet->BindFieldLocators();

			// 9
			pResultSet->FetchToCursor(&bFetchAborted);
		}
		else // if (pStmt->nFlags & SQLEXECEX_DEST_VARIABLE)
		{
			pResultSet->BindVariableLocators();

			// 7
			pResultSet->PrepareColumnBindings();

			// 8
			pResultSet->BindColumns();

			// 9
			pResultSet->FetchToVariables();
		}

		if (pStmt->pInfoArray)
		{
			// TODO combine - rowcounts if SQLEXECEX_PARAMETER_ARRAY
			unsigned int nIndex = pStmt->pInfoArray.Grow();
			pStmt->pInfoArray(nIndex, 1) = (pStmt->nFlags & SQLEXECEX_DEST_CURSOR) ? pResultSet->pCursorName : pStmt->pGetDataBuffer.Len(0);
			vRowCount.ev_real = (double)pResultSet->nRowsFetched;
			pStmt->pInfoArray(nIndex, 2) = vRowCount;
			pStmt->pInfoArray(nIndex, 3) = false;
		}

		// callback if there is more info (from TSQL PRINT or RAISERROR statements)
		pStmt->InfoCallbackOrStore();

	// 10
	SQLResultSetChecking:

		nApiRet = SQLMoreResults(pStmt->hStmt);
		if (nApiRet == SQL_ERROR)
		{
			SaveODBCError("SQLMoreResult", pStmt->hStmt);
			throw E_APIERROR;
		}
		else if (nApiRet == SQL_SUCCESS_WITH_INFO)
		{
			if (pStmt->nFlags & SQLEXECEX_PARAMETER_ARRAY)
			{
				pStmt->SaveParameterArrayErrors();
				if (pStmt->nFlags & SQLEXECEX_PARAMETER_ARRAY_RESUME_ON_ERROR)
				{
					nApiRet = pStmt->ResumeOnParameterArrayError();
					if (nApiRet != SQL_NO_DATA)
						goto SQLParameterArrayResumePoint;
				}
			}
			else if (!pStmt->InfoCallbackOrStore() && pStmt->bStoreInfoAsError)
				AddODBCError("SQLMoreResult", pStmt->hStmt);
		}
		else if (nApiRet == SQL_PARAM_DATA_AVAILABLE)
		{
			pStmt->StreamParameters();
			nApiRet = SQL_NO_DATA;
		}

		if (nApiRet != SQL_NO_DATA)
		{
			if (pStmt->nFlags & SQLEXECEX_PARAMETER_ARRAY_MERGE_OUTPUT)
				goto SQLParameterArrayMergeOutput;

			pResultSet->SaveColumnBinding();
			if (!pStmt->bExecutedOnce)
				pStmt->UnbindColumns();

			// if the parameter processed pointer did change we set the flag for reexecution 
			if (pStmt->pParameterArray.nParamsProcessed != pStmt->pParameterArray.nParamsLastProcessed)
				pStmt->bExecutedOnce = true;

			goto SQLResultSetProcessing;
		}

		if ((pStmt->nFlags & SQLEXECEX_PARAMETER_ARRAY))
		{
			if (pStmt->LoadNextParameterArrayBatch())
			{
				nApiRet = SQLExecute(pStmt->hStmt);
				goto SQLParameterArrayResumePoint;
			}
		}

		// save output parameters
		pStmt->SaveOutputParameters();

		// we're finished .. clean up everything ...
		Return(tls.ErrorCount == 0 ? 1 : 2);
		if (prepared)
		{
			pStmt->FreeParameters();
			pStmt->bExecutedOnce = true;
		}
		else
		{
			if (!pStmt->FreeHandle())
				throw E_APIERROR;
			delete pStmt;
		}
	}
	catch (int nErrorNo)
	{
		if (!prepared)
		{
			if (pStmt)
			{
				pStmt->FreeHandle();
				delete pStmt;
			}
		}
			
		if (nErrorNo != E_APIERROR)
			RaiseError(nErrorNo);
		else if (!bFetchAborted)
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
	bool bParameters = false;

	try
	{
		VFP2CTls& tls = VFP2CTls::Tls();
		// 1
		pStmt = SQLAllocStatement(parm, true, tls);

		// 2 - check statement for parameters
		if (!(pStmt->nFlags & SQLEXECEX_NATIVE_SQL))
			bParameters = pStmt->ParseParameters();

		// if statement contained parameters parse them out
		if (bParameters)
		{
			pStmt->AllocateParameters();

			pStmt->ExtractParametersAndRewriteStatement();
			pSqlCommand = pStmt->pSQLSend.Ptr<SQLCHAR*>();
			nSqlLen = (SQLINTEGER)pStmt->pSQLSend.Len();

			// 2.4 - parse parameterschema
			pStmt->ParseParameterSchema();
		}
		else // no parameters in SQL statement .. just send it as it is ..
		{
			pSqlCommand = pStmt->pSQLInput.Ptr<SQLCHAR*>();
			nSqlLen = (SQLINTEGER)pStmt->pSQLInput.Len();
		}

		// 4.
		nApiRet = SQLPrepare(pStmt->hStmt, pSqlCommand, nSqlLen);
		if (nApiRet == SQL_ERROR)
		{
			SaveODBCError("SQLPrepare", pStmt->hStmt);
			throw E_APIERROR;
		}
		else if (nApiRet == SQL_SUCCESS_WITH_INFO && pStmt->bStoreInfoAsError)
			SaveODBCError("SQLPrepare", pStmt->hStmt);

		// we're finished .. clean up everything ...
		Return(pStmt);
	}
	catch (int nErrorNo)
	{
		if (pStmt)
		{
			pStmt->FreeHandle();
			delete pStmt;
		}

		if (nErrorNo != E_APIERROR)
			RaiseError(nErrorNo);
		else
			Return(-1);
	}
}

void _fastcall SQLCancelEx(ParamBlkEx& parm)
{
	SqlStatement* pStmt = parm(1)->Ptr<SqlStatement*>();
	try
	{
		if (pStmt)
		{
			pStmt->FreeHandle();
			delete pStmt;
		}
		else
		{
			SaveCustomError("SQLCancelEx", "Invalid statement handle passed.");
			throw E_APIERROR;
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
SqlStatement* _fastcall SQLAllocStatement(ParamBlkEx& parm, bool prepared, VFP2CTls& tls) throw(int)
{
	SqlStatement* pStmt = 0;
try
{
	SQLRETURN nApiRet;
	ValueEx vConHandle;
	ValueEx vSetting;
	SQLSMALLINT nLen;
	CStrBuilder<VFP2C_MAX_FUNCTIONBUFFER> pBuffer;
	CStrBuilder<VFP2C_MAX_FUNCTIONBUFFER> pBuffer2;
	vConHandle.SetNull();
	vSetting.SetNull();
	
	pStmt = new(std::nothrow) SqlStatement();
	if (!pStmt)
		throw E_INSUFMEMORY;

	pStmt->bPrepared = prepared;
	pStmt->bUnicodeConversion = tls.SqlUnicodeConversion;
	pStmt->bStoreInfoAsError = tls.SqlStoreInfoAsError;
	pStmt->bMapBinaryToGeneral = tls.SqlMapBinaryToGeneral;

	if (parm.PCount() >= 5 && parm(5)->ev_long)
	{
		pStmt->nFlags = parm(5)->ev_long;
		if (pStmt->nFlags & SQLEXECEX_APPEND_CURSOR)
			pStmt->nFlags |= SQLEXECEX_REUSE_CURSOR;
		
		if (pStmt->nFlags & SQLEXECEX_REUSE_CURSOR)
			pStmt->nFlags |= SQLEXECEX_DEST_CURSOR;
		
		if ((pStmt->nFlags & (SQLEXECEX_DEST_CURSOR | SQLEXECEX_DEST_VARIABLE)) == 0)
			pStmt->nFlags |= SQLEXECEX_DEST_CURSOR;
	}
	else
		pStmt->nFlags = SQLEXECEX_DEST_CURSOR;

	if ((pStmt->nFlags & SQLEXECEX_DEST_VARIABLE) && (pStmt->nFlags & (SQLEXECEX_DEST_CURSOR | SQLEXECEX_REUSE_CURSOR | SQLEXECEX_APPEND_CURSOR | SQLEXECEX_PRESERVE_RECNO)))
	{
		SaveCustomErrorEx("SqlExecEx", "Invalid nFlags parameter, SQLEXECEX_DEST_VARIABLE in combination with SQLEXECEX_DEST_CURSOR, SQLEXECEX_REUSE_CURSOR, SQLEXECEX_APPEND_CURSOR or SQLEXECEX_PRESERVE_RECNO.", E_VFP_INVALIDPARAMS);
		throw E_APIERROR;
	}

	if ((pStmt->nFlags & SQLEXECEX_PRESERVE_RECNO) && (pStmt->nFlags & SQLEXECEX_APPEND_CURSOR) == 0)
	{
		SaveCustomErrorEx("SqlExecEx", "Invalid nFlags parameter, SQLEXECEX_PRESERVE_RECNO requires SQLEXECEX_APPEND_CURSOR.", E_VFP_INVALIDPARAMS);
		throw E_APIERROR;
	}

	if ((pStmt->nFlags & (SQLEXECEX_CALLBACK_INFO | SQLEXECEX_STORE_INFO)) == (SQLEXECEX_CALLBACK_INFO | SQLEXECEX_STORE_INFO))
	{
		SaveCustomErrorEx("SqlExecEx", "Invalid nFlags parameter, SQLEXECEX_CALLBACK_INFO and SQLEXECEX_STORE_INFO or mutually exclusive.", E_VFP_INVALIDPARAMS);
		throw E_APIERROR;
	}

	pStmt->pSQLInput.Attach(parm(2));

	if (parm.PCount() >= 3 && parm(3)->Len())
	{
		pStmt->pCursorNames.Attach(parm(3));
		if (prepared)
			pStmt->pCursorNames.DetachParameter();
	}

	if (parm.PCount() >= 4 && parm(4)->Len())
	{
		pStmt->pInfoArray.Dimension(parm(4), 1, 3);
	}
	else if (pStmt->nFlags & SQLEXECEX_STORE_INFO)
	{
		SaveCustomErrorEx("SqlExecEx", "SQLEXECEX_STORE_INFO requires the passing of an array name in parameter cArray.", E_VFP_INVALIDPARAMS);
		throw E_APIERROR;
	}

	if (parm.PCount() >= 6 && parm(6)->Len())
	{
		pStmt->pCursorSchema.Attach(parm(6));
		if (prepared)
			pStmt->pCursorSchema.DetachParameter();
	}

	if (parm.PCount() >= 7 && parm(7)->Len())
	{
		pStmt->pParamSchema.Attach(parm(7));
		if (prepared)
			pStmt->pParamSchema.DetachParameter();
	}

	if (parm.PCount() >= 8 && parm(8)->Len())
	{
		if ((pStmt->nFlags & (SQLEXECEX_CALLBACK_INFO | SQLEXECEX_CALLBACK_PROGRESS)) == 0)
			pStmt->nFlags |= (SQLEXECEX_CALLBACK_INFO | SQLEXECEX_CALLBACK_PROGRESS);

		if (parm(8)->Len() > VFP2C_MAX_CALLBACKFUNCTION)
		{
			SaveCustomErrorEx("SqlExecEx", "Callback function length is greater than maximum length of 1024.", E_VFP_INVALIDPARAMS);
			throw E_APIERROR;
		}
		FoxString pCallback(parm(8));
		pStmt->pCallback.SetCallback(pCallback);
	}
	else if (pStmt->nFlags & (SQLEXECEX_CALLBACK_INFO | SQLEXECEX_CALLBACK_PROGRESS))
	{
		SaveCustomErrorEx("SqlExecEx", "SQLEXECEX_CALLBACK_INFO or SQLEXECEX_CALLBACK_PROGRESS requires the passing of a callback function.", E_VFP_INVALIDPARAMS);
		throw E_APIERROR;
	}

	if (parm.PCount() >= 9 && parm(9)->ev_long)
		pStmt->nCallbackInterval = parm(9)->ev_long;

	if (parm.PCount() >= 10 && parm(10)->Len())
	{
		if (pStmt->nFlags & SQLEXECEX_PARAMETER_ARRAY)
		{
			pStmt->pParameterArray.pParamStatusDestination.Attach(parm(10));
			if (prepared)
				pStmt->pParameterArray.pParamStatusDestination.DetachParameter();
		}
		else
		{
			SaveCustomErrorEx("SqlExecEx", "Status cursor for SQLEXECEX_PARAMETER_ARRAY provided but the flag SQLEXECEX_PARAMETER_ARRAY was not passed.", E_VFP_INVALIDPARAMS);
			throw E_APIERROR;
		}
	}

	if (parm.PCount() >= 11)
		pStmt->pParameterArray.nBatchSize = parm(11)->ev_long;

	if (parm.PCount() >= 12)
	{
		if (parm(12)->ev_long > 0)
			pStmt->nRowSetSize = parm(12)->ev_long;
		else
		{
			SaveCustomErrorEx("SqlExecEx", "Rowsetsize parameter has to be greater 0.", E_VFP_INVALIDPARAMS);
			throw E_APIERROR;
		}
	}

	// 1.1 - build command to evaluate connection handle
	pBuffer.Format("SQLGETPROP(%U,'ODBChdbc')", parm(1)->ev_long);

	// 1.2 - evaluate connection handle
	Evaluate(vConHandle, pBuffer);
	pStmt->hConn = vConHandle.DynamicPtr<SQLHDBC>();
	if (pStmt->hConn == 0)
	{
		// the connection is currently "idle" disconnected, force a reconnection by calling SQLCOMMIT, besides from reestablishing the connection SQLCOMMIT is basically a noop when executed on an "idle" connection
		// disable 'DispLogin' setting before if neccessary to supress the connection dialog on reconnection failure
		pBuffer2.Format("SQLGETPROP(%U, 'DispLogin')", parm(1)->ev_long);
		Evaluate(vSetting, pBuffer2);
		if (vSetting.ev_long != 3)
		{
			pBuffer2.Format("SQLSETPROP(%U, 'DispLogin', 3)", parm(1)->ev_long);
			Execute(pBuffer2);
		}

		pBuffer2.Format("SQLCOMMIT(%U)", parm(1)->ev_long);
		Execute(pBuffer2);

		if (vSetting.ev_long != 3)
		{
			pBuffer2.Format("SQLSETPROP(%U, 'DispLogin', %U)", parm(1)->ev_long, vSetting.ev_long);
			Execute(pBuffer2);
		}

		Evaluate(vConHandle, pBuffer);
		pStmt->hConn = vConHandle.DynamicPtr<SQLHDBC>();
		if (pStmt->hConn == 0)
		{
			SaveCustomError("SQLAllocStatement", "ODBC connection was idle disconnected and the reconnection attempt failed.");
			throw E_APIERROR;
		}
		vSetting.SetNull();
	}

	if (CFoxVersion::MajorVersion() >= 9) 
	{
		Evaluate(vSetting, "CURSORGETPROP('UseMemoSize',0)+IIF(CURSORGETPROP('MapVarchar',0),256,0)+IIF(CURSORGETPROP('MapBinary',0),512,0)");
		pStmt->nUseMemoSize = (vSetting.ev_long & 255);
		pStmt->bMapVarchar = (vSetting.ev_long & 256) > 0;
		pStmt->bMapVarbinary = (vSetting.ev_long & 512) > 0;
	}
	else
	{
		Evaluate(vSetting, "CURSORGETPROP('UseMemoSize',0)");
		pStmt->nUseMemoSize = vSetting.ev_long;
	}

	nApiRet = SQLGetInfo(pStmt->hConn, SQL_DBMS_NAME, pBuffer.Ptr<SQLPOINTER>(), (SQLSMALLINT)pBuffer.Size(), &nLen);
	if (nApiRet == SQL_ERROR)
	{
		SaveODBCError("SQLGetInfo - SQL_DBMS_NAME", pStmt->hConn);
		throw E_APIERROR;
	}
	else if (nApiRet == SQL_SUCCESS_WITH_INFO && pStmt->bStoreInfoAsError)
		AddODBCError("SQLGetInfo - SQL_DBMS_NAME", pStmt->hConn);
	else
	{
		pBuffer.Len(nLen > 6 ? 6 : nLen, true);
		CStringView pDBName = pBuffer;
		if (pDBName.ICompare(__sv("MICROS")))
			pStmt->ServerType = SQLServer;
		else if (pDBName.ICompare(__sv("ORACLE")))
			pStmt->ServerType = Oracle;
		else if (pDBName.ICompare(__sv("MYSQL")))
			pStmt->ServerType = MySql;
		else if (pDBName.ICompare(__sv("MARIAD")))
			pStmt->ServerType = MariaDB;
		else if (pDBName.ICompare(__sv("POSTGR")))
			pStmt->ServerType = PostgreSQL;
		else if (pDBName.ICompare(__sv("INFORM")))
			pStmt->ServerType = Informix;
		else if (pDBName.ICompare(__sv("FIREBI")))
			pStmt->ServerType = Firebird;
		else if (pDBName.ICompare(__sv("ACCESS")))
			pStmt->ServerType = Access;
	}

	// 1.3 - allocate statement handle on connection
	nApiRet = SQLAllocHandle(SQL_HANDLE_STMT,pStmt->hConn, pStmt->hStmt);
	if (nApiRet == SQL_ERROR)
	{
		SaveODBCError("SQLAllocHandle", pStmt->hConn);
		throw E_APIERROR;
	}
	else if (nApiRet == SQL_SUCCESS_WITH_INFO && pStmt->bStoreInfoAsError)
		AddODBCError("SQLAllocHandle", pStmt->hConn);

	return pStmt;
}
catch (int nErrorNo)
{
	if (pStmt)
	{
		pStmt->FreeHandle();
		delete pStmt;
	}
	throw nErrorNo;
}
}
#pragma warning(default : 4290)

unsigned int _fastcall FixCursorColumnName(CStringView pColumnName)
{
	if (!pColumnName)
		return 0;

	unsigned int nMaxLen = VFP2C_VFP_MAX_COLUMN_NAME;
	unsigned int nLen;
	char* pCol = pColumnName.Data;
	if (pColumnName.Len > nMaxLen)
	{
		nLen = nMaxLen;
	}
	else
	{
		nLen = pColumnName.Len;
		nMaxLen = pColumnName.Len;
	}

	if (!IsCharacter(*pCol) && *pCol != '_')
	{
		*pCol++ = '_';
		nMaxLen--;
	}

	while (nMaxLen--)
	{
		if (IsCharacter(*pCol) || IsDigit(*pCol) || *pCol == '_')
			pCol++;
		else
			*pCol++ = '_';
	}
	return nLen;
}

BOOL _fastcall SQLTypeConvertible(SQLSMALLINT nSQLType, SQLCHAR aVFPType)
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
	case SQL_TYPE_DATE:
	case SQL_TYPE_TIMESTAMP:
		return aVFPType == 'D' || aVFPType == 'T' || aVFPType == 'C' ||
			aVFPType == 'V' || aVFPType == 'Q';

	case SQL_INTERVAL:
	case SQL_TYPE_TIME:
		return aVFPType == 'C' || aVFPType == 'V' || aVFPType == 'Q';

	case SQL_BIT:
		return aVFPType == 'C' || aVFPType == 'V' || aVFPType == 'L' || aVFPType == 'I';

	case SQL_GUID:
	case SQL_SS_TIME2:
	case SQL_SS_TIMESTAMPOFFSET:
		return aVFPType == 'C' || aVFPType == 'Q';

	case SQL_SS_VARIANT:
	case SQL_SS_UDT:
		return aVFPType == 'C' || aVFPType == 'Q' || aVFPType == 'M' || aVFPType == 'W';

	case SQL_SS_XML:
		return aVFPType == 'M' || aVFPType == 'W';

	default:
		return TRUE;
	}
}

int _fastcall SQLStoreByBinding(SqlColumn* lpSC)
{
	if (lpSC->nIndicator == SQL_NULL_DATA)
	{
		Value vNull = { '0' };
		return _DBReplace(lpSC->lField, &vNull);
	}
	else
		return _DBReplace(lpSC->lField, lpSC->vData);
}

int _fastcall SQLStoreByBindingVar(SqlColumn* lpSC)
{
	if (lpSC->nIndicator == SQL_NULL_DATA)
	{
		Value vNull = { '0' };
		return _Store(lpSC->lField, &vNull);
	}
	else
		return _Store(lpSC->lField, lpSC->vData);
}

int _fastcall SQLStoreIntByBindingSet(SqlColumn* lpSC, int nRow)
{
	SQLLEN nIndicator = ((SQLLEN*)lpSC->nIndicator)[nRow];
	if (nIndicator == SQL_NULL_DATA)
	{
		Value vNull = { '0' };
		return _DBReplace(lpSC->lField, &vNull);
	}
	else
	{
		int* pData = IndexPtr<int*>(lpSC->pDataPtr, nRow);
		lpSC->vData.ev_long = *pData;
		return _DBReplace(lpSC->lField, lpSC->vData);
	}
}

int _fastcall SQLStoreDoubleByBindingSet(SqlColumn* lpSC, int nRow)
{
	SQLLEN nIndicator = ((SQLLEN*)lpSC->nIndicator)[nRow];
	if (nIndicator == SQL_NULL_DATA)
	{
		Value vNull = { '0' };
		return _DBReplace(lpSC->lField, &vNull);
	}
	else
	{
		double* pData = IndexPtr<double*>(lpSC->pDataPtr, nRow);
		lpSC->vData.ev_real = *pData;
		return _DBReplace(lpSC->lField, lpSC->vData);
	}
}

int _fastcall SQLStoreByGetData(SqlColumn* lpSC)
{
	SQLRETURN nApiRet = SQLGetData(lpSC->pStmt->hStmt,lpSC->nColNo,lpSC->nCType,lpSC->pDataPtr,lpSC->nBufferSize,&lpSC->nIndicator);
	if (SQL_SUCCEEDED(nApiRet))
	{
		if (nApiRet == SQL_SUCCESS_WITH_INFO && lpSC->bStoreInfoAsError)
		{
			AddODBCError("SQLGetData", lpSC->pStmt->hStmt);
			lpSC->bStoreInfoAsError = false;
		}

		if (lpSC->nIndicator == SQL_NULL_DATA)
		{
			Value vNull = { '0' };
			return _DBReplace(lpSC->lField, &vNull);
		}
		else
			return _DBReplace(lpSC->lField, lpSC->vData);
	}
	SaveODBCError("SQLGetData", lpSC->pStmt->hStmt);
	return E_APIERROR;
}

int _fastcall SQLStoreByGetDataVar(SqlColumn* lpSC)
{
	SQLRETURN nApiRet = SQLGetData(lpSC->pStmt->hStmt,lpSC->nColNo,lpSC->nCType,lpSC->pDataPtr,lpSC->nBufferSize,&lpSC->nIndicator);
	if (SQL_SUCCEEDED(nApiRet))
	{
		if (nApiRet == SQL_SUCCESS_WITH_INFO && lpSC->bStoreInfoAsError)
		{
			AddODBCError("SQLGetData", lpSC->pStmt->hStmt);
			lpSC->bStoreInfoAsError = false;
		}

		if (lpSC->nIndicator == SQL_NULL_DATA)
		{
			Value vNull = { '0' };
			return _Store(lpSC->lField, &vNull);
		}
		else
			return _Store(lpSC->lField, lpSC->vData);
	}
	SaveODBCError("SQLGetData", lpSC->pStmt->hStmt);
	return E_APIERROR;
}

int _fastcall SQLStoreCharByBinding(SqlColumn* lpSC)
{
	if (lpSC->nIndicator == SQL_NULL_DATA)
	{
		Value vNull = { '0' };
		return _DBReplace(lpSC->lField, &vNull);
	}
	else
	{
		lpSC->vData.ev_length = lpSC->nIndicator != SQL_NO_TOTAL ? lpSC->nIndicator : lpSC->nSize;
		return _DBReplace(lpSC->lField, lpSC->vData);
	}
}

int _fastcall SQLStoreCharByBindingSet(SqlColumn* lpSC, int nRow)
{
	SQLLEN nIndicator = ((SQLLEN*)lpSC->nIndicator)[nRow];
	if (nIndicator == SQL_NULL_DATA)
	{
		Value vNull = { '0' };
		return _DBReplace(lpSC->lField, &vNull);
	}
	else
	{
		FoxString& pValue = lpSC->pStmt->pGetDataBuffer;
		SQLPOINTER pData = AdvancePtr<SQLPOINTER>(lpSC->pDataPtr, nRow * lpSC->nBufferSize);
		SQLLEN nSize = nIndicator != SQL_NO_TOTAL ? nIndicator : (SQLINTEGER)lpSC->nSize;
		memcpy(pValue.Ptr<char*>(), pData, nSize);
		pValue.Len(nSize);
		return _DBReplace(lpSC->lField, pValue);
	}
}

int _fastcall SQLStoreCharByBindingVar(SqlColumn* lpSC)
{
	if (lpSC->nIndicator == SQL_NULL_DATA)
	{
		Value vNull = { '0' };
		return _Store(lpSC->lField, &vNull);
	}
	else
	{
		lpSC->vData.ev_length = lpSC->nIndicator != SQL_NO_TOTAL ? lpSC->nIndicator : lpSC->nSize;
		return _Store(lpSC->lField, lpSC->vData);
	}
}

int _fastcall SQLStoreCharByGetData(SqlColumn* lpSC)
{
	SQLRETURN nApiRet = SQLGetData(lpSC->pStmt->hStmt,lpSC->nColNo,lpSC->nCType, lpSC->pStmt->pGetDataBuffer.Ptr<SQLPOINTER>(),
		(SQLINTEGER)lpSC->pStmt->pGetDataBuffer.Size(), &lpSC->nIndicator);
	if (SQL_SUCCEEDED(nApiRet))
	{
		if (nApiRet == SQL_SUCCESS_WITH_INFO && lpSC->bStoreInfoAsError)
		{
			AddODBCError("SQLGetData", lpSC->pStmt->hStmt);
			lpSC->bStoreInfoAsError = false;
		}

		if (lpSC->nIndicator == SQL_NULL_DATA)
		{
			Value vNull = { '0' };
			return _DBReplace(lpSC->lField, &vNull);
		}
		else
		{
			lpSC->pStmt->pGetDataBuffer.Len(lpSC->nIndicator != SQL_NO_TOTAL ? lpSC->nIndicator : lpSC->nSize);
			return _DBReplace(lpSC->lField, lpSC->pStmt->pGetDataBuffer);
		}
	 }
	SaveODBCError("SQLGetData", lpSC->pStmt->hStmt);
	return E_APIERROR;
}

int _fastcall SQLStoreCharByGetDataVar(SqlColumn* lpSC)
{
	SQLRETURN nApiRet = SQLGetData(lpSC->pStmt->hStmt,lpSC->nColNo,lpSC->nCType, lpSC->pStmt->pGetDataBuffer.Ptr<SQLPOINTER>(),
		(SQLINTEGER)lpSC->pStmt->pGetDataBuffer.Size(), &lpSC->nIndicator);
	if (SQL_SUCCEEDED(nApiRet))
	{
		if (nApiRet == SQL_SUCCESS_WITH_INFO && lpSC->bStoreInfoAsError)
		{
			AddODBCError("SQLGetData", lpSC->pStmt->hStmt);
			lpSC->bStoreInfoAsError = false;
		}

		if (lpSC->nIndicator == SQL_NULL_DATA)
		{
			Value vNull = { '0' };
			return _Store(lpSC->lField, &vNull);
		}
		else
		{
			lpSC->pStmt->pGetDataBuffer.Len(lpSC->nIndicator != SQL_NO_TOTAL ? lpSC->nIndicator : lpSC->nSize);
			return _Store(lpSC->lField, lpSC->pStmt->pGetDataBuffer);
		}
	}
	SaveODBCError("SQLGetData", lpSC->pStmt->hStmt);
	return E_APIERROR;
}

int _fastcall SQLStoreDateByBinding(SqlColumn* lpSC)
{
	if (lpSC->nIndicator == SQL_NULL_DATA)
	{
		Value vNull = { '0' };
		return _DBReplace(lpSC->lField, &vNull);
	}
	else
	{
		Date_StructToValue(&lpSC->sDateStruct, lpSC->vData);
		return _DBReplace(lpSC->lField, lpSC->vData);
	}
}

int _fastcall SQLStoreDateByBindingVar(SqlColumn* lpSC)
{
	if (lpSC->nIndicator == SQL_NULL_DATA)
	{
		Value vNull = { '0' };
		return _Store(lpSC->lField, &vNull);
	}
	else
	{
		Date_StructToValue(&lpSC->sDateStruct, lpSC->vData);
		return _Store(lpSC->lField, lpSC->vData);
	}
}

int _fastcall SQLStoreDateByGetData(SqlColumn* lpSC)
{
	SQLRETURN nApiRet = SQLGetData(lpSC->pStmt->hStmt,lpSC->nColNo,lpSC->nCType,lpSC->pDataPtr,lpSC->nBufferSize,&lpSC->nIndicator);
	if (SQL_SUCCEEDED(nApiRet))
	{
		if (nApiRet == SQL_SUCCESS_WITH_INFO && lpSC->bStoreInfoAsError)
		{
			AddODBCError("SQLGetData", lpSC->pStmt->hStmt);
			lpSC->bStoreInfoAsError = false;
		}

		if (lpSC->nIndicator == SQL_NULL_DATA)
		{
			Value vNull = { '0' };
			return _DBReplace(lpSC->lField, &vNull);
		}
		else
		{
			Date_StructToValue(&lpSC->sDateStruct, lpSC->vData);
			return _DBReplace(lpSC->lField, lpSC->vData);
		}
	}
	SaveODBCError("SQLGetData", lpSC->pStmt->hStmt);
	return E_APIERROR;
}

int _fastcall SQLStoreDateByGetDataVar(SqlColumn* lpSC)
{
	SQLRETURN nApiRet = SQLGetData(lpSC->pStmt->hStmt,lpSC->nColNo,lpSC->nCType,lpSC->pDataPtr,lpSC->nBufferSize,&lpSC->nIndicator);
	if (SQL_SUCCEEDED(nApiRet))
	{
		if (nApiRet == SQL_SUCCESS_WITH_INFO && lpSC->bStoreInfoAsError)
		{
			AddODBCError("SQLGetData", lpSC->pStmt->hStmt);
			lpSC->bStoreInfoAsError = false;
		}

		if (lpSC->nIndicator == SQL_NULL_DATA)
		{
			Value vNull = { '0' };
			return _Store(lpSC->lField, &vNull);
		}
		else
		{
			Date_StructToValue(&lpSC->sDateStruct, lpSC->vData);
			return _Store(lpSC->lField, lpSC->vData);
		}
	}
	SaveODBCError("SQLGetData", lpSC->pStmt->hStmt);
	return E_APIERROR;
}

int _fastcall SQLStoreDateByBindingSet(SqlColumn* lpSC, int nRow)
{
	SQLLEN nIndicator = ((SQLLEN*)lpSC->nIndicator)[nRow];
	if (nIndicator == SQL_NULL_DATA)
	{
		Value vNull = { '0' };
		return _DBReplace(lpSC->lField, &vNull);
	}
	else
	{
		SQL_DATE_STRUCT *pDate = IndexPtr<SQL_DATE_STRUCT*>(lpSC->pDataPtr, nRow);
		Date_StructToValue(pDate, lpSC->vData);
		return _DBReplace(lpSC->lField, lpSC->vData);
	}
}

int _fastcall SQLStoreDateTimeByBinding(SqlColumn* lpSC)
{
	if (lpSC->nIndicator == SQL_NULL_DATA)
	{
		Value vNull = { '0' };
		return _DBReplace(lpSC->lField, &vNull);
	}
	else
	{
		Timestamp_StructToValue(&lpSC->sTimestampStruct, lpSC->vData);
		return _DBReplace(lpSC->lField, lpSC->vData);
	}
}

int _fastcall SQLStoreDateTimeByBindingVar(SqlColumn* lpSC)
{
	if (lpSC->nIndicator == SQL_NULL_DATA)
	{
		Value vNull = { '0' };
		return _Store(lpSC->lField, &vNull);
	}
	else
	{
		Timestamp_StructToValue(&lpSC->sTimestampStruct, lpSC->vData);
		return _Store(lpSC->lField, lpSC->vData);
	}
}

int _fastcall SQLStoreDateTimeByGetData(SqlColumn* lpSC)
{
	SQLRETURN nApiRet = SQLGetData(lpSC->pStmt->hStmt,lpSC->nColNo,lpSC->nCType,lpSC->pDataPtr,lpSC->nBufferSize,&lpSC->nIndicator);
	if (SQL_SUCCEEDED(nApiRet))
	{
		if (nApiRet == SQL_SUCCESS_WITH_INFO && lpSC->bStoreInfoAsError)
		{
			AddODBCError("SQLGetData", lpSC->pStmt->hStmt);
			lpSC->bStoreInfoAsError = false;
		}

		if (lpSC->nIndicator == SQL_NULL_DATA)
		{
			Value vNull = { '0' };
			return _DBReplace(lpSC->lField, &vNull);
		}
		else
		{
			Timestamp_StructToValue(&lpSC->sTimestampStruct, lpSC->vData);
			return _DBReplace(lpSC->lField, lpSC->vData);
		}
	}
	SaveODBCError("SQLGetData", lpSC->pStmt->hStmt);
	return E_APIERROR;
}

int _fastcall SQLStoreDateTimeByGetDataVar(SqlColumn* lpSC)
{
	SQLRETURN nApiRet = SQLGetData(lpSC->pStmt->hStmt,lpSC->nColNo,lpSC->nCType,lpSC->pDataPtr,lpSC->nBufferSize,&lpSC->nIndicator);
	if (SQL_SUCCEEDED(nApiRet))
	{
		if (nApiRet == SQL_SUCCESS_WITH_INFO && lpSC->bStoreInfoAsError)
		{
			AddODBCError("SQLGetData", lpSC->pStmt->hStmt);
			lpSC->bStoreInfoAsError = false;
		}

		if (lpSC->nIndicator == SQL_NULL_DATA)
		{
			Value vNull = { '0' };
			return _Store(lpSC->lField, &vNull);
		}
		else
		{
			Timestamp_StructToValue(&lpSC->sTimestampStruct, lpSC->vData);
			return _Store(lpSC->lField, lpSC->vData);
		}
	}
	SaveODBCError("SQLGetData", lpSC->pStmt->hStmt);
	return E_APIERROR;
}

int _fastcall SQLStoreDateTimeByBindingSet(SqlColumn* lpSC, int nRow)
{
	SQLLEN nIndicator = ((SQLLEN*)lpSC->nIndicator)[nRow];
	if (nIndicator == SQL_NULL_DATA)
	{
		Value vNull = { '0' };
		return _DBReplace(lpSC->lField, &vNull);
	}
	else
	{
		SQL_TIMESTAMP_STRUCT* pDate = IndexPtr<SQL_TIMESTAMP_STRUCT*>(lpSC->pDataPtr, nRow);
		Timestamp_StructToValue(pDate, lpSC->vData);
		return _DBReplace(lpSC->lField, lpSC->vData);
	}
}

int _fastcall SQLStoreCurrencyByBinding(SqlColumn* lpSC)
{
	if (lpSC->nIndicator == SQL_NULL_DATA)
	{
		Value vNull = { '0' };
		return _DBReplace(lpSC->lField, &vNull);
	}
	else
	{
		Numeric_StructToValue(&lpSC->sNumericStruct, lpSC->vData);
		return _DBReplace(lpSC->lField, lpSC->vData);
	}
}

int _fastcall SQLStoreCurrencyByBindingVar(SqlColumn* lpSC)
{
	if (lpSC->nIndicator == SQL_NULL_DATA)
	{
		Value vNull = { '0' };
		return _Store(lpSC->lField, &vNull);
	}
	else
	{
		Numeric_StructToValue(&lpSC->sNumericStruct, lpSC->vData);
		return _Store(lpSC->lField, lpSC->vData);
	}
}

int _fastcall SQLStoreCurrencyByGetData(SqlColumn* lpSC)
{
	SQLRETURN nApiRet = SQLGetData(lpSC->pStmt->hStmt, lpSC->nColNo, SQL_ARD_TYPE, &lpSC->sNumericStruct, lpSC->nBufferSize, &lpSC->nIndicator);
	if (SQL_SUCCEEDED(nApiRet))
	{
		if (nApiRet == SQL_SUCCESS_WITH_INFO && lpSC->bStoreInfoAsError)
		{
			AddODBCError("SQLGetData", lpSC->pStmt->hStmt);
			lpSC->bStoreInfoAsError = false;
		}

		if (lpSC->nIndicator == SQL_NULL_DATA)
		{
			Value vNull = { '0' };
			return _DBReplace(lpSC->lField, &vNull);
		}
		else
		{
			Numeric_StructToValue(&lpSC->sNumericStruct, lpSC->vData);
			return _DBReplace(lpSC->lField, lpSC->vData);
		}
	}
	SaveODBCError("SQLGetData", lpSC->pStmt->hStmt);
	return E_APIERROR;
}

int _fastcall SQLStoreCurrencyByGetDataVar(SqlColumn* lpSC)
{
	SQLRETURN nApiRet = SQLGetData(lpSC->pStmt->hStmt, lpSC->nColNo, SQL_ARD_TYPE, &lpSC->sNumericStruct, lpSC->nBufferSize, &lpSC->nIndicator);
	if (SQL_SUCCEEDED(nApiRet))
	{
		if (nApiRet == SQL_SUCCESS_WITH_INFO && lpSC->bStoreInfoAsError)
		{
			AddODBCError("SQLGetData", lpSC->pStmt->hStmt);
			lpSC->bStoreInfoAsError = false;
		}
			
		if (lpSC->nIndicator == SQL_NULL_DATA)
		{
			Value vNull = { '0' };
			return _Store(lpSC->lField, &vNull);
		}
		else
		{
			Numeric_StructToValue(&lpSC->sNumericStruct, lpSC->vData);
			return _Store(lpSC->lField, lpSC->vData);
		}
	}
	SaveODBCError("SQLGetData", lpSC->pStmt->hStmt);
	return E_APIERROR;
}

int _fastcall SQLStoreCurrencyByBindingSet(SqlColumn* lpSC, int nRow)
{
	SQLLEN nIndicator = ((SQLLEN*)lpSC->nIndicator)[nRow];
	if (nIndicator == SQL_NULL_DATA)
	{
		Value vNull = { '0' };
		return _DBReplace(lpSC->lField, &vNull);
	}
	else
	{
		SQL_NUMERIC_STRUCT *pNum = IndexPtr<SQL_NUMERIC_STRUCT*>(lpSC->pDataPtr, nRow);
		Numeric_StructToValue(pNum, lpSC->vData);
		return _DBReplace(lpSC->lField, lpSC->vData);
	}
}

int _fastcall SQLStoreLogicalByBindingSet(SqlColumn* lpSC, int nRow)
{
	SQLLEN nIndicator = ((SQLLEN*)lpSC->nIndicator)[nRow];
	if (nIndicator == SQL_NULL_DATA)
	{
		Value vNull = { '0' };
		return _DBReplace(lpSC->lField, &vNull);
	}
	else
	{
		unsigned char* pLogical = IndexPtr<unsigned char*>(lpSC->pDataPtr, nRow);
		lpSC->vData.ev_length = *pLogical;
		return _DBReplace(lpSC->lField, lpSC->vData);
	}
}

int _fastcall SQLStoreMemoChar(SqlColumn* lpSC)
{
	SQLRETURN nApiRet;
	long nLoc;
	int nErrorNo;

	nApiRet = SQLGetData(lpSC->pStmt->hStmt, lpSC->nColNo, lpSC->nCType, lpSC->pStmt->pGetDataBuffer.Ptr<SQLPOINTER>(), 
		(SQLINTEGER)lpSC->pStmt->pGetDataBuffer.Size(), &lpSC->nIndicator);
	if (nApiRet == SQL_SUCCESS)
	{
		if (lpSC->nIndicator != SQL_NULL_DATA)
			return FoxMemo::Replace(lpSC->lField, lpSC->pStmt->pGetDataBuffer.Ptr<char*>(), (int)lpSC->nIndicator, lpSC->hMemoFile);
		else if (lpSC->bNullable)
		{
			Value vNull = { '0' };
			return _DBReplace(lpSC->lField, &vNull);
		}
	}
	else if (nApiRet == SQL_SUCCESS_WITH_INFO)
	{
		// result is greater than buffer, allocate space for the memo field, nIndicator contains length of data
		nErrorNo = FoxMemo::Alloc(lpSC->lField, (int)lpSC->nIndicator, &nLoc);
		if (nErrorNo)
			return nErrorNo;

		// append the data from the buffer
		nErrorNo = FoxMemo::Append(lpSC->pStmt->pGetDataBuffer.Ptr<char*>(), (int)(lpSC->pStmt->pGetDataBuffer.Size() - 1), lpSC->hMemoFile, &nLoc);
		if (nErrorNo)
			return nErrorNo;

		do
		{
			nApiRet = SQLGetData(lpSC->pStmt->hStmt, lpSC->nColNo, lpSC->nCType, lpSC->pStmt->pGetDataBuffer.Ptr<SQLPOINTER>(), 
				(SQLINTEGER)lpSC->pStmt->pGetDataBuffer.Size(), &lpSC->nIndicator);
			if (SQL_SUCCEEDED(nApiRet))
			{
				nErrorNo = FoxMemo::Append(lpSC->pStmt->pGetDataBuffer.Ptr<char*>(), (int)(lpSC->pStmt->pGetDataBuffer.Size() - 1), lpSC->hMemoFile, 0);
				if (nErrorNo)
					return nErrorNo;
			}
		} while (nApiRet == SQL_SUCCESS_WITH_INFO);

		if (nApiRet == SQL_ERROR)
		{
			SaveODBCError("SQLGetData", lpSC->pStmt->hStmt);
			return E_APIERROR;
		}
	}
	else
	{
		SaveODBCError("SQLGetData", lpSC->pStmt->hStmt);
		return E_APIERROR;
	}

	return 0;
}

int _fastcall SQLStoreMemoCharVar(SqlColumn* lpSC)
{
	SQLRETURN nApiRet = SQLGetData(lpSC->pStmt->hStmt, lpSC->nColNo, lpSC->nCType, lpSC->pStmt->pGetDataBuffer.Ptr<SQLPOINTER>(), 
		(SQLINTEGER)lpSC->pStmt->pGetDataBuffer.Size(), &lpSC->nIndicator);
	if (nApiRet == SQL_SUCCESS)
	{
		if (lpSC->nIndicator == SQL_NULL_DATA)
		{
			Value vNull = { '0' };
			return _Store(lpSC->lField, &vNull);
		}
		else
		{
			lpSC->pStmt->pGetDataBuffer.Len(lpSC->nIndicator);
			return _Store(lpSC->lField, lpSC->pStmt->pGetDataBuffer);
		}
	}
	else if (nApiRet == SQL_SUCCESS_WITH_INFO)
	{
		unsigned int nDataSize = lpSC->pStmt->pGetDataBuffer.Size() - 1;
		lpSC->pStmt->pGetDataBuffer.Size(lpSC->nIndicator + 1);
		lpSC->pStmt->pGetDataBuffer.Len(lpSC->nIndicator);
		nApiRet = SQLGetData(lpSC->pStmt->hStmt, lpSC->nColNo, lpSC->nCType, lpSC->pStmt->pGetDataBuffer.Ptr<char*>() + nDataSize, 
			(SQLINTEGER)(lpSC->pStmt->pGetDataBuffer.Size() - nDataSize), &lpSC->nIndicator);
		if (nApiRet == SQL_SUCCESS)
			return _Store(lpSC->lField, lpSC->pStmt->pGetDataBuffer);
		else
		{
			SaveODBCError("SQLGetData", lpSC->pStmt->hStmt);
			return E_APIERROR;
		}
	}
	else
	{
		SaveODBCError("SQLGetData", lpSC->pStmt->hStmt);
		return E_APIERROR;
	}
}

int _fastcall SQLStoreMemoCharByBindingSet(SqlColumn* lpSC, int nRow)
{
	SQLLEN nIndicator = ((SQLLEN*)lpSC->nIndicator)[nRow];
	if (nIndicator == SQL_NULL_DATA)
	{
		Value vNull = { '0' };
		return _DBReplace(lpSC->lField, &vNull);
	}
	else
	{
		int nSize = lpSC->nIndicator == SQL_NO_TOTAL ? (int)lpSC->nIndicator : (int)lpSC->nSize;
		memcpy(lpSC->pStmt->pGetDataBuffer.Ptr<char*>(), IndexPtr<char*>(lpSC->pDataPtr, nRow), nSize);
		return FoxMemo::Replace(lpSC->lField, lpSC->pStmt->pGetDataBuffer.Ptr<char*>(), nSize, lpSC->hMemoFile);
	}
}

int _fastcall SQLStoreMemoWChar(SqlColumn* lpSC)
{
	SQLRETURN nApiRet;
	long nLoc;
	int nErrorNo;

	nApiRet = SQLGetData(lpSC->pStmt->hStmt,lpSC->nColNo,lpSC->nCType, lpSC->pStmt->pGetDataBuffer.Ptr<SQLPOINTER>(), 
		(SQLINTEGER)lpSC->pStmt->pGetDataBuffer.Size(), &lpSC->nIndicator);
	if (nApiRet == SQL_SUCCESS)
	{
		if (lpSC->nIndicator != SQL_NULL_DATA)
			return FoxMemo::Replace(lpSC->lField, lpSC->pStmt->pGetDataBuffer.Ptr<char*>(), (int)lpSC->nIndicator, lpSC->hMemoFile);
		else if (lpSC->bNullable)
		{
			Value vNull = { '0' };
			return _DBReplace(lpSC->lField, &vNull);
		}
	}
	else if (nApiRet == SQL_SUCCESS_WITH_INFO)
	{
		// result is greater than buffer, allocate space for the memo field, nIndicator contains length of data
		nErrorNo = FoxMemo::Alloc(lpSC->lField, (int)lpSC->nIndicator, &nLoc);
		if (nErrorNo)
			return nErrorNo;

		// append the data from the buffer
		nErrorNo = FoxMemo::Append(lpSC->pStmt->pGetDataBuffer.Ptr<char*>(), (int)(lpSC->pStmt->pGetDataBuffer.Size() - 2), lpSC->hMemoFile, &nLoc);
		if (nErrorNo)
			return nErrorNo;

		do 
		{
			nApiRet = SQLGetData(lpSC->pStmt->hStmt,lpSC->nColNo,lpSC->nCType, lpSC->pStmt->pGetDataBuffer.Ptr<SQLPOINTER>(), 
				(SQLINTEGER)lpSC->pStmt->pGetDataBuffer.Size(), &lpSC->nIndicator);
			if (SQL_SUCCEEDED(nApiRet))
			{
				nErrorNo = FoxMemo::Append(lpSC->pStmt->pGetDataBuffer.Ptr<char*>(), (int)(lpSC->pStmt->pGetDataBuffer.Size() - 2), lpSC->hMemoFile, 0);
				if (nErrorNo)
					return nErrorNo;
			}
		} while (nApiRet == SQL_SUCCESS_WITH_INFO);

		if (nApiRet == SQL_ERROR)
		{
			SaveODBCError("SQLGetData", lpSC->pStmt->hStmt);
			return E_APIERROR;
		}
	}
	else
	{
		SaveODBCError("SQLGetData", lpSC->pStmt->hStmt);
		return E_APIERROR;
	}
	return 0;
}

int _fastcall SQLStoreMemoWCharVar(SqlColumn* lpSC)
{
	SQLRETURN nApiRet = SQLGetData(lpSC->pStmt->hStmt,lpSC->nColNo,lpSC->nCType, lpSC->pStmt->pGetDataBuffer.Ptr<SQLPOINTER>(),
		(SQLINTEGER)lpSC->pStmt->pGetDataBuffer.Size(), &lpSC->nIndicator);
	if (nApiRet == SQL_SUCCESS)
	{
		if (lpSC->nIndicator == SQL_NULL_DATA)
		{
			Value vNull = { '0' };
			return _Store(lpSC->lField, &vNull);
		}
		else
		{
			lpSC->pStmt->pGetDataBuffer.Len(lpSC->nIndicator);
			return _Store(lpSC->lField, lpSC->pStmt->pGetDataBuffer);
		}
	}
	else if (nApiRet == SQL_SUCCESS_WITH_INFO)
	{
		unsigned int nDataSize = lpSC->pStmt->pGetDataBuffer.Size() - 2;
		lpSC->pStmt->pGetDataBuffer.Size(lpSC->nIndicator + 2);
		lpSC->pStmt->pGetDataBuffer.Len(lpSC->nIndicator);
		nApiRet = SQLGetData(lpSC->pStmt->hStmt, lpSC->nColNo, lpSC->nCType, lpSC->pStmt->pGetDataBuffer.Ptr<char*>() + nDataSize,
			(SQLINTEGER)(lpSC->pStmt->pGetDataBuffer.Size() - nDataSize), &lpSC->nIndicator);
		if (nApiRet == SQL_SUCCESS)
			return _Store(lpSC->lField, lpSC->pStmt->pGetDataBuffer);
		else
		{
			SaveODBCError("SQLGetData", lpSC->pStmt->hStmt);
			return E_APIERROR;
		}
	}
	else
	{
		SaveODBCError("SQLGetData", lpSC->pStmt->hStmt);
		return E_APIERROR;
	}
}

int _fastcall SQLStoreMemoWCharByBindingSet(SqlColumn* lpSC, int nRow)
{
	SQLLEN nIndicator = ((SQLLEN*)lpSC->nIndicator)[nRow];
	if (nIndicator == SQL_NULL_DATA)
	{
		Value vNull = { '0' };
		return _DBReplace(lpSC->lField, &vNull);
	}
	else
	{
		int nSize = lpSC->nIndicator == SQL_NO_TOTAL ? (int)lpSC->nIndicator : (int)lpSC->nSize;
		memcpy(lpSC->pStmt->pGetDataBuffer.Ptr<char*>(), IndexPtr<wchar_t*>(lpSC->pDataPtr, nRow), nSize);
		return FoxMemo::Replace(lpSC->lField, lpSC->pStmt->pGetDataBuffer.Ptr<char*>(), nSize, lpSC->hMemoFile);
	}
}

int _fastcall SQLStoreMemoBinary(SqlColumn* lpSC)
{
	SQLRETURN nApiRet;
	long nLoc;
	int nErrorNo;

	nApiRet = SQLGetData(lpSC->pStmt->hStmt,lpSC->nColNo,lpSC->nCType, lpSC->pStmt->pGetDataBuffer.Ptr<SQLPOINTER>(),
		(SQLINTEGER)lpSC->pStmt->pGetDataBuffer.Size(), &lpSC->nIndicator);
	
	if (nApiRet == SQL_SUCCESS)
	{
		if (lpSC->nIndicator != SQL_NULL_DATA)
			return FoxMemo::Replace(lpSC->lField, lpSC->pStmt->pGetDataBuffer.Ptr<char*>(), (int)lpSC->nIndicator, lpSC->hMemoFile);
		else if (lpSC->bNullable)
		{
			Value vNull = { '0' };
			return _DBReplace(lpSC->lField, &vNull);
		}
	}
	else if (nApiRet == SQL_SUCCESS_WITH_INFO)
	{
		// allocate space for the memo field, nIncicator contains length of full data
		nErrorNo = FoxMemo::Alloc(lpSC->lField, (int)lpSC->nIndicator, &nLoc);
		if (nErrorNo)
			return nErrorNo;

		// append the data from the buffer
		nErrorNo = FoxMemo::Append(lpSC->pStmt->pGetDataBuffer.Ptr<char*>(), (int)lpSC->pStmt->pGetDataBuffer.Size(), lpSC->hMemoFile, &nLoc);
		if (nErrorNo)
			return nErrorNo;

		do 
		{
			nApiRet = SQLGetData(lpSC->pStmt->hStmt,lpSC->nColNo,lpSC->nCType, lpSC->pStmt->pGetDataBuffer.Ptr<SQLPOINTER>(),
				(SQLINTEGER)lpSC->pStmt->pGetDataBuffer.Size(), &lpSC->nIndicator);
			if (SQL_SUCCEEDED(nApiRet))
			{
				nErrorNo = FoxMemo::Append(lpSC->pStmt->pGetDataBuffer.Ptr<char*>(), (int)lpSC->pStmt->pGetDataBuffer.Size(), lpSC->hMemoFile, 0);
				if (nErrorNo)
					return nErrorNo;
			}
		} while (nApiRet == SQL_SUCCESS_WITH_INFO);

		if (nApiRet == SQL_ERROR)
		{
			SaveODBCError("SQLGetData", lpSC->pStmt->hStmt);
			return E_APIERROR;
		}
	}
	else
	{
		SaveODBCError("SQLGetData", lpSC->pStmt->hStmt);
		return E_APIERROR;
	}

	return 0;
}

int _fastcall SQLStoreMemoBinaryVar(SqlColumn* lpSC)
{
	SQLRETURN nApiRet = SQLGetData(lpSC->pStmt->hStmt,lpSC->nColNo,lpSC->nCType, lpSC->pStmt->pGetDataBuffer.Ptr<SQLPOINTER>(), 
		(SQLINTEGER)lpSC->pStmt->pGetDataBuffer.Size(), &lpSC->nIndicator);
	if (nApiRet == SQL_SUCCESS)
	{
		if (lpSC->nIndicator == SQL_NULL_DATA)
		{
			Value vNull = { '0' };
			return _Store(lpSC->lField, &vNull);
		}
		else
		{
			lpSC->pStmt->pGetDataBuffer.Len(lpSC->nIndicator);
			return _Store(lpSC->lField, lpSC->pStmt->pGetDataBuffer);
		}
	}
	else if (nApiRet == SQL_SUCCESS_WITH_INFO)
	{
		char* pData = lpSC->pStmt->pGetDataBuffer.Ptr<char*>();
		unsigned int nDataSize = lpSC->pStmt->pGetDataBuffer.Size();
		pData += nDataSize;
		lpSC->pStmt->pGetDataBuffer.Size(lpSC->nIndicator);
		lpSC->pStmt->pGetDataBuffer.Len(lpSC->nIndicator);
		nApiRet = SQLGetData(lpSC->pStmt->hStmt, lpSC->nColNo, lpSC->nCType, pData, (SQLINTEGER)(lpSC->pStmt->pGetDataBuffer.Size() - nDataSize), &lpSC->nIndicator);
		if (nApiRet == SQL_SUCCESS)
			return _Store(lpSC->lField, lpSC->pStmt->pGetDataBuffer);
		else
		{
			SaveODBCError("SQLGetData", lpSC->pStmt->hStmt);
			return E_APIERROR;
		}
	}
	else
	{
		SaveODBCError("SQLGetData", lpSC->pStmt->hStmt);
		return E_APIERROR;
	}
}

int _fastcall SQLStoreMemoBinaryByBindingSet(SqlColumn* lpSC, int nRow)
{
	SQLLEN nIndicator = ((SQLLEN*)lpSC->nIndicator)[nRow];
	if (nIndicator == SQL_NULL_DATA)
	{
		Value vNull = { '0' };
		return _DBReplace(lpSC->lField, &vNull);
	}
	else
	{
		int nSize = lpSC->nIndicator == SQL_NO_TOTAL ? (int)lpSC->nIndicator : (int)lpSC->nSize;
		memcpy(lpSC->pStmt->pGetDataBuffer.Ptr<char*>(), IndexPtr<char*>(lpSC->pDataPtr, nRow), nSize);
		return FoxMemo::Replace(lpSC->lField, lpSC->pStmt->pGetDataBuffer.Ptr<char*>(), nSize, lpSC->hMemoFile);
	}
}

int _fastcall SQLStoreSqlVariantByGetData(SqlColumn* lpSC)
{
	SQLLEN nLen = 0, nCType = 0;
	char* pData = lpSC->pStmt->pGetDataBuffer.Ptr<char*>();
	SqlVariant* pVariant = (SqlVariant*)pData;
	pVariant->nSize = 0;
	pVariant->nScale = 0;
	SQLRETURN nApiRet = SQLGetData(lpSC->pStmt->hStmt, lpSC->nColNo, SQL_C_BINARY, pData, 0, &nLen);
	if (SQL_SUCCEEDED(nApiRet) && nLen != -1)
	{
		nApiRet = SQLColAttribute(lpSC->pStmt->hStmt, lpSC->nColNo, SQL_CA_SS_VARIANT_TYPE, 0, 0, 0, &nCType);
		if (SQL_SUCCEEDED(nApiRet))
		{
			switch (nCType)
			{
				case SQL_C_BINARY:
					lpSC->pStmt->GetImplementationRowDescriptorField((SQLSMALLINT)lpSC->nColNo, SQL_CA_SS_COLUMN_SSTYPE, &pVariant->nType, 0, 0);
					switch (pVariant->nType)
					{
						case SQLTIMEN:
							lpSC->pStmt->GetImplementationRowDescriptorField((SQLSMALLINT)lpSC->nColNo, SQL_DESC_SCALE, &pVariant->nScale, 0, 0);
							pVariant->nType = SQL_C_SS_TIME2;
							nCType = SQL_C_CHAR;
							break;
						case SQLDATETIMEOFFSETN:
							lpSC->pStmt->GetImplementationRowDescriptorField((SQLSMALLINT)lpSC->nColNo, SQL_DESC_SCALE, &pVariant->nScale, 0, 0);
							pVariant->nType = SQL_C_SS_TIMESTAMPOFFSET;
							nCType = SQL_C_CHAR;
							break;
						default:
							pVariant->nType = (SQLSMALLINT)nCType;
							pVariant->nSize = (SQLSMALLINT)nLen;
					}
					break;

				case SQL_C_NUMERIC:
					lpSC->pStmt->GetImplementationRowDescriptorField((SQLSMALLINT)lpSC->nColNo, SQL_CA_SS_COLUMN_SSTYPE, &pVariant->nType, 0, 0);
					switch (pVariant->nType)
					{
						case SQLMONEY:
						case SQLMONEYN:
						case SQLMONEY4:
							pVariant->nType = SQL_C_MONEY;
							break;
						default:
							lpSC->pStmt->GetImplementationRowDescriptorField((SQLSMALLINT)lpSC->nColNo, SQL_DESC_PRECISION, &pVariant->nSize, 0, 0);
							lpSC->pStmt->GetImplementationRowDescriptorField((SQLSMALLINT)lpSC->nColNo, SQL_DESC_SCALE, &pVariant->nScale, 0, 0);
							pVariant->nType = SQL_C_NUMERIC;
							nCType = SQL_C_DOUBLE;
					}
					break;

				case SQL_C_TIME:
					pVariant->nType = (SQLSMALLINT)nCType;
					lpSC->pStmt->GetImplementationRowDescriptorField((SQLSMALLINT)lpSC->nColNo, SQL_DESC_SCALE, &pVariant->nScale, 0, 0);
					nCType = SQL_C_CHAR;
					break;

				case SQL_C_TIMESTAMP:
					pVariant->nType = (SQLSMALLINT)nCType;
					lpSC->pStmt->GetImplementationRowDescriptorField((SQLSMALLINT)lpSC->nColNo, SQL_DESC_SCALE, &pVariant->nScale, 0, 0);
					if (pVariant->nScale)
						nCType = SQL_C_CHAR;
					break;

				case SQL_C_SBIGINT:
					pVariant->nType = (SQLSMALLINT)nCType;
					nCType = SQL_C_CHAR;
					break;

				default:
					pVariant->nType = (SQLSMALLINT)nCType;
					pVariant->nSize = (SQLSMALLINT)nLen;
			}
			
			nApiRet = SQLGetData(lpSC->pStmt->hStmt, lpSC->nColNo, (SQLSMALLINT)nCType,
				pData + sizeof(SqlVariant), (SQLINTEGER)(lpSC->pStmt->pGetDataBuffer.Size() - sizeof(SqlVariant)), &lpSC->nIndicator);
			if (SQL_SUCCEEDED(nApiRet))
			{
				lpSC->pStmt->pGetDataBuffer.Len((unsigned int)lpSC->nIndicator + sizeof(SqlVariant));
				if (lpSC->lField.IsField())
					return _DBReplace(lpSC->lField, lpSC->pStmt->pGetDataBuffer);
				else
					return _Store(lpSC->lField, lpSC->pStmt->pGetDataBuffer);
			}
		}
	}
	if (nLen == -1)
	{
		Value vNull = { '0' };
		return _DBReplace(lpSC->lField, &vNull);
	}
	SaveODBCError("SQLColAttribute", lpSC->pStmt->hStmt);
	return E_APIERROR;
}

void _fastcall Timestamp_StructToValue(SQL_TIMESTAMP_STRUCT *pTimestamp, Value *pValue)
{
	int lnA, lnY, lnM, lnJDay;
	lnA = (14 - pTimestamp->month) / 12;
	lnY = pTimestamp->year + 4800 - lnA;
	lnM = pTimestamp->month + 12 * lnA - 3;
	lnJDay = pTimestamp->day + (153 * lnM + 2) / 5 + lnY * 365 + lnY / 4 - lnY / 100 + lnY / 400 - 32045;
	pValue->ev_real = ((double)lnJDay) + (((double)pTimestamp->hour) * 3600 + pTimestamp->minute * 60 + pTimestamp->second) / 86400;
}

void _fastcall ValueToTimestamp_Struct(Value *pValue, SQL_TIMESTAMP_STRUCT *pTimestamp)
{
	int lnA, lnB, lnC, lnD, lnE, lnM;
	DWORD lnDays, lnSecs;
	double dDays, dSecs;

	dSecs = modf(pValue->ev_real,&dDays);
	lnDays = static_cast<DWORD>(dDays);

	lnA = (int)(lnDays + 32044);
	lnB = (4 * lnA + 3) / 146097;
	lnC = lnA - (lnB * 146097) / 4;

	lnD = (4 * lnC + 3) / 1461;
	lnE = lnC - (1461 * lnD) / 4;
	lnM = (5 * lnE + 2) / 153;
	
	pTimestamp->day = (SQLUSMALLINT)(lnE - (153 * lnM + 2) / 5 + 1);
	pTimestamp->month = (SQLUSMALLINT)(lnM + 3 - 12 * (lnM / 10));
	pTimestamp->year = (SQLSMALLINT)(lnB * 100 + lnD - 4800 + lnM / 10);

	lnSecs = (int)floor(dSecs * 86400.0 + 0.1);
	pTimestamp->hour = (SQLUSMALLINT)(lnSecs / 3600);
	lnSecs %= 3600;
	pTimestamp->minute = (SQLUSMALLINT)(lnSecs / 60);
	lnSecs %= 60;
	pTimestamp->second = (SQLUSMALLINT)lnSecs;
	pTimestamp->fraction = 0;
}

void _fastcall Date_StructToValue(SQL_DATE_STRUCT *pDate, Value *pValue)
{
	int lnA, lnY, lnM, lnJDay;
	lnA = (14 - pDate->month) / 12;
	lnY = pDate->year + 4800 - lnA;
	lnM = pDate->month + 12 * lnA - 3;
	lnJDay = pDate->day + (153 * lnM + 2) / 5 + lnY * 365 + lnY / 4 - lnY / 100 + lnY / 400 - 32045;
	pValue->ev_real = ((double)lnJDay);
}

void _fastcall ValueToDate_Struct(Value *pValue, SQL_DATE_STRUCT *pDate)
{
	int lnA, lnB, lnC, lnD, lnE, lnM;
	DWORD lnDays;

	lnDays = static_cast<DWORD>(pValue->ev_real);

	lnA = (int)(lnDays + 32044);
	lnB = (4 * lnA + 3) / 146097;
	lnC = lnA - (lnB * 146097) / 4;

	lnD = (4 * lnC + 3) / 1461;
	lnE = lnC - (1461 * lnD) / 4;
	lnM = (5 * lnE + 2) / 153;
	
	pDate->day = (SQLUSMALLINT)(lnE - (153 * lnM + 2) / 5 + 1);
	pDate->month = (SQLUSMALLINT)(lnM + 3 - 12 * (lnM / 10));
	// NOLINTNEXTLINE(bugprone-narrowing-conversions)
	pDate->year = (SQLSMALLINT)(lnB * 100 + lnD - 4800 + lnM / 10);
}

void _fastcall StringToDate_Struct(char* lpString, SQLLEN nLen, SQL_DATE_STRUCT* pTime)
{
	CStringView pView(lpString, nLen);
	pView = pView.Alltrim();
	CStringMatcher pMatcher(pView);
	if (!pMatcher.MatchShort(&pTime->year))
	{
		SaveCustomError("StringToDate_Struct", "Invalid data parsing date string, exptected year, near '%20V'", pMatcher.CurrentView());
		throw E_APIERROR;
	}
	if (!pMatcher.MatchChar('-'))
	{
		SaveCustomError("StringToDate_Struct", "Invalid data parsing date string, exptected '-', near '%20V'", pMatcher.CurrentView());
		throw E_APIERROR;
	}
	if (!pMatcher.MatchUShort(&pTime->month))
	{
		SaveCustomError("StringToDate_Struct", "Invalid data parsing date string, exptected month, near '%20V'", pMatcher.CurrentView());
		throw E_APIERROR;
	}
	if (!pMatcher.MatchChar('-'))
	{
		SaveCustomError("StringToDate_Struct", "Invalid data parsing date string, exptected '-', near '%20V'", pMatcher.CurrentView());
		throw E_APIERROR;
	}
	if (!pMatcher.MatchUShort(&pTime->day))
	{
		SaveCustomError("StringToDate_Struct", "Invalid data parsing date string, exptected day, near '%20V'", pMatcher.CurrentView());
		throw E_APIERROR;
	}
}

void _fastcall StringToTime_Struct(char* lpString, SQLLEN nLen, SQL_TIME_STRUCT* pTime)
{
	CStringView pView(lpString, nLen);
	pView = pView.Alltrim();
	CStringMatcher pMatcher(pView);
	if (!pMatcher.MatchUShort(&pTime->hour))
	{
		SaveCustomError("StringToTime_Struct", "Invalid data parsing time string, exptected hour, near '%20V'", pMatcher.CurrentView());
		throw E_APIERROR;
	}
	if (!pMatcher.MatchChar(':'))
	{
		SaveCustomError("StringToTime_Struct", "Invalid data parsing time string, exptected ':', near '%20V'", pMatcher.CurrentView());
		throw E_APIERROR;
	}
	if (!pMatcher.MatchUShort(&pTime->minute))
	{
		SaveCustomError("StringToTime_Struct", "Invalid data parsing time string, exptected minute, near '%20V'", pMatcher.CurrentView());
		throw E_APIERROR;
	}
	if (!pMatcher.MatchChar(':'))
	{
		SaveCustomError("StringToTime_Struct", "Invalid data parsing time string, exptected ':', near '%20V'", pMatcher.CurrentView());
		throw E_APIERROR;
	}
	if (!pMatcher.MatchUShort(&pTime->second))
	{
		SaveCustomError("StringToTime_Struct", "Invalid data parsing time string, exptected seconds, near '%20V'", pMatcher.CurrentView());
		throw E_APIERROR;
	}
}

void _fastcall StringToTimestamp_Struct(char* lpString, SQLLEN nLen, SQLSMALLINT nDecimals, SQL_TIMESTAMP_STRUCT* pTime)
{
	int nFractionLen, nFractionMultiplier;
	CStringView pView(lpString, nLen);
	pView = pView.Alltrim();
	CStringMatcher pMatcher(pView);
	if (!pMatcher.MatchShort(&pTime->year))
	{
		SaveCustomError("StringToTimestamp_Struct", "Invalid data parsing datetime string, exptected year, near '%20V'", pMatcher.CurrentView());
		throw E_APIERROR;
	}
	if (!pMatcher.MatchChar('-'))
	{
		SaveCustomError("StringToTimestamp_Struct", "Invalid data parsing datetime string, exptected '-', near '%20V'", pMatcher.CurrentView());
		throw E_APIERROR;
	}
	if (!pMatcher.MatchUShort(&pTime->month))
	{
		SaveCustomError("StringToTimestamp_Struct", "Invalid data parsing datetime string, exptected month, near '%20V'", pMatcher.CurrentView());
		throw E_APIERROR;
	}
	if (!pMatcher.MatchChar('-'))
	{
		SaveCustomError("StringToTimestamp_Struct", "Invalid data parsing datetime string, exptected '-', near '%20V'", pMatcher.CurrentView());
		throw E_APIERROR;
	}
	if (!pMatcher.MatchUShort(&pTime->day))
	{
		SaveCustomError("StringToTimestamp_Struct", "Invalid data parsing datetime string, exptected day, near '%20V'", pMatcher.CurrentView());
		throw E_APIERROR;
	}
	if (!pMatcher.MatchChar(' '))
	{
		SaveCustomError("StringToTimestamp_Struct", "Invalid data parsing datetime string, exptected space, near '%20V'", pMatcher.CurrentView());
		throw E_APIERROR;
	}
	if (!pMatcher.MatchUShort(&pTime->hour))
	{
		SaveCustomError("StringToTimestamp_Struct", "Invalid data parsing datetime string, exptected hour, near '%20V'", pMatcher.CurrentView());
		throw E_APIERROR;
	}
	if (!pMatcher.MatchChar(':'))
	{
		SaveCustomError("StringToTimestamp_Struct", "Invalid data parsing datetime string, exptected ':', near '%20V'", pMatcher.CurrentView());
		throw E_APIERROR;
	}
	if (!pMatcher.MatchUShort(&pTime->minute))
	{
		SaveCustomError("StringToTimestamp_Struct", "Invalid data parsing datetime string, exptected minute, near '%20V'", pMatcher.CurrentView());
		throw E_APIERROR;
	}
	if (!pMatcher.MatchChar(':'))
	{
		SaveCustomError("StringToTimestamp_Struct", "Invalid data parsing datetime string, exptected ':', near '%20V'", pMatcher.CurrentView());
		throw E_APIERROR;
	}
	if (!pMatcher.MatchUShort(&pTime->second))
	{
		SaveCustomError("StringToTimestamp_Struct", "Invalid data parsing datetime string, exptected seconds, near '%20V'", pMatcher.CurrentView());
		throw E_APIERROR;
	}
	if (pMatcher.MatchChar('.'))
	{
		if (!pMatcher.MatchULong(&pTime->fraction, &nFractionLen))
			pTime->fraction = 0;
		else
		{
			nFractionMultiplier = 100;
			for (int xj = 0; xj < (7 - nFractionLen); xj++)
				nFractionMultiplier *= 10;
			pTime->fraction *= nFractionMultiplier;
		}
	}
	else
		pTime->fraction = 0;
}

void _fastcall StringToTimestampOffset_Struct(char* lpString, SQLLEN nLen, SQLSMALLINT nDecimals, SQL_SS_TIMESTAMPOFFSET_STRUCT* pTime)
{
	bool bPositive;
	int nFractionLen, nFractionMultiplier;
	CStringView pView(lpString, nLen);
	pView = pView.Alltrim();
	CStringMatcher pMatcher(pView);
	if (!pMatcher.MatchShort(&pTime->year))
	{
		SaveCustomError("StringToTimestampOffset_Struct", "Invalid data parsing datetime string, exptected year, near '%20V'", pMatcher.CurrentView());
		throw E_APIERROR;
	}
	if (!pMatcher.MatchChar('-'))
	{
		SaveCustomError("StringToTimestampOffset_Struct", "Invalid data parsing datetime string, exptected '-', near '%20V'", pMatcher.CurrentView());
		throw E_APIERROR;
	}
	if (!pMatcher.MatchUShort(&pTime->month))
	{
		SaveCustomError("StringToTimestampOffset_Struct", "Invalid data parsing datetime string, exptected month, near '%20V'", pMatcher.CurrentView());
		throw E_APIERROR;
	}
	if (!pMatcher.MatchChar('-'))
	{
		SaveCustomError("StringToTimestampOffset_Struct", "Invalid data parsing datetime string, exptected '-', near '%20V'", pMatcher.CurrentView());
		throw E_APIERROR;
	}
	if (!pMatcher.MatchUShort(&pTime->day))
	{
		SaveCustomError("StringToTimestampOffset_Struct", "Invalid data parsing datetime string, exptected day, near '%20V'", pMatcher.CurrentView());
		throw E_APIERROR;
	}
	if (!pMatcher.MatchChar(' '))
	{
		SaveCustomError("StringToTimestampOffset_Struct", "Invalid data parsing datetime string, exptected space, near '%20V'", pMatcher.CurrentView());
		throw E_APIERROR;
	}
	if (!pMatcher.MatchUShort(&pTime->hour))
	{
		SaveCustomError("StringToTimestampOffset_Struct", "Invalid data parsing datetime string, exptected hour, near '%20V'", pMatcher.CurrentView());
		throw E_APIERROR;
	}
	if (!pMatcher.MatchChar(':'))
	{
		SaveCustomError("StringToTimestampOffset_Struct", "Invalid data parsing datetime string, exptected ':', near '%20V'", pMatcher.CurrentView());
		throw E_APIERROR;
	}
	if (!pMatcher.MatchUShort(&pTime->minute))
	{
		SaveCustomError("StringToTimestampOffset_Struct", "Invalid data parsing datetime string, exptected minute, near '%20V'", pMatcher.CurrentView());
		throw E_APIERROR;
	}
	if (!pMatcher.MatchChar(':'))
	{
		SaveCustomError("StringToTimestampOffset_Struct", "Invalid data parsing datetime string, exptected ':', near '%20V'", pMatcher.CurrentView());
		throw E_APIERROR;
	}
	if (!pMatcher.MatchUShort(&pTime->second))
	{
		SaveCustomError("StringToTimestampOffset_Struct", "Invalid data parsing datetime string, exptected seconds, near '%20V'", pMatcher.CurrentView());
		throw E_APIERROR;
	}
	if (pMatcher.MatchChar('.'))
	{
		if (!pMatcher.MatchULong(&pTime->fraction, &nFractionLen))
			pTime->fraction = 0;
		else
		{
			nFractionMultiplier = 100;
			for (int xj = 0; xj < (7 - nFractionLen); xj++)
				nFractionMultiplier *= 10;
			pTime->fraction *= nFractionMultiplier;
		}
	}
	else
		pTime->fraction = 0;

	if (pMatcher.MatchChar('-'))
		bPositive = false;
	else if (pMatcher.MatchChar('+'))
		bPositive = true;
	else
	{
		SaveCustomError("StringToTimestampOffset_Struct", "Invalid data parsing datetime string, exptected timezone sign, near '%20V'", pMatcher.CurrentView());
		throw E_APIERROR;
	}

	if (!pMatcher.MatchShort(&pTime->timezone_hour))
	{
		SaveCustomError("StringToTimestampOffset_Struct", "Invalid data parsing datetime string, exptected timezone hour, near '%20V'", pMatcher.CurrentView());
		throw E_APIERROR;
	}
	if (!pMatcher.MatchChar(':'))
	{
		SaveCustomError("StringToTimestampOffset_Struct", "Invalid data parsing datetime string, exptected ':', near '%20V'", pMatcher.CurrentView());
		throw E_APIERROR;
	}
	if (!pMatcher.MatchShort(&pTime->timezone_minute))
	{
		SaveCustomError("StringToTimestampOffset_Struct", "Invalid data parsing datetime string, exptected timezone minutes, near '%20V'", pMatcher.CurrentView());
		throw E_APIERROR;
	}
	if (!bPositive)
	{
		pTime->timezone_hour *= -1;
		pTime->timezone_minute *= -1;
	}
}

void _fastcall StringToTime2_Struct(char* lpString, SQLLEN nLen, SQLSMALLINT nDecimals, SQL_SS_TIME2_STRUCT* pTime)
{
	int nFractionLen, nFractionMultiplier;
	CStringView pView(lpString, nLen);
	pView = pView.Alltrim();
	CStringMatcher pMatcher(pView);
	if (!pMatcher.MatchUShort(&pTime->hour))
	{
		SaveCustomError("StringToTime2_Struct", "Invalid data parsing time string, exptected hour, near '%20V'", pMatcher.CurrentView());
		throw E_APIERROR;
	}
	if (!pMatcher.MatchChar(':'))
	{
		SaveCustomError("StringToTime2_Struct", "Invalid data parsing time string, exptected ':', near '%20V'", pMatcher.CurrentView());
		throw E_APIERROR;
	}
	if (!pMatcher.MatchUShort(&pTime->minute))
	{
		SaveCustomError("StringToTime2_Struct", "Invalid data parsing time string, exptected minute, near '%20V'", pMatcher.CurrentView());
		throw E_APIERROR;
	}
	if (!pMatcher.MatchChar(':'))
	{
		SaveCustomError("StringToTime2_Struct", "Invalid data parsing time string, exptected ':', near '%20V'", pMatcher.CurrentView());
		throw E_APIERROR;
	}
	if (!pMatcher.MatchUShort(&pTime->second))
	{
		SaveCustomError("StringToTime2_Struct", "Invalid data parsing time string, exptected seconds, near '%20V'", pMatcher.CurrentView());
		throw E_APIERROR;
	}
	if (pMatcher.MatchChar('.'))
	{
		if (!pMatcher.MatchULong(&pTime->fraction, &nFractionLen))
			pTime->fraction = 0;
		else
		{
			nFractionMultiplier = 100;
			for (int xj = 0; xj < (7 - nFractionLen); xj++)
				nFractionMultiplier *= 10;
			pTime->fraction *= nFractionMultiplier;
		}
	}
	else
		pTime->fraction = 0;
}

void _fastcall Numeric_StructToValue(SQL_NUMERIC_STRUCT * lpNumeric, Value *pValue)
{
	__int64* lpNum = (__int64*)&lpNumeric->val[0];
	pValue->ev_currency.QuadPart = *lpNum;
	if (lpNumeric->sign == 0)
		pValue->ev_currency.QuadPart = -pValue->ev_currency.QuadPart;
}

void _fastcall ValueToNumeric_Struct(Value *pValue, SQL_NUMERIC_STRUCT* lpNumeric)
{
	__int64* lpNumLow = (__int64*)&lpNumeric->val[0];
	__int64* lpNumHigh = (__int64*)&lpNumeric->val[8];
	if (pValue->ev_currency.QuadPart >= 0)
	{
		lpNumeric->sign = 1;
		*lpNumLow = pValue->ev_currency.QuadPart;
	}
	else
	{
		lpNumeric->sign = 0;
		*lpNumLow = -pValue->ev_currency.QuadPart;
	}
	*lpNumHigh = 0;
	lpNumeric->precision = VFP2C_VFP_CURRENCY_PRECISION;
	lpNumeric->scale = VFP2C_VFP_CURRENCY_SCALE;
}

void _fastcall SQLVariantToValue(ParamBlkEx& parm)
{
	try
	{
		ValueEx vValue;
		SqlVariant* pVariant = 0;
		CBuffer pBuffer;
		unsigned int nLength = 0;
		char cType = parm(1)->Vartype();
		char pStackBuffer[288];

		if (cType == 'R')
		{
			FoxMemo vMemo(parm(1));
			if (vMemo.Size() == 0)
			{
				vValue.SetNull();
				Return(vValue);
				return;
			}

			nLength = vMemo.Size();
			if (nLength <= sizeof(pStackBuffer))
			{
				vMemo.Read(pStackBuffer, nLength);
				pVariant = (SqlVariant*)pStackBuffer;
			}
			else
			{
				pBuffer.Size(nLength);
				vMemo.Read(pBuffer.Ptr<char*>(), nLength);
				pVariant = pBuffer.Ptr<SqlVariant*>();
			}
		}
		else if (cType == 'C')
		{
			pVariant = parm(1)->HandleToPtr<SqlVariant*>();
			nLength = parm(1)->Len();
		}
		else if (cType == '0')
		{
			vValue.SetNull();
			Return(vValue);
			return;
		}
		else
			throw E_INVALIDPARAMS;

		switch (pVariant->nType)
		{
			case SQL_C_CHAR:
			case SQL_C_WCHAR:
			case SQL_C_TIME:
			case SQL_C_SS_TIME2:
			case SQL_C_SS_TIMESTAMPOFFSET:
				vValue.SetString(nLength - sizeof(SqlVariant));
				if (!vValue.AllocHandle(vValue.Len()))
					throw E_INSUFMEMORY;
				memcpy(vValue.HandleToPtr(), &pVariant->pData, vValue.Len());
				Return(vValue);
				break;

			case SQL_C_BINARY:
				vValue.SetBinaryString(nLength - sizeof(SqlVariant));
				if (!vValue.AllocHandle(vValue.Len()))
					throw E_INSUFMEMORY;
				memcpy(vValue.HandleToPtr(), &pVariant->pData, vValue.Len());
				Return(vValue);
				break;

			case SQL_C_SLONG:
				vValue.SetInt();
				vValue.ev_long = *(int*)pVariant->pData;
				Return(vValue);
				break;

			case SQL_C_DOUBLE:
				vValue.SetDouble();
				vValue.ev_real = *(double*)pVariant->pData;
				Return(vValue);
				break;

			case SQL_C_BIT:
				vValue.SetLogical();
				vValue.ev_length = *(unsigned char*)pVariant->pData;
				Return(vValue);
				break;

			case SQL_C_NUMERIC:
				if (pVariant->nSize > 20 || pVariant->nScale > 16)
				{
					vValue.SetString(nLength - sizeof(SqlVariant));
					if (!vValue.AllocHandle(vValue.Len()))
						throw E_INSUFMEMORY;
					memcpy(vValue.HandleToPtr(), &pVariant->pData, vValue.Len());
					Return(vValue);
				}
				else
				{
					vValue.SetDouble(*(double*)&pVariant->pData, pVariant->nSize + 2);
					Return(vValue);
				}
				break;

			case SQL_C_MONEY:
				vValue.SetCurrency();
				Numeric_StructToValue((SQL_NUMERIC_STRUCT*)&pVariant->pData, vValue);
				_RetCurrency(vValue.ev_currency, 21);
				break;

			case SQL_C_TIMESTAMP:
				if (pVariant->nScale == 0)
				{
					vValue.SetDateTime();
					Timestamp_StructToValue((SQL_TIMESTAMP_STRUCT*)&pVariant->pData, vValue);
					Return(vValue);
				}
				else
				{
					vValue.SetString(nLength - sizeof(SqlVariant));
					if (!vValue.AllocHandle(vValue.Len()))
						throw E_INSUFMEMORY;
					memcpy(vValue.HandleToPtr(), &pVariant->pData, vValue.Len());
					Return(vValue);
				}
				break;

			case SQL_C_DATE:
				vValue.SetDate();
				Date_StructToValue((SQL_DATE_STRUCT*)&pVariant->pData, vValue);
				Return(vValue);
				break;

			case SQL_C_SBIGINT:
				vValue.SetString(nLength - sizeof(SqlVariant));
				if (!vValue.AllocHandle(vValue.Len()))
					throw E_INSUFMEMORY;
				memcpy(vValue.HandleToPtr(), &pVariant->pData, vValue.Len());
				Return(vValue);
				break;

			default:
				SaveCustomError("SQLVariant2Value", "Unknown SQL_VARIANT type %i", pVariant->nType);
				throw E_APIERROR;
		}

		if (parm.PCount() >= 2)
		{
			LocatorEx& pRef = parm(2);
			pRef = pVariant->nType;
		}

		if (parm.PCount() >= 3)
		{
			LocatorEx& pRef = parm(3);
			pRef = pVariant->nSize;
		}

		if (parm.PCount() >= 4)
		{
			LocatorEx& pRef = parm(4);
			pRef = pVariant->nScale;
		}
	}
	catch (int nErrorNo)
	{
		RaiseError(nErrorNo);
	}
}

void _fastcall ValueToSQLVariant(ParamBlkEx& parm)
{
	int nErrorNo = 0;
	SqlVariant* pSqlVariant;
	ValueEx vVariant;
	vVariant.SetString();
	int nLength = 0;

	switch (parm(1)->Vartype())
	{
		case 'I':
			nLength = sizeof(SqlVariant) + sizeof(int);
			if (vVariant.AllocHandle(nLength))
			{
				pSqlVariant = vVariant.HandleToPtr<SqlVariant*>();
				pSqlVariant->nType = SQL_C_SLONG;
				*(int*)pSqlVariant->pData = parm(1)->ev_long;
				break;
			}

		case 'N':
			nLength = sizeof(SqlVariant) + sizeof(double);
			if (vVariant.AllocHandle(nLength))
			{
				pSqlVariant = vVariant.HandleToPtr<SqlVariant*>();
				pSqlVariant->nType = SQL_C_DOUBLE;
				*(double*)pSqlVariant->pData = parm(1)->ev_real;
				break;
			}

		case 'C':
			nLength = (int)(sizeof(SqlVariant) + parm(1)->ev_length);
			if (vVariant.AllocHandle(nLength))
			{
				if (parm.PCount() == 1)
				{
					pSqlVariant = vVariant.HandleToPtr<SqlVariant*>();
					pSqlVariant->nType = parm(1)->Binary() ? SQL_C_BINARY : SQL_C_CHAR;
					memcpy(&pSqlVariant->pData, parm(1)->HandleToPtr(), parm(1)->ev_length);
				}
				else if (parm.PCount() == 2)
				{
					pSqlVariant = vVariant.HandleToPtr<SqlVariant*>();
					pSqlVariant->nType = (SQLSMALLINT)parm(2)->ev_long;
					memcpy(&pSqlVariant->pData, parm(1)->HandleToPtr(), parm(1)->ev_length);
				}
				else if (parm.PCount() == 3)
				{
					pSqlVariant = vVariant.HandleToPtr<SqlVariant*>();
					pSqlVariant->nType = (SQLSMALLINT)parm(2)->ev_long;
					pSqlVariant->nScale = (SQLSMALLINT)parm(3)->ev_long;
					memcpy(&pSqlVariant->pData, parm(1)->HandleToPtr(), parm(1)->ev_length);
				}
				break;
			}

		case 'L':
			nLength = sizeof(SqlVariant) + sizeof(char);
			if (vVariant.AllocHandle(nLength))
			{
				pSqlVariant = vVariant.HandleToPtr<SqlVariant*>();
				pSqlVariant->nType = SQL_C_BIT;
				*(unsigned char*)pSqlVariant->pData = (parm(1)->ev_length ? 1 : 0);
				break;
			}

		case 'T':
			nLength = sizeof(SqlVariant) + sizeof(SQL_TIMESTAMP_STRUCT);
			if (vVariant.AllocHandle(nLength))
			{
				pSqlVariant = vVariant.HandleToPtr<SqlVariant*>();
				pSqlVariant->nType = SQL_C_TIMESTAMP;
				ValueToTimestamp_Struct(&parm(1).val, (SQL_TIMESTAMP_STRUCT*)&pSqlVariant->pData);
				break;
			}

		case 'D':
			nLength = sizeof(SqlVariant) + sizeof(SQL_DATE_STRUCT);
			if (vVariant.AllocHandle(nLength))
			{
				pSqlVariant = vVariant.HandleToPtr<SqlVariant*>();
				pSqlVariant->nType = SQL_C_DATE;
				ValueToDate_Struct(&parm(1).val, (SQL_DATE_STRUCT*)&pSqlVariant->pData);
				break;
			}

		case 'Y':
			nLength = sizeof(SqlVariant) + sizeof(SQL_NUMERIC_STRUCT);
			if (vVariant.AllocHandle(nLength))
			{
				pSqlVariant = vVariant.HandleToPtr<SqlVariant*>();
				pSqlVariant->nType = SQL_C_NUMERIC;
				pSqlVariant->nSize = VFP2C_VFP_CURRENCY_PRECISION;
				pSqlVariant->nScale = VFP2C_VFP_CURRENCY_SCALE;
				ValueToNumeric_Struct(&parm(1).val, (SQL_NUMERIC_STRUCT*)&pSqlVariant->pData);
				break;
			}

		case '0':
			vVariant.SetNull();
			break;

		default:
			nErrorNo = E_INVALIDPARAMS;
	}

	if (nErrorNo == 0)
	{
		vVariant.ev_length = nLength;
		vVariant.Return();
	}
	else
		RaiseError(nErrorNo);
}