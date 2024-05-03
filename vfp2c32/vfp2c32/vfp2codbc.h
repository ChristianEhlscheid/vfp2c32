#ifndef _VFP2CODBC_H__
#define _VFP2CODBC_H__

#include <odbcinst.h>
#include <sqlext.h>
// #include <odbcss.h> // MS-SQL Server specific include file
#include "msodbcsql.h"
#include "vfp2ccppapi.h"
#include "vfp2codbcclasses.h"

// function forward definitions
void _stdcall SaveODBCError(char *pFunction, SQLHANDLE hHandle, SQLSMALLINT nHandleType, bool bAddError);
inline void SaveODBCError(char *pFunction, SQLHENVPARAM hParam) { SaveODBCError(pFunction, hParam.hHandle, SQL_HANDLE_ENV, false); }
inline void SaveODBCError(char* pFunction, SQLHDBCPARAM hParam) { SaveODBCError(pFunction, hParam.hHandle, SQL_HANDLE_DBC, false); }
inline void SaveODBCError(char* pFunction, SQLHSTMTPARAM hParam) { SaveODBCError(pFunction, hParam.hHandle, SQL_HANDLE_STMT, false); }
inline void SaveODBCError(char* pFunction, SQLHDESCPARAM hParam) { SaveODBCError(pFunction, hParam.hHandle, SQL_HANDLE_DESC, false); }
inline void AddODBCError(char* pFunction, SQLHENVPARAM hParam) { SaveODBCError(pFunction, hParam.hHandle, SQL_HANDLE_ENV, true); }
inline void AddODBCError(char* pFunction, SQLHDBCPARAM hParam) { SaveODBCError(pFunction, hParam.hHandle, SQL_HANDLE_DBC, true); }
inline void AddODBCError(char* pFunction, SQLHSTMTPARAM hParam) { SaveODBCError(pFunction, hParam.hHandle, SQL_HANDLE_STMT, true); }
inline void AddODBCError(char* pFunction, SQLHDESCPARAM hParam) { SaveODBCError(pFunction, hParam.hHandle, SQL_HANDLE_DESC, true); }

#ifdef __cplusplus
extern "C" {
#endif

void _fastcall CreateSQLDataSource(ParamBlkEx& parm);
void _fastcall DeleteSQLDataSource(ParamBlkEx& parm);
void _fastcall ChangeSQLDataSource(ParamBlkEx& parm);
void _fastcall ASQLDataSources(ParamBlkEx& parm);
void _fastcall ASQLDrivers(ParamBlkEx& parm);
void _fastcall SQLGetPropEx(ParamBlkEx& parm);
void _fastcall SQLSetPropEx(ParamBlkEx& parm);
void _fastcall SQLRegisterTableType(ParamBlkEx& parm);
void _fastcall SQLCreateTableTypeCursor(ParamBlkEx& parm);
void _fastcall SQLTablesEx(ParamBlkEx& parm);
void _fastcall SQLColumnsEx(ParamBlkEx& parm);
void _fastcall SQLPrimaryKeysEx(ParamBlkEx& parm);
void _fastcall SQLForeignKeysEx(ParamBlkEx& parm);
void _fastcall SQLProceduresEx(ParamBlkEx& parm);
void _fastcall SQLProcedureColumnsEx(ParamBlkEx& parm);
void _fastcall SQLTablePrivilegesEx(ParamBlkEx& parm);
void _fastcall SQLColumnPrivilegesEx(ParamBlkEx& parm);
void _fastcall SQLSpecialColumnsEx(ParamBlkEx& parm);
void _fastcall SQLStatisticsEx(ParamBlkEx& parm);
void _fastcall SQLGetTypeInfoEx(ParamBlkEx& parm);
void _fastcall SQLExecEx(ParamBlkEx& parm);
void _fastcall SQLPrepareEx(ParamBlkEx& parm);
void _fastcall SQLCancelEx(ParamBlkEx& parm);
void _fastcall SQLVariantToValue(ParamBlkEx& parm);
void _fastcall ValueToSQLVariant(ParamBlkEx& parm);

unsigned int _fastcall FixCursorColumnName(CStringView pColumnName);
#pragma warning(disable : 4290)
SqlStatement* _fastcall SQLAllocStatement(ParamBlkEx& parm, bool prepared, VFP2CTls& tls) throw(int);
SqlStatement* _fastcall SQLAllocCatalogStatement(ParamBlkEx& parm, VFP2CTls& tls, DWORD nFlags) throw(int);
void _fastcall SQLCatalogFetch(SqlStatement* pStmt, FoxString& pCursor, VFP2CTls& tls) throw(int);
#pragma warning(default : 4290)

BOOL _fastcall SQLTypeConvertible(SQLSMALLINT nSQLType, char aVFPType);

int _fastcall SQLStoreByBinding(SqlColumn* lpCS);
int _fastcall SQLStoreByBindingVar(SqlColumn* lpCS);
int _fastcall SQLStoreByGetData(SqlColumn* lpCS);
int _fastcall SQLStoreByGetDataVar(SqlColumn* lpCS);
int _fastcall SQLStoreCharByBinding(SqlColumn* lpCS);
int _fastcall SQLStoreCharByBindingVar(SqlColumn* lpCS);
int _fastcall SQLStoreCharByGetData(SqlColumn* lpCS);
int _fastcall SQLStoreCharByGetDataVar(SqlColumn* lpCS);
int _fastcall SQLStoreCharByBindingSet(SqlColumn* lpCS, int nRow);
int _fastcall SQLStoreDateByBinding(SqlColumn* lpCS);
int _fastcall SQLStoreDateByBindingVar(SqlColumn* lpCS);
int _fastcall SQLStoreDateByGetData(SqlColumn* lpCS);
int _fastcall SQLStoreDateByGetDataVar(SqlColumn* lpCS);
int _fastcall SQLStoreDateByBindingSet(SqlColumn* lpSC, int nRow);
int _fastcall SQLStoreDateTimeByBinding(SqlColumn* lpCS);
int _fastcall SQLStoreDateTimeByBindingVar(SqlColumn* lpCS);
int _fastcall SQLStoreDateTimeByGetData(SqlColumn* lpCS);
int _fastcall SQLStoreDateTimeByGetDataVar(SqlColumn* lpCS);
int _fastcall SQLStoreDateTimeByBindingSet(SqlColumn* lpSC, int nRow);
int _fastcall SQLStoreCurrencyByBinding(SqlColumn* lpCS);
int _fastcall SQLStoreCurrencyByBindingVar(SqlColumn* lpCS);
int _fastcall SQLStoreCurrencyByGetData(SqlColumn* lpCS);
int _fastcall SQLStoreCurrencyByGetDataVar(SqlColumn* lpCS);
int _fastcall SQLStoreCurrencyByBindingSet(SqlColumn* lpSC, int nRow);
int _fastcall SQLStoreLogicalByBindingSet(SqlColumn* lpSC, int nRow);
int _fastcall SQLStoreIntByBindingSet(SqlColumn* lpSC, int nRow);
int _fastcall SQLStoreDoubleByBindingSet(SqlColumn* lpSC, int nRow);
int _fastcall SQLStoreMemoChar(SqlColumn* lpCS);
int _fastcall SQLStoreMemoCharVar(SqlColumn* lpCS);
int _fastcall SQLStoreMemoCharByBindingSet(SqlColumn* lpCS, int nRow);
int _fastcall SQLStoreMemoWChar(SqlColumn* lpCS);
int _fastcall SQLStoreMemoWCharVar(SqlColumn* lpCS);
int _fastcall SQLStoreMemoWCharByBindingSet(SqlColumn* lpCS, int nRow);
int _fastcall SQLStoreMemoBinary(SqlColumn* lpCS);
int _fastcall SQLStoreMemoBinaryVar(SqlColumn* lpCS);
int _fastcall SQLStoreMemoBinaryByBindingSet(SqlColumn* lpCS, int nRow);
int _fastcall SQLStoreSqlVariantByBinding(SqlColumn* lpSC);
int _fastcall SQLStoreSqlVariantByGetData(SqlColumn* lpSC);
int _fastcall SQLStoreSqlVariantByBindingVar(SqlColumn* lpSC);
int _fastcall SQLStoreSqlVariantByGetDataVar(SqlColumn* lpSC);


void _fastcall Timestamp_StructToValue(SQL_TIMESTAMP_STRUCT *pTimestamp, Value *pValue);
void _fastcall ValueToTimestamp_Struct(Value *pValue, SQL_TIMESTAMP_STRUCT *pTimestamp);
void _fastcall Date_StructToValue(SQL_DATE_STRUCT *pDate, Value *pValue);
void _fastcall ValueToDate_Struct(Value *pValue, SQL_DATE_STRUCT *pDate);
void _fastcall Numeric_StructToValue(SQL_NUMERIC_STRUCT* lpNum, Value* pValue);
void _fastcall ValueToNumeric_Struct(Value* pValue, SQL_NUMERIC_STRUCT* lpNum);

#pragma warning(disable : 4290)
void _fastcall StringToDate_Struct(char* lpString, SQLLEN nLen, SQL_DATE_STRUCT* pTime) throw(int);
void _fastcall StringToTime_Struct(char* lpString, SQLLEN nLen, SQL_TIME_STRUCT* pTime) throw(int);
void _fastcall StringToTime2_Struct(char* lpString, SQLLEN nLen, SQLSMALLINT nDecimals, SQL_SS_TIME2_STRUCT* pTime) throw(int);
void _fastcall StringToTimestamp_Struct(char* lpString, SQLLEN  nLen, SQLSMALLINT nDecimals, SQL_TIMESTAMP_STRUCT *pTime) throw(int);
void _fastcall StringToTimestampOffset_Struct(char* lpString, SQLLEN nLen, SQLSMALLINT nDecimals, SQL_SS_TIMESTAMPOFFSET_STRUCT *pTime) throw(int);
#pragma warning(default : 4290)

#ifdef __cplusplus
}
#endif // end of extern "C"

#endif _VFP2CODBC_H__