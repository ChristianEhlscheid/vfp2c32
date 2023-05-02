//+--------------------------------------------------------------------------
//
//	File:		pro_ext.h
//
//	Copyright:	(c) 1999, Microsoft Corporation.
//				All Rights Reserved.
//				Information Contained Herein is Proprietary
//				and Confidential.
//
//	Contents:	API Header for the Library Construction Kit
//
//	Notes: 
//
//---------------------------------------------------------------------------
#ifndef PRO_EXT_INCLUDED
#define PRO_EXT_INCLUDED

#ifdef __cplusplus
extern "C" {            			// Assume C declarations for C++
#endif

#define MAC_API 0

//
//	Semi-portable way to deal with 'far' pointers 
//

#ifndef FAR
	#if !defined(_WIN32)
		#define FAR
	#endif
#endif  // FAR

//
// Global Defines:
//

#ifndef GLOBAL_INCLUDED

#if !defined(_MT) || !defined(_DLL) 
	#error The /MD or /MDd compiler switch (Multithreaded Using DLL) is required.
#endif


//
// Microsoft C for Windows
//
#include <windows.h>

#if defined(_MSC_VER)
#pragma pack(push, 1)         		// Assume byte structure packing
#endif

#define _far
#define __far


// Fastcall calling convention
#define FASTCALL _fastcall

typedef unsigned MHANDLE;       		// A memory handle.
typedef long		 NTI;					// A name table index.
typedef unsigned __int64 Ticks;			// A timer tick count.
#define TRUE	1
#define FALSE	0
#define YES		1
#define NO		0
typedef char TEXT;

#define MAXFILENAME					261		/* longest pathname */

#endif      // GLOBAL_INCLUDED

typedef unsigned MHandle;				// An api memory handle (equivalent to MHANDLE)
typedef unsigned long WHandle;			// A Window Handle
#ifndef FOXMENU_INCLUDED
	typedef long MENUID;					// A Menu id.
#endif
typedef long MenuId;					// A Menu id.
typedef long ITEMID;					// An item id.
typedef long ItemId;					// An item id.

typedef int		 FCHAN;					// A file I/O channel.
#define SCHEMEWIDTH		11
typedef char Scheme[SCHEMEWIDTH];		// A FoxPro color scheme


#ifdef GLOBAL_INCLUDED
#define WHANDLE WHandle
#else
typedef WHandle WHANDLE;				// Users can use WHANDLE
#endif

//
// FoxWindow Defines:
//

#ifndef FOXWIND_INCLUDED

// _WOpen() flag values:
#define WCURSOR		(1<<1)				/* Enable the cursor						*/
#define ZOOM		(1<<2)				/* Permit the window to be zoomed			*/
#define ADJ			(1<<3)				/* Allow Window Size to be Adjusted			*/
#define CLOSE		(1<<4)				/* Permit Closing of System Window			*/
#define MOVE		(1<<5)				/* Allow the window to be moved				*/
#define HSCROLLBAR	(1<<6)				/* Show Horizontal Scrollbars				*/
#define VSCROLLBAR	(1<<7)				/* Show Vertical Scrollbars					*/
#define AUTOSCROLL	(1<<8)				/* Window should Auto scroll				*/
#define WEVENT		(1<<10) 			/* Participates in activate/deact events	*/
#define SHADOW		(1<<11) 			/* Window will produce a shadow				*/
#define WMODAL		(1<<12) 			/* Window is a modal window					*/
#define WMINIMIZE	(1<<13) 			/* Window can be minimized					*/

#endif      // FOXWIND_INCLUDED

//
// FoxEvent Defines:
//

#ifndef FOXEVENT_INCLUDED

//  _FindWindow() return values.
#define inBorder		 0				/* In the window border region			*/
#define inHelp			 1				/* In the Help region					*/
#define inContent		 2				/* In the content/text region			*/
#define inDrag			 3				/* In the Drag/Title bar region			*/
#define inGrow			 4				/* In the grow/resize region			*/
#define inGoAway		 5				/* In the goAway/close region			*/
#define inZoom			 6				/* In the zoom region					*/
#define inVUpArrow		 7				/* In the vertical up arrow region		*/
#define inVDownArrow	 8				/* In the vertical down arrow region	*/
#define inVPageUp		 9				/* In the vertical page up region		*/
#define inVPageDown		10				/* In the vertical page down region		*/
#define inVThumb		11				/* In the vertical thumb region			*/
#define inHUpArrow		12				/* In the horizontal up arrow region	*/
#define inHDownArrow	13				/* In the horizontal down arrow region	*/
#define inHPageUp		14				/* In the horizontal page up region		*/
#define inHPageDown		15				/* In the horizontal page down region	*/
#define inHThumb		16				/* In the horizontal thumb region		*/
#define inMenuBar		17				/* In the menu bar						*/


// EventRec.what values
#define nullEvent				0		/* Null							*/
#define activateEvent			1		/* Activate window				*/
#define deactivateEvent			2		/* Deactivate window			*/
#define showEvent			  	3		/* Show window					*/
#define hideEvent				4		/* Hide window					*/
#define updateEvent				5		/* Redraw window				*/
#define sizeEvent			   	6		/* Size window event			*/
#define moveEvent			   	7		/* Move window event			*/
#define closeEvent				8		/* Close window					*/
#define mouseDownEvent			9		/* Left mouse down				*/
#define mouseUpEvent		   10		/* Left mouse up				*/
#define mMouseDownEvent		   11		/* Middle mouse down event		*/
#define mMouseUpEvent		   12		/* Middle mouse up event		*/
#define rMouseDownEvent		   13		/* Right mouse down event		*/
#define rMouseUpEvent		   14		/* Right mouse up event			*/
#define mouseMoveEvent		   15		/* Mouse move event				*/
#define mouseWheelEvent		   16		/* Mouse wheel event			*/
#define keyDownEvent		   17		/* Key down						*/
#define hotkeyEvent			   18		/* An ON KEY LABEL was pressed	*/
#define menuInitEvent		   19		/* Menu initialization event	*/
#define menuUpdateEvent		   20		/* Menu update required. No longer used	*/
#define menuHitEvent		   21		/* Menu hit						*/
#define toolbarEvent		   22		/* Toolbar button hit			*/
#define alarmEvent			   25		/* Alarm/Timer event			*/
#define zoomEvent			  999		/* Not supported in Windows/Mac	*/

// EventRec.modifiers defines
#define charCodeMask	0x0fff			/* Character code mask			*/
#define shiftCodeMask	0xf000			/* Shift key mask				*/

#define shiftKey		0x1000			/* Either shift key	   1 - down */
#define ctrlKey			0x2000			/* Control key		   1 - down */
#define altKey			0x4000			/* Alternate key	   1 - down */
#define literalKey		0x8000			/* This key is interpreted literally */


// EventRec.mbState code defines
#define leftButton		(1<<0)			/* Left button status	1 - down */
#define rightButton		(1<<1)			/* Right button status	1 - down */
#define centerButton	(1<<2)			/* Center button status 1 - down */


// EventRec.mcState code defines
#define posChange		(1<<0)			/* Position change		  1 - change   */
#define leftPressed		(1<<1)			/* Left button pressed	  1 - pressed  */
#define leftReleased	(1<<2)			/* Left button released	  1 - released */
#define rightPressed	(1<<3)			/* Right button pressed	  1 - pressed  */
#define rightReleased	(1<<4)			/* Right button released  1 - released */
#define centerPressed	(1<<5)			/* Center button pressed  1 - pressed  */
#define centerReleased	(1<<6)			/* Center button released 1 - released */

#endif      // FOXEVENT_INCLUDED


// Flag values for _InKey()
#define SHOWCURSOR		(1<<0)
#define HIDECURSOR		(1<<1)
#define MOUSEACTIVE		(1<<2)

// Flag values for the _ALen() function
enum {
	AL_ELEMENTS,
	AL_SUBSCRIPT1,
	AL_SUBSCRIPT2
};

// FPFI is a 32 bit pointer to a function returning an int
typedef long (FAR *FPFI)();

#ifndef GLOBAL_INCLUDED

typedef struct
{
    LONG h;
    LONG v;
} FoxPoint;

#define FoxRect RECT

#endif          // GLOBAL_INCLUDED

#if defined(_WIN32)
#define Point FoxPoint
#define Rect FoxRect
#endif

#ifndef FOXMENU_INCLUDED

// Menu structure type entry definitions used by _NewMenu()
#define MPOPUP			1				/* Menu is a POPUP type menu	*/
#define MPAD			2				/* Menu is a PAD type menu		*/
#define MBAR			3				/* Menu is a BAR type menu		*/

#endif      // FOXMENU_INCLUDED


// These are the API identifiers for the System Menu Constants.
enum {
	_LASTITEM = -2, _FIRSTITEM,

	_SYSMENU,
	_SYSMSYSTEM, _SYSMFILE, _SYSMEDIT, _SYSMDATA, _SYSMRECORD, _SYSMPROG,
	_SYSMWINDOW, _SYSMVIEW, _SYSMTOOLS, _SYSMFORMAT,

	_SYSTEM,
	_SYSTEMABOUT, _SYSTEMHELP, _SYSTEMMACRO, _SYSTEMSEP100, _SYSTEMFILER,
	_SYSTEMCALC, _SYSTEMDIARY, _SYSTEMSPECIAL, _SYSTEMASCII,
	_SYSTEMCAPTURE, _SYSTEMPUZZLE, _SYSTEMTECHSUPPORT, _SYSTEMOFFICE, _SYSTEMSEP200, 
	_SYSTEMSEP300, _SYSTEMHELPSRCH, _SYSTEMHELPHOWTO, _SYSTEMDBASE,

	_FILE,
	_FILENEW, _FILEOPEN, _FILECLOSE, _FILECLOSEALL, _FILESEP100, _FILESAVE,
	_FILESAVEAS, _FILEREVERT, _FILESEP200, _FILESETUP, _FILEPRINT, _FILEOSPRINT, 
	_FILEPRINTONECOPY, _FILESEP300,	_FILEQUIT, _FILEPRINTPREVIEW, _FILEPAGESETUP,
	_FILEIMPORT, _FILEEXPORT, _FILESEP400, _FILESEND,

	_EDIT,
	_EDITUNDO, _EDITREDO, _EDITSEP100, _EDITCUT, _EDITCOPY, _EDITPASTE,
	_EDITCLEAR, _EDITSEP200, _EDITSELECTALL, _EDITSEP300, _EDITGOTO,
	_EDITFIND, _EDITFINDAGAIN, _EDITREPLACE, _EDITREPLACEALL, _EDITSEP400, 
#if 0
	_EDITBUILDEXPR,	_EDITEXECUTEBLOCK, _EDITSEP600, 
#endif
	_EDITPREF,

	_DATA,
	_DATASETUP, _DATABROWSE, _DATASEP100, _DATAAPPEND, _DATACOPY, _DATASORT,
	_DATATOTAL, _DATASEP200, _DATAAVERAGE, _DATACOUNT, _DATASUM, _DATACALCULATE,
	_DATAREPORT, _DATALABEL, _DATASEP300, _DATAPACK, _DATAREINDEX,

	_RECORD,
	_RECORDAPPEND, _RECORDCHANGE, _RECORDSEP100, _RECORDGOTO, _RECORDLOCATE,
	_RECORDCONTINUE, _RECORDSEEK, _RECORDSEP200, _RECORDREPLACE, _RECORDDELETE,
	_RECORDRECALL,

	_PROG,
	_PROGDO, _PROGSEP100, _PROGCANCEL, _PROGRESUME, _PROGSEP200,
	_PROGCOMPILE, _PROGGENERATE, _PROGDOCUMENT, _PROGGRAPH, _PROGSUSPEND,
	_PROGFORMWIZARD,

	_WINDOW,
	_WINDOWHIDE, _WINDOWHIDEALL, _WINDOWSHOWALL, _WINDOWCLEAR, _WINDOWSEP100,
	_WINDOWMOVE, _WINDOWSIZE, _WINDOWZOOM, _WINDOWMINMAX, _WINDOWROTATE,
	_WINDOWCOLOR, _WINDOWSEP200, _WINDOWCOMMAND, _WINDOWDEBUG, _WINDOWTRACE,
	_WINDOWVIEW, _WINDOWARRANGEALL, _WINDOWTOOLBARS, _WINDOWLAYOUTS,

	_VIEW,

	_TOOLS,
	_TOOLSWIZARDS, _TOOLSFILLER1, _TOOLSFILLER2, _TOOLSFILLER3, _TOOLSOPTIONS,
	_TOOLSDEBUGGER, _TOOLSTRACE, _TOOLSWATCH, _TOOLSLOCALS, _TOOLSDBGOUT,
	_TOOLSSTACK, _TOOLSFILLER4, _TOOLSTOOLBOX, _TOOLSTASKPANE, _TOOLSREFERENCES, 

	_REPORT, _LABEL, 

	_BROWSE, _BROWSEMODE, _BROWSEGRID, _BROWSELINK, _BROWSECHPART,
	_BROWSEFILLER1, _BROWSEFONT, _BROWSESFIELD, _BROWSEMFIELD, _BROWSEMPART,
	_BROWSEFILLER2, _BROWSEGOTO, _BROWSESEEK, _BROWSEDELREC, _BROWSEAPPEND,	

	_MACRO, _DIARY, _DAFILER, _SCREEN,
	
	_MBLDR, 
	_MBLDRGLOBAL, _MBLDRMENUOPT, _MBLDRFILLER1, _MBLDRTRYIT, _MBLDRFILLER2,
 	_MBLDRINSERT, _MBLDRDELETE, _MBLDRMOVEITEM, _MBLDRFILLER3, _MBLDRQUICK, _MBLDRGENERATE,

	_PROJECT, _RQBE,

	_EDITPASTELINK, _EDITLINK,
	_EDITINSERTOBJ,	_EDITCVTSTATIC, _EDITSEP500,  _PROGSEP300,

	_TEXT,

	_WIZARDLIST,
	_WIZARDTABLE, _WIZARDQUERY, _WIZARDFORM, _WIZARDREPRT, _WIZARDLABEL,
	_WIZARDMAIL, _WIZARDPIVOT, _WIZARDIMPORT, _WIZARDFOXDOC, _WIZARDSETUP, 
	_WIZARDUPSIZING, _WIZARDALL,

	_TABLE, 
	_TABLEPROPERTIES, _TABLEFILLER1, _TABLEGOTO, _TABLEAPPEND,
	_TABLEDELREC, _TABLEFILLER2, _TABLEDELETE, _TABLERECALL, _TABLESFIELD,
	_TABLEMFIELD, _TABLEMPART, _TABLEFILLER3, _TABLELINK, _TABLECHPART, _TABLEFILLER4,

	_EDITOBJECT, _PROGBEAUT, _SYSTEMDOCUMENTATION, _SYSTEMSAMPLEAPPS, 
	_MBLDRINSERTBAR, _EDITBEAUTIFY,
	_TOOLSBROWSER, _FOXCODE,

	_WEBMENU, 
	_HELPWEBVFPFREESTUFF,_HELPWEBVFPHOMEPAGE, _HELPWEBVFPFAQ,
	_HELPWEBVFPONLINESUPPORT, _HELPWEBFILLER1, _HELPWEBDEVONLY, _HELPWEBVFPSENDFEEDBACK, _HELPWEBDIRECTORY,
	_HELPWEBSEARCH, _HELPWEBTUTORIAL, _HELPWEBFILLER2, _HELPWEBMSFTHOMEPAGE,

	// New for 6.0 (always add new ones to the end!)
	_WIZARDAPPLICATION, _WIZARDDATABASE, _WIZARDWEBPUBLISHING,
	_TOOLSGALLERY, _TOOLSCOVERAGE, _TOOLSRUNACTIVEDOC,
	_TOOLSOBJBROWSER, _TOOLSTASKLIST, _FILESAVEASHTML,
	_SYSTEMMSDNCONTENTS, _SYSTEMMSDNINDEX, _SYSTEMMSDNSEARCH,
	_HELPWEBMSDNONLINE, _HELPWEBVSPRODNEWS, 
	_TOOLSDOCVIEW, _TOOLSBREAKPOINT, 
	_EDIT_LISTMEMBERS, _EDIT_QUICKINFO, _EDIT_BOOKMARKS,
	_WINDOW_CASCADE, _WINDOW_DOCKABLE, _WINDOW_PROPERTIES,

	_BKMKTOGTASK, _BKMKTOGBKMK, _BKMKNEXT, _BKMKPREV,
	_SYSTEMVFPWEB, _WIZARDWEBSERVICES

};


#ifdef _WIN64
// VFP Advanced (x64)
#define _BreakPoint() DebugBreak()
#else
#define _BreakPoint() __asm     \
{	                            \
	 int 3h                 	\
}
#endif


// Alternate values for parmCount to modify how FoxPro treats the function
#define INTERNAL		-1		/* Not callable from FoxPro				*/
#define CALLONLOAD		-2		/* Call when library is loaded			*/
#define CALLONUNLOAD	-3		/* Call when library is unloaded		*/


// The FoxInfo structure contains the descriptions of the functions
// contained in the library.
typedef struct {
	char FAR *	funcName;		/* Function name (all caps)				*/
    FPFI        function;       /* Actual function address              */
	short		parmCount;		/* # parameters specified or a flag value */
	char FAR *	parmTypes;		/* Parameter list description			*/
} FoxInfo;

typedef struct _FoxTable {
	struct _FoxTable FAR *nextLibrary;	/* Linked list of libraries		*/
	short infoCount;			/* # of functions in this library		*/
	FoxInfo FAR *infoPtr;		/* Function list						*/
} FoxTable;


typedef LARGE_INTEGER CCY;

// An expression's value
typedef struct {
	char				ev_type;
	char				ev_padding;
	short				ev_width;
	unsigned 			ev_length;
	long				ev_long;
	double				ev_real;
	CCY					ev_currency;
	MHandle				ev_handle;
	unsigned long		ev_object;
} Value;

// A reference to a database or memory variable
typedef struct {
	char		l_type;
	SHORT		l_where;		/* Database number or -1 for memory		*/
	USHORT		l_NTI,			/* Variable name table offset			*/
				l_offset,		/* Index into database					*/
				l_subs,			/* # subscripts specified 0 <= x <= 2	*/
				l_sub1, l_sub2; /* subscript integral values			*/		// these are not changed to 4 bytes so FLL compatibility is maintaines
} Locator;

// A parameter to a library function.
#ifndef __Parameter_FWD_DEFINED__
#define FoxParameter Parameter
#endif

typedef union {
	Value		val;
	Locator		loc;			/* An 'R' in l_type means the Locator	*/
								/* part of this union is in use.		*/
} FoxParameter;

// A paramter list to a library function.
typedef struct {
	short 		pCount;			/* Number of Parameters PASSED.			*/
	FoxParameter	p[1];		/* pCount Parameters.					*/
} ParamBlk;


HANDLE _GetAPIHandle(void);

typedef long EDPOS;					/* Editor text offset					*/
typedef long EDLINE;				/* Editor line number					*/

#define MAXFONTNAME		64			/* Max length of a font name			*/

#ifndef EDITOR_INCLUDED

#define EDCOMMAND		0
#define EDSCRIP			EDCOMMAND
#define EDPROGRAM		1
#define EDFILE			2
#define EDMEMO			3
#define EDQUERY			6
#define EDSCREEN		7
#define EDMENU			8
#define EDVIEW			9
#define EDSNIP		   10
#define EDTEXT		   11
#define EDPROC		   12
#define EDPROJTEXT     13

#define RO_BYFILE		(0x01)		/* readOnly bits						*/
#define RO_BYUSER		(0x02)
#define RO_BYRECORD		(0x04)

#endif      // EDITOR_INCLUDED


// An editor's environment.
typedef struct{
	char 			filename[MAXFILENAME];
	EDPOS			length;			/* #bytes in text							*/
	unsigned short	lenLimit;		/* Max allowable length. 0 = infinite.		*/
	unsigned short	dirty,			/* Has the file been changed?				*/
					autoIndent,		/* Auto indent?								*/
					backup,			/* Make backup files?						*/
					addLineFeeds, 	/* add line feeds when saving?				*/
					autoCompile,	/* Shall we auto compile this thing?		*/
					addCtrlZ,		/* add end of file ctrl-z?					*/
					savePrefs,		/* save edit preferences?					*/
					dragAndDrop,	/* Allow drag-and-drop						*/
					readOnly,		/* RO_BYFILE, RO_BYUSER, RO_BYRECORD		*/
					syntaxColor,	/* Syntax coloring in effect for this file? */
					status,			/* display status bar?						*/
					lockPrefs,		/* Can update the preferences ?				*/
					insertMode;
	short			wrap;			/* if <0, new line at Return only			*/

	EDPOS			selStart;		/* Selection start							*/
	EDPOS			selEnd;			/* Selection end							*/
	EDPOS			selAnchor;		/* Selection anchor point					*/
	short			justMode;		/* Justification							*/
	short			tabWidth;		/* TAB size in spaces						*/

	char 	 		fontName[MAXFONTNAME];
	short			fontSize;
	short			fontStyle;
	short			kind;			/* Kind of editor session					*/
} EDENV;

// Event record definitions
typedef struct
{	unsigned short		what;			/* Event code					*/
	Ticks				when;			/* Ticks since startup			*/
	FoxPoint			where;			/* Mouse location				*/
	unsigned long		message;		/* Key/window					*/
	unsigned long		misc;			/* Event dependant misc info	*/
	unsigned long		misc2;			/* Event dependant misc info	*/
	unsigned char		mbState;		/* Mouse buttons state			*/
	unsigned char		mcState;		/* Mouse cond activate state	*/
	unsigned long		modifiers;
} EventRec;


// Flags for the _DBStatus function
#define DB_BOF			1		/* BOF()								*/
#define DB_EOF			2		/* EOF()								*/
#define DB_RLOCKED		4		/* Current record is RLOCKed			*/
#define DB_FLOCKED		8		/* Database is FLOCKed					*/
#define DB_EXCLUSIVE	16		/* Database is open EXCLUSIVEly			*/
#define DB_READONLY		32		/* Database is READONLY					*/

// Flag values for _DBLock()
#define DBL_RECORD		0
#define DBL_FILE		1

// Flag values for the _NewVar function
#define NV_PUBLIC		0
#define NV_PRIVATE		1

// Mode flag values for the __FIO function
#define FIO_FGETS		0
#define FIO_FREAD		1
#define FIO_FPUTS		2
#define FIO_FWRITE		3

// Mode flag values for the _FOpen function
#define FO_READONLY		0
#define FO_WRITEONLY	1
#define FO_READWRITE	2

// Mode flag values for the _FCreate function
#define FC_NORMAL		0
#define FC_READONLY		1
#define FC_HIDDEN		2
#define FC_SYSTEM		4
#define FC_TEMPORARY	128

// Mode flag values for the _FSeek function
#define FS_FROMBOF		0		/* From beginning of file		*/
#define FS_RELATIVE		1		/* From current position		*/
#define FS_FROMEOF		2		/* From end of file				*/

// Mode flag values for the __WStat function
#define WS_TOP			0
#define WS_BOTTOM		1
#define WS_LEFT			2
#define WS_RIGHT		3
#define WS_HEIGHT		4
#define WS_WIDTH		5
#define WS_SETPORT		7

// Mode flag values for the __WControl function
#define WC_CLEAR		0
#define WC_CLOSE		1
#define WC_HIDE			2
#define WC_SHOW			3
#define WC_SELECT		4
#define WC_SENDBEHIND	5

// Mode flag values for the __WAdjust function
#define WA_MOVE			0
#define WA_SIZE			1
#define WA_POSCURSOR	2

// Mode flag value for the __WPort function
#define WP_ONTOP		0
#define WP_OUTPUT		1

// Mode flag value for the _WZoom function
#define WZ_MAXIMIZED	0
#define WZ_NORMAL		1
#define WZ_MINIMIZED	2

// Border strings for typical window borders.
#define WO_DOUBLEBOX	"\x0cd\x0cd\x0ba\x0ba\x0c9\x0bb\x0c8\x0bc\x0cd\x0cd\x0ba\x0ba\x0c9\x0bb\x0c8\x0bc"
#define WO_SINGLEBOX	"\x0c4\x0c4\x0b3\x0b3\x0da\x0bf\x0c0\x0d9\x0c4\x0c4\x0b3\x0b3\x0da\x0bf\x0c0\x0d9"
#define WO_PANELBORDER	"\x0db\x0db\x0db\x0db\x0db\x0db\x0db\x0db\x0db\x0db\x0db\x0db\x0db\x0db\x0db\x0db"
#define WO_SYSTEMBORDER "\x020\x020\x020\x020\x0fe\x0f0\x020\x0f9\x020\x020\x020\x020\x020\x020\x020\x020"

#ifndef FOXWIND_INCLUDED

// Border string offsets.
#define selectBorder		0
#define deselectBorder		8

#define topEdge				0
#define bottomEdge			1
#define leftEdge			2
#define rightEdge			3
#define topLeftCorner		4
#define topRightCorner		5
#define bottomLeftCorner	6
#define bottomRightCorner	7
#define maxBorderLen	   17	/* Border string length (maximum)		   */

#endif      // FOXWIND_INCLUDED


#ifndef COLORS_INCLUDED

#define BLACK_ON 	0x00   /* Foreground color attributes		   */
#define BLUE_ON		0x01
#define GREEN_ON	0x02
#define CYAN_ON		0x03
#define	RED_ON		0x04
#define MAGENTA_ON	0x05
#define BROWN_ON	0x06
#define WHITE_ON	0x07

#define BLACK	(BLACK_ON << 4)	/* Background color attributes.		   */
#define BLUE	(BLUE_ON << 4)
#define GREEN	(GREEN_ON << 4)
#define CYAN	(CYAN_ON << 4)
#define RED		(RED_ON << 4)
#define MAGENTA (MAGENTA_ON << 4)
#define BROWN	(BROWN_ON << 4)
#define WHITE	(WHITE_ON << 4)

#define BRIGHT	0x08		/* Intensify foreground color		    */
#define BLINK	0x80		/* Blink this				    */


// The following values are used in the WA_ISSHADOW column of the
// schemes to indicate whether the window casts a shadow.

#define CASTSSHADOW (BRIGHT | BLACK_ON | BLACK)
#define CASTSNOSHADOW (BRIGHT | WHITE_ON | WHITE | BLINK)


// Color scheme numbers for use by _WOpen and others
enum
{
	USER_SCHEME,
	USERMENU_SCHEME,
	MBAR_SCHEME,
	POPUP_SCHEME,
	DIALOG_SCHEME,
	MODAL_POPUP_SCHEME,
	ALERT_SCHEME,
	WINDOW_SCHEME,
	NONMODAL_POPUP_SCHEME,
	BROWSE_SCHEME,
	REPORT_SCHEME,
	ALERT_POPUP_SCHEME
};

// Color index numbers used by _WSetAttr() and _WAttr()
enum
{
	WA_PENCOLOR = -1,	/* CURRENT PEN COLOR							*/

	WA_NORMAL,			/* normal text									*/
	WA_ENHANCED,		/* enhanced text								*/
	WA_BORDER,			/* window border								*/
	WA_FOREMOST,		/* title when foremost							*/
	WA_TITLE,			/* title otherwise								*/
	WA_SELECTED,		/* selected text								*/
	WA_HOTKEY,			/* control hotkeys								*/
	WA_SHADOW,			/* color of shadows that fall on this window.	*/
	WA_ENABLED,			/* enabled control								*/
	WA_DISABLED,		/* disabled control								*/
	WA_ISSHADOW			/* window casts a shad							*/
};


#endif      // COLORS_INCLUDED


//
// Prototypes for the API Functions
//

// Streaming output routines:
void FASTCALL _PutChr(int character);
void FASTCALL _PutStr(char FAR *string);
void FASTCALL _PutValue(Value FAR *val);

// Memory management functions:
MHandle FASTCALL _AllocHand(ULONG size);
void FASTCALL _FreeHand(MHandle h);
void FAR * FASTCALL _HandToPtr(MHandle h);
void FASTCALL _HLock(MHandle h);
void FASTCALL _HUnLock(MHandle h);
ULONG FASTCALL _GetHandSize(MHandle h);
USHORT FASTCALL _SetHandSize(MHandle h, ULONG size);
BOOL FASTCALL _MemAvail(ULONG bytes);

// String handling functions:
int	 FASTCALL _StrLen(char FAR *string);
void FASTCALL _StrCpy(char FAR *dest, char FAR *src);
int	 FASTCALL _StrCmp(char FAR *string1, char FAR *string2);

// Memory management functions:
#if _MSC_VER
#define _Alloca(size) _alloca(size)

#define _MemCmp(dest, src, length) memcmp(dest, src, length)
#define _MemMove(dest, src, length) memmove(dest, src, length)
#define _MemFill(ptr, c, length) memset(ptr, c, length)

#endif

// Functions to set the return value of a library functiion:
void FASTCALL _RetVal(Value FAR *val);
void FASTCALL _RetChar(char FAR *string);
void FASTCALL _RetInt(long ival, int width);
void FASTCALL _RetFloat(double flt, int width, int dec);
void FASTCALL _RetDateStr(char FAR *string);
void FASTCALL _RetDateTimeStr(char FAR *string);
void FASTCALL _RetLogical(int);
void FASTCALL _RetCurrency(CCY money,int width);

// Database Input/Output functions:
long FASTCALL _DBRecNo(int workarea);
long FASTCALL _DBRecCount(int workarea);
int	 FASTCALL _DBStatus(int workarea);
int  FASTCALL _DBRead(int workarea, long record);
int  FASTCALL _DBWrite(int workarea);
int	 FASTCALL _DBAppend(int workarea, int carryflag);
long FASTCALL _DBRewind(int workarea);
long FASTCALL _DBSkip(int workarea, long distance);
long FASTCALL _DBUnwind(int workarea);
int  FASTCALL _DBReplace(Locator FAR *fld, Value FAR *val);
long FASTCALL _DBSeek(Value FAR *val);
int  FASTCALL _DBLock(int workarea, int what);
void FASTCALL _DBUnLock(int workarea);
int  FASTCALL _DBAppendRecords(int workarea, unsigned short nrecs, char FAR *buffer);

// Memo Field Input/Output functions:
FCHAN FASTCALL  _MemoChan(int workarea);
long FASTCALL _AllocMemo(Locator FAR *fld, long size);
long FASTCALL _FindMemo(Locator FAR *fld);
long FASTCALL _MemoSize(Locator FAR *fld);

// Memory variable manipulation functions:
NTI FASTCALL _NewVar(char FAR *name, Locator FAR *loc, int flag);
int FASTCALL _Release(NTI nti);
int FASTCALL _Store(Locator FAR *loc, Value FAR *val);
int FASTCALL _Load(Locator FAR *loc, Value FAR *val);
long FASTCALL _ALen(NTI nti, int mode);
int FASTCALL _FindVar(NTI nti, int where, Locator FAR *loc);
NTI FASTCALL _NameTableIndex(char FAR *name);

// File Input/Output:

FCHAN FASTCALL __FOpen(char FAR *filename, int mode, int create);

#define _FOpen(filename, mode)			__FOpen(filename, mode, NO)
#define _FCreate(filename, mode)		__FOpen(filename, mode, YES)

int FASTCALL __FFlush(FCHAN chan, int close);

#define _FFlush(chan)	__FFlush(chan, NO)
#define _FClose(chan)	__FFlush(chan, YES)

int FASTCALL __FStat(FCHAN chan, int error);

#define _FEOF(chan)		__FStat(chan, NO)
#define _FError()		__FStat(0, YES)

unsigned int FASTCALL __FIO(FCHAN chan, char FAR *buffer, unsigned int maxlen, int mode);

#define _FGets(chan, buffer, maxlen)	__FIO(chan, buffer, maxlen, FIO_FGETS)
#define _FRead(chan, buffer, maxlen)	__FIO(chan, buffer, maxlen, FIO_FREAD)
#define _FPuts(chan, buffer)			__FIO(chan, buffer, 0,		FIO_FPUTS)
#define _FWrite(chan, buffer, maxlen)	__FIO(chan, buffer, maxlen, FIO_FWRITE)

long	FASTCALL _FSeek(FCHAN chan, long position, int mode);
int		FASTCALL _FCHSize(FCHAN chan, long length);
int		FASTCALL _FCopy(FCHAN dchan, long dpos, FCHAN schan, long spos, long len);

// User Interface routines:

#define PIXELMODE	0
#define CHARMODE	1

unsigned FASTCALL __ActivateHandler(FPFI handler, short charmode);

#define _ActivateHandler(handler)		__ActivateHandler(handler, CHARMODE)
#define _ActivateHandlerP(handler)		__ActivateHandler(handler, PIXELMODE)

void	FASTCALL _DeActivateHandler(unsigned);
unsigned FASTCALL __ActivateIdle(FPFI handler, short charmode);

#define  _ActivateIdle(handler)			__ActivateIdle(handler, CHARMODE)
#define  _ActivateIdleP(handler)		__ActivateIdle(handler, PIXELMODE)

void	FASTCALL _DeActivateIdle(unsigned);

int		FASTCALL __GetNextEvent(EventRec FAR *event, short charmode);

#define _GetNextEvent(event)			__GetNextEvent(event, CHARMODE)
#define _GetNextEventP(event)			__GetNextEvent(event, PIXELMODE)

void	FASTCALL __DefaultProcess(EventRec FAR *event, short charmode);

#define _DefaultProcess(event)			__DefaultProcess(event, CHARMODE)
#define	_DefaultProcessP(event)			__DefaultProcess(event, PIXELMODE)

#define _MousePos(pt)			__MousePos(pt, CHARMODE)
#define _MousePosP(pt)			__MousePos(pt, PIXELMODE)

int		FASTCALL __MousePos(Point FAR *pt, int charmode);

// Windowing routines:

int		FASTCALL __FindWindow(WHANDLE FAR *wh, Point pt, int charmode);
void	FASTCALL __GlobalToLocal(Point FAR *pt, WHANDLE wh, int charmode);
WHANDLE FASTCALL __WOpen(int top, int left, int bottom, int right, int flag, int
			   scheme_num, Scheme FAR *scheme, char FAR *bord, int pixelmode);

#define _WOpenP(top, left, bottom, right, flag, scheme_num, scheme, bord) __WOpen(top, left, bottom, right, flag, scheme_num, scheme, bord, PIXELMODE)
#define _WOpen(top, left, bottom, right, flag, scheme_num, scheme, bord) __WOpen(top, left, bottom, right, flag, scheme_num, scheme, bord, CHARMODE)

#define _GlobalToLocal(pt, wh)		__GlobalToLocal(pt, wh, CHARMODE)
#define _GlobalToLocalP(pt, wh)		__GlobalToLocal(pt, wh, PIXELMODE)

#define _FindWindow(wh, pt)			__FindWindow(wh, pt, CHARMODE)
#define _FindWindowP(wh, pt)		__FindWindow(wh, pt, PIXELMODE)


#define _WTop(wh)		((unsigned)__WStat(wh, WS_TOP, CHARMODE))
#define _WBottom(wh)	((unsigned)__WStat(wh, WS_BOTTOM, CHARMODE))
#define _WLeft(wh)		((unsigned)__WStat(wh, WS_LEFT, CHARMODE))
#define _WRight(wh)		((unsigned)__WStat(wh, WS_RIGHT, CHARMODE))
#define _WHeight(wh)	((unsigned)__WStat(wh, WS_HEIGHT, CHARMODE))
#define _WWidth(wh)		((unsigned)__WStat(wh, WS_WIDTH, CHARMODE))
#define _WSetPort(wh)	((WHANDLE)__WStat(wh, WS_SETPORT, CHARMODE))

#define _WTopP(wh)		((unsigned)__WStat(wh, WS_TOP, PIXELMODE))
#define _WBottomP(wh)	((unsigned)__WStat(wh, WS_BOTTOM, PIXELMODE))
#define _WLeftP(wh)		((unsigned)__WStat(wh, WS_LEFT, PIXELMODE))
#define _WRightP(wh)	((unsigned)__WStat(wh, WS_RIGHT, PIXELMODE))
#define _WHeightP(wh)	((unsigned)__WStat(wh, WS_HEIGHT, PIXELMODE))
#define _WWidthP(wh)	((unsigned)__WStat(wh, WS_WIDTH, PIXELMODE))

unsigned long FASTCALL __WStat(WHANDLE wh, int mode, int pixelmode);

#define _WMove(wh, pt)			__WAdjust(wh, pt, WA_MOVE, CHARMODE)
#define _WSize(wh, pt)			__WAdjust(wh, pt, WA_SIZE, CHARMODE)
#define _WPosCursor(wh, pt)		__WAdjust(wh, pt, WA_POSCURSOR, CHARMODE)
#define _WMoveP(wh, pt)			__WAdjust(wh, pt, WA_MOVE, PIXELMODE)
#define _WSizeP(wh, pt)			__WAdjust(wh, pt, WA_SIZE, PIXELMODE)
#define _WPosCursorP(wh, pt)	__WAdjust(wh, pt, WA_POSCURSOR, PIXELMODE)

void	FASTCALL __WAdjust(WHANDLE wh, Point pt, int mode, int charmode);
void    FASTCALL __WScroll(WHANDLE wh, Rect r, int dv, int dh, int charmode);

#define	_WScroll(wh, r, dv, dh)		__WScroll(wh, r, dv, dh, CHARMODE)
#define	_WScrollP(wh, r, dv, dh)	__WScroll(wh, r, dv, dh, PIXELMODE)
#define _WClearRect(wh, r)			__WScroll(wh, r, 0, 0, CHARMODE);
#define _WClearRectP(wh, r)			__WScroll(wh, r, 0, 0, PIXELMODE);

#define _WGetCursor(wh)			 __WGetCursor(wh, CHARMODE)
#define _WGetCursorP(wh)		 __WGetCursor(wh, PIXELMODE)

Point	FASTCALL __WGetCursor(WHANDLE wh, int);

void	FASTCALL __SetMenuPoint(MenuId menuid, FoxPoint loc, int charmode);

#define _SetMenuPoint(menuid, loc)		__SetMenuPoint(menuid, loc, CHARMODE)
#define _SetMenuPointP(menuid, loc)		__SetMenuPoint(menuid, loc, PIXELMODE)



int 	FASTCALL _InKey(int timeout, int flag);
void	FASTCALL _RefreshDisplay(void);
void 	FASTCALL _RefreshVideo(void);

#define _WClear(wh)		__WControl(wh, WC_CLEAR)
#define _WClose(wh)		__WControl(wh, WC_CLOSE)
#define _WHide(wh)		__WControl(wh, WC_HIDE)
#define _WShow(wh)		__WControl(wh, WC_SHOW)
#define _WSelect(wh)	__WControl(wh, WC_SELECT)
#define _WSendBehind(w) __WControl(w, WC_SENDBEHIND)

void FASTCALL __WControl(WHANDLE wh, int mode);

#define _WOnTop()		__WPort(WP_ONTOP)
#define _WGetPort()		__WPort(WP_OUTPUT)

WHandle FASTCALL __WPort(int mode);

void	FASTCALL _WZoom(WHANDLE wh, int newstate);

void	FASTCALL __WSetTitle(WHANDLE wh, char FAR *text, int mode);

#define WT_SETFOOTER	0
#define WT_SETTITLE		1
#define WT_GETFOOTER	2
#define WT_GETTITLE		3

#define _WSetFooter(wh, footer) __WSetTitle(wh, footer, WT_SETFOOTER)
#define _WSetTitle(wh, title)	__WSetTitle(wh, title, WT_SETTITLE)
#define _WFooter(wh, footer)	__WSetTitle(wh, footer, WT_GETFOOTER)
#define _WTitle(wh, title)		__WSetTitle(wh, title, WT_GETTITLE)

int		FASTCALL _WAttr(WHANDLE wh, int color);
void	FASTCALL _WSetAttr(WHANDLE wh, int color, int attr);
void	FASTCALL _WPutChr(WHANDLE wh, int character);
void	FASTCALL _WPutStr(WHANDLE wh, char FAR *str);

// Functions to execute FoxPro statements and evaluate FoxPro expressions:

int FASTCALL _Execute(char FAR *stmt);
int FASTCALL _Evaluate(Value FAR *val, char FAR *expr);

// Menuing functions:

ITEMID	FASTCALL __MenuStat(long x, int mode);

#define _MenuId(literal)				__MenuStat(literal, 0)
#define _GetNewItemId(menuid)			__MenuStat(menuid, 1)
#define _CountItems(menuid)				__MenuStat(menuid, 2)
#define _GetNewMenuId()					__MenuStat(0, 3)

int		FASTCALL _MenuInteract(MenuId FAR *menuid, ItemId FAR *itemid);

void	FASTCALL __MenuAction(MenuId menuid, int mode);

#define _ActivateMenu(menuid)			__MenuAction(menuid, 0)
#define _DeActivateMenu(menuid)			__MenuAction(menuid, 1)
#ifndef GLOBAL_INCLUDED
#define _DisposeMenu(menuid)			__MenuAction(menuid, 2)
#endif

int		FASTCALL _NewMenu(int mtype, MenuId menuid);
void	FASTCALL _SetMenuColor(MenuId menuid, int scheme);
ItemId	FASTCALL _GetItemId(MenuId menuid, long index);
int		FASTCALL _NewItem(MenuId menuid, ItemId itemid, ItemId beforeid, char FAR *prompt);
void	FASTCALL _DisposeItem(MenuId menuid, ItemId itemid);

void	FASTCALL __GetItemText(MenuId menuid, ItemId itemid, char FAR *text, int mode);

#define _GetItemText(menuid, itemid, text) __GetItemText(menuid, itemid, text, 0)
#define _GetOSPrompt(menuid, itemid, text) __GetItemText(menuid, itemid, text, 1)

void	FASTCALL _SetItemSubMenu(MenuId menuid, ItemId itemid, MenuId submenuid);
MenuId	FASTCALL _GetItemSubMenu(MenuId menuid, ItemId itemid);
void	FASTCALL _SetItemColor(MenuId menuid, ItemId itemid, int scheme);
void	FASTCALL _SetItemText(MenuId menuid, ItemId itemid, char FAR *text);
int		FASTCALL _GetItemCmdKey(MenuId menuid, ItemId itemid, char FAR *text);
void	FASTCALL _SetItemCmdKey(MenuId menuid, ItemId itemid, int key, char FAR *text);
void    FASTCALL _OnSelection(MenuId menuid, ItemId itemid, FPFI routine);

// FoxPro Dialogs:
int		FASTCALL _Dialog(int scheme, char FAR *text, char FAR *button1,
				char FAR *button2, char FAR *button3, int def, int esc);


// Error Handling:
void	FASTCALL _Error(int code);
void	FASTCALL _UserError(char FAR *message);
int		FASTCALL _ErrorInfo(int code, char FAR *message);


#define ED_SAVENOASK	0
#define	ED_SAVEASK		1
#define ED_SAVEAS		2

//
// Mode flags for the editor functions:
//

/* Mode flag values for the __EdPos function */
#define ED_SETPOS		0
#define ED_GETLINEPOS	1
#define ED_GETLINENUM	2
#define ED_GETPOS		3

/* Mode flag values for the __EdScroll function */
#define ED_SCROLLTOPOS	0
#define ED_SCROLLTOSEL	1

/* Mode flag values for the __EdManip function */
#define ED_SENDKEY		0
#define ED_DELETE		1
#define ED_INDENT		2
#define ED_COMMENT		3
#define ED_PROPERTIES	4
#define ED_PROCLIST		5

/* Mode flag values for the __EdClipBrd function */
#define ED_COPY			0
#define ED_CUT			1
#define ED_PASTE		2
#define ED_UNDO			3
#define ED_REDO			4

/* Mode flag values for the __EdEnv function */
#define ED_SETENV		0
#define ED_GETENV		1

/* Mode flag values for the __EdRevert function */
#define ED_REVERT		0
#define ED_SAVE			1

/* Mode flag values for the __WFindTitle function */
#define WFINDTITLE 	0
#define WMAINWINDOW	1
#define WFUNCTIONMASK	0x0f
#define WCLIENTFLAG		0x10


// Editor Functions:

WHandle FASTCALL _EdOpenFile(char FAR *filename, int mode);
int		FASTCALL _EdCloseFile(WHandle wh, int opt);
void	FASTCALL __EdRevert(WHandle wh, int mode);

#define _EdRevert(wh)				__EdRevert(wh, ED_REVERT)
#define _EdSave(wh)					__EdRevert(wh, ED_SAVE)

#define _EdSetPos(wh, pos)			(__EdPos(wh, (EDPOS) pos, ED_SETPOS))
#define _EdGetLinePos(wh, line)		((EDPOS)__EdPos(wh, (EDLINE) line, ED_GETLINEPOS))
#define _EdGetLineNum(wh, pos)		((EDLINE)__EdPos(wh, (EDPOS) pos, ED_GETLINENUM))
#define _EdGetPos(wh)				((EDPOS)__EdPos(wh, 0, ED_GETPOS))

EDPOS	FASTCALL __EdPos(WHandle wh, long pos, int mode);
int		FASTCALL _EdPosInView(WHandle wh, EDPOS pos);

#define _EdScrollToPos(wh, pos, flag)	(__EdScroll(wh, pos, flag, ED_SCROLLTOPOS))
#define _EdScrollToSel(wh, flag)		(__EdScroll(wh, 0, flag, ED_SCROLLTOSEL))

void	FASTCALL __EdScroll(WHandle wh, EDPOS pos, int flags, int mode);

#define _EdSendKey(wh, key)			__EdManip(wh, key, ED_SENDKEY)
#define	_EdDelete(wh)				__EdManip(wh, 0, ED_DELETE)
#define _EdIndent(wh, tabs)			__EdManip(wh, tabs, ED_INDENT)
#define _EdComment(wh, tabs)		__EdManip(wh, tabs, ED_COMMENT)
#define _EdProperties(wh)			__EdManip(wh, 0, ED_PROPERTIES)
#define _EdProcList(wh)				__EdManip(wh, 0, ED_PROCLIST)

void	FASTCALL __EdManip(WHandle wh, int n, int mode);

EDPOS	FASTCALL _EdSkipLines(WHandle wh, EDPOS pos, int n);
void 	FASTCALL _EdInsert(WHandle wh, char FAR *str, unsigned long len);
TEXT	FASTCALL _EdGetChar(WHandle wh, EDPOS pos);
void	FASTCALL _EdGetStr(WHandle wh, EDPOS pos1, EDPOS pos2, TEXT FAR *str);

#define _EdCopy(wh)					__EdClipBrd(wh, ED_COPY)
#define _EdCut(wh)					__EdClipBrd(wh, ED_CUT)
#define _EdPaste(wh)				__EdClipBrd(wh, ED_PASTE)
#define _EdUndo(wh)					__EdClipBrd(wh, ED_UNDO)
#define _EdRedo(wh)					__EdClipBrd(wh, ED_REDO)

void 	FASTCALL __EdClipBrd(WHandle wh, int mode);
void	FASTCALL _EdSelect(WHandle wh, EDPOS pos1, EDPOS pos2);
void 	FASTCALL _EdUndoOn(WHandle wh, int flag);
void	FASTCALL _EdActive(WHandle wh, int flag);
int		FASTCALL _EdLastError(WHandle wh);

#define _EdSetEnv(wh, env)			__EdEnv(wh, env, ED_SETENV)
#define _EdGetEnv(wh, env)			__EdEnv(wh, env, ED_GETENV)

#define _WFindTitle(title)			__WFindTitle(title, WFINDTITLE)
#define _WFindClientTitle(title)	__WFindTitle(title, WFINDTITLE | WCLIENTFLAG)
#define _WMainWindow()				__WFindTitle(0, WMAINWINDOW)
#define _WMainClientWindow()		__WFindTitle(0, WMAINWINDOW | WCLIENTFLAG)


int		FASTCALL __EdEnv(WHandle, EDENV FAR *, int);
HWND 	FASTCALL _WhToHwnd(WHandle);
WHandle FASTCALL __WFindTitle(char FAR *, int);

// Object model extensions
#define _WGetObjectClientWindow(objct)	__WGetObjectWindow(objct, WCLIENTFLAG)
#define _WGetObjectWindow(objct)		__WGetObjectWindow(objct, 0)

WHandle FASTCALL __WGetObjectWindow(Value FAR *, int);

int	FASTCALL _SetObjectProperty(Value FAR *objct, char FAR *prop, Value FAR *val, int fadd);
int	FASTCALL _GetObjectProperty(Value FAR *val, Value FAR *objct, char FAR *prop);

int FASTCALL _ObjectReference(Value FAR *objct);
int FASTCALL _ObjectRelease(Value FAR *objct);
int FASTCALL _ObjectCmp(Value FAR *objct1, Value FAR *objct2);
void FASTCALL _FreeObject(Value FAR *objct);

//_OCXAPI will allow developers of OCXs to use the LCK
//	Sample call: _OCXAPI(AfxGetInstanceHandle(),DLL_PROCESS_ATTACH);

BOOL WINAPI _OCXAPI(HINSTANCE hInstance,DWORD dwReason);

#ifdef GLOBAL_INCLUDED
#undef	WHANDLE
#endif


#ifdef _WIN64
// VFP Advanced (x64)
void * FASTCALL _MyNtCurrentTeb(void);
int FASTCALL _MyGetVersion(void);
#endif


#ifdef __cplusplus
}                       // End of extern "C" {
#endif

#if defined(_MSC_VER)
#pragma pack(pop)          // Restore structure packing
#endif

#endif                  // PRO_EXT_INCLUDED

