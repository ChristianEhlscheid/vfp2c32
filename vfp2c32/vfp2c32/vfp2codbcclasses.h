#ifndef _VFP2CODBCCLASSES_H__
#define _VFP2CODBCCLASSES_H__

#include <windows.h>
#include <sql.h>

#include <odbcinst.h>
#include <sqlext.h>
// #include <odbcss.h> // MS-SQL Server specific include file
#include "msodbcsql.h"
#include "vfp2ccppapi.h"
#include "vfp2codbcconstants.h"

// typedef for dynamic binding to SQLBindParameter for bypassing the Driver Manager, needed for SQL_PARAM_INPUT_OUTPUT_STREAM and SQL_PARAM_OUTPUT_STREAM types
typedef SQLRETURN(_stdcall* PSQLBINDPARAMETER)(SQLHSTMT, SQLUSMALLINT, SQLSMALLINT, SQLSMALLINT, SQLSMALLINT, SQLULEN, SQLSMALLINT, SQLPOINTER, SQLLEN, SQLLEN*);
typedef SQLRETURN(_stdcall* PSQLSETDESCFIELD)(SQLHDESC, SQLSMALLINT, SQLSMALLINT, SQLPOINTER,SQLINTEGER);

typedef struct _SQLHENVPARAM
{
public:
	_SQLHENVPARAM() : hHandle(0) {};
	_SQLHENVPARAM(SQLHENV hEnv) : hHandle(hEnv) {};
	operator bool() { return hHandle != 0; }
	operator SQLHENV() { return hHandle; }
	operator SQLHENV* () { return &hHandle; }
	SQLHENV hHandle;
} SQLHENVPARAM;

typedef struct _SQLHDBCPARAM
{
public:
	_SQLHDBCPARAM() : hHandle(0) {};
	_SQLHDBCPARAM(SQLHDBC hDbc) : hHandle(hDbc) {};
	operator bool() { return hHandle != 0; }
	operator SQLHDBC() { return hHandle; }
	operator SQLHDBC* () { return &hHandle; }
	SQLHDBC hHandle;
} SQLHDBCPARAM;

typedef struct _SQLHSTMTPARAM
{
public:
	_SQLHSTMTPARAM() : hHandle(0) {};
	_SQLHSTMTPARAM(SQLHSTMT hStmt) : hHandle(hStmt) {};
	operator bool() { return hHandle != 0; }
	operator SQLHSTMT() { return hHandle; } 
	operator SQLHSTMT*()  { return &hHandle; } 
	SQLHSTMT hHandle;
} SQLHSTMTPARAM;

typedef struct _SQLHDESCPARAM
{
public:
	_SQLHDESCPARAM() : hHandle(0) {};
	_SQLHDESCPARAM(SQLHDESC hDesc) : hHandle(hDesc) {};
	operator bool() { return hHandle != 0; }
	operator SQLHDESC() { return hHandle; }
	operator SQLHDESC* () { return &hHandle; }
	SQLHDESC hHandle;
} SQLHDESCPARAM;

typedef enum _SqlDBType
{
	UnknownDBServer,
	SQLServer,
	Oracle,
	MySql,
	MariaDB,
	PostgreSQL,
	Informix,
	Firebird,
	Access
} SqlDBType;

// forward declarations
class SqlResultSet;
class SqlColumn;
class SqlParameter;

// function typedef for indirect funtion call's in SQLCOLUMNDATA
typedef int(_fastcall* LPSQLSTOREFUNC)(SqlColumn*);
typedef int(_fastcall* LPSQLSTOREFUNCSET)(SqlColumn*, int);

class SqlParameterArray
{
public:
	SqlParameterArray();
	CStringView pCursor;
	int nWorkArea;
	long nRecCount;
	long nBatchSize;
	long nMaxBatchSize;
	long nLastBatchSize;
	long nBatchRecordsProcessed;
	long nBatchErrorCount;
	SQLLEN nParamsLastProcessed;
	SQLLEN nParamsProcessed;
	SQLLEN nParamsResumeOnError;
	SQLLEN nParamOffset;
	SQLLEN nParamLastOutput;
	AutoRecNoRestore pRecNoRestore;
	CArray<long> pParamRecNo;
	CArray<SQLLEN> pIndicator;
	CArray<SQLUSMALLINT> pParamStatus;
	CArray<SQLUSMALLINT> pParamOperation;
	FoxString pParamStatusDestination;
	FoxCursor pParamStatusCursor;
};

// all common data for a SQL statement, generated resultsets, parameters etc.
class SqlStatement {
public:
	SqlStatement();
	~SqlStatement();

	bool FreeHandle();
	void FreeParameters();
	void Reset();
	bool ParseParameters();
	void AllocateParameters();
	void ExtractParametersAndRewriteStatement();
	void ParseParameterSchema();
	void EvaluateParameters();
	void BindParameters();
	SQLRETURN Exec();
	SQLRETURN SendParameters();
	void StreamParameters();
	void SaveOutputParameters();
	
	// parameter array
	void ParseParameterArraySchema();
	bool UseInMemoryParameterArray();
	void BindParameterArray();
	SQLRETURN SendParameterArray();
	void SaveParameterArrayErrors();
	SQLRETURN ResumeOnParameterArrayError();
	bool LoadNextParameterArrayBatch();

	// table valued parameters
	void BindTableValuedParameter(SqlParameter* lpSP);
	void SendTableValuedParameter(SqlParameter* lpSP);

	// callback
	bool InfoCallbackOrStore();
	void ProgressCallback(int nRowsFetched, bool* bAbort);
	
	// descriptors
	void SetApplicationParameterDescriptorField(SQLSMALLINT nRecNo, SQLSMALLINT nField, SQLPOINTER pValue, SQLINTEGER nBufferLen);
	void GetApplicationParameterDescriptorField(SQLSMALLINT nRecNo, SQLSMALLINT nField, SQLPOINTER pValue, SQLINTEGER nBufferLen, SQLINTEGER* nStringLen);
	void SetImplementationParameterDescriptorField(SQLSMALLINT nRecNo, SQLSMALLINT nField, SQLPOINTER pValue, SQLINTEGER nBufferLen);
	void GetImplementationParameterDescriptorField(SQLSMALLINT nRecNo, SQLSMALLINT nField, SQLPOINTER pValue, SQLINTEGER nBufferLen, SQLINTEGER* nStringLen);
	void GetImplementationRowDescriptorField(SQLSMALLINT nRecNo, SQLSMALLINT nField, SQLPOINTER pValue, SQLINTEGER nBufferLen, SQLINTEGER* nStringLen);

	SqlResultSet* AddResultSet();
	SQLSMALLINT GetDefaultDecimals();
	SQLSMALLINT GetColumnCount();
	void UnbindColumns();
	SQLLEN GetRowCount();
	void SaveRowCount();

	SqlDBType ServerType;
	CDynamicPtrArray<SqlResultSet> pResultsets;
	CPtrArray<SqlParameter> pParameters;
	SqlParameterArray pParameterArray;
	SQLHSTMTPARAM hStmt;
	SQLHDBCPARAM hConn;
	FoxString pGetDataBuffer;
	FoxString pCursorNames;
	FoxString pSQLInput;
	FoxString pSQLSend;
	FoxString pCursorSchema;
	FoxString pParamSchema;
	FoxArray pInfoArray;
	CFoxCallback pCallback;
	int nResultset;
	int nCallbackInterval;
	SQLLEN nRowSetSize;
	SQLLEN nRowsFetched;
	CArray<SQLLEN> pRowStatus;
	SQLLEN nRowsTotal;
	DWORD nFlags;
	DWORD nUseMemoSize;
	bool bOutputParams;
	bool bMapVarchar;
	bool bMapVarbinary;
	bool bMapBinaryToGeneral;
	bool bUnicodeConversion;
	bool bStoreInfoAsError;
	bool bPrepared;
	bool bExecutedOnce;

private:
	int nParameterCount;
	bool bResumeOnError;
	SQLHDESCPARAM hAppParamDesc;
	SQLHDESCPARAM hImpParamDesc;
	SQLHDESCPARAM hAppRowDesc;
	SQLHDESCPARAM hImpRowDesc;
	SQLSMALLINT nDefaultDecimals;
	void LoadInMemoryParameterArray();
	void FreeInMemoryParameterArray();
	void SetParameterArrayAttributes(bool bInMemory);
	void SaveParameterArrayErrorsToCursor(CArray<SQLINTEGER>& pErrorMap);
	SqlParameter* FindParameter(CStringView pField);
	bool MatchLength(CStringMatcher& pMatcher, SqlParameter* lpSC);
	bool MatchPrecision(CStringMatcher& pMatcher, SqlParameter* lpSC);
	bool MatchLengthAndPrecision(CStringMatcher& pMatcher, SqlParameter* lpSC);
	unsigned int ExtractInfo(char* pMessage, unsigned int nMsgLen);
	void LoadDriverHandles(HMODULE& hMod, SQLHSTMT& hDriverStmt, SQLHSTMT hStmt);
};

class SqlResultSet
{
public:
	SqlResultSet();
	~SqlResultSet();

	void AllocateColumns(SQLSMALLINT nColCount);
	void GetColumnMetaData();
	void BindColumns();
	void BindFieldLocators();
	void BindVariableLocators();
	void SetCursorName();
	void SaveColumnBinding();
	void ParseCursorSchema();
	void ParseExistingCursorSchema();
	void PrepareColumnBindings();
	void CreateCursor();
	void FetchToCursor(bool* bAborted);
	void FetchToVariables();
	void FetchOutputColumns(SqlParameterArray& pParamArray);
	
	SqlStatement* pStmt;
	SQLHDESCPARAM hAppRowDesc;
	CPtrArray<SqlColumn> pColumns;
	FoxString pCursorName;
	AutoRecNoRestore pRecNoRestore;
	SQLLEN nRowsFetched;
	int nWorkArea;
	bool bStoreInfoAsError;
	bool bRowSetFetchCapable;

private:
	bool bRebindColumn;
	void FreeColumnBinding();
	void ReleaseColumnBuffers();
	SqlColumn* FindColumn(CStringView pColName);
	void AllocateColumnBuffer(SqlColumn* lpSC, int nLen, bool bBindColumn);
	void AllocateColumnBufferSet(SqlColumn* lpSC, int nLen);
};

// holds data for each column in a SQL resultset
class SqlColumn {
public:
	SqlColumn();
	SqlStatement* pStmt;
	LocatorEx lField;
	ValueEx vData;
	SQLPOINTER pDataPtr;
	SQLUSMALLINT nColNo;
	SQLULEN nSize;
	SQLLEN nBufferSize;
	SQLLEN nIndicator;
	SQLLEN bUnsigned;
	SQLLEN bMoney;
	SQLLEN nDisplaySize;
	SQLSMALLINT nSQLType;
	SQLSMALLINT nCType;
	SQLSMALLINT nScale;
	SQLSMALLINT bNullable;
	SQLSMALLINT bBinary;
	bool bBindColumn;
	bool bCustomSchema;
	bool bStoreInfoAsError;
	union
	{
		LPSQLSTOREFUNC pStore;
		LPSQLSTOREFUNCSET pStoreSet;
	};
	FCHAN hMemoFile;
	SQLCHAR cVFPType;
	CStrBuilder<VFP2C_ODBC_MAX_COLUMN_NAME + 1> pColumnName;
	union
	{
		SQL_DATE_STRUCT sDateStruct;
		SQL_TIMESTAMP_STRUCT sTimestampStruct;
		SQL_NUMERIC_STRUCT sNumericStruct;
		SQL_INTERVAL_STRUCT sIntervalStruct;
	};
};

class SqlTableTypeParameter
{
public:
	SqlTableTypeParameter();
	LocatorEx lField;
	SQLINTEGER nDescAutoUniqueValue;
	SQLSMALLINT nColumnComputed;
	SQLSMALLINT nColumnInUniqueKey;
	SQLSMALLINT nColumnSortOrder;
	SQLSMALLINT nColumnSortOrdinal;
	SQLSMALLINT nColumnHasDefaultValue;
};

class SqlTableTypeColumn
{
public:
	SqlTableTypeColumn();
	CStrBuilder<VFP2C_SQLSERVER_MAX_TYPENAME + 1> pColumnName;
	SQLUSMALLINT nColumnNo;		// column no - 0 based
	SQLULEN nSize;			// size of parameter in bytes
	SQLULEN nDisplaySize;	// octet length
	SQLSMALLINT nSQLType;		// the SQL datatype for the parameter
	SQLSMALLINT nSQLTypeCode;	// typecode for SQL_DATETIME and SQL_INTERVAL
	SQLSMALLINT nCType;			// the C datatype for the parameter
	SQLSMALLINT nScale;			// scale of numeric types
	SQLSMALLINT bMoney;			// fixed monetary type
	SQLSMALLINT bNullable;		// nullable
	bool bVFPBinary;			// VFP binary flag
	SQLCHAR cVFPType;			// VFP datatype
};

class SqlTableType
{
public:
	SqlTableType(CStringView vCatalog, CStringView vSchema, CStringView vType);
	const char* Key() const;
	CDynamicPtrArray<SqlTableTypeColumn> pColumns;
	CStrBuilder<VFP2C_SQLSERVER_MAX_TYPENAME + 1> pTypename;
	CStrBuilder<VFP2C_SQLSERVER_MAX_TYPENAME + 1> pCatalog;
	CStrBuilder<VFP2C_SQLSERVER_MAX_TYPENAME + 1> pSchema;
	SQLULEN nUseMemoSize;
	bool bMapVarchar;
	bool bMapVarbinary;
	bool bMapBinaryToGeneral;
	bool bUnicodeConversion;
	SQLSMALLINT nDefaultDecimals;
private:
	CStrBuilder<VFP2C_SQLSERVER_MAX_TYPENAME * 3 + 1> pMapKey;
};

// holds data for each parameter in a SQL statement
class SqlParameter {
public:
	SqlParameter();
	void DeduceParameterType();
	void DeduceParameterType(char cType, bool bBinary);

	char aParmExpr[VFP2C_ODBC_MAX_PARAMETER_EXPR];		// buffer into which the parameter expression is stored
	CStringView pParmName;		// name of parameter, if it's a named parameter
	CStringView pParmTypeCatalog; // catalog of TVP parameter
	CStringView pParmTypeSchema; // schema of TVP parameter
	CStringView pParmTypeName;	// name of parameter type if it's a TVP (table valued parameter)
	LocatorEx lVarOrField;		// for output parameters one can only pass variables or fieldnames of course, this will hold the Locator referencing the variable/field
	ValueEx vParmValue;			// Value structure into which the parameter is stored
	ValueEx vMemoValue;			// buffer for memo data
	SQLPOINTER pParmPtr;		// pointer to the data of the parameter
	SQLUSMALLINT nParmNo;		// parameter number (1 based)
	SQLUINTEGER nSize;			// size in bytes or precision of parameter, depending on SQL datatype
	SQLINTEGER nBufferSize;		// size of buffer in bytes for output parameters
	SQLLEN nIndicator;			// indicator if the parameter is NULL, it's size or SQL_LEN_DATA_AT_EXEC macro
	SQLSMALLINT nParmType;		// input, (streaming) input/output or output parameter ?
	SQLSMALLINT nSQLType;		// the SQL datatype for the parameter
	SQLSMALLINT nSQLTypeCode;	// subtype for SQL_DATETIME, SQL_TYPE_TIMESTAMP, SQL_TIME, SQL_INTERVAL etc...
	SQLSMALLINT nCType;			// the C datatype for the parameter
	SQLSMALLINT nScale;			// scale of numeric types
	int nWorkArea;				// workarea of cursor if it's a table valued parameter
	SqlTableType* pTableType; // sql server table type
	CPtrArray<SqlTableTypeParameter> pTableTypeParameters;	// parameters for table valued parameter 
	char cVFPType;				// static Foxtype for use in parameter arrays
	bool bVFPBinary;			// binary flag for use in parameter arrays
	bool bPutData;				// bind parameter or use SQLParamData/SQLPsutData for long values (char/binary > 255 bytes)
	bool bCustomSchema;			// use custom type from parameter schema?
	union
	{
		SQL_DATE_STRUCT sDateStruct;
		SQL_TIMESTAMP_STRUCT sTimestampStruct;
		SQL_NUMERIC_STRUCT sNumericStruct;
		SQL_INTERVAL_STRUCT sIntervalStruct;
	};
};

#pragma warning( push )
#pragma warning( disable : 4200 )
typedef struct _SqlVariant
{
	SQLSMALLINT nType;
	SQLSMALLINT nSize;
	SQLSMALLINT nScale;
	char pData[0];
} SqlVariant;
#pragma warning( pop )

class CSimpleArraySqlTableEqualHelper
{
public:
	static bool IsEqualKey(
		_In_ const char* const& t1,
		_In_ const char* const& t2)
	{
		return (strcmp(t1, t2) == 0);
	}

	static bool IsEqualValue(
		_In_ const SqlTableType*& v1,
		_In_ const SqlTableType*& v2)
	{
		return (strcmp(v1->Key(), v2->Key()) == 0);
	}
};

typedef CSimpleMap<const char*, SqlTableType*, CSimpleArraySqlTableEqualHelper> SqlTTMap;
// extern definitions
extern SqlTTMap g_RegisteredTableTypes;
#endif
