#define SZ_THDOCPROP_DISPLAYNAME	STRCONV("DisplayName"+CHR(0),5)
#define SZ_THDOCPROP_CANONICALNAME	STRCONV("ThemeName"+CHR(0),5)
#define SZ_THDOCPROP_TOOLTIP		STRCONV("ToolTip"+CHR(0),5)
#define SZ_THDOCPROP_AUTHOR			STRCONV("author"+CHR(0),5)

#define GWL_WNDPROC        -4
#define WM_THEMECHANGED  0x031A

DEFINE CLASS cWindowsTheme AS Custom
	
	hTheme = 0
	ThemeActive = .F.
	ThemeFile = ""
	ThemeColor = ""
	ThemeSize = ""

	ThemeDisplayName = ""
	ThemeName = ""
	ThemeTooltip = ""
	ThemeAuthor = ""
	
	PROTECTED nOrigWindowProc
	nOrigWindowProc = 0
			
	FUNCTION Init
		DECLARE INTEGER IsThemeActive IN uxtheme.dll

		DECLARE INTEGER GetCurrentThemeName IN uxtheme.dll ;
			STRING @ pszThemeFileName, ;
			INTEGER dwMaxNameChars, ;
			STRING @ pszColorBuff, ;
			INTEGER cchMaxColorChars, ;
			STRING @ pszSizeBuff, ;
			INTEGER cchMaxSizeChars

		DECLARE INTEGER GetThemeDocumentationProperty IN uxtheme.dll ;
			STRING pszThemeName, ;
			STRING pszPropertyName, ;
			STRING @ pszValueBuff, ;
			INTEGER cchMaxValChars

		DECLARE INTEGER GetWindowLong IN WIN32API INTEGER hWnd, INTEGER nIndex
		DECLARE INTEGER SetWindowLong IN WIN32API INTEGER hWnd, INTEGER nIndex, INTEGER nValue
		
		DECLARE INTEGER CallWindowProc IN WIN32API ;
			INTEGER lpPrevWndFunc, ;
			INTEGER hWnd, ;
			INTEGER uMsg, ;
			INTEGER wParam, ;
			INTEGER lParam   
			
        THIS.nOrigWindowProc = GetWindowLong(_SCREEN.hWnd,GWL_WNDPROC)
        BINDEVENT(_SCREEN.Hwnd,WM_THEMECHANGED,THIS,'OnThemeChange')
		
		THIS.ReadThemeData()
	ENDFUNC
	
	FUNCTION Destroy
		UNBINDEVENTS(_SCREEN.HWnd,WM_THEMECHANGED,THIS,'OnThemeChange')
	ENDFUNC
	
	FUNCTION OnThemeChange(hWnd, uMsg, wParam, lParam)
		THIS.ReadThemeData()
		RETURN CallWindowProc(THIS.nOrigWindowProc,hWnd,uMsg,wParam,lParam)
	ENDFUNC

	FUNCTION ReadThemeData
	
		THIS.ThemeActive = (IsThemeActive() = 1)
		
		IF THIS.ThemeActive	
		
			LOCAL lcFile, lcColor, lcSize, lcProperty, lnRetVal
			STORE SPACE(512) TO lcFile, lcColor, lcSize, lcProperty
			lnRetVal = GetCurrentThemeName(@lcFile,256,@lcColor,256,@lcSize,256)
			
			THIS.ThemeFile = THIS.ConvUCStr(lcFile)
			THIS.ThemeColor = THIS.ConvUCStr(lcColor)
			THIS.ThemeSize = THIS.ConvUCStr(lcSize)
			
			lnRetVal = GetThemeDocumentationProperty(lcFile,SZ_THDOCPROP_DISPLAYNAME,@lcProperty,256)
			THIS.ThemeDisplayName = THIS.ConvUCStr(lcProperty)
			
			lnRetVal = GetThemeDocumentationProperty(lcFile,SZ_THDOCPROP_CANONICALNAME,@lcProperty,256)
			THIS.ThemeName = THIS.ConvUCStr(lcProperty)
			
			lnRetVal = GetThemeDocumentationProperty(lcFile,SZ_THDOCPROP_TOOLTIP,@lcProperty,256)
			THIS.ThemeTooltip = THIS.ConvUCStr(lcProperty)
			
			lnRetVal = GetThemeDocumentationProperty(lcFile,SZ_THDOCPROP_AUTHOR,@lcProperty,256)
			THIS.ThemeAuthor = THIS.ConvUCStr(lcProperty)		
			
		ELSE
		
			THIS.ThemeFile = ""
			THIS.ThemeColor = ""
			THIS.ThemeSize = ""
			THIS.ThemeDisplayName = ""
			THIS.ThemeName = ""
			THIS.ThemeTooltip = ""
			THIS.ThemeAuthor = ""
		
		ENDIF
		
	ENDFUNC
	
	PROTECTED FUNCTION ConvUCStr(lcUnicodeCString)
		RETURN STRCONV(LEFT(lcUnicodeCString,AT(CHR(0)+CHR(0),lcUnicodeCString)),6)	
	ENDFUNC

ENDDEFINE