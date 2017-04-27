#INCLUDE "vfp2c.h"
#INCLUDE "vfpservice.h"

DEFINE CLASS DEV_BROADCAST_HDR AS Exception

	Address = 0
	SizeOf = 12
	Name = "DEV_BROADCAST_HDR"
	&& structure fields
	dbch_size = .F.
	dbch_devicetype = .F.
	dbch_reserved = .F.

	PROCEDURE Init(lnAddress)
		THIS.Address = lnAddress
	ENDPROC

	PROCEDURE dbch_size_Access()
		RETURN ReadUInt(THIS.Address)
	ENDPROC

	PROCEDURE dbch_size_Assign(lnNewVal)
		WriteUInt(THIS.Address,lnNewVal)
	ENDPROC

	PROCEDURE dbch_devicetype_Access()
		RETURN ReadUInt(THIS.Address+4)
	ENDPROC

	PROCEDURE dbch_devicetype_Assign(lnNewVal)
		WriteUInt(THIS.Address+4,lnNewVal)
	ENDPROC

	PROCEDURE dbch_reserved_Access()
		RETURN ReadUInt(THIS.Address+8)
	ENDPROC

	PROCEDURE dbch_reserved_Assign(lnNewVal)
		WriteUInt(THIS.Address+8,lnNewVal)
	ENDPROC

ENDDEFINE

DEFINE CLASS DEV_BROADCAST_DEVICEINTERFACE AS Exception

	Address = 0
	SizeOf = 32
	Name = "DEV_BROADCAST_DEVICEINTERFACE"
	&& structure fields
	dbcc_size = .F.
	dbcc_devicetype = .F.
	dbcc_reserved = .F.
	dbcc_classguid = .NULL.
	dbcc_name = .F.

	PROCEDURE Init(lnAddress)
		THIS.Address = m.lnAddress
		THIS.dbcc_classguid = CREATEOBJECT('GUID',THIS.Address + 12)
	ENDPROC

	PROCEDURE dbcc_size_Access()
		RETURN ReadUInt(THIS.Address)
	ENDPROC

	PROCEDURE dbcc_size_Assign(lnNewVal)
		WriteUInt(THIS.Address, m.lnNewVal)
	ENDPROC

	PROCEDURE dbcc_devicetype_Access()
		RETURN ReadUInt(THIS.Address + 4)
	ENDPROC

	PROCEDURE dbcc_devicetype_Assign(lnNewVal)
		WriteUInt(THIS.Address + 4, m.lnNewVal)
	ENDPROC

	PROCEDURE dbcc_reserved_Access()
		RETURN ReadUInt(THIS.Address + 8)
	ENDPROC

	PROCEDURE dbcc_reserved_Assign(lnNewVal)
		WriteUInt(THIS.Address + 8, m.lnNewVal)
	ENDPROC

	PROCEDURE dbcc_name_Access()
		RETURN ReadWCharArray(THIS.Address + 28, 1)
	ENDPROC

	PROCEDURE dbcc_name_Assign(lnNewVal)
		WriteWCharArray(THIS.Address + 28, m.lnNewVal, 1)
	ENDPROC

ENDDEFINE

DEFINE CLASS DEV_BROADCAST_HANDLE AS Exception

	Address = 0
	SizeOf = 42
	Name = "DEV_BROADCAST_HANDLE"
	&& structure fields
	dbch_size = .F.
	dbch_devicetype = .F.
	dbch_reserved = .F.
	dbch_handle = .F.
	dbch_hdevnotify = .F.
	dbch_eventguid = .NULL.
	dbch_nameoffset = .F.
	DIMENSION dbch_data[1]

	PROCEDURE Init(lnAddress)
		THIS.Address = m.lnAddress
		THIS.dbch_eventguid = CREATEOBJECT('GUID',THIS.Address + 20)
	ENDPROC

	PROCEDURE dbch_size_Access()
		RETURN ReadUInt(THIS.Address)
	ENDPROC

	PROCEDURE dbch_size_Assign(lnNewVal)
		WriteUInt(THIS.Address, m.lnNewVal)
	ENDPROC

	PROCEDURE dbch_devicetype_Access()
		RETURN ReadUInt(THIS.Address + 4)
	ENDPROC

	PROCEDURE dbch_devicetype_Assign(lnNewVal)
		WriteUInt(THIS.Address + 4, m.lnNewVal)
	ENDPROC

	PROCEDURE dbch_reserved_Access()
		RETURN ReadUInt(THIS.Address + 8)
	ENDPROC

	PROCEDURE dbch_reserved_Assign(lnNewVal)
		WriteUInt(THIS.Address + 8, m.lnNewVal)
	ENDPROC

	PROCEDURE dbch_handle_Access()
		RETURN ReadPointer(THIS.Address + 12)
	ENDPROC

	PROCEDURE dbch_handle_Assign(lnNewVal)
		WritePointer(THIS.Address + 12, m.lnNewVal)
	ENDPROC

	PROCEDURE dbch_hdevnotify_Access()
		RETURN ReadPointer(THIS.Address + 16)
	ENDPROC

	PROCEDURE dbch_hdevnotify_Assign(lnNewVal)
		WritePointer(THIS.Address + 16, m.lnNewVal)
	ENDPROC

	PROCEDURE dbch_nameoffset_Access()
		RETURN ReadInt(THIS.Address + 36)
	ENDPROC

	PROCEDURE dbch_nameoffset_Assign(lnNewVal)
		WriteInt(THIS.Address + 36, m.lnNewVal)
	ENDPROC

	PROCEDURE dbch_data_Access(lnRow)
		RETURN ReadUInt8(THIS.Address + 40 + (m.lnRow-1) * 1)
	ENDPROC

	PROCEDURE dbch_data_Assign(lnNewVal, lnRow)
		WriteUInt8(THIS.Address + 40 + (m.lnRow-1) * 1, m.lnNewVal)
	ENDPROC

ENDDEFINE

DEFINE CLASS POWERBROADCAST_SETTING AS Exception

	Address = 0
	Name = "POWERBROADCAST_SETTING"
	&& structure fields
	PowerSetting = .NULL.
	DataLength = .F.
	Data = .F.
	DataGuid = .F. && added manually to access GUID at offset of Data member
	DataDword = .F. && added manually to access DWORD at offset of Data member
	
	PROCEDURE Init(lnAddress)
		THIS.Address = lnAddress
		THIS.PowerSetting = CREATEOBJECT('GUID', THIS.Address)
		THIS.DataGuid = CREATEOBJECT('GUID', THIS.Address+20) && added manually
	ENDPROC

	PROCEDURE DataLength_Access()
		RETURN ReadUInt(THIS.Address+16)
	ENDPROC

	PROCEDURE DataDword_Access()
		RETURN ReadUInt(THIS.Address+20)
	ENDPROC

ENDDEFINE

DEFINE CLASS GUID AS Exception

	Address = 0
	SizeOf = 16
	PROTECTED Embedded
	Embedded = .F.
	Name = "GUID"
	&& structure fields
	Data1 = .F.
	Data2 = .F.
	Data3 = .F.
	Data4 = .F.
	Guid = .F. && custom member to assign and access GUID as a binary string
	GuidString = .F. && custom member to assign and access GUID as a readable string
	
	PROCEDURE Init(lnAddress)
		IF PCOUNT() = 0
			THIS.Address = AllocMem(THIS.SizeOf)
		ELSE
			THIS.Address = lnAddress
			THIS.Embedded = .T.
		ENDIF
	ENDPROC

	PROCEDURE Destroy()
		IF !THIS.Embedded
			FreeMem(THIS.Address)
		ENDIF
	ENDPROC

	PROCEDURE Data1_Access()
		RETURN ReadUInt(THIS.Address)
	ENDPROC

	PROCEDURE Data1_Assign(lnNewVal)
		WriteUInt(THIS.Address,lnNewVal)
	ENDPROC

	PROCEDURE Data2_Access()
		RETURN ReadUShort(THIS.Address+4)
	ENDPROC

	PROCEDURE Data2_Assign(lnNewVal)
		WriteUShort(THIS.Address+4,lnNewVal)
	ENDPROC

	PROCEDURE Data3_Access()
		RETURN ReadUShort(THIS.Address+6)
	ENDPROC

	PROCEDURE Data3_Assign(lnNewVal)
		WriteUShort(THIS.Address+6,lnNewVal)
	ENDPROC

	PROCEDURE Data4_Access()
		RETURN ReadCharArray(THIS.Address+8,8)
	ENDPROC

	PROCEDURE Data4_Assign(lnNewVal)
		WriteCharArray(THIS.Address+8,lnNewVal,8)
	ENDPROC

	PROCEDURE Guid_Access()
		RETURN ReadBytes(THIS.Address, 16)
	ENDPROC

	PROCEDURE Guid_Assign(lnNewVal)
		WriteBytes(THIS.Address, m.lnNewVal, 16)
	ENDPROC

	PROCEDURE GuidString_Access()
		RETURN STRINGFROMCLSID(THIS.Address)
	ENDPROC

	PROCEDURE GuidString_Assign(lnNewVal)
		LOCAL lcGuid
		m.lcGuid = CLSIDFROMSTRING(m.lnNewVal)
		WriteBytes(THIS.Address, m.lcGuid)
	ENDPROC

ENDDEFINE

DEFINE CLASS LARGE_INTEGER AS Exception

	Address = 0
	SizeOf = 8
	Name = "LARGE_INTEGER"
	&& structure fields
	LowPart = .F.
	HighPart = .F.
	QuadPart = .F.

	PROCEDURE Init(lnAddress)
		THIS.Address = lnAddress
	ENDPROC

	PROCEDURE LowPart_Access()
		RETURN ReadUInt(THIS.Address)
	ENDPROC

	PROCEDURE LowPart_Assign(lnNewVal)
		WriteUInt(THIS.Address,lnNewVal)
	ENDPROC

	PROCEDURE HighPart_Access()
		RETURN ReadInt(THIS.Address+4)
	ENDPROC

	PROCEDURE HighPart_Assign(lnNewVal)
		WriteInt(THIS.Address+4,lnNewVal)
	ENDPROC

	PROCEDURE QuadPart_Access()
		RETURN ReadInt64(THIS.Address)
	ENDPROC

	PROCEDURE QuadPart_Assign(lnNewVal)
		WriteInt64(THIS.Address,lnNewVal)
	ENDPROC

ENDDEFINE

DEFINE CLASS FILETIME AS Exception

	Address = 0
	SizeOf = 8
	PROTECTED Embedded	
	Embedded = .F.
	&& structure fields
	dwLowDateTime = .F.
	dwHighDateTime = .F.
	dwQuadPart = .F.
	&& additional properties to convert the filetime structure to/from a VFP datetime 
	mDate = .F.
	mUTCDate = .F.
	
	PROCEDURE Init(lnAddress)
		IF PCOUNT() = 0
			THIS.Address = AllocMem(THIS.SizeOf)
		ELSE
			ASSERT VARTYPE(m.lnAddress) = 'N' AND m.lnAddress != 0 MESSAGE 'Address of structure must be specified!'
			THIS.Address = m.lnAddress
			THIS.Embedded = .T.
		ENDIF
	ENDPROC
	
	PROCEDURE Destroy()
		IF !THIS.Embedded
			FreeMem(THIS.Address)
		ENDIF
	ENDPROC

	PROCEDURE dwLowDateTime_Access()
		RETURN ReadUInt(THIS.Address)
	ENDPROC

	PROCEDURE dwLowDateTime_Assign(lnNewVal)
		WriteUInt(THIS.Address, m.lnNewVal)
	ENDPROC

	PROCEDURE dwHighDateTime_Access()
		RETURN ReadUInt(THIS.Address + 4)
	ENDPROC

	PROCEDURE dwHighDateTime_Assign(lnNewVal)
		WriteUInt(THIS.Address + 4, m.lnNewVal)
	ENDPROC

	PROCEDURE dwQuadPart_Access()
		RETURN ReadUInt64(THIS.Address)
	ENDPROC

	PROCEDURE dwQuadPart_Assign(lnNewVal)
		WriteUInt64(THIS.Address, m.lnNewVal)
	ENDPROC
	
	PROCEDURE mDate_Access()
		RETURN FT2DT(THIS.Address)
	ENDPROC

	PROCEDURE mDate_Assign(lnNewVal)
		DT2FT(m.lnNewVal, THIS.Address)
	ENDPROC

	PROCEDURE mUTCDate_Access()
		RETURN FT2DT(THIS.Address, .T.)
	ENDPROC

	PROCEDURE mUTCDate_Assign(lnNewVal)
		DT2FT(lnNewVal,THIS.Address, .T.)
	ENDPROC

ENDDEFINE
