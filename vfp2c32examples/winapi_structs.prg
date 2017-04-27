DEFINE CLASS SYSTEMTIME AS Exception

	Address = 0
	SizeOf = 16
	PROTECTED Embedded	
	Embedded = .F.
	&& structure fields
	wYear = .F.
	wMonth = .F.
	wDayOfWeek = .F.
	wDay = .F.
	wHour = .F.
	wMinute = .F.
	wSecond = .F.
	wMilliseconds = .F.
	&& additional properties to convert the systemtime structure to/from a VFP datetime 
	mDate = .F.
	mUTCDate = .F.
	
	PROCEDURE Init(lnAddress)
		IF PCOUNT() = 0
			THIS.Address = AllocMem(THIS.SizeOf)
		ELSE
			ASSERT VARTYPE(lnAddress) = 'N' AND lnAddress != 0 MESSAGE 'Address of structure must be specified!'
			THIS.Address = lnAddress
			THIS.Embedded = .T.
		ENDIF
	ENDPROC
	
	PROCEDURE Destroy()
		IF !THIS.Embedded
			FreeMem(THIS.Address)
		ENDIF
	ENDPROC

	PROCEDURE wYear_Access()
		RETURN ReadUShort(THIS.Address)
	ENDPROC

	PROCEDURE wYear_Assign(lnNewVal)
		WriteUShort(THIS.Address,lnNewVal)
	ENDPROC

	PROCEDURE wMonth_Access()
		RETURN ReadUShort(THIS.Address+2)
	ENDPROC

	PROCEDURE wMonth_Assign(lnNewVal)
		WriteUShort(THIS.Address+2,lnNewVal)
	ENDPROC

	PROCEDURE wDayOfWeek_Access()
		RETURN ReadUShort(THIS.Address+4)
	ENDPROC

	PROCEDURE wDayOfWeek_Assign(lnNewVal)
		WriteUShort(THIS.Address+4,lnNewVal)
	ENDPROC

	PROCEDURE wDay_Access()
		RETURN ReadUShort(THIS.Address+6)
	ENDPROC

	PROCEDURE wDay_Assign(lnNewVal)
		WriteUShort(THIS.Address+6,lnNewVal)
	ENDPROC

	PROCEDURE wHour_Access()
		RETURN ReadUShort(THIS.Address+8)
	ENDPROC

	PROCEDURE wHour_Assign(lnNewVal)
		WriteUShort(THIS.Address+8,lnNewVal)
	ENDPROC

	PROCEDURE wMinute_Access()
		RETURN ReadUShort(THIS.Address+10)
	ENDPROC

	PROCEDURE wMinute_Assign(lnNewVal)
		WriteUShort(THIS.Address+10,lnNewVal)
	ENDPROC

	PROCEDURE wSecond_Access()
		RETURN ReadUShort(THIS.Address+12)
	ENDPROC

	PROCEDURE wSecond_Assign(lnNewVal)
		WriteUShort(THIS.Address+12,lnNewVal)
	ENDPROC

	PROCEDURE wMilliseconds_Access()
		RETURN ReadUShort(THIS.Address+14)
	ENDPROC

	PROCEDURE wMilliseconds_Assign(lnNewVal)
		WriteUShort(THIS.Address+14,lnNewVal)
	ENDPROC
	
	PROCEDURE mDate_Access()
		RETURN ST2DT(THIS.Address)
	ENDPROC

	PROCEDURE mDate_Assign(lnNewVal)
		DT2ST(lnNewVal,THIS.Address)
	ENDPROC

	PROCEDURE mUTCDate_Access()
		RETURN ST2DT(THIS.Address,.T.)
	ENDPROC

	PROCEDURE mUTCDate_Assign(lnNewVal)
		DT2ST(lnNewVal,THIS.Address,.T.)
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
	&& additional properties to convert the filetime structure to/from a VFP datetime 
	mDate = .F.
	mUTCDate = .F.
	
	PROCEDURE Init(lnAddress)
		IF PCOUNT() = 0
			THIS.Address = AllocMem(THIS.SizeOf)
		ELSE
			ASSERT VARTYPE(lnAddress) = 'N' AND lnAddress != 0 MESSAGE 'Address of structure must be specified!'
			THIS.Address = lnAddress
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
		WriteUInt(THIS.Address,lnNewVal)
	ENDPROC

	PROCEDURE dwHighDateTime_Access()
		RETURN ReadUInt(THIS.Address+4)
	ENDPROC

	PROCEDURE dwHighDateTime_Assign(lnNewVal)
		WriteUInt(THIS.Address+4,lnNewVal)
	ENDPROC
	
	PROCEDURE mDate_Access()
		RETURN FT2DT(THIS.Address)
	ENDPROC

	PROCEDURE mDate_Assign(lnNewVal)
		DT2FT(lnNewVal,THIS.Address)
	ENDPROC

	PROCEDURE mUTCDate_Access()
		RETURN FT2DT(THIS.Address,.T.)
	ENDPROC

	PROCEDURE mUTCDate_Assign(lnNewVal)
		DT2FT(lnNewVal,THIS.Address,.T.)
	ENDPROC

ENDDEFINE

DEFINE CLASS POINT AS Exception

	Address = 0
	SizeOf = 8
	PROTECTED Embedded
	Embedded = .F.
	&& structure fields
	x = .F.
	y = .F.

	PROCEDURE Init(lnAddress)
		IF PCOUNT() = 0
			THIS.Address = AllocMem(THIS.SizeOf)
		ELSE
			ASSERT TYPE('lnAddress') = 'N' AND lnAddress != 0 MESSAGE 'Address of structure must be specified!'
			THIS.Address = lnAddress
			THIS.Embedded = .T.
		ENDIF
	ENDPROC
	
	PROCEDURE Destroy()
		IF !THIS.Embedded
			FreeMem(THIS.Address)
		ENDIF
	ENDPROC

	PROCEDURE x_Access()
		RETURN ReadInt(THIS.Address)
	ENDPROC

	PROCEDURE x_Assign(lnNewVal)
		WriteInt(THIS.Address,lnNewVal)
	ENDPROC

	PROCEDURE y_Access()
		RETURN ReadInt(THIS.Address+4)
	ENDPROC

	PROCEDURE y_Assign(lnNewVal)
		WriteInt(THIS.Address+4,lnNewVal)
	ENDPROC

ENDDEFINE

DEFINE CLASS SIZE AS Exception

	Address = 0
	SIZEOf = 8
	PROTECTED Embedded
	Embedded = .F.
	&& structure fields
	cx = .F.
	cy = .F.

	PROCEDURE Init(lnAddress)
		IF PCOUNT() = 0
			THIS.Address = AllocMem(THIS.SizeOf)
		ELSE
			ASSERT TYPE('lnAddress') = 'N' AND lnAddress != 0 MESSAGE 'Address of structure must be specified!'
			THIS.Address = lnAddress
			THIS.Embedded = .T.
		ENDIF
	ENDPROC
	
	PROCEDURE Destroy()
		IF !THIS.Embedded
			FreeMem(THIS.Address)
		ENDIF
	ENDPROC

	PROCEDURE cx_Access()
		RETURN ReadInt(THIS.Address)
	ENDPROC

	PROCEDURE cx_Assign(lnNewVal)
		WriteInt(THIS.Address,lnNewVal)
	ENDPROC

	PROCEDURE cy_Access()
		RETURN ReadInt(THIS.Address+4)
	ENDPROC

	PROCEDURE cy_Assign(lnNewVal)
		WriteInt(THIS.Address+4,lnNewVal)
	ENDPROC

ENDDEFINE

DEFINE CLASS RECT AS Exception
	
	Address = 0 
	SizeOf = 16
	PROTECTED Embedded	
	Embedded = .F.
	mLeft = 0
	mTop = 0
	mRigth = 0
	mBottom = 0
	
	PROCEDURE Init(lnAddress)
		IF PCOUNT() = 0
			THIS.Address = AllocMem(THIS.SizeOf)
		ELSE
			ASSERT TYPE('lnAddress') = 'N' AND lnAddress != 0 MESSAGE 'Address of structure must be specified!'
			THIS.Address = lnAddress
			THIS.Embedded = .T.
		ENDIF
	ENDFUNC
	
	PROCEDURE Destroy
		IF !THIS.Embedded
			FreeMem(THIS.Address)
		ENDIF
	ENDPROC
	
	PROCEDURE mLeft_Access
		RETURN ReadInt(THIS.Address)
	ENDPROC
	
	PROCEDURE mLeft_Assign(lnNewVal)
		WriteInt(THIS.Address,lnNewVal)
	ENDPROC
	
	PROCEDURE mTop_Access
		RETURN ReadInt(THIS.Address+4)
	ENDPROC
	
	PROCEDURE mTop_Assign(lnNewVal)
		WriteInt(THIS.Address+4,lnNewVal)
	ENDPROC

	PROCEDURE mRigth_Access
		RETURN ReadInt(THIS.Address+8)
	ENDPROC
	
	PROCEDURE mRigth_Assign(lnNewVal)
		WriteInt(THIS.Address+8,lnNewVal)			
	ENDPROC

	PROCEDURE mBottom_Access
		RETURN ReadInt(THIS.Address+12)
	ENDPROC
	
	PROCEDURE mBottom_Assign(lnNewVal)
		WriteInt(THIS.Address+12,lnNewVal)						
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