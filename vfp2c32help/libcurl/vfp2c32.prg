********************************************************************************
*!* vfp2c32.prg
********************************************************************************

Set Procedure To Sys(16) Additive

If "VFP2C32.FLL" $ Upper(Set("Library")) Then
	If !InitVFP2C32(0xFFFFFFFF)
		Debugout "InitVFP2C32 FAILED!"
	Endif
	Return
Endif

Local ;
	lcLocalPath As String, ;
	lcFile As String

m.lcFile = "vfp2c32.fll"

m.lcLocalPath = Addbs(Justpath(Sys(16)))

Set Library To (m.lcLocalPath + m.lcFile) Additive

*!* Add vfp2c32.fll path to search path or InitVFP2C32 fails:
*!* (fixed in 2.0.0.4)
Local m.lcLocalPath As String

m.lcLocalPath = Addbs(Justpath(m.lcFile))

If Empty(Set("Path")) Then
	Set Path To (m.lcLocalPath)
Else
	If Not m.lcLocalPath $ Set("Path") Then
		Set Path To (Set("Path") + ";" + m.lcLocalPath)
	Endif
Endif

If !InitVFP2C32(0xFFFFFFFF)
	Debugout "InitVFP2C32 FAILED!"
Endif

Return

*!* 20080904 -http://ctl32.freeforums.org/error-no-1-utilizando-ctl32-progressbar-t41.html
*!* Protect declared vfp2c32 functions against CLEAR DLLS:

Function ReadDouble
	Lparameters Param1
	Declare Double ReadDouble In vfp2c32.fll Integer
	Return ReadDouble(m.Param1)
Endfunc

Function ReadFloat
	Lparameters Param1
	Declare Single ReadFloat In vfp2c32.fll Integer
	Return ReadFloat(m.Param1)
Endfunc

Function ReadInt
	Lparameters Param1
	Declare Integer ReadInt In vfp2c32.fll Integer
	Return ReadInt(m.Param1)
Endfunc

Function ReadInt8
	Lparameters Param1
	Declare Short ReadInt8 In vfp2c32.fll Integer
	Return ReadInt8(m.Param1)
Endfunc

Function ReadPDouble
	Lparameters Param1
	Declare Double ReadPDouble In vfp2c32.fll Integer
	Return ReadPDouble(m.Param1)
Endfunc

Function ReadPFloat
	Lparameters Param1
	Declare Single ReadPFloat In vfp2c32.fll Integer
	Return ReadPFloat(m.Param1)
Endfunc

Function ReadPInt
	Lparameters Param1
	Declare Integer ReadPInt In vfp2c32.fll Integer
	Return ReadPInt(m.Param1)
Endfunc

Function ReadPInt8
	Lparameters Param1
	Declare Short ReadPInt8 In vfp2c32.fll Integer
	Return ReadPInt8(m.Param1)
Endfunc

Function ReadPShort
	Lparameters Param1
	Declare Short ReadPShort In vfp2c32.fll Integer
	Return ReadPShort(m.Param1)
Endfunc

Function ReadPUInt8
	Lparameters Param1
	Declare Short ReadPUInt8 In vfp2c32.fll Integer
	Return ReadPUInt8(m.Param1)
Endfunc

Function ReadPUShort
	Lparameters Param1
	Declare Integer ReadPUShort In vfp2c32.fll Integer
	Return ReadPUShort(m.Param1)
Endfunc

Function ReadShort
	Lparameters Param1
	Declare Short ReadShort In vfp2c32.fll Integer
	Return ReadShort(m.Param1)
Endfunc

Function ReadUInt8
	Lparameters Param1
	Declare Short ReadUInt8 In vfp2c32.fll Integer
	Return ReadUInt8(m.Param1)
Endfunc

Function ReadUShort
	Lparameters Param1
	Declare Integer ReadUShort In vfp2c32.fll Integer
	Return ReadUShort(m.Param1)
Endfunc

Function WriteChar
	Lparameters Param1, Param2
	Declare WriteChar In vfp2c32.fll Integer, String
	Return WriteChar(m.Param1, m.Param2)
Endfunc

Function WriteDouble
	Lparameters Param1, Param2
	Declare WriteDouble In vfp2c32.fll Integer, Double
	Return WriteDouble(m.Param1, m.Param2)
Endfunc

Function WriteFloat
	Lparameters Param1, Param2
	Declare WriteFloat In vfp2c32.fll Integer, Single
	Return WriteFloat(m.Param1, m.Param2)
Endfunc

Function WriteInt
	Lparameters Param1, Param2
	Declare WriteInt In vfp2c32.fll Integer, Integer
	Return WriteInt(m.Param1, m.Param2)
Endfunc

Function WriteInt8
	Lparameters Param1, Param2
	Declare WriteInt8 In vfp2c32.fll Integer, Integer
	Return WriteInt8(m.Param1, m.Param2)
Endfunc

Function WritePChar
	Lparameters Param1, Param2
	Declare WritePChar In vfp2c32.fll Integer, String
	Return WritePChar(m.Param1, m.Param2)
Endfunc

Function WritePDouble
	Lparameters Param1, Param2
	Declare WritePDouble In vfp2c32.fll Integer, Double
	Return WritePDouble(m.Param1, m.Param2)
Endfunc

Function WritePFloat
	Lparameters Param1, Param2
	Declare WritePFloat In vfp2c32.fll Integer, Single
	Return WritePFloat(m.Param1, m.Param2)
Endfunc

Function WritePInt
	Lparameters Param1, Param2
	Declare WritePInt In vfp2c32.fll Integer, Integer
	Return WritePInt(m.Param1, m.Param2)
Endfunc

Function WritePInt8
	Lparameters Param1, Param2
	Declare WritePInt8 In vfp2c32.fll Integer, Integer
	Return WritePInt8(m.Param1, m.Param2)
Endfunc

Function WritePointer
	Lparameters Param1, Param2
	Declare WritePointer In vfp2c32.fll Integer, Integer
	Return WritePointer(m.Param1, m.Param2)
Endfunc

Function WritePPointer
	Lparameters Param1, Param2
	Declare WritePPointer In vfp2c32.fll Integer, Integer
	Return WritePPointer(m.Param1, m.Param2)
Endfunc

Function WritePShort
	Lparameters Param1, Param2
	Declare WritePShort In vfp2c32.fll Integer, Short
	Return WritePShort(m.Param1, m.Param2)
Endfunc

Function WritePUInt
	Lparameters Param1, Param2
	Declare WritePUInt In vfp2c32.fll Integer, Integer
	Return WritePUInt(m.Param1, m.Param2)
Endfunc

Function WritePUInt8
	Lparameters Param1, Param2
	Declare WritePUInt8 In vfp2c32.fll Integer, Integer
	Return WritePUInt8(m.Param1, m.Param2)
Endfunc

Function WritePUShort
	Lparameters Param1, Param2
	Declare WritePUShort In vfp2c32.fll Integer, Short
	Return WritePUShort(m.Param1, m.Param2)
Endfunc

Function WriteShort
	Lparameters Param1, Param2
	Declare WriteShort In vfp2c32.fll Integer, Short
	Return WriteShort(m.Param1, m.Param2)
Endfunc

Function WriteUInt
	Lparameters Param1, Param2
	Declare WriteUInt In vfp2c32.fll Integer, Integer
	Return WriteUInt(m.Param1, m.Param2)
Endfunc

Function WriteUInt8
	Lparameters Param1, Param2
	Declare WriteUInt8 In vfp2c32.fll Integer, Integer
	Return WriteUInt8(m.Param1, m.Param2)
Endfunc

Function WriteUShort
	Lparameters Param1, Param2
	Declare WriteUShort In vfp2c32.fll Integer, Short
	Return WriteUShort(m.Param1, m.Param2)
Endfunc

*!* don't call this function ...
*!* it's only purpose is to make the function names visible for compilation
*!* the functions are DECLARE'd inside the library automatically

Function VFP2CDummyProc
	Declare AAverage In vfp2c32.fll
	Declare AbortUrlDownloadToFileEx In vfp2c32.fll
	Declare ADesktopArea In vfp2c32.fll
	Declare ADesktops In vfp2c32.fll
	Declare ADialUpConnections In vfp2c32.fll
	Declare ADirectoryInfo In vfp2c32.fll
	Declare ADirEx In vfp2c32.fll
	Declare ADisplayDevices In vfp2c32.fll
	Declare ADriveInfo In vfp2c32.fll
	Declare AErrorEx In vfp2c32.fll
	Declare AFHandlesEx In vfp2c32.fll
	Declare AFileAttributes In vfp2c32.fll
	Declare AFileAttributesEx In vfp2c32.fll
	Declare AHeapBlocks In vfp2c32.fll
	Declare AIPAddresses In vfp2c32.fll
	Declare AllocHGlobal In vfp2c32.fll
	Declare AllocMem In vfp2c32.fll
	Declare AllocMemTo In vfp2c32.fll
	Declare AMax In vfp2c32.fll
	Declare AMemBlocks In vfp2c32.fll
	Declare AMin In vfp2c32.fll
	Declare ANetFiles In vfp2c32.fll
	Declare ANetServers In vfp2c32.fll
	Declare APrinterForms In vfp2c32.fll
	Declare APrintersEx In vfp2c32.fll
	Declare APrinterTrays In vfp2c32.fll
	Declare APrintJobs In vfp2c32.fll
	Declare AProcesses In vfp2c32.fll
	Declare AProcessHeaps In vfp2c32.fll
	Declare AProcessModules In vfp2c32.fll
	Declare AProcessThreads In vfp2c32.fll
	Declare ARegistryKeys In vfp2c32.fll
	Declare ARegistryValues In vfp2c32.fll
	Declare AResolutions In vfp2c32.fll
	Declare AResourceLanguages In vfp2c32.fll
	Declare AResourceNames In vfp2c32.fll
	Declare AResourceTypes In vfp2c32.fll
	Declare AServices In vfp2c32.fll
	Declare AServiceStatus In vfp2c32.fll
	Declare ASplitStr In vfp2c32.fll
	Declare ASQLDataSources In vfp2c32.fll
	Declare ASQLDrivers In vfp2c32.fll
	Declare ASum In vfp2c32.fll
	Declare AsyncInvoke In vfp2c32.fll
	Declare ATimeZones In vfp2c32.fll
	Declare AWindowProps In vfp2c32.fll
	Declare AWindows In vfp2c32.fll
	Declare AWindowsEx In vfp2c32.fll
	Declare AWindowStations In vfp2c32.fll
	Declare BindEventsEx In vfp2c32.fll
	Declare CancelFileChange In vfp2c32.fll
	Declare CancelRegistryChange In vfp2c32.fll
	Declare CenterWindowEx In vfp2c32.fll
	Declare ChangeSQLDataSource In vfp2c32.fll
	Declare CloseRegistryKey In vfp2c32.fll
	Declare CloseServiceHandle In vfp2c32.fll
	Declare CLSIDFromProgID In vfp2c32.fll
	Declare CLSIDFromString In vfp2c32.fll
	Declare Colors2RGB In vfp2c32.fll
	Declare CompactMem In vfp2c32.fll
	Declare CompareFileTimes In vfp2c32.fll
	Declare ContinueService In vfp2c32.fll
	Declare ControlService In vfp2c32.fll
	Declare CopyFileEx In vfp2c32.fll
	Declare CreateCallbackFunc In vfp2c32.fll
	Declare CreateGuid In vfp2c32.fll
	Declare CreateRegistryKey In vfp2c32.fll
	Declare CreateSQLDataSource In vfp2c32.fll
	Declare Decimals In vfp2c32.fll
	Declare DeleteDirectory In vfp2c32.fll
	Declare DeleteFileEx In vfp2c32.fll
	Declare DeleteRegistryKey In vfp2c32.fll
	Declare DeleteSQLDataSource In vfp2c32.fll
	Declare DestroyCallbackFunc In vfp2c32.fll
	Declare Double2DT In vfp2c32.fll
	Declare Double2Str In vfp2c32.fll
	Declare DT2Double In vfp2c32.fll
	Declare DT2FT In vfp2c32.fll
	Declare DT2ST In vfp2c32.fll
	Declare DT2Timet In vfp2c32.fll
	Declare DT2UTC In vfp2c32.fll
	Declare ExpandEnvironmentStrings In vfp2c32.fll
	Declare FChSizeEx In vfp2c32.fll
	Declare FCloseEx In vfp2c32.fll
	Declare FCreateEx In vfp2c32.fll
	Declare FEoFEx In vfp2c32.fll
	Declare FFlushEx In vfp2c32.fll
	Declare FGetsEx In vfp2c32.fll
	Declare FHandleEx In vfp2c32.fll
	Declare FindFileChange In vfp2c32.fll
	Declare FindRegistryChange In vfp2c32.fll
	Declare Float2Str In vfp2c32.fll
	Declare FLockFile In vfp2c32.fll
	Declare FLockFileEx In vfp2c32.fll
	Declare FOpenEx In vfp2c32.fll
	Declare FormatMessageEx In vfp2c32.fll
	Declare FPutsEx In vfp2c32.fll
	Declare FReadEx In vfp2c32.fll
	Declare FreeHGlobal In vfp2c32.fll
	Declare FreeMem In vfp2c32.fll
	Declare FreePMem In vfp2c32.fll
	Declare FreeRefArray In vfp2c32.fll
	Declare FSeekEx In vfp2c32.fll
	Declare FT2DT In vfp2c32.fll
	Declare FUnlockFile In vfp2c32.fll
	Declare FUnlockFileEx In vfp2c32.fll
	Declare FWriteEx In vfp2c32.fll
	Declare GetCursorPosEx In vfp2c32.fll
	Declare GetFileAttributes In vfp2c32.fll
	Declare GetFileOwner In vfp2c32.fll
	Declare GetFileSize In vfp2c32.fll
	Declare GetFileTimes In vfp2c32.fll
	Declare GetIUnknown In vfp2c32.fll
	Declare GetLongPathName In vfp2c32.fll
	Declare GetOpenFileName In vfp2c32.fll
	Declare GetSaveFileName In vfp2c32.fll
	Declare GetServerTime In vfp2c32.fll
	Declare GetShortPathName In vfp2c32.fll
	Declare GetSystemDirectory In vfp2c32.fll
	Declare GetSystemTime In vfp2c32.fll
	Declare GetWindowRectEx In vfp2c32.fll
	Declare GetWindowsDirectory In vfp2c32.fll
	Declare GetWindowTextEx In vfp2c32.fll
	Declare IcmpPing In vfp2c32.fll
	Declare InitVFP2C32 In vfp2c32.fll
	Declare Int64_Add In vfp2c32.fll
	Declare Int64_Div In vfp2c32.fll
	Declare Int64_Mod In vfp2c32.fll
	Declare Int64_Mul In vfp2c32.fll
	Declare Int64_Sub In vfp2c32.fll
	Declare Invoke In vfp2c32.fll
	Declare Ip2MacAddress In vfp2c32.fll
	Declare IsEqualGuid In vfp2c32.fll
	Declare LockHGlobal In vfp2c32.fll
	Declare Long2Str In vfp2c32.fll
	Declare MarshalArrayCharArray In vfp2c32.fll
	Declare MarshalArrayCString In vfp2c32.fll
	Declare MarshalArrayDouble In vfp2c32.fll
	Declare MarshalArrayFloat In vfp2c32.fll
	Declare MarshalArrayInt In vfp2c32.fll
	Declare MarshalArrayLogical In vfp2c32.fll
	Declare MarshalArrayShort In vfp2c32.fll
	Declare MarshalArrayUInt In vfp2c32.fll
	Declare MarshalArrayUShort In vfp2c32.fll
	Declare MarshalArrayWCharArray In vfp2c32.fll
	Declare MarshalArrayWString In vfp2c32.fll
	Declare MarshalCursorCharArray In vfp2c32.fll
	Declare MarshalCursorCString In vfp2c32.fll
	Declare MarshalCursorDouble In vfp2c32.fll
	Declare MarshalCursorFloat In vfp2c32.fll
	Declare MarshalCursorInt In vfp2c32.fll
	Declare MarshalCursorLogical In vfp2c32.fll
	Declare MarshalCursorShort In vfp2c32.fll
	Declare MarshalCursorUInt In vfp2c32.fll
	Declare MarshalCursorUShort In vfp2c32.fll
	Declare MarshalCursorWCharArray In vfp2c32.fll
	Declare MarshalCursorWString In vfp2c32.fll
	Declare MoveFileEx In vfp2c32.fll
	Declare Num2Binary In vfp2c32.fll
	Declare OnLoad In vfp2c32.fll
	Declare OnUnload In vfp2c32.fll
	Declare OpenRegistryKey In vfp2c32.fll
	Declare OpenService In vfp2c32.fll
	Declare PauseService In vfp2c32.fll
	Declare PG_ByteA2Str In vfp2c32.fll
	Declare PG_Str2ByteA In vfp2c32.fll
	Declare ProgIDFromCLSID In vfp2c32.fll
	Declare ReadBytes In vfp2c32.fll
	Declare ReadChar In vfp2c32.fll
	Declare ReadCharArray In vfp2c32.fll
	Declare ReadCString In vfp2c32.fll
	Declare ReadInt64AsDouble In vfp2c32.fll
	Declare ReadLogical In vfp2c32.fll
	Declare ReadPChar In vfp2c32.fll
	Declare ReadPCString In vfp2c32.fll
	Declare ReadPInt64AsDouble In vfp2c32.fll
	Declare ReadPLogical In vfp2c32.fll
	Declare ReadPointer In vfp2c32.fll
	Declare ReadPPointer In vfp2c32.fll
	Declare ReadProcessMemoryEx In vfp2c32.fll
	Declare ReadPUInt In vfp2c32.fll
	Declare ReadPUInt64AsDouble In vfp2c32.fll
	Declare ReadPWString In vfp2c32.fll
	Declare ReadRegistryKey In vfp2c32.fll
	Declare ReadUInt In vfp2c32.fll
	Declare ReadUInt64AsDouble In vfp2c32.fll
	Declare ReadWCharArray In vfp2c32.fll
	Declare ReadWString In vfp2c32.fll
	Declare ReAllocHGlobal In vfp2c32.fll
	Declare ReAllocMem In vfp2c32.fll
	Declare RegisterActiveObject In vfp2c32.fll
	Declare RegisterObjectAsFileMoniker In vfp2c32.fll
	Declare RegistryHiveToObject In vfp2c32.fll
	Declare RegistryValuesToObject In vfp2c32.fll
	Declare ResolveHostToIp In vfp2c32.fll
	Declare RevokeActiveObject In vfp2c32.fll
	Declare RGB2Colors In vfp2c32.fll
	Declare SetFileAttributes In vfp2c32.fll
	Declare SetFileTimes In vfp2c32.fll
	Declare SetSystemTime In vfp2c32.fll
	Declare SetSystemTimeEx In vfp2c32.fll
	Declare SHBrowseFolder In vfp2c32.fll
	Declare SHCopyFiles In vfp2c32.fll
	Declare SHDeleteFiles In vfp2c32.fll
	Declare SHMoveFiles In vfp2c32.fll
	Declare Short2Str In vfp2c32.fll
	Declare SHRenameFiles In vfp2c32.fll
	Declare SHSpecialFolder In vfp2c32.fll
	Declare SizeOfMem In vfp2c32.fll
	Declare SQLExecEx In vfp2c32.fll
	Declare SQLGetPropEx In vfp2c32.fll
	Declare SQLSetPropEx In vfp2c32.fll
	Declare ST2DT In vfp2c32.fll
	Declare StartService In vfp2c32.fll
	Declare StopService In vfp2c32.fll
	Declare Str2Double In vfp2c32.fll
	Declare Str2Float In vfp2c32.fll
	Declare Str2Long In vfp2c32.fll
	Declare Str2Short In vfp2c32.fll
	Declare Str2ULong In vfp2c32.fll
	Declare Str2UShort In vfp2c32.fll
	Declare StringFromCLSID In vfp2c32.fll
	Declare SyncToServerTime In vfp2c32.fll
	Declare SyncToSNTPServer In vfp2c32.fll
	Declare Timet2DT In vfp2c32.fll
	Declare ULong2Str In vfp2c32.fll
	Declare UnBindEventsEx In vfp2c32.fll
	Declare UnlockHGlobal In vfp2c32.fll
	Declare UnMarshalArrayCharArray In vfp2c32.fll
	Declare UnMarshalArrayCString In vfp2c32.fll
	Declare UnMarshalArrayDouble In vfp2c32.fll
	Declare UnMarshalArrayFloat In vfp2c32.fll
	Declare UnMarshalArrayInt In vfp2c32.fll
	Declare UnMarshalArrayLogical In vfp2c32.fll
	Declare UnMarshalArrayShort In vfp2c32.fll
	Declare UnMarshalArrayUInt In vfp2c32.fll
	Declare UnMarshalArrayUShort In vfp2c32.fll
	Declare UnMarshalArrayWCharArray In vfp2c32.fll
	Declare UnMarshalArrayWString In vfp2c32.fll
	Declare UnMarshalCursorCharArray In vfp2c32.fll
	Declare UnMarshalCursorCString In vfp2c32.fll
	Declare UnMarshalCursorDouble In vfp2c32.fll
	Declare UnMarshalCursorFloat In vfp2c32.fll
	Declare UnMarshalCursorInt In vfp2c32.fll
	Declare UnMarshalCursorLogical In vfp2c32.fll
	Declare UnMarshalCursorShort In vfp2c32.fll
	Declare UnMarshalCursorUInt In vfp2c32.fll
	Declare UnMarshalCursorUShort In vfp2c32.fll
	Declare UnMarshalCursorWCharArray In vfp2c32.fll
	Declare UnMarshalCursorWString In vfp2c32.fll
	Declare UrlDownloadToFileEx In vfp2c32.fll
	Declare UShort2Str In vfp2c32.fll
	Declare UTC2DT In vfp2c32.fll
	Declare ValidateMem In vfp2c32.fll
	Declare Value2Variant In vfp2c32.fll
	Declare Variant2Value In vfp2c32.fll
	Declare VFP2CSys In vfp2c32.fll
	Declare WriteBytes In vfp2c32.fll
	Declare WriteCharArray In vfp2c32.fll
	Declare WriteCString In vfp2c32.fll
	Declare WriteInt64 In vfp2c32.fll
	Declare WriteLogical In vfp2c32.fll
	Declare WritePCString In vfp2c32.fll
	Declare WritePInt64 In vfp2c32.fll
	Declare WritePLogical In vfp2c32.fll
	Declare WritePUInt64 In vfp2c32.fll
	Declare WritePWChar In vfp2c32.fll
	Declare WritePWString In vfp2c32.fll
	Declare WriteRegistryKey In vfp2c32.fll
	Declare WriteUInt64 In vfp2c32.fll
	Declare WriteWChar In vfp2c32.fll
	Declare WriteWCharArray In vfp2c32.fll
	Declare WriteWString In vfp2c32.fll
Endfunc


