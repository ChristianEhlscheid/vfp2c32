#INCLUDE "vfp2c.h"
#INCLUDE "foxpro.h"
#INCLUDE "vfpservice.h"
#INCLUDE "exampleservice3.h"

&& this service watches the subdirectory "watchedfolder" for file changes
DEFINE CLASS ExampleService3 AS ServiceBaseclass OLEPUBLIC

	&& 	custom properties
	oWorkerThread = .NULL.
	oServiceController = .NULL.
	
	FUNCTION  OnStart(laArgs[] As String,  loServiceController AS Object) As Void
		TRY
#IFDEF _DEBUG
		DebugOutput('ExampleService3::OnStart')
#ENDIF
			LOCAL lcDirectory
			SET LIBRARY TO vfp2c32t.fll ADDITIVE
			IF INITVFP2C32(VFP2C_INIT_MARSHAL) = .F.
				LOCAL laError[1]
				AERROREX('laError')
				THROW CREATEOBJECT('Win32Exception', m.laError[1], m.laError[2], m.laError[3])
			ENDIF
			THIS.oServiceController = m.loServiceController
			THIS.oWorkerThread = CreateThreadObject('VFPExampleService.ExampleWorker3', THIS, .T.)
			m.lcDirectory = ADDBS(FULLPATH(CURDIR())) + 'watchedfolder'
			&& by the following call is syncronous 
			THIS.oWorkerThread.Object.Initialize(m.lcDirectory)
			THIS.oWorkerThread.ThreadFunc()
		CATCH TO loExc
			THIS.oWorkerThread = .NULL.
			THIS.ThrowError(m.loExc)
		ENDTRY
	ENDFUNC

	FUNCTION OnStop() AS Void
		TRY
#IFDEF _DEBUG
		DebugOutput('ExampleService3::OnStop')
#ENDIF	
			THIS.oWorkerThread.AbortCall()
			THIS.oWorkerThread = .NULL.
			SET LIBRARY TO
		CATCH TO loExc
			THIS.ThrowError(m.loExc)
		ENDTRY
		RETURN 0
	ENDFUNC

	FUNCTION OnPause() AS Void
		TRY
#IFDEF _DEBUG
		DebugOutput('ExampleService3::OnPause')
#ENDIF	
			THIS.oWorkerThread.AbortCall()
		CATCH
			THIS.ThrowError(m.loExc)
		ENDTRY
	ENDFUNC

	FUNCTION OnContinue() AS Void
		TRY
#IFDEF _DEBUG
		DebugOutput('ExampleService3::OnContinue')
#ENDIF		
			THIS.oWorkerThread.ThreadFunc()
		CATCH
			THIS.ThrowError(m.loExc)
		ENDTRY
	ENDFUNC

	&& gets called when the threaded function returns an error, we just stop the service ...
	FUNCTION OnError(callid AS Long, callcontext AS Variant, errornumber AS Long, errorsource AS String, errordescription AS String) AS Void
#IFDEF _DEBUG
		DebugOutput('ExampleService3::OnError')
#ENDIF
		THIS.oServiceController.Stop()
	ENDFUNC
	
ENDDEFINE

DEFINE CLASS ExampleWorker3 AS ServiceBaseclass OLEPUBLIC

	DataSession = 2
	oDirWatcher = .NULL.
	CallInfo = .NULL.

#IFDEF _DEBUG
	ChangeReason_COMAttrib = 0x01 && don't expose property
	DIMENSION ChangeReason[1]
#ENDIF
	
	FUNCTION Initialize(lcDirectory AS String) AS Void
		TRY
			SET LIBRARY TO vfp2c32t.fll ADDITIVE
			THIS.oDirWatcher = CREATEOBJECT('DirectoryWatcher', m.lcDirectory)
		CATCH TO loExc
			THIS.ThrowError(m.loExc)
		ENDTRY

#IFDEF _DEBUG
		DIMENSION THIS.ChangeReason[5]
		THIS.ChangeReason[1] = 'Added'
		THIS.ChangeReason[2] = 'Removed'
		THIS.ChangeReason[3] = 'Modified'
		THIS.ChangeReason[4] = 'Renamed Old Name'
		THIS.ChangeReason[5] = 'Renamed New Name'
#ENDIF		
	ENDFUNC
	
	FUNCTION ThreadFunc() As Void
		TRY

#IFDEF _DEBUG
		DebugOutput('ExampleWorker3::ThreadFunc')
#ENDIF
	
			LOCAL lnRet, lnFileCount, laChangedFiles[1,2]
			DO WHILE .T.
			
				IF THIS.oDirWatcher.WatchDirectory(.F., FILE_NOTIFY_CHANGE_FILE_NAME + FILE_NOTIFY_CHANGE_SIZE, THIS.CallInfo.AbortEvent)

					m.lnFileCount = THIS.oDirWatcher.GetChanges(@m.laChangedFiles)
					
#IFDEF _DEBUG
					LOCAL xj
					FOR m.xj = 1 TO m.lnFileCount
						DebugOutput('Directory changed - Action: ' + THIS.ChangeReason[m.laChangedFiles[m.xj,1]] + ' File: ' + m.laChangedFiles[m.xj, 2])
					ENDFOR
					RELEASE xj
#ENDIF	
			
				ELSE
				
					THIS.oDirWatcher.CancelWatch()
					EXIT
					
				ENDIF
				
			ENDDO
		
		CATCH TO loExc
			THIS.ThrowError(m.loExc)
		ENDTRY
				
	ENDFUNC
	
ENDDEFINE


DEFINE CLASS DirectoryWatcher AS Custom

	cDirectory = ''
	PROTECTED hDirHandle
	hDirHandle = INVALID_HANDLE_VALUE && handle to the directory which is monitored for changes
	PROTECTED bWatching 
	bWatching = .F.	&& flag if the directory is currently monitored or not
	PROTECTED oOverlapped 
	oOverlapped = .NULL. && OVERLAPPED structure for asynchronous call to ReadDirectoryChangesW
	PROTECTED oEvent 
	oEvent = .NULL. && a CEvent object which wraps an Winapi event (CreateEvent) which is signaled when the directory changes
	PROTECTED oNotifyData 
	oNotifyData = .NULL. && FILE_NOTIFY_INFORMATION structure which will receive the information about the directory changes
	
	FUNCTION Init
		LPARAMETERS lcDirectory
		THIS.oOverlapped = CREATEOBJECT('OVERLAPPED')
		&& create a manual reset event object
		THIS.oEvent = CREATEOBJECT('CEvent', .T.)		
		&& store handle of the event object in the OVERLAPPED structure
		THIS.oOverlapped.hEvent = THIS.oEvent.GetHandle()
		THIS.oNotifyData = CREATEOBJECT('FILE_NOTIFY_INFORMATION')
		 && allocate a buffer of 32KB
		THIS.oNotifyData.BufferSize = 0x10000
		&& open a handle to the current directory
		THIS.cDirectory = m.lcDirectory
		THIS.hDirHandle = CreateFile(m.lcDirectory, FILE_LIST_DIRECTORY, FILE_SHARE_READ + FILE_SHARE_WRITE + FILE_SHARE_DELETE, 0, ;
								OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS + FILE_FLAG_OVERLAPPED, 0)
		IF THIS.hDirHandle = INVALID_HANDLE_VALUE
			THROW CREATEOBJECT('Win32Exception', GetLastError(), 'CreateFile')
		ENDIF
	ENDFUNC

	FUNCTION Destroy
		THIS.CancelWatch()
		IF THIS.hDirHandle != INVALID_HANDLE_VALUE
			CloseHandle(THIS.hDirHandle)
		ENDIF
	ENDFUNC

	FUNCTION WatchDirectory
		LPARAMETERS bSubDirectories, nFilter, nAbortEvent
		LOCAL lnRet
		IF !THIS.bWatching
			m.lnRet = ReadDirectoryChangesW(THIS.hDirHandle, THIS.oNotifyData.Address, THIS.oNotifyData.BufferSize, ;
						IIF(m.bSubDirectories, TRUE, FALSE), nFilter, 0, THIS.oOverlapped.Address, 0)
			IF m.lnRet = 0
				THROW CREATEOBJECT('Win32Exception', GetLastError(), 'ReadDirectoryChangesW')
			ENDIF
			THIS.bWatching = .T.
		ENDIF
		m.lnRet = THIS.oEvent.Wait(INFINITE, m.nAbortEvent)
		THIS.bWatching = (m.lnRet != WAIT_OBJECT_0)
		RETURN m.lnRet = WAIT_OBJECT_0
	ENDFUNC

	FUNCTION CancelWatch
		LOCAL lnRet, lnBytes
		IF THIS.bWatching 
			m.lnRet = CancelIo(THIS.hDirHandle)
			IF m.lnRet = 0
				THROW CREATEOBJECT('Win32Exception', GetLastError(), 'CancelIoEx')
			ENDIF
			&& wait till the cancel operation completed
			m.lnBytes = 0
			m.lnRet = GetOverlappedResult(THIS.hDirHandle, THIS.oOverlapped.Address, @m.lnBytes, TRUE)
			THIS.bWatching = .F.
		ENDIF
	ENDFUNC

	FUNCTION GetChanges(laChanges)
		EXTERNAL ARRAY laChanges
		LOCAL lnBytes, lnRet, lnCount
		m.lnBytes = 0
		m.lnCount = 0
		m.lnRet = GetOverlappedResult(THIS.hDirHandle, THIS.oOverlapped.Address, @m.lnBytes, FALSE)
		IF m.lnRet = 0
			THROW CREATEOBJECT('Win32Exception', GetLastError(), 'GetOverlappedResult')		
		ENDIF

		DO WHILE .T.
			m.lnCount = m.lnCount + 1 
			DIMENSION m.laChanges[m.lnCount, 2]
			m.laChanges[m.lnCount, 1] = THIS.oNotifyData.Action
			m.laChanges[m.lnCount, 2] = THIS.oNotifyData.FileName			
			IF !THIS.oNotifyData.SkipEntry()
				EXIT				
			ENDIF
		ENDDO

		RETURN m.lnCount		
	ENDFUNC

ENDDEFINE

DEFINE CLASS OVERLAPPED AS Exception

	Address = 0
	SizeOf = 20
	Name = "OVERLAPPED"
	&& structure fields
	Internal = .F.
	InternalHigh = .F.
	mOffset = .F.
	OffsetHigh = .F.
	Pointer = .F.
	hEvent = .F.

	PROCEDURE Init()
		THIS.Address = AllocMem(THIS.SizeOf)
	ENDPROC

	PROCEDURE Destroy()
		FreeMem(THIS.Address)
	ENDPROC

	PROCEDURE Internal_Access()
		RETURN ReadUInt(THIS.Address)
	ENDPROC

	PROCEDURE Internal_Assign(lnNewVal)
		WriteUInt(THIS.Address,lnNewVal)
	ENDPROC

	PROCEDURE InternalHigh_Access()
		RETURN ReadUInt(THIS.Address+4)
	ENDPROC

	PROCEDURE InternalHigh_Assign(lnNewVal)
		WriteUInt(THIS.Address+4,lnNewVal)
	ENDPROC

	PROCEDURE mOffset_Access()
		RETURN ReadUInt(THIS.Address+8)
	ENDPROC

	PROCEDURE mOffset_Assign(lnNewVal)
		WriteUInt(THIS.Address+8,lnNewVal)
	ENDPROC

	PROCEDURE OffsetHigh_Access()
		RETURN ReadUInt(THIS.Address+12)
	ENDPROC

	PROCEDURE OffsetHigh_Assign(lnNewVal)
		WriteUInt(THIS.Address+12,lnNewVal)
	ENDPROC

	PROCEDURE Pointer_Access()
		RETURN ReadPointer(THIS.Address+8)
	ENDPROC

	PROCEDURE Pointer_Assign(lnNewVal)
		WritePointer(THIS.Address+8,lnNewVal)
	ENDPROC

	PROCEDURE hEvent_Access()
		RETURN ReadPointer(THIS.Address+16)
	ENDPROC

	PROCEDURE hEvent_Assign(lnNewVal)
		WritePointer(THIS.Address+16,lnNewVal)
	ENDPROC

ENDDEFINE

&& the FILE_NOTIFY_INFORMATION is somewhat special
&& it's contents are dynamic, the custom coded SkipEntry method allows to iterate over the structure contents
&& it returns .T. if there is another entry, if there isn't another entry the structure is reset to the first entry and the function returns .F.
&& FileName_Access is also modified to return only upto FileNameLength characters
DEFINE CLASS FILE_NOTIFY_INFORMATION AS Exception

	Address = 0
	SizeOf = 16
	BufferSize = 0
	Offset = 0
	Name = "FILE_NOTIFY_INFORMATION"
	&& structure fields
	NextEntryOffset = .F.
	Action = .F.
	FileNameLength = .F.
	FileName = .F.

	PROCEDURE Destroy()
		FreeMem(THIS.Address)
	ENDPROC

	PROCEDURE BufferSize_Assign(lnBufferSize)
		LOCAL lnAddress
		lnAddress = ReAllocMem(THIS.Address, lnBufferSize)
		THIS.Address = lnAddress
		THIS.Offset = lnAddress
		THIS.BufferSize = lnBufferSize
	ENDPROC

	PROCEDURE SkipEntry
		LOCAL lnNextOffset
		m.lnNextOffset = THIS.NextEntryOffset
		IF m.lnNextOffset = 0
			THIS.Offset = THIS.Address
		ELSE
			THIS.Offset = THIS.Offset + m.lnNextOffset 
		ENDIF
		RETURN m.lnNextOffset > 0
	ENDPROC

	PROCEDURE NextEntryOffset_Access()
		RETURN ReadUInt(THIS.Offset)
	ENDPROC

	PROCEDURE Action_Access()
		RETURN ReadUInt(THIS.Offset+4)
	ENDPROC

	PROCEDURE FileNameLength_Access()
		RETURN ReadUInt(THIS.Offset+8)
	ENDPROC

	PROCEDURE FileName_Access()
		RETURN ReadWCharArray(THIS.Offset+12, THIS.FileNameLength / 2)
	ENDPROC

ENDDEFINE

FUNCTION CreateFile(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile)
	DECLARE INTEGER CreateFile IN WIN32API STRING lpFileName, INTEGER dwDesiredAccess, INTEGER dwShareMode, INTEGER lpSecurityAttributes, ;
		INTEGER dwCreationDisposition, INTEGER dwFlagsAndAttributes, INTEGER hTemplateFile
	RETURN CreateFile(m.lpFileName, m.dwDesiredAccess, m.dwShareMode, m.lpSecurityAttributes, m.dwCreationDisposition, m.dwFlagsAndAttributes, m.hTemplateFile)		
ENDFUNC

FUNCTION ReadDirectoryChangesW(hDirectory, lpBuffer, nBufferLength, bWatchSubtree, dwNotifyFilter, lpBytesReturned, lpOverlapped, lpCompletionRoutine)
	DECLARE INTEGER ReadDirectoryChangesW IN WIN32API INTEGER hDirectory, INTEGER lpBuffer, INTEGER nBufferLength, INTEGER bWatchSubtree, ;
		INTEGER dwNotifyFilter, INTEGER lpBytesReturned, INTEGER lpOverlapped, INTEGER lpCompletionRoutine
	RETURN ReadDirectoryChangesW(m.hDirectory, m.lpBuffer, m.nBufferLength, m.bWatchSubtree, m.dwNotifyFilter, m.lpBytesReturned, m.lpOverlapped, m.lpCompletionRoutine)
ENDFUNC

FUNCTION  GetOverlappedResult(hFile, lpOverlapped, lpNumberOfBytesTransferred, bWait)
	DECLARE INTEGER GetOverlappedResult IN WIN32API INTEGER hFile, INTEGER lpOverlapped, INTEGER @lpNumberOfBytesTransferred, INTEGER bWait
	RETURN GetOverlappedResult(m.hFile, m.lpOverlapped, @m.lpNumberOfBytesTransferred, m.bWait)
ENDFUNC

FUNCTION CancelIo(hFile)
	DECLARE INTEGER CancelIo IN WIN32API INTEGER hFile
	RETURN CancelIo(m.hFile)
ENDFUNC

FUNCTION CancelIoEx(hFile, lpOverlapped)
	DECLARE INTEGER CancelIoEx IN WIN32API INTEGER hFile, INTEGER lpOverlapped
	RETURN CancelIoEx(m.hFile, m.lpOverlapped)
ENDFUNC