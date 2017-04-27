#ifndef _VFP2CDBF_H__
#define _VFP2CDBF_H__

// filetype's in DBFTABLEHEADER
#define DBF_FILETYPE_FOXBASE			0x03
#define DBF_FILETYPE_FOXBASEPLUS		0x30
#define DBF_FILETYPE_FOXBASEPLUS_MEMO	0x8B
#define DBF_FILETYPE_VISUALFOX			0x31
#define DBF_FILETYPE_VISUALFOXAUTO		0x32
#define DBF_FILETYPE_VISUALFOXVAR		0x43		
#define DBF_FILETYPE_DBASE4				0x63
#define DBF_FILETYPE_DBASE4_MEMO		0xCB
#define DBF_FILETYPE_DBASE4_SYSTEM		0x83
#define DBF_FILETYPE_DBASE4_SYSTEM_MEMO	0xF5
#define DBF_FILETYPE_FOXPRO				0xFB

// table flags in DBFTABLEHEADER
#define DBF_FLAG_CDX	0x01
#define DBF_FLAG_MEMO	0x02
#define DBF_FLAG_DBC	0x04

// dbf header terminator
#define DBF_HEADER_TERMINATOR	0x0D

// field flags in DBFIELDHEADER
#define DBF_FIELD_FLAG_SYSTEM	0x01
#define DBF_FIELD_FLAG_NULL		0x02
#define DBF_FIELD_FLAG_BINARY	0x04
#define DBF_FIELD_FLAG_AUTOINC	0x0C

// fieldheader
typedef struct _DBFFIELDHEADER
{
	char Name[11];
	char Type;
	long Displacement;
	unsigned char Length;
	unsigned char Decimals;
	unsigned char Flags;
	long NextAutoIncValue;
	unsigned char AutoIncStep;
	char aReserved[7];
} DBFFIELDHEADER, *LPDBFFIELDHEADER;

// DBF table header
typedef struct _DBFTABLEHEADER
{
	char Filetype;
	struct LastUpdate
	{
		unsigned char nYY;
		unsigned char nMM;
		unsigned char nDD;
	};
	long NumberOfRecords;
	short DataSectionOffset;
	short RecordLength;
	char aReserved[16];
	char TableFlags;
	char CodePage;
	char aReserved2[2];
	DBFFIELDHEADER aFields[];
} DBFTABLEHEADER, *LPDBFTABLEHEADER;

#endif // _VFP2CDBF_H__
