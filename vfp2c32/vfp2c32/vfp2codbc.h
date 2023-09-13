#ifndef _VFP2CODBC_H__
#define _VFP2CODBC_H__

#include <odbcinst.h>
#include <sqlext.h>
#include <odbcss.h> // MS-SQL Server specific include file
#include "vfp2ccppapi.h"

// odbc datasource enumeration
const int ODBC_MAX_DESCRIPTION_LEN	= 512;
const int ODBC_MAX_DRIVER_LEN		= 512;
const int ODBC_MAX_ATTRIBUTES_LEN	= 2048;

const int SQL_MAX_DSN_LENGTH_EX		= SQL_MAX_DSN_LENGTH + 5; // +5 to prepend 'DSN=' (+1 for null terminator)

// constants for SQLExecEx
const int VFP2C_ODBC_MAX_BUFFER				= 8192;
const int VFP2C_ODBC_MAX_PARAMETER_EXPR		= 256;
const int VFP2C_ODBC_MAX_PARAMETER_NAME		= 128;
const int VFP2C_ODBC_MAX_SCHEMA_NAME		= 256;
const int VFP2C_ODBC_MAX_COLUMN_NAME		= 256;
const int VFP2C_ODBC_MAX_TABLE_NAME			= 256;
const int VFP2C_ODBC_MAX_FIELD_NAME			= VFP2C_ODBC_MAX_SCHEMA_NAME + 1 + VFP2C_ODBC_MAX_TABLE_NAME + 1 + VFP2C_ODBC_MAX_COLUMN_NAME;
const int VFP2C_ODBC_MAX_CHAR_LEN			= 255;
const int VFP2C_ODBC_MAX_SQLTYPE			= 32;
const int VFP2C_ODBC_MAX_BIGINT_LITERAL		= 20;
const int VFP2C_ODBC_MAX_CURRENCY_LITERAL	= 22;
const int VFP2C_ODBC_MAX_GUID_LITERAL		= 36;
const int VFP2C_ODBC_MAX_SERVER_NAME		= 128;

const int VFP2C_VFP_DOUBLE_PRECISION	= 18;
const int VFP2C_VFP_CURRENCY_PRECISION	= 19;
const int VFP2C_VFP_CURRENCY_SCALE		= 4;

// flags for SQLExecEx
const unsigned int SQLEXECEX_DEST_CURSOR		= 0x00000001;
const unsigned int SQLEXECEX_DEST_VARIABLE		= 0x00000002;
const unsigned int SQLEXECEX_REUSE_CURSOR		= 0x00000004;
const unsigned int SQLEXECEX_NATIVE_SQL			= 0x00000008;
const unsigned int SQLEXECEX_CALLBACK_PROGRESS	= 0x00000010;
const unsigned int SQLEXECEX_CALLBACK_INFO		= 0x00000020;
const unsigned int SQLEXECEX_STORE_INFO			= 0x00000040;
const unsigned int SQLEXECEX_APPEND_CURSOR		= 0x00000080;
const unsigned int SQLEXECEX_PRESERVE_RECNO		= 0x00000100;

// forward declarations
class SqlResultSet;
class SqlColumn;
class SqlParameter;

// function typedef for indirect funtion call's in SQLCOLUMNDATA
typedef int (_fastcall *LPSQLSTOREFUNC)(SqlColumn*);

// all common data for a SQL statement + pointers to column & parameter data
class SqlStatement {
public:
	SqlStatement();
	~SqlStatement();

	void FreeParameters();
	void SaveOutputParameters();
	void PutData();
	void BindParameters();
	void NumParamsEx(char* pSQL);
	void ExtractParamsAndRewriteStatement();
	void ParseParamSchema();
	void EvaluateParams();
	void InfoCallbackOrStore();
	void ProgressCallback(int nRowsFetched, BOOL* bAbort);
	SqlResultSet* AddResultSet();

	CAutoPtrArray<SqlResultSet> pResultSets;
	SqlParameter* pParamData;
	SQLHSTMT hStmt;
	SQLHDBC hConn;
	SQLSMALLINT nNoOfParms;
	BOOL bOutputParams;
	FoxString pGetDataBuffer;
	FoxString pCursorNames;
	FoxString pSQLInput;
	FoxString pSQLSend;
	FoxString pCursorSchema;
	FoxString pParamSchema;
	FoxArray pInfoArray;
	CFoxCallback pCallback;
	unsigned int nResultset;
	int nCallbackInterval;
	SQLLEN nRowsTotal;
	int nRowsFetched;
	DWORD nFlags;
	bool bMapVarchar;
	bool bPrepared;
	bool bExecutedOnce;
};

class SqlResultSet
{
public:
	SqlResultSet(SqlStatement* stmt);
	~SqlResultSet();

	void AllocateColumns(SQLSMALLINT columncount);
	void GetMetaData();
	void BindColumns();
	void BindFieldLocators();
	void BindVariableLocators();
	void BindVariableLocatorsEx();
	void ParseCursorSchema();
	void ParseCursorSchemaEx();
	void PrepareColumnBindings();
	void CreateCursor();
	void FetchToCursor(BOOL* bAborted);
	void FetchToVariables();

	SqlStatement* pStmt;
	SqlColumn* pColumnData;
	FoxString pCursorName;
	SQLSMALLINT nNoOfCols;
	int nWorkArea;

private:
	SqlColumn* FindColumn(CStringView pColName);
	void FixColumnName(char* pColumn);
	void BindGetDataBuffer(SqlColumn* lpCs, FoxString* pGetDataBuffer);
	void AllocateColumnBuffer(SqlColumn* lpSC, int nLen, BOOL bBindColumn, FoxString* pGetDataBuffer);
};

// holds data for each column in a SQL resultset
class SqlColumn {
public:
	SqlColumn();
	ValueEx vData;
	ValueEx vNull;
	LocatorEx lField;
	SQLHSTMT hStmt;
	SQLPOINTER pData;
	SQLULEN nSize;
	SQLSMALLINT nSQLType;
	SQLSMALLINT nCType;
	SQLSMALLINT nScale;
	SQLSMALLINT bNullable;
	SQLSMALLINT bBinary;
	SQLUSMALLINT nColNo;
	SQLLEN bUnsigned;
	SQLLEN bMoney;
	SQLLEN nDisplaySize;
	SQLLEN nBufferSize;
	SQLLEN nIndicator;
	BOOL bBindColumn;
	BOOL bCustomSchema;
	LPSQLSTOREFUNC pStore;
	FCHAN hMemoFile;
	SQLCHAR aVFPType;
	SQLSMALLINT nNameLen;
	SQLCHAR aColName[VFP2C_ODBC_MAX_COLUMN_NAME];
	SQL_TIMESTAMP_STRUCT sDateTime;
	SQLCHAR aNumeric[VFP2C_ODBC_MAX_CURRENCY_LITERAL+1];
};

// holds data for each parameter in a SQL statement
class SqlParameter {
public:
	SqlParameter();
	char aParmExpr[VFP2C_ODBC_MAX_PARAMETER_EXPR]; // buffer into which the parameter expression is stored
	char aParmName[VFP2C_ODBC_MAX_PARAMETER_NAME]; // name of parameter, if it's a named parameter
	LocatorEx lVarOrField;		// for output parameters one can only pass variables or fieldnames of course, this will hold the Locator referencing the variable/field
	ValueEx vParmValue;			// Value structure into which the parameter is stored
	SQLPOINTER pParmData;		// pointer to the data of the parameter
	SQLUSMALLINT nParmNo;		// number of parameter (1 based)
	SQLUINTEGER nSize;			// size of parameter
	SQLINTEGER nBufferSize;		// buffersize for output parameters
	SQLLEN nIndicator;		// for output parameters, indicator if the parameter was set and it's size
	SQLSMALLINT nParmDirection; // input, input/output or output parameter ?
	SQLINTEGER nPrecision;		// precision of numeric/datetime/interval types
	SQLSMALLINT nScale;			// scale of numeric types
	SQLSMALLINT nSQLType;		// the SQL datatype for the parameter
	SQLSMALLINT nCType;			// the C datatype for the parameter
	BOOL bPutData;				// bind parameter or use SQLParamData/SQLPutData for long values (char/binary > 255 bytes)
	BOOL bCustomSchema;			// use custom type from parameter schema?
	BOOL bNamed;				// named parameter?
	SQL_TIMESTAMP_STRUCT sDateTime; // we need this if the parameter is of type date/datetime
	SQLCHAR aNumeric[VFP2C_ODBC_MAX_CURRENCY_LITERAL + 1];
};

#ifdef __cplusplus
extern "C" {
#endif

// function forward definitions
void _stdcall SaveODBCError(char *pFunction, SQLHANDLE hHandle, SQLSMALLINT nHandleType);

inline void SafeODBCDbcError(char *pFunction, SQLHANDLE hHandle) { SaveODBCError(pFunction, hHandle, SQL_HANDLE_DBC); }
inline void SafeODBCStmtError(char *pFunction, SQLHANDLE hHandle) { SaveODBCError(pFunction, hHandle, SQL_HANDLE_STMT); }

int _stdcall VFP2C_Init_Odbc();

void _fastcall CreateSQLDataSource(ParamBlkEx& parm);
void _fastcall DeleteSQLDataSource(ParamBlkEx& parm);
void _fastcall ChangeSQLDataSource(ParamBlkEx& parm);
void _fastcall ASQLDataSources(ParamBlkEx& parm);
void _fastcall ASQLDrivers(ParamBlkEx& parm);
void _fastcall SQLGetPropEx(ParamBlkEx& parm);
void _fastcall SQLSetPropEx(ParamBlkEx& parm);
void _fastcall SQLExecEx(ParamBlkEx& parm);
void _fastcall SQLPrepareEx(ParamBlkEx& parm);
void _fastcall SQLCancelEx(ParamBlkEx& parm);

#pragma warning(disable : 4290)
SqlStatement* _fastcall SQLAllocStatement(ParamBlkEx& parm, bool prepared) throw(int);
#pragma warning(default : 4290)

BOOL _fastcall SQLTypeConvertible(SQLSMALLINT nSQLType, char aVFPType);
unsigned int _fastcall SQLExtractInfo(char *pMessage, unsigned int nMsgLen);

int _fastcall SQLStoreByBinding(SqlColumn* lpCS);
int _fastcall SQLStoreByBindingVar(SqlColumn* lpCS);
int _fastcall SQLStoreByGetData(SqlColumn* lpCS);
int _fastcall SQLStoreByGetDataVar(SqlColumn* lpCS);
int _fastcall SQLStoreCharByBinding(SqlColumn* lpCS);
int _fastcall SQLStoreCharByBindingVar(SqlColumn* lpCS);
int _fastcall SQLStoreCharByGetData(SqlColumn* lpCS);
int _fastcall SQLStoreCharByGetDataVar(SqlColumn* lpCS);
int _fastcall SQLStoreDateByBinding(SqlColumn* lpCS);
int _fastcall SQLStoreDateByBindingVar(SqlColumn* lpCS);
int _fastcall SQLStoreDateByGetData(SqlColumn* lpCS);
int _fastcall SQLStoreDateByGetDataVar(SqlColumn* lpCS);
int _fastcall SQLStoreDateTimeByBinding(SqlColumn* lpCS);
int _fastcall SQLStoreDateTimeByBindingVar(SqlColumn* lpCS);
int _fastcall SQLStoreDateTimeByGetData(SqlColumn* lpCS);
int _fastcall SQLStoreDateTimeByGetDataVar(SqlColumn* lpCS);
int _fastcall SQLStoreCurrencyByBinding(SqlColumn* lpCS);
int _fastcall SQLStoreCurrencyByBindingVar(SqlColumn* lpCS);
int _fastcall SQLStoreCurrencyByGetData(SqlColumn* lpCS);
int _fastcall SQLStoreCurrencyByGetDataVar(SqlColumn* lpCS);
int _fastcall SQLStoreMemoChar(SqlColumn* lpCS);
int _fastcall SQLStoreMemoCharVar(SqlColumn* lpCS);
int _fastcall SQLStoreMemoWChar(SqlColumn* lpCS);
int _fastcall SQLStoreMemoWCharVar(SqlColumn* lpCS);
int _fastcall SQLStoreMemoBinary(SqlColumn* lpCS);
int _fastcall SQLStoreMemoBinaryVar(SqlColumn* lpCS);

void _fastcall Timestamp_StructToDateTime(TIMESTAMP_STRUCT *pTime, Value *pDateTime);
void _fastcall DateTimeToTimestamp_Struct(Value *pDateTime, TIMESTAMP_STRUCT *pTime);
void _fastcall Timestamp_StructToDate(TIMESTAMP_STRUCT *pTime, Value *pDateTime);
void _fastcall DateToTimestamp_Struct(Value *pDateTime, TIMESTAMP_STRUCT *pTime);
void _fastcall Numeric_StructToCurrency(SQL_NUMERIC_STRUCT *lpNum, Value *pValue);
void _fastcall CurrencyToNumeric_Struct(Value *pValue, SQL_NUMERIC_STRUCT *lpNum);
void _fastcall NumericLiteralToCurrency(SQLCHAR *pLiteral, Value *pValue);
void _fastcall CurrencyToNumericLiteral(Value *pValue, SQLCHAR *pLiteral);

#ifdef __cplusplus
}
#endif // end of extern "C"

#endif _VFP2CODBC_H__