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
const int VFP2C_ODBC_MAX_VARCHAR			= 8000;
const int VFP2C_ODBC_DEFAULT_BUFFER			= 256;
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

// defines for TableUpdateEx
const int VFP2C_ODBC_MAX_SQLSTATEMENT	= 16384;

// flags for TableUpdateEx
const int TABLEUPDATEEX_CURRENT_ROW			= 0x00000001;
const int TABLEUPDATEEX_ALL_ROWS			= 0x00000002;
const int TABLEUPDATEEX_KEY_ONLY			= 0x00000004;
const int TABLEUPDATEEX_KEY_AND_MODIFIED	= 0x00000008;

const int VFP_FLDSTATE_CHANGED	= 1;
const int VFP_FLDSTATE_DELETED	= 2;
const int VFP_FLDSTATE_APPENDED	= 3;

// function typedef for indirect funtion call's in SQLCOLUMNDATA
typedef int (_stdcall *LPSQLSTOREFUNC)(struct _SQLCOLUMNDATA*);

// holds data for each column in a SQL resultset
typedef struct _SQLCOLUMNDATA {
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
	SQLSMALLINT nNameLen;
	SQLCHAR aVFPType;
	SQLCHAR aColName[VFP2C_ODBC_MAX_COLUMN_NAME];
	SQL_TIMESTAMP_STRUCT sDateTime;
	SQLCHAR aNumeric[VFP2C_ODBC_MAX_CURRENCY_LITERAL+1];
} SQLCOLUMNDATA, *LPSQLCOLUMNDATA;

// holds data for each parameter in a SQL statement
typedef struct _SQLPARAMDATA {
	char aParmExpr[VFP2C_ODBC_MAX_PARAMETER_EXPR]; // buffer into which the parameter expression is stored
	char aParmName[VFP2C_ODBC_MAX_PARAMETER_NAME]; // name of parameter, if it's a named parameter
	Locator lVarOrField;		// for output parameters one can only pass variables or fieldnames of course, this will hold the Locator referencing the variable/field
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
	SQLCHAR aNumeric[VFP2C_ODBC_MAX_CURRENCY_LITERAL+1];
} SQLPARAMDATA, *LPSQLPARAMDATA;

// all common data for a SQL statement + pointers to column & parameter data
class SqlStatement {
public:
	SqlStatement() {
		pColumnData = 0;
		pParamData = 0;
		hStmt = 0;
		hConn = 0;
		nNoOfCols = 0;
		nNoOfParms = 0;
		bOutputParams = FALSE;
		pSQLSend = 0;
		pCursorname = 0;
		nSQLLen = 0;
		nResultset = 0;
		nCallbackInterval = 100;
		nRowsTotal = 0;
		nRowsFetched = 0;
		nFlags = 0;
		bMapVarchar = false;
		bPrepared = false;
	}
	~SqlStatement();

	void CreateCursor();
	void FreeParameters();
	void FreeColumnBuffers();
	void SaveOutputParameters();
	void PutData();
	void GetMetaData();
	void BindColumns();
	void BindParameters();
	void BindFieldLocators();
	void BindVariableLocators();
	void BindVariableLocatorsEx();
	void ParseCursorSchema();
	void ParseCursorSchemaEx(char* pCursor);
	void PrepareColumnBindings();
	void NumParamsEx(char* pSQL);
	void FetchToCursor(BOOL* bAborted);
	void FetchToVariables();
	LPSQLCOLUMNDATA FindColumn(char* pColName);
	void FixColumnName(char* pColumn);
	void ExtractParamsAndRewriteStatement(SQLINTEGER* nLen);
	void ParseParamSchema();
	void EvaluateParams();
	void InfoCallbackOrStore();
	void ProgressCallback(int nRowsFetched, BOOL* bAbort);

	LPSQLCOLUMNDATA pColumnData;
	LPSQLPARAMDATA pParamData;
	SQLHSTMT hStmt;
	SQLHDBC hConn;
	SQLSMALLINT nNoOfCols;
	SQLSMALLINT nNoOfParms;
	BOOL bOutputParams;
	FoxString pGetDataBuffer;
	FoxString pCursorName;
	FoxString pCursorNames;
	FoxString pSQLInput;
	FoxArray pInfoArray;
	FoxString pCursorSchema;
	FoxString pParamSchema;
	CFoxCallback pCallback;
	char *pSQLSend;
	char *pCursorname;
	SQLINTEGER nSQLLen;
	int nResultset;
	int nCallbackInterval;
	SQLLEN nRowsTotal;
	int nRowsFetched;
	DWORD nFlags;
	bool bMapVarchar;
	bool bPrepared;
};

#ifdef __cplusplus
extern "C" {
#endif

// function forward definitions
void _stdcall SaveODBCError(char *pFunction, SQLHANDLE hHandle, SQLSMALLINT nHandleType);
void _stdcall ODBCInstallerErrorHandler(char *pFunction);

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
void _fastcall DBSetPropEx(ParamBlkEx& parm);
void _fastcall SQLExecEx(ParamBlkEx& parm);
void _fastcall SQLPrepareEx(ParamBlkEx& parm);
void _fastcall SQLCancelEx(ParamBlkEx& parm);

#pragma warning(disable : 4290)
SqlStatement* _stdcall SQLAllocStatement(ParamBlkEx& parm, bool prepared) throw(int);
#pragma warning(default : 4290)

BOOL _stdcall SQLTypeConvertible(SQLSMALLINT nSQLType, char aVFPType);
unsigned int _stdcall SQLExtractInfo(char *pMessage, unsigned int nMsgLen);

int _stdcall SQLStoreByBinding(LPSQLCOLUMNDATA lpCS);
int _stdcall SQLStoreByBindingVar(LPSQLCOLUMNDATA lpCS);
int _stdcall SQLStoreByGetData(LPSQLCOLUMNDATA lpCS);
int _stdcall SQLStoreByGetDataVar(LPSQLCOLUMNDATA lpCS);
int _stdcall SQLStoreCharByBinding(LPSQLCOLUMNDATA lpCS);
int _stdcall SQLStoreCharByBindingVar(LPSQLCOLUMNDATA lpCS);
int _stdcall SQLStoreCharByGetData(LPSQLCOLUMNDATA lpCS);
int _stdcall SQLStoreCharByGetDataVar(LPSQLCOLUMNDATA lpCS);
int _stdcall SQLStoreDateByBinding(LPSQLCOLUMNDATA lpCS);
int _stdcall SQLStoreDateByBindingVar(LPSQLCOLUMNDATA lpCS);
int _stdcall SQLStoreDateByGetData(LPSQLCOLUMNDATA lpCS);
int _stdcall SQLStoreDateByGetDataVar(LPSQLCOLUMNDATA lpCS);
int _stdcall SQLStoreDateTimeByBinding(LPSQLCOLUMNDATA lpCS);
int _stdcall SQLStoreDateTimeByBindingVar(LPSQLCOLUMNDATA lpCS);
int _stdcall SQLStoreDateTimeByGetData(LPSQLCOLUMNDATA lpCS);
int _stdcall SQLStoreDateTimeByGetDataVar(LPSQLCOLUMNDATA lpCS);
int _stdcall SQLStoreCurrencyByBinding(LPSQLCOLUMNDATA lpCS);
int _stdcall SQLStoreCurrencyByBindingVar(LPSQLCOLUMNDATA lpCS);
int _stdcall SQLStoreCurrencyByGetData(LPSQLCOLUMNDATA lpCS);
int _stdcall SQLStoreCurrencyByGetDataVar(LPSQLCOLUMNDATA lpCS);
int _stdcall SQLStoreMemoChar(LPSQLCOLUMNDATA lpCS);
int _stdcall SQLStoreMemoCharVar(LPSQLCOLUMNDATA lpCS);
int _stdcall SQLStoreMemoWChar(LPSQLCOLUMNDATA lpCS);
int _stdcall SQLStoreMemoWCharVar(LPSQLCOLUMNDATA lpCS);
int _stdcall SQLStoreMemoBinary(LPSQLCOLUMNDATA lpCS);
int _stdcall SQLStoreMemoBinaryVar(LPSQLCOLUMNDATA lpCS);

void _stdcall Timestamp_StructToDateTime(TIMESTAMP_STRUCT *pTime, Value *pDateTime);
void _stdcall DateTimeToTimestamp_Struct(Value *pDateTime, TIMESTAMP_STRUCT *pTime);
void _stdcall Timestamp_StructToDate(TIMESTAMP_STRUCT *pTime, Value *pDateTime);
void _stdcall DateToTimestamp_Struct(Value *pDateTime, TIMESTAMP_STRUCT *pTime);
void _stdcall Numeric_StructToCurrency(SQL_NUMERIC_STRUCT *lpNum, Value *pValue);
void _stdcall CurrencyToNumeric_Struct(Value *pValue, SQL_NUMERIC_STRUCT *lpNum);
void _stdcall NumericLiteralToCurrency(SQLCHAR *pLiteral, Value *pValue);
void _stdcall CurrencyToNumericLiteral(Value *pValue, SQLCHAR *pLiteral);

#ifdef __cplusplus
}
#endif // end of extern "C"

#endif _VFP2CODBC_H__