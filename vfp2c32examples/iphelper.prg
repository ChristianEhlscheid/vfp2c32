#INCLUDE "iptypes.h" && include iphelper #defines's and enum's
#INCLUDE "windows.h" && include general windows #define's
#INCLUDE "winsock2.h"
#INCLUDE "vfp2c.h"

SET LIBRARY TO vfp2c32.fll ADDITIVE
INITVFP2C32(VFP2C_INIT_MARSHAL)

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

DECLARE STRING inet_ntoa IN wsock32.dll INTEGER

&& this brings in some helper function to examine DLL's and their exported functions
DO apihelper

*!* the '\'s behind the C code lines are there cause VFP would otherwise interpret ';' at the end of a line as line continuation character
*!* (also after a && style comment which is more than strange ..) they are not part of the C code .. just ignore them ..

*!* ULONG		ulOutBufLen; \
*!* DWORD		dwRetVal; \
LOCAL ulOutBufLen, dwRetVal

*!*	PMIB_IFTABLE ifTable; \
LOCAL ifTable

*!*	ifTable = (MIB_IFTABLE*) malloc(sizeof(MIB_IFTABLE)); \
*!*	ulOutBufLen = sizeof(MIB_IFTABLE); \
ifTable = CREATEOBJECT('MIB_IFTABLE')
ulOutBufLen = ifTable.BufferSize

*!*	if ((dwRetVal = GetIfTable(ifTable, &ulOutBufLen, 0)) == ERROR_INSUFFICIENT_BUFFER) {
*!*		free(ifTable); \
*!*		ifTable = (MIB_IFTABLE *) malloc(ulOutBufLen); \
*!*		dwRetVal = GetIfTable(ifTable,&ulOutBufLen); \
*!*	}
dwRetVal = GetIfTable(ifTable.Address,@ulOutBufLen,0)
IF dwRetVal = ERROR_INSUFFICIENT_BUFFER
	ifTable.BufferSize = ulOutBufLen
	dwRetVal = GetIfTable(ifTable.Address,@ulOutBufLen,0)
ENDIF

*!*	if (dwRetVal != NO_ERROR) {
*!*	  printf("\tGetIfTable failed.\n"); \
*!* } else {
IF dwRetVal != NO_ERROR
	? "GetIfTable failed."
ELSE
*!*  for (int xj = 0; xj < ifTable->dwNumEntries; xj++)
*!*  {
*!*    printf("Description: %s",ifTable->table[xj].bDescr); \
*!*  }
*!* }
*!* free(ifTable); \
	LOCAL xj
	FOR xj = 1 TO ifTable.dwNumEntries
		? "Interface Name: " + ifTable.mTable[xj].wszName
		? "Description: " + ifTable.mTable[xj].bDescr
		? "MTU : ",  ifTable.mTable[xj].dwMtu
		? "Speed: " + ALLTRIM(STR(ifTable.mTable[xj].dwSpeed / 100000)) + " MBit"
		? "MAC Address Len: ", ifTable.mTable[xj].dwPhysAddrLen
		? "MAC Address : ", ifTable.mTable[xj].bPhysAddr
	ENDFOR
ENDIF
ifTable = .NULL.


? "------------------------"
? "This is GetAdaptersInfo"
? "------------------------"

*!*		IP_ADAPTER_INFO		*pAdapterInfo; \
*!*		IP_ADAPTER_INFO		*pAdapter; \
LOCAL pAdapterInfo, pAdapter

*!*		pAdapterInfo = (IP_ADAPTER_INFO *) malloc( sizeof(IP_ADAPTER_INFO) );
*!*		ulOutBufLen = sizeof(IP_ADAPTER_INFO); \
pAdapterInfo = CREATEOBJECT('IP_ADAPTER_INFO')
ulOutBufLen = pAdapterInfo.BufferSize

*!*	dwRetVal = GetAdaptersInfo( pAdapterInfo, &ulOutBufLen)
*!*	if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
*!*		free (pAdapterInfo); \
*!*		pAdapterInfo = (IP_ADAPTER_INFO *) malloc ( sizeof(ulOutBufLen) ); \
*!*		dwRetVal = GetAdaptersInfo( pAdapterInfo, &ulOutBufLen );  \
*!*	} \
dwRetVal = GetAdaptersInfo(pAdapterInfo.Address,@ulOutBufLen)
IF dwRetVal = ERROR_BUFFER_OVERFLOW
	pAdapterInfo.BufferSize = ulOutBufLen && memory reallocation is handled in BufferSize_Assign method
	dwRetVal = GetAdaptersInfo(pAdapterInfo.Address,@ulOutBufLen)
ENDIF

*!* if (dwRetVal != NO_ERROR) {
*!* 	printf("Call to GetAdaptersInfo failed.\n"); \
*!*	}
IF dwRetVal != NO_ERROR
	? "Call to GetAdaptersInfo failed."
	
*!* else {
*!*	pAdapter = pAdapterInfo; \

ELSE
	pAdapter = CREATEOBJECT('IP_ADAPTER_INFO',pAdapterInfo.Address)
	
*!*		while (pAdapter) {
*!*			printf("\tAdapter Name: \t%s\n", pAdapter->AdapterName); \
*!*			printf("\tAdapter Desc: \t%s\n", pAdapter->Description); \
*!*			printf("\tAdapter Addr: \t%ld\n", pAdapter->Address); \
*!*			printf("\tIP Address: \t%s\n", pAdapter->IpAddressList.IpAddress.String); \
*!*			printf("\tIP Mask: \t%s\n", pAdapter->IpAddressList.IpMask.String); \
*!*			printf("\tGateway: \t%s\n", pAdapter->GatewayList.IpAddress.String); \
*!*			printf("\t***\n"); \
	DO WHILE pAdapter.Address != 0
			? CHR(9) + "Adapter Name: " + CHR(9) + pAdapter.AdapterName
			? CHR(9) + "Adapter Desc: " + CHR(9) + pAdapter.Description
			? CHR(9) + "Adapter Addr: " + CHR(9), pAdapter.mAddress && Address is renamed to mAddress
			? CHR(9) + "IP Address: " + CHR(9) + pAdapter.IpAddressList.IpAddress.String
			? CHR(9) + "IP Mask: " + CHR(9) + pAdapter.IpAddressList.IpMask.String
			? CHR(9) + "Gateway: " + CHR(9) + pAdapter.GatewayList.IpAddress.String
			? CHR(9) + "***"
	
*!*			if (pAdapter->DhcpEnabled) {
*!*				printf("\tDHCP Enabled: Yes\n"); \
*!*				printf("\t\tDHCP Server: \t%s\n", pAdapter->DhcpServer.IpAddress.String); \
*!*				printf("\tLease Obtained: %ld\n", pAdapter->LeaseObtained); \
*!*			}
*!*			else
*!*				printf("\tDHCP Enabled: No\n"); \
			IF pAdapter.DhcpEnabled
				? CHR(9) + "DHCP Enabled Yes"
				? CHR(9) + "DHCP Server" + CHR(9) + pAdapter.DhcpServer.IpAddress.String
				? CHR(9) + "Lease Obtained:", pAdapter.LeaseObtained
			ELSE
				? CHR(9) + "DHCP Enabled: No"
			ENDIF

*!*			if (pAdapter->HaveWins) {
*!*				printf("\tHave Wins: Yes\n"); \
*!*				printf("\t\tPrimary Wins Server: \t%s\n", pAdapter->PrimaryWinsServer.IpAddress.String); \
*!*				printf("\t\tSecondary Wins Server: \t%s\n", pAdapter->SecondaryWinsServer.IpAddress.String); \
*!*			}
*!*			else
*!*				printf("\tHave Wins: No\n"); \
			IF pAdapter.HaveWins
				? CHR(9) + "Have Wins: Yes"
				? CHR(9) + CHR(9) + "Primary Wins Server:" + CHR(9) + pAdapter.PrimaryWinsServer.IpAddress.String
				? CHR(9) + CHR(9) + "Secondary Wins Server:" + CHR(9) + pAdapter.SecondaryWinsServer.IpAddress.String
			ELSE
				? CHR(9) + "Have Wins: No"
			ENDIF

*!*			pAdapter = pAdapter->Next; \
*!*		}
*!* 	}
*!*	free( pAdapterInfo); \
			pAdapter.Address = pAdapter.Next
	ENDDO
ENDIF
pAdapterInfo = .NULL.
pAdapter = .NULL.

&& check if function is exported by the dll
IF IsFuncExported('iphlpapi.dll','GetAdaptersAddresses') > 0

? "----------------------------"
? "This is GetAdaptersAddresses"
? "----------------------------"
*!*	PIP_ADAPTER_ADDRESSES pAddresses, pAddr; \
*!*	pAddresses = (IP_ADAPTER_ADDRESSES*) malloc(sizeof(IP_ADAPTER_ADDRESSES)); \
*!*	ULONG ulOutBufLen = 0; \
*!*	DWORD dwRetVal = 0; \
LOCAL pAddresses, pAddr
pAddresses = CREATEOBJECT('IP_ADAPTER_ADDRESSES')
ulOutBufLen = pAddresses.BufferSize

*!*	// Make an initial call to GetAdaptersAddresses to get the 
*!*	// size needed into the outBufLen variable
*!*	if ((dwRetVal = GetAdaptersAddresses(AF_INET,0,NULL,pAddresses,&ulOutBufLen)) == ERROR_INSUFFICIENT_BUFFER) {
*!*		free(pAddresses);
*!*		pAddresses = (IP_ADAPTER_ADDRESSES*) malloc(sizeof(ulOutBufLen)); \
*!*		dwRetVal = GetAdaptersAddresses(AF_INET,0,NULL,pAddresses,&ulOutBufLen); \
*!*	}
dwRetVal = GetAdaptersAddresses(AF_INET,0,0,pAddresses.Address,@ulOutBufLen)
IF dwRetVal = ERROR_INSUFFICIENT_BUFFER
	pAddresses.BufferSize = ulOutBufLen && memory reallocation is handled in BufferSize_Assign method
	dwRetVal = GetAdaptersAddresses(AF_INET,0,0,pAddresses.Address,@ulOutBufLen)
ENDIF

*!*	if (dwRetVal != NO_ERROR) {
*!*	 printf("Call to GetAdaptersAddresses failed.\n"); \
*!*	} else {
*!*	  pAddr = pAddresses;
*!*	  while (pAddr) {
*!*	    printf("\tFriendly name: %S\n", pAddr->FriendlyName); \
*!*	    printf("\tDescription: %S\n", pAddr->Description); \
*!* 	/* other display funcs ommitted because this should be clear by the way .. */
*!*	    pAddr = pAddr->Next; \
*!*	  }
*!*	}
*!*	free(pAddresses); \
IF dwRetVal != NO_ERROR
	? "Call to GetAdaptersAddresses failed."
	? FormatMessageEx(GetLastError())
ELSE
	pAddr = CREATEOBJECT('IP_ADAPTER_ADDRESSES',pAddresses.Address)
	DO WHILE pAddr.Address != 0
		? "Friendly name: " + pAddr.FriendlyName
		? "Description: " + pAddr.Description
		? "MAC Address: " + pAddr.PhysicalAddress
		? "Maximum transmission unit: ", pAddr.Mtu
		pAddr.Address = pAddr.Next
	ENDDO
ENDIF
pAddr = .NULL.
pAddresses = .NULL.

ELSE
	? "Function not available on this platform!"
ENDIF

? "------------------------"
? "This is GetInterfaceInfo"
? "------------------------"
*!*	IP_INTERFACE_INFO* pInfo; int xj; \
*!*	pInfo = (IP_INTERFACE_INFO *) malloc( sizeof(IP_INTERFACE_INFO) ); \
*!*	ulOutBufLen = sizeof(IP_INTERFACE_INFO); \
LOCAL pInfo, xj
pInfo = CREATEOBJECT('IP_INTERFACE_INFO')
ulOutBufLen = pInfo.BufferSize

*!*	dwRetVal = GetInterfaceInfo(pInfo, &ulOutBufLen); \
*!*	if (dwRetVal == ERROR_INSUFFICIENT_BUFFER) {
*!*			free(pInfo); \
*!*			pInfo = (IP_INTERFACE_INFO *) malloc(ulOutBufLen); \
*!*			dwRetVal = GetInterfaceInfo(pInfo, &ulOutBufLen); \
*!*		}
dwRetVal = GetInterfaceInfo(pInfo.Address,@ulOutBufLen)
IF dwRetVal = ERROR_INSUFFICIENT_BUFFER
	pInfo.BufferSize = ulOutButLen
	dwRetVal = GetInterfaceInfo(pInfo.Address,@ulOutBufLen)
ENDIF

*!*	if (dwRetVal != NO_ERROR ) {
*!*		printf("Call to GetInterfaceInfo failed.\n"); \
*!*	}
*!*  else {
IF dwRetVal != NO_ERROR
	? "Call to GetInterfaceInfo failed."
ELSE 

*!*  printf("Num Adapters: %ld\n", pInfo->NumAdapters); \
*!*	for (xj=0 ; xj < pInfo->NumAdapters ; xj++)	{
*!*		printf("Adapter Index: %ld\n", pInfo->Adapter[xj].Index); \
*!*		printf("Adapter Name: %ws\n", pInfo->Adapter[xj].Name); \
*!*		}
*!*  }
*!* free(pInfo); \
	? "Num Adapters: ", pInfo.NumAdapters
	FOR xj = 1 TO pInfo.NumAdapters && Visual FoxPro arrays are 1 based!
		? "Adapter Index:" , pInfo.Adapter[xj].Index
		? "Adapter Name :" , pInfo.Adapter[xj].mName
	ENDFOR

ENDIF
pInfo = .NULL.


? "------------------------"
? "This is GetIpAddrTable"
? "------------------------"

*!*		MIB_IPADDRTABLE		*pIPAddrTable;
LOCAL pIPAddrTable, dwSize, IPAddr, strIpAddr

*!*		pIPAddrTable = (MIB_IPADDRTABLE*) malloc( sizeof( MIB_IPADDRTABLE) ); \
*!*		ulOutBufLen = sizeof(MIB_IPADDRTABLE); \
pIPAddrTable = CREATEOBJECT('MIB_IPADDRTABLE')
ulOutBufLen = pIPAddrTable.BufferSize

*!*	dwRetVal = GetIpAddrTable(pIPAddrTable, &ulOutBufLen,0); \
*!*	if (dwRetVal == ERROR_INSUFFICIENT_BUFFER) {
*!*		free( pIPAddrTable ); \
*!*		pIPAddrTable = (MIB_IPADDRTABLE *) malloc(ulOutBufLen); \
*!*		dwRetVal = GetIpAddrTable(pIPAddrTable, &ulOutBufLen,0); \
*!*	}
dwRetVal = GetIpAddrTable(pIPAddrTable.Address,@ulOutBufLen,0)
IF dwRetVal = ERROR_INSUFFICIENT_BUFFER
	pIPAddrTable.BufferSize = ulOutBufLen
	dwRetVal = GetIpAddrTable(pIPAddrTable.Address,@ulOutBufLen,0)
ENDIF

*!*	if (dwRetVal != NO_ERROR )
*!*		printf("Call to GetIpAddrTable failed.\n"); \
*!*	else {
IF dwRetVal != NO_ERROR
	? "Call to GetIpAddrTable failed."
ELSE

*!*	for (xj=0 ; xj < pIPAddrTable->dwNumEntries ; xj++)	{
*!*		printf("Address: %ld\n", pIPAddrTable->table[xj].dwAddr); \
*!*		printf("Mask:    %ld\n", pIPAddrTable->table[xj].dwMask); \
*!*		printf("Index:   %ld\n", pIPAddrTable->table[xj].dwIndex); \
*!*		printf("BCast:   %ld\n", pIPAddrTable->table[xj].dwBCastAddr); \
*!*		printf("Reasm:   %ld\n", pIPAddrTable->table[xj].dwReasmSize); \
*!* 		}
*!*	}
*!*	free(pIPAddrTable); \
	FOR xj = 1 TO pIPAddrTable.dwNumEntries
		? "Address: ", pIPAddrTable.mTable[xj].dwAddr && table is a reserverd word .. that's why it got translated to mTable
		? "Mask:	", pIPAddrTable.mTable[xj].dwMask
		? "Index:	", pIPAddrTable.mTable[xj].dwIndex
		? "BCast:	", pIPAddrTable.mTable[xj].dwBCastAddr
		? "Reasm:	", pIPAddrTable.mTable[xj].dwReasmSize
	ENDFOR
ENDIF
pIPAddrTable = .NULL.


? "-----------------------"
? "This is GetIPStatistics"
? "-----------------------"

*!*	MIB_IPSTATS			*pStats; \
*!*	pStats = (MIB_IPSTATS*) malloc(sizeof(MIB_IPSTATS)); \
LOCAL pStats
pStats = CREATEOBJECT('MIB_IPSTATS')

*!*	if ((dwRetVal = GetIpStatistics(pStats)) != NO_ERROR) {
*!*		printf("\tError getting stats.\n"); \
*!*	}
*!* 	else {
dwRetVal = GetIpStatistics(pStats.Address)
IF dwRetVal != NO_ERROR
	? "Error getting stats"
ELSE
*!*		printf("Number of IP addresses: %ld\n", pStats->dwNumAddr); \
*!*		printf("Number of Interfaces: %ld\n", pStats->dwNumIf); \
*!*		printf("Receives: %ld\n", pStats->dwInReceives); \
*!*		printf("Out Requests: %ld\n", pStats->dwOutRequests); \
*!*		printf("Routes: %ld\n", pStats->dwNumRoutes); \
*!*		printf("Timeout Time: %ld\n", pStats->dwReasmTimeout); \
*!*		printf("In Delivers: %ld\n", pStats->dwInDelivers); \
*!*		printf("In Discards: %ld\n", pStats->dwInDiscards); \
*!*		printf("Total In: %ld\n", pStats->dwInDelivers + pStats->dwInDiscards); \
*!*		printf("In Header Errors: %ld\n", pStats->dwInHdrErrors); \
*!* }
*!* free(pStats); \
? "Number of IP addresses:", pStats.dwNumAddr
? "Number of Interfaces:", pStats.dwNumIf
? "Receives:", pStats.dwInReceives
? "Out Requests:", pStats.dwOutRequests
? "Routes:", pStats.dwNumRoutes
? "Timeout Time:", pStats.dwReasmTimeout
? "In Delivers:", pStats.dwInDelivers
? "In Discards:", pStats.dwInDiscards
? "Total In:", pStats.dwInDelivers + pStats.dwInDiscards
? "In Header Errors:", pStats.dwInHdrErrors
ENDIF
pStats = .NULL.


*!*		printf("-------------------------\n"); \
*!*		printf("This is GetTCPStatistics()\n"); \
*!*		printf("-------------------------\n"); \
*!*		MIB_TCPSTATS		*pTCPStats; \
*!*		pTCPStats = (MIB_TCPSTATS*) malloc (sizeof(MIB_TCPSTATS)); \
*!*		if ((dwRetVal = GetTcpStatistics(pTCPStats)) != NO_ERROR) 
*!*			printf("Error getting TCP Stats.\n"); \
*!*		printf("\tActive Opens: %ld\n", pTCPStats->dwActiveOpens); \
*!*		printf("\tPassive Opens: %ld\n", pTCPStats->dwPassiveOpens); \
*!*		printf("\tSegments Recv: %ld\n", pTCPStats->dwInSegs); \
*!*		printf("\tSegments Xmit: %ld\n", pTCPStats->dwOutSegs); \
*!*		printf("\tTotal # Conxs: %ld\n", pTCPStats->dwNumConns); \
*!*	}

SET LIBRARY TO


&& IPHelper structure wrappers

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
		RETURN BinMacToMac(ReadBytes(THIS.Address+404,THIS.AddressLength))
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
		WriteUInt(THIS.Address,lnNewVal)
	ENDPROC

	PROCEDURE mName_Access()
		RETURN ReadWCharArray(THIS.Address+4,128)
	ENDPROC

	PROCEDURE mName_Assign(lnNewVal)
		WriteWCharArray(THIS.Address+4,lnNewVal,128)
	ENDPROC

ENDDEFINE

DEFINE CLASS IP_ADDRESS_STRING AS Relation

	Address = 0
	SizeOf = 16
	&& structure fields
	String = .F.

	PROCEDURE Init(lnAddress)
		THIS.Address = lnAddress
	ENDPROC

	PROCEDURE String_Access()
		RETURN ReadCharArray(THIS.Address,16)
	ENDPROC

	PROCEDURE String_Assign(lnNewVal)
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

	PROCEDURE dwIndex_Access()
		RETURN ReadUInt(THIS.Address+512)
	ENDPROC

	PROCEDURE dwType_Access()
		RETURN ReadUInt(THIS.Address+516)
	ENDPROC

	PROCEDURE dwMtu_Access()
		RETURN ReadUInt(THIS.Address+520)
	ENDPROC

	PROCEDURE dwSpeed_Access()
		RETURN ReadUInt(THIS.Address+524)
	ENDPROC

	PROCEDURE dwPhysAddrLen_Access()
		RETURN ReadUInt(THIS.Address+528)
	ENDPROC

	PROCEDURE bPhysAddr_Access()
		RETURN BinMacToMac(ReadBytes(THIS.Address+532,THIS.dwPhysAddrLen))
	ENDPROC

	PROCEDURE dwAdminStatus_Access()
		RETURN ReadLogical(THIS.Address+540)
	ENDPROC

	PROCEDURE dwOperStatus_Access()
		RETURN ReadUInt(THIS.Address+544)
	ENDPROC

	PROCEDURE dwLastChange_Access()
		RETURN ReadUInt(THIS.Address+548)
	ENDPROC

	PROCEDURE dwInOctets_Access()
		RETURN ReadUInt(THIS.Address+552)
	ENDPROC

	PROCEDURE dwInUcastPkts_Access()
		RETURN ReadUInt(THIS.Address+556)
	ENDPROC

	PROCEDURE dwInNUcastPkts_Access()
		RETURN ReadUInt(THIS.Address+560)
	ENDPROC

	PROCEDURE dwInDiscards_Access()
		RETURN ReadUInt(THIS.Address+564)
	ENDPROC

	PROCEDURE dwInErrors_Access()
		RETURN ReadUInt(THIS.Address+568)
	ENDPROC

	PROCEDURE dwInUnknownProtos_Access()
		RETURN ReadUInt(THIS.Address+572)
	ENDPROC

	PROCEDURE dwOutOctets_Access()
		RETURN ReadUInt(THIS.Address+576)
	ENDPROC

	PROCEDURE dwOutUcastPkts_Access()
		RETURN ReadUInt(THIS.Address+580)
	ENDPROC

	PROCEDURE dwOutNUcastPkts_Access()
		RETURN ReadUInt(THIS.Address+584)
	ENDPROC

	PROCEDURE dwOutDiscards_Access()
		RETURN ReadUInt(THIS.Address+588)
	ENDPROC

	PROCEDURE dwOutErrors_Access()
		RETURN ReadUInt(THIS.Address+592)
	ENDPROC

	PROCEDURE dwOutQLen_Access()
		RETURN ReadUInt(THIS.Address+596)
	ENDPROC

	PROCEDURE dwDescrLen_Access()
		RETURN ReadUInt(THIS.Address+600)
	ENDPROC

	PROCEDURE bDescr_Access()
		RETURN ReadCharArray(THIS.Address+604,256)
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
		RETURN inet_ntoa(ReadUInt(THIS.Address))
	ENDPROC

	PROCEDURE dwIndex_Access()
		RETURN ReadUInt(THIS.Address+4)
	ENDPROC

	PROCEDURE dwMask_Access()
		RETURN inet_ntoa(ReadUInt(THIS.Address+8))
	ENDPROC

	PROCEDURE dwBCastAddr_Access()
		RETURN ReadUInt(THIS.Address+12)
	ENDPROC

	PROCEDURE dwReasmSize_Access()
		RETURN ReadUInt(THIS.Address+16)
	ENDPROC

	PROCEDURE unused1_Access()
		RETURN ReadUShort(THIS.Address+20)
	ENDPROC

	PROCEDURE unused2_Access()
		RETURN ReadUShort(THIS.Address+22)
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

	PROCEDURE dwDefaultTTL_Access()
		RETURN ReadUInt(THIS.Address+4)
	ENDPROC

	PROCEDURE dwInReceives_Access()
		RETURN ReadUInt(THIS.Address+8)
	ENDPROC

	PROCEDURE dwInHdrErrors_Access()
		RETURN ReadUInt(THIS.Address+12)
	ENDPROC

	PROCEDURE dwInAddrErrors_Access()
		RETURN ReadUInt(THIS.Address+16)
	ENDPROC

	PROCEDURE dwForwDatagrams_Access()
		RETURN ReadUInt(THIS.Address+20)
	ENDPROC

	PROCEDURE dwInUnknownProtos_Access()
		RETURN ReadUInt(THIS.Address+24)
	ENDPROC

	PROCEDURE dwInDiscards_Access()
		RETURN ReadUInt(THIS.Address+28)
	ENDPROC

	PROCEDURE dwInDelivers_Access()
		RETURN ReadUInt(THIS.Address+32)
	ENDPROC

	PROCEDURE dwOutRequests_Access()
		RETURN ReadUInt(THIS.Address+36)
	ENDPROC

	PROCEDURE dwRoutingDiscards_Access()
		RETURN ReadUInt(THIS.Address+40)
	ENDPROC

	PROCEDURE dwOutDiscards_Access()
		RETURN ReadUInt(THIS.Address+44)
	ENDPROC

	PROCEDURE dwOutNoRoutes_Access()
		RETURN ReadUInt(THIS.Address+48)
	ENDPROC

	PROCEDURE dwReasmTimeout_Access()
		RETURN ReadUInt(THIS.Address+52)
	ENDPROC

	PROCEDURE dwReasmReqds_Access()
		RETURN ReadUInt(THIS.Address+56)
	ENDPROC

	PROCEDURE dwReasmOks_Access()
		RETURN ReadUInt(THIS.Address+60)
	ENDPROC

	PROCEDURE dwReasmFails_Access()
		RETURN ReadUInt(THIS.Address+64)
	ENDPROC

	PROCEDURE dwFragOks_Access()
		RETURN ReadUInt(THIS.Address+68)
	ENDPROC

	PROCEDURE dwFragFails_Access()
		RETURN ReadUInt(THIS.Address+72)
	ENDPROC

	PROCEDURE dwFragCreates_Access()
		RETURN ReadUInt(THIS.Address+76)
	ENDPROC

	PROCEDURE dwNumIf_Access()
		RETURN ReadUInt(THIS.Address+80)
	ENDPROC

	PROCEDURE dwNumAddr_Access()
		RETURN ReadUInt(THIS.Address+84)
	ENDPROC

	PROCEDURE dwNumRoutes_Access()
		RETURN ReadUInt(THIS.Address+88)
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

	PROCEDURE dwRtoMin_Access()
		RETURN ReadUInt(THIS.Address+4)
	ENDPROC

	PROCEDURE dwRtoMax_Access()
		RETURN ReadUInt(THIS.Address+8)
	ENDPROC

	PROCEDURE dwMaxConn_Access()
		RETURN ReadUInt(THIS.Address+12)
	ENDPROC

	PROCEDURE dwActiveOpens_Access()
		RETURN ReadUInt(THIS.Address+16)
	ENDPROC

	PROCEDURE dwPassiveOpens_Access()
		RETURN ReadUInt(THIS.Address+20)
	ENDPROC

	PROCEDURE dwAttemptFails_Access()
		RETURN ReadUInt(THIS.Address+24)
	ENDPROC

	PROCEDURE dwEstabResets_Access()
		RETURN ReadUInt(THIS.Address+28)
	ENDPROC

	PROCEDURE dwCurrEstab_Access()
		RETURN ReadUInt(THIS.Address+32)
	ENDPROC

	PROCEDURE dwInSegs_Access()
		RETURN ReadUInt(THIS.Address+36)
	ENDPROC

	PROCEDURE dwOutSegs_Access()
		RETURN ReadUInt(THIS.Address+40)
	ENDPROC

	PROCEDURE dwRetransSegs_Access()
		RETURN ReadUInt(THIS.Address+44)
	ENDPROC

	PROCEDURE dwInErrs_Access()
		RETURN ReadUInt(THIS.Address+48)
	ENDPROC

	PROCEDURE dwOutRsts_Access()
		RETURN ReadUInt(THIS.Address+52)
	ENDPROC

	PROCEDURE dwNumConns_Access()
		RETURN ReadUInt(THIS.Address+56)
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

	PROCEDURE dwNoPorts_Access()
		RETURN ReadUInt(THIS.Address+4)
	ENDPROC

	PROCEDURE dwInErrors_Access()
		RETURN ReadUInt(THIS.Address+8)
	ENDPROC

	PROCEDURE dwOutDatagrams_Access()
		RETURN ReadUInt(THIS.Address+12)
	ENDPROC

	PROCEDURE dwNumAddrs_Access()
		RETURN ReadUInt(THIS.Address+16)
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

	PROCEDURE dwErrors_Access()
		RETURN ReadUInt(THIS.Address+4)
	ENDPROC

	PROCEDURE dwDestUnreachs_Access()
		RETURN ReadUInt(THIS.Address+8)
	ENDPROC

	PROCEDURE dwTimeExcds_Access()
		RETURN ReadUInt(THIS.Address+12)
	ENDPROC

	PROCEDURE dwParmProbs_Access()
		RETURN ReadUInt(THIS.Address+16)
	ENDPROC

	PROCEDURE dwSrcQuenchs_Access()
		RETURN ReadUInt(THIS.Address+20)
	ENDPROC

	PROCEDURE dwRedirects_Access()
		RETURN ReadUInt(THIS.Address+24)
	ENDPROC

	PROCEDURE dwEchos_Access()
		RETURN ReadUInt(THIS.Address+28)
	ENDPROC

	PROCEDURE dwEchoReps_Access()
		RETURN ReadUInt(THIS.Address+32)
	ENDPROC

	PROCEDURE dwTimestamps_Access()
		RETURN ReadUInt(THIS.Address+36)
	ENDPROC

	PROCEDURE dwTimestampReps_Access()
		RETURN ReadUInt(THIS.Address+40)
	ENDPROC

	PROCEDURE dwAddrMasks_Access()
		RETURN ReadUInt(THIS.Address+44)
	ENDPROC

	PROCEDURE dwAddrMaskReps_Access()
		RETURN ReadUInt(THIS.Address+48)
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

	PROCEDURE dwLocalAddr_Access()
		RETURN ReadUInt(THIS.Address+4)
	ENDPROC

	PROCEDURE dwLocalPort_Access()
		RETURN ReadUInt(THIS.Address+8)
	ENDPROC

	PROCEDURE dwRemoteAddr_Access()
		RETURN ReadUInt(THIS.Address+12)
	ENDPROC

	PROCEDURE dwRemotePort_Access()
		RETURN ReadUInt(THIS.Address+16)
	ENDPROC

ENDDEFINE


&& some helper functions
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




