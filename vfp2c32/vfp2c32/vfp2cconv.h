#ifndef _VFP2CCONV_H__
#define _VFP2CCONV_H__

const int HEX_PREFIX		= 1;
const int HEX_LEADINGNULLS	= 2;

#pragma pack(push,1)	// set structure padding to 1
typedef struct _PackedValue
{
	union
	{
		double ev_real;
		CCY ev_currency;
		int ev_long;
		unsigned int ev_length;
	};
	char ev_type;
	unsigned char ev_width;
	unsigned char ev_decimals;
	unsigned char ev_unused;
} PackedValue, *LPPackedValue;
#pragma pack(pop)

#ifdef __cplusplus
extern "C" {
#endif

void _fastcall PG_ByteA2Str(ParamBlkEx& parm);
void _fastcall PG_Str2ByteA(ParamBlkEx& parm);
void _fastcall RGB2Colors(ParamBlkEx& parm);
void _fastcall Colors2RGB(ParamBlkEx& parm);
void _fastcall GetCursorPosEx(ParamBlkEx& parm);

void _fastcall Int64_Add(ParamBlkEx& parm);
void _fastcall Int64_Sub(ParamBlkEx& parm);
void _fastcall Int64_Mul(ParamBlkEx& parm);
void _fastcall Int64_Div(ParamBlkEx& parm);
void _fastcall Int64_Mod(ParamBlkEx& parm);

void _fastcall Value2Variant(ParamBlkEx& parm);
void _fastcall Variant2Value(ParamBlkEx& parm);

void _fastcall Decimals(ParamBlkEx& parm);
void _fastcall Num2Binary(ParamBlkEx& parm);

void _fastcall CreatePublicShadowObjReference(ParamBlkEx& parm);
void _fastcall ReleasePublicShadowObjReference(ParamBlkEx& parm);

void _fastcall GetLocaleInfoExLib(ParamBlkEx& parm);
void _fastcall OsEx(ParamBlkEx& parm);

void _fastcall Str2Short(ParamBlkEx& parm);
void _fastcall Short2Str(ParamBlkEx& parm);
void _fastcall Str2UShort(ParamBlkEx& parm);
void _fastcall UShort2Str(ParamBlkEx& parm);
void _fastcall Str2Long(ParamBlkEx& parm);
void _fastcall Long2Str(ParamBlkEx& parm);
void _fastcall Str2ULong(ParamBlkEx& parm);
void _fastcall ULong2Str(ParamBlkEx& parm);
void _fastcall Str2Double(ParamBlkEx& parm);
void _fastcall Double2Str(ParamBlkEx& parm);
void _fastcall Str2Float(ParamBlkEx& parm);
void _fastcall Float2Str(ParamBlkEx& parm);
void _fastcall Int642Str(ParamBlkEx& parm);
void _fastcall Str2Int64(ParamBlkEx& parm);
void _fastcall UInt642Str(ParamBlkEx& parm);
void _fastcall Str2UInt64(ParamBlkEx& parm);

#ifdef __cplusplus
}
#endif

#endif // _VFP2CCONV_H__