#ifndef _VFP2CPRINT_H__
#define _VFP2CPRINT_H__

#ifdef __cplusplus
extern "C" {
#endif

const int APRINT_DEST_ARRAY			= 1;
const int APRINT_DEST_OBJECTARRAY	= 2;

const int PRINT_ENUM_BUFFER		= 2048;
const int PRINT_TRAY_BUFFER		= 24;

const int PAPERSIZE_UNIT_MM		= 1;
const int PAPERSIZE_UNIT_INCH	= 2;
const int PAPERSIZE_UNIT_POINT	= 3;

const double POINTS_PER_MM	= 0.2834645669;
const double INCH_PER_MM	= 0.039370079;

void _fastcall APrintersEx(ParamBlk *parm);
void _fastcall APrintJobs(ParamBlk *parm);
void _fastcall APrinterForms(ParamBlk *parm);
void _fastcall APaperSizes(ParamBlk *parm);
void _fastcall APrinterTrays(ParamBlk *parm);

#ifdef __cplusplus
}
#endif // end of extern "C"

#endif // _VFP2CPRINT_H__