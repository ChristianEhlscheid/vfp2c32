#INCLUDE vfp2c.h

CD (FULLPATH(JUSTPATH(SYS(16))))
SET LIBRARY TO vfp2c32.fll ADDITIVE
INITVFP2C32(VFP2C_INIT_ALL)

MESSAGEBOXEX('Hello VFP', 0, 'The Caption', _VFP.hWnd, 102, VFP2CSYS(1))

LOCAL lcFolder
IF SHBROWSEFOLDER('Choose a Folder',0,@lcFolder)
	? "Folder", lcFolder
ELSE
	? "Dialog aborted"
ENDIF

?SHBROWSEFOLDER('Choose a Folder',0,@lcFolder,'C:\Windows')

?SHBROWSEFOLDER('Choose yet another Folder :)',0,@lcFolder,'','BrowseFolderCallback')

&& like standard GETFILE, but without places bar
LOCAL lcFile
lcFile = GETOPENFILENAME(0,"All Files" + CHR(0) + "*.*","","C:","Custom Title",OFN_EX_NOPLACESBAR)
DO CASE
	CASE VARTYPE(lcFile) = 'C'
		? "File" + lcFile + " selected"
	CASE lcFile = 0
		? "Dialog box aborted"
	CASE lcFile = -1
		? "Error in dialog box."
ENDCASE

&& multiselect GETFILE, by passing an arrayname in the 7th parameter
&& always returns a numeric value !!
lnFiles = GETOPENFILENAME(0,"All Files" + CHR(0) + "*.*","","C:","Multiselect Example",0,"laFiles")
DO CASE
	CASE lnFiles = -1
		AERROREX('laError')
		DISPLAY MEMORY LIKE laError
	CASE lnFiles = 0
		? "Dialog box aborted"
	OTHERWISE
		&& Path is in laFiles[1], all other elements contain a filename (without the path)
		DISPLAY MEMORY LIKE laFiles	
ENDCASE

&& use of a callback function to respond to events inside the dialog 
lnFiles = GETOPENFILENAME(0,"Special Files" + CHR(0) + "au*.bat","","C:","",0,"","OpenFileCallback")
DO CASE
	CASE VARTYPE(lnFiles) = 'C'
		? "File" + lnFiles + " selected"
	CASE lnFiles = 0
		? "Dialog box aborted"
	CASE lnFiles = -1
		AERROREX('laError')
		DISPLAY MEMORY LIKE laError
ENDCASE




#DEFINE CDN_FIRST			-601
#DEFINE CDN_LAST			-699
#DEFINE CDN_INITDONE		-601
#DEFINE CDN_SELCHANGE		-602
#DEFINE CDN_FOLDERCHANGE	-603
#DEFINE CDN_SHAREVIOLATION	-604
#DEFINE CDN_HELP			-605
#DEFINE CDN_FILEOK			-606
#DEFINE CDN_TYPECHANGE		-607
#DEFINE CDN_INCLUDEITEM		-608

#DEFINE CDM_FIRST			0x464
#DEFINE CDM_LAST			0x4C8
#DEFINE CDM_GETSPEC			0x464
#DEFINE CDM_GETFILEPATH		0x465
#DEFINE CDM_GETFOLDERPATH	0x466
#DEFINE CDM_GETFOLDERIDLIST	0x467
#DEFINE CDM_SETCONTROLTEXT	0x468
#DEFINE CDM_HIDECONTROL		0x469
#DEFINE CDM_SETDEFEXT		0x470

#DEFINE IDOK				1
#DEFINE IDCANCEL			2


FUNCTION BrowseFolderCallback(lnHwnd, uMsg, lParam)
	? lnHwnd, uMsg, lParam
ENDFUNC

FUNCTION OpenFileCallback(lnHwnd, lnControlID, lnCode)
	IF lnCode = CDN_INITDONE
		&& in this event the appearance of the dialog can be customized
		&& center the window in it's parent window
		CenterWindowEx(lnHwnd)
		&& change button captions
		DECLARE INTEGER SendMessage IN user32.dll AS SendMessageString INTEGER, INTEGER, INTEGER, STRING
		SendMessageString(lnHwnd,CDM_SETCONTROLTEXT,IDOK,"Choose")
		SendMessageString(lnHwnd,CDM_SETCONTROLTEXT,IDCANCEL,"Cancel")
		&& for more examples google for "GetOpenFileName" and you'll find a bunch of articles/examples.
		&& Most of them for Visual Basic, but they can be easily translated to VFP.
	ENDIF
	RETURN 0 && the function must return an integer or logical value
ENDFUNC

