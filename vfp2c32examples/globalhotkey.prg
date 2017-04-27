#DEFINE WM_HOTKEY 0x0312
#DEFINE GWL_WNDPROC -4
#DEFINE FORMAT_MESSAGE_FROM_SYSTEM 0x1000

DEFINE CLASS cGlobalHotKeyManager AS Custom

	PROTECTED aKeys[1,3]
	DIMENSION aKeys[1,3]
	nLastError = 0
	
	FUNCTION Init
		DECLARE INTEGER RegisterHotKey IN WIN32API INTEGER hWnd, INTEGER id, INTEGER fsModifiers, INTEGER vk
		DECLARE INTEGER UnregisterHotKey IN WIN32API INTEGER hWnd, INTEGER id
		DECLARE INTEGER GlobalAddAtom IN WIN32API STRING lpString
		DECLARE SHORT GlobalDeleteAtom IN WIN32API SHORT nAtom
		DECLARE INTEGER GetLastError IN WIN32API
		BINDEVENTSEX(_SCREEN.HWnd, WM_HOTKEY, THIS, 'HotKeyEvent', 'wParam, HIWORD(lParam), LOWORD(lParam)')
	ENDFUNC
	
	FUNCTION Destroy
		THIS.UnregAllHotKeys()
		UNBINDEVENTSEX(_SCREEN.HWnd, WM_HOTKEY)
	ENDFUNC
	
	FUNCTION UnRegAllHotKeys
		LOCAL xj
		FOR m.xj = 1 TO ALEN(THIS.aKeys, 1)
			IF VARTYPE(THIS.aKeys[m.xj,1]) = 'N'
				UnregisterHotKey(_SCREEN.HWnd, THIS.aKeys[m.xj,1])
				GlobalDeleteAtom(THIS.aKeys[m.xj,1])
			ENDIF
		ENDFOR
		DIMENSION THIS.aKeys[1,3]
		THIS.aKeys[1,1] = .F.
		THIS.aKeys[1,2] = .F.
		THIS.aKeys[1,3] = .F.
	ENDFUNC

	FUNCTION FindHotKey
		LPARAMETERS nAtom, nKey
		LOCAL lnRow, xj, fsModifiers
		IF VARTYPE(THIS.aKeys[1,1]) != 'N'
			RETURN 0
		ENDIF
		IF VARTYPE(m.nKey) = 'N'
			m.lnRow = 0
			m.fsModifiers = m.nAtom
			FOR m.xj = 1 TO ALEN(THIS.aKeys, 1)
				IF THIS.aKeys[m.xj,2] = m.fsModifiers AND THIS.aKeys[m.xj,3] = m.nKey
					m.lnRow = m.xj
					EXIT
				ENDIF	
			ENDFOR
		ELSE
			m.lnRow = ASCAN(THIS.aKeys, m.nAtom, 1, -1, 1, 8)
		ENDIF
		RETURN m.lnRow
	ENDFUNC
	
	FUNCTION AHotKeys
		LPARAMETERS laKeys
		EXTERNAL ARRAY laKeys
		DIMENSION m.laKeys[1,3]
		ACOPY(THIS.aKeys, m.laKeys)
		RETURN IIF(VARTYPE(m.laKeys[1,1]) = 'N', ALEN(m.laKeys, 1), 0)
	ENDFUNC
	
	FUNCTION RegHotKey
		LPARAMETERS fsModifiers, nKey, nAtom
		LOCAL lcAtomString, lnRow
		&& create a new hotkey
		IF VARTYPE(m.nAtom) != 'N'
			m.lcAtomString = SYS(2015) + SYS(2015) + SYS(2015) + SYS(2015) && should be unique enough
			m.nAtom = GlobalAddAtom(m.lcAtomString)
			&& VFP can't handle the "unsigned short" datatype, only "signed SHORT" is supported
			&& to solve this DECLARE the returntype as INTEGER and just strip off the upper 16bits with BITAND ..
			m.nAtom = BITAND(m.nAtom,0xFFFF)
			IF m.nAtom != 0
				IF RegisterHotKey(_SCREEN.HWnd, m.nAtom, m.fsModifiers, m.nKey) > 0
					THIS.AddHotKey(m.nAtom, m.fsModifiers, m.nKey)
					RETURN m.nAtom
				ELSE
					THIS.nLastError = GetLastError()
					GlobalDeleteAtom(m.nAtom)
				ENDIF
			ELSE
				THIS.nLastError = GetLastError()
			ENDIF
			RETURN 0
		&& else replace an existing hotkey
		ELSE
			m.lnRow = THIS.FindHotKey(m.nAtom)
			IF m.lnRow > 0
				IF RegisterHotKey(_SCREEN.HWnd, m.nAtom, m.fsModifiers, m.nKey) > 0
					THIS.ReplaceHotKey(m.lnRow, m.fsModifiers, m.nKey)
					RETURN m.nAtom
				ELSE
					THIS.nLastError = GetLastError()
				ENDIF
			ENDIF
			RETURN 0		
		ENDIF
	ENDFUNC
	
	&& to unregister the hotkey pass the nAtom value returned from RegHotKey, or the modifier key combination
	FUNCTION UnRegHotKey
		LPARAMETERS nAtom, nKey
		LOCAL lnRow, lnIndex
		m.lnRow = THIS.FindHotKey(m.nAtom, m.nKey)
		IF m.lnRow > 0
			IF UnregisterHotKey(_SCREEN.HWnd, THIS.aKeys[m.lnRow,1]) = 0
				THIS.nLastError = GetLastError()
				RETURN .F.
			ENDIF
			GlobalDeleteAtom(THIS.aKeys[m.lnRow,1])
			THIS.RemoveHotKey(m.lnRow)
		ENDIF
		RETURN m.lnRow > 0
	ENDFUNC

	FUNCTION HotKeyEvent
		LPARAMETERS nHotKeyID, nKeyCode, nShiftAltCtrl	
		WAIT WINDOW TRANSFORM(nHotKeyID) + ':' + TRANSFORM(nKeyCode) + '-' + TRANSFORM(nShiftAltCtrl) NOWAIT NOCLEAR
	ENDFUNC

	FUNCTION GetLastErrorMessage
		RETURN FORMATMESSAGEEX(THIS.nLastError)
	ENDFUNC

	&& internal
	PROTECTED FUNCTION AddHotKey
		LPARAMETERS nAtom, fsModifiers, nKey
		LOCAL lnRow
		IF VARTYPE(THIS.aKeys[1,1]) = 'N'
			m.lnRow = ALEN(THIS.aKeys, 1) + 1
			DIMENSION THIS.aKeys[m.lnRow, 3]
		ELSE
			m.lnRow = 1
		ENDIF
		THIS.aKeys[m.lnRow,1] = m.nAtom
		THIS.aKeys[m.lnRow,2] = m.fsModifiers
		THIS.aKeys[m.lnRow,3] = m.nKey
	ENDFUNC

	&& internal
	PROTECTED FUNCTION RemoveHotKey
		LPARAMETERS nRow
		LOCAL lnLen
		m.lnLen = ALEN(THIS.aKeys, 1)
		IF m.lnLen > 1
			ADEL(THIS.aKeys, m.nRow)
			DIMENSION THIS.aKeys[m.lnLen-1,3]
		ELSE
			THIS.aKeys[1,1] = .F.
			THIS.aKeys[1,2] = .F.
			THIS.aKeys[1,3] = .F.
		ENDIF
	ENDFUNC
	
	&& internal
	PROTECTED FUNCTION ReplaceHotKey
		LPARAMETERS nRow, fsModifiers, nKey
		THIS.aKeys[m.nRow,2] = m.fsModifiers
		THIS.aKeys[m.nRow,3] = m.nKey
	ENDFUNC

ENDDEFINE