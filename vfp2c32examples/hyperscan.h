#IFNDEF _HYPERSCAN_H__
#DEFINE _HYPERSCAN_H__

#DEFINE HS_FLAG_CASELESS        1
#DEFINE HS_FLAG_DOTALL          2
#DEFINE HS_FLAG_MULTILINE       4
#DEFINE HS_FLAG_SINGLEMATCH     8
#DEFINE HS_FLAG_ALLOWEMPTY      16
#DEFINE HS_FLAG_UTF8            32
#DEFINE HS_FLAG_UCP             64
#DEFINE HS_FLAG_PREFILTER       128
#DEFINE HS_FLAG_SOM_LEFTMOST    256
#DEFINE HS_FLAG_COMBINATION     512
#DEFINE HS_FLAG_QUIET           1024

#DEFINE HS_MODE_BLOCK           1
#DEFINE HS_MODE_STREAM          2
#DEFINE HS_MODE_SOM_HORIZON_LARGE   16777216
#DEFINE HS_MODE_SOM_HORIZON_MEDIUM  33554432
#DEFINE HS_MODE_SOM_HORIZON_SMALL   67108864

&& extended flags
#DEFINE HS_MODE_EX_LINENO				0x00000100
#DEFINE HS_MODE_EX_MATCH_ID				0x00000200
#DEFINE HS_MODE_EX_LITERAL				0x00000400
#DEFINE HS_MODE_EX_SKIP_OVERLAPPED_EXPR	0x00000800
#DEFINE HS_MODE_EX_STORE_SKIPPED_FILES	0x00001000

&& platform compilation flags
#DEFINE HS_CPU_FEATURES_AVX2            0x0004 	&& platform supports AVX2 instructions.
#DEFINE HS_CPU_FEATURES_AVX512			0x0008	&& platform supports AVX512 instructions, specifically AVX-512BW. Using AVX512 implies the use of AVX2.
#DEFINE HS_CPU_FEATURES_AVX512VBMI      0x0010	&& platform supports AVX512VBMI instructions. Using AVX512VBMI implies the use of AVX512.

#DEFINE HS_TUNE_FAMILY_GENERIC			0		&& not tune for any particular target platform
#DEFINE HS_TUNE_FAMILY_SNB				1		&& tune for the Sandy Bridge microarchitecture.
#DEFINE HS_TUNE_FAMILY_IVB				2		&& tune for the Ivy Bridge microarchitecture.
#DEFINE HS_TUNE_FAMILY_HSW				3		&& tune for the Haswell microarchitecture.
#DEFINE HS_TUNE_FAMILY_SLM				4		&& tune for the Silvermont microarchitecture.
#DEFINE HS_TUNE_FAMILY_BDW				5		&& tune for the Broadwell microarchitecture.
#DEFINE HS_TUNE_FAMILY_SKL				6		&& tune for the Skylake microarchitecture.
#DEFINE HS_TUNE_FAMILY_SKX				7		&& tune for the Skylake Server microarchitecture.
#DEFINE HS_TUNE_FAMILY_GLM				8		&& tune for the Goldmont microarchitecture.
#DEFINE HS_TUNE_FAMILY_ICL				9		&& tune for the Icelake microarchitecture.
#DEFINE HS_TUNE_FAMILY_ICX				10		&& tune for the Icelake Server microarchitecture.

&& error codes
#DEFINE HS_SUCCESS				(0)
#DEFINE HS_INVALID				(-1)
#DEFINE HS_NOMEM				(-2)
#DEFINE HS_SCAN_TERMINATED		(-3)
#DEFINE HS_COMPILER_ERROR		(-4)
#DEFINE HS_DB_VERSION_ERROR		(-5)
#DEFINE HS_DB_PLATFORM_ERROR	(-6)
#DEFINE HS_DB_MODE_ERROR		(-7)
#DEFINE HS_BAD_ALIGN			(-8)
#DEFINE HS_BAD_ALLOC			(-9)
#DEFINE HS_SCRATCH_IN_USE		(-10)
#DEFINE HS_ARCH_ERROR			(-11)
#DEFINE HS_INSUFFICIENT_SPACE	(-12)
#DEFINE HS_UNKNOWN_ERROR		(-13)

#ENDIF