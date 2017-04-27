&& declare needed functions
DECLARE INTEGER GetNetworkParams IN iphlpapi.dll INTEGER pFixedInfo, INTEGER @ dwOutBufLen

DECLARE INTEGER GetIpAddrTable IN iphlpapi.dll INTEGER pIpAddrTable, INTEGER @ pdwSize, INTEGER bOrder
DECLARE INTEGER GetIpNetTable IN iphlpapi.dll INTEGER pIpNetTable, INTEGER @ pdwSize, INTEGER bOrder

DECLARE INTEGER GetTcpStatistics IN iphlpapi.dll INTEGER pStats
DECLARE INTEGER GetUdpStatistics IN iphlpapi.dll INTEGER pStats
DECLARE INTEGER GetIpStatistics IN iphlpapi.dll INTEGER pStats
DECLARE INTEGER GetIcmpStatistics IN iphlpapi.dll INTEGER pStats

DECLARE INTEGER GetTcpTable IN iphlpapi.dll INTEGER pTcpTable, INTEGER @ pdwSize, INTEGER bOrder
DECLARE INTEGER GetUdpTable IN iphlpapi.dll INTEGER pUdpTable, INTEGER @ pdwSize, INTEGER bOrder
DECLARE INTEGER SetTcpEntry IN iphlpapi.dll INTEGER pTcpRow

DECLARE INTEGER GetNumberOfInterfaces IN iphlpapi.dll INTEGER @ pdwNumIf
DECLARE INTEGER GetIfTable IN iphlpapi.dll INTEGER pIfTable, INTEGER @ pdwSize, INTEGER bOrder
DECLARE INTEGER GetInterfaceInfo IN iphlpapi.dll INTEGER pIfTable, INTEGER @ dwOutBufLen
DECLARE INTEGER GetIfEntry IN iphlpapi.dll INTEGER pIfRow

DECLARE INTEGER GetAdaptersAddresses IN iphlpapi.dll INTEGER Family, INTEGER Flags, INTEGER Reserved, ;
									INTEGER pAdapterAddresses, INTEGER @ pOutBufLen
DECLARE INTEGER GetAdaptersInfo IN iphlpapi.dll INTEGER pAdapterInfo, INTEGER @ dwOutBufLen

&& make struct wrapper classes below visible
SET PROCEDURE TO iptypes.prg ADDITIVE

DEFINE CLASS FIXED_INFO AS Relation

	Address = 0
	SizeOf = 584
	BufferSize = 16384
	&& structure fields
	HostName = .F.
	DomainName = .F.
	CurrentDnsServer = .NULL.
	DnsServerList = .NULL.
	NodeType = .F.
	ScopeId = .F.
	EnableRouting = .F.
	EnableProxy = .F.
	EnableDns = .F.

	PROCEDURE Init()
		THIS.Address = AllocMem(THIS.BufferSize)
		IF THIS.Address = 0
			ERROR(43)
			RETURN .F.
		ENDIF
		THIS.DnsServerList = CREATEOBJECT('IP_ADDR_STRING',THIS.Address+268)
	ENDPROC

	PROCEDURE Destroy()
		THIS.DnsServerList = .NULL.
		FreeMem(THIS.Address)
	ENDPROC

	PROCEDURE Address_Assign(lnAddress)
		DO CASE
			CASE THIS.Address = 0
				THIS.Address = lnAddress
			CASE THIS.Address = lnAddress
			OTHERWISE
				THIS.Address = lnAddress
				THIS.DnsServerList.Address = lnAddress+268
		ENDCASE
	ENDPROC

	PROCEDURE BufferSize_Assign(nBufferSize)
		LOCAL lnAddress
		lnAddress = ReAllocMem(THIS.Address,nBufferSize)
		IF lnAddress != 0
			THIS.Address = lnAddress
		ELSE
			ERROR(43)
		ENDIF
	ENDPROC

	PROCEDURE HostName_Access()
		RETURN ReadCharArray(THIS.Address,132)
	ENDPROC

	PROCEDURE DomainName_Access()
		RETURN ReadCharArray(THIS.Address+132,132)
	ENDPROC

	PROCEDURE CurrentDnsServer_Access()
		RETURN ReadPointer(THIS.Address+264)
	ENDPROC

	PROCEDURE NodeType_Access()
		RETURN ReadUInt(THIS.Address+308)
	ENDPROC

	PROCEDURE ScopeId_Access()
		RETURN ReadCharArray(THIS.Address+312,260)
	ENDPROC

	PROCEDURE EnableRouting_Access()
		RETURN ReadLogical(THIS.Address+572)
		&& changed manually to ReadLogical cause it represents a boolen value
		&& and we can use ReadLogical anywhere a 32 bit value represents a boolean value where 0 means .F. and any other value .T.
	ENDPROC

	PROCEDURE EnableProxy_Access()
		RETURN ReadLogical(THIS.Address+576)
		&& changed manually to ReadLogical 
	ENDPROC

	PROCEDURE EnableDns_Access()
		RETURN ReadLogical(THIS.Address+580)
		&& changed manually to ReadLogical 
	ENDPROC

ENDDEFINE

DEFINE CLASS IP_ADAPTER_INFO AS Relation

	Address = 0
	AddrUpdate = 0
	SizeOf = 640
	BufferSize = 32768
	PROTECTED Embedded
	Embedded = .F.
	&& structure fields
	Next = .NULL.
	ComboIndex = .F.
	AdapterName = .F.
	Description = .F.
	AddressLength = .F.
	mAddress = .F.
	Index = .F.
	Type = .F.
	DhcpEnabled = .F.
	CurrentIpAddress = .NULL.
	IpAddressList = .NULL.
	GatewayList = .NULL.
	DhcpServer = .NULL.
	HaveWins = .F.
	PrimaryWinsServer = .NULL.
	SecondaryWinsServer = .NULL.
	LeaseObtained = .F.
	LeaseExpires = .F.

	PROCEDURE Init(lnAddress)
		IF PCOUNT() = 0
			THIS.Address = AllocMem(THIS.BufferSize)
			IF THIS.Address = 0
				ERROR(43)
				RETURN .F.
			ENDIF
		ELSE
			THIS.Address = lnAddress
			THIS.Embedded = .T.
		ENDIF
		THIS.IpAddressList = CREATEOBJECT('IP_ADDR_STRING',THIS.Address+428)
		THIS.GatewayList = CREATEOBJECT('IP_ADDR_STRING',THIS.Address+468)
		THIS.DhcpServer = CREATEOBJECT('IP_ADDR_STRING',THIS.Address+508)
		THIS.PrimaryWinsServer = CREATEOBJECT('IP_ADDR_STRING',THIS.Address+552)
		THIS.SecondaryWinsServer = CREATEOBJECT('IP_ADDR_STRING',THIS.Address+592)
	ENDPROC

	PROCEDURE Destroy()
		THIS.IpAddressList = .NULL.
		THIS.GatewayList = .NULL.
		THIS.DhcpServer = .NULL.
		THIS.PrimaryWinsServer = .NULL.
		THIS.SecondaryWinsServer = .NULL.
		IF !THIS.Embedded
			FreeMem(THIS.Address)
		ENDIF
	ENDPROC

	PROCEDURE Address_Assign(lnAddress)
		DO CASE
			CASE THIS.Address = 0
				THIS.Address = lnAddress
			CASE THIS.Address = lnAddress
			OTHERWISE
				THIS.Address = lnAddress
				THIS.IpAddressList.Address = lnAddress+428
				THIS.GatewayList.Address = lnAddress+468
				THIS.DhcpServer.Address = lnAddress+508
				THIS.PrimaryWinsServer.Address = lnAddress+552
				THIS.SecondaryWinsServer.Address = lnAddress+592
		ENDCASE
	ENDPROC

	PROCEDURE BufferSize_Assign(nBufferSize)
		LOCAL lnAddress
		lnAddress = ReAllocMem(THIS.Address,nBufferSize)
		IF lnAddress != 0
			THIS.Address = lnAddress
		ELSE
			ERROR(43)
		ENDIF
	ENDPROC

	PROCEDURE Next_Access()
		RETURN ReadPointer(THIS.Address)
	ENDPROC

	PROCEDURE ComboIndex_Access()
		RETURN ReadUInt(THIS.Address+4)
	ENDPROC

	PROCEDURE AdapterName_Access()
		RETURN ReadCharArray(THIS.Address+8,260)
	ENDPROC

	PROCEDURE Description_Access()
		RETURN ReadCharArray(THIS.Address+268,132)
	ENDPROC

	PROCEDURE AddressLength_Access()
		RETURN ReadUInt(THIS.Address+400)
	ENDPROC

	PROCEDURE mAddress_Access()
		RETURN ReadInt(THIS.Address+404)
	ENDPROC

	PROCEDURE Index_Access()
		RETURN ReadUInt(THIS.Address+412)
	ENDPROC

	PROCEDURE Type_Access()
		RETURN ReadUInt(THIS.Address+416)
	ENDPROC

	PROCEDURE DhcpEnabled_Access()
		RETURN ReadLogical(THIS.Address+420)
	ENDPROC

	PROCEDURE CurrentIpAddress_Access()
		RETURN ReadPointer(THIS.Address+424)
	ENDPROC

	PROCEDURE HaveWins_Access()
		RETURN ReadLogical(THIS.Address+548)
	ENDPROC

	PROCEDURE LeaseObtained_Access()
		RETURN ReadInt(THIS.Address+632)
	ENDPROC

	PROCEDURE LeaseExpires_Access()
		RETURN ReadInt(THIS.Address+636)
	ENDPROC

ENDDEFINE

DEFINE CLASS IP_INTERFACE_INFO AS Relation

	Address = 0
	SizeOf = 8
	BufferSize = 4096
	&& structure fields
	NumAdapters = .F.
	DIMENSION Adapter[1]

	PROCEDURE Init()
		THIS.Address = AllocMem(THIS.BufferSize)
		IF THIS.Address = 0
			ERROR(43)
			RETURN .F.
		ENDIF
		THIS.Adapter[1] = CREATEOBJECT('IP_ADAPTER_INDEX_MAP',THIS.Address+4)
	ENDPROC

	PROCEDURE Destroy()
		FreeMem(THIS.Address)
	ENDPROC

	PROCEDURE Address_Assign(lnAddress)
		DO CASE
			CASE THIS.Address = 0
				THIS.Address = lnAddress
			CASE THIS.Address = lnAddress
			OTHERWISE
				THIS.Address = lnAddress
				THIS.Adapter[1].Address = lnAddress+4
		ENDCASE
	ENDFUNC

	PROCEDURE BufferSize_Assign(nBufferSize)
		LOCAL lnAddress
		lnAddress = ReAllocMem(THIS.Address,nBufferSize)
		IF lnAddress != 0
			THIS.Address = lnAddress
		ELSE
			ERROR(43)
		ENDIF
	ENDPROC
	
	PROCEDURE NumAdapters_Access()
		RETURN ReadInt(THIS.Address)
	ENDPROC

	PROCEDURE Adapter_Access(lnRow)
		THIS.Adapter[1].Address = THIS.Address+4+(lnRow-1)*260
		RETURN THIS.Adapter[1]
	ENDPROC
	
ENDDEFINE


DEFINE CLASS IP_ADAPTER_INDEX_MAP AS Relation

	Address = 0
	SizeOf = 260
	PROTECTED Embedded	
	Embedded = .F.
	&& structure fields
	Index = .F.
	mName = ""

	PROCEDURE Init(lnAddress)
		IF PCOUNT() = 0
			THIS.Address = AllocMem(THIS.SizeOf)
			IF THIS.Address = 0
				ERROR(43)
				RETURN .F.
			ENDIF
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

	PROCEDURE Index_Access()
		RETURN ReadUInt(THIS.Address)
	ENDPROC

	PROCEDURE Index_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address,lnNewVal)
	ENDPROC

	PROCEDURE mName_Access()
		RETURN ReadWCharArray(THIS.Address+4,128)
	ENDPROC

	PROCEDURE mName_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'C' MESSAGE 'Wrong datatype or value out of range!'
		WriteWCharArray(THIS.Address+4,lnNewVal,128)
	ENDPROC

ENDDEFINE

DEFINE CLASS IP_ADDRESS_STRING AS Relation

	Address = 0
	SizeOf = 16
	&& structure fields
	String = .F.

	PROCEDURE Init(lnAddress)
		ASSERT TYPE('lnAddress') = 'N' AND lnAddress != 0 MESSAGE 'Invalid structure address!'
		THIS.Address = lnAddress
	ENDPROC

	PROCEDURE String_Access()
		RETURN ReadCharArray(THIS.Address,16)
	ENDPROC

	PROCEDURE String_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'C' MESSAGE 'Wrong datatype or value out of range!'
		WriteCharArray(THIS.Address,lnNewVal,16)
	ENDPROC

ENDDEFINE

DEFINE CLASS IP_ADDR_STRING AS Relation

	Address = 0
	SizeOf = 40
	PROTECTED Embedded	
	Embedded = .F.
	&& structure fields
	Next = .NULL.
	IpAddress = .NULL.
	IpMask = .NULL.
	Context = .F.

	PROCEDURE Init(lnAddress)
		IF PCOUNT() = 0
			THIS.Address = AllocMem(THIS.SizeOf)
			IF THIS.Address = 0
				ERROR(43)
				RETURN .F.
			ENDIF
		ELSE
			ASSERT TYPE('lnAddress') = 'N' AND lnAddress != 0 MESSAGE 'Invalid structure address!'
			THIS.Address = lnAddress
			THIS.Embedded = .T.
		ENDIF
		THIS.IpAddress = CREATEOBJECT('IP_ADDRESS_STRING',THIS.Address+4)
		THIS.IpMask = CREATEOBJECT('IP_ADDRESS_STRING',THIS.Address+20)
	ENDPROC

	PROCEDURE Destroy()
		THIS.IpAddress = .NULL.
		THIS.IpMask = .NULL.
		IF !THIS.Embedded
			FreeMem(THIS.Address)
		ENDIF
	ENDPROC

	PROCEDURE Address_Assign(lnAddress)
		DO CASE
			CASE THIS.Address = 0
				THIS.Address = lnAddress
			CASE THIS.Address = lnAddress
			OTHERWISE
				THIS.Address = lnAddress
				THIS.IpAddress.Address = lnAddress+4
				THIS.IpMask.Address = lnAddress+20
		ENDCASE
	ENDPROC

	PROCEDURE Next_Access()
		RETURN ReadPointer(THIS.Address)
	ENDPROC

	PROCEDURE Context_Access()
		RETURN ReadUInt(THIS.Address+36)
	ENDPROC

	PROCEDURE Context_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+36,lnNewVal)
	ENDPROC

ENDDEFINE

DEFINE CLASS MIB_IFTABLE AS Relation

	Address = 0
	SizeOf = 864
	BufferSize = 16384
	&& structure fields
	dwNumEntries = .F.
	DIMENSION mtable[1]

	PROCEDURE Init()
		THIS.Address = AllocMem(THIS.BufferSize)
		IF THIS.Address = 0
			ERROR(43)
			RETURN .F.
		ENDIF
		THIS.mtable[1] = CREATEOBJECT('MIB_IFROW',THIS.Address+4)
	ENDPROC

	PROCEDURE Destroy()
		THIS.mtable[1] = .NULL.
		FreeMem(THIS.Address)
	ENDPROC

	PROCEDURE Address_Assign(lnAddress)
		DO CASE
			CASE THIS.Address = 0
				THIS.Address = lnAddress
			CASE THIS.Address = lnAddress
			OTHERWISE
				THIS.Address = lnAddress
				THIS.mtable[1].Address = lnAddress+4
		ENDCASE
	ENDPROC

	PROCEDURE BufferSize_Assign(nBufferSize)
		LOCAL lnAddress
		lnAddress = ReAllocMem(THIS.Address,nBufferSize)
		IF lnAddress != 0
			THIS.Address = lnAddress
		ELSE
			ERROR(43)
		ENDIF
	ENDPROC

	PROCEDURE dwNumEntries_Access()
		RETURN ReadUInt(THIS.Address)
	ENDPROC

	PROCEDURE mtable_Access(lnRow)
		ASSERT TYPE('lnRow') = 'N' AND lnRow > 0 MESSAGE 'Invalid row subscript!'
		THIS.mtable[1].Address = THIS.Address-856+lnRow*860
		RETURN THIS.mtable[1]
	ENDPROC

ENDDEFINE

DEFINE CLASS MIB_IFROW AS Relation

	Address = 0
	SizeOf = 860
	PROTECTED Embedded
	Embedded = .F.
	&& structure fields
	wszName = .F.
	dwIndex = .F.
	dwType = .F.
	dwMtu = .F.
	dwSpeed = .F.
	dwPhysAddrLen = .F.
	bPhysAddr = .F.
	dwAdminStatus = .F.
	dwOperStatus = .F.
	dwLastChange = .F.
	dwInOctets = .F.
	dwInUcastPkts = .F.
	dwInNUcastPkts = .F.
	dwInDiscards = .F.
	dwInErrors = .F.
	dwInUnknownProtos = .F.
	dwOutOctets = .F.
	dwOutUcastPkts = .F.
	dwOutNUcastPkts = .F.
	dwOutDiscards = .F.
	dwOutErrors = .F.
	dwOutQLen = .F.
	dwDescrLen = .F.
	bDescr = .F.

	PROCEDURE Init(lnAddress)
		IF PCOUNT() = 0
			THIS.Address = AllocMem(THIS.SizeOf)
			IF THIS.Address = 0
				ERROR(43)
				RETURN .F.
			ENDIF
		ELSE
			ASSERT TYPE('lnAddress') = 'N' AND lnAddress != 0 MESSAGE 'Invalid structure address!'
			THIS.Address = lnAddress
			THIS.Embedded = .T.
		ENDIF
	ENDPROC

	PROCEDURE Destroy()
		IF !THIS.Embedded
			FreeMem(THIS.Address)
		ENDIF
	ENDPROC

	PROCEDURE wszName_Access()
		RETURN ReadWCharArray(THIS.Address,256)
	ENDPROC

	PROCEDURE wszName_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'C' MESSAGE 'Wrong datatype or value out of range!'
		WriteWCharArray(THIS.Address,lnNewVal,256)
	ENDPROC

	PROCEDURE dwIndex_Access()
		RETURN ReadUInt(THIS.Address+512)
	ENDPROC

	PROCEDURE dwIndex_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+512,lnNewVal)
	ENDPROC

	PROCEDURE dwType_Access()
		RETURN ReadUInt(THIS.Address+516)
	ENDPROC

	PROCEDURE dwType_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+516,lnNewVal)
	ENDPROC

	PROCEDURE dwMtu_Access()
		RETURN ReadUInt(THIS.Address+520)
	ENDPROC

	PROCEDURE dwMtu_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+520,lnNewVal)
	ENDPROC

	PROCEDURE dwSpeed_Access()
		RETURN ReadUInt(THIS.Address+524)
	ENDPROC

	PROCEDURE dwSpeed_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+524,lnNewVal)
	ENDPROC

	PROCEDURE dwPhysAddrLen_Access()
		RETURN ReadUInt(THIS.Address+528)
	ENDPROC

	PROCEDURE dwPhysAddrLen_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+528,lnNewVal)
	ENDPROC

	PROCEDURE bPhysAddr_Access()
		RETURN BinMacToMac(ReadBytes(THIS.Address+532,THIS.dwPhysAddrLen))
	ENDPROC

	PROCEDURE bPhysAddr_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'C' MESSAGE 'Wrong datatype or value out of range!'
		WriteBytes(THIS.Address+532,lnNewVal)
	ENDPROC

	PROCEDURE dwAdminStatus_Access()
		RETURN ReadLogical(THIS.Address+540)
	ENDPROC

	PROCEDURE dwAdminStatus_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteLogical(THIS.Address+540,lnNewVal)
	ENDPROC

	PROCEDURE dwOperStatus_Access()
		RETURN ReadUInt(THIS.Address+544)
	ENDPROC

	PROCEDURE dwOperStatus_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+544,lnNewVal)
	ENDPROC

	PROCEDURE dwLastChange_Access()
		RETURN ReadUInt(THIS.Address+548)
	ENDPROC

	PROCEDURE dwLastChange_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+548,lnNewVal)
	ENDPROC

	PROCEDURE dwInOctets_Access()
		RETURN ReadUInt(THIS.Address+552)
	ENDPROC

	PROCEDURE dwInOctets_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+552,lnNewVal)
	ENDPROC

	PROCEDURE dwInUcastPkts_Access()
		RETURN ReadUInt(THIS.Address+556)
	ENDPROC

	PROCEDURE dwInUcastPkts_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+556,lnNewVal)
	ENDPROC

	PROCEDURE dwInNUcastPkts_Access()
		RETURN ReadUInt(THIS.Address+560)
	ENDPROC

	PROCEDURE dwInNUcastPkts_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+560,lnNewVal)
	ENDPROC

	PROCEDURE dwInDiscards_Access()
		RETURN ReadUInt(THIS.Address+564)
	ENDPROC

	PROCEDURE dwInDiscards_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+564,lnNewVal)
	ENDPROC

	PROCEDURE dwInErrors_Access()
		RETURN ReadUInt(THIS.Address+568)
	ENDPROC

	PROCEDURE dwInErrors_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+568,lnNewVal)
	ENDPROC

	PROCEDURE dwInUnknownProtos_Access()
		RETURN ReadUInt(THIS.Address+572)
	ENDPROC

	PROCEDURE dwInUnknownProtos_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+572,lnNewVal)
	ENDPROC

	PROCEDURE dwOutOctets_Access()
		RETURN ReadUInt(THIS.Address+576)
	ENDPROC

	PROCEDURE dwOutOctets_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+576,lnNewVal)
	ENDPROC

	PROCEDURE dwOutUcastPkts_Access()
		RETURN ReadUInt(THIS.Address+580)
	ENDPROC

	PROCEDURE dwOutUcastPkts_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+580,lnNewVal)
	ENDPROC

	PROCEDURE dwOutNUcastPkts_Access()
		RETURN ReadUInt(THIS.Address+584)
	ENDPROC

	PROCEDURE dwOutNUcastPkts_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+584,lnNewVal)
	ENDPROC

	PROCEDURE dwOutDiscards_Access()
		RETURN ReadUInt(THIS.Address+588)
	ENDPROC

	PROCEDURE dwOutDiscards_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+588,lnNewVal)
	ENDPROC

	PROCEDURE dwOutErrors_Access()
		RETURN ReadUInt(THIS.Address+592)
	ENDPROC

	PROCEDURE dwOutErrors_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+592,lnNewVal)
	ENDPROC

	PROCEDURE dwOutQLen_Access()
		RETURN ReadUInt(THIS.Address+596)
	ENDPROC

	PROCEDURE dwOutQLen_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+596,lnNewVal)
	ENDPROC

	PROCEDURE dwDescrLen_Access()
		RETURN ReadUInt(THIS.Address+600)
	ENDPROC

	PROCEDURE dwDescrLen_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+600,lnNewVal)
	ENDPROC

	PROCEDURE bDescr_Access()
		RETURN ReadCharArray(THIS.Address+604,256)
	ENDPROC

	PROCEDURE bDescr_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'C' MESSAGE 'Wrong datatype or value out of range!'
		WriteCharArray(THIS.Address+604,lnNewVal,256)
	ENDPROC

ENDDEFINE

DEFINE CLASS MIB_IPADDRTABLE AS Relation

	Address = 0
	SizeOf = 28
	BufferSize = 16384
	&& structure fields
	dwNumEntries = .F.
	DIMENSION mtable[1]

	PROCEDURE Init()
		THIS.Address = AllocMem(THIS.BufferSize)
		IF THIS.Address = 0
			ERROR(43)
			RETURN .F.
		ENDIF
		THIS.mtable[1] = CREATEOBJECT('MIB_IPADDRROW',THIS.Address+4)
	ENDPROC

	PROCEDURE Destroy()
		THIS.mtable[1] = .NULL.
		FreeMem(THIS.Address)
	ENDPROC

	PROCEDURE Address_Assign(lnAddress)
		DO CASE
			CASE THIS.Address = 0
				THIS.Address = lnAddress
			CASE THIS.Address = lnAddress
			OTHERWISE
				THIS.Address = lnAddress
				THIS.mtable[1].Address = lnAddress+4
		ENDCASE
	ENDPROC

	PROCEDURE BufferSize_Assign(nBufferSize)
		LOCAL lnAddress
		lnAddress = ReAllocMem(THIS.Address,nBufferSize)
		IF lnAddress != 0
			THIS.Address = lnAddress
		ELSE
			ERROR(43)
		ENDIF
	ENDPROC

	PROCEDURE dwNumEntries_Access()
		RETURN ReadUInt(THIS.Address)
	ENDPROC

	PROCEDURE mtable_Access(lnRow)
		THIS.mtable[1].Address = THIS.Address-20+lnRow*24
		RETURN THIS.mtable[1]
	ENDPROC

ENDDEFINE

DEFINE CLASS MIB_IPADDRROW AS Relation

	Address = 0
	SizeOf = 24
	&& structure fields
	dwAddr = .F.
	dwIndex = .F.
	dwMask = .F.
	dwBCastAddr = .F.
	dwReasmSize = .F.
	unused1 = .F.
	unused2 = .F.

	PROCEDURE Init(lnAddress)
		ASSERT TYPE('lnAddress') = 'N' AND lnAddress != 0 MESSAGE 'Invalid structure address!'
		THIS.Address = lnAddress
	ENDPROC

	PROCEDURE dwAddr_Access()
		RETURN ReadUInt(THIS.Address)
	ENDPROC

	PROCEDURE dwAddr_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address,lnNewVal)
	ENDPROC

	PROCEDURE dwIndex_Access()
		RETURN ReadUInt(THIS.Address+4)
	ENDPROC

	PROCEDURE dwIndex_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+4,lnNewVal)
	ENDPROC

	PROCEDURE dwMask_Access()
		RETURN ReadUInt(THIS.Address+8)
	ENDPROC

	PROCEDURE dwMask_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+8,lnNewVal)
	ENDPROC

	PROCEDURE dwBCastAddr_Access()
		RETURN ReadUInt(THIS.Address+12)
	ENDPROC

	PROCEDURE dwBCastAddr_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+12,lnNewVal)
	ENDPROC

	PROCEDURE dwReasmSize_Access()
		RETURN ReadUInt(THIS.Address+16)
	ENDPROC

	PROCEDURE dwReasmSize_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+16,lnNewVal)
	ENDPROC

	PROCEDURE unused1_Access()
		RETURN ReadUShort(THIS.Address+20)
	ENDPROC

	PROCEDURE unused1_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,65535) MESSAGE 'Wrong datatype or value out of range!'
		WriteUShort(THIS.Address+20,lnNewVal)
	ENDPROC

	PROCEDURE unused2_Access()
		RETURN ReadUShort(THIS.Address+22)
	ENDPROC

	PROCEDURE unused2_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,65535) MESSAGE 'Wrong datatype or value out of range!'
		WriteUShort(THIS.Address+22,lnNewVal)
	ENDPROC

ENDDEFINE

DEFINE CLASS MIB_IPSTATS AS Relation

	Address = 0
	SizeOf = 92
	&& structure fields
	dwForwarding = .F.
	dwDefaultTTL = .F.
	dwInReceives = .F.
	dwInHdrErrors = .F.
	dwInAddrErrors = .F.
	dwForwDatagrams = .F.
	dwInUnknownProtos = .F.
	dwInDiscards = .F.
	dwInDelivers = .F.
	dwOutRequests = .F.
	dwRoutingDiscards = .F.
	dwOutDiscards = .F.
	dwOutNoRoutes = .F.
	dwReasmTimeout = .F.
	dwReasmReqds = .F.
	dwReasmOks = .F.
	dwReasmFails = .F.
	dwFragOks = .F.
	dwFragFails = .F.
	dwFragCreates = .F.
	dwNumIf = .F.
	dwNumAddr = .F.
	dwNumRoutes = .F.

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

	PROCEDURE dwForwarding_Access()
		RETURN ReadUInt(THIS.Address)
	ENDPROC

	PROCEDURE dwForwarding_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address,lnNewVal)
	ENDPROC

	PROCEDURE dwDefaultTTL_Access()
		RETURN ReadUInt(THIS.Address+4)
	ENDPROC

	PROCEDURE dwDefaultTTL_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+4,lnNewVal)
	ENDPROC

	PROCEDURE dwInReceives_Access()
		RETURN ReadUInt(THIS.Address+8)
	ENDPROC

	PROCEDURE dwInReceives_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+8,lnNewVal)
	ENDPROC

	PROCEDURE dwInHdrErrors_Access()
		RETURN ReadUInt(THIS.Address+12)
	ENDPROC

	PROCEDURE dwInHdrErrors_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+12,lnNewVal)
	ENDPROC

	PROCEDURE dwInAddrErrors_Access()
		RETURN ReadUInt(THIS.Address+16)
	ENDPROC

	PROCEDURE dwInAddrErrors_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+16,lnNewVal)
	ENDPROC

	PROCEDURE dwForwDatagrams_Access()
		RETURN ReadUInt(THIS.Address+20)
	ENDPROC

	PROCEDURE dwForwDatagrams_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+20,lnNewVal)
	ENDPROC

	PROCEDURE dwInUnknownProtos_Access()
		RETURN ReadUInt(THIS.Address+24)
	ENDPROC

	PROCEDURE dwInUnknownProtos_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+24,lnNewVal)
	ENDPROC

	PROCEDURE dwInDiscards_Access()
		RETURN ReadUInt(THIS.Address+28)
	ENDPROC

	PROCEDURE dwInDiscards_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+28,lnNewVal)
	ENDPROC

	PROCEDURE dwInDelivers_Access()
		RETURN ReadUInt(THIS.Address+32)
	ENDPROC

	PROCEDURE dwInDelivers_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+32,lnNewVal)
	ENDPROC

	PROCEDURE dwOutRequests_Access()
		RETURN ReadUInt(THIS.Address+36)
	ENDPROC

	PROCEDURE dwOutRequests_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+36,lnNewVal)
	ENDPROC

	PROCEDURE dwRoutingDiscards_Access()
		RETURN ReadUInt(THIS.Address+40)
	ENDPROC

	PROCEDURE dwRoutingDiscards_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+40,lnNewVal)
	ENDPROC

	PROCEDURE dwOutDiscards_Access()
		RETURN ReadUInt(THIS.Address+44)
	ENDPROC

	PROCEDURE dwOutDiscards_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+44,lnNewVal)
	ENDPROC

	PROCEDURE dwOutNoRoutes_Access()
		RETURN ReadUInt(THIS.Address+48)
	ENDPROC

	PROCEDURE dwOutNoRoutes_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+48,lnNewVal)
	ENDPROC

	PROCEDURE dwReasmTimeout_Access()
		RETURN ReadUInt(THIS.Address+52)
	ENDPROC

	PROCEDURE dwReasmTimeout_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+52,lnNewVal)
	ENDPROC

	PROCEDURE dwReasmReqds_Access()
		RETURN ReadUInt(THIS.Address+56)
	ENDPROC

	PROCEDURE dwReasmReqds_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+56,lnNewVal)
	ENDPROC

	PROCEDURE dwReasmOks_Access()
		RETURN ReadUInt(THIS.Address+60)
	ENDPROC

	PROCEDURE dwReasmOks_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+60,lnNewVal)
	ENDPROC

	PROCEDURE dwReasmFails_Access()
		RETURN ReadUInt(THIS.Address+64)
	ENDPROC

	PROCEDURE dwReasmFails_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+64,lnNewVal)
	ENDPROC

	PROCEDURE dwFragOks_Access()
		RETURN ReadUInt(THIS.Address+68)
	ENDPROC

	PROCEDURE dwFragOks_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+68,lnNewVal)
	ENDPROC

	PROCEDURE dwFragFails_Access()
		RETURN ReadUInt(THIS.Address+72)
	ENDPROC

	PROCEDURE dwFragFails_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+72,lnNewVal)
	ENDPROC

	PROCEDURE dwFragCreates_Access()
		RETURN ReadUInt(THIS.Address+76)
	ENDPROC

	PROCEDURE dwFragCreates_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+76,lnNewVal)
	ENDPROC

	PROCEDURE dwNumIf_Access()
		RETURN ReadUInt(THIS.Address+80)
	ENDPROC

	PROCEDURE dwNumIf_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+80,lnNewVal)
	ENDPROC

	PROCEDURE dwNumAddr_Access()
		RETURN ReadUInt(THIS.Address+84)
	ENDPROC

	PROCEDURE dwNumAddr_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+84,lnNewVal)
	ENDPROC

	PROCEDURE dwNumRoutes_Access()
		RETURN ReadUInt(THIS.Address+88)
	ENDPROC

	PROCEDURE dwNumRoutes_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+88,lnNewVal)
	ENDPROC

ENDDEFINE

DEFINE CLASS MIB_TCPSTATS AS Relation

	Address = 0
	SizeOf = 60
	&& structure fields
	dwRtoAlgorithm = .F.
	dwRtoMin = .F.
	dwRtoMax = .F.
	dwMaxConn = .F.
	dwActiveOpens = .F.
	dwPassiveOpens = .F.
	dwAttemptFails = .F.
	dwEstabResets = .F.
	dwCurrEstab = .F.
	dwInSegs = .F.
	dwOutSegs = .F.
	dwRetransSegs = .F.
	dwInErrs = .F.
	dwOutRsts = .F.
	dwNumConns = .F.

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

	PROCEDURE dwRtoAlgorithm_Access()
		RETURN ReadUInt(THIS.Address)
	ENDPROC

	PROCEDURE dwRtoAlgorithm_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address,lnNewVal)
	ENDPROC

	PROCEDURE dwRtoMin_Access()
		RETURN ReadUInt(THIS.Address+4)
	ENDPROC

	PROCEDURE dwRtoMin_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+4,lnNewVal)
	ENDPROC

	PROCEDURE dwRtoMax_Access()
		RETURN ReadUInt(THIS.Address+8)
	ENDPROC

	PROCEDURE dwRtoMax_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+8,lnNewVal)
	ENDPROC

	PROCEDURE dwMaxConn_Access()
		RETURN ReadUInt(THIS.Address+12)
	ENDPROC

	PROCEDURE dwMaxConn_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+12,lnNewVal)
	ENDPROC

	PROCEDURE dwActiveOpens_Access()
		RETURN ReadUInt(THIS.Address+16)
	ENDPROC

	PROCEDURE dwActiveOpens_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+16,lnNewVal)
	ENDPROC

	PROCEDURE dwPassiveOpens_Access()
		RETURN ReadUInt(THIS.Address+20)
	ENDPROC

	PROCEDURE dwPassiveOpens_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+20,lnNewVal)
	ENDPROC

	PROCEDURE dwAttemptFails_Access()
		RETURN ReadUInt(THIS.Address+24)
	ENDPROC

	PROCEDURE dwAttemptFails_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+24,lnNewVal)
	ENDPROC

	PROCEDURE dwEstabResets_Access()
		RETURN ReadUInt(THIS.Address+28)
	ENDPROC

	PROCEDURE dwEstabResets_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+28,lnNewVal)
	ENDPROC

	PROCEDURE dwCurrEstab_Access()
		RETURN ReadUInt(THIS.Address+32)
	ENDPROC

	PROCEDURE dwCurrEstab_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+32,lnNewVal)
	ENDPROC

	PROCEDURE dwInSegs_Access()
		RETURN ReadUInt(THIS.Address+36)
	ENDPROC

	PROCEDURE dwInSegs_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+36,lnNewVal)
	ENDPROC

	PROCEDURE dwOutSegs_Access()
		RETURN ReadUInt(THIS.Address+40)
	ENDPROC

	PROCEDURE dwOutSegs_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+40,lnNewVal)
	ENDPROC

	PROCEDURE dwRetransSegs_Access()
		RETURN ReadUInt(THIS.Address+44)
	ENDPROC

	PROCEDURE dwRetransSegs_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+44,lnNewVal)
	ENDPROC

	PROCEDURE dwInErrs_Access()
		RETURN ReadUInt(THIS.Address+48)
	ENDPROC

	PROCEDURE dwInErrs_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+48,lnNewVal)
	ENDPROC

	PROCEDURE dwOutRsts_Access()
		RETURN ReadUInt(THIS.Address+52)
	ENDPROC

	PROCEDURE dwOutRsts_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+52,lnNewVal)
	ENDPROC

	PROCEDURE dwNumConns_Access()
		RETURN ReadUInt(THIS.Address+56)
	ENDPROC

	PROCEDURE dwNumConns_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+56,lnNewVal)
	ENDPROC

ENDDEFINE

DEFINE CLASS MIB_UDPSTATS AS Relation

	Address = 0
	SizeOf = 20
	&& structure fields
	dwInDatagrams = .F.
	dwNoPorts = .F.
	dwInErrors = .F.
	dwOutDatagrams = .F.
	dwNumAddrs = .F.

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

	PROCEDURE dwInDatagrams_Access()
		RETURN ReadUInt(THIS.Address)
	ENDPROC

	PROCEDURE dwInDatagrams_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address,lnNewVal)
	ENDPROC

	PROCEDURE dwNoPorts_Access()
		RETURN ReadUInt(THIS.Address+4)
	ENDPROC

	PROCEDURE dwNoPorts_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+4,lnNewVal)
	ENDPROC

	PROCEDURE dwInErrors_Access()
		RETURN ReadUInt(THIS.Address+8)
	ENDPROC

	PROCEDURE dwInErrors_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+8,lnNewVal)
	ENDPROC

	PROCEDURE dwOutDatagrams_Access()
		RETURN ReadUInt(THIS.Address+12)
	ENDPROC

	PROCEDURE dwOutDatagrams_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+12,lnNewVal)
	ENDPROC

	PROCEDURE dwNumAddrs_Access()
		RETURN ReadUInt(THIS.Address+16)
	ENDPROC

	PROCEDURE dwNumAddrs_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+16,lnNewVal)
	ENDPROC

ENDDEFINE

DEFINE CLASS MIB_ICMP AS Relation

	Address = 0
	SizeOf = 104
	&& structure fields
	stats = .NULL.

	PROCEDURE Init()
		THIS.Address = AllocMem(THIS.SizeOf)
		IF THIS.Address = 0
			ERROR(43)
			RETURN .F.
		ENDIF
		THIS.stats = CREATEOBJECT('MIBICMPINFO',THIS.Address)
	ENDPROC

	PROCEDURE Destroy()
		THIS.stats = .NULL.
		FreeMem(THIS.Address)
	ENDPROC

	PROCEDURE Address_Assign(lnAddress)
		DO CASE
			CASE THIS.Address = 0
				THIS.Address = lnAddress
			CASE THIS.Address = lnAddress
			OTHERWISE
				THIS.Address = lnAddress
				THIS.stats.Address = lnAddress
		ENDCASE
	ENDPROC

ENDDEFINE

DEFINE CLASS MIBICMPINFO AS Relation

	Address = 0
	SizeOf = 104
	&& structure fields
	icmpInStats = .NULL.
	icmpOutStats = .NULL.

	PROCEDURE Init(lnAddress)
		ASSERT TYPE('lnAddress') = 'N' AND lnAddress != 0 MESSAGE 'Invalid structure address!'
		THIS.Address = lnAddress
		THIS.icmpInStats = CREATEOBJECT('MIBICMPSTATS',THIS.Address)
		THIS.icmpOutStats = CREATEOBJECT('MIBICMPSTATS',THIS.Address+52)
	ENDPROC

	PROCEDURE Destroy()
		THIS.icmpInStats = .NULL.
		THIS.icmpOutStats = .NULL.
	ENDPROC

	PROCEDURE Address_Assign(lnAddress)
		DO CASE
			CASE THIS.Address = 0
				THIS.Address = lnAddress
			CASE THIS.Address = lnAddress
			OTHERWISE
				THIS.Address = lnAddress
				THIS.icmpInStats.Address = lnAddress
				THIS.icmpOutStats.Address = lnAddress+52
		ENDCASE
	ENDPROC

ENDDEFINE

DEFINE CLASS MIBICMPSTATS AS Relation

	Address = 0
	SizeOf = 52
	&& structure fields
	dwMsgs = .F.
	dwErrors = .F.
	dwDestUnreachs = .F.
	dwTimeExcds = .F.
	dwParmProbs = .F.
	dwSrcQuenchs = .F.
	dwRedirects = .F.
	dwEchos = .F.
	dwEchoReps = .F.
	dwTimestamps = .F.
	dwTimestampReps = .F.
	dwAddrMasks = .F.
	dwAddrMaskReps = .F.

	PROCEDURE Init(lnAddress)
		ASSERT TYPE('lnAddress') = 'N' AND lnAddress != 0 MESSAGE 'Invalid structure address!'
		THIS.Address = lnAddress
	ENDPROC

	PROCEDURE dwMsgs_Access()
		RETURN ReadUInt(THIS.Address)
	ENDPROC

	PROCEDURE dwMsgs_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address,lnNewVal)
	ENDPROC

	PROCEDURE dwErrors_Access()
		RETURN ReadUInt(THIS.Address+4)
	ENDPROC

	PROCEDURE dwErrors_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+4,lnNewVal)
	ENDPROC

	PROCEDURE dwDestUnreachs_Access()
		RETURN ReadUInt(THIS.Address+8)
	ENDPROC

	PROCEDURE dwDestUnreachs_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+8,lnNewVal)
	ENDPROC

	PROCEDURE dwTimeExcds_Access()
		RETURN ReadUInt(THIS.Address+12)
	ENDPROC

	PROCEDURE dwTimeExcds_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+12,lnNewVal)
	ENDPROC

	PROCEDURE dwParmProbs_Access()
		RETURN ReadUInt(THIS.Address+16)
	ENDPROC

	PROCEDURE dwParmProbs_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+16,lnNewVal)
	ENDPROC

	PROCEDURE dwSrcQuenchs_Access()
		RETURN ReadUInt(THIS.Address+20)
	ENDPROC

	PROCEDURE dwSrcQuenchs_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+20,lnNewVal)
	ENDPROC

	PROCEDURE dwRedirects_Access()
		RETURN ReadUInt(THIS.Address+24)
	ENDPROC

	PROCEDURE dwRedirects_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+24,lnNewVal)
	ENDPROC

	PROCEDURE dwEchos_Access()
		RETURN ReadUInt(THIS.Address+28)
	ENDPROC

	PROCEDURE dwEchos_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+28,lnNewVal)
	ENDPROC

	PROCEDURE dwEchoReps_Access()
		RETURN ReadUInt(THIS.Address+32)
	ENDPROC

	PROCEDURE dwEchoReps_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+32,lnNewVal)
	ENDPROC

	PROCEDURE dwTimestamps_Access()
		RETURN ReadUInt(THIS.Address+36)
	ENDPROC

	PROCEDURE dwTimestamps_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+36,lnNewVal)
	ENDPROC

	PROCEDURE dwTimestampReps_Access()
		RETURN ReadUInt(THIS.Address+40)
	ENDPROC

	PROCEDURE dwTimestampReps_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+40,lnNewVal)
	ENDPROC

	PROCEDURE dwAddrMasks_Access()
		RETURN ReadUInt(THIS.Address+44)
	ENDPROC

	PROCEDURE dwAddrMasks_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+44,lnNewVal)
	ENDPROC

	PROCEDURE dwAddrMaskReps_Access()
		RETURN ReadUInt(THIS.Address+48)
	ENDPROC

	PROCEDURE dwAddrMaskReps_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+48,lnNewVal)
	ENDPROC

ENDDEFINE

DEFINE CLASS IP_ADAPTER_ADDRESSES AS Relation

	Address = 0
	SizeOf = 144
	BufferSize = 16384
	PROTECTED Embedded
	Embedded = .F.
	&& structure fields
	Length = .F.
	IfIndex = .F.
	Next = .NULL.
	AdapterName = .F.
	FirstUnicastAddress = .NULL.
	FirstAnycastAddress = .NULL.
	FirstMulticastAddress = .NULL.
	FirstDnsServerAddress = .NULL.
	DnsSuffix = .F.
	Description = .F.
	FriendlyName = .F.
	PhysicalAddress = .F.
	PhysicalAddressLength = .F.
	Flags = .F.
	Mtu = .F.
	IfType = .F.
	OperStatus = .F.
	Ipv6IfIndex = .F.
	DIMENSION ZoneIndices[1]
	FirstPrefix = .NULL.

	PROCEDURE Init(lnAddress)
		IF PCOUNT() = 0
			THIS.Address = AllocMem(THIS.BufferSize)
			IF THIS.Address = 0
				ERROR(43)
				RETURN .F.
			ENDIF
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

	PROCEDURE BufferSize_Assign(nBufferSize)
		LOCAL lnAddress
		lnAddress = ReAllocMem(THIS.Address,nBufferSize)
		IF lnAddress != 0
			THIS.Address = lnAddress
		ELSE
			ERROR(43)
		ENDIF
	ENDPROC

	PROCEDURE Length_Access()
		RETURN ReadUInt(THIS.Address)
	ENDPROC

	PROCEDURE IfIndex_Access()
		RETURN ReadUInt(THIS.Address+4)
	ENDPROC

	PROCEDURE Next_Access()
		RETURN ReadPointer(THIS.Address+8)
	ENDPROC

	PROCEDURE AdapterName_Access()
		RETURN ReadPCString(THIS.Address+12)
	ENDPROC

	PROCEDURE FirstUnicastAddress_Access()
		RETURN ReadPointer(THIS.Address+16)
	ENDPROC

	PROCEDURE FirstAnycastAddress_Access()
		RETURN ReadPointer(THIS.Address+20)
	ENDPROC

	PROCEDURE FirstMulticastAddress_Access()
		RETURN ReadPointer(THIS.Address+24)
	ENDPROC

	PROCEDURE FirstDnsServerAddress_Access()
		RETURN ReadPointer(THIS.Address+28)
	ENDPROC

	PROCEDURE DnsSuffix_Access()
		RETURN ReadPWString(THIS.Address+32)
	ENDPROC

	PROCEDURE Description_Access()
		RETURN ReadPWString(THIS.Address+36)
	ENDPROC

	PROCEDURE FriendlyName_Access()
		RETURN ReadPWString(THIS.Address+40)
	ENDPROC

	PROCEDURE PhysicalAddress_Access()
		&& changed to ReadBytes (cause it contains binary data)
		&& and second parameter changed to THIS.PhysicalAddressLenght which contains the actual length
		&& BinMacToMac (declared at the bottom of this file) converts a binary mac address into a readable mac address 
		RETURN BinMacToMac(ReadBytes(THIS.Address+44,THIS.PhysicalAddressLength))
	ENDPROC

	PROCEDURE PhysicalAddressLength_Access()
		RETURN ReadUInt(THIS.Address+52)
	ENDPROC

	PROCEDURE Flags_Access()
		RETURN ReadUInt(THIS.Address+56)
	ENDPROC

	PROCEDURE Mtu_Access()
		RETURN ReadUInt(THIS.Address+60)
	ENDPROC

	PROCEDURE IfType_Access()
		RETURN ReadUInt(THIS.Address+64)
	ENDPROC

	PROCEDURE OperStatus_Access()
		RETURN ReadInt(THIS.Address+68)
	ENDPROC

	PROCEDURE Ipv6IfIndex_Access()
		RETURN ReadUInt(THIS.Address+72)
	ENDPROC

	PROCEDURE ZoneIndices_Access(lnRow)
		ASSERT TYPE('lnRow') = 'N' AND BETWEEN(lnRow,1,16) MESSAGE 'Invalid row subscript!'
		RETURN ReadUInt(THIS.Address+72+lnRow*4)
	ENDPROC

	PROCEDURE FirstPrefix_Access()
		RETURN ReadPointer(THIS.Address+140)
	ENDPROC

ENDDEFINE

DEFINE CLASS SOCKADDR AS Relation
	Address = 0
	SizeOf = 16
	PROTECTED Embedded
	Embedded = .F.
	&& structure fields
	sa_family = .F.
	sa_data = .F.

	PROCEDURE Init(lnAddress)
		IF PCOUNT() = 0
			THIS.Address = AllocMem(THIS.SizeOf)
			IF THIS.Address = 0
				ERROR(43)
				RETURN .F.
			ENDIF
		ELSE
			ASSERT TYPE('lnAddress') = 'N' AND lnAddress != 0 MESSAGE 'Invalid structure address!'
			THIS.Address = lnAddress
			THIS.Embedded = .T.
		ENDIF
	ENDPROC

	PROCEDURE Destroy()
		IF !THIS.Embedded
			FreeMem(THIS.Address)
		ENDIF
	ENDPROC

	PROCEDURE sa_family_Access()
		RETURN ReadUShort(THIS.Address)
	ENDPROC

	PROCEDURE sa_family_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,65535) MESSAGE 'Wrong datatype or value out of range!'
		WriteUShort(THIS.Address,lnNewVal)
	ENDPROC

	PROCEDURE sa_data_Access()
		RETURN ReadBytes(THIS.Address+2,14)
	ENDPROC

	PROCEDURE sa_data_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'C' MESSAGE 'Wrong datatype or value out of range!'
		WriteBytes(THIS.Address+2,lnNewVal,14)
	ENDPROC

ENDDEFINE

DEFINE CLASS SOCKET_ADDRESS AS Relation

	Address = 0
	SizeOf = 8
	PROTECTED Embedded
	Embedded = .F.
	&& structure fields
	lpSockaddr = .NULL.
	iSockaddrLength = .F.

	PROCEDURE Init(lnAddress)
		IF PCOUNT() = 0
			THIS.Address = AllocMem(THIS.SizeOf)
			IF THIS.Address = 0
				ERROR(43)
				RETURN .F.
			ENDIF
		ELSE
			ASSERT TYPE('lnAddress') = 'N' AND lnAddress != 0 MESSAGE 'Invalid structure address!'
			THIS.Address = lnAddress
			THIS.Embedded = .T.
		ENDIF
	ENDPROC

	PROCEDURE Destroy()
		IF !THIS.Embedded
			FreeMem(THIS.Address)
		ENDIF
	ENDPROC

	PROCEDURE lpSockaddr_Access()
		RETURN ReadPointer(THIS.Address)
	ENDPROC

	PROCEDURE lpSockaddr_Assign(lnNewVal)
		WritePointer(THIS.Address,lnNewVal)
	ENDPROC

	PROCEDURE iSockaddrLength_Access()
		RETURN ReadInt(THIS.Address+4)
	ENDPROC

	PROCEDURE iSockaddrLength_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,-2147483648,2147483647) MESSAGE 'Wrong datatype or value out of range!'
		WriteInt(THIS.Address+4,lnNewVal)
	ENDPROC

ENDDEFINE

DEFINE CLASS IP_ADAPTER_UNICAST_ADDRESS AS Relation

	Address = 0
	SizeOf = 48
	PROTECTED Embedded
	Embedded = .F.
	&& structure fields
	Length = .F.
	Flags = .F.
	Next = .NULL.
	mAddress = .NULL.
	PrefixOrigin = .F.
	SuffixOrigin = .F.
	DadState = .F.
	ValidLifetime = .F.
	PreferredLifetime = .F.
	LeaseLifetime = .F.

	PROCEDURE Init(lnAddress)
		IF PCOUNT() = 0
			THIS.Address = AllocMem(THIS.SizeOf)
			IF THIS.Address = 0
				ERROR(43)
				RETURN .F.
			ENDIF
		ELSE
			ASSERT TYPE('lnAddress') = 'N' AND lnAddress != 0 MESSAGE 'Invalid structure address!'
			THIS.Address = lnAddress
			THIS.Embedded = .T.
		ENDIF
		THIS.mAddress = CREATEOBJECT('SOCKET_ADDRESS',THIS.Address+12)
	ENDPROC

	PROCEDURE Destroy()
		THIS.mAddress = .NULL.
		IF !THIS.Embedded
			FreeMem(THIS.Address)
		ENDIF
	ENDPROC

	PROCEDURE Address_Assign(lnAddress)
		DO CASE
			CASE THIS.Address = 0
				THIS.Address = lnAddress
			CASE THIS.Address = lnAddress
			OTHERWISE
				THIS.Address = lnAddress
				THIS.mAddress.Address = lnAddress+12
		ENDCASE
	ENDPROC

	PROCEDURE Length_Access()
		RETURN ReadUInt(THIS.Address)
	ENDPROC

	PROCEDURE Length_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address,lnNewVal)
	ENDPROC

	PROCEDURE Flags_Access()
		RETURN ReadUInt(THIS.Address+4)
	ENDPROC

	PROCEDURE Flags_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+4,lnNewVal)
	ENDPROC

	PROCEDURE Next_Access()
		RETURN ReadPointer(THIS.Address+8)
	ENDPROC

	PROCEDURE Next_Assign(lnNewVal)
		WritePointer(THIS.Address+8,lnNewVal)
	ENDPROC

	PROCEDURE PrefixOrigin_Access()
		RETURN ReadInt(THIS.Address+20)
	ENDPROC

	PROCEDURE PrefixOrigin_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,-2147483648,2147483647) MESSAGE 'Wrong datatype or value out of range!'
		WriteInt(THIS.Address+20,lnNewVal)
	ENDPROC

	PROCEDURE SuffixOrigin_Access()
		RETURN ReadInt(THIS.Address+24)
	ENDPROC

	PROCEDURE SuffixOrigin_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,-2147483648,2147483647) MESSAGE 'Wrong datatype or value out of range!'
		WriteInt(THIS.Address+24,lnNewVal)
	ENDPROC

	PROCEDURE DadState_Access()
		RETURN ReadInt(THIS.Address+28)
	ENDPROC

	PROCEDURE DadState_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,-2147483648,2147483647) MESSAGE 'Wrong datatype or value out of range!'
		WriteInt(THIS.Address+28,lnNewVal)
	ENDPROC

	PROCEDURE ValidLifetime_Access()
		RETURN ReadUInt(THIS.Address+32)
	ENDPROC

	PROCEDURE ValidLifetime_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+32,lnNewVal)
	ENDPROC

	PROCEDURE PreferredLifetime_Access()
		RETURN ReadUInt(THIS.Address+36)
	ENDPROC

	PROCEDURE PreferredLifetime_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+36,lnNewVal)
	ENDPROC

	PROCEDURE LeaseLifetime_Access()
		RETURN ReadUInt(THIS.Address+40)
	ENDPROC

	PROCEDURE LeaseLifetime_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+40,lnNewVal)
	ENDPROC

ENDDEFINE

DEFINE CLASS IP_ADAPTER_ANYCAST_ADDRESS AS Relation

	Address = 0
	SizeOf = 24
	PROTECTED Embedded
	Embedded = .F.
	&& structure fields
	Length = .F.
	Flags = .F.
	Next = .NULL.
	mAddress = .NULL.

	PROCEDURE Init(lnAddress)
		IF PCOUNT() = 0
			THIS.Address = AllocMem(THIS.SizeOf)
			IF THIS.Address = 0
				ERROR(43)
				RETURN .F.
			ENDIF
		ELSE
			ASSERT TYPE('lnAddress') = 'N' AND lnAddress != 0 MESSAGE 'Invalid structure address!'
			THIS.Address = lnAddress
			THIS.Embedded = .T.
		ENDIF
		THIS.mAddress = CREATEOBJECT('SOCKET_ADDRESS',THIS.Address+12)
	ENDPROC

	PROCEDURE Destroy()
		THIS.mAddress = .NULL.
		IF !THIS.Embedded
			FreeMem(THIS.Address)
		ENDIF
	ENDPROC

	PROCEDURE Address_Assign(lnAddress)
		DO CASE
			CASE THIS.Address = 0
				THIS.Address = lnAddress
			CASE THIS.Address = lnAddress
			OTHERWISE
				THIS.Address = lnAddress
				THIS.mAddress.Address = lnAddress+12
		ENDCASE
	ENDPROC

	PROCEDURE Length_Access()
		RETURN ReadUInt(THIS.Address)
	ENDPROC

	PROCEDURE Length_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address,lnNewVal)
	ENDPROC

	PROCEDURE Flags_Access()
		RETURN ReadUInt(THIS.Address+4)
	ENDPROC

	PROCEDURE Flags_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+4,lnNewVal)
	ENDPROC

	PROCEDURE Next_Access()
		RETURN ReadPointer(THIS.Address+8)
	ENDPROC

	PROCEDURE Next_Assign(lnNewVal)
		WritePointer(THIS.Address+8,lnNewVal)
	ENDPROC

ENDDEFINE

DEFINE CLASS IP_ADAPTER_MULTICAST_ADDRESS AS Relation

	Address = 0
	SizeOf = 24
	PROTECTED Embedded
	Embedded = .F.
	&& structure fields
	Length = .F.
	Flags = .F.
	Next = .NULL.
	mAddress = .NULL.

	PROCEDURE Init(lnAddress)
		IF PCOUNT() = 0
			THIS.Address = AllocMem(THIS.SizeOf)
			IF THIS.Address = 0
				ERROR(43)
				RETURN .F.
			ENDIF
		ELSE
			ASSERT TYPE('lnAddress') = 'N' AND lnAddress != 0 MESSAGE 'Invalid structure address!'
			THIS.Address = lnAddress
			THIS.Embedded = .T.
		ENDIF
		THIS.mAddress = CREATEOBJECT('SOCKET_ADDRESS',THIS.Address+12)
	ENDPROC

	PROCEDURE Destroy()
		THIS.mAddress = .NULL.
		IF !THIS.Embedded
			FreeMem(THIS.Address)
		ENDIF
	ENDPROC

	PROCEDURE Address_Assign(lnAddress)
		DO CASE
			CASE THIS.Address = 0
				THIS.Address = lnAddress
			CASE THIS.Address = lnAddress
			OTHERWISE
				THIS.Address = lnAddress
				THIS.mAddress.Address = lnAddress+12
		ENDCASE
	ENDPROC

	PROCEDURE Length_Access()
		RETURN ReadUInt(THIS.Address)
	ENDPROC

	PROCEDURE Length_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address,lnNewVal)
	ENDPROC

	PROCEDURE Flags_Access()
		RETURN ReadUInt(THIS.Address+4)
	ENDPROC

	PROCEDURE Flags_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+4,lnNewVal)
	ENDPROC

	PROCEDURE Next_Access()
		RETURN ReadPointer(THIS.Address+8)
	ENDPROC

	PROCEDURE Next_Assign(lnNewVal)
		WritePointer(THIS.Address+8,lnNewVal)
	ENDPROC

ENDDEFINE

DEFINE CLASS IP_ADAPTER_DNS_SERVER_ADDRESS AS Relation

	Address = 0
	SizeOf = 24
	&& structure fields
	Length = .F.
	Reserved = .F.
	Next = .NULL.
	mAddress = .NULL.

	PROCEDURE Init(lnAddress)
		ASSERT TYPE('lnAddress') = 'N' AND lnAddress != 0 MESSAGE 'Invalid structure address!'
		THIS.Address = lnAddress
		THIS.mAddress = CREATEOBJECT('SOCKET_ADDRESS',THIS.Address+12)
	ENDPROC

	PROCEDURE Destroy()
		THIS.mAddress = .NULL.
	ENDPROC

	PROCEDURE Address_Assign(lnAddress)
		DO CASE
			CASE THIS.Address = 0
				THIS.Address = lnAddress
			CASE THIS.Address = lnAddress
			OTHERWISE
				THIS.Address = lnAddress
				THIS.mAddress.Address = lnAddress+12
		ENDCASE
	ENDPROC

	PROCEDURE Length_Access()
		RETURN ReadUInt(THIS.Address)
	ENDPROC

	PROCEDURE Length_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address,lnNewVal)
	ENDPROC

	PROCEDURE Reserved_Access()
		RETURN ReadUInt(THIS.Address+4)
	ENDPROC

	PROCEDURE Reserved_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+4,lnNewVal)
	ENDPROC

	PROCEDURE Next_Access()
		RETURN ReadPointer(THIS.Address+8)
	ENDPROC

	PROCEDURE Next_Assign(lnNewVal)
		WritePointer(THIS.Address+8,lnNewVal)
	ENDPROC

ENDDEFINE

DEFINE CLASS IP_ADAPTER_PREFIX AS Relation

	Address = 0
	SizeOf = 24
	PROTECTED Embedded
	Embedded = .F.
	&& structure fields
	Length = .F.
	Flags = .F.
	Next = .NULL.
	mAddress = .NULL.
	PrefixLength = .F.

	PROCEDURE Init(lnAddress)
		IF PCOUNT() = 0
			THIS.Address = AllocMem(THIS.SizeOf)
			IF THIS.Address = 0
				ERROR(43)
				RETURN .F.
			ENDIF
		ELSE
			ASSERT TYPE('lnAddress') = 'N' AND lnAddress != 0 MESSAGE 'Invalid structure address!'
			THIS.Address = lnAddress
			THIS.Embedded = .T.
		ENDIF
		THIS.mAddress = CREATEOBJECT('SOCKET_ADDRESS',THIS.Address+12)
	ENDPROC

	PROCEDURE Destroy()
		THIS.mAddress = .NULL.
		IF !THIS.Embedded
			FreeMem(THIS.Address)
		ENDIF
	ENDPROC

	PROCEDURE Address_Assign(lnAddress)
		DO CASE
			CASE THIS.Address = 0
				THIS.Address = lnAddress
			CASE THIS.Address = lnAddress
			OTHERWISE
				THIS.Address = lnAddress
				THIS.mAddress.Address = lnAddress+12
		ENDCASE
	ENDPROC

	PROCEDURE Length_Access()
		RETURN ReadUInt(THIS.Address)
	ENDPROC

	PROCEDURE Length_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address,lnNewVal)
	ENDPROC

	PROCEDURE Flags_Access()
		RETURN ReadUInt(THIS.Address+4)
	ENDPROC

	PROCEDURE Flags_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+4,lnNewVal)
	ENDPROC

	PROCEDURE Next_Access()
		RETURN ReadPointer(THIS.Address+8)
	ENDPROC

	PROCEDURE Next_Assign(lnNewVal)
		WritePointer(THIS.Address+8,lnNewVal)
	ENDPROC

	PROCEDURE PrefixLength_Access()
		RETURN ReadUInt(THIS.Address+20)
	ENDPROC

	PROCEDURE PrefixLength_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+20,lnNewVal)
	ENDPROC

ENDDEFINE

DEFINE CLASS MIB_TCPTABLE AS Relation

	Address = 0
	SizeOf = 24
	BufferSize = 16384
	&& structure fields
	dwNumEntries = .F.
	DIMENSION mtable[1]

	PROCEDURE Init()
		THIS.Address = AllocMem(THIS.BufferSize)
		IF THIS.Address = 0
			ERROR(43)
			RETURN .F.
		ENDIF
		THIS.mtable[1] = CREATEOBJECT('MIB_TCPROW',THIS.Address+4)
	ENDPROC

	PROCEDURE Destroy()
		THIS.mtable[1] = .NULL.
		FreeMem(THIS.Address)
	ENDPROC

	PROCEDURE Address_Assign(lnAddress)
		DO CASE
			CASE THIS.Address = 0
				THIS.Address = lnAddress
			CASE THIS.Address = lnAddress
			OTHERWISE
				THIS.Address = lnAddress
				THIS.mtable[1].Address = lnAddress+4
		ENDCASE
	ENDPROC

	PROCEDURE BufferSize_Assign(nBufferSize)
		LOCAL lnAddress
		lnAddress = ReAllocMem(THIS.Address,nBufferSize)
		IF lnAddress != 0
			THIS.Address = lnAddress
		ELSE
			ERROR(43)
		ENDIF
	ENDPROC

	PROCEDURE dwNumEntries_Access()
		RETURN ReadUInt(THIS.Address)
	ENDPROC

	PROCEDURE mtable_Access(lnRow)
		THIS.mtable[1].Address = THIS.Address-16+20*lnRow
		RETURN THIS.mtable1[1]
	ENDPROC

ENDDEFINE

DEFINE CLASS MIB_TCPROW AS Relation

	Address = 0
	SizeOf = 20
	&& structure fields
	dwState = .F.
	dwLocalAddr = .F.
	dwLocalPort = .F.
	dwRemoteAddr = .F.
	dwRemotePort = .F.

	PROCEDURE Init(lnAddress)
		ASSERT TYPE('lnAddress') = 'N' AND lnAddress != 0 MESSAGE 'Invalid structure address!'
		THIS.Address = lnAddress
	ENDPROC

	PROCEDURE dwState_Access()
		RETURN ReadUInt(THIS.Address)
	ENDPROC

	PROCEDURE dwState_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address,lnNewVal)
	ENDPROC

	PROCEDURE dwLocalAddr_Access()
		RETURN ReadUInt(THIS.Address+4)
	ENDPROC

	PROCEDURE dwLocalAddr_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+4,lnNewVal)
	ENDPROC

	PROCEDURE dwLocalPort_Access()
		RETURN ReadUInt(THIS.Address+8)
	ENDPROC

	PROCEDURE dwLocalPort_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+8,lnNewVal)
	ENDPROC

	PROCEDURE dwRemoteAddr_Access()
		RETURN ReadUInt(THIS.Address+12)
	ENDPROC

	PROCEDURE dwRemoteAddr_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+12,lnNewVal)
	ENDPROC

	PROCEDURE dwRemotePort_Access()
		RETURN ReadUInt(THIS.Address+16)
	ENDPROC

	PROCEDURE dwRemotePort_Assign(lnNewVal)
		ASSERT TYPE('lnNewVal') = 'N' AND BETWEEN(lnNewVal,0,4294967295) MESSAGE 'Wrong datatype or value out of range!'
		WriteUInt(THIS.Address+16,lnNewVal)
	ENDPROC

ENDDEFINE

&& converts a binary MAC address passed in lcBinAddr to a human readable MAC address
FUNCTION BinMacToMac(lcBinAddr)

	LOCAL lcMacAddr, lnLen, xj
	lcMacAddr = ""
	lnLen = LEN(lcBinAddr)
	IF lnLen = 0
		RETURN ""
	ENDIF
	
	FOR xj = 1 TO lnLen - 1
		lcMacAddr = lcMacAddr + RIGHT(TRANSFORM(ASC(SUBSTR(lcBinAddr,xj,1)),"@0"),2) + "-"
	ENDFOR
	lcMacAddr = lcMacAddr + RIGHT(TRANSFORM(ASC(SUBSTR(lcBinAddr,lnLen,1)),"@0"),2)

	RETURN lcMacAddr

ENDFUNC

&& the counterpart to the above function - just for fun ..
FUNCTION MacToBinMac(lcMacAddr)
	
	LOCAL lcBinAddr, xj, lnParts, laParts[1]
	lcBinAddr = ""
	lnParts = ALINES(laParts,lcMacAddr,5,"-")

	IF lnParts = 0
		RETURN ""
	ENDIF

	FOR xj = 1 TO lnParts
		lcBinAddr = lcBinAddr + CHR(EVALUATE("0x"+laParts[xj]))
	ENDFOR
	
	RETURN lcBinAddr

ENDFUNC

&& converts a numerical IP address to a dotted readable IP address
FUNCTION NumIPToIP(lnBinAddr)
	LOCAL lcIPAddr
	lcIPAddr =	ALLTRIM(STR(BITAND(lnBinAddr,0x000000FF))) + "." + ;
				ALLTRIM(STR(BITAND(BITRSHIFT(lnBinAddr,8),0x000000FF))) + "." + ;
				ALLTRIM(STR(BITAND(BITRSHIFT(lnBinAddr,16),0x000000FF))) + "." + ;
				ALLTRIM(STR(BITRSHIFT(lnBinAddr,24),0x000000FF))
				&& we don't have to BITAND in the last line cause the value is filled with 0's by the rightshift
	RETURN lcIPAddr
ENDFUNC

&& counterpart .. 
FUNCTION IPToNumIP(lcIPAddr)
	LOCAL lnBinAddr, laParts[1], xj
	lnParts = ALINES(laParts,lcIpAddr,5,'.')
	IF lnParts != 4
		RETURN 0
	ENDIF
	
	lnBinAddr = VAL(laParts[1]) + ;
				VAL(laParts[2]) * 256 + ;
				VAL(laParts[3]) * 65536 + ;
				VAL(laParts[4]) * 16777216

	RETURN lnBinAddr
ENDFUNC

