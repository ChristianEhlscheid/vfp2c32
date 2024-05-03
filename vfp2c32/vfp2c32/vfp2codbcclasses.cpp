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
#include "vfp2codbcclasses.h"
#include "vfp2ccppapi.h"

SqlTTMap g_RegisteredTableTypes;

SqlStatement::SqlStatement()
{
	ServerType = UnknownDBServer;
	nResultset = 0;
	nCallbackInterval = 100;
	nRowSetSize = 1;
	nRowsFetched = 0;
	nRowsTotal = 0;
	nFlags = 0;
	nUseMemoSize = 255;
	bOutputParams = false;
	bMapVarchar = false;
	bMapVarbinary = false;
	bMapBinaryToGeneral = false;
	bUnicodeConversion = false;
	bStoreInfoAsError = false;
	bPrepared = false;
	bExecutedOnce = false;
	// private members
	nDefaultDecimals = -1;
}

SqlStatement::~SqlStatement()
{
	FreeParameters();
}

bool SqlStatement::FreeHandle()
{
	if (hStmt)
	{
		SQLRETURN nApiRet = SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
		hStmt = 0;
		if (nApiRet != 0)
		{
			SaveODBCError("SQLFreeHandle", hStmt);
			return false;
		}
	}
	return true;
}

SqlResultSet* SqlStatement::AddResultSet()
{
	SqlResultSet* pResultset = 0;
	if (nFlags & SQLEXECEX_PARAMETER_ARRAY)
	{
		if (((nFlags & SQLEXECEX_PARAMETER_ARRAY_RESUME_ON_ERROR) == 0 || bResumeOnError) && pResultsets.GetCount())
		{
			pResultset = pResultsets[nResultset-1];
		}
		else if (pParameterArray.nParamsProcessed != pParameterArray.nParamsLastProcessed && pResultsets.GetCount())
		{
			pResultset = pResultsets[0];
			nResultset = 1;
			pParameterArray.nParamsLastProcessed = pParameterArray.nParamsProcessed;
		}
		if (pResultset)
			return pResultset;
	}

	if (pResultsets.GetCount() > nResultset)
	{
		pResultset = pResultsets[nResultset];
	}
	else
	{
		pResultset = pResultsets.Add();
		pResultset->pStmt = this;
	}
	nResultset++;
	return pResultset;
}

void SqlStatement::FreeParameters()
{
	for (int xj = 0; xj < pParameters.GetCount(); xj++)
	{
		SqlParameter* lpSP = pParameters[xj];
		if (lpSP->vParmValue.Vartype() == 'C')
		{
			lpSP->vParmValue.UnlockHandle();
			lpSP->vParmValue.FreeHandle();
			lpSP->vParmValue.SetNull();
		}
		else if (lpSP->vParmValue.Vartype() == 'O')
		{
			lpSP->vParmValue.FreeObject();
		}

		if (lpSP->vMemoValue.Vartype() == 'C')
		{
			lpSP->vMemoValue.FreeHandle();
			lpSP->vMemoValue.SetNull();
		}
	}
	FreeInMemoryParameterArray();
}

void SqlStatement::Reset()
{
	nResultset = 0;
	// reset info array on successive executions 
	if (bExecutedOnce && pInfoArray)
	{
		pInfoArray.Dimension(1, 3);
		pInfoArray.Reset();
	}
}

void SqlStatement::SetApplicationParameterDescriptorField(SQLSMALLINT nRecNo, SQLSMALLINT nField, SQLPOINTER pValue, SQLINTEGER nBufferLen)
{
	SQLRETURN nApiRet;
	if (hAppParamDesc == false)
	{
		nApiRet = SQLGetStmtAttr(hStmt, SQL_ATTR_APP_PARAM_DESC, &hAppParamDesc, 0, 0);
		if (nApiRet == SQL_ERROR)
		{
			SaveODBCError("SQLGetStmtAttr", hStmt);
			throw E_APIERROR;
		}
		else if (nApiRet == SQL_SUCCESS_WITH_INFO && bStoreInfoAsError)
			AddODBCError("SQLGetStmtAttr", hStmt);
	}

	nApiRet = SQLSetDescField(hAppParamDesc, nRecNo, nField, pValue, nBufferLen);
	if (nApiRet == SQL_ERROR)
	{
		SaveODBCError("SQLSetDescField", hAppParamDesc);
		throw E_APIERROR;
	}
	else if (nApiRet == SQL_SUCCESS_WITH_INFO && bStoreInfoAsError)
		AddODBCError("SQLSetDescField", hAppParamDesc);
}

void SqlStatement::GetApplicationParameterDescriptorField(SQLSMALLINT nRecNo, SQLSMALLINT nField, SQLPOINTER pValue, SQLINTEGER nBufferLen, SQLINTEGER* nStringLen)
{
	SQLRETURN nApiRet;
	if (hAppParamDesc == false)
	{
		nApiRet = SQLGetStmtAttr(hStmt, SQL_ATTR_APP_PARAM_DESC, &hAppParamDesc, 0, 0);
		if (nApiRet == SQL_ERROR)
		{
			SaveODBCError("SQLGetStmtAttr", hStmt);
			throw E_APIERROR;
		}
		else if (nApiRet == SQL_SUCCESS_WITH_INFO && bStoreInfoAsError)
			AddODBCError("SQLGetStmtAttr", hStmt);
	}

	nApiRet = SQLGetDescField(hAppParamDesc, nRecNo, nField, pValue, nBufferLen, nStringLen);
	if (nApiRet == SQL_ERROR)
	{
		SaveODBCError("SQLSetDescField", hAppParamDesc);
		throw E_APIERROR;
	}
	else if (nApiRet == SQL_SUCCESS_WITH_INFO && bStoreInfoAsError)
		AddODBCError("SQLSetDescField", hAppParamDesc);
}

void SqlStatement::SetImplementationParameterDescriptorField(SQLSMALLINT nRecNo, SQLSMALLINT nField, SQLPOINTER pValue, SQLINTEGER nBufferLen)
{
	SQLRETURN nApiRet;
	if (hImpParamDesc == false)
	{
		SQLRETURN nApiRet = SQLGetStmtAttr(hStmt, SQL_ATTR_IMP_PARAM_DESC, &hImpParamDesc, 0, 0);
		if (nApiRet == SQL_ERROR)
		{
			SaveODBCError("SQLGetStmtAttr", hStmt);
			throw E_APIERROR;
		}
		else if (nApiRet == SQL_SUCCESS_WITH_INFO && bStoreInfoAsError)
			AddODBCError("SQLGetStmtAttr", hStmt);
	}

	nApiRet = SQLSetDescField(hImpParamDesc, nRecNo, nField, pValue, nBufferLen);
	if (nApiRet == SQL_ERROR)
	{
		SaveODBCError("SQLSetDescField", hImpParamDesc);
		throw E_APIERROR;
	}
	else if (nApiRet == SQL_SUCCESS_WITH_INFO && bStoreInfoAsError)
		AddODBCError("SQLSetDescField", hImpParamDesc);
}

void SqlStatement::GetImplementationParameterDescriptorField(SQLSMALLINT nRecNo, SQLSMALLINT nField, SQLPOINTER pValue, SQLINTEGER nBufferLen, SQLINTEGER* nStringLen)
{
	SQLRETURN nApiRet;
	if (hImpParamDesc == false)
	{
		SQLRETURN nApiRet = SQLGetStmtAttr(hStmt, SQL_ATTR_IMP_PARAM_DESC, &hImpParamDesc, 0, 0);
		if (nApiRet == SQL_ERROR)
		{
			SaveODBCError("SQLGetStmtAttr", hStmt);
			throw E_APIERROR;
		}
		else if (nApiRet == SQL_SUCCESS_WITH_INFO && bStoreInfoAsError)
			AddODBCError("SQLGetStmtAttr", hStmt);
	}

	nApiRet = SQLGetDescField(hImpParamDesc, nRecNo, nField, pValue, nBufferLen, nStringLen);
	if (nApiRet == SQL_ERROR)
	{
		SaveODBCError("SQLSetDescField", hImpParamDesc);
		throw E_APIERROR;
	}
	else if (nApiRet == SQL_SUCCESS_WITH_INFO && bStoreInfoAsError)
		AddODBCError("SQLSetDescField", hImpParamDesc);
}

void SqlStatement::GetImplementationRowDescriptorField(SQLSMALLINT nRecNo, SQLSMALLINT nField, SQLPOINTER pValue, SQLINTEGER nBufferLen, SQLINTEGER* nStringLen)
{
	SQLRETURN nApiRet;
	if (hImpRowDesc == false)
	{
		SQLRETURN nApiRet = SQLGetStmtAttr(hStmt, SQL_ATTR_IMP_ROW_DESC, &hImpRowDesc, 0, 0);
		if (nApiRet == SQL_ERROR)
		{
			SaveODBCError("SQLGetStmtAttr", hStmt);
			throw E_APIERROR;
		}
		else if (nApiRet == SQL_SUCCESS_WITH_INFO && bStoreInfoAsError)
			AddODBCError("SQLGetStmtAttr", hStmt);
	}

	nApiRet = SQLGetDescField(hImpRowDesc, nRecNo, nField, pValue, nBufferLen, nStringLen);
	if (nApiRet == SQL_ERROR)
	{
		SaveODBCError("SQLSetDescField", hImpRowDesc);
		throw E_APIERROR;
	}
	else if (nApiRet == SQL_SUCCESS_WITH_INFO && bStoreInfoAsError)
		AddODBCError("SQLSetDescField", hImpRowDesc);
}

SQLSMALLINT SqlStatement::GetColumnCount()
{
	SQLSMALLINT nColCount = 0;
	// get no of columns in resultset (if 0 the SQL statement didn't produce a resultset)
	SQLRETURN nApiRet = SQLNumResultCols(hStmt, &nColCount);
	if (nApiRet == SQL_ERROR)
	{
		SaveODBCError("SQLNumResultCols", hStmt);
		throw E_APIERROR;
	}
	else if (nApiRet == SQL_SUCCESS_WITH_INFO && bStoreInfoAsError)
		AddODBCError("SQLNumResultCols", hStmt);

	return nColCount;
}

void SqlStatement::UnbindColumns()
{
	// unbind column buffers from the statement
	SQLRETURN nApiRet = SQLFreeStmt(hStmt, SQL_UNBIND);
	if (nApiRet == SQL_ERROR)
	{
		SaveODBCError("SQLFreeStmt - SQL_UNBIND", hStmt);
		throw E_APIERROR;
	}
	else if (nApiRet == SQL_SUCCESS_WITH_INFO && bStoreInfoAsError)
		AddODBCError("SQLFreeStmt - SQL_UNBIND", hStmt);
}

SQLLEN SqlStatement::GetRowCount()
{
	SQLLEN nRowCount = 0;
	// read total rows found (if not supported it'll be -1/0 depending on driver in use)
	SQLRETURN nApiRet = SQLRowCount(hStmt, &nRowCount);
	if (nApiRet == SQL_ERROR)
	{
		SaveODBCError("SQLRowCount", hStmt);
		throw E_APIERROR;
	}
	else if (nApiRet == SQL_SUCCESS_WITH_INFO && bStoreInfoAsError)
		AddODBCError("SQLRowCount", hStmt);

	return nRowCount;
}

void SqlStatement::SaveRowCount()
{
	if (!pInfoArray)
		return;

	SQLRETURN nApiRet;
	SQLLEN nCount;
	SQLLEN nNCStatus;
	int nIndex;
	ValueEx vRowCount;
	ValueEx vNoCountStatus;
	vRowCount.SetDouble();
	vNoCountStatus.SetLogical(false);

	nApiRet = SQLRowCount(hStmt, &nCount);
	if (nApiRet == SQL_ERROR)
	{
		SaveODBCError("SQLRowCount", hStmt);
		throw E_APIERROR;
	}
	else if (nApiRet == SQL_SUCCESS_WITH_INFO && bStoreInfoAsError)
		AddODBCError("SQLRowCount", hStmt);

	if (ServerType == SQLServer)
	{
		if (nCount <= 0)
		{
			nApiRet = SQLGetStmtAttr(hStmt, SQL_SOPT_SS_NOCOUNT_STATUS, (SQLPOINTER)&nNCStatus, SQL_IS_POINTER, 0);
			if (nApiRet == SQL_ERROR)
			{
				SaveODBCError("SQLGetStmtAttr", hStmt);
				throw E_APIERROR;
			}
			else if (nApiRet == SQL_SUCCESS_WITH_INFO && bStoreInfoAsError)
				AddODBCError("SQLGetStmtAttr", hStmt);
			vNoCountStatus.ev_length = nNCStatus;
		}
	}

	nIndex = pInfoArray.Grow();
	pInfoArray(nIndex, 1) = pGetDataBuffer.Len(0);

	vRowCount.ev_real = (double)nCount;
	pInfoArray(nIndex, 2) = vRowCount;

	pInfoArray(nIndex, 3) = vNoCountStatus;
}

SQLSMALLINT SqlStatement::GetDefaultDecimals()
{
	if (nDefaultDecimals == -1)
	{
		ValueEx vDecimals;
		vDecimals = 0;
		Evaluate(vDecimals, "SET('Decimals')");
		nDefaultDecimals = (SQLSMALLINT)vDecimals.ev_long;
	}
	return nDefaultDecimals;
}

bool SqlStatement::ParseParameters()
{
	char* pSQL = pSQLInput;
	nParameterCount = 0;
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
		else if (*pSQL == '?' &&
			(pSQL[1] == '{' ||
				((pSQL[1] == '@' || pSQL[1] == '$') && pSQL[2] == '{') ||
				(pSQL[1] == '@' && pSQL[2] == '$' && pSQL[3] == '{') ||
				(pSQL[1] == '&' && pSQL[2] == '{')))
		{
			nParameterCount++;
			while (*pSQL)
			{
				if (*pSQL == '\\' && pSQL[1] == '}')
					pSQL += 2;
				else if (*pSQL++ == '}')
					break;
			}
		}
		else
			pSQL++;
	}
	return nParameterCount > 0;
}

void SqlStatement::ExtractParametersAndRewriteStatement()
{
	SqlParameter* lpSP;
	char* pExpr, * pSQLIn, * pSQLOut, * pSqlOutStart;
	int nParamNo = 0, nOutputColNo = 0, nLen;

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
		else if (*pSQLIn == '?' &&
			(pSQLIn[1] == '{' ||
				((pSQLIn[1] == '@' || pSQLIn[1] == '$') && pSQLIn[2] == '{') ||
				(pSQLIn[1] == '@' && pSQLIn[2] == '$' && pSQLIn[3] == '{') ||
				(pSQLIn[1] == '&' && pSQLIn[2] == '{')))
		{
			lpSP = pParameters[nParamNo];
			lpSP->nParmNo = ++nParamNo;

			*pSQLOut++ = *pSQLIn; // write "?" 

			switch (pSQLIn[1])
			{
			case '@':
				if (pSQLIn[2] == '$')
				{
					lpSP->nParmType = SQL_PARAM_INPUT_OUTPUT_STREAM;
					pSQLIn += 4; // skip over "?@${"
				}
				else
				{
					lpSP->nParmType = SQL_PARAM_INPUT_OUTPUT;
					pSQLIn += 3; // skip over "?@{"
				}
				break;
			case '$':
				lpSP->nParmType = SQL_PARAM_OUTPUT_STREAM;
				pSQLIn += 3; // skip over "?${"
				break;
			case '&':
				lpSP->nParmType = SQL_PARAM_OUTPUT;
				pSQLIn += 3; // skip over "?&{"
				break;
			default:
				// lpSP->nParmType = SQL_PARAM_INPUT; is already the set by default
				pSQLIn += 2; // skip over "?{"
			}

			nLen = VFP2C_ODBC_MAX_PARAMETER_EXPR;
			pExpr = lpSP->aParmExpr;
			while (*pSQLIn && nLen)
			{
				if (*pSQLIn == '\\' && pSQLIn[1] == '}')
				{
					*pExpr++ = '}';
					pSQLIn += 2;
				}
				else if (*pSQLIn == '}')
					break;
				else
					*pExpr++ = *pSQLIn++;

				nLen--;
			}
			*pExpr = '\0'; // nullterminate parameter expression
			if (!nLen)
			{
				SaveCustomError("SQLExecEx", "Parameter expression '%100S...' exceeds limit of 256 characters", lpSP->aParmExpr);
				throw E_APIERROR;
			}
			pSQLIn++; // skip over "}"
		}
		else
			*pSQLOut++ = *pSQLIn++;
	}

	*pSQLOut = '\0'; // nullterminate Sql output
	pSQLSend.Len(pSQLOut - pSqlOutStart); // store length of rewritten SQL statement
}

void SqlStatement::AllocateParameters()
{
	if (nParameterCount)
		pParameters.Allocate(nParameterCount);
}

bool SqlStatement::MatchLength(CStringMatcher& pMatcher, SqlParameter* lpSP)
{
	if (pMatcher.MatchChar('('))
	{
		if (!pMatcher.MatchULong(&lpSP->nSize))
		{
			SaveCustomError("SQLExecEx", "Parameter schema contained invalid data near '%20V', expected length.", &pMatcher.CurrentView());
			throw E_APIERROR;
		}
		if (!pMatcher.MatchChar(')'))
		{
			SaveCustomError("SQLExecEx", "Parameter schema contained invalid data near '%20V', expected ')'.", &pMatcher.CurrentView());
			throw E_APIERROR;
		}
		return true;
	}
	return false;
}

bool SqlStatement::MatchPrecision(CStringMatcher& pMatcher, SqlParameter* lpSP)
{
	if (pMatcher.MatchChar('('))
	{
		if (!pMatcher.MatchShort(&lpSP->nScale))
		{
			SaveCustomError("SQLExecEx", "Parameter schema contained invalid data near '%20V', expected precision.", &pMatcher.CurrentView());
			throw E_APIERROR;
		}
		if (!pMatcher.MatchChar(')'))
		{
			SaveCustomError("SQLExecEx", "Parameter schema contained invalid data near '%20V', expected ')'.", &pMatcher.CurrentView());
			throw E_APIERROR;
		}
		return true;
	}
	return false;
}

bool SqlStatement::MatchLengthAndPrecision(CStringMatcher& pMatcher, SqlParameter* lpSP)
{
	if (pMatcher.MatchChar('('))
	{
		if (!pMatcher.MatchULong(&lpSP->nSize))
		{
			SaveCustomError("SQLExecEx", "Parameter schema contained invalid data near '%20V', expected precision.", &pMatcher.CurrentView());
			throw E_APIERROR;
		}
		if (pMatcher.MatchChar(','))
		{
			if (!pMatcher.MatchShort(&lpSP->nScale))
			{
				SaveCustomError("SQLExecEx", "Parameter schema contained invalid data near '%20V', expected scale.", &pMatcher.CurrentView());
				throw E_APIERROR;
			}
		}
		if (!pMatcher.MatchChar(')'))
		{
			SaveCustomError("SQLExecEx", "Parameter schema contained invalid data near '%20V', expected ')'.", &pMatcher.CurrentView());
			throw E_APIERROR;
		}
		return true;
	}
	return false;
}

void SqlStatement::ParseParameterSchema()
{
	SqlParameter* lpSP;
	int nParmNo, nTVPCol;
	CStringView pSqlType, pAttribute;
	CStringView pSchema, pCatalog, pTableType;
	if (!pParamSchema.Len())
		return;

	CStringMatcher pMatcher(pParamSchema);

	if (nFlags & SQLEXECEX_PARAMETER_ARRAY)
	{
		if (!pMatcher.MatchIdentifier(pParameterArray.pCursor))
		{
			SaveCustomError("SQLExecEx", "Parameter schema contained invalid data near '%20V', expected cursor name for parameter array.", &pMatcher.CurrentView());
			throw E_APIERROR;
		}
		if (pMatcher.End())
			return;

		do
		{
			if (!pMatcher.MatchInteger(&nParmNo))
			{
				SaveCustomError("SQLExecEx", "Parameter schema contained invalid data near '%20V', expected parameter number.", &pMatcher.CurrentView());
				throw E_APIERROR;
			}
			if (nParmNo < 1 || nParmNo > pParameters.GetCount())
			{
				SaveCustomError("SQLExecEx", "Parameter schema contained invalid data, parameter number '%I' out of bounds.", nParmNo);
				throw E_APIERROR;
			}

			lpSP = pParameters[nParmNo - 1];

			if (!pMatcher.MatchChars(VFP2C_VFP_DATATYPE_CHARACTERS, &lpSP->cVFPType))
			{
				SaveCustomError("SQLExecEx", "Datatype in parameterschema for parameter %U is invalid.", nParmNo);
				throw E_APIERROR;
			}

			lpSP->cVFPType = ToUpper(lpSP->cVFPType);
			switch (lpSP->cVFPType)
			{
				case 'C':
				case 'V':
				case 'Q':
					if (!MatchLength(pMatcher, lpSP))
					{
						SaveCustomError("SQLExecEx", "Parameter schema contained invalid data near '%20V', expected size of datatype.", &pMatcher.CurrentView());
						throw E_APIERROR;
					}
					break;
				case 'M':
					lpSP->bVFPBinary = pMatcher.MatchStringCaseInsensitive("NOCPTRANS");
					break;
				case 'N':
				case 'F':
					MatchLengthAndPrecision(pMatcher, lpSP);
					break;
			}
		} while (pMatcher.MatchChar(','));

		if (!pMatcher.End())
		{
			SaveCustomError("SQLExecEx", "Parameter schema contained invalid data near '%20V', expected end of schema.", &pMatcher.CurrentView());
			throw E_APIERROR;
		}
		return;
	}

	do
	{
		if (!pMatcher.MatchInteger(&nParmNo))
		{
			SaveCustomError("SQLExecEx", "Parameter schema contained invalid data near '%20V', expected parameter number.", &pMatcher.CurrentView());
			throw E_APIERROR;
		}
		if (nParmNo < 1 || nParmNo > pParameters.GetCount())
		{
			SaveCustomError("SQLExecEx", "Parameter schema contained invalid data, parameter number '%I' out of bounds.", nParmNo);
			throw E_APIERROR;
		}
		// set pointer to parameter number X
		lpSP = pParameters[nParmNo - 1];

		pMatcher.MatchQuotedIdentifier(lpSP->pParmName, VFP2C_ODBC_MAX_PARAMETER_NAME);

		if (pMatcher.MatchIdentifier(pSqlType))
		{
			if (pSqlType == "SQL_SS_TABLE")
			{
				lpSP->nSQLType = SQL_SS_TABLE;
				lpSP->nCType = SQL_C_DEFAULT;
				if (pMatcher.MatchQuotedIdentifier(lpSP->pParmTypeCatalog, VFP2C_ODBC_MAX_PARAMETER_NAME))
				{
					if (pMatcher.MatchChar('.'))
					{
						if (pMatcher.MatchQuotedIdentifier(lpSP->pParmTypeSchema, VFP2C_ODBC_MAX_PARAMETER_NAME))
						{
							if (pMatcher.MatchChar('.'))
							{
								if (!pMatcher.MatchQuotedIdentifier(lpSP->pParmTypeName, VFP2C_ODBC_MAX_PARAMETER_NAME))
								{
									SaveCustomError("SQLExecEx", "Parameter schema contained invalid data '%20V', expected quoted table valued type name.", &pMatcher.CurrentView());
									throw E_APIERROR;
								}
							}
							else
							{
								lpSP->pParmTypeName = lpSP->pParmTypeSchema;
								lpSP->pParmTypeSchema = lpSP->pParmTypeCatalog;
								lpSP->pParmTypeCatalog = 0;
							}
						}
					}
					else
					{
						lpSP->pParmTypeName = lpSP->pParmTypeCatalog;
						lpSP->pParmTypeCatalog = 0;
					}
				}
				else
				{
					SaveCustomError("SQLExecEx", "Parameter schema contained invalid data '%20V', expected quoted table valued type name.", &pMatcher.CurrentView());
					throw E_APIERROR;
				}
				{
					SqlTableType pType(lpSP->pParmTypeCatalog, lpSP->pParmTypeSchema, lpSP->pParmTypeName);
					lpSP->pTableType = g_RegisteredTableTypes.Lookup(pType.Key());
					if (!lpSP->pTableType)
					{
						SaveCustomError("SQLExecEx", "Table value type '%S' not registered.", pType.Key());
						throw E_APIERROR;
					}
					lpSP->pTableTypeParameters.Allocate(lpSP->pTableType->pColumns.GetCount());
				}
			}
			else if (pSqlType == "SQL_CHAR")
			{
				lpSP->nSQLType = SQL_CHAR;
				lpSP->nCType = SQL_C_CHAR;
			}
			else if (pSqlType == "SQL_VARCHAR")
			{
				lpSP->nSQLType = SQL_VARCHAR;
				lpSP->nCType = SQL_C_CHAR;
			}
			else if (pSqlType == "SQL_LONGVARCHAR")
			{
				lpSP->nSQLType = SQL_LONGVARCHAR;
				lpSP->nCType = SQL_C_CHAR;
			}
			else if (pSqlType == "SQL_WCHAR")
			{
				lpSP->nSQLType = SQL_WCHAR;
				lpSP->nCType = SQL_C_WCHAR;
			}
			else if (pSqlType == "SQL_WVARCHAR")
			{
				lpSP->nSQLType = SQL_WVARCHAR;
				lpSP->nCType = SQL_C_WCHAR;
			}
			else if (pSqlType == "SQL_WLONGVARCHAR")
			{
				lpSP->nSQLType = SQL_WLONGVARCHAR;
				lpSP->nCType = SQL_C_WCHAR;
			}
			else if (pSqlType == "SQL_BINARY")
			{
				lpSP->nSQLType = SQL_BINARY;
				lpSP->nCType = SQL_C_BINARY;
			}
			else if (pSqlType == "SQL_VARBINARY")
			{
				lpSP->nSQLType = SQL_VARBINARY;
				lpSP->nCType = SQL_C_BINARY;
			}
			else if (pSqlType == "SQL_LONGVARBINARY")
			{
				lpSP->nSQLType = SQL_LONGVARBINARY;
				lpSP->nCType = SQL_C_BINARY;
			}
			else if (pSqlType == "SQL_BIGINT")
			{
				lpSP->nSQLType = SQL_BIGINT;
				lpSP->nCType = SQL_C_CHAR;
			}
			else if (pSqlType == "SQL_INTEGER")
			{
				lpSP->nSQLType = SQL_INTEGER;
				lpSP->nCType = SQL_C_SLONG;
			}
			else if (pSqlType == "SQL_TINYINT")
			{
				lpSP->nSQLType = SQL_TINYINT;
				lpSP->nCType = SQL_C_SLONG;
			}
			else if (pSqlType == "SQL_SMALLINT")
			{
				lpSP->nSQLType = SQL_SMALLINT;
				lpSP->nCType = SQL_C_SLONG;
			}
			else if (pSqlType == "SQL_DOUBLE")
			{
				lpSP->nSQLType = SQL_DOUBLE;
				lpSP->nCType = SQL_C_DOUBLE;
			}
			else if (pSqlType == "SQL_FLOAT")
			{
				lpSP->nSQLType = SQL_FLOAT;
				lpSP->nCType = SQL_C_DOUBLE;
			}
			else if (pSqlType == "SQL_REAL")
			{
				lpSP->nSQLType = SQL_REAL;
				lpSP->nCType = SQL_C_DOUBLE;
			}
			else if (pSqlType == "SQL_NUMERIC")
			{
				lpSP->nSQLType = SQL_NUMERIC;
				lpSP->nCType = SQL_C_CHAR;
				MatchLengthAndPrecision(pMatcher, lpSP);
			}
			else if (pSqlType == "SQL_DECIMAL")
			{
				lpSP->nSQLType = SQL_DECIMAL;
				lpSP->nCType = SQL_C_CHAR;
				MatchLengthAndPrecision(pMatcher, lpSP);
			}
			else if (pSqlType == "SQL_BIT")
			{
				lpSP->nSQLType = SQL_BIT;
				lpSP->nCType = SQL_C_LONG;
			}
			else if (pSqlType == "SQL_DATE")
			{
				lpSP->nSQLType = SQL_DATE;
				lpSP->nCType = SQL_C_DATE;
			}
			else if (pSqlType == "SQL_TYPE_TIME")
			{
				lpSP->nSQLType = SQL_TYPE_TIME;
				lpSP->nCType = SQL_C_BINARY;
				lpSP->nBufferSize = sizeof(SQL_TIME_STRUCT);
			}
			else if (pSqlType == "SQL_INTERVAL")
			{
				lpSP->nSQLType = SQL_INTERVAL;
				MatchPrecision(pMatcher, lpSP);
				if (pMatcher.MatchIdentifier(pAttribute, 64))
				{
					if (pAttribute == "SQL_INTERVAL_DAY")
						lpSP->nCType = lpSP->nSQLTypeCode = SQL_INTERVAL_DAY;
					else if (pAttribute == "SQL_INTERVAL_DAY_TO_HOUR")
						lpSP->nCType = lpSP->nSQLTypeCode = SQL_INTERVAL_DAY_TO_HOUR;
					else if (pAttribute == "SQL_INTERVAL_DAY_TO_MINUTE")
						lpSP->nCType = lpSP->nSQLTypeCode = SQL_INTERVAL_DAY_TO_MINUTE;
					else if (pAttribute == "SQL_INTERVAL_DAY_TO_SECOND")
						lpSP->nCType = lpSP->nSQLTypeCode = SQL_INTERVAL_DAY_TO_SECOND;
					else if (pAttribute == "SQL_INTERVAL_HOUR")
						lpSP->nCType = lpSP->nSQLTypeCode = SQL_INTERVAL_HOUR;
					else if (pAttribute == "SQL_INTERVAL_HOUR_TO_MINUTE")
						lpSP->nCType = lpSP->nSQLTypeCode = SQL_INTERVAL_HOUR_TO_MINUTE;
					else if (pAttribute == "SQL_INTERVAL_HOUR_TO_SECOND")
						lpSP->nCType = lpSP->nSQLTypeCode = SQL_INTERVAL_HOUR_TO_SECOND;
					else if (pAttribute == "SQL_INTERVAL_MONTH")
						lpSP->nCType = lpSP->nSQLTypeCode = SQL_INTERVAL_MONTH;
					else if (pAttribute == "SQL_INTERVAL_SECOND")
						lpSP->nCType = lpSP->nSQLTypeCode = SQL_INTERVAL_SECOND;
					else if (pAttribute == "SQL_INTERVAL_YEAR")
						lpSP->nCType = lpSP->nSQLTypeCode = SQL_INTERVAL_YEAR;
					else if (pAttribute == "SQL_INTERVAL_YEAR_TO_MONTH")
						lpSP->nCType = lpSP->nSQLTypeCode = SQL_INTERVAL_YEAR_TO_MONTH;
					else
					{
						SaveCustomError("SQLExecEx", "Parameter schema contained invalid data '%V', invalid SQL_INTERVAL subcode.", &pAttribute);
						throw E_APIERROR;
					}
				}
			}
			else if (pSqlType == "SQL_DATETIME")
			{
				lpSP->nSQLType = SQL_DATETIME;
				lpSP->nSQLTypeCode = SQL_CODE_TIMESTAMP;
				lpSP->nCType = SQL_C_TIMESTAMP;
				MatchPrecision(pMatcher, lpSP);
			}
			else if (pSqlType == "SQL_TYPE_TIMESTAMP")
			{
				lpSP->nSQLType = SQL_TYPE_TIMESTAMP;
				lpSP->nCType = SQL_C_TYPE_TIMESTAMP;
				MatchPrecision(pMatcher, lpSP);
				lpSP->nSize = 20 + lpSP->nScale; // yyyy-dd-mm hh:mm:ss.fffffff 
				lpSP->nBufferSize = sizeof(SQL_TIMESTAMP_STRUCT);
			}
			else if (pSqlType == "SQL_TIMESTAMP")
			{
				lpSP->nSQLType = SQL_TIMESTAMP;
				lpSP->nCType = SQL_C_BINARY;
				lpSP->nBufferSize = sizeof(TIMESTAMP_STRUCT);
				MatchPrecision(pMatcher, lpSP);
				lpSP->nSize = 20 + lpSP->nScale; // yyyy-dd-mm hh:mm:ss.fffffff 
			}
			else if (pSqlType == "SQL_SS_VARIANT")
			{
				lpSP->nSQLType = SQL_SS_VARIANT;
				lpSP->nCType = SQL_C_BINARY;
			}
			else if (pSqlType == "SQL_SS_TIME2")
			{
				lpSP->nSQLType = SQL_SS_TIME2;
				lpSP->nCType = SQL_C_BINARY;
				lpSP->nBufferSize = sizeof(SQL_SS_TIME2_STRUCT);
				MatchPrecision(pMatcher, lpSP);
				lpSP->nSize = 9 + lpSP->nScale; // hh:mm:ss.fffffff 
			}
			else if (pSqlType == "SQL_SS_TIMESTAMPOFFSET")
			{
				lpSP->nSQLType = SQL_SS_TIMESTAMPOFFSET;
				lpSP->nCType = SQL_C_BINARY;
				MatchPrecision(pMatcher, lpSP);
				lpSP->nSize = 27 + lpSP->nScale; // yyyy-dd-mm hh:mm:ss.fffffff +-hh:mm 
			}
			else
			{
				SaveCustomError("SQLExecEx", "Parameter schema contained invalid data '%V', unknown SQL type.", &pSqlType);
				throw E_APIERROR;
			}

			lpSP->bCustomSchema = true;
		}
		else if (pMatcher.MatchInteger(&nTVPCol))
		{
			if (!lpSP->pTableType)
			{
				SaveCustomError("SQLExecEx", "Parameter schema invalid, parameter no. '%I' not defined as table valued parameter.", nParmNo);
				throw E_APIERROR;
			}

			if (nTVPCol < 1 || nTVPCol > lpSP->pTableTypeParameters.GetCount())
			{
				SaveCustomError("SQLExecEx", "Parameter schema invalid, column no. '%I' out range for table valued parameter '%I'", nTVPCol, nParmNo);
				throw E_APIERROR;
			}

			SqlTableTypeParameter* lpTVPCol = lpSP->pTableTypeParameters[nTVPCol - 1];

			if (!pMatcher.MatchIdentifier(pAttribute, 64))
			{
				SaveCustomError("SQLExecEx", "Parameter schema contained invalid data '%64V', expected column attribute for table value parameter '%I' column '%'", &pMatcher.CurrentView(), nParmNo, nTVPCol);
				throw E_APIERROR;
			}

			do
			{
				if (pAttribute == "SQL_DESC_AUTO_UNIQUE_VALUE")
				{
					lpTVPCol->nDescAutoUniqueValue = SQL_TRUE;
				}
				else if (pAttribute == "SQL_CA_SS_COLUMN_COMPUTED")
				{
					lpTVPCol->nColumnComputed = SQL_TRUE;
				}
				else if (pAttribute == "SQL_CA_SS_COLUMN_IN_UNIQUE_KEY")
				{
					lpTVPCol->nColumnInUniqueKey = SQL_TRUE;
				}
				else if (pAttribute == "SQL_CA_SS_COLUMN_SORT_ORDER")
				{
					if (!pMatcher.MatchIdentifier(pAttribute, 32))
					{
						SaveCustomError("SQLExecEx", "Parameter schema contained invalid data '%32V', expected sort order for table value parameter '%I' column '%'", &pMatcher.CurrentView(), nParmNo, nTVPCol);
						throw E_APIERROR;
					}
					if (pAttribute == "SQL_SS_ASCENDING_ORDER")
					{
						lpTVPCol->nColumnSortOrder = SQL_SS_ASCENDING_ORDER;
					}
					else if (pAttribute == "SQL_SS_DESCENDING_ORDER")
					{
						lpTVPCol->nColumnSortOrder = SQL_SS_DESCENDING_ORDER;
					}
					else
					{
						SaveCustomError("SQLExecEx", "Parameter schema invalid, unknown sort order '%V' for table value parameter '%I' column '%'", &pAttribute, nParmNo, nTVPCol);
						throw E_APIERROR;
					}
				}
				else if (pAttribute == "SQL_CA_SS_COLUMN_SORT_ORDINAL")
				{
					if (!pMatcher.MatchShort(&lpTVPCol->nColumnSortOrdinal))
					{
						SaveCustomError("SQLExecEx", "Parameter schema contained invalid data '%32V', expected sort ordinal for table value parameter '%I' column '%'", &pMatcher.CurrentView(), nParmNo, nTVPCol);
						throw E_APIERROR;
					}
				}
				else if (pAttribute == "SQL_CA_SS_COLUMN_HAS_DEFAULT_VALUE")
				{
					lpTVPCol->nColumnHasDefaultValue = SQL_TRUE;
				}
				else
				{
					SaveCustomError("SQLExecEx", "Parameter schema contained invalid table value parameter column attribute '%V' for parameter '%I' column '%I'", &pAttribute, nParmNo, nTVPCol);
					throw E_APIERROR;
				}

			} while (pMatcher.MatchIdentifier(pAttribute, 64));
		}
		// if no type identified and also no parameter name was specified the parameter schema is invalid
		else if (!lpSP->pParmName)
		{
			SaveCustomError("SQLExecEx", "Parameter schema contained invalid data near '%20V', expected SQL type.", &pMatcher.CurrentView());
			throw E_APIERROR;
		}

	} while (pMatcher.MatchChar(','));

	if (!pMatcher.End())
	{
		SaveCustomError("SQLExecEx", "Parameter schema contained invalid data near '%20V', expected end of schema.", &pMatcher.CurrentView());
		throw E_APIERROR;
	}
}

/*
void SqlStatement::SetCursorAdapterProperties()
{
	if (pCursorAdapters.Len() == 0)
		return;

	SQLRETURN nApiRet;
	SQLSMALLINT nStringLen;
	SQLLEN nNumericAttr;
	SqlResultSet* lpRS = pResultsets[nResultset - 1];
	SqlColumn* lpCS;
	FoxObject pCA, pAttachedCA;
	CStrBuilder<VFP_MAX_VARIABLE_NAME> pCAName;
	CStrBuilder<VFP2C_MAX_FUNCTIONBUFFER> pBuffer;
	FoxString pUpdateNameList, pUpdatableFieldList, pTimestampFieldList, pTables;
	pCAName = pCursorAdapters.GetWordNum(nResultset, ',');
	if (pCAName.Len() == 0)
		return;

	Evaluate(pCA, pCAName);

	pUpdateNameList.Size(lpRS->pColumns.GetCount() * (VFP_MAX_COLUMN_NAME + 1 + VFP2C_ODBC_MAX_COLUMN_NAME + 1 + VFP2C_ODBC_MAX_COLUMN_NAME));
	pUpdatableFieldList.Size(lpRS->pColumns.GetCount() * VFP_MAX_COLUMN_NAME);
	pTimestampFieldList.Size(VFP_MAX_COLUMN_NAME * 4);
	pTables.Size(VFP_MAX_COLUMN_NAME * 4);

	for (int xj = 1; xj <= lpRS->pColumns.GetCount(); xj++)
	{
		lpCS = lpRS->pColumns[xj - 1];
		if (lpCS->nSQLType == SQL_TIMESTAMP)
		{
			if (pTimestampFieldList.Len())
				pTimestampFieldList += ",";
			pTimestampFieldList += lpCS->pColumnName;
		}

		if (pUpdateNameList.Len())
			pUpdateNameList += ",";

		pUpdateNameList += lpCS->pColumnName;
		pUpdateNameList += " ";

		if (pUpdatableFieldList.Len())
			pUpdatableFieldList += ",";
		pUpdatableFieldList += lpCS->pColumnName;

		nApiRet = SQLColAttribute(hStmt, lpCS->nColNo, SQL_DESC_CATALOG_NAME, pBuffer.Ptr<SQLPOINTER>(), pBuffer.Size(), &nStringLen, 0);
		if (nApiRet == SQL_ERROR)
		{
			SaveODBCError("SQLColAttribute", hStmt);
			throw E_APIERROR;
		}
		else if (nApiRet == SQL_SUCCESS_WITH_INFO && bStoreInfoAsError)
			AddODBCError("SQLColAttribute", hStmt);

		if (nStringLen)
		{
			pBuffer.Len(nStringLen);
			pUpdateNameList += pBuffer;
			pUpdateNameList += ".";
		}

		nApiRet = SQLColAttribute(hStmt, lpCS->nColNo, SQL_DESC_SCHEMA_NAME, pBuffer.Ptr<SQLPOINTER>(), pBuffer.Size(), &nStringLen, 0);
		if (nApiRet == SQL_ERROR)
		{
			SaveODBCError("SQLColAttribute", hStmt);
			throw E_APIERROR;
		}
		else if (nApiRet == SQL_SUCCESS_WITH_INFO && bStoreInfoAsError)
			AddODBCError("SQLColAttribute", hStmt);

		if (nStringLen)
		{
			pBuffer.Len(nStringLen);
			pUpdateNameList += pBuffer;
			pUpdateNameList += ".";
		}

		nApiRet = SQLColAttribute(hStmt, lpCS->nColNo, SQL_DESC_TABLE_NAME, pBuffer.Ptr<SQLPOINTER>(), pBuffer.Size(), &nStringLen, 0);
		if (nApiRet == SQL_ERROR)
		{
			SaveODBCError("SQLColAttribute", hStmt);
			throw E_APIERROR;
		}
		else if (nApiRet == SQL_SUCCESS_WITH_INFO && bStoreInfoAsError)
			AddODBCError("SQLColAttribute", hStmt);

		if (nStringLen)
		{
			pBuffer.Len(nStringLen);
			pUpdateNameList += pBuffer;
			pUpdateNameList += ".";

			CStringView pCurrentTable = pBuffer;
			CStringView pTable;
			bool bFound = false;
			for (int xi = 0; xi < pTables.GetWordCount(','); xi++)
			{
				pTable = pTables.GetWordNum(xi + 1, ',');
				if (pCurrentTable == pTable)
				{
					bFound = true;
					break;
				}
			}

			if (!bFound)
			{
				if (pTables.Len() > 0)
					pTables += ";";

				pTables += pCurrentTable;
			}
		}

		nApiRet = SQLColAttribute(hStmt, lpCS->nColNo, SQL_DESC_BASE_COLUMN_NAME, pBuffer.Ptr<SQLPOINTER>(), pBuffer.Size(), &nStringLen, 0);
		if (nApiRet == SQL_ERROR)
		{
			SaveODBCError("SQLColAttribute", hStmt);
			throw E_APIERROR;
		}
		else if (nApiRet == SQL_SUCCESS_WITH_INFO && bStoreInfoAsError)
			AddODBCError("SQLColAttribute", hStmt);

		if (nStringLen)
		{
			pBuffer.Len(nStringLen);
			pUpdateNameList += pBuffer;
			pUpdateNameList += ".";
		}
	}

	pBuffer.Format("GETCURSORADAPTER('%V')", &lpRS->pCursorName.StringView());
	int nErrorNo = _Evaluate(pAttachedCA, pBuffer);
	if (nErrorNo != 0 || pAttachedCA.Vartype() != 'O' || pAttachedCA->ev_object != pCA->ev_object)
	{
		pBuffer.Format("%V.CursorAttach('%V',.F.)", &pCAName.StringView(), &lpRS->pCursorName.StringView());
		Execute(pBuffer);
	}

	pCA("Tables") << pTables;
	pCA("UpdateNameList") << pUpdateNameList;
	pCA("UpdatableFieldList ") << pUpdatableFieldList;
	pCA("TimestampFieldList") << pTimestampFieldList;
}
*/

void SqlStatement::EvaluateParameters()
{
	SqlParameter* lpSP;
	SqlVariant* pVariant;
	int nErrorNo, xj;
	char cType;

	for (xj = 0; xj < pParameters.GetCount(); xj++)
	{
		lpSP = pParameters[xj];
		if (lpSP->nParmType > SQL_PARAM_INPUT)
		{
			if (nErrorNo = FindFoxVarOrFieldEx(lpSP->aParmExpr, &lpSP->lVarOrField))
				throw nErrorNo;

			bOutputParams = true;
		}

		Evaluate(lpSP->vParmValue, lpSP->aParmExpr);
		cType = lpSP->vParmValue.Vartype();

		switch (cType)
		{
		case 'C':
			if (lpSP->nParmType == SQL_PARAM_INPUT_OUTPUT || lpSP->nParmType == SQL_PARAM_OUTPUT)
			{
				if (!lpSP->bCustomSchema)
				{
					lpSP->nBufferSize = VFP2C_SQLSERVER_MAX_VARCHAR_LEN + (!lpSP->vParmValue.Binary() ? 1L : 0L);
					if (lpSP->vParmValue.Len() < (unsigned int)lpSP->nBufferSize)
					{
						if (!lpSP->vParmValue.SetHandleSize(lpSP->nBufferSize))
							throw E_INSUFMEMORY;
					}
					else
						lpSP->nBufferSize = lpSP->vParmValue.Len();

					lpSP->vParmValue.LockHandle();
					lpSP->pParmPtr = lpSP->vParmValue.HandleToPtr();
					lpSP->nSize = lpSP->vParmValue.Len();
					lpSP->nIndicator = SQL_LEN_DATA_AT_EXEC((int)lpSP->nSize);
					lpSP->bPutData = true;
					if (!lpSP->vParmValue.Binary())
					{
						lpSP->nCType = SQL_C_CHAR;
						lpSP->nSQLType = SQL_VARCHAR;
					}
					else
					{
						lpSP->nCType = SQL_C_BINARY;
						lpSP->nSQLType = SQL_VARBINARY;
					}
				}
				else
				{
					switch (lpSP->nSQLType)
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
						if (lpSP->vParmValue.Len() < VFP2C_SQLSERVER_MAX_VARCHAR_LEN + 2)
						{
							if (!lpSP->vParmValue.SetHandleSize(VFP2C_SQLSERVER_MAX_VARCHAR_LEN + 2))
								throw E_INSUFMEMORY;
							lpSP->nBufferSize = VFP2C_SQLSERVER_MAX_VARCHAR_LEN + 2;
						}
						else
							lpSP->nBufferSize = lpSP->vParmValue.Len();

						lpSP->vParmValue.LockHandle();
						lpSP->pParmPtr = lpSP->vParmValue.HandleToPtr();
						lpSP->nSize = lpSP->vParmValue.Len();
						lpSP->nIndicator = SQL_LEN_DATA_AT_EXEC((int)lpSP->nSize);
						lpSP->bPutData = true;
						if (lpSP->nSQLType == SQL_CHAR || lpSP->nSQLType == SQL_VARCHAR || lpSP->nSQLType == SQL_LONGVARCHAR)
						{
							lpSP->nCType = SQL_C_CHAR;
						}
						else if (lpSP->nSQLType == SQL_BINARY || lpSP->nSQLType == SQL_VARBINARY || lpSP->nSQLType == SQL_LONGVARBINARY)
						{
							lpSP->nCType = SQL_C_BINARY;
						}
						else
						{
							lpSP->nCType = SQL_C_WCHAR;
						}
						break;
					case SQL_TIMESTAMP:
						if (lpSP->vParmValue.Len() < 32)
						{
							if (!lpSP->vParmValue.SetHandleSize(32))
								throw E_INSUFMEMORY;
							lpSP->nBufferSize = 32;
						}
						else
							lpSP->nBufferSize = lpSP->vParmValue.Len();

						lpSP->nSize = lpSP->vParmValue.Len();
						lpSP->vParmValue.LockHandle();
						lpSP->pParmPtr = lpSP->vParmValue.HandleToPtr();
						lpSP->nCType = SQL_C_CHAR;
						break;
					case SQL_SS_VARIANT:
						break; // TODO
					default:
						SaveCustomError("SQLExecEx", "Invalid datatype conversion specified for parameter '%S'.", lpSP->aParmExpr);
						throw E_APIERROR;
					}
				}
			}
			else if (lpSP->nParmType == SQL_PARAM_INPUT_OUTPUT_STREAM || lpSP->nParmType == SQL_PARAM_OUTPUT_STREAM)
			{
				lpSP->bPutData = true;
				if (lpSP->nParmType == SQL_PARAM_INPUT_OUTPUT_STREAM)
				{
					lpSP->vParmValue.LockHandle();
					lpSP->pParmPtr = lpSP->vParmValue.HandleToPtr();
					lpSP->nSize = lpSP->vParmValue.Len();
					lpSP->nIndicator = SQL_LEN_DATA_AT_EXEC((int)lpSP->nSize);
				}

				if (!lpSP->bCustomSchema)
				{
					if (!lpSP->vParmValue.Binary())
					{
						lpSP->nCType = SQL_C_CHAR;
						lpSP->nSQLType = SQL_VARCHAR;
					}
					else
					{
						lpSP->nCType = SQL_C_BINARY;
						lpSP->nSQLType = SQL_VARBINARY;
					}
				}
				else
				{
					switch (lpSP->nSQLType)
					{
					case SQL_CHAR:
					case SQL_VARCHAR:
					case SQL_LONGVARCHAR:
						lpSP->nCType = SQL_C_CHAR;
						lpSP->nSQLType = SQL_VARCHAR;
						break;
					case SQL_WCHAR:
					case SQL_WVARCHAR:
					case SQL_WLONGVARCHAR:
						lpSP->nCType = SQL_C_WCHAR;
						lpSP->nSQLType = SQL_WVARCHAR;
						break;
					case SQL_BINARY:
					case SQL_VARBINARY:
					case SQL_LONGVARBINARY:
						lpSP->nCType = SQL_C_BINARY;
						lpSP->nSQLType = SQL_VARBINARY;
						break;
					default:
						SaveCustomError("SQLExecEx", "Invalid datatype conversion specified for parameter '%S'.", lpSP->aParmExpr);
						throw E_APIERROR;
					}
				}
			}
			else
			{
				if (lpSP->nSQLType == SQL_SS_TABLE)
				{
					lpSP->pParmPtr = lpSP->pParmTypeName.Data;
					lpSP->nBufferSize = lpSP->pParmTypeName.Len;
					lpSP->vParmValue.NullTerminate();
					lpSP->vParmValue.LockHandle();
					lpSP->nWorkArea = Select(lpSP->vParmValue.HandleToPtr());
					// lpSP->nSize = 1; // no. of rows
					if (lpSP->nWorkArea == 0)
					{
						SaveCustomError("SQLExecEx", "Cursor '%S', passed as table value parameter, does not exist.", lpSP->vParmValue.HandleToPtr());
						throw E_APIERROR;
					}
					lpSP->nSize = RecCount(lpSP->nWorkArea);
					lpSP->nIndicator = Bof(lpSP->nWorkArea) && Eof(lpSP->nWorkArea) ? SQL_DEFAULT_PARAM : SQL_DATA_AT_EXEC; // variable row binding
					break;
				}

				lpSP->vParmValue.LockHandle();
				lpSP->pParmPtr = lpSP->vParmValue.HandleToPtr();
				if (!lpSP->bCustomSchema)
				{
					lpSP->nSize = lpSP->nBufferSize = lpSP->vParmValue.Len();
					if (lpSP->nSize < VFP2C_SQLSERVER_MAX_VARCHAR_LEN)
					{
						lpSP->nIndicator = lpSP->nSize;
					}
					else
					{
						lpSP->nIndicator = SQL_LEN_DATA_AT_EXEC((int)lpSP->nSize);
						lpSP->bPutData = true;
					}
					lpSP->nCType = lpSP->vParmValue.Binary() ? SQL_C_BINARY : SQL_C_CHAR;
					if (lpSP->nSize < VFP2C_ODBC_MAX_CHAR_LEN)
						lpSP->nSQLType = lpSP->vParmValue.Binary() ? SQL_BINARY : SQL_CHAR;
					else if (lpSP->nSize <= VFP2C_SQLSERVER_MAX_VARCHAR_LEN)
						lpSP->nSQLType = lpSP->vParmValue.Binary() ? SQL_VARBINARY : SQL_VARCHAR;
					else
						lpSP->nSQLType = lpSP->vParmValue.Binary() ? SQL_LONGVARBINARY : SQL_LONGVARCHAR;
				}
				else
				{
					switch (lpSP->nSQLType)
					{
						case SQL_WCHAR:
						case SQL_WVARCHAR:
						case SQL_WLONGVARCHAR:
							lpSP->nCType = SQL_C_WCHAR;
							break;
						case SQL_CHAR:
						case SQL_VARCHAR:
						case SQL_LONGVARCHAR:
							lpSP->nCType = SQL_C_CHAR;
							break;
						case SQL_BINARY:
						case SQL_VARBINARY:
						case SQL_LONGVARBINARY:
							lpSP->nCType = SQL_C_BINARY;
							break;
						case SQL_TIMESTAMP:
						case SQL_TYPE_TIMESTAMP:
							StringToTimestamp_Struct((char*)lpSP->pParmPtr, lpSP->vParmValue.Len(), lpSP->nScale, &lpSP->sTimestampStruct);
							lpSP->pParmPtr = &lpSP->sTimestampStruct;
							break;
						case SQL_SS_VARIANT:
							pVariant = (SqlVariant*)lpSP->pParmPtr;
							lpSP->nCType = pVariant->nType;
							lpSP->nSize = pVariant->nSize;
							lpSP->nScale = pVariant->nScale;
							lpSP->pParmPtr = &pVariant->pData;
							break;
						default:
							SaveCustomError("SQLExecEx", "Invalid datatype conversion specified for parameter '%S'.", lpSP->aParmExpr);
							throw E_APIERROR;
					}

					if (lpSP->nSize == 0)
					{
						lpSP->nSize = lpSP->nBufferSize = lpSP->vParmValue.Len();
						if (lpSP->nSize < VFP2C_SQLSERVER_MAX_VARCHAR_LEN)
						{
							lpSP->nIndicator = lpSP->nSize;
						}
						else
						{
							lpSP->nIndicator = SQL_LEN_DATA_AT_EXEC((int)lpSP->nSize);
							lpSP->bPutData = true;
						}
					}
				}
			}
			break;

		case 'M':
		case 'W':
		case 'G':
			if (lpSP->nParmType <= SQL_PARAM_OUTPUT)
			{
				lpSP->bPutData = true;
				lpSP->nSize = lpSP->nBufferSize = lpSP->vParmValue.ev_long;
				lpSP->nIndicator = SQL_LEN_DATA_AT_EXEC((int)lpSP->nSize);
				if (!lpSP->bCustomSchema)
				{
					// lpSP->vParmValue.ev_length contains either the codepage of the memo/blob or 0 if it's a blob/NOCPTRANS field
					if (lpSP->vParmValue.Vartype() == 'M' && lpSP->vParmValue.ev_length != 0)
					{
						lpSP->nCType = SQL_C_CHAR;
						lpSP->nSQLType = SQL_LONGVARCHAR;
					}
					else
					{
						lpSP->nCType = SQL_C_BINARY;
						lpSP->nSQLType = SQL_LONGVARBINARY;
					}
				}
				else
				{
					switch (lpSP->nSQLType)
					{
						case SQL_WCHAR:
						case SQL_WVARCHAR:
						case SQL_WLONGVARCHAR:
							if (lpSP->nSQLType == SQL_WCHAR)
							{
								if (lpSP->nSize > VFP2C_SQLSERVER_MAX_VARCHAR_LEN)
									lpSP->nSQLType = SQL_WLONGVARCHAR;
								else if (lpSP->nSize >= VFP2C_ODBC_MAX_CHAR_LEN)
									lpSP->nSQLType = SQL_WVARCHAR;
							}
							lpSP->nCType = SQL_C_WCHAR;
							break;
						case SQL_BINARY:
						case SQL_VARBINARY:
						case SQL_LONGVARBINARY:
							if (lpSP->nSQLType == SQL_BINARY)
							{
								if (lpSP->nSize > VFP2C_SQLSERVER_MAX_VARCHAR_LEN)
									lpSP->nSQLType = SQL_LONGVARBINARY;
								else if (lpSP->nSize >= VFP2C_ODBC_MAX_CHAR_LEN)
									lpSP->nSQLType = SQL_VARBINARY;
							}
							lpSP->nCType = SQL_BINARY;
							break;
						case SQL_CHAR:
						case SQL_VARCHAR:
						case SQL_LONGVARCHAR:
							if (lpSP->nSQLType == SQL_BINARY)
							{
								if (lpSP->nSize > VFP2C_SQLSERVER_MAX_VARCHAR_LEN)
									lpSP->nSQLType = SQL_LONGVARCHAR;
								else if (lpSP->nSize >= VFP2C_ODBC_MAX_CHAR_LEN)
									lpSP->nSQLType = SQL_VARCHAR;
							}
							lpSP->nCType = SQL_C_CHAR;
							break;
						case SQL_SS_VARIANT:
							lpSP->bPutData = false;
							lpSP->nIndicator = 0;
							lpSP->nBufferSize = lpSP->nParmType == SQL_PARAM_INPUT ? lpSP->nSize : VFP2C_ODBC_MAX_VARIABLE_BUFFER;
							lpSP->vMemoValue.SetString();
							if (!lpSP->vMemoValue.AllocHandle(lpSP->nBufferSize))
								throw E_INSUFMEMORY;
							pVariant = lpSP->vMemoValue.HandleToPtr<SqlVariant*>();
							nErrorNo = GetMemoContent(lpSP->vParmValue, (char*)pVariant);
							if (nErrorNo)
								throw nErrorNo;
							switch (pVariant->nType)
							{
								case SQL_C_BINARY:
									if (pVariant->nSize < VFP2C_ODBC_MAX_CHAR_LEN)
									{
										 lpSP->nSQLType = lpSP->nCType = pVariant->nType;
									}
									else
									{
										lpSP->nSQLType = SQL_VARBINARY;
										lpSP->nCType = pVariant->nType;
									}
									lpSP->nIndicator = lpSP->nSize = pVariant->nSize;
									break;
								case SQL_C_CHAR:
									if (pVariant->nSize < VFP2C_ODBC_MAX_CHAR_LEN)
									{
										lpSP->nSQLType = lpSP->nCType = pVariant->nType;
									}
									else
									{
										lpSP->nSQLType = SQL_VARCHAR;
										lpSP->nCType = pVariant->nType;
									}
									lpSP->nIndicator = lpSP->nSize = pVariant->nSize;
									break;
								case SQL_C_SLONG:
									lpSP->nSQLType = SQL_INTEGER;
									lpSP->nCType = pVariant->nType;
									break;
								case SQL_C_SBIGINT:
									lpSP->nSQLType = SQL_BIGINT;
									lpSP->nCType = SQL_C_CHAR;
									lpSP->nIndicator = lpSP->nSize - sizeof(SqlVariant);
									lpSP->nSize -= sizeof(SqlVariant);
									break;
								case SQL_C_NUMERIC:
									lpSP->nSQLType = SQL_NUMERIC;
									if (pVariant->nSize > 20 || pVariant->nScale > 16)
									{
										lpSP->nCType = SQL_C_CHAR;
										lpSP->nIndicator = lpSP->nSize - sizeof(SqlVariant);
										lpSP->nSize = pVariant->nSize;
										lpSP->nScale = pVariant->nScale;
									}
									else
									{
										lpSP->nCType = SQL_C_DOUBLE;
										lpSP->nSize = pVariant->nSize;
										lpSP->nScale = pVariant->nScale;
									}
									break;
								case SQL_C_MONEY:
									lpSP->nSQLType = lpSP->nCType = SQL_NUMERIC;
									lpSP->nIndicator = sizeof(SQL_NUMERIC_STRUCT);
									lpSP->nSize = VFP2C_VFP_CURRENCY_PRECISION;
									lpSP->nScale = VFP2C_VFP_CURRENCY_SCALE;
									lpSP->cVFPType = 'Y';
									// lpSP->bMoney = true;
									break;
								case SQL_C_TIMESTAMP:
									if (pVariant->nScale)
									{
										lpSP->nSQLType = SQL_TIMESTAMP;
										lpSP->nCType = SQL_C_CHAR;
										lpSP->nIndicator = lpSP->nSize - sizeof(SqlVariant);
										lpSP->nScale = pVariant->nScale;
									}
									else
									{
										lpSP->nSQLType = lpSP->nCType = pVariant->nType;
										lpSP->nScale = pVariant->nScale;
									}
									break;
								case SQL_C_SS_TIME2:
									lpSP->nSQLType = SQL_SS_TIME2;
									lpSP->nScale = pVariant->nScale;
									break;
								case SQL_C_SS_TIMESTAMPOFFSET:
									lpSP->nSQLType = SQL_SS_TIMESTAMPOFFSET;
									lpSP->nCType = SQL_C_CHAR;
									lpSP->nIndicator = lpSP->nSize - sizeof(SqlVariant);
									lpSP->nScale = pVariant->nScale;
									break;
								
								default:
									lpSP->nSQLType = lpSP->nCType = pVariant->nType;
									lpSP->nIndicator = lpSP->nSize = pVariant->nSize;
									lpSP->nScale = pVariant->nScale;
							}
							lpSP->pParmPtr = &pVariant->pData;
							break;
						default:
							SaveCustomError("SQLExecEx", "Invalid datatype conversion specified for parameter '%S'.", lpSP->aParmExpr);
							throw E_APIERROR;
					}
				}
			}
			else if (lpSP->nParmType == SQL_PARAM_INPUT_OUTPUT_STREAM || lpSP->nParmType == SQL_PARAM_OUTPUT_STREAM)
			{
				lpSP->bPutData = true;
				if (lpSP->nParmType == SQL_PARAM_INPUT_OUTPUT_STREAM)
				{
					lpSP->nSize = lpSP->vParmValue.ev_long;
					lpSP->nIndicator = SQL_LEN_DATA_AT_EXEC((int)lpSP->nSize);
				}

				if (!lpSP->bCustomSchema)
				{
					if (lpSP->vParmValue.Vartype() == 'M' && lpSP->vParmValue.ev_length != 0)
					{
						lpSP->nCType = SQL_C_CHAR;
						lpSP->nSQLType = SQL_VARCHAR;
					}
					else
					{
						lpSP->nCType = SQL_C_BINARY;
						lpSP->nSQLType = SQL_VARBINARY;
					}
				}
				else
				{
					switch (lpSP->nSQLType)
					{
					case SQL_BINARY:
					case SQL_VARBINARY:
					case SQL_LONGVARBINARY:
						lpSP->nCType = SQL_C_BINARY;
						lpSP->nSQLType = SQL_VARBINARY;
						break;
					case SQL_CHAR:
					case SQL_VARCHAR:
					case SQL_LONGVARCHAR:
						lpSP->nCType = SQL_C_CHAR;
						lpSP->nSQLType = SQL_VARCHAR;
						break;
					case SQL_WCHAR:
					case SQL_WVARCHAR:
					case SQL_WLONGVARCHAR:
						lpSP->nCType = SQL_C_WCHAR;
						lpSP->nSQLType = SQL_WVARCHAR;
						break;
					default:
						SaveCustomError("SQLExecEx", "Invalid datatype conversion specified for parameter '%S'.", lpSP->aParmExpr);
						throw E_APIERROR;
					}
				}
			}
			break;

		case 'N':
			lpSP->pParmPtr = &lpSP->vParmValue.ev_real;
			lpSP->nCType = SQL_C_DOUBLE;
			if (!lpSP->bCustomSchema)
				lpSP->nSQLType = SQL_DOUBLE;
			break;

		case 'I':
			lpSP->pParmPtr = &lpSP->vParmValue.ev_long;
			lpSP->nCType = SQL_C_SLONG;
			if (!lpSP->bCustomSchema)
				lpSP->nSQLType = SQL_INTEGER;
			break;

		case 'T':
			if (!lpSP->bCustomSchema)
			{
				if (lpSP->vParmValue.ev_real == 0.0)
					lpSP->nIndicator = SQL_NULL_DATA;
				else
					ValueToTimestamp_Struct(lpSP->vParmValue, &lpSP->sTimestampStruct);

				lpSP->pParmPtr = &lpSP->sTimestampStruct;
				lpSP->nCType = SQL_C_TIMESTAMP;
				lpSP->nSQLType = SQL_TIMESTAMP;
				lpSP->nSize = lpSP->nBufferSize = sizeof(lpSP->sTimestampStruct);
			}
			else
			{
				if (lpSP->nSQLType != SQL_DATE && lpSP->nSQLType != SQL_TIMESTAMP)
				{
					SaveCustomError("SQLExecEx", "Invalid datatype conversion specified for parameter '%S'.", lpSP->aParmExpr);
					throw E_APIERROR;
				}

				if (lpSP->vParmValue.ev_real == 0.0)
				{
					lpSP->nIndicator = SQL_NULL_DATA;
					lpSP->nCType = lpSP->nSQLType == SQL_TIMESTAMP ? SQL_C_TIMESTAMP : SQL_C_DATE;
				}
				else
				{
					if (lpSP->nSQLType == SQL_TIMESTAMP)
					{
						ValueToTimestamp_Struct(lpSP->vParmValue, &lpSP->sTimestampStruct);
						lpSP->pParmPtr = &lpSP->sTimestampStruct;
						lpSP->nCType = SQL_C_TIMESTAMP;
						lpSP->nSize = lpSP->nBufferSize = sizeof(lpSP->sTimestampStruct);
					}
					else
					{
						ValueToDate_Struct(lpSP->vParmValue, &lpSP->sDateStruct);
						lpSP->pParmPtr = &lpSP->sDateStruct;
						lpSP->nCType = SQL_C_DATE;
						lpSP->nSize = lpSP->nBufferSize = sizeof(lpSP->sDateStruct);
					}
				}
			}
			break;

		case 'D':
			if (!lpSP->bCustomSchema)
			{
				if (lpSP->vParmValue.ev_real == 0.0)
					lpSP->nIndicator = SQL_NULL_DATA;
				else
					ValueToDate_Struct(lpSP->vParmValue, &lpSP->sDateStruct);

				lpSP->pParmPtr = &lpSP->sDateStruct;
				lpSP->nCType = SQL_C_DATE;
				lpSP->nSQLType = SQL_DATE;
				lpSP->nSize = lpSP->nBufferSize = sizeof(lpSP->sDateStruct);
			}
			else
			{
				if (lpSP->nSQLType != SQL_DATE && lpSP->nSQLType != SQL_TIMESTAMP)
				{
					SaveCustomError("SQLExecEx", "Invalid datatype conversion specified for parameter '%S'.", lpSP->aParmExpr);
					throw E_APIERROR;
				}

				if (lpSP->vParmValue.ev_real == 0.0)
					lpSP->nIndicator = SQL_NULL_DATA;

				if (lpSP->nSQLType == SQL_TIMESTAMP)
				{
					ValueToTimestamp_Struct(lpSP->vParmValue, &lpSP->sTimestampStruct);
					lpSP->pParmPtr = &lpSP->sTimestampStruct;
					lpSP->nCType = SQL_C_TIMESTAMP;
					lpSP->nSize = lpSP->nBufferSize = sizeof(lpSP->sTimestampStruct);
				}
				else
				{
					ValueToDate_Struct(lpSP->vParmValue, &lpSP->sDateStruct);
					lpSP->pParmPtr = &lpSP->sDateStruct;
					lpSP->nCType = SQL_C_DATE;
					lpSP->nSize = lpSP->nBufferSize = sizeof(lpSP->sDateStruct);
				}
			}
			break;

		case 'L':
			lpSP->pParmPtr = &lpSP->vParmValue.ev_length;
			lpSP->nCType = SQL_C_SLONG;
			lpSP->nSQLType = SQL_BIT;
			break;

		case 'Y':
			ValueToNumeric_Struct(lpSP->vParmValue, &lpSP->sNumericStruct);
			lpSP->pParmPtr = &lpSP->sNumericStruct;
			lpSP->nBufferSize = sizeof(SQL_NUMERIC_STRUCT);
			lpSP->nCType = SQL_C_NUMERIC;
			lpSP->nSQLType = SQL_NUMERIC;
			lpSP->nSize = VFP2C_VFP_CURRENCY_PRECISION;
			lpSP->nScale = VFP2C_VFP_CURRENCY_SCALE;
			break;

		case '0':
			lpSP->nIndicator = SQL_NULL_DATA;
			cType = (char)lpSP->vParmValue.ev_long;
			// set default if not specified in parameter schema
			if (lpSP->nParmType == SQL_PARAM_INPUT_OUTPUT || lpSP->nParmType == SQL_PARAM_OUTPUT)
			{
				if (!lpSP->bCustomSchema)
				{
					switch (cType)
					{
					case 'C':
						lpSP->nCType = SQL_C_CHAR;
						lpSP->nSQLType = SQL_CHAR;
						lpSP->nBufferSize = VFP_MAX_CHARCOLUMN;
						lpSP->vParmValue.SetString();
						if (!lpSP->vParmValue.AllocHandle(lpSP->nSize))
							throw E_INSUFMEMORY;
						lpSP->pParmPtr = lpSP->vParmValue.HandleToPtr();
						break;
					case 'V':
						lpSP->nCType = SQL_C_CHAR;
						lpSP->nSQLType = SQL_VARCHAR;
						lpSP->nBufferSize = VFP_MAX_CHARCOLUMN;
						lpSP->vParmValue.SetString();
						if (!lpSP->vParmValue.AllocHandle(lpSP->nSize))
							throw E_INSUFMEMORY;
						lpSP->pParmPtr = lpSP->vParmValue.HandleToPtr();
						break;
					case 'Q':
						lpSP->nCType = SQL_C_BINARY;
						lpSP->nSQLType = SQL_VARBINARY;
						lpSP->nBufferSize = VFP_MAX_CHARCOLUMN;
						lpSP->vParmValue.SetString();
						if (!lpSP->vParmValue.AllocHandle(lpSP->nSize))
							throw E_INSUFMEMORY;
						lpSP->pParmPtr = lpSP->vParmValue.HandleToPtr();
						break;
					case 'M':
						lpSP->nCType = SQL_C_CHAR;
						lpSP->nSQLType = SQL_LONGVARCHAR;
						lpSP->bPutData = true;
						break;
					case 'W':
					case 'G':
						lpSP->nCType = SQL_C_BINARY;
						lpSP->nSQLType = SQL_LONGVARBINARY;
						lpSP->bPutData = true;
						break;
					case 'N':
						lpSP->nCType = SQL_C_DOUBLE;
						lpSP->nSQLType = SQL_DOUBLE;
						lpSP->vParmValue.SetDouble();
						lpSP->pParmPtr = &lpSP->vParmValue.ev_real;
						break;
					case 'I':
						lpSP->nCType = SQL_C_SLONG;
						lpSP->nSQLType = SQL_INTEGER;
						lpSP->vParmValue.SetInt();
						lpSP->pParmPtr = &lpSP->vParmValue.ev_long;
						break;
					case 'T':
						lpSP->nCType = SQL_C_TIMESTAMP;
						lpSP->nSQLType = SQL_TIMESTAMP;
						break;
					case 'D':
						lpSP->nCType = SQL_C_DATE;
						lpSP->nSQLType = SQL_DATE;
						break;
					case 'L':
						lpSP->nCType = SQL_C_BIT;
						lpSP->nSQLType = SQL_BIT;
						break;
					case 'Y':
						lpSP->pParmPtr = &lpSP->sNumericStruct;
						lpSP->nCType = SQL_C_NUMERIC;
						lpSP->nSQLType = SQL_NUMERIC;
						break;
					}
				}
				else
				{
					switch (lpSP->nSQLType)
					{
					case SQL_BIT:
						lpSP->nCType = SQL_C_BIT;
						break;
					case SQL_VARBINARY:
						lpSP->nCType = SQL_C_BINARY;
						break;
					case SQL_LONGVARCHAR:
						lpSP->nCType = SQL_C_CHAR;
						break;
					case SQL_CHAR:
						lpSP->nCType = SQL_C_CHAR;
						break;
					case SQL_DECIMAL:
						lpSP->nCType = SQL_C_CHAR;
						break;
					case SQL_INTEGER:
						lpSP->nCType = SQL_C_SLONG;
						break;
					case SQL_DATE:
						lpSP->nCType = SQL_C_DATE;
						break;
					case SQL_TIMESTAMP:
						lpSP->nCType = SQL_C_TIMESTAMP;
						break;
					case SQL_VARCHAR:
						lpSP->nCType = SQL_C_CHAR;
						break;
					case SQL_DOUBLE:
						lpSP->nCType = SQL_C_DOUBLE;
						break;
					case SQL_GUID:
						lpSP->nCType = SQL_C_CHAR;
						break;
					case SQL_SS_VARIANT:
						lpSP->nCType = SQL_C_BINARY;
						break;
					}
				}
			}
			else if (lpSP->nParmType == SQL_PARAM_INPUT_OUTPUT_STREAM || lpSP->nParmType == SQL_PARAM_OUTPUT_STREAM)
			{
				lpSP->bPutData = true;
				if (!lpSP->bCustomSchema)
				{
					switch (cType)
					{
					case 'C':
					case 'V':
					case 'M':
						lpSP->nCType = SQL_C_CHAR;
						lpSP->nSQLType = SQL_VARCHAR;
						break;
					case 'Q':
					case 'W':
					case 'G':
						lpSP->nCType = SQL_C_BINARY;
						lpSP->nSQLType = SQL_VARBINARY;
						break;
					default:
						SaveCustomError("SQLExecEx", "Invalid datatype for streaming parameter '%S'.", lpSP->aParmExpr);
						throw E_APIERROR;
					}
				}
				else
				{
					switch (lpSP->nSQLType)
					{
					case SQL_CHAR:
					case SQL_VARCHAR:
						lpSP->nCType = SQL_C_CHAR;
						lpSP->nSQLType = SQL_VARCHAR;
						break;
					case SQL_BINARY:
					case SQL_VARBINARY:
						lpSP->nCType = SQL_C_BINARY;
						lpSP->nSQLType = SQL_VARBINARY;
						break;
					case SQL_WVARCHAR:
					case SQL_WCHAR:
						lpSP->nCType = SQL_C_WCHAR;
						lpSP->nSQLType = SQL_WVARCHAR;
						break;
					default:
						SaveCustomError("SQLExecEx", "Invalid datatype for streaming parameter '%S'.", lpSP->aParmExpr);
						throw E_APIERROR;
					}
				}
			}
			else // SQL_PARAM_INPUT
			{
				if (!lpSP->bCustomSchema)
				{
					switch (cType)
					{
					case 'C':
						lpSP->nCType = SQL_C_CHAR;
						lpSP->nSQLType = SQL_CHAR;
						break;
					case 'V':
						lpSP->nCType = SQL_C_CHAR;
						lpSP->nSQLType = SQL_VARCHAR;
						break;
					case 'Q':
						lpSP->nCType = SQL_C_BINARY;
						lpSP->nSQLType = SQL_VARBINARY;
						break;
					case 'M':
						lpSP->nCType = SQL_C_CHAR;
						lpSP->nSQLType = SQL_LONGVARCHAR;
						break;
					case 'W':
					case 'G':
						lpSP->nCType = SQL_C_BINARY;
						lpSP->nSQLType = SQL_LONGVARBINARY;
						break;
					case 'N':
						lpSP->nCType = SQL_C_DOUBLE;
						lpSP->nSQLType = SQL_DOUBLE;
						break;
					case 'I':
						lpSP->nCType = SQL_C_SLONG;
						lpSP->nSQLType = SQL_INTEGER;
						break;
					case 'T':
						lpSP->nCType = SQL_C_TIMESTAMP;
						lpSP->nSQLType = SQL_TIMESTAMP;
						break;
					case 'D':
						lpSP->nCType = SQL_C_DATE;
						lpSP->nSQLType = SQL_DATE;
						break;
					case 'L':
						lpSP->nCType = SQL_C_BIT;
						lpSP->nSQLType = SQL_BIT;
						break;
					case 'Y':
						lpSP->nCType = SQL_C_NUMERIC;
						lpSP->nSQLType = SQL_NUMERIC;
						break;
					}
				}
				else
				{
					switch (lpSP->nSQLType)
					{
					case SQL_INTEGER:
					case SQL_SMALLINT:
						lpSP->nCType = SQL_C_LONG;
						break;

					case SQL_DOUBLE:
					case SQL_FLOAT:
					case SQL_REAL:
						lpSP->nCType = SQL_C_DOUBLE;
						break;

					case SQL_WCHAR:
					case SQL_WVARCHAR:
						lpSP->nCType = SQL_C_WCHAR;
						break;

					case SQL_BINARY:
					case SQL_VARBINARY:
						lpSP->nCType = SQL_C_BINARY;
						break;

					case SQL_CHAR:
					case SQL_VARCHAR:
						lpSP->nCType = SQL_C_CHAR;
						break;

					case SQL_DATE:
						lpSP->nCType = SQL_C_DATE;
						break;

					case SQL_TIMESTAMP:
						lpSP->nCType = SQL_C_TIMESTAMP;
						break;

					case SQL_BIGINT:
						lpSP->nCType = SQL_C_SBIGINT;
						break;

					case SQL_NUMERIC:
					case SQL_DECIMAL:
						lpSP->nCType = SQL_C_NUMERIC;
						break;
					}
				}
			}
		}
	}
}

void SqlStatement::BindParameters()
{
	SQLRETURN nApiRet;
	SqlParameter* lpSP;
	SQLPOINTER nValue;
	PSQLBINDPARAMETER fpSQLBindParameter = 0;
	HMODULE hDriver;
	SQLHSTMT hDriverStmt;
	bool bVerifyAppDescriptor, bVerifyImpDescriptor;

	for (int xj = 0; xj < pParameters.GetCount(); xj++)
	{
		lpSP = pParameters[xj];
		bVerifyAppDescriptor = bVerifyImpDescriptor = false;

		if (lpSP->nParmType <= SQL_RETURN_VALUE)
		{
			nApiRet = SQLBindParameter(hStmt, lpSP->nParmNo, lpSP->nParmType, lpSP->nCType,
				lpSP->nSQLType, lpSP->nSize, lpSP->nScale,
				lpSP->bPutData ? lpSP : lpSP->pParmPtr, lpSP->nBufferSize, &lpSP->nIndicator);
		}
		else
		{
			// bypass the the driver manager which rejects SQL_PARAM_INPUT_OUTPUT_STREAM and SQL_PARAM_OUTPUT_STREAM parameter types
			// since FoxPro is compiled for an old ODBC version, instead call the driver function directly 
			// https://jeffpar.github.io/kbarchive/kb/101/Q101525/
			if (!fpSQLBindParameter)
			{
				LoadDriverHandles(hDriver, hDriverStmt, hStmt);
				fpSQLBindParameter = (PSQLBINDPARAMETER)GetProcAddress(hDriver, "SQLBindParameter");
			}

			nApiRet = fpSQLBindParameter(hDriverStmt, lpSP->nParmNo, lpSP->nParmType, lpSP->nCType,
				lpSP->nSQLType, lpSP->nSize, lpSP->nScale,
				lpSP->bPutData ? lpSP : lpSP->pParmPtr, lpSP->nBufferSize, &lpSP->nIndicator);
		}

		if (nApiRet == SQL_ERROR)
		{
			SaveODBCError("SQLBindParameter", hStmt);
			throw E_APIERROR;
		}
		else if (nApiRet == SQL_SUCCESS_WITH_INFO && bStoreInfoAsError)
			AddODBCError("SQLBindParameter", hStmt);

		// if we have a named parameter, set additional descriptor information
		if (lpSP->pParmName)
		{
			SetImplementationParameterDescriptorField(lpSP->nParmNo, SQL_DESC_NAME, lpSP->pParmName.Data, lpSP->pParmName.Len);
			SetImplementationParameterDescriptorField(lpSP->nParmNo, SQL_DESC_UNNAMED, SQL_NAMED, SQL_IS_SMALLINT);
			bVerifyImpDescriptor = true;
		}

		switch (lpSP->nSQLType)
		{
		case SQL_TIMESTAMP:
		case SQL_TYPE_TIMESTAMP:
		case SQL_TYPE_TIME:
		case SQL_SS_TIME2:
		case SQL_SS_TIMESTAMPOFFSET:
			if (lpSP->nScale)
			{
				nValue = reinterpret_cast<SQLPOINTER>(static_cast<UINT_PTR>(lpSP->nScale));
				SetApplicationParameterDescriptorField(lpSP->nParmNo, SQL_DESC_PRECISION, nValue, SQL_IS_SMALLINT);
				bVerifyAppDescriptor = true;
			}
			break;

		case SQL_NUMERIC:
		case SQL_DECIMAL:
			if (lpSP->nSize)
			{
				nValue = reinterpret_cast<SQLPOINTER>(static_cast<UINT_PTR>(lpSP->nSize));
				SetApplicationParameterDescriptorField(lpSP->nParmNo, SQL_DESC_PRECISION, nValue, SQL_IS_SMALLINT);
				SetImplementationParameterDescriptorField(lpSP->nParmNo, SQL_DESC_PRECISION, nValue, SQL_IS_SMALLINT);
				bVerifyAppDescriptor = bVerifyImpDescriptor = true;
			}

			if (lpSP->nScale)
			{
				nValue = reinterpret_cast<SQLPOINTER>(static_cast<UINT_PTR>(lpSP->nScale));
				SetApplicationParameterDescriptorField(lpSP->nParmNo, SQL_DESC_SCALE, nValue, SQL_IS_SMALLINT);
				SetImplementationParameterDescriptorField(lpSP->nParmNo, SQL_DESC_SCALE, nValue, SQL_IS_SMALLINT);
				bVerifyAppDescriptor = bVerifyImpDescriptor = true;
			}

		case SQL_SS_TABLE:
			if (lpSP->pParmTypeSchema)
			{
				SetImplementationParameterDescriptorField(lpSP->nParmNo, SQL_CA_SS_SCHEMA_NAME, lpSP->pParmTypeSchema.Data, lpSP->pParmTypeSchema.Len);
			}

			// already set by SQLBindParameter
			if (lpSP->pParmTypeName)
			{
				SetImplementationParameterDescriptorField(lpSP->nParmNo, SQL_CA_SS_TYPE_NAME, lpSP->pParmTypeName.Data, lpSP->pParmTypeName.Len);
			}

			// only bind columns if there are rows to send
			if (lpSP->nIndicator == SQL_DATA_AT_EXEC)
			{
				BindTableValuedParameter(lpSP);
			}
			lpSP->bPutData = true; // reset SQL_DESC_DATA_PTR to the SqlParameter* pointer
			bVerifyAppDescriptor = bVerifyImpDescriptor = true;
			break;
		}

		if (bVerifyAppDescriptor)
			SetApplicationParameterDescriptorField(lpSP->nParmNo, SQL_DESC_DATA_PTR, lpSP->bPutData ? lpSP : lpSP->pParmPtr, SQL_IS_POINTER);
		if (bVerifyImpDescriptor)
			SetImplementationParameterDescriptorField(lpSP->nParmNo, SQL_DESC_DATA_PTR, lpSP->bPutData ? lpSP : lpSP->pParmPtr, SQL_IS_POINTER);
	}
}

SQLRETURN SqlStatement::Exec()
{
	SQLRETURN nApiRet;
	SQLCHAR* pSQL;
	SQLINTEGER nSQLLen;
	if (bPrepared)
	{
		nApiRet = SQLExecute(hStmt);
	}
	else if (nFlags & SQLEXECEX_PARAMETER_ARRAY)
	{
		if (pParameterArray.nBatchRecordsProcessed == 0)
		{
			if (nParameterCount)
			{
				pSQL = pSQLSend.Ptr<SQLCHAR*>();
				nSQLLen = pSQLSend.Len();
			}
			else
			{
				pSQL = pSQLInput.Ptr<SQLCHAR*>();
				nSQLLen = pSQLInput.Len();
			}

			nApiRet = SQLPrepare(hStmt, pSQL, nSQLLen);
			if (nApiRet == SQL_ERROR)
			{
				SaveODBCError("SQLPrepare", hStmt);
				throw E_APIERROR;
			}
			else if (nApiRet == SQL_SUCCESS_WITH_INFO)
				AddODBCError("SQLPrepare", hStmt);
		}
		nApiRet = SQLExecute(hStmt);
	}
	else
	{
		if (nParameterCount)
		{
			pSQL = pSQLSend.Ptr<SQLCHAR*>();
			nSQLLen = pSQLSend.Len();
		}
		else
		{
			pSQL = pSQLInput.Ptr<SQLCHAR*>();
			nSQLLen = pSQLInput.Len();
		}
		nApiRet = SQLExecDirect(hStmt, pSQL, nSQLLen);
	}
	return nApiRet;
}

void SqlStatement::ParseParameterArraySchema()
{
	SqlParameter* lpSP;
	ValueEx vRowValue;
	char* pValue;
	bool bBinary;
	int nRow = 1;
	FoxVariable pArray;
	CStrBuilder<64> pArrayName;
	CStrBuilder<VFP2C_MAX_FUNCTIONBUFFER> pExeBuffer;

	pParameterArray.nWorkArea = Select(pParameterArray.pCursor);
	if (pParameterArray.nWorkArea == 0)
	{
		SaveCustomError("SQLExecEx", "Cursor '%V' specified in parameter schema for parameter array not found.", &pParameterArray.pCursor);
		throw E_APIERROR;
	}

	// create unique array name .. since pStmt is a dynamically allocated pointer it's value is
	// always unique .. so we can use it to build a variable name ..
	pArrayName.Format("__VFP2C_ODBC_ARRAY_%U", this);
	pExeBuffer.Format("AFIELDS(%V,'%V')", &pArrayName.StringView(), &pParameterArray.pCursor);

	Execute(pExeBuffer);
	pArray.Attach(pArrayName);
	LocatorEx& lArrayLoc = pArray();
	lArrayLoc.ResetArray(2);

	for (int xj = 0; xj < pParameters.GetCount(); xj++)
	{
		lpSP = pParameters[xj];
		if (FindFoxField(lpSP->aParmExpr, lpSP->lVarOrField, pParameterArray.nWorkArea) > 0)
		{
			if (lpSP->cVFPType == '0')
			{
				SaveCustomError("SQLExecEx", "Parameter %U (%60S...) is no column in the cursor '%V' and no parameter schema was specified.", lpSP->nParmNo, lpSP->aParmExpr, &pParameterArray.pCursor);
				throw E_APIERROR;
			}
			if (lpSP->nParmType != SQL_PARAM_INPUT)
			{
				SaveCustomError("SQLExecEx", "Parameter %U (%60S...) was specified as output variable, but is no column in cursor '%V'.");
				throw E_APIERROR;
			}
		}

		if (lpSP->lVarOrField.l_NTI)
		{
			nRow = lpSP->lVarOrField.l_offset + 1;
			// fieldtype
			vRowValue = lArrayLoc(nRow, 2);
			pValue = vRowValue.HandleToPtr();

			lpSP->cVFPType = *pValue;
			vRowValue.FreeHandle();
			vRowValue.SetNull();
		}

		// precision/width
		switch (lpSP->cVFPType)
		{
			case 'C':
			case 'V':
			case 'Q':
				if (lpSP->lVarOrField.l_NTI)
				{
					vRowValue = lArrayLoc(nRow, 3);
					lpSP->nSize = (SQLUINTEGER)vRowValue.ev_long;
				}
				lpSP->nBufferSize = lpSP->nSize;
				break;
			case 'I':
				lpSP->nBufferSize = sizeof(int);
				break;
			case 'N':
			case 'F':
				if (lpSP->lVarOrField.l_type == 'R')
				{
					vRowValue = lArrayLoc(nRow, 3);
					lpSP->nSize = (SQLUINTEGER)vRowValue.ev_long;
					vRowValue = lArrayLoc(nRow, 4);
					lpSP->nScale = (SQLSMALLINT)vRowValue.ev_long;
				}
				lpSP->nBufferSize = sizeof(double);
				break;
			case 'B':
				if (lpSP->lVarOrField.l_type == 'R')
				{
					vRowValue = lArrayLoc(nRow, 4);
					lpSP->nScale = (SQLSMALLINT)vRowValue.ev_long;
				}
				lpSP->nBufferSize = sizeof(double);
				break;
			case 'L':
				lpSP->nBufferSize = sizeof(int);
				break;
			case 'D':
				lpSP->nBufferSize = sizeof(SQL_DATE_STRUCT);
				break;
			case 'T':
				lpSP->nBufferSize = sizeof(SQL_TIMESTAMP_STRUCT);
				break;
			case 'Y':
				lpSP->nSize = VFP2C_VFP_CURRENCY_PRECISION;
				lpSP->nScale = VFP2C_VFP_CURRENCY_SCALE;
				lpSP->nBufferSize = sizeof(SQL_NUMERIC_STRUCT);
				break;
		}

		if (lpSP->lVarOrField.l_NTI)
		{
			// binary
			vRowValue = lArrayLoc(nRow, 6);
			bBinary = vRowValue.ev_length > 0;
		}
		else
			bBinary = lpSP->bVFPBinary;

		lpSP->DeduceParameterType(lpSP->cVFPType, bBinary);
	}
}

bool SqlStatement::UseInMemoryParameterArray()
{
	return (ServerType == MySql || ServerType == MariaDB);
}

void SqlStatement::BindParameterArray()
{
	SQLRETURN nApiRet;
	SqlParameter* lpSP;
	SQLPOINTER nValue;
	bool bVerifyAppDescriptor, bVerifyImpDescriptor;
	bool bInMemory;
	int nRow = 0;
	pParameterArray.nBatchRecordsProcessed = 0;
	pParameterArray.nBatchErrorCount = 0;
	pParameterArray.pRecNoRestore.SaveRecNo(pParameterArray.nWorkArea, true);

	pParameterArray.nRecCount = RecCountEx(pParameterArray.nWorkArea);
	if (pParameterArray.nRecCount < 0)
	{
		SaveCustomErrorEx("SQLExecEx", "RecCount for cursor '%V' failed.", -pParameterArray.nRecCount, &pParameterArray.pCursor);
		throw E_APIERROR;
	}
	else if(pParameterArray.nRecCount == 0)
	{
		SaveCustomError("SQLExecEx", "Cursor '%V' specified in parameter schema for parameter array has no records or is EOF().", &pParameterArray.pCursor);
		throw E_APIERROR;
	}
	pParameterArray.nBatchSize = pParameterArray.nBatchSize > 0 ? pParameterArray.nBatchSize : 16;
	if (pParameterArray.nBatchSize > pParameterArray.nRecCount)
		pParameterArray.nBatchSize = pParameterArray.nRecCount;
	pParameterArray.nMaxBatchSize = pParameterArray.nBatchSize;
	pParameterArray.nLastBatchSize = pParameterArray.nBatchSize;
	pParameterArray.pParamStatus.Reserve(pParameterArray.nBatchSize, true, true);
	pParameterArray.pParamRecNo.Reserve(pParameterArray.nBatchSize, false, true);
	if (nFlags & SQLEXECEX_PARAMETER_ARRAY_RESUME_ON_ERROR)
		pParameterArray.pParamOperation.Reserve(pParameterArray.nBatchSize, true, true);
	
	bInMemory = UseInMemoryParameterArray();

	if (bInMemory)
	{
		pParameterArray.pIndicator.Reserve(pParameterArray.nBatchSize * pParameters.GetCount(), false, true);
		for (int xj = 0; xj < pParameters.GetCount(); xj++)
		{
			pParameters[xj]->nIndicator = (SQLLEN)&pParameterArray.pIndicator[nRow];
			nRow += pParameterArray.nBatchSize;
		}

		// load the data upfront into indiviual columns
		LoadInMemoryParameterArray();

		SetParameterArrayAttributes(true);
	}
	else
	{
		pParameterArray.pIndicator.Reserve(pParameters.GetCount(), false, true);
		for (int xj = 0; xj < pParameterArray.pIndicator.GetCount(); xj++)
		{
			pParameterArray.pIndicator[xj] = SQL_DATA_AT_EXEC;
			pParameters[xj]->pParmPtr = &pParameterArray.pIndicator[xj];
			pParameters[xj]->nIndicator = (SQLLEN)&pParameterArray.pIndicator[xj];
		}

		SetParameterArrayAttributes(false);
	}

	for (SQLLEN xj = 0; xj < pParameters.GetCount(); xj++)
	{
		lpSP = pParameters[xj];
		bVerifyAppDescriptor = bVerifyImpDescriptor = false;

		nApiRet = SQLBindParameter(hStmt, lpSP->nParmNo, lpSP->nParmType, lpSP->nCType,
			lpSP->nSQLType, lpSP->nSize, lpSP->nScale, (SQLPOINTER)lpSP->pParmPtr, lpSP->nBufferSize, (SQLLEN*)lpSP->nIndicator);

		if (nApiRet == SQL_ERROR)
		{
			SaveODBCError("SQLBindParameter", hStmt);
			throw E_APIERROR;
		}
		else if (nApiRet == SQL_SUCCESS_WITH_INFO && bStoreInfoAsError)
			AddODBCError("SQLBindParameter", hStmt);

		switch (lpSP->nSQLType)
		{
			case SQL_TIMESTAMP:
			case SQL_TYPE_TIMESTAMP:
			case SQL_TYPE_TIME:
			case SQL_SS_TIME2:
			case SQL_SS_TIMESTAMPOFFSET:
				if (lpSP->nScale)
				{
					nValue = reinterpret_cast<SQLPOINTER>(static_cast<UINT_PTR>(lpSP->nScale));
					SetApplicationParameterDescriptorField(lpSP->nParmNo, SQL_DESC_PRECISION, nValue, SQL_IS_SMALLINT);
					bVerifyAppDescriptor = true;
				}
				break;

			case SQL_DATETIME:
			case SQL_INTERVAL:
				if (lpSP->nSQLTypeCode)
				{
					nValue = reinterpret_cast<SQLPOINTER>(static_cast<UINT_PTR>(lpSP->nSQLTypeCode));
					SetApplicationParameterDescriptorField(lpSP->nParmNo, SQL_DESC_DATETIME_INTERVAL_CODE, nValue, SQL_IS_SMALLINT);
					bVerifyAppDescriptor = true;
				}
				if (lpSP->nScale)
				{
					nValue = reinterpret_cast<SQLPOINTER>(static_cast<UINT_PTR>(lpSP->nScale));
					SetApplicationParameterDescriptorField(lpSP->nParmNo, SQL_DESC_PRECISION, nValue, SQL_IS_SMALLINT);
					bVerifyAppDescriptor = true;
				}
				break;


			case SQL_NUMERIC:
			case SQL_DECIMAL:
				if (lpSP->nSize)
				{
					nValue = reinterpret_cast<SQLPOINTER>(static_cast<UINT_PTR>(lpSP->nSize));
					SetApplicationParameterDescriptorField(lpSP->nParmNo, SQL_DESC_PRECISION, nValue, SQL_IS_SMALLINT);
					bVerifyAppDescriptor = true;
				}

				if (lpSP->nScale)
				{
					nValue = reinterpret_cast<SQLPOINTER>(static_cast<UINT_PTR>(lpSP->nScale));
					SetApplicationParameterDescriptorField(lpSP->nParmNo, SQL_DESC_SCALE, nValue, SQL_IS_SMALLINT);
					bVerifyAppDescriptor = true;
				}
				break;
		}

		if (bVerifyAppDescriptor)
			SetApplicationParameterDescriptorField(lpSP->nParmNo, SQL_DESC_DATA_PTR, (SQLPOINTER)lpSP->pParmPtr, SQL_IS_POINTER);
		if (bVerifyImpDescriptor)
			SetImplementationParameterDescriptorField(lpSP->nParmNo, SQL_DESC_DATA_PTR, (SQLPOINTER)lpSP->pParmPtr, SQL_IS_POINTER);
	}
}

void SqlStatement::SetParameterArrayAttributes(bool bInMemory)
{
	SQLPOINTER nValue = reinterpret_cast<SQLPOINTER>(static_cast<UINT_PTR>(pParameterArray.nBatchSize));
	SQLRETURN nApiRet = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAMSET_SIZE, nValue, 0);
	if (nApiRet == SQL_ERROR)
	{
		SaveODBCError("SQLSetStmtAttr", hStmt);
		throw E_APIERROR;
	}
	else if (nApiRet == SQL_SUCCESS_WITH_INFO && bStoreInfoAsError)
		AddODBCError("SQLSetStmtAttr", hStmt);

	if (bInMemory)
	{
		// use column wise binding since some databases do not support data at execution parameters for parameter arrays
		nValue = SQL_PARAM_BIND_BY_COLUMN;
	}
	else
	{
		// use row wise binding with sizeof(indicator) * parametercount, that is not where the actual data resides,
		// we just use these addresses for binding data at execution parameters and compute the parameter index from them
		nValue = reinterpret_cast<SQLPOINTER>(static_cast<UINT_PTR>(sizeof(SQLLEN) * pParameters.GetCount()));
	}

	nApiRet = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAM_BIND_TYPE, nValue, 0);
	if (nApiRet == SQL_ERROR)
	{
		SaveODBCError("SQLSetStmtAttr", hStmt);
		throw E_APIERROR;
	}
	else if (nApiRet == SQL_SUCCESS_WITH_INFO && bStoreInfoAsError)
		AddODBCError("SQLSetStmtAttr", hStmt);

	nApiRet = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAM_STATUS_PTR, (SQLPOINTER)&pParameterArray.pParamStatus[0], 0);
	if (nApiRet == SQL_ERROR)
	{
		SaveODBCError("SQLSetStmtAttr", hStmt);
		throw E_APIERROR;
	}
	else if (nApiRet == SQL_SUCCESS_WITH_INFO && bStoreInfoAsError)
		AddODBCError("SQLSetStmtAttr", hStmt);

	nApiRet = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAMS_PROCESSED_PTR, (SQLPOINTER)&pParameterArray.nParamsProcessed, 0);
	if (nApiRet == SQL_ERROR)
	{
		SaveODBCError("SQLSetStmtAttr", hStmt);
		throw E_APIERROR;
	}
	else if (nApiRet == SQL_SUCCESS_WITH_INFO && bStoreInfoAsError)
		AddODBCError("SQLSetStmtAttr", hStmt);

	if (nFlags & SQLEXECEX_PARAMETER_ARRAY_RESUME_ON_ERROR)
	{
		SetApplicationParameterDescriptorField(0, SQL_DESC_ARRAY_STATUS_PTR, (SQLPOINTER)&pParameterArray.pParamOperation[0], 0);
	}

	if (!bInMemory)
	{
		pParameterArray.nParamOffset = 0;
		SetApplicationParameterDescriptorField(0, SQL_DESC_BIND_OFFSET_PTR, (SQLPOINTER)&pParameterArray.nParamOffset, 0);
	}
}

void SqlStatement::LoadInMemoryParameterArray()
{
	SqlParameter* lpSP;
	ValueEx vFieldValue;
	vFieldValue.SetNull();
	char* pBuffer;
	int nRowNo = 0, nRecNoIndex = pParameterArray.nParamsResumeOnError, nIndicatorIndex;

	if (pParameterArray.nBatchRecordsProcessed == 0)
	{
		for (int xj = 0; xj < pParameters.GetCount(); xj++)
		{
			lpSP = pParameters[xj];
			lpSP->pParmPtr = new char[lpSP->nBufferSize * pParameterArray.nBatchSize];
			if (!lpSP->pParmPtr)
				throw E_INSUFMEMORY;
		}
	}

	pParameterArray.pParamRecNo[nRecNoIndex++] = RecNo(pParameterArray.nWorkArea);
	do
	{
		for (int xj = 0; xj < pParameters.GetCount(); xj++)
		{
			lpSP = pParameters[xj];
			nIndicatorIndex = nRowNo + (pParameterArray.nBatchSize * xj);
			if (lpSP->lVarOrField.l_NTI)
				vFieldValue = lpSP->lVarOrField;
			else
				Evaluate(vFieldValue, lpSP->aParmExpr);
			if (vFieldValue.Vartype() == '0')
			{
				pParameterArray.pIndicator[nIndicatorIndex] = SQL_NULL_DATA;
				continue;
			}
			pBuffer = (char*)lpSP->pParmPtr;
			switch (lpSP->cVFPType)
			{
			case 'C':
			case 'V':
			case 'Q':
				if (vFieldValue.Len() > (unsigned int)lpSP->nBufferSize)
					memcpy(pBuffer + nRowNo * lpSP->nBufferSize, vFieldValue.HandleToPtr(), vFieldValue.Len());
				else
				{
					SaveCustomError("SQLExecEx", "String data right truncation for parameter %U (%20S...), size of parameter was to small.\n"
						"Buffersize = %U, Datasize = %U", lpSP->nParmNo, lpSP->aParmExpr, lpSP->nBufferSize, vFieldValue.Len());
					throw E_APIERROR;
				}
				pParameterArray.pIndicator[nIndicatorIndex] = vFieldValue.Len();
				vFieldValue.FreeHandle();
				vFieldValue.SetNull();
				break;
			case 'I':
				*reinterpret_cast<int*>(pBuffer + nRowNo * sizeof(int)) = (int)vFieldValue.ev_real;
				pParameterArray.pIndicator[nIndicatorIndex] = 0; // sizeof(int);
				break;
			case 'L':
				*reinterpret_cast<int*>(pBuffer + nRowNo * sizeof(int)) = (int)vFieldValue.ev_length;
				pParameterArray.pIndicator[nIndicatorIndex] = 0; // sizeof(int);
				break;
			case 'N':
			case 'F':
			case 'B':
				*reinterpret_cast<double*>(pBuffer + nRowNo * sizeof(double)) = vFieldValue.ev_real;
				pParameterArray.pIndicator[nIndicatorIndex] = 0; // sizeof(double);
				break;
			case 'D':
				ValueToDate_Struct(vFieldValue, (SQL_DATE_STRUCT*)(pBuffer + nRowNo * sizeof(SQL_DATE_STRUCT)));
				pParameterArray.pIndicator[nIndicatorIndex] = 0; // sizeof(SQL_DATE_STRUCT);
				break;
			case 'T':
				ValueToTimestamp_Struct(vFieldValue, (SQL_TIMESTAMP_STRUCT*)(pBuffer + nRowNo * sizeof(SQL_TIMESTAMP_STRUCT)));
				pParameterArray.pIndicator[nIndicatorIndex] = 0; // sizeof(SQL_TIMESTAMP_STRUCT);
				break;
			case 'Y':
				ValueToNumeric_Struct(vFieldValue, (SQL_NUMERIC_STRUCT*)pBuffer + nRowNo * sizeof(SQL_NUMERIC_STRUCT));
				pParameterArray.pIndicator[nIndicatorIndex] = 0;
				break;

			default:
				SaveCustomError("SQLExecEx", "Invalid datatype '%C' for parameter array - not supported on MySQL/MariaDB drivers.", lpSP->cVFPType);
				throw E_APIERROR;
			}
		}

		nRowNo++;
		if (nRowNo < pParameterArray.nBatchSize)
			pParameterArray.pParamRecNo[nRecNoIndex++] = Skip(1, pParameterArray.nWorkArea);
		else
			break;

	} while (true);

}

bool SqlStatement::LoadNextParameterArrayBatch()
{
	SQLRETURN nApiRet;
	SQLPOINTER nValue;
	SqlParameter* lpSP;
	bool bInMemory = UseInMemoryParameterArray();
	pParameterArray.nBatchRecordsProcessed += pParameterArray.nLastBatchSize;
	if (pParameterArray.nBatchRecordsProcessed == pParameterArray.nRecCount)
		return false;

	if (pParameterArray.nBatchErrorCount >= 2 && pParameterArray.nBatchErrorCount < pParameterArray.nBatchSize)
		pParameterArray.nBatchSize /= 2;
	else if (pParameterArray.nBatchErrorCount == 0 && pParameterArray.nBatchSize < pParameterArray.nMaxBatchSize)
		pParameterArray.nBatchSize *= 2;

	Go(pParameterArray.pParamRecNo[pParameterArray.pParamRecNo.GetIndex()], pParameterArray.nWorkArea);
	
	int nRow = 0;
	int nRecordsRemaining = (pParameterArray.nRecCount - pParameterArray.nBatchRecordsProcessed);
	if (nRecordsRemaining < pParameterArray.nBatchSize)
		pParameterArray.nBatchSize = nRecordsRemaining;
	if (pParameterArray.nBatchSize != pParameterArray.nLastBatchSize)
	{
		pParameterArray.nLastBatchSize = pParameterArray.nBatchSize;
		// if batch size changes - possible on last batch - then adjust parameters settings acordingly
		nValue = reinterpret_cast<SQLPOINTER>(static_cast<UINT_PTR>(pParameterArray.nBatchSize));
		nApiRet = SQLSetStmtAttr(hStmt, SQL_ATTR_PARAMSET_SIZE, nValue, 0);
		if (nApiRet == SQL_ERROR)
		{
			SaveODBCError("SQLSetStmtAttr", hStmt);
			throw E_APIERROR;
		}
		else if (nApiRet == SQL_SUCCESS_WITH_INFO && bStoreInfoAsError)
			AddODBCError("SQLSetStmtAttr", hStmt);

		if (bInMemory)
		{
			for (int xj = 0; xj < pParameters.GetCount(); xj++)
			{
				lpSP = pParameters[xj];
				lpSP->nIndicator = (SQLLEN)&pParameterArray.pIndicator[nRow];
				nRow += pParameterArray.nBatchSize;

				// reset pointers
				SetApplicationParameterDescriptorField(lpSP->nParmNo, SQL_DESC_INDICATOR_PTR, (SQLPOINTER)lpSP->nIndicator, 0);
				SetApplicationParameterDescriptorField(lpSP->nParmNo, SQL_DESC_OCTET_LENGTH_PTR, (SQLPOINTER)lpSP->nIndicator, 0);
			}
		}
		pParameterArray.pParamOperation.SetIndex(pParameterArray.nBatchSize - 1);
		pParameterArray.pParamRecNo.SetIndex(pParameterArray.nBatchSize - 1);
		pParameterArray.pParamStatus.SetIndex(pParameterArray.nBatchSize - 1);
	}

	if (pParameterArray.nBatchErrorCount)
	{
		for (int xj = 0; xj < pParameterArray.nBatchSize; xj++)
		{
			pParameterArray.pParamStatus[xj] = SQL_PARAM_SUCCESS;
			pParameterArray.pParamOperation[xj] = SQL_PARAM_PROCEED;
		}
	}
	pParameterArray.nBatchErrorCount = 0;
	pParameterArray.nParamOffset = 0;
	pParameterArray.nParamLastOutput = 0;
	pParameterArray.nParamsProcessed = 0;
	pParameterArray.nParamsLastProcessed = 0;
	pParameterArray.nParamsResumeOnError = 0;

	if (bInMemory)
		LoadInMemoryParameterArray();
	
	Skip(1, pParameterArray.nWorkArea);
	return true;
}

void SqlStatement::SaveParameterArrayErrors()
{
	VFP2CTls& tls = VFP2CTls::Tls();
	SQLINTEGER nErrorCount;
	SQLLEN nRowNo, nSeverity;
	CArray<SQLINTEGER> pErrorMap;
	bResumeOnError = false;
	// store all row numbers for each available error message in a lookup array 
	SQLRETURN nApiRet = SQLGetDiagField(SQL_HANDLE_STMT, hStmt, 0, SQL_DIAG_NUMBER, (SQLPOINTER)&nErrorCount, 0, 0);
	if (!SQL_SUCCEEDED(nApiRet))
	{
		SaveODBCError("SQLGetDiagField", hStmt);
		throw E_APIERROR;
	}
	if (nErrorCount == 0)
		return;

	pErrorMap.Reserve(nErrorCount, false);
	pErrorMap.SetIndex(-1);
	for (int xj = 1; xj <= nErrorCount; xj++)
	{
		nApiRet = SQLGetDiagField(SQL_HANDLE_STMT, hStmt, xj, SQL_DIAG_ROW_NUMBER, (SQLPOINTER)&nRowNo, 0, 0);
		if (SQL_SUCCEEDED(nApiRet))
		{
			if (nRowNo >= 1 && pErrorMap.Find(nRowNo) != -1)
				nRowNo = SQL_ROW_NUMBER_UNKNOWN;
			pErrorMap.SetIndex(xj - 1);
			pErrorMap[xj - 1] = nRowNo;
		}
		else
		{
			SaveODBCError("SQLGetDiagField", hStmt);
			throw E_APIERROR;
		}
	}

	// store all error messages which have no valid rownumber into the normal Error structure
	if (tls.ErrorCount < VFP2C_MAX_ERRORS)
	{
		LPVFP2CERROR pError = &tls.ErrorInfo[tls.ErrorCount];
		SQLSMALLINT nErrorLength;
		for (int xj = 0; xj < nErrorCount; xj++)
		{
			if (pErrorMap[xj] < 1)
			{
				nApiRet = SQLGetDiagRec(SQL_HANDLE_STMT, hStmt, xj + 1,	(SQLCHAR*)pError->aSqlState,(SQLINTEGER*)&pError->nErrorNo,
					(SQLCHAR*)pError->aErrorMessage, VFP2C_ERROR_MESSAGE_LEN, &nErrorLength);
				if (SQL_SUCCEEDED(nApiRet))
				{
					nApiRet = SQLGetDiagField(SQL_HANDLE_STMT, hStmt, xj + 1, SQL_DIAG_SS_SEVERITY, (SQLPOINTER)&nSeverity, 0, 0);
					if (SQL_SUCCEEDED(nApiRet) && nSeverity < EX_MAXISEVERITY)
						break;
					pError->nErrorType = VFP2C_ERRORTYPE_ODBC;
					pError++;
					tls.ErrorCount++;
					if (tls.ErrorCount == VFP2C_MAX_ERRORS)
						break;
				}
			}
		}
	}

	bResumeOnError = pParameterArray.pParamStatus[pParameterArray.pParamStatus.GetIndex()] != SQL_PARAM_ERROR && 
			pParameterArray.nParamsResumeOnError + 1 < pParameterArray.nBatchSize;
	nRowNo = !bResumeOnError ? 1 : 2;
	pParameterArray.pParamStatus[pParameterArray.nParamsProcessed - nRowNo] = SQL_PARAM_ERROR;	

	// store all row related status information and error messages into a cursor/array
	SaveParameterArrayErrorsToCursor(pErrorMap);
}

void SqlStatement::SaveParameterArrayErrorsToCursor(CArray<SQLINTEGER>& pErrorMap)
{
	SQLRETURN nApiRet;
	SQLINTEGER nNativeError;
	SQLSMALLINT nMsgLen, nErrorNo;
	SQLINTEGER nColumnNo;
	FoxString vSqlState;
	AutoWorkAreaRestore pRestoreWorkArea(0, 0);
	vSqlState.Size(16);
	if (pParameterArray.pParamStatusCursor.WorkArea() == 0)
	{
		pGetDataBuffer.Size(VFP2C_ODBC_MAX_VARIABLE_BUFFER);
		if (nFlags & SQLEXECEX_PARAMETER_ARRAY_SAVE_ERROR_MESSAGE)
			pParameterArray.pParamStatusCursor.Create(pParameterArray.pParamStatusDestination, "rec_number I,rec_status I,column_number I NULL,error_number I NULL, sqlstate C(5) NULL, message M NULL");
		else
			pParameterArray.pParamStatusCursor.Create(pParameterArray.pParamStatusDestination, "rec_number I,rec_status I,column_number I NULL,error_number I NULL, sqlstate C(5) NULL");
	}
	FoxCursor& pCursor = pParameterArray.pParamStatusCursor;
	int nRow = pParameterArray.pParamStatus[pParameterArray.nParamsResumeOnError] == SQL_PARAM_ERROR ? 1 : pParameterArray.nParamsResumeOnError + 1;
	for (int xj = pParameterArray.nParamsResumeOnError; xj < pParameterArray.nParamsProcessed; xj++)
	{
		if (pParameterArray.pParamStatus[xj] == SQL_SUCCESS)
		{
			nRow++;
			continue;
		}
		nErrorNo = pErrorMap.Find(nRow++);
		if (nErrorNo >= 0)
		{
			pCursor.AppendBlank();
			pCursor(0) = pParameterArray.pParamRecNo[xj];
			pCursor(1) = pParameterArray.pParamStatus[xj];
			nErrorNo++;
			nApiRet = SQLGetDiagField(SQL_HANDLE_STMT, hStmt, nErrorNo, SQL_DIAG_COLUMN_NUMBER, (SQLPOINTER)&nColumnNo, SQL_IS_INTEGER, 0);
			if (SQL_SUCCEEDED(nApiRet))
				pCursor(2) = nColumnNo;
			else
				pCursor(2) = -1;

			if (nFlags & SQLEXECEX_PARAMETER_ARRAY_SAVE_ERROR_MESSAGE)
				nApiRet = SQLGetDiagRec(SQL_HANDLE_STMT, hStmt, nErrorNo, vSqlState.Ptr<SQLCHAR*>(), &nNativeError,
					pGetDataBuffer.Ptr<SQLCHAR*>(), pGetDataBuffer.Size(), &nMsgLen);
			else
				nApiRet = SQLGetDiagRec(SQL_HANDLE_STMT, hStmt, nErrorNo, vSqlState.Ptr<SQLCHAR*>(), &nNativeError,	0, 0, &nMsgLen);

			if (SQL_SUCCEEDED(nApiRet))
			{
				pCursor(3) = nNativeError;
				pCursor(4) = vSqlState.StringLen();
				if (nFlags & SQLEXECEX_PARAMETER_ARRAY_SAVE_ERROR_MESSAGE)
					pCursor(5) = pGetDataBuffer.Len(nMsgLen);
			}
			else
			{
				pCursor(3) = 0;
				pCursor(4) = pGetDataBuffer.Len(0);
				if (nFlags & SQLEXECEX_PARAMETER_ARRAY_SAVE_ERROR_MESSAGE)
					pCursor(5) = pGetDataBuffer.Len(0);
			}
		}
	}
}

SQLRETURN SqlStatement::ResumeOnParameterArrayError()
{
	SQLRETURN nApiRet;

	if (bResumeOnError)
	{
		pParameterArray.nParamsProcessed--;
		for (int xj = pParameterArray.nParamsResumeOnError; xj < pParameterArray.nParamsProcessed; xj++)
			pParameterArray.pParamOperation[xj] = SQL_PARAM_IGNORE;

		pParameterArray.nBatchErrorCount++;
		pParameterArray.nParamLastOutput = pParameterArray.nParamsProcessed;
		pParameterArray.nParamsResumeOnError = pParameterArray.nParamsProcessed;
		pParameterArray.nParamsLastProcessed = pParameterArray.nParamsProcessed;
		pParameterArray.nParamOffset = -(SQLLEN)(pParameterArray.nParamsProcessed * (sizeof(SQLLEN) * pParameters.GetCount()));
		pParameterArray.nParamsProcessed = 0;
		Go(pParameterArray.pParamRecNo[pParameterArray.nParamsResumeOnError], pParameterArray.nWorkArea);

		nApiRet = SQLMoreResults(hStmt);
		while(nApiRet == SQL_SUCCESS_WITH_INFO)
		{
			SaveParameterArrayErrors();
			nApiRet = SQLMoreResults(hStmt);
		}
		if (nApiRet != SQL_NO_DATA)
		{
			SaveODBCError("SQLMoreResults", hStmt);
			throw E_APIERROR;
		}
		nApiRet = SQLExecute(hStmt);
	}
	else
		nApiRet = SQLMoreResults(hStmt);
	return nApiRet;
}

void SqlStatement::FreeInMemoryParameterArray()
{
	char* pBuffer;
	if ((nFlags & SQLEXECEX_PARAMETER_ARRAY) && UseInMemoryParameterArray())
	{
		for (int xj = 0; xj < pParameters.GetCount(); xj++)
		{
			pBuffer = (char*)pParameters[xj]->pParmPtr;
			if (pBuffer)
			{
				delete[] pBuffer;
				pParameters[xj]->pParmPtr = 0;
			}
		}
	}
}

SqlParameter* SqlStatement::FindParameter(CStringView pField)
{
	for (int xj = 0; xj < pParameters.GetCount(); xj++)
	{
		if (pField.ICompare(pParameters[xj]->aParmExpr))
			return pParameters[xj];
	}
	return 0;
}

void SqlStatement::BindTableValuedParameter(SqlParameter* lpSP)
{
	SQLRETURN nApiRet;
	SqlTableTypeColumn* lpTTC;
	SqlTableTypeParameter* lpTTP;
	SQLSMALLINT nColumnNo;
	SQLPOINTER nValue;
	int nErrorNo;
	bool bVerifyAppDescriptor, bVerifyImpDescriptor;

	nApiRet = SQLSetStmtAttr(hStmt, SQL_SOPT_SS_PARAM_FOCUS, (SQLPOINTER)lpSP->nParmNo, SQL_IS_INTEGER);
	if (nApiRet == SQL_ERROR)
	{
		SaveODBCError("SQLSetStmtAttr", hStmt);
		throw E_APIERROR;
	}
	else if (nApiRet == SQL_SUCCESS_WITH_INFO && bStoreInfoAsError)
		AddODBCError("SQLSetStmtAttr", hStmt);

	for (int xj = 0; xj < lpSP->pTableType->pColumns.GetCount(); xj++)
	{
		lpTTC = lpSP->pTableType->pColumns[xj];
		lpTTP = lpSP->pTableTypeParameters[xj];
		nColumnNo = lpTTC->nColumnNo + 1;

		nErrorNo = FindFoxField(lpTTC->pColumnName.Ptr<char*>(), lpTTP->lField, lpSP->nWorkArea);
		if (nErrorNo)
		{
			SaveCustomErrorEx("SQLExecEx", "Field '%S', for table valued parameter '%S' not found.", E_FIELDNOTFOUND, lpTTC->pColumnName.Ptr<char*>(), lpSP->vParmValue.HandleToPtr());
			throw E_APIERROR;
		}

		bVerifyAppDescriptor = bVerifyImpDescriptor = false;
		nApiRet = SQLBindParameter(hStmt, nColumnNo, lpSP->nParmType, lpTTC->nCType,
			lpTTC->nSQLType, lpTTC->nSize, lpTTC->nScale, (SQLPOINTER)lpTTC->nColumnNo, 0, &lpSP->nIndicator);
		if (nApiRet == SQL_ERROR)
		{
			SaveODBCError("SQLBindParameter", hStmt);
			throw E_APIERROR;
		}
		else if (nApiRet == SQL_SUCCESS_WITH_INFO && bStoreInfoAsError)
			AddODBCError("SQLBindParameter", hStmt);

		if ((lpTTC->nSQLType == SQL_DATETIME || lpTTC->nSQLType == SQL_INTERVAL) && lpTTC->nSQLTypeCode)
		{
			nValue = reinterpret_cast<SQLPOINTER>(static_cast<UINT_PTR>(lpTTC->nSQLTypeCode));
			SetApplicationParameterDescriptorField(nColumnNo, SQL_DESC_DATETIME_INTERVAL_CODE, nValue, SQL_IS_SMALLINT);
			bVerifyAppDescriptor = true;
		}

		switch (lpTTC->nSQLType)
		{
			case SQL_TIMESTAMP:
			case SQL_TYPE_TIMESTAMP:
			case SQL_TIME:
			case SQL_TYPE_TIME:
			case SQL_SS_TIME2:
			case SQL_SS_TIMESTAMPOFFSET:
				if (lpTTC->nScale)
				{
					nValue = reinterpret_cast<SQLPOINTER>(static_cast<UINT_PTR>(lpTTC->nScale));
					SetApplicationParameterDescriptorField(nColumnNo, SQL_DESC_PRECISION, nValue, SQL_IS_SMALLINT);
					bVerifyAppDescriptor = true;
				}
				break;

			case SQL_NUMERIC:
			case SQL_DECIMAL:
				if (lpTTC->nSize)
				{
					nValue = reinterpret_cast<SQLPOINTER>(static_cast<UINT_PTR>(lpTTC->nSize));
					SetApplicationParameterDescriptorField(nColumnNo, SQL_DESC_PRECISION, nValue, SQL_IS_SMALLINT);
					SetImplementationParameterDescriptorField(nColumnNo, SQL_DESC_PRECISION, nValue, SQL_IS_SMALLINT);
					bVerifyAppDescriptor = bVerifyImpDescriptor = true;
				}

				if (lpTTC->nScale)
				{
					nValue = reinterpret_cast<SQLPOINTER>(static_cast<UINT_PTR>(lpTTC->nScale));
					SetApplicationParameterDescriptorField(nColumnNo, SQL_DESC_SCALE, nValue, SQL_IS_SMALLINT);
					SetImplementationParameterDescriptorField(nColumnNo, SQL_DESC_SCALE, nValue, SQL_IS_SMALLINT);
					bVerifyAppDescriptor = bVerifyImpDescriptor = true;
				}
				break;
		}

		if (lpTTP->nDescAutoUniqueValue)
		{
			nValue = reinterpret_cast<SQLPOINTER>(static_cast<UINT_PTR>(lpTTP->nDescAutoUniqueValue));
			SetImplementationParameterDescriptorField(nColumnNo, SQL_DESC_AUTO_UNIQUE_VALUE, nValue, SQL_IS_INTEGER);
			bVerifyImpDescriptor = true;
		}

		if (lpTTP->nColumnComputed)
		{
			nValue = reinterpret_cast<SQLPOINTER>(static_cast<UINT_PTR>(lpTTP->nColumnComputed));
			SetImplementationParameterDescriptorField(nColumnNo, SQL_CA_SS_COLUMN_COMPUTED, nValue, SQL_IS_SMALLINT);
			bVerifyImpDescriptor = true;
		}

		if (lpTTP->nColumnInUniqueKey)
		{
			nValue = reinterpret_cast<SQLPOINTER>(static_cast<UINT_PTR>(lpTTP->nColumnInUniqueKey));
			SetImplementationParameterDescriptorField(nColumnNo, SQL_CA_SS_COLUMN_IN_UNIQUE_KEY, nValue, SQL_IS_SMALLINT);
			bVerifyImpDescriptor = true;
		}

		if (lpTTP->nColumnSortOrder)
		{
			nValue = reinterpret_cast<SQLPOINTER>(static_cast<UINT_PTR>(lpTTP->nColumnSortOrder));
			SetImplementationParameterDescriptorField(nColumnNo, SQL_CA_SS_COLUMN_SORT_ORDER, nValue, SQL_IS_SMALLINT);
			bVerifyImpDescriptor = true;
		}

		if (lpTTP->nColumnSortOrdinal)
		{
			nValue = reinterpret_cast<SQLPOINTER>(static_cast<UINT_PTR>(lpTTP->nColumnSortOrdinal));
			SetImplementationParameterDescriptorField(nColumnNo, SQL_CA_SS_COLUMN_SORT_ORDINAL, nValue, SQL_IS_SMALLINT);
			bVerifyImpDescriptor = true;
		}

		if (lpTTP->nColumnHasDefaultValue)
		{
			nValue = reinterpret_cast<SQLPOINTER>(static_cast<UINT_PTR>(lpTTP->nColumnHasDefaultValue));
			SetImplementationParameterDescriptorField(nColumnNo, SQL_CA_SS_COLUMN_HAS_DEFAULT_VALUE, nValue, SQL_IS_SMALLINT);
			bVerifyImpDescriptor = true;
		}

		if (bVerifyAppDescriptor)
			SetApplicationParameterDescriptorField(nColumnNo, SQL_DESC_DATA_PTR, (SQLPOINTER)lpTTC->nColumnNo, SQL_IS_POINTER);
		if (bVerifyImpDescriptor)
			SetImplementationParameterDescriptorField(nColumnNo, SQL_DESC_DATA_PTR, (SQLPOINTER)lpTTC->nColumnNo, SQL_IS_POINTER);

	}

	nApiRet = SQLSetStmtAttr(hStmt, SQL_SOPT_SS_PARAM_FOCUS, 0, SQL_IS_INTEGER);
	if (nApiRet == SQL_ERROR)
	{
		SaveODBCError("SQLSetStmtAttr", hStmt);
		throw E_APIERROR;
	}
	else if (nApiRet == SQL_SUCCESS_WITH_INFO && bStoreInfoAsError)
		AddODBCError("SQLSetStmtAttr", hStmt);
}

SQLRETURN SqlStatement::SendParameters()
{
	SQLRETURN nApiRet;
	SqlParameter* lpSP;
	char* lpData;
	int nErrorNo;

	nApiRet = SQLParamData(hStmt, (SQLPOINTER*)&lpSP);
	while (nApiRet == SQL_NEED_DATA)
	{
		if (lpSP->nSQLType == SQL_SS_TABLE)
		{
			SendTableValuedParameter(lpSP);
		}
		else if (lpSP->vParmValue.Vartype() == 'C')
		{
			nApiRet = SQLPutData(hStmt, lpSP->pParmPtr, lpSP->nSize);
			if (nApiRet == SQL_ERROR)
			{
				SaveODBCError("SQLPutData", hStmt);
				throw E_APIERROR;
			}
			else if (nApiRet == SQL_SUCCESS_WITH_INFO && bStoreInfoAsError)
				AddODBCError("SQLPutData", hStmt);
		}
		else
		{
			lpSP->vMemoValue.SetString();
			if (!lpSP->vMemoValue.AllocHandle(lpSP->nSize))
				throw E_INSUFMEMORY;
			lpData = lpSP->vMemoValue.HandleToPtr();
			if (nErrorNo = GetMemoContent(lpSP->vParmValue, lpData))
				throw nErrorNo;

			nApiRet = SQLPutData(hStmt, lpData, lpSP->nSize);

			lpSP->vMemoValue.FreeHandle();
			lpSP->vMemoValue.SetNull();

			if (nApiRet == SQL_ERROR)
			{
				SaveODBCError("SQLPutData", hStmt);
				throw E_APIERROR;
			}
			else if (nApiRet == SQL_SUCCESS_WITH_INFO && bStoreInfoAsError)
				AddODBCError("SQLPutData", hStmt);
		}

		nApiRet = SQLParamData(hStmt, (SQLPOINTER*)&lpSP);
	}
	if (nApiRet == SQL_ERROR)
	{
		SaveODBCError("SQLParamData", hStmt);
		throw E_APIERROR;
	}
	else if (nApiRet == SQL_SUCCESS_WITH_INFO && bStoreInfoAsError)
		AddODBCError("SQLParamData", hStmt);
	return nApiRet;
}

SQLRETURN SqlStatement::SendParameterArray()
{
	SQLRETURN nApiRet;
	SqlParameter* lpSP;
	SQLPOINTER lpData;
	SQLLEN nIndicator, nParamIndex;
	int nErrorNo, nParameterCount = 0, nFirstRow = pParameterArray.nParamsResumeOnError, nRecNoIndex = pParameterArray.nParamsResumeOnError;
	nApiRet = SQLParamData(hStmt, (SQLPOINTER*)&nParamIndex);
	while (nApiRet == SQL_NEED_DATA)
	{
		// recalculate parameter index from returned memory address
		nParamIndex -= (SQLLEN)&pParameterArray.pIndicator[0];
		nParamIndex >>= (sizeof(SQLLEN) == 8 ? 3 : 2); // quick divide by 8 or 4 using a right shift
		lpSP = pParameters[nParamIndex];

		if (nParamIndex == 0)
		{
			if (nRecNoIndex == nFirstRow)
				pParameterArray.pParamRecNo[nRecNoIndex++] = RecNo(pParameterArray.nWorkArea);
			else
				pParameterArray.pParamRecNo[nRecNoIndex++] = Skip(1, pParameterArray.nWorkArea);
		}

		if (lpSP->lVarOrField.l_NTI > 0)
			lpSP->vParmValue = lpSP->lVarOrField;
		else
			Evaluate(lpSP->vParmValue, lpSP->aParmExpr);

		if (lpSP->vParmValue.Vartype() != '0')
		{
			switch (lpSP->cVFPType)
			{
				case 'C':
				case 'V':
				case 'Q':
					lpData = lpSP->vParmValue.HandleToPtr<SQLPOINTER>();
					nIndicator = lpSP->vParmValue.Len();
					break;

				case 'M':
				case 'W':
				case 'G':
					if (lpSP->vMemoValue.Vartype() != 'C')
					{
						lpSP->vMemoValue.SetString();
						if (!lpSP->vMemoValue.AllocHandle(lpSP->vParmValue.ev_long))
							throw E_INSUFMEMORY;
						lpSP->vMemoValue.ev_length = lpSP->vParmValue.ev_long;
						lpData = lpSP->vMemoValue.HandleToPtr<SQLPOINTER>();
						if (nErrorNo = GetMemoContent(lpSP->vParmValue, (char*)lpData))
							throw nErrorNo;
					}
					else if (lpSP->vMemoValue.GetHandleSize() < (unsigned long)lpSP->vParmValue.ev_long)
					{
						if (!lpSP->vMemoValue.SetHandleSize(lpSP->vParmValue.ev_long))
							throw E_INSUFMEMORY;
						lpSP->vMemoValue.ev_length = lpSP->vParmValue.ev_long;
						lpData = lpSP->vMemoValue.HandleToPtr<SQLPOINTER>();
						if (nErrorNo = GetMemoContent(lpSP->vParmValue, (char*)lpData))
							throw nErrorNo;
					}
					else
					{
						lpSP->vMemoValue.ev_length = lpSP->vParmValue.ev_long;
						lpData = lpSP->vMemoValue.HandleToPtr<SQLPOINTER>();
						if (nErrorNo = GetMemoContent(lpSP->vParmValue, (char*)lpData))
							throw nErrorNo;
					}
					nIndicator = lpSP->vParmValue.ev_long;
					break;

				case 'I':
					lpSP->vParmValue.ev_long = static_cast<long>(lpSP->vParmValue.ev_real);
					lpData = &lpSP->vParmValue.ev_long;
					nIndicator = sizeof(int);
					break;

				case 'N':
				case 'B':
				case 'F':
					lpData = &lpSP->vParmValue.ev_real;
					nIndicator = sizeof(double);
					break;

				case 'Y':
					ValueToNumeric_Struct(lpSP->vParmValue, &lpSP->sNumericStruct);
					lpData = &lpSP->sNumericStruct;
					nIndicator = sizeof(SQL_NUMERIC_STRUCT);
					break;

				case 'T':
					if (lpSP->vParmValue.ev_real != 0.0)
					{
						ValueToTimestamp_Struct(lpSP->vParmValue, &lpSP->sTimestampStruct);
						lpData = &lpSP->sTimestampStruct;
						nIndicator = sizeof(SQL_TIMESTAMP_STRUCT);
					}
					else
					{
						lpData = 0;
						nIndicator = SQL_NULL_DATA;
					}
					break;

				case 'D':
					if (lpSP->vParmValue.ev_real != 0.0)
					{
						ValueToDate_Struct(lpSP->vParmValue, &lpSP->sDateStruct);
						lpData = &lpSP->sDateStruct;
						nIndicator = sizeof(SQL_DATE_STRUCT);
					}
					else
					{
						lpData = 0;
						nIndicator = SQL_NULL_DATA;
					}
					break;
			}
		}
		else
		{
			lpData = 0;
			nIndicator = SQL_NULL_DATA;
		}

		nApiRet = SQLPutData(hStmt, lpData, nIndicator);
		if (lpSP->vParmValue.Vartype() == 'C')
		{
			lpSP->vParmValue.FreeHandle();
			lpSP->vParmValue.SetNull();
		}
		if (nApiRet == SQL_ERROR)
		{
			SaveODBCError("SQLPutData", hStmt);
			throw E_APIERROR;
		}
		else if (nApiRet == SQL_SUCCESS_WITH_INFO && bStoreInfoAsError)
			AddODBCError("SQLPutData", hStmt);

		if (++nParameterCount == pParameters.GetCount())
		{
			pParameterArray.nParamOffset -= sizeof(SQLLEN) * pParameters.GetCount();
			nParameterCount = 0;
		}

		nApiRet = SQLParamData(hStmt, (SQLPOINTER*)&nParamIndex);
	}
	if (nApiRet == SQL_ERROR)
	{
		SaveODBCError("SQLParamData", hStmt);
		throw E_APIERROR;
	}

	return nApiRet;
}

void SqlStatement::SendTableValuedParameter(SqlParameter* lpSP)
{
	SQLRETURN nApiRet;
	SqlTableTypeColumn* lpTVPCol;
	SqlTableTypeParameter* lpTVPParam;
	SQLPOINTER lpData;
	SQLLEN nIndicator;
	ValueEx vFieldValue;
	AutoRecNoRestore pRecNoRestore(lpSP->nWorkArea);
	vFieldValue.SetNull();
	int nColumnNumber, nErrorNo;
	GoTop(lpSP->nWorkArea);

	do
	{
		nApiRet = SQLPutData(hStmt, (SQLPOINTER)1, 1); // send 1 to indicate an available row
		if (nApiRet == SQL_ERROR)
		{
			SaveODBCError("SQLPutData", hStmt);
			throw E_APIERROR;
		}
		else if (nApiRet == SQL_SUCCESS_WITH_INFO && bStoreInfoAsError)
			AddODBCError("SQLPutData", hStmt);

		do
		{
			nApiRet = SQLParamData(hStmt, &lpData);
			if (nApiRet == SQL_ERROR)
			{
				SaveODBCError("SQLParamData", hStmt);
				throw E_APIERROR;
			}
			else if (nApiRet == SQL_SUCCESS_WITH_INFO && bStoreInfoAsError)
				AddODBCError("SQLParamData", hStmt);

			if (lpData == (SQLPOINTER)lpSP)
				break;

			nColumnNumber = reinterpret_cast<INT_PTR>(lpData);
			lpTVPCol = lpSP->pTableType->pColumns[nColumnNumber];
			lpTVPParam = lpSP->pTableTypeParameters[nColumnNumber];
			vFieldValue = lpTVPParam->lField;
			if (vFieldValue.Vartype() == '0')
			{
				lpData = 0;
				nIndicator = SQL_NULL_DATA;
			}
			else
			{
				switch (lpTVPCol->cVFPType)
				{
				case 'C':
				case 'V':
				case 'Q':
					lpData = vFieldValue.HandleToPtr<SQLPOINTER>();
					nIndicator = vFieldValue.Len();
					break;

				case 'M':
				case 'W':
				case 'G':
					if (lpSP->vMemoValue.Vartype() != 'C')
					{
						lpSP->vMemoValue.SetString();
						if (!lpSP->vMemoValue.AllocHandle(vFieldValue.ev_long))
							throw E_INSUFMEMORY;
						lpSP->vMemoValue.ev_length = vFieldValue.ev_long;
						lpData = lpSP->vMemoValue.HandleToPtr<SQLPOINTER>();
						if (nErrorNo = GetMemoContent(vFieldValue, (char*)lpData))
							throw nErrorNo;
					}
					else if (lpSP->vMemoValue.GetHandleSize() < (unsigned long)vFieldValue.ev_long)
					{
						if (!lpSP->vMemoValue.SetHandleSize(vFieldValue.ev_long))
							throw E_INSUFMEMORY;
						lpSP->vMemoValue.ev_length = vFieldValue.ev_long;
						lpData = lpSP->vMemoValue.HandleToPtr<SQLPOINTER>();
						if (nErrorNo = GetMemoContent(vFieldValue, (char*)lpData))
							throw nErrorNo;
					}
					else
					{
						lpSP->vMemoValue.ev_length = vFieldValue.ev_long;
						lpData = lpSP->vMemoValue.HandleToPtr<SQLPOINTER>();
						if (nErrorNo = GetMemoContent(vFieldValue, (char*)lpData))
							throw nErrorNo;
					}
					nIndicator = vFieldValue.ev_long;
					break;

				case 'T':
					if (vFieldValue.ev_real != 0.0)
					{
						ValueToTimestamp_Struct(vFieldValue, &lpSP->sTimestampStruct);
						lpData = &lpSP->sTimestampStruct;
						nIndicator = sizeof(lpSP->sTimestampStruct);
					}
					else
					{
						lpData = 0;
						nIndicator = SQL_NULL_DATA;
					}
					break;

				case 'D':
					if (vFieldValue.ev_real != 0.0)
					{
						ValueToDate_Struct(vFieldValue, &lpSP->sDateStruct);
						lpData = &lpSP->sDateStruct;
						nIndicator = sizeof(lpSP->sDateStruct);
					}
					else
					{
						lpData = 0;
						nIndicator = SQL_NULL_DATA;
					}
					break;

				case 'I':
					vFieldValue.ev_long = static_cast<long>(vFieldValue.ev_real);
					lpData = &vFieldValue.ev_long;
					nIndicator = sizeof(vFieldValue.ev_long);
					break;

				case 'N':
				case 'B':
				case 'F':
					lpData = &vFieldValue.ev_real;
					nIndicator = sizeof(vFieldValue.ev_real);
					break;

				case 'Y':
					ValueToNumeric_Struct(vFieldValue, &lpSP->sNumericStruct);
					lpData = &lpSP->sNumericStruct;
					nIndicator = sizeof(SQL_NUMERIC_STRUCT);
					break;
				}
			}

			nApiRet = SQLPutData(hStmt, lpData, nIndicator);
			if (vFieldValue.Vartype() == 'C')
				vFieldValue.FreeHandle();
			vFieldValue.SetNull();

			if (nApiRet == SQL_ERROR)
			{
				SaveODBCError("SQLPutData", hStmt);
				throw E_APIERROR;
			}
			else if (nApiRet == SQL_SUCCESS_WITH_INFO && bStoreInfoAsError)
				AddODBCError("SQLPutData", hStmt);

		} while (true);

		Skip(1, lpSP->nWorkArea);

	} while (!Eof(lpSP->nWorkArea));

	nApiRet = SQLPutData(hStmt, (SQLPOINTER)0, 0); // send 0 to indicate no more available rows
	if (nApiRet == SQL_ERROR)
	{
		SaveODBCError("SQLPutData", hStmt);
		throw E_APIERROR;
	}
	else if (nApiRet == SQL_SUCCESS_WITH_INFO && bStoreInfoAsError)
		AddODBCError("SQLPutData", hStmt);
}

void SqlStatement::StreamParameters()
{
	SQLRETURN nApiRet;
	SQLPOINTER lpData;
	SQLLEN nBufferSize;
	SqlParameter* lpSP;
	nApiRet = SQLParamData(hStmt, (SQLPOINTER*)&lpSP);
	while (nApiRet == SQL_PARAM_DATA_AVAILABLE)
	{
		if (lpSP->vParmValue.Vartype() == 'C')
		{
			if (lpSP->vParmValue.Len() < VFP2C_ODBC_MAX_FETCH_BUFFER)
			{
				if (lpSP->nParmType == SQL_PARAM_INPUT_OUTPUT_STREAM)
					lpSP->vParmValue.UnlockHandle();
				if (!lpSP->vParmValue.SetHandleSize(VFP2C_ODBC_MAX_FETCH_BUFFER))
					throw E_INSUFMEMORY;
				nBufferSize = VFP2C_ODBC_MAX_FETCH_BUFFER;
				lpData = lpSP->vParmValue.HandleToPtr<SQLPOINTER>();
			}
			else
			{
				nBufferSize = lpSP->vParmValue.Len();
				lpData = lpSP->pParmPtr ? lpSP->pParmPtr : lpSP->vParmValue.HandleToPtr<SQLPOINTER>();
			}
		}
		else
		{
			if (lpSP->vMemoValue.Vartype() != 'C')
			{
				lpSP->vMemoValue.SetString();
				if (!lpSP->vMemoValue.AllocHandle(VFP2C_ODBC_MAX_FETCH_BUFFER))
					throw E_INSUFMEMORY;
				nBufferSize = VFP2C_ODBC_MAX_FETCH_BUFFER;
			}
			else
				nBufferSize = lpSP->vMemoValue.GetHandleSize();

			lpData = lpSP->vMemoValue.HandleToPtr<SQLPOINTER>();
		}

		SQLLEN nDataSize = nBufferSize;
		if (lpSP->nSQLType == SQL_VARCHAR)
			nDataSize -= 1;
		else if (lpSP->nSQLType == SQL_WVARCHAR)
			nDataSize -= 2;

		nApiRet = SQLGetData(hStmt, lpSP->nParmNo, lpSP->nCType, lpData, nBufferSize, &lpSP->nIndicator);
		if (nApiRet == SQL_SUCCESS_WITH_INFO)
		{
			if (lpSP->nSQLType == SQL_VARCHAR)
				lpSP->nIndicator += 1;
			else if (lpSP->nSQLType == SQL_WVARCHAR)
				lpSP->nIndicator += 2;

			if (lpSP->vParmValue.Vartype() == 'C')
			{
				if (!lpSP->vParmValue.SetHandleSize(lpSP->nIndicator))
					throw E_INSUFMEMORY;
				lpData = lpSP->vParmValue.HandleToPtr<SQLPOINTER>();
			}
			else
			{
				if (!lpSP->vMemoValue.SetHandleSize(lpSP->nIndicator))
					throw E_INSUFMEMORY;
				lpData = lpSP->vMemoValue.HandleToPtr<SQLPOINTER>();
			}

			lpData = AdvancePtr<SQLPOINTER>(lpData, nDataSize);
			nApiRet = SQLGetData(hStmt, lpSP->nParmNo, lpSP->nCType, lpData, lpSP->nIndicator - nDataSize, &lpSP->nIndicator);
		}
		if (nApiRet == SQL_ERROR)
		{
			SaveODBCError("SQLGetData", hStmt);
			throw E_APIERROR;
		}
		nApiRet = SQLParamData(hStmt, (SQLPOINTER*)&lpSP);
	}
	if (nApiRet == SQL_ERROR)
	{
		SaveODBCError("SQLParamData", hStmt);
		throw E_APIERROR;
	}
}

void SqlStatement::SaveOutputParameters()
{
	if (!bOutputParams)
		return;

	SqlParameter* lpSP;
	Value vNull = { '0' };
	char cType;

	for (int xj = 0; xj < pParameters.GetCount(); xj++)
	{
		lpSP = pParameters[xj];

		if (lpSP->nParmType == SQL_PARAM_INPUT)
			continue;

		if (lpSP->nIndicator == SQL_NULL_DATA)
		{
			lpSP->lVarOrField = vNull;
			continue;
		}

		cType = lpSP->vParmValue.Vartype();
		if (cType == '0')
			cType = (char)lpSP->vParmValue.ev_long;

		switch (cType)
		{

		case 'L':
		case 'I':
		case 'N':
			lpSP->lVarOrField = lpSP->vParmValue;
			break;

		case 'D':
			Date_StructToValue(&lpSP->sDateStruct, lpSP->vParmValue);
			lpSP->lVarOrField = lpSP->vParmValue;
			break;

		case 'T':
			Timestamp_StructToValue(&lpSP->sTimestampStruct, lpSP->vParmValue);
			lpSP->lVarOrField = lpSP->vParmValue;
			break;

		case 'C':
		case 'V':
		case 'Q':
			lpSP->vParmValue.ev_length = lpSP->nIndicator;
			lpSP->lVarOrField = lpSP->vParmValue;
			break;

		case 'M':
		case 'W':
		case 'G':
			lpSP->vMemoValue.ev_length = lpSP->nIndicator;
			lpSP->lVarOrField = lpSP->vMemoValue;
			break;

		case 'Y':
			Numeric_StructToValue(&lpSP->sNumericStruct, lpSP->vParmValue);
			lpSP->lVarOrField = lpSP->vParmValue;
			break;

		default:
			throw E_APIERROR;
		}
	}
}

void SqlStatement::ProgressCallback(int nRowsFetched, bool* bAbort)
{
	ValueEx vCallRet;
	vCallRet = 0;
	int nErrorNo;

	if (nErrorNo = pCallback.Evaluate(vCallRet, nResultset, nRowsFetched, nRowsTotal))
		throw nErrorNo;

	if (vCallRet.Vartype() == 'L')
		*bAbort = !vCallRet.ev_length;
	else if (vCallRet.Numeric())
		*bAbort = vCallRet.NumericCast<DWORD>() == 0;
	else
	{
		*bAbort = false;
		vCallRet.Release();
	}

	if (*bAbort)
		throw E_APIERROR;
}

bool SqlStatement::InfoCallbackOrStore()
{
	SQLRETURN nApiRet;
	SQLSMALLINT nMsgLen, nError = 1;
	SQLINTEGER nNativeError;
	SQLCHAR aSQLState[16];

	if (nFlags & (SQLEXECEX_CALLBACK_INFO | SQLEXECEX_STORE_INFO))
	{
		pGetDataBuffer.Size(VFP2C_ODBC_MAX_VARIABLE_BUFFER);
		nApiRet = SQLGetDiagRec(SQL_HANDLE_STMT, hStmt, nError++, aSQLState, &nNativeError,
			pGetDataBuffer.Ptr<SQLCHAR*>(), VFP2C_ODBC_MAX_VARIABLE_BUFFER, &nMsgLen);

		while (nApiRet == SQL_SUCCESS)
		{
			pGetDataBuffer.Len(ExtractInfo(pGetDataBuffer, nMsgLen));
			if (nFlags & SQLEXECEX_STORE_INFO)
			{
				int nIndex = pInfoArray.Grow();
				pInfoArray(nIndex, 1) = pGetDataBuffer;
				if (nFlags & SQLEXECEX_PARAMETER_ARRAY)
					pInfoArray(nIndex, 2) = pParameterArray.nParamsProcessed;
			}
			else if (nFlags & SQLEXECEX_CALLBACK_INFO)
			{
				CStringView pInfo = pGetDataBuffer;
				if (nFlags & SQLEXECEX_PARAMETER_ARRAY)
					pCallback.Execute(-1, pInfo, pParameterArray.nParamsProcessed);
				else
					pCallback.Execute(-1, pInfo);
			}
			nApiRet = SQLGetDiagRec(SQL_HANDLE_STMT, hStmt, nError++, aSQLState, &nNativeError,
				pGetDataBuffer.Ptr<SQLCHAR*>(), pGetDataBuffer.Size(), &nMsgLen);
		}
		return true;
	}
	return false;
}

unsigned int SqlStatement::ExtractInfo(char* pMessage, unsigned int nMsgLen)
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
		if (pStart != pMessage)
			memmove(pStart, pMessage, nLength); // move the content to the start of the string ..
	}

	return nLength;
}

void SqlStatement::LoadDriverHandles(HMODULE& hDriver, SQLHSTMT& hDriverStmt, SQLHSTMT hStmt)
{
	SQLRETURN nApiRet = SQLGetInfo(hConn, SQL_DRIVER_HLIB, &hDriver, 0, 0);
	if (nApiRet == SQL_ERROR)
	{
		SaveODBCError("SQLGetInfo", hConn);
		throw E_APIERROR;
	}
	else if (nApiRet == SQL_SUCCESS_WITH_INFO && bStoreInfoAsError)
		AddODBCError("SQLGetInfo", hConn);

	hDriverStmt = hStmt;
	nApiRet = SQLGetInfo(hConn, SQL_DRIVER_HSTMT, &hDriverStmt, 0, 0);
	if (nApiRet == SQL_ERROR)
	{
		SaveODBCError("SQLGetInfo", hConn);
		throw E_APIERROR;
	}
	else if (nApiRet == SQL_SUCCESS_WITH_INFO && bStoreInfoAsError)
		AddODBCError("SQLGetInfo", hConn);
}

SqlResultSet::SqlResultSet()
{
	pStmt = 0;
	hAppRowDesc = 0;
	nWorkArea = 0;
	bRebindColumn = false;
}

SqlResultSet::~SqlResultSet()
{
	FreeColumnBinding();
	ReleaseColumnBuffers();
}

void SqlResultSet::ReleaseColumnBuffers()
{
	SqlColumn* lpSC;
	bool bFreeColumnRowSet = pStmt && pStmt->nRowSetSize > 1;
	if (bFreeColumnRowSet == false)
	{
		for (int xj = 0; xj < pColumns.GetCount(); xj++)
		{
			lpSC = pColumns[xj];
			if (lpSC->vData.Vartype() == 'C' && lpSC->vData.ValidHandle())
			{
				lpSC->vData.UnlockHandle();
				lpSC->vData.FreeHandle();
			}
		}
	}
	else
	{
		for (int xj = 0; xj < pColumns.GetCount(); xj++)
		{
			lpSC = pColumns[xj];
			if (lpSC->pDataPtr)
				delete lpSC->pDataPtr;
			if (lpSC->nIndicator)
				delete (SQLLEN*)lpSC->nIndicator;
		}
	}
}

void SqlResultSet::FreeColumnBinding()
{
	if (hAppRowDesc)
	{
		SQLRETURN nApiRet = SQLFreeHandle(SQL_HANDLE_DESC, hAppRowDesc);
		hAppRowDesc = 0;
	}
}

void SqlResultSet::AllocateColumns(SQLSMALLINT nColCount)
{
	if (pColumns.GetCount() != nColCount)
	{
		FreeColumnBinding();
		ReleaseColumnBuffers();
		pColumns.Free();
		pColumns.Allocate(nColCount);
	}
}

void SqlResultSet::GetColumnMetaData()
{
	if (pStmt->bExecutedOnce)
		return;

	SQLRETURN nApiRet = SQL_SUCCESS;
	SqlColumn* lpSC;
	SQLSMALLINT xj, nColLen;
	int nUnnamedCol = 0;
	bRowSetFetchCapable = true;

	for (xj = 1; xj <= pColumns.GetCount(); xj++)
	{
		lpSC = pColumns[xj - 1];
		nApiRet = SQLDescribeCol(pStmt->hStmt, xj, lpSC->pColumnName.Ptr<SQLCHAR*>(), lpSC->pColumnName.Size(),
			&nColLen, &lpSC->nSQLType, &lpSC->nSize, &lpSC->nScale, &lpSC->bNullable);
		lpSC->pColumnName.Len(nColLen, true);
		if (nApiRet == SQL_ERROR)
		{
			SaveODBCError("SQLDescribeCol", pStmt->hStmt);
			throw E_APIERROR;
		}
		else if (nApiRet == SQL_SUCCESS_WITH_INFO && pStmt->bStoreInfoAsError)
			AddODBCError("SQLDescribeCol", pStmt->hStmt);

		switch (lpSC->nSQLType)
		{
		case SQL_INTEGER:
		case SQL_SMALLINT:
		case SQL_TINYINT:
			// if integer type check sign
			nApiRet = SQLColAttribute(pStmt->hStmt, xj, SQL_DESC_UNSIGNED, 0, 0, 0, &lpSC->bUnsigned);
			if (nApiRet == SQL_ERROR)
			{
				SaveODBCError("SQLColAttribute", pStmt->hStmt);
				throw E_APIERROR;
			}
			else if (nApiRet == SQL_SUCCESS_WITH_INFO && pStmt->bStoreInfoAsError)
				AddODBCError("SQLColAttribute", pStmt->hStmt);
			break;

		case SQL_NUMERIC:
		case SQL_DECIMAL:
			// if numeric or decimal check for money datatype, add 2 to the size, if NUMERIC or DECIMAL since FoxPro N datatype defines size including scale and decimal point
			nApiRet = SQLColAttribute(pStmt->hStmt, xj, SQL_COLUMN_MONEY, 0, 0, 0, &lpSC->bMoney);
			if (nApiRet == SQL_ERROR)
			{
				SaveODBCError("SQLColAttribute", pStmt->hStmt);
				throw E_APIERROR;
			}
			else if (nApiRet == SQL_SUCCESS_WITH_INFO && pStmt->bStoreInfoAsError)
				AddODBCError("SQLColAttribute", pStmt->hStmt);

			if (!lpSC->bMoney)
				lpSC->nSize += 2;
			break;

		case SQL_REAL:
		case SQL_FLOAT:
		case SQL_DOUBLE:
			lpSC->nSize = 20;
			lpSC->nScale = pStmt->GetDefaultDecimals();
			break;

		case SQL_CHAR:
		case SQL_VARCHAR:
		case SQL_BINARY:
		case SQL_VARBINARY:
		case SQL_SS_UDT:
		case SQL_SS_VARIANT:
		case SQL_BIGINT:
		case SQL_BIT:
		case SQL_GUID:
			break;

		case SQL_WCHAR:
		case SQL_WVARCHAR:
			nApiRet = SQLColAttribute(pStmt->hStmt, xj, SQL_DESC_DISPLAY_SIZE, 0, 0, 0, &lpSC->nDisplaySize);
			if (nApiRet == SQL_ERROR)
			{
				SaveODBCError("SQLColAttribute", pStmt->hStmt);
				throw E_APIERROR;
			}
			else if (nApiRet == SQL_SUCCESS_WITH_INFO && pStmt->bStoreInfoAsError)
				AddODBCError("SQLColAttribute", pStmt->hStmt);
			break;

		case SQL_LONGVARCHAR:
		case SQL_LONGVARBINARY:
		case SQL_WLONGVARCHAR:
			bRowSetFetchCapable = false;
			break;

		/*
		case SQL_SS_XML:
		case SQL_SS_TIME2:
		case SQL_SS_TIMESTAMPOFFSET:
			break;
		*/

		case SQL_DATETIME:
		case SQL_TIMESTAMP:
		case SQL_TYPE_DATE:
		case SQL_TYPE_TIMESTAMP:
			if (lpSC->nScale == 0)
				break;

		default:
			nApiRet = SQLColAttribute(pStmt->hStmt, xj, SQL_DESC_DISPLAY_SIZE, 0, 0, 0, &lpSC->nDisplaySize);
			if (nApiRet == SQL_ERROR)
			{
				SaveODBCError("SQLColAttribute", pStmt->hStmt);
				throw E_APIERROR;
			}
			else if (nApiRet == SQL_SUCCESS_WITH_INFO && pStmt->bStoreInfoAsError)
				AddODBCError("SQLColAttribute", pStmt->hStmt);

			if (lpSC->nDisplaySize > 8000)
				bRowSetFetchCapable = false;
		}

		// empty column name (from an expression for example)
		if (lpSC->pColumnName.Len() == 0)
		{
			if (nUnnamedCol)
				lpSC->pColumnName.Format("expr%I", nUnnamedCol);
			else
				lpSC->pColumnName = "expr";

			nUnnamedCol++;
		}
		else
			lpSC->pColumnName.Len(FixCursorColumnName(lpSC->pColumnName), true);
	}
}

void SqlResultSet::BindColumns()
{
	SQLRETURN nApiRet;
	SqlColumn* lpSC;
	SQLHDESCPARAM hImplicitAppRowDesc;
	if (pStmt->bExecutedOnce && !bRebindColumn)
		return;

	if (hAppRowDesc == false)
	{
		if (pStmt->nRowSetSize > 1)
		{
			if (!bRowSetFetchCapable)
			{
				SaveCustomError("SQLExecEx", "A RowsetSize parameter > 1 is incompatible with resultsets than contain memo fields.");
				throw E_APIERROR;
			}

			nApiRet = SQLSetStmtAttr(pStmt->hStmt, SQL_ATTR_ROW_BIND_TYPE, SQL_BIND_BY_COLUMN, 0);
			if (nApiRet == SQL_ERROR)
			{
				SaveODBCError("SQLSetStmtAttr", pStmt->hStmt);
				throw E_APIERROR;
			}
			else if (nApiRet == SQL_SUCCESS_WITH_INFO && pStmt->bStoreInfoAsError)
				AddODBCError("SQLSetStmtAttr", pStmt->hStmt);

			nApiRet = SQLSetStmtAttr(pStmt->hStmt, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER)pStmt->nRowSetSize, 0);
			if (nApiRet == SQL_ERROR)
			{
				SaveODBCError("SQLSetStmtAttr", pStmt->hStmt);
				throw E_APIERROR;
			}
			else if (nApiRet == SQL_SUCCESS_WITH_INFO && pStmt->bStoreInfoAsError)
				AddODBCError("SQLSetStmtAttr", pStmt->hStmt);

			pStmt->pRowStatus.Reserve(pStmt->nRowSetSize, false, true);
			nApiRet = SQLSetStmtAttr(pStmt->hStmt, SQL_ATTR_ROW_STATUS_PTR, (SQLPOINTER)&pStmt->pRowStatus[0], 0);
			if (nApiRet == SQL_ERROR)
			{
				SaveODBCError("SQLSetStmtAttr", pStmt->hStmt);
				throw E_APIERROR;
			}
			else if (nApiRet == SQL_SUCCESS_WITH_INFO && pStmt->bStoreInfoAsError)
				AddODBCError("SQLSetStmtAttr", pStmt->hStmt);

			nApiRet = SQLSetStmtAttr(pStmt->hStmt, SQL_ATTR_ROWS_FETCHED_PTR, (SQLPOINTER)&pStmt->nRowsFetched, 0);
			if (nApiRet == SQL_ERROR)
			{
				SaveODBCError("SQLSetStmtAttr", pStmt->hStmt);
				throw E_APIERROR;
			}
			else if (nApiRet == SQL_SUCCESS_WITH_INFO && pStmt->bStoreInfoAsError)
				AddODBCError("SQLSetStmtAttr", pStmt->hStmt);
		}

		for (int xj = 0; xj < pColumns.GetCount(); xj++)
		{
			lpSC = pColumns[xj];
			if (lpSC->bBindColumn)
			{
				nApiRet = SQLBindCol(pStmt->hStmt, lpSC->nColNo, lpSC->nCType, lpSC->pDataPtr, lpSC->nBufferSize, pStmt->nRowSetSize == 1 ? &lpSC->nIndicator : (SQLLEN*)lpSC->nIndicator);
				if (nApiRet == SQL_ERROR)
				{
					SaveODBCError("SQLBindCol", pStmt->hStmt);
					throw E_APIERROR;
				}
				else if (nApiRet == SQL_SUCCESS_WITH_INFO && pStmt->bStoreInfoAsError)
					AddODBCError("SQLBindCol", pStmt->hStmt);
			}

			
			if (lpSC->nCType == SQL_C_NUMERIC)
			{
				if (hImplicitAppRowDesc == false)
				{
					nApiRet = SQLGetStmtAttr(pStmt->hStmt, SQL_ATTR_APP_ROW_DESC, &hImplicitAppRowDesc, 0, 0);
					if (nApiRet == SQL_ERROR)
					{
						SaveODBCError("SQLGetStmtAttr", pStmt->hStmt);
						throw E_APIERROR;
					}
					else if (nApiRet == SQL_SUCCESS_WITH_INFO && pStmt->bStoreInfoAsError)
						AddODBCError("SQLGetStmtAttr", pStmt->hStmt);
				}

				nApiRet = SQLSetDescField(hImplicitAppRowDesc, lpSC->nColNo, SQL_DESC_PRECISION, (SQLPOINTER)lpSC->nSize, 0);
				if (nApiRet == SQL_ERROR)
				{
					SaveODBCError("SQLSetDescField", hImplicitAppRowDesc);
					throw E_APIERROR;
				}
				else if (nApiRet == SQL_SUCCESS_WITH_INFO && pStmt->bStoreInfoAsError)
					AddODBCError("SQLSetDescField", hImplicitAppRowDesc);

				nApiRet = SQLSetDescField(hImplicitAppRowDesc, lpSC->nColNo, SQL_DESC_SCALE, (SQLPOINTER)lpSC->nScale, 0);
				if (nApiRet == SQL_ERROR)
				{
					SaveODBCError("SQLSetDescField", hImplicitAppRowDesc);
					throw E_APIERROR;
				}
				else if (nApiRet == SQL_SUCCESS_WITH_INFO && pStmt->bStoreInfoAsError)
					AddODBCError("SQLSetDescField", hImplicitAppRowDesc);

				nApiRet = SQLSetDescField(hImplicitAppRowDesc, lpSC->nColNo, SQL_DESC_DATA_PTR, lpSC->pDataPtr, 0);
				if (nApiRet == SQL_ERROR)
				{
					SaveODBCError("SQLSetDescField", hImplicitAppRowDesc);
					throw E_APIERROR;
				}
				else if (nApiRet == SQL_SUCCESS_WITH_INFO && pStmt->bStoreInfoAsError)
					AddODBCError("SQLSetDescField", hImplicitAppRowDesc);
			}
		}
	}
	else
	{
		nApiRet = SQLSetStmtAttr(pStmt->hStmt, SQL_ATTR_APP_ROW_DESC, (SQLPOINTER)hAppRowDesc, SQL_IS_POINTER);
		if (nApiRet == SQL_ERROR)
		{
			SaveODBCError("SQLSetStmtAttr", pStmt->hStmt);
			throw E_APIERROR;
		}
		else if (nApiRet == SQL_SUCCESS_WITH_INFO && pStmt->bStoreInfoAsError)
			AddODBCError("SQLSetStmtAttr", pStmt->hStmt);
	}
}

void SqlResultSet::SaveColumnBinding()
{
	SQLRETURN nApiRet;
	SQLHDESC hImplicitAppRowDesc;
	if ((!pStmt->bPrepared && !(pStmt->nFlags & SQLEXECEX_PARAMETER_ARRAY)) || (hAppRowDesc == true))
		return;

	bRebindColumn = true;
	nApiRet = SQLGetStmtAttr(pStmt->hStmt, SQL_ATTR_APP_ROW_DESC, &hImplicitAppRowDesc, 0, 0);
	if (nApiRet == SQL_ERROR)
	{
		SaveODBCError("SQLGetStmtAttr", pStmt->hStmt);
		throw E_APIERROR;
	}
	else if (nApiRet == SQL_SUCCESS_WITH_INFO && pStmt->bStoreInfoAsError)
		AddODBCError("SQLGetStmtAttr", pStmt->hStmt);

	nApiRet = SQLAllocHandle(SQL_HANDLE_DESC, pStmt->hConn, hAppRowDesc);
	if (nApiRet == SQL_ERROR)
	{
		SaveODBCError("SQLAllocHandle", pStmt->hConn);
		throw E_APIERROR;
	}
	else if (nApiRet == SQL_SUCCESS_WITH_INFO && pStmt->bStoreInfoAsError)
		AddODBCError("SQLAllocHandle", pStmt->hConn);

	nApiRet = SQLCopyDesc(hImplicitAppRowDesc, hAppRowDesc);
	if (nApiRet == SQL_ERROR)
	{
		SaveODBCError("SQLCopyDesc", hAppRowDesc);
		throw E_APIERROR;
	}
	else if (nApiRet == SQL_SUCCESS_WITH_INFO && pStmt->bStoreInfoAsError)
		AddODBCError("SQLCopyDesc", hAppRowDesc);
}

void SqlResultSet::PrepareColumnBindings()
{
	if (pStmt->bExecutedOnce)
		return;

	SQLRETURN nApiRet;
	SQLUINTEGER nGetDataExt;
	SqlColumn* lpSC;
	bool bBindCol = true, bGetData;

	// does the ODBC driver support calling of SQLBindColumn for columns that are to the right
	// of columns that need to be retrieved by SQLGetData?
	// if so we can bind any fixed length column and only need to call
	// SQLGetData for non-fixed length columns
	// if no extension is supported, we need to call SQLGetData for each column
	// after the first column that doesn't support binding ..
	// all this is done mainly for performance reasons since binding columns is many times
	// faster than calling SQLGetData ..
	nApiRet = SQLGetInfo(pStmt->hConn, SQL_GETDATA_EXTENSIONS, &nGetDataExt, sizeof(SQLUINTEGER), 0);
	if (nApiRet == SQL_SUCCESS_WITH_INFO)
	{
		if (pStmt->bStoreInfoAsError)
			AddODBCError("SQLGetInfo", pStmt->hConn);
	}
	else if (nApiRet == SQL_ERROR)
	{
		SaveODBCError("SQLGetInfo", pStmt->hConn);
		throw E_APIERROR;
	}
	bGetData = (nGetDataExt & SQL_GD_ANY_COLUMN) > 0;

	for (int xj = 0; xj < pColumns.GetCount(); xj++)
	{

		lpSC = pColumns[xj];
		lpSC->nColNo = xj + 1;
		lpSC->pStmt = pStmt;
		lpSC->bStoreInfoAsError = pStmt->bStoreInfoAsError;

		switch (lpSC->nSQLType)
		{
			// character data
		case SQL_CHAR:
			lpSC->nCType = SQL_C_CHAR;
			if (!lpSC->bCustomSchema)
			{
				if (lpSC->nSize > 0 && lpSC->nSize < pStmt->nUseMemoSize)
				{
					lpSC->cVFPType = 'C';
					if (pStmt->nRowSetSize == 1)
					{
						AllocateColumnBuffer(lpSC, lpSC->nSize + 1, bBindCol);
						if (!lpSC->lField.IsVariable())
							lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
						else
							lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
					}
					else
					{
						AllocateColumnBufferSet(lpSC, lpSC->nSize + 1);
						lpSC->pStoreSet = SQLStoreCharByBindingSet;
					}
				}
				else
				{
					lpSC->cVFPType = 'M';
					if (pStmt->nRowSetSize == 1)
					{
						pStmt->pGetDataBuffer.Size(VFP2C_ODBC_MAX_FETCH_BUFFER);
						bBindCol = bGetData;
						if (!lpSC->lField.IsVariable())
							lpSC->pStore = SQLStoreMemoChar;
						else
							lpSC->pStore = SQLStoreMemoCharVar;
					}
					else
					{
						pStmt->pGetDataBuffer.Size(lpSC->nSize);
						AllocateColumnBufferSet(lpSC, lpSC->nSize + 1);
						lpSC->pStoreSet = SQLStoreMemoCharByBindingSet;
					}
				}
			}
			else
			{
				if (lpSC->cVFPType == 'M' || lpSC->cVFPType == 'W')
				{
					pStmt->pGetDataBuffer.Size(lpSC->nSize);
					if (pStmt->nRowSetSize == 1)
					{
						bBindCol = bGetData;
						if (!lpSC->lField.IsVariable())
							lpSC->pStore = SQLStoreMemoChar;
						else
							lpSC->pStore = SQLStoreMemoCharVar;
					}
					else
					{
						AllocateColumnBufferSet(lpSC, lpSC->nSize + 1);
						lpSC->pStoreSet = SQLStoreMemoCharByBindingSet;
					}
				}
				else // if (lpSC->cVFPType == 'C' || lpSC->cVFPType == 'V' || lpSC->cVFPType == 'Q')
				{
					if (pStmt->nRowSetSize == 1)
					{
						AllocateColumnBuffer(lpSC, lpSC->nSize + 1, bBindCol);
						if (!lpSC->lField.IsVariable())
							lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
						else
							lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
					}
					else
					{
						AllocateColumnBufferSet(lpSC, lpSC->nSize + 1);
						lpSC->pStoreSet = SQLStoreCharByBindingSet;
					}
				}
			}
			break;

		case SQL_BINARY:
			lpSC->nCType = SQL_C_BINARY;
			if (!lpSC->bCustomSchema)
			{
				if (pStmt->bMapVarbinary && lpSC->nSize > 0 && lpSC->nSize < pStmt->nUseMemoSize)
				{
					if (CFoxVersion::MajorVersion() >= 9)
						lpSC->cVFPType = 'Q';
					else
					{
						lpSC->cVFPType = 'C';
						lpSC->bBinary = TRUE;
					}

					if (pStmt->nRowSetSize == 1)
					{
						AllocateColumnBuffer(lpSC, lpSC->nSize, bBindCol);
						if (!lpSC->lField.IsVariable())
							lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
						else
							lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
					}
					else
					{
						AllocateColumnBufferSet(lpSC, lpSC->nSize);
						lpSC->pStoreSet = SQLStoreCharByBindingSet;
					}
				}
				else
				{
					if (pStmt->bMapVarbinary == false)
					{
						if (lpSC->nSize == 0)
						{
							lpSC->cVFPType = 'G';
						}
						else
						{
							lpSC->cVFPType = 'M';
							lpSC->bBinary = TRUE;
						}
					}
					else
					{
						if (CFoxVersion::MajorVersion() >= 9)
							lpSC->cVFPType = 'W';
						else
						{
							lpSC->cVFPType = 'M';
							lpSC->bBinary = TRUE;
						}
					}

					if (pStmt->nRowSetSize == 1)
					{
						pStmt->pGetDataBuffer.Size(lpSC->nSize);
						bBindCol = bGetData;
						if (!lpSC->lField.IsVariable())
							lpSC->pStore = SQLStoreMemoBinary;
						else
							lpSC->pStore = SQLStoreMemoBinaryVar;
					}
					else
					{
						pStmt->pGetDataBuffer.Size(lpSC->nSize);
						AllocateColumnBufferSet(lpSC, lpSC->nSize);
						lpSC->pStoreSet = SQLStoreMemoBinaryByBindingSet;
					}
				}
			}
			else
			{
				if (lpSC->cVFPType == 'M' || lpSC->cVFPType == 'W')
				{
					if (pStmt->nRowSetSize == 1)
					{
						pStmt->pGetDataBuffer.Size(lpSC->nSize);
						bBindCol = bGetData;
						if (!lpSC->lField.IsVariable())
							lpSC->pStore = SQLStoreMemoBinary;
						else
							lpSC->pStore = SQLStoreMemoBinaryVar;
					}
					else
					{
						pStmt->pGetDataBuffer.Size(lpSC->nSize);
						AllocateColumnBufferSet(lpSC, lpSC->nSize);
						lpSC->pStoreSet = SQLStoreMemoBinaryByBindingSet;
					}
				}
				else
				{
					if (pStmt->nRowSetSize == 1)
					{
						AllocateColumnBuffer(lpSC, lpSC->nSize, bBindCol);
						if (!lpSC->lField.IsVariable())
							lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
						else
							lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
					}
					else
					{
						AllocateColumnBufferSet(lpSC, lpSC->nSize);
						lpSC->pStoreSet = SQLStoreCharByBindingSet;
					}
				}
			}
			break;

		case SQL_VARCHAR:
			lpSC->nCType = SQL_C_CHAR;
			if (!lpSC->bCustomSchema)
			{
				if (lpSC->nSize > 0 && lpSC->nSize < pStmt->nUseMemoSize)
				{
					if (CFoxVersion::MajorVersion() >= 9)
						lpSC->cVFPType = pStmt->bMapVarchar ? 'V' : 'C';
					else
						lpSC->cVFPType = 'C';

					if (pStmt->nRowSetSize == 1)
					{
						AllocateColumnBuffer(lpSC, lpSC->nSize + 1, bBindCol);
						if (!lpSC->lField.IsVariable())
							lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
						else
							lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
					}
					else
					{
						AllocateColumnBufferSet(lpSC, lpSC->nSize + 1);
						lpSC->pStoreSet = SQLStoreCharByBindingSet;
					}
				}
				else
				{
					lpSC->cVFPType = 'M';
					if (pStmt->nRowSetSize == 1)
					{
						pStmt->pGetDataBuffer.Size(lpSC->nSize);
						bBindCol = bGetData;
						if (!lpSC->lField.IsVariable())
							lpSC->pStore = SQLStoreMemoChar;
						else
							lpSC->pStore = SQLStoreMemoCharVar;
					}
					else
					{
						AllocateColumnBufferSet(lpSC, lpSC->nSize);
						lpSC->pStoreSet = SQLStoreMemoCharByBindingSet;
					}
				}
			}
			else
			{
				if (lpSC->cVFPType == 'M' || lpSC->cVFPType == 'W')
				{
					if (pStmt->nRowSetSize == 1)
					{
						pStmt->pGetDataBuffer.Size(VFP2C_ODBC_MAX_FETCH_BUFFER);
						bBindCol = bGetData;
						if (!lpSC->lField.IsVariable())
							lpSC->pStore = SQLStoreMemoChar;
						else
							lpSC->pStore = SQLStoreMemoCharVar;
					}
					else
					{
						AllocateColumnBufferSet(lpSC, lpSC->nSize);
						lpSC->pStoreSet = SQLStoreMemoCharByBindingSet;
					}
				}
				else
				{
					if (pStmt->nRowSetSize == 1)
					{
						AllocateColumnBuffer(lpSC, lpSC->nSize + 1, bBindCol);
						if (!lpSC->lField.IsVariable())
							lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
						else
							lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
					}
					else
					{
						AllocateColumnBufferSet(lpSC, lpSC->nSize);
						lpSC->pStoreSet = SQLStoreCharByBindingSet;
					}
				}
			}
			break;

		case SQL_VARBINARY:
			lpSC->nCType = SQL_C_BINARY;
			if (!lpSC->bCustomSchema)
			{
				if (pStmt->bMapVarbinary && lpSC->nSize > 0 && lpSC->nSize < pStmt->nUseMemoSize)
				{
					if (CFoxVersion::MajorVersion() >= 9)
						lpSC->cVFPType = 'Q';
					else
					{
						lpSC->cVFPType = 'C';
						lpSC->bBinary = TRUE;
					}

					if (pStmt->nRowSetSize == 1)
					{
						AllocateColumnBuffer(lpSC, lpSC->nSize, bBindCol);
						if (!lpSC->lField.IsVariable())
							lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
						else
							lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
					}
					else
					{
						AllocateColumnBufferSet(lpSC, lpSC->nSize);
						lpSC->pStoreSet = SQLStoreCharByBindingSet;
					}
				}
				else
				{
					pStmt->pGetDataBuffer.Size(lpSC->nSize);
					bBindCol = bGetData;
					if (CFoxVersion::MajorVersion() >= 9 && pStmt->bMapVarbinary)
						lpSC->cVFPType = 'W';
					else
					{
						if (lpSC->nSize == 0 && pStmt->bMapBinaryToGeneral)
						{
							lpSC->cVFPType = 'G';
						}
						else
						{
							lpSC->cVFPType = 'M';
							lpSC->bBinary = TRUE;
						}
					}
					if (!lpSC->lField.IsVariable())
						lpSC->pStore = SQLStoreMemoBinary;
					else
						lpSC->pStore = SQLStoreMemoBinaryVar;
				}
			}
			else
			{
				if (lpSC->cVFPType == 'M' || lpSC->cVFPType == 'W')
				{
					pStmt->pGetDataBuffer.Size(VFP2C_ODBC_MAX_FETCH_BUFFER);
					bBindCol = bGetData;
					if (!lpSC->lField.IsVariable())
						lpSC->pStore = SQLStoreMemoBinary;
					else
						lpSC->pStore = SQLStoreMemoBinaryVar;
				}
				else
				{
					AllocateColumnBuffer(lpSC, lpSC->nSize, bBindCol);
					if (!lpSC->lField.IsVariable())
						lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
			}
			break;

		case SQL_LONGVARCHAR:
			lpSC->nCType = SQL_C_CHAR;
			if (!lpSC->bCustomSchema)
			{
				pStmt->pGetDataBuffer.Size(VFP2C_ODBC_MAX_FETCH_BUFFER);
				bBindCol = bGetData;
				lpSC->cVFPType = 'M';
				if (!lpSC->lField.IsVariable())
					lpSC->pStore = SQLStoreMemoChar;
				else
					lpSC->pStore = SQLStoreMemoCharVar;
			}
			else
			{
				if (lpSC->cVFPType == 'M' || lpSC->cVFPType == 'W')
				{
					pStmt->pGetDataBuffer.Size(VFP2C_ODBC_MAX_FETCH_BUFFER);
					bBindCol = bGetData;
					if (!lpSC->lField.IsVariable())
						lpSC->pStore = SQLStoreMemoChar;
					else
						lpSC->pStore = SQLStoreMemoCharVar;
				}
				else
				{
					pStmt->pGetDataBuffer.Size(VFP2C_ODBC_MAX_FETCH_BUFFER);
					bBindCol = bGetData;
					if (!lpSC->lField.IsVariable())
						lpSC->pStore = SQLStoreCharByGetData;
					else
						lpSC->pStore = SQLStoreCharByGetDataVar;
				}
			}
			break;

		case SQL_LONGVARBINARY:
			lpSC->nCType = SQL_C_BINARY;
			if (!lpSC->bCustomSchema)
			{
				pStmt->pGetDataBuffer.Size(VFP2C_ODBC_MAX_FETCH_BUFFER);
				bBindCol = bGetData;
				if (CFoxVersion::MajorVersion() >= 9 && pStmt->bMapVarbinary)
				{
					lpSC->cVFPType = 'W';
				}
				else
				{
					if (!pStmt->bMapBinaryToGeneral)
					{
						lpSC->cVFPType = 'M';
						lpSC->bBinary = TRUE;
					}
					else
						lpSC->cVFPType = 'G';
				}
				if (!lpSC->lField.IsVariable())
					lpSC->pStore = SQLStoreMemoBinary;
				else
					lpSC->pStore = SQLStoreMemoBinaryVar;
			}
			else
			{
				if (lpSC->cVFPType == 'M' || lpSC->cVFPType == 'W' || lpSC->cVFPType == 'G')
				{
					pStmt->pGetDataBuffer.Size(VFP2C_ODBC_MAX_FETCH_BUFFER);
					bBindCol = bGetData;
					if (!lpSC->lField.IsVariable())
						lpSC->pStore = SQLStoreMemoBinary;
					else
						lpSC->pStore = SQLStoreMemoBinaryVar;
				}
				else
				{
					AllocateColumnBuffer(lpSC, lpSC->nSize, bBindCol);
					if (!lpSC->lField.IsVariable())
						lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
			}
			break;

		case SQL_WCHAR:
			if (!lpSC->bCustomSchema)
			{
				if ((pStmt->bUnicodeConversion && lpSC->nSize > 0 && lpSC->nSize < pStmt->nUseMemoSize) ||
					(!pStmt->bUnicodeConversion && lpSC->nDisplaySize > 0 && lpSC->nDisplaySize < (SQLINTEGER)pStmt->nUseMemoSize))
				{
					lpSC->cVFPType = 'C';
					lpSC->nCType = pStmt->bUnicodeConversion ? SQL_C_CHAR : SQL_C_WCHAR;
					if (pStmt->nRowSetSize == 1)
					{
						AllocateColumnBuffer(lpSC, (pStmt->bUnicodeConversion ? lpSC->nSize : lpSC->nDisplaySize) + 2, bBindCol);
						if (!lpSC->lField.IsVariable())
							lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
						else
							lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
					}
					else
					{
						AllocateColumnBufferSet(lpSC, (pStmt->bUnicodeConversion ? lpSC->nSize : lpSC->nDisplaySize) + 2);
						lpSC->pStoreSet = SQLStoreCharByBindingSet;
					}
				}
				else
				{
					pStmt->pGetDataBuffer.Size(VFP2C_ODBC_MAX_FETCH_BUFFER);
					bBindCol = bGetData;
					lpSC->cVFPType = 'M';
					if (pStmt->bUnicodeConversion)
					{
						lpSC->nCType = SQL_C_CHAR;
						lpSC->pStore = !lpSC->lField.IsVariable() ? SQLStoreMemoChar : SQLStoreMemoCharVar;
					}
					else
					{
						lpSC->nCType = SQL_C_WCHAR;
						lpSC->pStore = !lpSC->lField.IsVariable() ? SQLStoreMemoWChar : SQLStoreMemoWCharVar;
					}
				}
			}
			else
			{
				if (lpSC->cVFPType == 'M' || lpSC->cVFPType == 'W')
				{
					pStmt->pGetDataBuffer.Size(VFP2C_ODBC_MAX_FETCH_BUFFER);
					bBindCol = bGetData;
					if (lpSC->cVFPType == 'W' || lpSC->bBinary)
					{
						lpSC->nCType = SQL_C_WCHAR;
						if (!lpSC->lField.IsVariable())
							lpSC->pStore = SQLStoreMemoWChar;
						else
							lpSC->pStore = SQLStoreMemoWCharVar;
					}
					else
					{
						lpSC->nCType = SQL_C_CHAR;
						if (!lpSC->lField.IsVariable())
							lpSC->pStore = SQLStoreMemoChar;
						else
							lpSC->pStore = SQLStoreMemoCharVar;
					}
				}
				else
				{
					if (lpSC->cVFPType == 'Q' || lpSC->bBinary)
						lpSC->nCType = SQL_C_WCHAR;
					else
						lpSC->nCType = SQL_C_CHAR;

					AllocateColumnBuffer(lpSC, (lpSC->nCType == SQL_C_CHAR ? lpSC->nSize : lpSC->nDisplaySize) + 2, bBindCol);
					if (!lpSC->lField.IsVariable())
						lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
			}
			break;

		case SQL_WVARCHAR:
			if (!lpSC->bCustomSchema)
			{
				if ((pStmt->bUnicodeConversion && lpSC->nSize > 0 && lpSC->nSize < pStmt->nUseMemoSize) ||
					(!pStmt->bUnicodeConversion && lpSC->nDisplaySize > 0 && lpSC->nDisplaySize < (SQLINTEGER)pStmt->nUseMemoSize))
				{
					lpSC->cVFPType = pStmt->bMapVarchar ? 'V' : 'C';
					lpSC->nCType = pStmt->bUnicodeConversion ? SQL_C_CHAR : SQL_C_WCHAR;
					if (pStmt->nRowSetSize == 1)
					{
						AllocateColumnBuffer(lpSC, (pStmt->bUnicodeConversion ? lpSC->nSize : lpSC->nDisplaySize) + 2, bBindCol);
						if (!lpSC->lField.IsVariable())
							lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
						else
							lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
					}
					else
					{
						AllocateColumnBufferSet(lpSC, (pStmt->bUnicodeConversion ? lpSC->nSize : lpSC->nDisplaySize) + 2);
						lpSC->pStoreSet = SQLStoreCharByBindingSet;
					}
				}
				else
				{
					pStmt->pGetDataBuffer.Size(VFP2C_ODBC_MAX_FETCH_BUFFER);
					bBindCol = bGetData;
					lpSC->cVFPType = 'M';
					if (pStmt->bUnicodeConversion)
					{
						lpSC->nCType = SQL_C_CHAR;
						if (!lpSC->lField.IsVariable())
							lpSC->pStore = SQLStoreMemoChar;
						else
							lpSC->pStore = SQLStoreMemoCharVar;
					}
					else
					{
						lpSC->nCType = SQL_C_WCHAR;
						if (!lpSC->lField.IsVariable())
							lpSC->pStore = SQLStoreMemoWChar;
						else
							lpSC->pStore = SQLStoreMemoWCharVar;
					}
				}
			}
			else
			{
				if (lpSC->cVFPType == 'M' || lpSC->cVFPType == 'W')
				{
					pStmt->pGetDataBuffer.Size(VFP2C_ODBC_MAX_FETCH_BUFFER);
					bBindCol = bGetData;
					if (lpSC->cVFPType == 'W' || lpSC->bBinary)
					{
						lpSC->nCType = SQL_C_WCHAR;
						if (!lpSC->lField.IsVariable())
							lpSC->pStore = SQLStoreMemoWChar;
						else
							lpSC->pStore = SQLStoreMemoWCharVar;
					}
					else
					{
						lpSC->nCType = SQL_C_CHAR;
						if (!lpSC->lField.IsVariable())
							lpSC->pStore = SQLStoreMemoChar;
						else
							lpSC->pStore = SQLStoreMemoCharVar;
					}
				}
				else
				{
					if (lpSC->cVFPType == 'Q' || lpSC->bBinary)
						lpSC->nCType = SQL_C_WCHAR;
					else
						lpSC->nCType = SQL_C_CHAR;

					AllocateColumnBuffer(lpSC, (lpSC->nCType == SQL_C_CHAR ? lpSC->nSize : lpSC->nDisplaySize) + 2, bBindCol);
					if (!lpSC->lField.IsVariable())
						lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
			}
			break;

		case SQL_WLONGVARCHAR:
			if (!lpSC->bCustomSchema)
			{
				pStmt->pGetDataBuffer.Size(VFP2C_ODBC_MAX_FETCH_BUFFER);
				bBindCol = bGetData;
				lpSC->cVFPType = 'M';
				if (pStmt->bUnicodeConversion)
				{
					lpSC->nCType = SQL_C_CHAR;
					if (!lpSC->lField.IsVariable())
						lpSC->pStore = SQLStoreMemoChar;
					else
						lpSC->pStore = SQLStoreMemoCharVar;
				}
				else
				{
					lpSC->nCType = SQL_C_WCHAR;
					if (!lpSC->lField.IsVariable())
						lpSC->pStore = SQLStoreMemoWChar;
					else
						lpSC->pStore = SQLStoreMemoWCharVar;
				}
			}
			else
			{
				if (lpSC->cVFPType == 'M' || lpSC->cVFPType == 'W')
				{
					pStmt->pGetDataBuffer.Size(VFP2C_ODBC_MAX_FETCH_BUFFER);
					bBindCol = bGetData;
					if (lpSC->cVFPType == 'W' || lpSC->bBinary)
					{
						lpSC->nCType = SQL_C_WCHAR;
						if (!lpSC->lField.IsVariable())
							lpSC->pStore = SQLStoreMemoWChar;
						else
							lpSC->pStore = SQLStoreMemoWCharVar;
					}
					else
					{
						lpSC->nCType = SQL_C_CHAR;
						if (!lpSC->lField.IsVariable())
							lpSC->pStore = SQLStoreMemoChar;
						else
							lpSC->pStore = SQLStoreMemoCharVar;
					}
				}
				else
				{
					AllocateColumnBuffer(lpSC, lpSC->nSize + 2, bBindCol);
					if (lpSC->cVFPType == 'Q' || lpSC->bBinary)
						lpSC->nCType = SQL_C_WCHAR;
					else
						lpSC->nCType = SQL_C_CHAR;
					if (!lpSC->lField.IsVariable())
						lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
			}
			break;

			// bit (logical) data
		case SQL_BIT:
			if (!lpSC->bCustomSchema)
			{
				lpSC->vData.SetLogical();
				lpSC->cVFPType = 'L';
				if (pStmt->nRowSetSize == 1)
				{
					lpSC->nCType = SQL_C_ULONG;
					lpSC->pDataPtr = &lpSC->vData.ev_length;
					lpSC->bBindColumn = bBindCol;
					if (!lpSC->lField.IsVariable())
						lpSC->pStore = bBindCol ? SQLStoreByBinding : SQLStoreByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreByBindingVar : SQLStoreByGetDataVar;
				}
				else
				{
					lpSC->nCType = SQL_C_TINYINT;
					AllocateColumnBufferSet(lpSC, sizeof(char));
					lpSC->pStoreSet = SQLStoreLogicalByBindingSet;

				}
			}
			else
			{
				if (lpSC->cVFPType == 'C' || lpSC->cVFPType == 'V')
				{
					AllocateColumnBuffer(lpSC, lpSC->nSize + 1, bBindCol);
					lpSC->nCType = SQL_C_CHAR;
					if (!lpSC->lField.IsVariable())
						lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
			}
			break;

			// integral numeric types
		case SQL_TINYINT:
		case SQL_SMALLINT:
		case SQL_INTEGER:
			if (!lpSC->bCustomSchema)
			{
				if (lpSC->nSQLType == SQL_INTEGER && lpSC->bUnsigned)
				{
					lpSC->vData.SetUInt();
					lpSC->nCType = SQL_C_DOUBLE;
					lpSC->cVFPType = 'N';
					lpSC->nSize = 10;
					lpSC->nScale = 0;
					if (pStmt->nRowSetSize == 1)
					{
						lpSC->pDataPtr = &lpSC->vData.ev_real;
						lpSC->bBindColumn = bBindCol;
						if (!lpSC->lField.IsVariable())
							lpSC->pStore = bBindCol ? SQLStoreByBinding : SQLStoreByGetData;
						else
							lpSC->pStore = bBindCol ? SQLStoreByBindingVar : SQLStoreByGetDataVar;
					}
					else
					{
						AllocateColumnBufferSet(lpSC, sizeof(double));
						lpSC->pStoreSet = SQLStoreDoubleByBindingSet;
					}
				}
				else
				{
					lpSC->vData.SetInt();
					lpSC->nCType = SQL_C_SLONG;
					lpSC->cVFPType = 'I';
					if (pStmt->nRowSetSize == 1)
					{
						lpSC->pDataPtr = &lpSC->vData.ev_long;
						lpSC->bBindColumn = bBindCol;
						if (!lpSC->lField.IsVariable())
							lpSC->pStore = bBindCol ? SQLStoreByBinding : SQLStoreByGetData;
						else
							lpSC->pStore = bBindCol ? SQLStoreByBindingVar : SQLStoreByGetDataVar;
					}
					else
					{
						AllocateColumnBufferSet(lpSC, sizeof(int));
						lpSC->pStoreSet = SQLStoreIntByBindingSet;
					}
				}
			}
			else
			{
				if (lpSC->cVFPType == 'C' || lpSC->cVFPType == 'V' || lpSC->cVFPType == 'Q')
				{
					AllocateColumnBuffer(lpSC, lpSC->nSize + 1, bBindCol);
					lpSC->nCType = SQL_C_CHAR;
					if (!lpSC->lField.IsVariable())
						lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
				else if (lpSC->cVFPType == 'N' || lpSC->cVFPType == 'F' || lpSC->cVFPType == 'B')
				{
					lpSC->vData.SetNumeric(lpSC->nSize, lpSC->nScale);
					lpSC->pDataPtr = &lpSC->vData.ev_real;
					lpSC->nCType = SQL_C_DOUBLE;
					if (!lpSC->lField.IsVariable())
						lpSC->pStore = bBindCol ? SQLStoreByBinding : SQLStoreByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreByBindingVar : SQLStoreByGetDataVar;
				}
				else if (lpSC->cVFPType == 'I')
				{
					lpSC->vData.SetInt();
					lpSC->pDataPtr = &lpSC->vData.ev_long;
					lpSC->nCType = SQL_C_SLONG;
					if (!lpSC->lField.IsVariable())
						lpSC->pStore = bBindCol ? SQLStoreByBinding : SQLStoreByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreByBindingVar : SQLStoreByGetDataVar;
				}
				else if (lpSC->cVFPType == 'L')
				{
					lpSC->vData.SetLogical();
					lpSC->pDataPtr = &lpSC->vData.ev_length;
					lpSC->nCType = SQL_C_SLONG;
					if (!lpSC->lField.IsVariable())
						lpSC->pStore = bBindCol ? SQLStoreByBinding : SQLStoreByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreByBindingVar : SQLStoreByGetDataVar;
				}
				lpSC->bBindColumn = bBindCol;
			}
			break;

		case SQL_BIGINT:
			if (!lpSC->bCustomSchema)
			{
				lpSC->nSize = VFP2C_ODBC_MAX_BIGINT_LITERAL;
				lpSC->cVFPType = 'C';
				lpSC->nCType = SQL_C_CHAR;
				if (pStmt->nRowSetSize == 1)
				{
					AllocateColumnBuffer(lpSC, VFP2C_ODBC_MAX_BIGINT_LITERAL + 1, bBindCol);
					if (!lpSC->lField.IsVariable())
						lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
				else
				{
					AllocateColumnBufferSet(lpSC, VFP2C_ODBC_MAX_BIGINT_LITERAL + 1);
					lpSC->pStoreSet = SQLStoreCharByBindingSet;
				}
			}
			else
			{
				if (lpSC->cVFPType == 'C' || lpSC->cVFPType == 'V' || lpSC->cVFPType == 'Q')
				{
					AllocateColumnBuffer(lpSC, VFP2C_ODBC_MAX_BIGINT_LITERAL + 1, bBindCol);
					lpSC->nCType = SQL_C_CHAR;
					if (!lpSC->lField.IsVariable())
						lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
			}
			break;

		case SQL_NUMERIC:
		case SQL_DECIMAL:
			if (!lpSC->bCustomSchema)
			{
				if (!lpSC->bMoney)
				{
					if (lpSC->nSize > 20 || lpSC->nScale > 16)
					{
						lpSC->cVFPType = 'C';
						lpSC->nSize = lpSC->nSize + lpSC->nScale + 1 - 2;
						lpSC->nCType = SQL_C_CHAR;
						if (pStmt->nRowSetSize == 1)
						{
							AllocateColumnBuffer(lpSC, VFP2C_ODBC_MAX_NUMERIC_LITERAL, bBindCol);
							if (!lpSC->lField.IsVariable())
								lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
							else
								lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
						}
						else
						{
							AllocateColumnBufferSet(lpSC, VFP2C_ODBC_MAX_NUMERIC_LITERAL);
							lpSC->pStoreSet = SQLStoreCharByBindingSet;
						}
					}
					else
					{
						lpSC->vData.SetNumeric(lpSC->nSize, lpSC->nScale);
						lpSC->nCType = SQL_C_DOUBLE;
						lpSC->cVFPType = 'N';
						if (pStmt->nRowSetSize == 1)
						{
							lpSC->pDataPtr = &lpSC->vData.ev_real;
							lpSC->bBindColumn = bBindCol;
							if (!lpSC->lField.IsVariable())
								lpSC->pStore = bBindCol ? SQLStoreByBinding : SQLStoreByGetData;
							else
								lpSC->pStore = bBindCol ? SQLStoreByBindingVar : SQLStoreByGetDataVar;
						}
						else
						{
							AllocateColumnBufferSet(lpSC, sizeof(double));
							lpSC->pStoreSet = SQLStoreDoubleByBindingSet;
						}
					}
				}
				else
				{
					lpSC->vData.SetCurrency();
					lpSC->cVFPType = 'Y';
					lpSC->nBufferSize = sizeof(SQL_NUMERIC_STRUCT);
					lpSC->nCType = SQL_C_NUMERIC;
					lpSC->nSize = VFP2C_VFP_CURRENCY_PRECISION;
					lpSC->nScale = VFP2C_VFP_CURRENCY_SCALE;
					if (pStmt->nRowSetSize == 1)
					{
						lpSC->pDataPtr = &lpSC->sNumericStruct;
						lpSC->bBindColumn = bBindCol;
						if (!lpSC->lField.IsVariable())
							lpSC->pStore = bBindCol ? SQLStoreCurrencyByBinding : SQLStoreCurrencyByGetData;
						else
							lpSC->pStore = bBindCol ? SQLStoreCurrencyByBindingVar : SQLStoreCurrencyByGetDataVar;
					}
					else
					{
						AllocateColumnBufferSet(lpSC, sizeof(SQL_NUMERIC_STRUCT));
						lpSC->pStoreSet = SQLStoreCurrencyByBindingSet;
					}
				}
			}
			else
			{
				if (lpSC->cVFPType == 'C' || lpSC->cVFPType == 'V')
				{
					AllocateColumnBuffer(lpSC, lpSC->nSize + 1, bBindCol);
					lpSC->nCType = SQL_C_CHAR;
					if (!lpSC->lField.IsVariable())
						lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
				else if (lpSC->cVFPType == 'I')
				{
					lpSC->vData.SetInt();
					lpSC->pDataPtr = &lpSC->vData.ev_long;
					lpSC->nCType = SQL_C_LONG;
					lpSC->bBindColumn = bBindCol;
					if (!lpSC->lField.IsVariable())
						lpSC->pStore = bBindCol ? SQLStoreByBinding : SQLStoreByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreByBindingVar : SQLStoreByGetDataVar;
				}
				else if (lpSC->cVFPType == 'N' || lpSC->cVFPType == 'B' || lpSC->cVFPType == 'F')
				{
					lpSC->vData.SetNumeric(lpSC->cVFPType == 'N' ? (short)lpSC->nSize : 20, lpSC->nScale);
					lpSC->pDataPtr = &lpSC->vData.ev_real;
					lpSC->nCType = SQL_DOUBLE;
					lpSC->bBindColumn = bBindCol;
					if (!lpSC->lField.IsVariable())
						lpSC->pStore = bBindCol ? SQLStoreByBinding : SQLStoreByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreByBindingVar : SQLStoreByGetDataVar;
				}
				else if (lpSC->cVFPType == 'Y')
				{
					lpSC->vData.SetCurrency();
					lpSC->pDataPtr = &lpSC->sNumericStruct;
					lpSC->nBufferSize = sizeof(SQL_NUMERIC_STRUCT);
					lpSC->nCType = SQL_C_NUMERIC;
					lpSC->nSize = VFP2C_VFP_CURRENCY_PRECISION;
					lpSC->nScale = VFP2C_VFP_CURRENCY_SCALE;
					lpSC->cVFPType = 'Y';
					lpSC->bBindColumn = bBindCol;
					if (!lpSC->lField.IsVariable())
						lpSC->pStore = bBindCol ? SQLStoreCurrencyByBinding : SQLStoreCurrencyByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreCurrencyByBindingVar : SQLStoreCurrencyByGetDataVar;
				}
			}
			break;

		case SQL_REAL:
		case SQL_FLOAT:
		case SQL_DOUBLE:
			if (!lpSC->bCustomSchema)
			{
				lpSC->vData.SetDouble();
				lpSC->nCType = SQL_C_DOUBLE;
				lpSC->cVFPType = 'B';
				if (pStmt->nRowSetSize == 1)
				{
					lpSC->pDataPtr = &lpSC->vData.ev_real;
					lpSC->bBindColumn = bBindCol;
					if (!lpSC->lField.IsVariable())
						lpSC->pStore = bBindCol ? SQLStoreByBinding : SQLStoreByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreByBindingVar : SQLStoreByGetDataVar;
				}
				else
				{
					AllocateColumnBufferSet(lpSC, sizeof(double));
					lpSC->pStoreSet = SQLStoreDoubleByBindingSet;
				}
			}
			else
			{
				if (lpSC->cVFPType == 'C' || lpSC->cVFPType == 'V' || lpSC->cVFPType == 'Q')
				{
					AllocateColumnBuffer(lpSC, lpSC->nSize + 1, bBindCol);
					lpSC->nCType = SQL_C_CHAR;
					if (!lpSC->lField.IsVariable())
						lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
				else if (lpSC->cVFPType == 'N' || lpSC->cVFPType == 'B' || lpSC->cVFPType == 'F')
				{
					lpSC->vData.SetNumeric(lpSC->nSize, lpSC->nScale);
					lpSC->pDataPtr = &lpSC->vData.ev_real;
					lpSC->nCType = SQL_C_DOUBLE;
					lpSC->bBindColumn = bBindCol;
					if (!lpSC->lField.IsVariable())
						lpSC->pStore = bBindCol ? SQLStoreByBinding : SQLStoreByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreByBindingVar : SQLStoreByGetDataVar;
				}
			}
			break;

			// this is only a date not a datetime ..	
		case SQL_DATE:
		case SQL_TYPE_DATE:
			if (!lpSC->bCustomSchema)
			{
				lpSC->vData.SetDate();
				lpSC->nCType = SQL_C_DATE;
				lpSC->cVFPType = 'D';
				if (pStmt->nRowSetSize == 1)
				{
					lpSC->pDataPtr = &lpSC->sDateStruct;
					lpSC->bBindColumn = bBindCol;
					if (!lpSC->lField.IsVariable())
						lpSC->pStore = bBindCol ? SQLStoreDateByBinding : SQLStoreDateByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreDateByBindingVar : SQLStoreDateByGetDataVar;
				}
				else
				{
					AllocateColumnBufferSet(lpSC, sizeof(SQL_DATE_STRUCT));
					lpSC->pStoreSet = SQLStoreDateByBindingSet;
				}
			}
			else
			{
				if ((lpSC->cVFPType == 'C' || lpSC->cVFPType == 'V') && !lpSC->bBinary)
				{
					AllocateColumnBuffer(lpSC, lpSC->nSize + 1, bBindCol);
					lpSC->nCType = SQL_C_CHAR;
					if (!lpSC->lField.IsVariable())
						lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
				else if (lpSC->cVFPType == 'Q' ||
					((lpSC->cVFPType == 'C' || lpSC->cVFPType == 'V') && lpSC->bBinary))
				{
					AllocateColumnBuffer(lpSC, sizeof(SQL_TIMESTAMP_STRUCT), bBindCol);
					lpSC->nCType = SQL_C_TIMESTAMP;
					if (!lpSC->lField.IsVariable())
						lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
				else if (lpSC->cVFPType == 'T' || lpSC->cVFPType == 'D')
				{
					if (lpSC->cVFPType == 'T')
						lpSC->vData.SetDateTime();
					else
						lpSC->vData.SetDate();
					lpSC->pDataPtr = &lpSC->sDateStruct;
					lpSC->nCType = SQL_C_DATE;
					if (!lpSC->lField.IsVariable())
						lpSC->pStore = bBindCol ? SQLStoreDateByBinding : SQLStoreDateByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreDateByBindingVar : SQLStoreDateByGetDataVar;
				}
				lpSC->bBindColumn = bBindCol;
			}
			break;

			// this is a datetime
		case SQL_TIMESTAMP:
		case SQL_TYPE_TIMESTAMP:
			if (!lpSC->bCustomSchema)
			{
				if (lpSC->nScale == 0)
				{
					lpSC->vData.SetDateTime();
					lpSC->nCType = SQL_C_TIMESTAMP;
					lpSC->cVFPType = 'T';
					if (pStmt->nRowSetSize == 1)
					{
						lpSC->pDataPtr = &lpSC->sTimestampStruct;
						lpSC->bBindColumn = bBindCol;
						if (!lpSC->lField.IsVariable())
							lpSC->pStore = bBindCol ? SQLStoreDateTimeByBinding : SQLStoreDateTimeByGetData;
						else
							lpSC->pStore = bBindCol ? SQLStoreDateTimeByBindingVar : SQLStoreDateTimeByGetDataVar;
					}
					else
					{
						AllocateColumnBufferSet(lpSC, sizeof(SQL_TIMESTAMP_STRUCT));
						lpSC->pStoreSet = SQLStoreDateTimeByBindingSet;
					}
				}
				else
				{
					lpSC->nSize = lpSC->nDisplaySize;
					lpSC->nCType = SQL_C_CHAR;
					lpSC->cVFPType = 'C';
					if (pStmt->nRowSetSize == 1)
					{
						AllocateColumnBuffer(lpSC, lpSC->nDisplaySize + 1, bBindCol);
						if (!lpSC->lField.IsVariable())
							lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
						else
							lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
					}
					else
					{
						AllocateColumnBufferSet(lpSC, lpSC->nDisplaySize + 1);
						lpSC->pStoreSet = SQLStoreCharByBindingSet;
					}
				}
			}
			else
			{
				if (lpSC->cVFPType == 'D')
				{
					lpSC->vData.SetDate();
					lpSC->pDataPtr = &lpSC->sDateStruct;
					lpSC->nCType = SQL_C_DATE;
					if (!lpSC->lField.IsVariable())
						lpSC->pStore = bBindCol ? SQLStoreDateByBinding : SQLStoreDateByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreDateByBindingVar : SQLStoreDateByGetDataVar;
				}
				else if (lpSC->cVFPType == 'T')
				{
					lpSC->vData.SetDateTime();
					lpSC->pDataPtr = &lpSC->sTimestampStruct;
					lpSC->nCType = SQL_C_TIMESTAMP;
					if (!lpSC->lField.IsVariable())
						lpSC->pStore = bBindCol ? SQLStoreDateTimeByBinding : SQLStoreDateTimeByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreDateTimeByBindingVar : SQLStoreDateTimeByGetDataVar;
				}
				else if (lpSC->cVFPType == 'Q' ||
					((lpSC->cVFPType == 'C' || lpSC->cVFPType == 'V') && lpSC->bBinary))
				{
					AllocateColumnBuffer(lpSC, sizeof(SQL_TIMESTAMP_STRUCT), bBindCol);
					lpSC->nSize = sizeof(SQL_TIMESTAMP_STRUCT);
					lpSC->nCType = SQL_C_TIMESTAMP;
					if (!lpSC->lField.IsVariable())
						lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
				else if (lpSC->cVFPType == 'C' || lpSC->cVFPType == 'V')
				{
					AllocateColumnBuffer(lpSC, lpSC->nSize + 1, bBindCol);
					lpSC->nCType = SQL_C_CHAR;
					if (!lpSC->lField.IsVariable())
						lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
				lpSC->bBindColumn = bBindCol;
			}
			break;

		case SQL_TIME:
		case SQL_TYPE_TIME:
			lpSC->nCType = SQL_C_CHAR;
			lpSC->cVFPType = 'C';
			if (pStmt->nRowSetSize == 1)
			{
				AllocateColumnBuffer(lpSC, lpSC->nDisplaySize + 1, bBindCol);
				if (!lpSC->lField.IsVariable())
					lpSC->pStore = bBindCol ? SQLStoreByBinding : SQLStoreByGetData;
				else
					lpSC->pStore = bBindCol ? SQLStoreByBindingVar : SQLStoreByGetDataVar;
			}
			else
			{
				AllocateColumnBufferSet(lpSC, lpSC->nDisplaySize + 1);
				lpSC->pStoreSet = SQLStoreCharByBindingSet;
			}
			break;

			// GUID
		case SQL_GUID:
			if (!lpSC->bCustomSchema)
			{
				lpSC->vData.ev_length = VFP2C_ODBC_MAX_GUID_LITERAL;
				lpSC->nCType = SQL_C_CHAR;
				lpSC->cVFPType = 'C';
				if (pStmt->nRowSetSize == 1)
				{
					AllocateColumnBuffer(lpSC, VFP2C_ODBC_MAX_GUID_LITERAL + 1, bBindCol);
					if (!lpSC->lField.IsVariable())
						lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
				else
				{
					AllocateColumnBufferSet(lpSC, VFP2C_ODBC_MAX_GUID_LITERAL + 1);
					lpSC->pStoreSet = SQLStoreCharByBindingSet;
				}
			}
			else
			{
				if (lpSC->cVFPType == 'Q')
				{
					AllocateColumnBuffer(lpSC, lpSC->nSize + 1, bBindCol);
					lpSC->vData.ev_length = lpSC->nSize;
					lpSC->nCType = SQL_C_GUID;
					lpSC->cVFPType = 'C';
					if (!lpSC->lField.IsVariable())
						lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
				}
			}
			break;

		case SQL_SS_VARIANT:
			if (!lpSC->bCustomSchema)
			{
				lpSC->nCType = SQL_C_BINARY;
				pStmt->pGetDataBuffer.Size(VFP2C_ODBC_MAX_FETCH_BUFFER);
				bBindCol = bGetData;
				lpSC->pStore = SQLStoreSqlVariantByGetData; // we don't use a version especially for variables since there too much code to duplicate, we handle both in one function

				if (CFoxVersion::MajorVersion() >= 9)
				{
					lpSC->cVFPType = 'W';
				}
				else
				{
					lpSC->cVFPType = 'M';
					lpSC->bBinary = TRUE;
				}
			}
			break;

		case SQL_SS_UDT:
			if (!lpSC->bCustomSchema)
			{
				lpSC->nCType = SQL_C_BINARY;
				if (lpSC->nSize > 0 && lpSC->nSize < pStmt->nUseMemoSize)
				{
					if (CFoxVersion::MajorVersion() >= 9)
						lpSC->cVFPType = 'Q';
					else
					{
						lpSC->cVFPType = 'C';
						lpSC->bBinary = TRUE;
					}
					if (pStmt->nRowSetSize == 1)
					{
						AllocateColumnBuffer(lpSC, lpSC->nSize, bBindCol);

						if (!lpSC->lField.IsVariable())
							lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
						else
							lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
					}
					else
					{
						AllocateColumnBufferSet(lpSC, lpSC->nSize);
						lpSC->pStoreSet = SQLStoreCharByBindingSet;
					}
				}
				else
				{
					pStmt->pGetDataBuffer.Size(VFP2C_ODBC_MAX_FETCH_BUFFER);
					bBindCol = bGetData;
					if (CFoxVersion::MajorVersion() >= 9)
					{
						lpSC->cVFPType = 'W';
					}
					else
					{
						lpSC->cVFPType = 'M';
						lpSC->bBinary = TRUE;
					}
					if (!lpSC->lField.IsVariable())
						lpSC->pStore = SQLStoreMemoBinary;
					else
						lpSC->pStore = SQLStoreMemoBinaryVar;
				}
			}
			break;

		default:
			if (!lpSC->bCustomSchema)
			{
				if (lpSC->nDisplaySize > 0 && lpSC->nDisplaySize < (SQLINTEGER)pStmt->nUseMemoSize)
				{

					lpSC->nSize = lpSC->nDisplaySize;
					lpSC->nCType = SQL_C_CHAR;
					lpSC->cVFPType = 'C';
					if (pStmt->nRowSetSize == 1)
					{
						AllocateColumnBuffer(lpSC, lpSC->nDisplaySize + 1, bBindCol);
						if (!lpSC->lField.IsVariable())
							lpSC->pStore = bBindCol ? SQLStoreCharByBinding : SQLStoreCharByGetData;
						else
							lpSC->pStore = bBindCol ? SQLStoreCharByBindingVar : SQLStoreCharByGetDataVar;
					}
					else
					{
						AllocateColumnBufferSet(lpSC, lpSC->nSize);
						lpSC->pStoreSet = SQLStoreCharByBindingSet;
					}
				}
				else
				{
					pStmt->pGetDataBuffer.Size(VFP2C_ODBC_MAX_FETCH_BUFFER);
					bBindCol = bGetData;
					lpSC->nCType = SQL_C_CHAR;
					lpSC->cVFPType = 'M';
					if (!lpSC->lField.IsVariable())
						lpSC->pStore = SQLStoreMemoChar;
					else
						lpSC->pStore = SQLStoreMemoCharVar;
				}
			}
			else
			{
				if (lpSC->cVFPType == 'C' || lpSC->cVFPType == 'V' || lpSC->cVFPType == 'Q')
				{
					pStmt->pGetDataBuffer.Size(VFP2C_ODBC_MAX_FETCH_BUFFER);
					bBindCol = bGetData;
					lpSC->nCType = SQL_C_CHAR;
					if (!lpSC->lField.IsVariable())
						lpSC->pStore = SQLStoreCharByGetData;
					else
						lpSC->pStore = SQLStoreCharByGetDataVar;
				}
				else if (lpSC->cVFPType == 'M' || lpSC->cVFPType == 'W')
				{
					pStmt->pGetDataBuffer.Size(VFP2C_ODBC_MAX_FETCH_BUFFER);
					bBindCol = bGetData;
					if (lpSC->bBinary || lpSC->cVFPType == 'W')
					{
						lpSC->nCType = SQL_C_BINARY;
						if (!lpSC->lField.IsVariable())
							lpSC->pStore = SQLStoreMemoBinary;
						else
							lpSC->pStore = SQLStoreMemoBinaryVar;
					}
					else
					{
						lpSC->nCType = SQL_C_CHAR;
						if (!lpSC->lField.IsVariable())
							lpSC->pStore = SQLStoreMemoChar;
						else
							lpSC->pStore = SQLStoreMemoCharVar;
					}
				}
				else if (lpSC->cVFPType == 'I')
				{
					lpSC->vData.SetInt();
					lpSC->pDataPtr = &lpSC->vData.ev_long;
					lpSC->nCType = SQL_C_SLONG;
					lpSC->bBindColumn = bBindCol;
					if (!lpSC->lField.IsVariable())
						lpSC->pStore = bBindCol ? SQLStoreByBinding : SQLStoreByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreByBindingVar : SQLStoreByGetDataVar;
				}
				else if (lpSC->cVFPType == 'B' || lpSC->cVFPType == 'N' || lpSC->cVFPType == 'F')
				{
					lpSC->vData.SetNumeric(lpSC->nSize, lpSC->nScale);
					lpSC->pDataPtr = &lpSC->vData.ev_real;
					lpSC->nCType = SQL_C_DOUBLE;
					lpSC->bBindColumn = bBindCol;
					if (!lpSC->lField.IsVariable())
						lpSC->pStore = bBindCol ? SQLStoreByBinding : SQLStoreByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreByBindingVar : SQLStoreByGetDataVar;
				}
				else if (lpSC->cVFPType == 'Y')
				{
					lpSC->vData.SetCurrency();
					lpSC->pDataPtr = &lpSC->sNumericStruct;
					lpSC->nBufferSize = sizeof(SQL_NUMERIC_STRUCT);
					lpSC->nCType = SQL_C_NUMERIC;
					lpSC->nSize = VFP2C_VFP_CURRENCY_PRECISION;
					lpSC->nScale = VFP2C_VFP_CURRENCY_SCALE;
					lpSC->bBindColumn = bBindCol;
					if (!lpSC->lField.IsVariable())
						lpSC->pStore = bBindCol ? SQLStoreCurrencyByBinding : SQLStoreCurrencyByGetData;
					else
						lpSC->pStore = bBindCol ? SQLStoreCurrencyByBindingVar : SQLStoreCurrencyByGetDataVar;
				}
			}
		} // switch(lpSC-nType)

	}
}

void SqlResultSet::AllocateColumnBuffer(SqlColumn* lpSC, int nLen, bool bBindColumn)
{
	if (bBindColumn)
	{
		lpSC->vData.SetString();
		if (!lpSC->vData.AllocHandle(nLen))
			throw E_INSUFMEMORY;
		lpSC->vData.LockHandle();
		lpSC->pDataPtr = lpSC->vData.HandleToPtr();
		lpSC->nBufferSize = nLen;
		lpSC->bBindColumn = bBindColumn;
	}
	else
	{
		lpSC->pStmt->pGetDataBuffer.Size(VFP2C_ODBC_MAX_FETCH_BUFFER);
	}
}

void SqlResultSet::AllocateColumnBufferSet(SqlColumn* lpSC, int nLen)
{
	lpSC->pStmt->pGetDataBuffer.Size(VFP2C_ODBC_MAX_FETCH_BUFFER);
	lpSC->pDataPtr = new char[nLen * pStmt->nRowSetSize];
	if (lpSC->pDataPtr == 0)
		throw E_INSUFMEMORY;
	lpSC->nIndicator = (SQLLEN)new SQLLEN[pStmt->nRowSetSize];
	if (lpSC->nIndicator == 0)
		throw E_INSUFMEMORY;
	lpSC->nBufferSize = nLen;
	lpSC->bBindColumn = true;
}

void SqlResultSet::ParseCursorSchema()
{
	if (pStmt->bExecutedOnce || !pStmt->pCursorSchema.Len())
		return;

	SqlColumn* lpSC;
	CStringView pColname;
	CStringView pSchema = pStmt->pCursorSchema.GetWordNum(pStmt->nResultset, '|');
	if (!pSchema)
		return;

	CStringMatcher pMatcher(pSchema);
	do
	{
		// try to match an identifier (fieldname)
		if (!pMatcher.MatchIdentifier(pColname, VFP2C_ODBC_MAX_COLUMN_NAME))
		{
			// if no normal identifier found, try to match a quoted (enclosed in ") identifier
			if (!pMatcher.MatchQuotedIdentifier(pColname, VFP2C_ODBC_MAX_COLUMN_NAME))
			{
				SaveCustomError("SQLExecEx", "Cursorschema contained invalid data near '%20V'.", &pMatcher.CurrentView());
				throw E_APIERROR;
			}
		}

		// get pointer to SQLCOLUMNDATA struct for the specified column
		lpSC = FindColumn(pColname);
		if (!lpSC)
		{
			SaveCustomError("SQLExecEx", "Column '%V' not contained in resultset but specified in cursorschema.", &pColname);
			throw E_APIERROR;
		}

		// match the VFP datatype .. upper or lowercase ..
		if (!pMatcher.MatchChars(VFP2C_VFP_DATATYPE_CHARACTERS, (char*)&lpSC->cVFPType))
		{
			SaveCustomError("SQLExecEx", "Datatype in cursorschema for column '%V' is invalid.", &pColname);
			throw E_APIERROR;
		}

		// convert to uppercase so we don't have to deal with case anymore ..
		lpSC->cVFPType = ToUpper(lpSC->cVFPType);

		// check if column is convertable to specified type
		if (!SQLTypeConvertible(lpSC->nSQLType, lpSC->cVFPType))
		{
			SaveCustomError("SQLExecEx", "Datatype conversion for column '%V' to VFP type '%s' not supported.", &pColname, lpSC->cVFPType);
			throw E_APIERROR;
		}

		if (lpSC->cVFPType == 'C' || lpSC->cVFPType == 'V' || lpSC->cVFPType == 'Q')
		{
			if (!pMatcher.MatchChar('('))
			{
				SaveCustomError("SQLExecEx", "Invalid cursorschema for column '%V', expected '('.", &pColname);
				throw E_APIERROR;
			}
			if (!pMatcher.MatchULong((unsigned long*)&lpSC->nSize))
			{
				SaveCustomError("SQLExecEx", "Invalid cursorschema for column '%V' - '%10V' found, expected column size.", &pColname, &pMatcher.CurrentView());
				throw E_APIERROR;
			}
			if (!pMatcher.MatchChar(')'))
			{
				SaveCustomError("SQLExecEx", "Invalid cursorschema for column '%V', expected ')'.", &pColname);
				throw E_APIERROR;
			}
			// set binary flag for varbinary type
			if (lpSC->cVFPType == 'Q')
				lpSC->bBinary = TRUE;
		}
		else if (lpSC->cVFPType == 'N' || lpSC->cVFPType == 'F')
		{
			if (pMatcher.MatchChar('('))
			{
				if (!pMatcher.MatchULong((unsigned long*)&lpSC->nSize))
				{
					SaveCustomError("SQLExecEx", "Invalid cursorschema for column '%V', expected column precision.", &pColname);
					throw E_APIERROR;
				}

				if (pMatcher.MatchChar(','))
				{
					if (!pMatcher.MatchShort(&lpSC->nScale))
					{
						SaveCustomError("SQLExecEx", "Invalid cursorschema for column '%S', expected column scale.", &pColname);
						throw E_APIERROR;
					}
				}
				else
					lpSC->nScale = 0;

				if (!pMatcher.MatchChar(')'))
				{
					SaveCustomError("SQLExecEx", "Invalid cursorschema for column '%V', expected ')'.", &pColname);
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
		else if (lpSC->cVFPType == 'B')
		{
			if (pMatcher.MatchChar('('))
			{
				if (!pMatcher.MatchShort(&lpSC->nScale))
				{
					SaveCustomError("SQLExecEx", "Invalid cursorschema for column '%V' - '%10V' found, expected column size.", &pColname, &pMatcher.CurrentView());
					throw E_APIERROR;
				}
				if (!pMatcher.MatchChar(')'))
				{
					SaveCustomError("SQLExecEx", "Invalid cursorschema for column '%V', expected ')'.", &pColname);
					throw E_APIERROR;
				}
				lpSC->nSize = VFP2C_VFP_DOUBLE_PRECISION;
			}
			else
			{
				lpSC->nSize = 8;
				lpSC->nScale = pStmt->GetDefaultDecimals();
			}
		}
		// set binary flag for blob datatype
		else if (lpSC->cVFPType == 'W')
			lpSC->bBinary = TRUE;

		if (pMatcher.MatchStringCaseInsensitive("NULL"))
			lpSC->bNullable = TRUE;

		if (pMatcher.MatchStringCaseInsensitive("NOCPTRANS"))
			lpSC->bBinary = TRUE;

		lpSC->bCustomSchema = true;

	} while (pMatcher.MatchChar(','));	// do while there are new ',' 

	if (!pMatcher.MatchChar('|') && !pMatcher.End())
	{
		SaveCustomError("SQLExecEx", "Cursorschema contained invalid data near '%20V'.", &pMatcher.CurrentView());
		throw E_APIERROR;
	}
}

void SqlResultSet::ParseExistingCursorSchema()
{
	if (pStmt->bExecutedOnce)
		return;

	SqlColumn* lpSC;
	ValueEx vValue;
	vValue.SetNull();
	char* pValue;
	int nFieldCount, nRow = 1;
	FoxVariable pArray;
	CStrBuilder<64> pArrayName;
	CStrBuilder<VFP2C_MAX_FUNCTIONBUFFER> pExeBuffer;
	CStringView pColNameView;

	// create unique array name .. since pStmt is a dynamically allocated pointer it's value is
	// always unique .. so we can use it to build a variable name ..
	pArrayName.Format("__VFP2C_ODBC_ARRAY_%U", this);
	pExeBuffer.Format("AFIELDS(%V,'%V')", &pArrayName.StringView(), &pCursorName.StringView());

	Evaluate(vValue, pExeBuffer);
	nFieldCount = vValue.ev_long;
	pArray.Attach(pArrayName);
	LocatorEx& lArrayLoc = pArray();;
	lArrayLoc.FindVar(pArrayName);
	lArrayLoc.ResetArray(2);

	while (nFieldCount--)
	{
		// fieldname
		vValue = lArrayLoc(nRow, 1);

		pColNameView.Data = vValue.HandleToPtr();
		pColNameView.Len = vValue.Len();

		lpSC = FindColumn(pColNameView);
		vValue.FreeHandle();
		vValue.SetNull();
		if (!lpSC)
		{
			SaveCustomError("SQLExecEx", "Column '%V' not contained in resultset but exists in cursor.", &pColNameView);
			throw E_APIERROR;
		}

		// fieldtype
		vValue = lArrayLoc(nRow, 2);
		pValue = vValue.HandleToPtr();
		lpSC->cVFPType = *pValue;
		vValue.FreeHandle();
		vValue.SetNull();

		// precision/width
		vValue = lArrayLoc(nRow, 3);
		lpSC->nSize = (SQLUINTEGER)vValue.ev_long;

		// scale
		vValue = lArrayLoc(nRow, 4);
		lpSC->nScale = (SQLSMALLINT)vValue.ev_long;

		// binary
		vValue = lArrayLoc(nRow, 6);
		lpSC->bBinary = (SQLSMALLINT)vValue.ev_length;

		lpSC->bCustomSchema = true;
		nRow++;
	}
}

void SqlResultSet::BindVariableLocators()
{
	if (pStmt->bExecutedOnce)
		return;

	SqlColumn* lpSC = pColumns[0];
	int nErrorNo, xj;
	char aCursorOrVar[VFP2C_VFP_MAX_CURSOR_NAME + 1];
	char aField[VFP2C_VFP_MAX_COLUMN_NAME + 1];
	unsigned int nPos = 0;
	CStringView pSchema = pStmt->pCursorNames.GetWordNum(pStmt->nResultset, '|');
	if (!pSchema)
	{
		SaveCustomError("SQLExecEx", "No variable list specified for resultset '%I'", pStmt->nResultset);
		throw E_APIERROR;
	}

	CStringMatcher pMatcher(pSchema);
	xj = pColumns.GetCount();

	do
	{
		// fill the reference to the specified variable or fieldname
		if (pMatcher.MatchIdentifier(aCursorOrVar, VFP2C_VFP_MAX_CURSOR_NAME))
		{
			if (pMatcher.MatchChar('.'))
			{
				if (pMatcher.MatchIdentifier(aField, VFP2C_VFP_MAX_COLUMN_NAME))
				{
					if (nErrorNo = FindFoxField(aField, lpSC->lField, aCursorOrVar))
						throw nErrorNo;
				}
				else
				{
					SaveCustomError("SQLExecEx", "Variable list contained invalid data near '%20V', expected columnname.", &pMatcher.CurrentView());
					throw E_APIERROR;
				}
			}
			else
			{
				if (nErrorNo = FindFoxVar(aCursorOrVar, lpSC->lField))
					throw nErrorNo;
			}

			if (!lpSC->lField.IsVariable() && (lpSC->cVFPType == 'M' || lpSC->cVFPType == 'W' || lpSC->cVFPType == 'G'))
			{
				if (nErrorNo = MemoChan(lpSC->lField.l_where, &lpSC->hMemoFile))
					throw nErrorNo;
			}
		}
		else
		{
			SaveCustomError("SQLExecEx", "Variable list contained invalid data near '%20V', expected columnname.", &pMatcher.CurrentView());
			throw E_APIERROR;
		}

		lpSC++;

		if (!--xj)
			break;

	} while (pMatcher.MatchChar(','));

	if (!pMatcher.MatchChar('|') && !pMatcher.End())
	{
		if (!xj)
		{
			SaveCustomError("SQLExecEx", "Variable list contained invalid data near '%20V'.", &pMatcher.CurrentView());
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

void SqlResultSet::SetCursorName()
{
	if (pCursorName.Len())
		return;

	pCursorName = pStmt->pCursorNames.GetWordNum(pStmt->nResultset, ',').Alltrim();

	/* if no cursorname was provided, generate a default cursorname */
	if (pCursorName.Len() == 0)
	{
		CStrBuilder<32> pCursor;
		if (pStmt->nResultset == 1)
			pCursor = "sqlresult";
		else
			pCursor.Format("sqlresult%U", pStmt->nResultset);

		pCursorName = pCursor;
	}
}

SqlColumn* SqlResultSet::FindColumn(CStringView pColName)
{
	SqlColumn* lpSC;
	for (int xj = 0; xj < pColumns.GetCount(); xj++)
	{
		lpSC = pColumns[xj];
		if (pColName.ICompare(lpSC->pColumnName))
			return lpSC;
	}
	return 0;
}

void SqlResultSet::CreateCursor()
{

	SqlColumn* lpSC;
	FoxArray pArray;
	FoxVariable pArrayRef; // create a reference to the array for automatic release at end of function
	CStrBuilder<64> pArrayName;
	CStrBuilder<VFP2C_MAX_FUNCTIONBUFFER> pExeBuffer;
	FoxString vStringValue;
	ValueEx vNumeric;
	ValueEx vLogical;
	vNumeric.SetInt();
	vLogical.SetLogical();

	// create unique array name .. since pStmt is a dynamically allocated pointer it's value is
	// always unique .. so we can use it to build a variable name ..
	pArrayName.Format("__VFP2C_ODBC_ARRAY_%U", this);
	pArray.Dimension(pArrayName, pColumns.GetCount(), 6);
	pArrayRef.Attach(pArrayName);
	vStringValue.Size(VFP2C_ODBC_MAX_COLUMN_NAME + 1);

	for (int nRow = 1; nRow <= pColumns.GetCount(); nRow++)
	{
		lpSC = pColumns[nRow - 1];
		if ((lpSC->cVFPType == 'C' || lpSC->cVFPType == 'V' || lpSC->cVFPType == 'Q') && lpSC->nSize > 254)
		{
			SaveCustomError("SqlExecEx", "Invalid cursor schema for field '%V': length %I", &lpSC->pColumnName.StringView(), lpSC->nSize);
			throw E_APIERROR;
		}
		else if ((lpSC->cVFPType == 'N' || lpSC->cVFPType == 'F') && (lpSC->nSize > 20 || lpSC->nScale > 19))
		{
			SaveCustomError("SqlExecEx", "Invalid cursor schema for field '%V': length %I, scale: %I", &lpSC->pColumnName.StringView(), lpSC->nSize, lpSC->nScale);
			throw E_APIERROR;
		}
		else if (lpSC->cVFPType == 'B' && lpSC->nScale > 18)
		{
			SaveCustomError("SqlExecEx", "Invalid cursor schema for field '%V': length %I", &lpSC->pColumnName.StringView(), lpSC->nScale);
			throw E_APIERROR;
		}

		// store fieldname
		vStringValue = lpSC->pColumnName;
		pArray(nRow, 1) = vStringValue;

		// store fieldtype

		vStringValue.Len(1);
		vStringValue[0] = lpSC->cVFPType;
		pArray(nRow, 2) = vStringValue;

		// store field width/precision
		// if the VFP datatype is none of the below ones .. the field width is unneccesary
		if (lpSC->cVFPType == 'C' || lpSC->cVFPType == 'V' || lpSC->cVFPType == 'Q' ||
			lpSC->cVFPType == 'F' || lpSC->cVFPType == 'N')
			vNumeric.ev_long = lpSC->nSize;
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
		vLogical.ev_length = lpSC->bBinary;
		pArray(nRow, 6) = vLogical;
	}

	pExeBuffer.Format("CREATE CURSOR %V FROM ARRAY %V", &pCursorName.StringView(), &pArrayName.StringView());
	Execute(pExeBuffer);
	nWorkArea = Select(0);
}

void SqlResultSet::BindFieldLocators()
{
	if (pStmt->bExecutedOnce)
		return;

	SqlColumn* lpSC;
	int nErrorNo;

	for (int xj = 0; xj < pColumns.GetCount(); xj++)
	{
		lpSC = pColumns[xj];
		if (nErrorNo = FindFoxField(lpSC->pColumnName.Ptr<char*>(), lpSC->lField, nWorkArea))
			throw nErrorNo;

		if (lpSC->cVFPType == 'M' || lpSC->cVFPType == 'W' || lpSC->cVFPType == 'G')
			MemoChan(lpSC->lField.l_where, &lpSC->hMemoFile);
	}
}

void SqlResultSet::FetchToCursor(bool* bAborted)
{
	SQLRETURN nApiRet;
	SqlColumn* lpSC;
	int nErrorNo;
	nRowsFetched = 0;
	bStoreInfoAsError = pStmt->bStoreInfoAsError;
	// reset fetch error flags if executed at least once
	if (pStmt->bExecutedOnce)
	{
		for (int xj = 0; xj < pColumns.GetCount(); xj++)
			pColumns[xj]->bStoreInfoAsError = pStmt->bStoreInfoAsError;
	}
	if (pStmt->nFlags & SQLEXECEX_CALLBACK_PROGRESS)
	{
		// callback once before first row is fetched
		pStmt->ProgressCallback(nRowsFetched, bAborted);

		if (pStmt->nRowSetSize == 1)
		{
			nApiRet = SQLFetch(pStmt->hStmt);
			while (SQL_SUCCEEDED(nApiRet))
			{
				if (nApiRet == SQL_SUCCESS_WITH_INFO && bStoreInfoAsError)
				{
					AddODBCError("SqlFetch", pStmt->hStmt);
					bStoreInfoAsError = false;
				}

				if (nErrorNo = Append(nWorkArea))
					throw nErrorNo;

				lpSC = pColumns[0];
				for (int xj = 0; xj < pColumns.GetCount(); xj++)
				{
					if (nErrorNo = lpSC->pStore(lpSC))
						throw nErrorNo;

					lpSC++;
				}

				nRowsFetched++;
				// callback for each Nth row 
				if (nRowsFetched % pStmt->nCallbackInterval == 0)
				{
					pStmt->ProgressCallback(nRowsFetched, bAborted);
				}
				nApiRet = SQLFetch(pStmt->hStmt);
			}
		}
		else
		{
			nApiRet = SQLFetchScroll(pStmt->hStmt, SQL_FETCH_NEXT, 0);
			while (SQL_SUCCEEDED(nApiRet))
			{
				if (nApiRet == SQL_SUCCESS_WITH_INFO && bStoreInfoAsError)
				{
					AddODBCError("SqlFetch", pStmt->hStmt);
					bStoreInfoAsError = false;
				}

				for (int nRow = 0; nRow < pStmt->nRowsFetched; nRow++)
				{
					if (nErrorNo = Append(nWorkArea))
						throw nErrorNo;

					lpSC = pColumns[0];
					for (int xj = 0; xj < pColumns.GetCount(); xj++)
					{
						if (nErrorNo = lpSC->pStoreSet(lpSC, nRow))
							throw nErrorNo;
						lpSC++;
					}
					nRowsFetched++;

					// callback for each Nth row 
					if (nRowsFetched % pStmt->nCallbackInterval == 0)
					{
						pStmt->ProgressCallback(nRowsFetched, bAborted);
					}
				}

				nApiRet = SQLFetchScroll(pStmt->hStmt, SQL_FETCH_NEXT, 0);
			}
		}

		if (nApiRet != SQL_ERROR)
		{
			// callback once after last row is fetched
			pStmt->ProgressCallback(nRowsFetched, bAborted);
		}
	}
	else
	{
		if (pStmt->nRowSetSize == 1)
		{
			nApiRet = SQLFetch(pStmt->hStmt);
			while (SQL_SUCCEEDED(nApiRet))
			{
				if (nApiRet == SQL_SUCCESS_WITH_INFO && bStoreInfoAsError)
				{
					AddODBCError("SqlFetch", pStmt->hStmt);
					bStoreInfoAsError = false;
				}

				if (nErrorNo = Append(nWorkArea))
					throw nErrorNo;

				lpSC = pColumns[0];
				for (int xj = 0; xj < pColumns.GetCount(); xj++)
				{
					if (nErrorNo = lpSC->pStore(lpSC))
						throw nErrorNo;

					lpSC++;
				}
				nRowsFetched++;
				nApiRet = SQLFetch(pStmt->hStmt);
			}
		}
		else
		{
			nApiRet = SQLFetchScroll(pStmt->hStmt, SQL_FETCH_NEXT, 0);
			while (SQL_SUCCEEDED(nApiRet))
			{
				if (nApiRet == SQL_SUCCESS_WITH_INFO && bStoreInfoAsError)
				{
					AddODBCError("SQLFetchScroll", pStmt->hStmt);
					bStoreInfoAsError = false;
				}

				for (int nRow = 0; nRow < pStmt->nRowsFetched; nRow++)
				{
					if (nErrorNo = Append(nWorkArea))
						throw nErrorNo;

					lpSC = pColumns[0];
					for (int xj = 0; xj < pColumns.GetCount(); xj++)
					{
						if (nErrorNo = lpSC->pStoreSet(lpSC, nRow))
							throw nErrorNo;
						lpSC++;
					}
					nRowsFetched++;
				}

				nApiRet = SQLFetchScroll(pStmt->hStmt, SQL_FETCH_NEXT, 0);
			}
		}
	}

	if (nApiRet == SQL_ERROR)
	{
		SaveODBCError(pStmt->nRowSetSize == 1 ? "SQLFetch" : "SQLFetchScroll", pStmt->hStmt);
		throw E_APIERROR;
	}
}

void SqlResultSet::FetchToVariables()
{
	SQLRETURN nApiRet;
	SqlColumn* lpSC;
	int nErrorNo;
	bool bFetchError = false;
	nRowsFetched = 0;
	nApiRet = SQLFetch(pStmt->hStmt);
	if (SQL_SUCCEEDED(nApiRet))
	{
		if (nApiRet == SQL_SUCCESS_WITH_INFO && pStmt->bStoreInfoAsError)
			AddODBCError("SqlFetch", pStmt->hStmt);

		lpSC = pColumns[0];
		for (int xj = 0; xj < pColumns.GetCount(); xj++)
		{
			if (nErrorNo = lpSC->pStore(lpSC))
				throw nErrorNo;

			lpSC++;
		}
		nRowsFetched++;
	}
	else if (nApiRet == SQL_ERROR)
	{
		SaveODBCError("SQLFetch", pStmt->hStmt);
		throw E_APIERROR;
	}
}

void SqlResultSet::FetchOutputColumns(SqlParameterArray& pParameterArray)
{
	SQLRETURN nApiRet;
	SqlColumn* lpSC;
	int nErrorNo;
	nApiRet = SQLFetch(pStmt->hStmt);
	while (SQL_SUCCEEDED(nApiRet))
	{
		if (nApiRet == SQL_SUCCESS_WITH_INFO && bStoreInfoAsError)
		{
			AddODBCError("SqlFetch", pStmt->hStmt);
			bStoreInfoAsError = false;
		}

		lpSC = pColumns[0];

		Go(pParameterArray.pParamRecNo[pParameterArray.nParamLastOutput++], pParameterArray.nWorkArea);

		for (int xj = 0; xj < pColumns.GetCount(); xj++)
		{
			if (nErrorNo = lpSC->pStore(lpSC))
				throw nErrorNo;

			lpSC++;
		}
		nApiRet = SQLFetch(pStmt->hStmt);
	}
}

SqlColumn::SqlColumn()
{
	vData.SetNull();
	pDataPtr = 0;
	nSize = 0;
	nCType = 0;
	bBinary = 0;
	bUnsigned = 0;
	bMoney = 0;
	nDisplaySize = 0;
	nBufferSize = 0;
	nIndicator = 0;
	bBindColumn = false;
	bCustomSchema = false;
}

SqlParameter::SqlParameter()
{
	lVarOrField.l_NTI = 0;
	vParmValue.SetNull();
	vMemoValue.SetNull();
	pParmPtr = 0;
	nParmNo = 0;
	nSize = 0;
	nBufferSize = 0;
	nIndicator = 0;
	nParmType = SQL_PARAM_INPUT;
	nSQLType = 0;
	nSQLTypeCode = 0;
	nCType = 0;
	nScale = 0;
	nWorkArea = 0;
	pTableType = 0;
	cVFPType = '0';
	bVFPBinary = false;
	bPutData = false;
	bCustomSchema = false;
}

void SqlParameter::DeduceParameterType()
{
	char cType = vParmValue.Vartype();
	if (cType == '0')
		cType = (char)vParmValue.ev_long;
	bool bBinary = vParmValue.Binary();
	DeduceParameterType(cType, bBinary);
}

void SqlParameter::DeduceParameterType(char cType, bool bBinary)
{
	if (nParmType <= SQL_PARAM_OUTPUT)
	{
		switch (cType)
		{
		case 'C':
			if (!bBinary)
			{
				nSQLType = SQL_CHAR;
				nCType = SQL_C_CHAR;
			}
			else
			{
				nSQLType = SQL_BINARY;
				nCType = SQL_C_BINARY;
			}
			break;
		case 'V':
			nSQLType = SQL_VARCHAR;
			nCType = SQL_C_CHAR;
			break;
		case 'Q':
			nSQLType = SQL_VARBINARY;
			nCType = SQL_C_BINARY;
			break;
		case 'M':
			if (!bBinary)
			{
				nSQLType = SQL_LONGVARCHAR;
				nCType = SQL_C_CHAR;
			}
			else
			{
				nSQLType = SQL_LONGVARBINARY;
				nCType = SQL_C_BINARY;
			}
			break;
		case 'W':
		case 'G':
			nSQLType = SQL_LONGVARBINARY;
			nCType = SQL_C_BINARY;
			break;
		case 'N':
		case 'F':
			nSQLType = SQL_DECIMAL;
			nCType = SQL_C_DOUBLE;
			break;
		case 'B':
			nSQLType = SQL_DOUBLE;
			nCType = SQL_C_DOUBLE;
			break;
		case 'I':
			nSQLType = SQL_INTEGER;
			nCType = SQL_C_SLONG;
			break;
		case 'T':
			nSQLType = SQL_TIMESTAMP;
			nSQLTypeCode = SQL_CODE_TIMESTAMP;
			nCType = SQL_C_TIMESTAMP;
			break;
		case 'D':
			nSQLType = SQL_DATETIME;
			nSQLTypeCode = SQL_CODE_DATE;
			nCType = SQL_C_DATE;
			break;
		case 'L':
			nSQLType = SQL_BIT;
			nCType = SQL_C_BIT;
			break;
		case 'Y':
			nSQLType = SQL_NUMERIC;
			nCType = SQL_C_NUMERIC;
			break;
		}
	}
	else
	{
		switch (cType)
		{
		case 'C':
		case 'M':
			if (!bBinary)
			{
				nCType = SQL_C_CHAR;
				nSQLType = SQL_VARCHAR;
			}
			else
			{
				nSQLType = SQL_VARBINARY;
				nCType = SQL_C_BINARY;
			}
			break;
		case 'V':
			nSQLType = SQL_VARCHAR;
			nCType = SQL_C_CHAR;
			break;
		case 'Q':
		case 'W':
		case 'G':
			nSQLType = SQL_VARBINARY;
			nCType = SQL_C_BINARY;
			break;
		}
	}
}

SqlTableTypeParameter::SqlTableTypeParameter()
{
	nDescAutoUniqueValue = SQL_FALSE;
	nColumnComputed = SQL_FALSE;
	nColumnInUniqueKey = SQL_FALSE;
	nColumnSortOrder = SQL_SS_ORDER_UNSPECIFIED;
	nColumnSortOrdinal = 0;
	nColumnHasDefaultValue = SQL_FALSE;
}

SqlTableTypeColumn::SqlTableTypeColumn()
{
	nColumnNo = 0;
	nSize = 0;
	nDisplaySize = 0;
	nSQLType = 0;
	nSQLTypeCode = 0;
	nCType = 0;
	nScale = 0;
	bMoney = 0;
	bNullable = 0;
	cVFPType = '0';
	bVFPBinary = false;
}

SqlTableType::SqlTableType(CStringView vCatalog, CStringView vSchema, CStringView vType)
{
	pCatalog = vCatalog.Alltrim();
	pSchema = vSchema.Alltrim();
	pTypename = vType.Alltrim();
	pMapKey.Append(pCatalog);
	pMapKey.Append('.');
	pMapKey.Append(pSchema);
	pMapKey.Append('.');
	pMapKey.Append(pTypename);
}

const char* SqlTableType::Key() const
{
	return pMapKey.Ptr<const char*>();
}

SqlParameterArray::SqlParameterArray()
{
	nWorkArea = 0;
	nBatchSize = 0;
	nParamsLastProcessed = 0;
	nParamsProcessed = 0;
	nParamsResumeOnError = 0;
	nParamLastOutput = 0;
}

/**



* 

*/