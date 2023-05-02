#DEFINE MOD_ALT 		0x0001
#DEFINE MOD_CONTROL		0x0002
#DEFINE MOD_SHIFT		0x0004
#DEFINE MOD_WIN			0x0008
#DEFINE MOD_NOREPEAT 	0x4000

#DEFINE VK_LBUTTON		0x01 && Left mouse button
#DEFINE VK_RBUTTON		0x02 && Right mouse button
#DEFINE VK_CANCEL		0x03 && Control-break processing
#DEFINE VK_MBUTTON		0x04 && Middle mouse button (three-button mouse)
#DEFINE VK_XBUTTON1		0x05 &&	X1 mouse button
#DEFINE VK_XBUTTON2		0x06 &&	X2 mouse button
#DEFINE VK_BACK			0x08 &&	BACKSPACE key
#DEFINE VK_TAB			0x09 &&	TAB key
#DEFINE VK_CLEAR		0x0C &&	CLEAR key
#DEFINE VK_RETURN		0x0D &&	ENTER key
#DEFINE VK_SHIFT		0x10 && SHIFT key
#DEFINE VK_CONTROL		0x11 &&	CTRL key
#DEFINE VK_MENU			0x12 &&	ALT key
#DEFINE VK_PAUSE		0x13 &&	PAUSE key
#DEFINE VK_CAPITAL		0x14 &&	CAPS LOCK key
#DEFINE VK_KANA			0x15 &&	IME Kana mode
#DEFINE VK_HANGUL		0x15 &&	IME Hangul mode
#DEFINE VK_IME_ON		0x16 &&	IME On
#DEFINE VK_JUNJA		0x17 &&	IME Junja mode
#DEFINE VK_FINAL		0x18 && IME final mode
#DEFINE VK_HANJA		0x19 &&	IME Hanja mode
#DEFINE VK_KANJI		0x19 &&	IME Kanji mode
#DEFINE VK_IME_OFF		0x1A &&	IME Off
#DEFINE VK_ESCAPE		0x1B &&	ESC key
#DEFINE VK_CONVERT		0x1C &&	IME convert
#DEFINE VK_NONCONVERT	0x1D &&	IME nonconvert
#DEFINE VK_ACCEPT		0x1E &&	IME accept
#DEFINE VK_MODECHANGE	0x1F &&	IME mode change request
#DEFINE VK_SPACE		0x20 &&	SPACEBAR
#DEFINE VK_PRIOR		0x21 &&	PAGE UP key
#DEFINE VK_NEXT			0x22 &&	PAGE DOWN key
#DEFINE VK_END			0x23 &&	END key
#DEFINE VK_HOME			0x24 &&	HOME key
#DEFINE VK_LEFT			0x25 &&	LEFT ARROW key
#DEFINE VK_UP			0x26 &&	UP ARROW key
#DEFINE VK_RIGHT		0x27 &&	RIGHT ARROW key
#DEFINE VK_DOWN			0x28 &&	DOWN ARROW key
#DEFINE VK_SELECT		0x29 &&	SELECT key
#DEFINE VK_PRINT		0x2A &&	PRINT key
#DEFINE VK_EXECUTE		0x2B &&	EXECUTE key
#DEFINE VK_SNAPSHOT		0x2C &&	PRINT SCREEN key
#DEFINE VK_INSERT		0x2D &&	INS key
#DEFINE VK_DELETE		0x2E &&	DEL key
#DEFINE VK_HELP			0x2F &&	HELP key
#DEFINE	VK_0			0x30 && 0 key
#DEFINE VK_1			0x31 &&	1 key
#DEFINE VK_2			0x32 && 2 key
#DEFINE VK_3			0x33 && 3 key
#DEFINE VK_4			0x34 &&	4 key
#DEFINE VK_5			0x35 &&	5 key
#DEFINE VK_6			0x36 &&	6 key
#DEFINE VK_7			0x37 &&	7 key
#DEFINE VK_8			0x38 && 8 key
#DEFINE VK_9			0x39 &&	9 key
#DEFINE VK_A			0x41 &&	A key
#DEFINE VK_B			0x42 &&	B key
#DEFINE VK_C			0x43 &&	C key
#DEFINE VK_D			0x44 &&	D key
#DEFINE VK_E			0x45 &&	E key
#DEFINE VK_F			0x46 &&	F key
#DEFINE VK_G			0x47 &&	G key
#DEFINE VK_H			0x48 &&	H key
#DEFINE VK_I			0x49 &&	I key
#DEFINE VK_J			0x4A && J key
#DEFINE VK_K			0x4B &&	K key
#DEFINE VK_L			0x4C &&	L key
#DEFINE VK_M			0x4D &&	M key
#DEFINE VK_N			0x4E &&	N key
#DEFINE VK_O			0x4F &&	O key
#DEFINE VK_P			0x50 &&	P key
#DEFINE VK_Q			0x51 &&	Q key
#DEFINE VK_R			0x52 &&	R key
#DEFINE VK_S 			0x53 &&	S key
#DEFINE VK_T			0x54 &&	T key
#DEFINE VK_U			0x55 &&	U key
#DEFINE VK_V			0x56 &&	V key
#DEFINE VK_W			0x57 &&	W key
#DEFINE VK_X			0x58 &&	X key
#DEFINE VK_Y			0x59 &&	Y key
#DEFINE VK_Z			0x5A &&	Z key
#DEFINE VK_LWIN			0x5B && Left Windows key (Natural keyboard)
#DEFINE VK_RWIN			0x5C &&	Right Windows key (Natural keyboard)
#DEFINE VK_APPS			0x5D &&	Applications key (Natural keyboard)
#DEFINE VK_SLEEP		0x5F &&	Computer Sleep key
#DEFINE VK_NUMPAD0		0x60 &&	Numeric keypad 0 key
#DEFINE VK_NUMPAD1		0x61 &&	Numeric keypad 1 key
#DEFINE VK_NUMPAD2		0x62 &&	Numeric keypad 2 key
#DEFINE VK_NUMPAD3		0x63 &&	Numeric keypad 3 key
#DEFINE VK_NUMPAD4		0x64 &&	Numeric keypad 4 key
#DEFINE VK_NUMPAD5		0x65 &&	Numeric keypad 5 key
#DEFINE VK_NUMPAD6		0x66 &&	Numeric keypad 6 key
#DEFINE VK_NUMPAD7		0x67 &&	Numeric keypad 7 key
#DEFINE VK_NUMPAD8		0x68 &&	Numeric keypad 8 key
#DEFINE VK_NUMPAD9		0x69 &&	Numeric keypad 9 key
#DEFINE VK_MULTIPLY		0x6A &&	Multiply key
#DEFINE VK_ADD			0x6B &&	Add key
#DEFINE VK_SEPARATOR	0x6C &&	Separator key
#DEFINE VK_SUBTRACT		0x6D &&	Subtract key
#DEFINE VK_DECIMAL		0x6E &&	Decimal key
#DEFINE VK_DIVIDE		0x6F &&	Divide key
#DEFINE VK_F1			0x70 &&	F1 key
#DEFINE VK_F2			0x71 &&	F2 key
#DEFINE VK_F3			0x72 &&	F3 key
#DEFINE VK_F4			0x73 &&	F4 key
#DEFINE VK_F5			0x74 &&	F5 key
#DEFINE VK_F6			0x75 &&	F6 key
#DEFINE VK_F7			0x76 &&	F7 key
#DEFINE VK_F8			0x77 &&	F8 key
#DEFINE VK_F9			0x78 &&	F9 key
#DEFINE VK_F10			0x79 &&	F10 key
#DEFINE VK_F11			0x7A &&	F11 key
#DEFINE VK_F12			0x7B &&	F12 key
#DEFINE VK_F13			0x7C &&	F13 key
#DEFINE VK_F14			0x7D &&	F14 key
#DEFINE VK_F15			0x7E &&	F15 key
#DEFINE VK_F16			0x7F && F16 key
#DEFINE VK_F17			0x80 &&	F17 key
#DEFINE VK_F18			0x81 &&	F18 key
#DEFINE VK_F19			0x82 &&	F19 key
#DEFINE VK_F20			0x83 &&	F20 key
#DEFINE VK_F21			0x84 &&	F21 key
#DEFINE VK_F22			0x85 &&	F22 key
#DEFINE VK_F23			0x86 &&	F23 key
#DEFINE VK_F24			0x87 &&	F24 key
#DEFINE VK_NUMLOCK		0x90 &&	NUM LOCK key
#DEFINE VK_SCROLL		0x91 && SCROLL LOCK key
#DEFINE VK_LSHIFT		0xA0 && Left SHIFT key
#DEFINE VK_RSHIFT		0xA1 &&	Right SHIFT key
#DEFINE VK_LCONTROL		0xA2 && Left CONTROL key
#DEFINE VK_RCONTROL		0xA3 &&	Right CONTROL key
#DEFINE VK_LMENU		0xA4 &&	Left ALT key
#DEFINE VK_RMENU		0xA5 &&	Right ALT key
#DEFINE VK_BROWSER_BACK			0xA6 &&	Browser Back key
#DEFINE VK_BROWSER_FORWARD		0xA7 &&	Browser Forward key
#DEFINE VK_BROWSER_REFRESH		0xA8 &&	Browser Refresh key
#DEFINE VK_BROWSER_STOP			0xA9 &&	Browser Stop key
#DEFINE VK_BROWSER_SEARCH		0xAA &&	Browser Search key
#DEFINE VK_BROWSER_FAVORITES	0xAB &&	Browser Favorites key
#DEFINE VK_BROWSER_HOME			0xAC &&	Browser Start and Home key
#DEFINE VK_VOLUME_MUTE			0xAD &&	Volume Mute key
#DEFINE VK_VOLUME_DOWN			0xAE &&	Volume Down key
#DEFINE VK_VOLUME_UP			0xAF &&	Volume Up key
#DEFINE VK_MEDIA_NEXT_TRACK		0xB0 &&	Next Track key
#DEFINE VK_MEDIA_PREV_TRACK		0xB1 &&	Previous Track key
#DEFINE VK_MEDIA_STOP			0xB2 &&	Stop Media key
#DEFINE VK_MEDIA_PLAY_PAUSE		0xB3 &&	Play/Pause Media key
#DEFINE VK_LAUNCH_MAIL			0xB4 &&	Start Mail key
#DEFINE VK_LAUNCH_MEDIA_SELECT	0xB5 &&	Select Media key
#DEFINE VK_LAUNCH_APP1			0xB6 &&	Start Application 1 key
#DEFINE VK_LAUNCH_APP2			0xB7 &&	Start Application 2 key
#DEFINE VK_OEM_1				0xBA &&	Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the ';:' key
#DEFINE VK_OEM_PLUS				0xBB &&	For any country/region, the '+' key
#DEFINE VK_OEM_COMMA			0xBC &&	For any country/region, the ',' key
#DEFINE VK_OEM_MINUS			0xBD &&	For any country/region, the '-' key
#DEFINE VK_OEM_PERIOD			0xBE &&	For any country/region, the '.' key
#DEFINE VK_OEM_2				0xBF &&	Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the '/?' key
#DEFINE VK_OEM_3				0xC0 &&	Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the '`~' key
#DEFINE VK_OEM_4				0xDB &&	Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the '[{' key
#DEFINE VK_OEM_5				0xDC &&	Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the '\|' key
#DEFINE VK_OEM_6				0xDD &&	Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the ']}' key
#DEFINE VK_OEM_7				0xDE &&	Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the 'single-quote/double-quote' key
#DEFINE VK_OEM_8				0xDF &&	Used for miscellaneous characters; it can vary by keyboard.
#DEFINE VK_OEM_102				0xE2 &&	The <> keys on the US standard keyboard, or the \\| key on the non-US 102-key keyboard
#DEFINE VK_PROCESSKEY			0xE5 &&	IME PROCESS key
#DEFINE VK_PACKET				0xE7 &&	Used to pass Unicode characters as if they were keystrokes. The VK_PACKET key is the low word of a 32-bit Virtual Key value used for non-keyboard input methods. For more information, see Remark in KEYBDINPUT, SendInput, WM_KEYDOWN, and WM_KEYUP
#DEFINE VK_ATTN					0xF6 &&	Attn key
#DEFINE VK_CRSEL				0xF7 &&	CrSel key
#DEFINE VK_EXSEL				0xF8 && ExSel key
#DEFINE VK_EREOF				0xF9 &&	Erase EOF key
#DEFINE VK_PLAY					0xFA &&	Play key
#DEFINE VK_ZOOM					0xFB &&	Zoom key
#DEFINE VK_PA1					0xFD &&	PA1 key
#DEFINE VK_OEM_CLEAR			0xFE &&	Clear key