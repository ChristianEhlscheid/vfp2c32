#ifndef _VFP2CTIME_H__
#define _VFP2CTIME_H__

const int VFP2C_MAX_TIMEZONE_NAME	= 512;

typedef DWORD(_stdcall* PGETDYNAMICTIMEZONEINFORMATION)(PDYNAMIC_TIME_ZONE_INFORMATION pTimeZoneInformation);
typedef BOOL(_stdcall* PGETTIMEZONEINFORMATIONFORYEAR)(USHORT wYear, PDYNAMIC_TIME_ZONE_INFORMATION pdtzi, LPTIME_ZONE_INFORMATION ptzi);
typedef BOOL(_stdcall* PTZSPECIFICLLOCALTIMETOSYSTEMTIMEEX)(const PDYNAMIC_TIME_ZONE_INFORMATION lpTimeZoneInformation, const SYSTEMTIME* lpLocalTime, SYSTEMTIME* lpUniversalTime);
typedef BOOL(_stdcall* PSYSTEMTIMETOTZSPECIFICLOCALTIMEEX)(const PDYNAMIC_TIME_ZONE_INFORMATION lpTimeZoneInformation, const SYSTEMTIME* lpUniversalTime, SYSTEMTIME* lpLocalTime);

typedef struct _REG_TIMEZONE_INFORMATION
{
	long Bias;
	long StandardBias;
	long DayligthBias;
	SYSTEMTIME StandardDate;
	SYSTEMTIME DayligthDate;
} REG_TIMEZONE_INFORMATION, *LPREG_TIMEZONE_INFORMATION;

class TimeZoneInfo
{
public:
	static TimeZoneInfo& GetTsi();
	void ToUTC(double& pDateTime);
	void ToLocal(double& pDateTime);

private:
	TimeZoneInfo();
	~TimeZoneInfo();
	static LRESULT _stdcall TimeChangeWindowProc(HWND nHwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void Refresh();

	double m_ZoneBias;
	DWORD m_CurrentZone;
	TIME_ZONE_INFORMATION m_ZoneInfo;
	DYNAMIC_TIME_ZONE_INFORMATION m_DynamicZoneInfo;
	HWND m_Hwnd;
	ATOM m_Atom;
};

#ifdef __cplusplus
extern "C" {
#endif

void _fastcall DT2FT(ParamBlkEx& parm);
void _fastcall FT2DT(ParamBlkEx& parm);
void _fastcall DT2ST(ParamBlkEx& parm);
void _fastcall ST2DT(ParamBlkEx& parm);
void _fastcall DT2UTC(ParamBlkEx& parm);
void _fastcall UTC2DT(ParamBlkEx& parm);
void _fastcall DT2Timet(ParamBlkEx& parm);
void _fastcall Timet2DT(ParamBlkEx& parm);
void _fastcall DT2Double(ParamBlkEx& parm);
void _fastcall Double2DT(ParamBlkEx& parm);
void _fastcall SetSystemTimeLib(ParamBlkEx& parm);
void _fastcall GetSystemTimeLib(ParamBlkEx& parm);
void _fastcall ATimeZones(ParamBlkEx& parm);

#ifdef __cplusplus
}
#endif // end of extern "C"

#endif // _VFP2CTIME_H__