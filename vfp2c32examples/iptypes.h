#IFNDEF _IPTYPES_H__
#DEFINE _IPTYPES_H__

&& IP_PREFIX_ORIGIN enum
#DEFINE IpPrefixOriginOther	0
#DEFINE IpPrefixOriginManual	1
#DEFINE IpPrefixOriginWellKnown	2
#DEFINE IpPrefixOriginDhcp	3
#DEFINE IpPrefixOriginRouterAdvertisement	4

&& IP_SUFFIX_ORIGIN enum
#DEFINE IpSuffixOriginOther	0
#DEFINE IpSuffixOriginManual	1
#DEFINE IpSuffixOriginWellKnown	2
#DEFINE IpSuffixOriginDhcp	3
#DEFINE IpSuffixOriginLinkLayerAddress	4
#DEFINE IpSuffixOriginRandom	5

&& IP_DAD_STATE enum
#DEFINE IpDadStateInvalid	0
#DEFINE IpDadStateTentative	1
#DEFINE IpDadStateDuplicate	2
#DEFINE IpDadStateDeprecated	3
#DEFINE IpDadStatePreferred	4

&& IF_OPER_STATUS enum
#DEFINE IfOperStatusUp	1
#DEFINE IfOperStatusDown	2
#DEFINE IfOperStatusTesting	3
#DEFINE IfOperStatusUnknown	4
#DEFINE IfOperStatusDormant	5
#DEFINE IfOperStatusNotPresent	6
#DEFINE IfOperStatusLowerLayerDown	7

&& SCOPE_LEVEL enum
#DEFINE ScopeLevelInterface	1
#DEFINE ScopeLevelLink	2
#DEFINE ScopeLevelSubnet	3
#DEFINE ScopeLevelAdmin	4
#DEFINE ScopeLevelSite	5
#DEFINE ScopeLevelOrganization	8
#DEFINE ScopeLevelGlobal	14

&& Per-address Flags
#define IP_ADAPTER_ADDRESS_DNS_ELIGIBLE 0x01
#define IP_ADAPTER_ADDRESS_TRANSIENT    0x02

&& Per-adapter Flags
#DEFINE IP_ADAPTER_DDNS_ENABLED               0x01
#DEFINE IP_ADAPTER_REGISTER_ADAPTER_SUFFIX    0x02
#DEFINE IP_ADAPTER_DHCP_ENABLED               0x04
#DEFINE IP_ADAPTER_RECEIVE_ONLY               0x08
#DEFINE IP_ADAPTER_NO_MULTICAST               0x10
#DEFINE IP_ADAPTER_IPV6_OTHER_STATEFUL_CONFIG 0x20

*!* Flags used as argument to GetAdaptersAddresses().
*!* "SKIP" flags are added when the default is to include the information.
*!* "INCLUDE" flags are added when the default is to skip the information.
#DEFINE GAA_FLAG_SKIP_UNICAST       0x0001
#DEFINE GAA_FLAG_SKIP_ANYCAST       0x0002
#DEFINE GAA_FLAG_SKIP_MULTICAST     0x0004
#DEFINE GAA_FLAG_SKIP_DNS_SERVER    0x0008
#DEFINE GAA_FLAG_INCLUDE_PREFIX     0x0010
#DEFINE GAA_FLAG_SKIP_FRIENDLY_NAME 0x0020

*!* Media types
#DEFINE MIN_IF_TYPE                     1
#DEFINE IF_TYPE_OTHER                   1   && None of the below
#DEFINE IF_TYPE_REGULAR_1822            2
#DEFINE IF_TYPE_HDH_1822                3
#DEFINE IF_TYPE_DDN_X25                 4
#DEFINE IF_TYPE_RFC877_X25              5
#DEFINE IF_TYPE_ETHERNET_CSMACD         6
#DEFINE IF_TYPE_IS088023_CSMACD         7
#DEFINE IF_TYPE_ISO88024_TOKENBUS       8
#DEFINE IF_TYPE_ISO88025_TOKENRING      9
#DEFINE IF_TYPE_ISO88026_MAN            10
#DEFINE IF_TYPE_STARLAN                 11
#DEFINE IF_TYPE_PROTEON_10MBIT          12
#DEFINE IF_TYPE_PROTEON_80MBIT          13
#DEFINE IF_TYPE_HYPERCHANNEL            14
#DEFINE IF_TYPE_FDDI                    15
#DEFINE IF_TYPE_LAP_B                   16
#DEFINE IF_TYPE_SDLC                    17
#DEFINE IF_TYPE_DS1                     18  && DS1-MIB
#DEFINE IF_TYPE_E1                      19  && Obsolete; see DS1-MIB
#DEFINE IF_TYPE_BASIC_ISDN              20
#DEFINE IF_TYPE_PRIMARY_ISDN            21
#DEFINE IF_TYPE_PROP_POINT2POINT_SERIAL 22  && proprietary serial
#DEFINE IF_TYPE_PPP                     23
#DEFINE IF_TYPE_SOFTWARE_LOOPBACK       24
#DEFINE IF_TYPE_EON                     25  && CLNP over IP
#DEFINE IF_TYPE_ETHERNET_3MBIT          26
#DEFINE IF_TYPE_NSIP                    27  && XNS over IP
#DEFINE IF_TYPE_SLIP                    28  && Generic Slip
#DEFINE IF_TYPE_ULTRA                   29  && ULTRA Technologies
#DEFINE IF_TYPE_DS3                     30  && DS3-MIB
#DEFINE IF_TYPE_SIP                     31  && SMDS, coffee
#DEFINE IF_TYPE_FRAMERELAY              32  && DTE only
#DEFINE IF_TYPE_RS232                   33
#DEFINE IF_TYPE_PARA                    34  && Parallel port
#DEFINE IF_TYPE_ARCNET                  35
#DEFINE IF_TYPE_ARCNET_PLUS             36
#DEFINE IF_TYPE_ATM                     37  && ATM cells
#DEFINE IF_TYPE_MIO_X25                 38
#DEFINE IF_TYPE_SONET                   39  && SONET or SDH
#DEFINE IF_TYPE_X25_PLE                 40
#DEFINE IF_TYPE_ISO88022_LLC            41
#DEFINE IF_TYPE_LOCALTALK               42
#DEFINE IF_TYPE_SMDS_DXI                43
#DEFINE IF_TYPE_FRAMERELAY_SERVICE      44  && FRNETSERV-MIB
#DEFINE IF_TYPE_V35                     45
#DEFINE IF_TYPE_HSSI                    46
#DEFINE IF_TYPE_HIPPI                   47
#DEFINE IF_TYPE_MODEM                   48  && Generic Modem
#DEFINE IF_TYPE_AAL5                    49  && AAL5 over ATM
#DEFINE IF_TYPE_SONET_PATH              50
#DEFINE IF_TYPE_SONET_VT                51
#DEFINE IF_TYPE_SMDS_ICIP               52  && SMDS InterCarrier Interface
#DEFINE IF_TYPE_PROP_VIRTUAL            53  && Proprietary virtual/internal
#DEFINE IF_TYPE_PROP_MULTIPLEXOR        54  && Proprietary multiplexing
#DEFINE IF_TYPE_IEEE80212               55  && 100BaseVG
#DEFINE IF_TYPE_FIBRECHANNEL            56
#DEFINE IF_TYPE_HIPPIINTERFACE          57
#DEFINE IF_TYPE_FRAMERELAY_INTERCONNECT 58  && Obsolete, use 32 or 44
#DEFINE IF_TYPE_AFLANE_8023             59  && ATM Emulated LAN for 802.3
#DEFINE IF_TYPE_AFLANE_8025             60  && ATM Emulated LAN for 802.5
#DEFINE IF_TYPE_CCTEMUL                 61  && ATM Emulated circuit
#DEFINE IF_TYPE_FASTETHER               62  && Fast Ethernet (100BaseT)
#DEFINE IF_TYPE_ISDN                    63  && ISDN and X.25
#DEFINE IF_TYPE_V11                     64  && CCITT V.11/X.21
#DEFINE IF_TYPE_V36                     65  && CCITT V.36
#DEFINE IF_TYPE_G703_64K                66  && CCITT G703 at 64Kbps
#DEFINE IF_TYPE_G703_2MB                67  && Obsolete; see DS1-MIB
#DEFINE IF_TYPE_QLLC                    68  && SNA QLLC
#DEFINE IF_TYPE_FASTETHER_FX            69  && Fast Ethernet (100BaseFX)
#DEFINE IF_TYPE_CHANNEL                 70
#DEFINE IF_TYPE_IEEE80211               71  && Radio spread spectrum
#DEFINE IF_TYPE_IBM370PARCHAN           72  && IBM System 360/370 OEMI Channel
#DEFINE IF_TYPE_ESCON                   73  && IBM Enterprise Systems Connection
#DEFINE IF_TYPE_DLSW                    74  && Data Link Switching
#DEFINE IF_TYPE_ISDN_S                  75  && ISDN S/T interface
#DEFINE IF_TYPE_ISDN_U                  76  && ISDN U interface
#DEFINE IF_TYPE_LAP_D                   77  && Link Access Protocol D
#DEFINE IF_TYPE_IPSWITCH                78  && IP Switching Objects
#DEFINE IF_TYPE_RSRB                    79  && Remote Source Route Bridging
#DEFINE IF_TYPE_ATM_LOGICAL             80  && ATM Logical Port
#DEFINE IF_TYPE_DS0                     81  && Digital Signal Level 0
#DEFINE IF_TYPE_DS0_BUNDLE              82  && Group of ds0s on the same ds1
#DEFINE IF_TYPE_BSC                     83  && Bisynchronous Protocol
#DEFINE IF_TYPE_ASYNC                   84  && Asynchronous Protocol
#DEFINE IF_TYPE_CNR                     85  && Combat Net Radio
#DEFINE IF_TYPE_ISO88025R_DTR           86  && ISO 802.5r DTR
#DEFINE IF_TYPE_EPLRS                   87  && Ext Pos Loc Report Sys
#DEFINE IF_TYPE_ARAP                    88  && Appletalk Remote Access Protocol
#DEFINE IF_TYPE_PROP_CNLS               89  && Proprietary Connectionless Proto
#DEFINE IF_TYPE_HOSTPAD                 90  && CCITT-ITU X.29 PAD Protocol
#DEFINE IF_TYPE_TERMPAD                 91  && CCITT-ITU X.3 PAD Facility
#DEFINE IF_TYPE_FRAMERELAY_MPI          92  && Multiproto Interconnect over FR
#DEFINE IF_TYPE_X213                    93  && CCITT-ITU X213
#DEFINE IF_TYPE_ADSL                    94  && Asymmetric Digital Subscrbr Loop
#DEFINE IF_TYPE_RADSL                   95  && Rate-Adapt Digital Subscrbr Loop
#DEFINE IF_TYPE_SDSL                    96  && Symmetric Digital Subscriber Loop
#DEFINE IF_TYPE_VDSL                    97  && Very H-Speed Digital Subscrb Loop
#DEFINE IF_TYPE_ISO88025_CRFPRINT       98  && ISO 802.5 CRFP
#DEFINE IF_TYPE_MYRINET                 99  && Myricom Myrinet
#DEFINE IF_TYPE_VOICE_EM                100 && Voice recEive and transMit
#DEFINE IF_TYPE_VOICE_FXO               101 && Voice Foreign Exchange Office
#DEFINE IF_TYPE_VOICE_FXS               102 && Voice Foreign Exchange Station
#DEFINE IF_TYPE_VOICE_ENCAP             103 && Voice encapsulation
#DEFINE IF_TYPE_VOICE_OVERIP            104 && Voice over IP encapsulation
#DEFINE IF_TYPE_ATM_DXI                 105 && ATM DXI
#DEFINE IF_TYPE_ATM_FUNI                106 && ATM FUNI
#DEFINE IF_TYPE_ATM_IMA                 107 && ATM IMA
#DEFINE IF_TYPE_PPPMULTILINKBUNDLE      108 && PPP Multilink Bundle
#DEFINE IF_TYPE_IPOVER_CDLC             109 && IBM ipOverCdlc
#DEFINE IF_TYPE_IPOVER_CLAW             110 && IBM Common Link Access to Workstn
#DEFINE IF_TYPE_STACKTOSTACK            111 && IBM stackToStack
#DEFINE IF_TYPE_VIRTUALIPADDRESS        112 && IBM VIPA
#DEFINE IF_TYPE_MPC                     113 && IBM multi-proto channel support
#DEFINE IF_TYPE_IPOVER_ATM              114 && IBM ipOverAtm
#DEFINE IF_TYPE_ISO88025_FIBER          115 && ISO 802.5j Fiber Token Ring
#DEFINE IF_TYPE_TDLC                    116 && IBM twinaxial data link control
#DEFINE IF_TYPE_GIGABITETHERNET         117
#DEFINE IF_TYPE_HDLC                    118
#DEFINE IF_TYPE_LAP_F                   119
#DEFINE IF_TYPE_V37                     120
#DEFINE IF_TYPE_X25_MLP                 121 && Multi-Link Protocol
#DEFINE IF_TYPE_X25_HUNTGROUP           122 && X.25 Hunt Group
#DEFINE IF_TYPE_TRANSPHDLC              123
#DEFINE IF_TYPE_INTERLEAVE              124 && Interleave channel
#DEFINE IF_TYPE_FAST                    125 && Fast channel
#DEFINE IF_TYPE_IP                      126 && IP (for APPN HPR in IP networks)
#DEFINE IF_TYPE_DOCSCABLE_MACLAYER      127 && CATV Mac Layer
#DEFINE IF_TYPE_DOCSCABLE_DOWNSTREAM    128 && CATV Downstream interface
#DEFINE IF_TYPE_DOCSCABLE_UPSTREAM      129 && CATV Upstream interface
#DEFINE IF_TYPE_A12MPPSWITCH            130 && Avalon Parallel Processor
#DEFINE IF_TYPE_TUNNEL                  131 && Encapsulation interface
#DEFINE IF_TYPE_COFFEE                  132 && Coffee pot
#DEFINE IF_TYPE_CES                     133 && Circuit Emulation Service
#DEFINE IF_TYPE_ATM_SUBINTERFACE        134 && ATM Sub Interface
#DEFINE IF_TYPE_L2_VLAN                 135 && Layer 2 Virtual LAN using 802.1Q
#DEFINE IF_TYPE_L3_IPVLAN               136 && Layer 3 Virtual LAN using IP
#DEFINE IF_TYPE_L3_IPXVLAN              137 && Layer 3 Virtual LAN using IPX
#DEFINE IF_TYPE_DIGITALPOWERLINE        138 && IP over Power Lines
#DEFINE IF_TYPE_MEDIAMAILOVERIP         139 && Multimedia Mail over IP
#DEFINE IF_TYPE_DTM                     140 && Dynamic syncronous Transfer Mode
#DEFINE IF_TYPE_DCN                     141 && Data Communications Network
#DEFINE IF_TYPE_IPFORWARD               142 && IP Forwarding Interface
#DEFINE IF_TYPE_MSDSL                   143 && Multi-rate Symmetric DSL
#DEFINE IF_TYPE_IEEE1394                144 && IEEE1394 High Perf Serial Bus
#DEFINE IF_TYPE_RECEIVE_ONLY            145 && TV adapter type
#DEFINE MAX_IF_TYPE                     145

&& Access types
#DEFINE IF_ACCESS_LOOPBACK              1
#DEFINE IF_ACCESS_BROADCAST             2
#DEFINE IF_ACCESS_POINTTOPOINT          3
#DEFINE IF_ACCESS_POINTTOMULTIPOINT     4

&& Interface Capabilities (bit flags)
#DEFINE IF_CHECK_NONE                   0x00
#DEFINE IF_CHECK_MCAST                  0x01
#DEFINE IF_CHECK_SEND                   0x02


&& Connection Types
#DEFINE IF_CONNECTION_DEDICATED         1
#DEFINE IF_CONNECTION_PASSIVE           2
#DEFINE IF_CONNECTION_DEMAND            3

#DEFINE IF_ADMIN_STATUS_UP              1
#DEFINE IF_ADMIN_STATUS_DOWN            2
#DEFINE IF_ADMIN_STATUS_TESTING         3

&& The following are the the operational states for WAN and LAN interfaces.
&& The order of the states seems weird, but is done for a purpose. All
&& states >= CONNECTED can transmit data right away. States >= DISCONNECTED
&& can tx data but some set up might be needed. States < DISCONNECTED can
&& not transmit data.
&& A card is marked UNREACHABLE if DIM calls InterfaceUnreachable for
&& reasons other than failure to connect.
&& NON_OPERATIONAL -- Valid for LAN Interfaces. Means the card is not
&&                      working or not plugged in or has no address.
&& UNREACHABLE     -- Valid for WAN Interfaces. Means the remote site is
&&                      not reachable at this time.
&& DISCONNECTED    -- Valid for WAN Interfaces. Means the remote site is
&&                      not connected at this time.
&& CONNECTING      -- Valid for WAN Interfaces. Means a connection attempt
&&                      has been initiated to the remote site.
&& CONNECTED       -- Valid for WAN Interfaces. Means the remote site is connected.
&& OPERATIONAL     -- Valid for LAN Interfaces. Means the card is plugged in and working.                                     &&
&& It is the users duty to convert these values to MIB-II values if they
&& are to be used by a subagent

#DEFINE IF_OPER_STATUS_NON_OPERATIONAL  0
#DEFINE IF_OPER_STATUS_UNREACHABLE      1
#DEFINE IF_OPER_STATUS_DISCONNECTED     2
#DEFINE IF_OPER_STATUS_CONNECTING       3
#DEFINE IF_OPER_STATUS_CONNECTED        4
#DEFINE IF_OPER_STATUS_OPERATIONAL      5

#DEFINE MIB_IF_TYPE_OTHER               1
#DEFINE MIB_IF_TYPE_ETHERNET            6
#DEFINE MIB_IF_TYPE_TOKENRING           9
#DEFINE MIB_IF_TYPE_FDDI                15
#DEFINE MIB_IF_TYPE_PPP                 23
#DEFINE MIB_IF_TYPE_LOOPBACK            24
#DEFINE MIB_IF_TYPE_SLIP                28

#DEFINE MIB_IF_ADMIN_STATUS_UP          1
#DEFINE MIB_IF_ADMIN_STATUS_DOWN        2
#DEFINE MIB_IF_ADMIN_STATUS_TESTING     3

#DEFINE MIB_IF_OPER_STATUS_NON_OPERATIONAL      0
#DEFINE MIB_IF_OPER_STATUS_UNREACHABLE          1
#DEFINE MIB_IF_OPER_STATUS_DISCONNECTED         2
#DEFINE MIB_IF_OPER_STATUS_CONNECTING           3
#DEFINE MIB_IF_OPER_STATUS_CONNECTED            4
#DEFINE MIB_IF_OPER_STATUS_OPERATIONAL          5

#DEFINE MIB_TCP_STATE_CLOSED            1
#DEFINE MIB_TCP_STATE_LISTEN            2
#DEFINE MIB_TCP_STATE_SYN_SENT          3
#DEFINE MIB_TCP_STATE_SYN_RCVD          4
#DEFINE MIB_TCP_STATE_ESTAB             5
#DEFINE MIB_TCP_STATE_FIN_WAIT1         6
#DEFINE MIB_TCP_STATE_FIN_WAIT2         7
#DEFINE MIB_TCP_STATE_CLOSE_WAIT        8
#DEFINE MIB_TCP_STATE_CLOSING           9
#DEFINE MIB_TCP_STATE_LAST_ACK         10
#DEFINE MIB_TCP_STATE_TIME_WAIT        11
#DEFINE MIB_TCP_STATE_DELETE_TCB       12

#ENDIF && _IPTYPES_H__
