#ifndef _VFP2CTIME_H__
#define _VFP2CTIME_H__

const int VFP2C_MAX_TIMEZONE_NAME	= 512;

typedef struct _REG_TIMEZONE_INFORMATION
{
	long Bias;
	long StandardBias;
	long DayligthBias;
	SYSTEMTIME StandardDate;
	SYSTEMTIME DayligthDate;
} REG_TIMEZONE_INFORMATION, *LPREG_TIMEZONE_INFORMATION;

#ifdef __cplusplus
extern "C" {
#endif

void _fastcall DT2FT(ParamBlk *parm);
void _fastcall FT2DT(ParamBlk *parm);
void _fastcall DT2ST(ParamBlk *parm);
void _fastcall ST2DT(ParamBlk *parm);
void _fastcall DT2UTC(ParamBlk *parm);
void _fastcall UTC2DT(ParamBlk *parm);
void _fastcall DT2Timet(ParamBlk *parm);
void _fastcall Timet2DT(ParamBlk *parm);
void _fastcall DT2Double(ParamBlk *parm);
void _fastcall Double2DT(ParamBlk *parm);
void _fastcall SetSystemTimeLib(ParamBlk *parm);
void _fastcall GetSystemTimeLib(ParamBlk *parm);
void _fastcall ATimeZones(ParamBlk *parm);

#ifdef __cplusplus
}
#endif // end of extern "C"

#endif // _VFP2CTIME_H__