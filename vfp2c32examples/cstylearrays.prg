#INCLUDE vfp2c.h

CD (FULLPATH(JUSTPATH(SYS(16))))

&& initialize the library
SET LIBRARY TO vfp2c32.fll ADDITIVE
INITVFP2C32(VFP2C_INIT_MARSHAL)

&& this prg contains the array classes
SET PROCEDURE TO vfp2carray.prg ADDITIVE
SET SAFETY OFF

&& examples for marshaling arrays

&& create sample data
CREATE CURSOR sourceCursor(intField I, intField2 N(10), doubleField B)
CREATE CURSOR targetCursor(intField I, intField2 N(10), doubleField B)
FOR xj = 1 TO 20
	INSERT INTO sourceCursor VALUES (xj, xj * 10, xj + xj / 10)
ENDFOR

&& create the C style array class of the correct type
LOCAL loCArray
*!*	loCArray = CREATEOBJECT('CShortArray')
*!*	loCArray.MarshalCursor('sourceCursor.intField')
*!*	loCArray.UnMarshalCursor('targetCursor.intField')
*!*	SELECT targetCursor
*!*	BROWSE 
*!*	ZAP IN targetCursor

*!*	loCArray = CREATEOBJECT('CUShortArray')
*!*	loCArray.MarshalCursor('sourceCursor.intField')
*!*	loCArray.UnMarshalCursor('targetCursor.intField')
*!*	SELECT targetCursor
*!*	BROWSE 
*!*	ZAP IN targetCursor

*!*	loCArray = CREATEOBJECT('CIntArray')
*!*	loCArray.MarshalCursor('sourceCursor.intField')
*!*	loCArray.UnMarshalCursor('targetCursor.intField')
*!*	SELECT targetCursor
*!*	BROWSE 
*!*	ZAP IN targetCursor

*!*	loCArray = CREATEOBJECT('CUIntArray')
*!*	loCArray.MarshalCursor('sourceCursor.intField2')
*!*	loCArray.UnMarshalCursor('targetCursor.intField2')
*!*	SELECT targetCursor
*!*	BROWSE 
*!*	ZAP IN targetCursor

*!*	loCArray = CREATEOBJECT('CFloatArray')
*!*	loCArray.MarshalCursor('sourceCursor.doubleField')
*!*	loCArray.UnMarshalCursor('targetCursor.doubleField')
*!*	SELECT targetCursor
*!*	BROWSE 
*!*	ZAP IN targetCursor

*!*	loCArray = CREATEOBJECT('CDoubleArray')
*!*	loCArray.MarshalCursor('sourceCursor.doubleField')
*!*	loCArray.UnMarshalCursor('targetCursor.doubleField')
*!*	SELECT targetCursor
*!*	BROWSE 
*!*	ZAP IN targetCursor

*!*	loCArray = CREATEOBJECT('C2DimShortArray')
*!*	loCArray.MarshalCursor('sourceCursor.intField, intField2')
*!*	loCArray.UnMarshalCursor('targetCursor.intField, intField2')
*!*	SELECT targetCursor
*!*	BROWSE 
*!*	ZAP IN targetCursor

*!*	loCArray = CREATEOBJECT('C2DimUShortArray')
*!*	loCArray.MarshalCursor('sourceCursor.intField, intField2')
*!*	loCArray.UnMarshalCursor('targetCursor.intField, intField2')
*!*	SELECT targetCursor
*!*	BROWSE 
*!*	ZAP IN targetCursor

*!*	loCArray = CREATEOBJECT('C2DimIntArray')
*!*	loCArray.MarshalCursor('sourceCursor.intField, intField2')
*!*	loCArray.UnMarshalCursor('targetCursor.intField, intField2')
*!*	SELECT targetCursor
*!*	BROWSE 
*!*	ZAP IN targetCursor

*!*	loCArray = CREATEOBJECT('C2DimUIntArray')
*!*	loCArray.MarshalCursor('sourceCursor.intField, intField2')
*!*	loCArray.UnMarshalCursor('targetCursor.intField, intField2')
*!*	SELECT targetCursor
*!*	BROWSE 
*!*	ZAP IN targetCursor

*!*	loCArray = CREATEOBJECT('C2DimFloatArray')
*!*	loCArray.MarshalCursor('sourceCursor.doubleField, intField2')
*!*	loCArray.UnMarshalCursor('targetCursor.doubleField, intField2')
*!*	SELECT targetCursor
*!*	BROWSE 
*!*	ZAP IN targetCursor

*!*	loCArray = CREATEOBJECT('C2DimDoubleArray')
*!*	loCArray.MarshalCursor('sourceCursor.doubleField, intField2')
*!*	loCArray.UnMarshalCursor('targetCursor.doubleField, intField2')
*!*	SELECT targetCursor
*!*	BROWSE 
*!*	ZAP IN targetCursor

CREATE CURSOR sourceCursor(varcharfield V(254), varcharfield2 V(254))
CREATE CURSOR targetCursor(varcharfield V(254), varcharfield2 V(254))
FOR xj = 1 TO 20
	INSERT INTO sourceCursor VALUES ('Col 1: ' + ALLTRIM(STR(xj)), 'Col 2: ' + ALLTRIM(STR(xj)))
ENDFOR

loCArray = CREATEOBJECT('CStringArray')
loCArray.MarshalCursor('sourceCursor.varcharfield')
loCArray.UnMarshalCursor('targetCursor.varcharfield')
SELECT targetCursor
BROWSE 
ZAP IN targetCursor

loCArray = CREATEOBJECT('C2DimStringArray')
loCArray.MarshalCursor('sourceCursor.varcharfield, varcharfield2')
loCArray.UnMarshalCursor('targetCursor.varcharfield, varcharfield2')
SELECT targetCursor
BROWSE 
ZAP IN targetCursor

&& loCArray = CREATEOBJECT('CShortArray') && short - 16 bit signed integer (WORD)
&& loCArray = CREATEOBJECT('CUShortArray') && unsigned short - 16 bit unsigned integer
&& loCArray = CREATEOBJECT('CUIntArray') && unsigned int - 32 bit unsigned integer
&& loCArray = CREATEOBJECT('CFloatArray') && float - 32bit floating point
&& loCArray = CREATEOBJECT('CDoubleArray') && double - 64bit floating point

&& marshal the FoxPro style array ( !!! pass array by reference !!!)
*!*	loCArray.MarshalArray(@laArray)

&& pass the C style array to some C function 
&& the Address property holds the pointer (memory address) of the C style array
&& someFunc(loCArray.Address)

&& if the array was altered by the function, unmarshal it back 
*!*	loCArray.UnMarshalArray(@laArray)

&& or marshal a field of a cursor ( !!! pass cursor.fieldname as a string !!!)
RETURN

LOCAL laArray[20]
FOR xj = 1 TO ALEN(laArray)
	laArray[xj] = 1000 * xj
ENDFOR

&& character arrays
&& C - char[X][X] laArray;
loCArray = CREATEOBJECT('CCharArray')
loCArray.MarshalCursor('testcursor.charfield',50) 
loCArray.MarshalCursor('testcursor.charfield',30) && automatic truncation of field !!

&& C - char* laArray[X];
&& array of pointers to C strings
loCArray = CREATEOBJECT('CStringArray')
loCArray.MarshalCursor('testcursor.charfield')

&& manual construction of C style array - !! rather slow - if the C style array is bigger than a few elements 
&& its faster to create a FoxPro array/cursor and then marshal it with MarshalArray/MarshalCursor
loCArray = CREATEOBJECT('CStringArray')
loCArray.Dimension(10)
FOR xj = 1 TO loCArray.Elements
	loCArray.Element(xj) = 'Hello C ' + ALLTRIM(STR(xj))
ENDFOR

FOR xj = 1 TO loCArray.Elements
	? loCArray.Element(xj)
ENDFOR

loCArray = null
