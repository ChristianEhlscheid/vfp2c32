#IFNDEF _WINSOCK2_H__
#DEFINE _WINSOCK2_H__

#DEFINE AF_UNSPEC       0  && unspecified
*!* Although  AF_UNSPEC  is  defined for backwards compatibility, using
*!* AF_UNSPEC for the "af" parameter when creating a socket is STRONGLY
*!* DISCOURAGED.    The  interpretation  of  the  "protocol"  parameter
*!* depends  on the actual address family chosen.  As environments grow
*!* to  include  more  and  more  address families that use overlapping
*!* protocol  values  there  is  more  and  more  chance of choosing an
*!* undesired address family when AF_UNSPEC is used.
#DEFINE AF_UNIX         1               && local to host (pipes, portals)
#DEFINE AF_INET         2               && internetwork: UDP, TCP, etc.
#DEFINE AF_IMPLINK      3               && arpanet imp addresses 
#DEFINE AF_PUP          4               && pup protocols: e.g. BSP
#DEFINE AF_CHAOS        5               && mit CHAOS protocols
#DEFINE AF_NS           6               && XEROX NS protocols
#DEFINE AF_IPX          6	            && IPX protocols: IPX, SPX, etc.
#DEFINE AF_ISO          7               && ISO protocols
#DEFINE AF_OSI          7		        && OSI is ISO
#DEFINE AF_ECMA         8               && european computer manufacturers
#DEFINE AF_DATAKIT      9               && datakit protocols
#DEFINE AF_CCITT        10              && CCITT protocols, X.25 etc
#DEFINE AF_SNA          11              && IBM SNA
#DEFINE AF_DECnet       12              && DECnet
#DEFINE AF_DLI          13              && Direct data link interface
#DEFINE AF_LAT          14              && LAT
#DEFINE AF_HYLINK       15              && NSC Hyperchannel
#DEFINE AF_APPLETALK    16              && AppleTalk
#DEFINE AF_NETBIOS      17              && NetBios-style addresses
#DEFINE AF_VOICEVIEW    18              && VoiceView
#DEFINE AF_FIREFOX      19              && Protocols from Firefox 
#DEFINE AF_UNKNOWN1     20              && Somebody is using this!
#DEFINE AF_BAN          21              && Banyan
#DEFINE AF_ATM          22              && Native ATM Services
#DEFINE AF_INET6        23              && Internetwork Version 6
#DEFINE AF_CLUSTER      24              && Microsoft Wolfpack
#DEFINE AF_12844        25              && IEEE 1284.4 WG AF
#DEFINE AF_IRDA         26              && IrDA
#DEFINE AF_NETDES       28              && Network Designers OSI & gateway enabled protocols

#ENDIF && _WINSOCK2_H__