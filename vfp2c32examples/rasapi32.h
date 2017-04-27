#IFNDEF _RAS32_H__
#DEFINE _RAS32_H__

&& RasPhonebookDlgEx flags
#DEFINE RASPBDFLAG_PositionDlg      0x00000001
#DEFINE RASPBDFLAG_ForceCloseOnDial 0x00000002
#DEFINE RASPBDFLAG_NoUser           0x00000010
#DEFINE RASPBDFLAG_UpdateDefaults   0x80000000

&& RasPhonebookDlgEx callback event types
#DEFINE RASPBDEVENT_AddEntry    1
#DEFINE RASPBDEVENT_EditEntry   2
#DEFINE RASPBDEVENT_RemoveEntry 3
#DEFINE RASPBDEVENT_DialEntry   4
#DEFINE RASPBDEVENT_EditGlobals 5
#DEFINE RASPBDEVENT_NoUser      6
#DEFINE RASPBDEVENT_NoUserEdit  7

&& enum RASCONNSTATE, used in callback function of asyncronous RasDialEx
#DEFINE RASCS_OpenPort	0
#DEFINE RASCS_PortOpened	1
#DEFINE RASCS_ConnectDevice	2
#DEFINE RASCS_DeviceConnected	3
#DEFINE RASCS_AllDevicesConnected	4
#DEFINE RASCS_Authenticate	5
#DEFINE RASCS_AuthNotify	6
#DEFINE RASCS_AuthRetry	7
#DEFINE RASCS_AuthCallback	8
#DEFINE RASCS_AuthChangePassword	9
#DEFINE RASCS_AuthProject	10
#DEFINE RASCS_AuthLinkSpeed	11
#DEFINE RASCS_AuthAck	12
#DEFINE RASCS_ReAuthenticate	13
#DEFINE RASCS_Authenticated	14
#DEFINE RASCS_PrepareForCallback	15
#DEFINE RASCS_WaitForModemReset	16
#DEFINE RASCS_WaitForCallback	17
#DEFINE RASCS_Projected	18
#DEFINE RASCS_StartAuthentication	19
#DEFINE RASCS_CallbackComplete	20
#DEFINE RASCS_LogonNetwork	21
#DEFINE RASCS_SubEntryConnected	22
#DEFINE RASCS_SubEntryDisconnected	23
#DEFINE RASCS_Interactive	4096
#DEFINE RASCS_RetryAuthentication	4097
#DEFINE RASCS_CallbackSetByCaller	4098
#DEFINE RASCS_PasswordExpired	4099
#DEFINE RASCS_InvokeEapUI	4100
#DEFINE RASCS_Connected	8192
#DEFINE RASCS_Disconnected	8193

&& RasDialEx flags
#DEFINE RDEOPT_UsePrefixSuffix           0x00000001
#DEFINE RDEOPT_PausedStates              0x00000002
#DEFINE RDEOPT_IgnoreModemSpeaker        0x00000004
#DEFINE RDEOPT_SetModemSpeaker           0x00000008
#DEFINE RDEOPT_IgnoreSoftwareCompression 0x00000010
#DEFINE RDEOPT_SetSoftwareCompression    0x00000020
#DEFINE RDEOPT_DisableConnectedUI        0x00000040
#DEFINE RDEOPT_DisableReconnectUI        0x00000080
#DEFINE RDEOPT_DisableReconnect          0x00000100
#DEFINE RDEOPT_NoUser                    0x00000200
#DEFINE RDEOPT_PauseOnScript             0x00000400
#DEFINE RDEOPT_Router                    0x00000800
#DEFINE RDEOPT_CustomDial                0x00001000
#DEFINE RDEOPT_UseCustomScripting        0x00002000

&& used in ARasPhonebookEntries
&& indicates that the phonebook to which this entry
&& belongs is a system phonebook.
#DEFINE REN_User                         0x00000000
#DEFINE REN_AllUsers                     0x00000001

&& flags for RasConnectionNotificationEx (notification reasons)
#DEFINE RASCN_Connection        0x00000001
#DEFINE RASCN_Disconnection     0x00000002
#DEFINE RASCN_BandwidthAdded    0x00000004
#DEFINE RASCN_BandwidthRemoved  0x00000008

#ENDIF && _RAS32_H__