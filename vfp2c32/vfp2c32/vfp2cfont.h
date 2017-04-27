#ifndef _VFP2CFONT_H__
#define _VFP2CFONT_H__

#pragma pack(push,1)	// set structure padding to 1

typedef struct _TT_TABLE_OFFSET
{
  unsigned short  uMajorVersion;
  unsigned short  uMinorVersion;
  unsigned short  uNumOfTables;
  unsigned short  uSearchRange;
  unsigned short  uEntrySelector;
  unsigned short  uRangeShift;
} TT_TABLE_OFFSET;

//Tables in the TTF file and their placement and name (tag)
typedef struct _TT_TABLE_DIRECTORY
{
  char  szTag[4];      //table name
  unsigned long uCheckSum;     //Check sum
  unsigned long uOffset;       //Offset from beginning of file
  unsigned long uLength;       //length of the table in bytes
} TT_TABLE_DIRECTORY;

//Header of the names table
typedef struct _TT_NAME_HEADER_TABLE
{
  unsigned short uFSelector;      //format selector. Always 0
  unsigned short uNRCount;        //Name Records count
  unsigned short uStorageOffset;  //Offset for strings storage, from 
                          //start of the table
} TT_NAME_HEADER_TABLE;

//Records in the names table
typedef struct _TT_NAME_RECORD
{
  unsigned short uPlatformID;
  unsigned short uEncodingID;
  unsigned short uLanguageID;
  unsigned short uNameID;
  unsigned short uStringLength;
  unsigned short uStringOffset;    //from start of storage area
} TT_NAME_RECORD;

// font headerheader 
typedef struct _TT_HEAD_TABLE
{
int   	tableVersion;
int  	fontRevision;
unsigned long 	checkSumAdjustment;
unsigned long 	magicNumber;
unsigned short 	flags;
unsigned short 	unitsPerEm;
__int64	created;
__int64	modified;
unsigned short 	xMin;
unsigned short 	yMin;
unsigned short 	xMax;
unsigned short 	yMax;
unsigned short 	macStyle;
unsigned short 	lowestRecPPEM;
unsigned short 	fontDirectionHint;
unsigned short 	indexToLocFormat;
unsigned short 	glyphDataFormat;
} TT_HEAD_TABLE;

typedef struct _TT_OS2_TABLE {
unsigned short  version;
short 	xAvgCharWidth;
unsigned short 	usWeightClass;
unsigned short 	usWidthClass;
unsigned short 	fsType;
short 	ySubscriptXSize;
short 	ySubscriptYSize;
short 	ySubscriptXOffset;
short 	ySubscriptYOffset;
short 	ySuperscriptXSize;
short 	ySuperscriptYSize;
short 	ySuperscriptXOffset;
unsigned short 	ySuperscriptYOffset;
unsigned short 	yStrikeoutSize;
unsigned short 	yStrikeoutPosition;
unsigned short 	sFamilyClass;
unsigned char 	panose[10];
unsigned long 	ulUnicodeRange1;
unsigned long 	ulUnicodeRange2;
unsigned long 	ulUnicodeRange3;
unsigned long 	ulUnicodeRange4;
char 	achVendID[4];
unsigned short 	fsSelection;
unsigned short 	usFirstCharIndex;
unsigned short 	usLastCharIndex;
short 	sTypoAscender;
short 	sTypoDescender;
short 	sTypoLineGap;
unsigned short 	usWinAscent;
unsigned short 	usWinDescent;
unsigned long 	ulCodePageRange1;
unsigned long 	ulCodePageRange2;
short 	sxHeight;
short 	sCapHeight;
unsigned short 	usDefaultChar;
unsigned short 	usBreakChar;
unsigned short 	usMaxContext;
} TT_OS2_TABLE;

typedef struct _TT_POST_TABLE
{
int Version;
int italicAngle;
short underlinePosition;
short underlineThickness;
unsigned long isFixedPitch;
unsigned long minMemType42;
unsigned long maxMemType42;
unsigned long minMemType1;
unsigned long maxMemType1;
} TT_POST_TABLE;

#pragma pack(pop)

#define SWAPWORD(x) MAKEWORD(HIBYTE(x), LOBYTE(x))
#define SWAPLONG(x) MAKELONG(SWAPWORD(HIWORD(x)),SWAPWORD(LOWORD(x)))

const int PLATFORMID_UNICODE	= 0;
const int PLATFORMID_WINDOWS	= 3;
const int LANGID_ENGLISH_US		= 1033;

#ifdef __cplusplus
extern "C" {
#endif

// function prototypes of vfp2cfont.cpp
void _fastcall AFontInfo(ParamBlk *parm);

#ifdef __cplusplus
}
#endif // end of extern "C"

TT_NAME_RECORD * __stdcall GetFontNameRecord(TT_NAME_RECORD *pRecords, unsigned short nCount, unsigned short uNameID, unsigned short dwPlatform, LANGID dwLanguage);
void __stdcall BigEndianWideCharToMultiByte(wchar_t *pWString, unsigned int nWideChars, char *pBuffer, unsigned int nBytes);

#endif _VFP2CFONT_H__