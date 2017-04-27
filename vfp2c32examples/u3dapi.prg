*!*	this file contains an objectified model over the U3 C-API (u3dapi10.dll from www.u3.com)
*!* the C structure's @ the bottom are converted with the VFP2C struct translator (see comments)
#INCLUDE u3dapi.h

SET PROCEDURE TO u3dapi.prg ADDITIVE

PUBLIC aDapiErrors[38,2]
aDapiErrors[1,1] = S_FALSE
aDapiErrors[1,2] = "Command succeeded however the result is negative."
aDapiErrors[2,1] = E_POINTER
aDapiErrors[2,2] = "Null pointer parameter."
aDapiErrors[3,1] = E_FAIL
aDapiErrors[3,2] = "The command was unable to be executed."
aDapiErrors[4,1] = E_HANDLE
aDapiErrors[4,2] = "Invalid handle parameter."
aDapiErrors[5,1] = E_INVALIDARG
aDapiErrors[5,2] = "One of the function arguments is invalid."
aDapiErrors[6,1] = DAPI_E_STATE_ERR
aDapiErrors[6,2] = "The current command cannot be executed as the current state of the device will not support the command. Try again."
aDapiErrors[7,1] = DAPI_E_CONFIGURATION_ERR
aDapiErrors[7,2] = "The configuration options supplied are either not valid or not supported."
aDapiErrors[8,1] = DAPI_E_PATH_ERR
aDapiErrors[8,2] = "The path does not match domain on the device."
aDapiErrors[9,1] = DAPI_E_GENERAL_ERR
aDapiErrors[9,2] = "A recoverable error has occurred."
aDapiErrors[10,1] = DAPI_E_INTERNAL_ERR
aDapiErrors[10,2] = "A recoverable error has occurred."
aDapiErrors[11,1] = DAPI_E_STRING_CONVERSION_ERR
aDapiErrors[11,2] = "An unsupported non Unicode strings was provided."
aDapiErrors[12,1] = DAPI_E_DOMAIN_TYPE_ERR
aDapiErrors[12,2] = "The domain referenced by the path argument is not a removable domain."
aDapiErrors[13,1] = DAPI_E_DOMAIN_PATH_ERR
aDapiErrors[13,2] = "The supplied path does not point to a valid domain for the operation."
aDapiErrors[14,1] = DAPI_E_COMM_FAIL
aDapiErrors[14,2] = "General communications error."
aDapiErrors[15,1] = DAPI_E_MEDIA_LOCK_FAIL
aDapiErrors[15,2] = "The device could not be locked."
aDapiErrors[16,1] = DAPI_E_FORMAT_FAIL
aDapiErrors[16,2] = "DAPI was unable to format the private area and reconfiguring the area." + CHR(13) + ;
					"The configuration succeeded, however the private area must be manually formatted."
aDapiErrors[17,1] = DAPI_E_HINT_FAIL
aDapiErrors[17,2] = "Error reading the hint data. Remaining data is still valid."
aDapiErrors[18,1] = DAPI_E_FS_REFRESH_FAIL
aDapiErrors[18,2] = "DAPI was unable to refresh the file system information"
aDapiErrors[19,1] = DAPI_E_NOT_CONNECTED
aDapiErrors[19,2] = "The device is not connected."
aDapiErrors[20,1] = DAPI_E_NOT_READY
aDapiErrors[20,2] = "The device is either not initialized, or has not yet completed a previous operation."
aDapiErrors[21,1] = DAPI_E_NOT_WRITABLE
aDapiErrors[21,2] = "The target area or device is read only."
aDapiErrors[22,1] = DAPI_E_PA_CONFIGURED
aDapiErrors[22,2] = "The private area was already configured."
aDapiErrors[23,1] = DAPI_E_CD_LOCKED
aDapiErrors[23,2] = "The operation could not be performed on the CD-ROM domain." + CHR(13) + ;
					"Credentials must be supplied to unlock the CDROM first."
aDapiErrors[24,1] = DAPI_E_HIDDEN_AREA_FULL
aDapiErrors[24,2] = "No more room available to write the cookie type to the hidden area."	
aDapiErrors[25,1] = DAPI_E_DATA_NOT_FOUND
aDapiErrors[25,2] = "The cookie could not be found."
aDapiErrors[26,1] = DAPI_E_OPERATION_NOT_COMPLETED
aDapiErrors[26,2] = "The operation was not completed. Try again."
aDapiErrors[27,1] = DAPI_E_READ_PWD_ERR
aDapiErrors[27,2] = "Bad read password."
aDapiErrors[28,1] = DAPI_E_WRITE_PWD_ERR
aDapiErrors[28,2] = "Bad write password."
aDapiErrors[29,1] = DAPI_E_COOKIE_EXISTS
aDapiErrors[29,2] = "The cookie already exists."
aDapiErrors[30,1] = DAPI_E_COOKIE_CONFIG_ERR
aDapiErrors[30,2] = "The combination of null read and write passwords is not supported."
aDapiErrors[31,1] = DAPI_E_PASSWORD_TOO_LONG
aDapiErrors[31,2] = "Password is too long."
aDapiErrors[32,1] = DAPI_E_BUFFER_TOO_LARGE
aDapiErrors[32,2] = "The data to be written is larger then the allocated space."
aDapiErrors[33,1] = DAPI_E_BUFFER_TOO_SMALL
aDapiErrors[33,2] = "The data to be copied is larger then the destination buffer size."
aDapiErrors[34,1] = DAPI_E_CMD_NOT_SUPPORTED
aDapiErrors[34,2] = "The device is not U3; device does not support capability; the function is no longer supported by DAPI."
aDapiErrors[35,1] = DAPI_E_CMD_NOT_SUPPORTED_IN_NON_ADMIN_MODE
aDapiErrors[35,2] = "The command cannot be executed in user mode. Run this command with administrator privileges."
aDapiErrors[36,1] = DAPI_E_OS_NOT_SUPPORTED
aDapiErrors[36,2] = "The operating system is not supported by DAPI."
aDapiErrors[37,1] = DAPI_E_WIN_2K_SP4_HOTFIX_MISSING
aDapiErrors[37,2] = "The latest Windows 2000 Service Pack 4 must be applied to this machine."
aDapiErrors[38,1] = DAPI_E_ADV_ERR
aDapiErrors[38,2] = "Advanced communications error."

&& wrapper class around the environment strings registered by the U3 launchpad application
DEFINE CLASS dapiEnvironment AS Custom

	DEVICE_SERIAL = .F.
	DEVICE_PATH = .F.
	DEVICE_DOCUMENT_PATH = .F.
	DEVICE_VENDOR = .F.
	DEVICE_PRODUCT = .F.
	DEVICE_VENDOR_ID = .F.
	APP_DATA_PATH = .F.
	HOST_EXEC_PATH = .F.
	DEVICE_EXEC_PATH = .F.
	ENV_VERSION = .F.
	ENV_LANGUAGE = .F.
	IS_UPGRADE = .F.
	IS_DEVICE_AVAILABLE = .F.
	IS_AUTORUN = .F.
	DAPI_CONNECT_STRING = .F.
	
	PROCEDURE DEVICE_SERIAL_Access()
		RETURN GETENV('U3_DEVICE_SERIAL')
	ENDPROC
	
	PROCEDURE DEVICE_PATH_Access()
		RETURN GETENV('U3_DEVICE_PATH')
	ENDPROC
	
	PROCEDURE DEVICE_DOCUMENT_PATH_Access()
		RETURN GETENV('U3_DEVICE_DOCUMENT_PATH')
	ENDPROC
	
	PROCEDURE DEVICE_VENDOR_Access()
		RETURN GETENV('U3_DEVICE_VENDOR')
	ENDPROC
	
	PROCEDURE DEVICE_PRODUCT_Access()
		RETURN GETENV('U3_DEVICE_PRODUCT')
	ENDPROC
	
	PROCEDURE DEVICE_VENDOR_ID_Access()
		RETURN GETENV('U3_DEVICE_VENDOR_ID')
	ENDPROC
	
	PROCEDURE APP_DATA_PATH_Access()
		RETURN GETENV('U3_APP_DATA_PATH')
	ENDPROC
	
	PROCEDURE HOST_EXEC_PATH_Access()
		RETURN GETENV('U3_HOST_EXEC_PATH')
	ENDPROC	
	
	PROCEDURE DEVICE_EXEC_PATH_Access()
		RETURN GETENV('U3_DEVICE_EXEC_PATH')
	ENDPROC
	
	PROCEDURE ENV_VERSION_Access()
		RETURN GETENV('U3_ENV_VERSION')
	ENDPROC
	
	PROCEDURE ENV_LANGUAGE_Access()
		RETURN VAL(GETENV('U3_ENV_LANGUAGE'))
	ENDPROC
	
	PROCEDURE IS_UPGRADE_Access()
		RETURN GETENV('U3_IS_UPGRADE') == "true"
	ENDPROC
	
	PROCEDURE IS_DEVICE_AVAILABLE_Access()
		RETURN GETENV('U3_IS_DEVICE_AVAILABLE') == "true"
	ENDPROC
	
	PROCEDURE IS_AUTORUN_Access()
		RETURN GETENV('U3_IS_AUTORUN') == "true"
	ENDPROC
		
	PROCEDURE DAPI_CONNECT_STRING_Access()
		RETURN GETENV('U3_DAPI_CONNECT_STRING')
	ENDPROC
	
ENDDEFINE


&& The top level object
DEFINE CLASS dapiSession AS Custom

	PROTECTED hSession
	hSession = ILLEGAL_HSESSION
	PROTECTED hCallback
	hCallback = ILLEGAL_HCALLBACK
	PROTECTED nCallbackFunc
	nCallbackFunc = 0
	
	PROCEDURE Init
		LOCAL laInst[1], lnCount
		m.lnCount = AINSTANCE(laInst,THIS.Class)
		IF m.lnCount = 0
			THIS.DeclareAPI() && first instance declares the API
		ENDIF
		THIS.AddObject('oDevices','Collection')
		THIS.CreateSession()
	ENDPROC
	
	PROCEDURE Destroy
		LOCAL laInst[1], lnCount
		THIS.UnregisterCallback()
		THIS.DestroySession()
		m.lnCount = AINSTANCE(laInst,THIS.Class)
		IF m.lnCount = 0
			THIS.ClearAPI() && last instance released? cleanup the API
		ENDIF
	ENDPROC
	
	PROTECTED PROCEDURE DeclareAPI()
		&& const wchar_t * const dapiGetVersion()
		DECLARE INTEGER dapiGetVersion IN u3dapi10.dll
		&& HRESULT dapiCreateSession       (HSESSION * hSession)
		DECLARE INTEGER dapiCreateSession IN u3dapi10.dll INTEGER @ hSession
		&& HRESULT dapiDestroySession      (HSESSION hSession)		
		DECLARE INTEGER dapiDestroySession IN u3dapi10.dll INTEGER hSession
*!*		HRESULT dapiRegisterCallback (HSESSION        hSession            ,
*!*		                                wchar_t*        pszConnectionString ,
*!*										DAPI_CALLBACK   pCallBack           ,
*!*										void*           pEx                 ,
*!*										HCALLBACK*      hCallback)
		DECLARE INTEGER dapiRegisterCallback IN u3dapi10.dll ;
			INTEGER hSession, ;
			STRING pszConnectionString, ;
			INTEGER pCallback, ;
			INTEGER pEx, ;
			INTEGER @ hCallback
		&& HRESULT	dapiUnregisterCallback		(HCALLBACK hCallback)
		DECLARE INTEGER dapiUnregisterCallback IN u3dapi10.dll INTEGER hCallback
		&& HRESULT	dapiQueryDeviceInformation	(HDEVICE hDev, devInfo* pInfo)
		DECLARE INTEGER dapiQueryDeviceInformation IN u3dapi10.dll INTEGER hDev, INTEGER pInfo
		&& HRESULT	dapiQueryDeviceCapability	(HDEVICE hDev, DWORD nCapability)
		DECLARE INTEGER dapiQueryDeviceCapability IN u3dapi10.dll INTEGER hDev, INTEGER nCapability
		&& HRESULT	dapiQueryDomainInformation	(HDEVICE hDev, domainInfo* pInfo, WORD* nCount)
		DECLARE INTEGER dapiQueryDomainInformation IN u3dapi10.dll INTEGER hDev, INTEGER pInfo, SHORT @ nCount
		&& HRESULT	dapiEjectDevice				(HDEVICE hDev)
		DECLARE INTEGER dapiEjectDevice IN u3dapi10.dll INTEGER hDev
		&& HRESULT dapiGetPrivateAreaInfo		(HDEVICE hDev, const wchar_t* szPath, praInfo* pInfo)
		DECLARE INTEGER dapiGetPrivateAreaInfo IN u3dapi10.dll INTEGER hDev, STRING szPath, INTEGER pInfo
		&& HRESULT	dapiLoginPrivateArea		(HDEVICE hDev, const wchar_t* szPath, const wchar_t* szPassword, BOOL bLock)
		DECLARE INTEGER dapiLoginPrivateArea IN u3dapi10.dll INTEGER hDev, STRING szPath, STRING szPassword, INTEGER bLock
		&& HRESULT	dapiLogoutPrivateArea		(HDEVICE hDev, const wchar_t* szPath, BOOL bLock)
		DECLARE INTEGER dapiLogoutPrivateArea IN u3dapi10.dll INTEGER hDev, STRING szPath, INTEGER bLock
*!*		HRESULT	dapiSetPrivateAreaPassword	(		HDEVICE		hDev			, 
*!*											const	wchar_t*	szPath			, 
*!*											const	wchar_t*	szOldPassword	, 
*!*											const	wchar_t*	szNewPassword	, 
*!*											const	wchar_t*	szNewHint		,
*!*													BOOL		bLock)
		DECLARE INTEGER dapiSetPrivateAreaPassword IN u3dapi10.dll ;
		INTEGER hDev, ;
		STRING szPath, ;
		STRING szOldPassword, ;
		STRING szNewPassword, ;
		STRING szNewHint, ;
		INTEGER bLock
*!*		HRESULT	dapiWriteTextCookie			(		HDEVICE		hDev		, 
*!*											const	wchar_t*	szSection	, 
*!*											const	wchar_t*	szEntry		,
*!*											const	wchar_t*	szValue)
		DECLARE INTEGER dapiWriteTextCookie IN u3dapi10.dll ;
		INTEGER hDev, ;
		STRING szSection, ;
		STRING szEntry, ;
		STRING szValue
*!*		HRESULT	dapiWriteBinaryCookie		(		HDEVICE		hDev		, 
*!*											const	wchar_t*	szSection	,
*!*											const	wchar_t*	szEntry		,
*!*											const	BYTE*		pBuffer		,  
*!*													DWORD		nBufLength)
		DECLARE INTEGER dapiWriteBinaryCookie IN u3dapi10.dll ;
		INTEGER hDev, ;
		STRING szSection, ;
		STRING szEntry, ;
		STRING szBuffer, ;
		INTEGER nBufLength
*!*		HRESULT	dapiReadTextCookie			(		HDEVICE		hDev		, 
*!*											const	wchar_t*	szSection	, 
*!*											const	wchar_t*	szEntry		,
*!*													wchar_t*	szValue		, 
*!*													DWORD*		nBufLength)
		DECLARE INTEGER dapiReadTextCookie IN u3dapi10.dll ;
			INTEGER hDev, ;
			STRING szSection, ;
			STRING szEntry, ;
			STRING @ szValue, ;
			INTEGER @ nBufLength
*!*		HRESULT	dapiReadBinaryCookie		(		HDEVICE		hDev		, 
*!*											const	wchar_t*	szSection	, 
*!*											const	wchar_t*	szEntry		,
*!*													BYTE*		pBuffer		, 
*!*													DWORD*		nBufLength)
		DECLARE INTEGER dapiReadBinaryCookie IN u3dapi10.dll ;
			INTEGER hDev, ;
			STRING szSection, ;
			STRING szEntry, ;
			STRING @ szValue, ;
			INTEGER @ nBufLength
*!*			HRESULT	dapiDeleteCookie			(		HDEVICE		hDev	 , 
*!*											const	wchar_t*	szSection, 
*!*											const	wchar_t*	szEntry)
		DECLARE INTEGER dapiDeleteCookie IN u3dapi10.dll ;
			INTEGER hDev, ;
			STRING szSection, ;
			STRING szEntry
	ENDPROC
	
	PROTECTED PROCEDURE ClearAPI()
		CLEAR DLLS 'dapiGetVersion', 'dapiCreateSession', 'dapiDestroySession', 'dapiRegisterCallback', 'dapiUnregisterCallback', ;
				'dapiQueryDeviceInformation', 'dapiQueryDeviceCapability', 'dapiQueryDomainInformation', 'dapiEjectDevice', ;
				'dapiGetPrivateAreaInfo', 'dapiLoginPrivateArea', 'dapiLogoutPrivateArea', 'dapiSetPrivateAreaPassword', ;
				'dapiWriteTextCookie', 'dapiWriteBinaryCookie', 'dapiReadTextCookie', 'dapiReadBinaryCookie', 'dapiDeleteCookie'
	ENDPROC
	
	PROCEDURE GetVersion()
		LOCAL lnVersion
		m.lnVersion = dapiGetVersion()
		RETURN ReadWString(m.lnVersion)
	ENDPROC
	
	PROTECTED PROCEDURE CreateSession()
		LOCAL lnHr, lnSession
		m.lnSession = 0
		m.lnHr = dapiCreateSession(@m.lnSession)
		IF m.lnHr = S_OK
			THIS.hSession = m.lnSession
		ELSE
			THIS.ApiError(lnHr)
		ENDIF
	ENDPROC
	
	PROTECTED PROCEDURE DestroySession()
		LOCAL lnHr
		IF THIS.hSession != ILLEGAL_HSESSION
			m.lnHr = dapiDestroySession(THIS.hSession)
			IF m.lnHr!= S_OK
				THIS.ApiError(m.lnHr)
			ENDIF
		ENDIF
	ENDPROC

	PROCEDURE RegisterCallback(lcConnectionString)
		ASSERT THIS.nCallbackProc = 0 AND THIS.hCallback = ILLEGAL_HCALLBACK MESSAGE 'Callback already registered!'
		
		LOCAL lnHr, lnCallback
		m.lnCallback = 0
	
		SET STEP ON
		&& typedef void (_stdcall *DAPI_CALLBACK)(HDEVICE hDev, DWORD eventType, void* pEx);
		THIS.nCallbackFunc = CreateCallbackFunc('ApiCallback','VOID','ULONG,ULONG,ULONG',THIS,CALLBACK_ASYNCRONOUS_POST)

		IF PCOUNT() = 1
			m.lcConnectionString = STRCONV(m.lcConnectionString+CHR(0),5)
		ELSE
			m.lcConnectionString = .NULL.
		ENDIF

		m.lnHr = dapiRegisterCallback(THIS.hSession,m.lcConnectionString,THIS.nCallbackFunc,0,@m.lnCallback)
		IF m.lnHr = S_OK
			THIS.hCallback = m.lnCallback
		ELSE
			THIS.ApiError(lnHr)
		ENDIF		
	ENDPROC
	
	PROCEDURE UnregisterCallback()
		IF THIS.hCallback != ILLEGAL_HCALLBACK
			dapiUnregisterCallback(THIS.hCallback)
			THIS.hCallback = ILLEGAL_HCALLBACK
		ENDIF
		IF THIS.nCallbackFunc != 0
			DestroyCallbackFunc(THIS.nCallbackFunc)
			THIS.nCallbackFunc = 0
		ENDIF
	ENDPROC
	
	PROCEDURE ApiCallback(hDev, eventType, pEx)
		DO CASE
			CASE eventType = DAPI_EVENT_DEVICE_CONNECT
				THIS.OnConnect(hDev)
			CASE eventType = DAPI_EVENT_DEVICE_DISCONNECT
				THIS.OnDisconnect(hDev)
		ENDCASE
	ENDPROC

	PROCEDURE ApiError(lnHr)
		LOCAL lcError, lnRow
		m.lnRow = ASCAN(aDapiErrors,lnHr,1,0,1,8)
		IF m.lnRow > 0
			m.lcError = m.aDapiErrors[m.lnRow,2]
		ELSE
			m.lcError = 'Unknown DAPI error.'
		ENDIF		
		MESSAGEBOX('DAPI Error:' + CHR(13) + m.lcError,16,'Error')
	ENDPROC
	
	PROCEDURE AddDevice(hDev)
		LOCAL m.loDevice
		m.loDevice = CREATEOBJECT('dapiDevice',m.hDev,THIS.hSession)
		THIS.oDevices.Add(m.loDevice,ALLTRIM(STR(hDev)))
	ENDPROC
	
	PROCEDURE RemoveDevice(hDev)
		THIS.oDevices.Remove(ALLTRIM(STR(m.hDev)))
	ENDPROC
		
	PROCEDURE OnConnect(hDev)
		THIS.AddDevice(m.hDev)
	ENDPROC
	
	PROCEDURE OnDisconnect(hDev)
		THIS.RemoveDevice(m.hDev)
	ENDPROC
	
ENDDEFINE

DEFINE CLASS dapiDevice AS Custom

	PROTECTED hSession
	hSession = ILLEGAL_HSESSION
	PROTECTED hDevice
	hDevice = ILLEGAL_HDEVICE
	PROTECTED hCallback
	hCallback = ILLEGAL_HCALLBACK
	PROTECTED nCallbackFunc
	nCallbackFunc = 0
	SerialNumber = ""
	UniqueID = ""
	VendorString = ""
	ProductVersion = ""
	VendorID = 0
	DeviceSize = 0
	
	PROCEDURE Init(hDeviceHandle, hSession)
		THIS.hDevice = m.hDeviceHandle
		THIS.hSession = m.hSession	
	ENDPROC
	
	FUNCTION Destroy
		THIS.UnregisterCallback()
	ENDFUNC

	PROCEDURE RegisterCallback()
		LOCAL lnHr, lcSerialNo, lnCallback
		
		IF EMPTY(THIS.SerialNumber)
			THIS.QueryInfo()	
		ENDIF
		
		THIS.nCallbackFunc = CreateCallbackFunc('ApiCallback','VOID','ULONG,ULONG,ULONG',THIS,CALLBACK_ASYNCRONOUS_POST)

		m.lcSerialNo = STRCONV(THIS.SerialNumber+CHR(0),5)

		m.lnHr = dapiRegisterCallback(THIS.hSession,m.lcSerialNo,THIS.nCallbackFunc,0,@m.lnCallback)
		IF m.lnHr = S_OK
			THIS.hCallback = m.lnCallback
		ELSE
			THIS.ApiError(lnHr)
		ENDIF	
	ENDPROC
	
	PROCEDURE UnregisterCallback()
		IF THIS.hCallback != ILLEGAL_HCALLBACK
			dapiUnregisterCallback(THIS.hCallback)
			THIS.hCallback = ILLEGAL_HCALLBACK
		ENDIF
		IF THIS.nCallbackFunc != 0
			DestroyCallbackFunc(THIS.nCallbackFunc)
			THIS.nCallbackFunc = 0
		ENDIF				
	ENDPROC

	PROCEDURE WriteTextCookie(lcSection, lcEntry, lcValue)
		LOCAL lnHr
		m.lcSection = STRCONV(m.lcSection+CHR(0),5)
		m.lcEntry = STRCONV(m.lcEntry+CHR(0),5)
		m.lcValue = STRCONV(m.lcValue+CHR(0),5)
		m.lnHr = dapiWriteTextCookie(THIS.hDevice,m.lcSection,m.lcEntry,m.lcValue)
		IF m.lnHr = S_OK
			RETURN .T.
		ELSE
			THIS.ApiError(m.lnHr)
			RETURN .F.
		ENDIF		
	ENDPROC
	
	PROCEDURE WriteBinaryCookie(lcSection, lcEntry, lcValue)
		LOCAL lnHr
		m.lcSection = STRCONV(m.lcSection+CHR(0),5)
		m.lcEntry = STRCONV(m.lcEntry+CHR(0),5)
		m.lnHr = dapiWriteBinaryCookie(THIS.hDevice,m.lcSection,m.lcEntry,m.lcValue,LEN(m.lcValue))
		IF m.lnHr = S_OK
			RETURN .T.
		ELSE
			THIS.ApiError(m.lnHr)
			RETURN .F.
		ENDIF
	ENDPROC
	
	PROCEDURE ReadTextCookie(lcSection, lcEntry)
		LOCAL lnHr, lnBuffLen, lcBuffer
		m.lcSection = STRCONV(m.lcSection+CHR(0),5)
		m.lcEntry = STRCONV(m.lcEntry+CHR(0),5)
		m.lnBuffLen = 0
		m.lnHr = dapiReadTextCookie(THIS.hDevice,m.lcSection,m.lcEntry,.NULL.,@m.lnBuffLen)
		IF m.lnHr = S_OK OR m.lnHr = DAPI_E_BUFFER_TOO_SMALL
			m.lcBuffer = SPACE(m.lnBuffLen)
			m.lnHr = dapiReadTextCookie(THIS.hDevice,m.lcSection,m.lcEntry,@lcBuffer,@m.lnBuffLen)					
			IF m.lnHr = S_OK
				RETURN STRCONV(LEFT(m.lcBuffer,m.lnBuffLen-2),6)
			ENDIF
		ENDIF		
		THIS.ApiError(m.lnHr)
	ENDPROC
	
	PROCEDURE ReadBinaryCookie(lcSection, lcEntry)
		LOCAL lnHr, lnBuffLen, lcBuffer
		m.lcSection = STRCONV(m.lcSection+CHR(0),5)
		m.lcEntry = STRCONV(m.lcEntry+CHR(0),5)
		m.lnBuffLen = 0
		m.lnHr = dapiReadBinaryCookie(THIS.hDevice,m.lcSection,m.lcEntry,.NULL.,@m.lnBuffLen)
		IF m.lnHr = S_OK OR m.lnHr = DAPI_E_BUFFER_TOO_SMALL
			m.lcBuffer = SPACE(m.lnBuffLen)
			m.lnHr = dapiReadBinaryCookie(THIS.hDevice,m.lcSection,m.lcEntry,@m.lcBuffer,@m.lnBuffLen)					
			IF m.lnHr = S_OK
				RETURN m.lcBuffer
			ENDIF
		ENDIF		
		THIS.ApiError(m.lnHr)
	ENDPROC
	
	PROCEDURE DeleteCookie(lcSection, lcEntry)
		LOCAL lnHr
		m.lcSection = STRCONV(m.lcSection+CHR(0),5)
		m.lcEntry = STRCONV(m.lcEntry+CHR(0),5)
		m.lnHr = dapiDeleteCookie(THIS.hDevice,m.lcSection,m.lcEntry)
		IF m.lnHr = S_OK
			RETURN .T.
		ELSE
			THIS.ApiError(m.lnHr)
			RETURN .F.
		ENDIF
	ENDPROC
	
	PROCEDURE QueryCapability(lnCapability)
		LOCAL lnHr
		m.lnHr = dapiQueryDeviceCapability(THIS.hDevice,m.lnCapability)
		DO CASE
			CASE m.lnHr = S_OK
				RETURN .T.
			CASE m.lnHr = S_FALSE
				RETURN .F.
			OTHERWISE
				THIS.ApiError(m.lnHr)
		ENDCASE
	ENDPROC
	
	PROCEDURE QueryInfo()
		LOCAL loDevInfo, lnHr
		m.loDevInfo = CREATEOBJECT('devInfo')
		m.lnHr = dapiQueryDeviceInformation(THIS.hDevice,m.loDevInfo.Address)
		IF m.lnHr = S_OK
			THIS.SerialNumber = m.loDevInfo.SerialNumber
			THIS.UniqueID = m.loDevInfo.UniqueID
			THIS.VendorString = m.loDevInfo.VendorString
			THIS.ProductVersion = m.loDevInfo.ProductVersion
			THIS.VendorID = m.loDevInfo.VendorID
			THIS.DeviceSize = m.loDevInfo.DeviceSize
		ELSE
			THIS.ApiError(m.lnHr)
		ENDIF
	ENDPROC
	
	PROCEDURE QueryDomains(laDomains)
		EXTERNAL ARRAY laDomains
		LOCAL loDomInfo, lnHr, lnCount, xj
		m.lnCount = 0
		m.lnHr = dapiQueryDomainInformation(THIS.hDevice,0,@m.lnCount)
		IF m.lnHr = S_OK
			m.loDomInfo = CREATEOBJECT('domainInfo',m.lnCount) 		
			DIMENSION m.laDomains[m.lnCount,3]
			m.lnHr = dapiQueryDomainInformation(THIS.hDevice,m.loDomInfo.Address,@m.lnCount)
			IF m.lnHr = S_OK
				FOR m.xj = 1 TO m.lnCount
					m.loDomInfo.AIndex(m.lnxj) && position index in array of structures
					m.laDomains[m.xj,1] = m.loDomInfo.szPath
					m.laDomains[m.xj,2] = m.loDomInfo.size
					m.laDomains[m.xj,3] = m.loDomInfo.typeMask
				ENDFOR
				RETURN m.lnCount
			ENDIF
		ENDIF
		THIS.ApiError(lnHr)
	ENDPROC
	
	PROCEDURE Eject()
		LOCAL lnHr
		m.lnHr = dapiEjectDevice(THIS.hDevice)
		DO CASE
			CASE m.lnHr = S_OK
				RETURN .T.
			CASE m.lnHr = S_FALSE
				RETURN .F.
			OTHERWISE
				THIS.ApiError(lnHr)
		ENDCASE
	ENDPROC
	
	PROCEDURE GetPrivateAreaInfo(lcPath)
		LOCAL lnHr, loArea
		m.loArea = CREATEOBJECT('praInfo')
		m.lcPath = STRCONV(m.lcPath+CHR(0),5)
		m.lnHr = dapiGetPrivateAreaInfo(THIS.hDevice,m.lcPath,m.loArea.Address)
		RETURN m.loArea
	ENDPROC
	
	PROCEDURE LoginPrivateArea(lcPath, lcPassword, lbLock)
		LOCAL lnHr
		m.lcPath = STRCONV(m.lcPath+CHR(0),5)
		m.lcPassword = STRCONV(m.lcPassword+CHR(0),5)
		m.lbLock = IIF(PCOUNT()=3,m.lbLock,.T.)
		m.lnHr = dapiLoginPrivateArea(THIS.hDevice,m.lcPath,m.lcPassword,IIF(m.lbLock,1,0))
		RETURN m.lnHr = S_OK
	ENDPROC
	
	PROCEDURE LogoutPrivateArea(lcPath, lbLock)
		LOCAL lnHr
		m.lcPath = STRCONV(m.lcPath+CHR(0),5)
		m.lbLock = IIF(PCOUNT()=2,m.lbLock,.T.)
		m.lnHr = dapiLogoutPrivateArea(THIS.hDevice,m.lcPath,IIF(m.lbLock,1,0))
		IF m.lnHr = S_OK
			RETURN .T.
		ELSE
			THIS.ApiError(m.lnHr)
		ENDIF
	ENDPROC
	
	PROCEDURE SetPrivateAreaPassword(lcPath, lcOldPassword, lcNewPassword, lcHint)
		LOCAL lnHr
		m.lcPath = STRCONV(m.lcPath+CHR(0),5)
		m.lcOldPassword = STRCONV(m.lcOldPassword+CHR(0),5)
		m.lcNewPassword = STRCONV(m.lcNewPassword+CHR(0),5)
		m.lcHint = STRCONV(m.lcHint+CHR(0),5)
		m.lnHr = dapiSetPrivateAreaPassword(THIS.hDevice,m.lcPath,m.lcOldPassword,m.lcNewPassword,m.lcHint,IIF(m.lbLock,1,0))
		IF m.lnHr = S_OK
			RETURN .T.
		ELSE
			THIS.ApiError(m.lnHr)
			RETURN .F.
		ENDIF
	ENDPROC

	PROCEDURE ApiCallback(hDev, eventType, pEx)
		DO CASE
			CASE eventType = DAPI_EVENT_DEVICE_CONNECT
				THIS.OnConnect()
			CASE eventType = DAPI_EVENT_DEVICE_DISCONNECT
				THIS.OnDisconnect()
			CASE eventType = DAPI_EVENT_DEVICE_UPDATE
				THIS.OnUpdate()
			CASE eventType = DAPI_EVENT_DEVICE_LOGIN
				THIS.OnLogin()
			CASE eventType = DAPI_EVENT_DEVICE_LOGOUT
				THIS.OnLogout()
			CASE eventType = DAPI_EVENT_DEVICE_WRITE_PROTECT_ON
				THIS.OnWriteProtectOn()
			CASE eventType = DAPI_EVENT_DEVICE_WRITE_PROTECT_OFF
				THIS.OnWriteProtectOff()
			CASE eventType = DAPI_EVENT_DEVICE_RECONNECT
				THIS.OnReconnect()
			CASE eventType = DAPI_EVENT_DEVICE_NEW_CONFIG
				THIS.OnReConfig()
		ENDCASE
	ENDPROC

	PROCEDURE OnDisconnect(hDev)

	ENDPROC
	
	PROCEDURE OnUpdate(hDev)
	
	ENDPROC
	
	PROCEDURE OnLogin(hDev)
	
	ENDPROC
	
	PROCEDURE OnLogout(hDev)
	
	ENDPROC
	
	PROCEDURE OnWriteProtectOn(hDev)
	
	ENDPROC
	
	PROCEDURE OnWriteProtectOff(hDev)
	
	ENDPROC
	
	PROCEDURE OnReconnect(hDev)
	
	ENDPROC
	
	PROCEDURE OnReConfig(hDev)
	
	ENDPROC	
	

ENDDEFINE

*!*	typedef struct TdevInfo
*!*	{
*!*		wchar_t	serialNumber	[DAPI_SERIAL_NUMBER_LEN]	;
*!*		BYTE	uniqueID		[DAPI_UNIQUE_ID_LEN]		;
*!*		wchar_t	vendorString	[DAPI_VENDOR_STRING_LEN]	;
*!*		wchar_t	productString	[DAPI_PRODUCT_STRING_LEN]	;
*!*		wchar_t	FWVersion		[DAPI_FW_VERSION_SIZE]		;
*!*		DWORD	vendorID			;
*!*		DWORD64	deviceSize			;
*!*	}devInfo ;

DEFINE CLASS devInfo AS Relation

	Address = 0
	SizeOf = 616
	Name = "devInfo"
	&& structure fields
	serialNumber = .F.
	uniqueID = .F.
	vendorString = .F.
	productString = .F.
	FWVersion = .F.
	vendorID = .F.
	deviceSize = .F.

	PROCEDURE Init()
		THIS.Address = AllocMem(THIS.SizeOf)
		IF THIS.Address = 0
			ERROR(43)
			RETURN .F.
		ENDIF
	ENDPROC

	PROCEDURE Destroy()
		FreeMem(THIS.Address)
	ENDPROC

	PROCEDURE serialNumber_Access()
		RETURN ReadWCharArray(THIS.Address,257)
	ENDPROC

	PROCEDURE serialNumber_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'C' MESSAGE 'Wrong datatype or value out of range!'
		WriteWCharArray(THIS.Address,lnNewVal,257)
	ENDPROC

	PROCEDURE uniqueID_Access()
		RETURN ReadBytes(THIS.Address+514,20)
	ENDPROC

	PROCEDURE uniqueID_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'C' MESSAGE 'Wrong datatype or value out of range!'
		WriteBytes(THIS.Address+514,lnNewVal,20)
	ENDPROC

	PROCEDURE vendorString_Access()
		RETURN ReadWCharArray(THIS.Address+534,10)
	ENDPROC

	PROCEDURE vendorString_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'C' MESSAGE 'Wrong datatype or value out of range!'
		WriteWCharArray(THIS.Address+534,lnNewVal,10)
	ENDPROC

	PROCEDURE productString_Access()
		RETURN ReadWCharArray(THIS.Address+554,18)
	ENDPROC

	PROCEDURE productString_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'C' MESSAGE 'Wrong datatype or value out of range!'
		WriteWCharArray(THIS.Address+554,lnNewVal,18)
	ENDPROC

	PROCEDURE FWVersion_Access()
		RETURN ReadWCharArray(THIS.Address+590,7)
	ENDPROC

	PROCEDURE FWVersion_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'C' MESSAGE 'Wrong datatype or value out of range!'
		WriteWCharArray(THIS.Address+590,lnNewVal,7)
	ENDPROC

	PROCEDURE vendorID_Access()
		RETURN ReadUInt(THIS.Address+604)
	ENDPROC

	PROCEDURE vendorID_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+604,lnNewVal)
	ENDPROC

	PROCEDURE deviceSize_Access()
		RETURN ReadUInt64AsDouble(THIS.Address+608)
	ENDPROC

	PROCEDURE deviceSize_Assign(lnNewVal)
		WriteUInt64(THIS.Address+608,lnNewVal)
	ENDPROC

ENDDEFINE

*!*	typedef struct TdomainInfo
*!*	{
*!*		wchar_t	szPath[DAPI_MAX_PATH+1]	;
*!*		DWORD64	size					;
*!*		DWORD	typeMask				;
*!*	}domainInfo ;

DEFINE CLASS domainInfo AS Relation

	Address = 0
	SizeOf = 544
	Offset = 0
	BufferSize = 0
	Name = "domainInfo"
	&& structure fields
	szPath = .F.
	size = .F.
	typeMask = .F.

	PROCEDURE Init(lnCount)
		THIS.BufferSize = lnCount * THIS.SizeOf
	ENDPROC

	PROCEDURE Destroy()
		FreeMem(THIS.Address)
	ENDPROC

	PROCEDURE AIndex(lnRow)
		THIS.Offset = THIS.Address+THIS.SizeOf*(lnRow-1)
	ENDPROC

	PROCEDURE BufferSize_Assign(lnBufferSize)
		LOCAL lnAddress
		lnAddress = ReAllocMem(THIS.Address,lnBufferSize)
		IF lnAddress != 0
			THIS.Address = lnAddress
			THIS.BufferSize = lnBufferSize
		ELSE
			ERROR(43)
		ENDIF
	ENDPROC

	PROCEDURE szPath_Access()
		RETURN ReadWCharArray(THIS.Offset,261)
	ENDPROC

	PROCEDURE size_Access()
		RETURN ReadUInt64AsDouble(THIS.Offset+528)
	ENDPROC

	PROCEDURE typeMask_Access()
		RETURN ReadUInt(THIS.Offset+536)
	ENDPROC

ENDDEFINE

*!*	typedef struct TpraInfo
*!*	{
*!*		DWORD	maxNOA	;
*!*		DWORD	currNOA	;
*!*		DWORD64	size	;
*!*		DWORD	status	;
*!*		wchar_t szHint [DAPI_HINT_SIZE] ;
*!*	} praInfo ;

DEFINE CLASS praInfo AS Relation

	Address = 0
	SizeOf = 92
	Name = "praInfo"
	&& structure fields
	maxNOA = .F.
	currNOA = .F.
	size = .F.
	status = .F.
	szHint = .F.

	PROCEDURE Init()
		THIS.Address = AllocMem(THIS.SizeOf)
		IF THIS.Address = 0
			ERROR(43)
			RETURN .F.
		ENDIF
	ENDPROC

	PROCEDURE Destroy()
		FreeMem(THIS.Address)
	ENDPROC

	PROCEDURE maxNOA_Access()
		RETURN ReadUInt(THIS.Address)
	ENDPROC

	PROCEDURE maxNOA_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address,lnNewVal)
	ENDPROC

	PROCEDURE currNOA_Access()
		RETURN ReadUInt(THIS.Address+4)
	ENDPROC

	PROCEDURE currNOA_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+4,lnNewVal)
	ENDPROC

	PROCEDURE size_Access()
		RETURN ReadUInt64AsDouble(THIS.Address+8)
	ENDPROC

	PROCEDURE size_Assign(lnNewVal)
		WriteUInt64(THIS.Address+8,lnNewVal)
	ENDPROC

	PROCEDURE status_Access()
		RETURN ReadUInt(THIS.Address+16)
	ENDPROC

	PROCEDURE status_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+16,lnNewVal)
	ENDPROC

	PROCEDURE szHint_Access()
		RETURN ReadWCharArray(THIS.Address+20,33)
	ENDPROC

	PROCEDURE szHint_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'C' MESSAGE 'Wrong datatype or value out of range!'
		WriteWCharArray(THIS.Address+20,lnNewVal,33)
	ENDPROC

ENDDEFINE
