&& prerequisites: NONE!

CD (FULLPATH(JUSTPATH(SYS(16))))
SET LIBRARY TO vfp2c32.fll ADDITIVE

LOCAL lnNumeric, lcBinary, lnNumeric2

&& numeric values into binary string conversion & vice versa

&& short (16 bit signed integer), valid range is -32768 to 32767
lnNumeric = -3233
lcBinary = Short2Str(lnNumeric) && short to binary string
lnNumeric2 = Str2Short(lcBinary)
? lnNumeric = lnNumeric2 && always .T.

&& unsigned short (16 bit unsigned integer), valid range is 0 to 65536
lnNumeric = 64333
lcBinary = UShort2Str(lnNumeric)
lnNumeric2 = Str2UShort(lcBinary)
? lnNumeric = lnNumeric2 && always .T.

&& long (32 bit signed integer), valid range is -2147483648 to 2147483647
lnNumeric = -345345345
lcBinary = Long2Str(lnNumeric)
lnNumeric2 = Str2Long(lcBinary)
? lnNumeric = lnNumeric2 && always .T.

&& unsigned long (32 bit unsigned integer), valid range is 0 to 4294967296
lnNumeric = 4294967293
lcBinary = ULong2Str(lnNumeric)
lnNumeric2 = Str2ULong(lcBinary)
? lnNumeric = lnNumeric2 && always .T.

&& float (32 bit floating point)
lnNumeric = 3.333
lcBinary = Float2Str(lnNumeric)
lnNumeric = Str2Float(lcBinary)
? lnNumeric = lnNumeric2
&& !! ATTENTION, this is not always true, it depends on the value you pass !! 
&& it's because FoxPro actually never stores fractional numbers in 32bit format
&& instead FoxPro always uses 64bit floating point format (double)
&& the function downcast's the value to a float in Float2Str and upcast's it again in Str2Float
&& so some minial loss of precision can occur
&& use only when you can live with that!

&& double (64 bit floating point)
lnNumeric = 3.33334534534
lcBinary = Double2Str(lnNumeric)
lnNumeric2 = Str2Double(lcBinary)
? lnNumeric = lnNumeric2 && always .T.


&& RGB Color conversion functions
LOCAL lnColor, lnRed, lnGreen, lnBlue
lnColor = GETCOLOR()
&& extract individual colors from RGB value
RGB2Colors(lnColor,@lnRed,@lnGreen,@lnBlue)
&& combine individual colors to RGB value
lnColor = Colors2RGB(lnRed,lnGreen,lnBlue)


LOCAL lnX, lnY
&& get cursor position relative to complete screen
GetCursorPosEx(@lnX,@lnY)
&& get cursor position relative to current active window
GetCursorPosEx(@lnX,@lnY,.T.)
&& get cursor position relative to a specific window
GetCursorPosEx(@lnX,@lnY,.T.,_SCREEN.HWnd)
&& GetCursorPosEx(@lnX,@lnY,.T.,THISFORM.HWnd)
&& get cursor position relative to a specific named window
GetCursorPosEx(@lnX,@lnY,.T.,'Command')


&& Variant datatype in a table column.
LOCAL lcBlockSize
lcBlockSize = SET("Blocksize") 
SET BLOCKSIZE TO 0 && save some diskspace .. the variant is 14 bytes in size (plus size of string data if it's a string), default blocksize is 36 bytes ..
CREATE CURSOR variantTest(cProp C(50), cValue M NOCPTRANS)
SET BLOCKSIZE TO lcBlockSize

INSERT INTO variantTest VALUES ('Property1',Value2Variant('Foobar'))
INSERT INTO variantTest VALUES ('Property2',Value2Variant(DATE()))
INSERT INTO variantTest VALUES ('Property3',Value2Variant(DATETIME()))
INSERT INTO variantTest VALUES ('Property4',Value2Variant(1))
INSERT INTO variantTest VALUES ('Property5',Value2Variant(2.222))
INSERT INTO variantTest VALUES ('Property6',Value2Variant(NTOM(3.33)))
INSERT INTO variantTest VALUES ('Property7',Value2Variant(.F.))
INSERT INTO variantTest VALUES ('Property8',Value2Variant(.T.))

SCAN
	? variantTest.cProp, Variant2Value(@variantTest.cValue) && !! pass memo/blob fields by reference !!
ENDSCAN

USE IN SELECT('variantTest')


&& get the number of decimals of a numeric value
?"1.1,", Decimals(1.1)
?"1.12,", Decimals(1.12)
?"1.123,", Decimals(1.123)
?"1.1234,", Decimals(1.1234)
