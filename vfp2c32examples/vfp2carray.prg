#INCLUDE vfp2c.h

DEFINE CLASS CArray AS Custom

	Address = 0
	Rows = 0
	DIMENSION Element[1]

	PROTECTED Contained
	Contained = .F.
	PROTECTED ElementSize
	ElementSize = 0
	PROTECTED ArrayOffset
	ArrayOffset = 0
	PROTECTED CType
	CType = 0

	FUNCTION Init(lnRows)
		IF PCOUNT() = 1
			RETURN THIS.Dimension(m.lnRows)
		ENDIF
	ENDFUNC

	FUNCTION Destroy
		THIS.FreeArray()
	ENDFUNC
	
	FUNCTION FreeArray
		IF !THIS.Contained AND THIS.Address != 0
			FreeMem(THIS.Address)
		ENDIF
		THIS.Contained = .F.
		THIS.Address = 0
	ENDFUNC
	
	FUNCTION AttachArray(lnBaseAddress, lnRows)
		IF PCOUNT() != 2
			ERROR E_INVALIDPARAMS, 'Not enough parameters to attach array!'
		ENDIF

		THIS.FreeArray()

		THIS.Contained = .T.		
		THIS.Address = m.lnBaseAddress
		THIS.Rows = m.lnRows
		THIS.ArrayOffset = m.lnBaseAddress - THIS.ElementSize && eases access to the array in Element_Access (for 1 based addressing)
	ENDFUNC

	FUNCTION Dimension(lnRows)
		IF THIS.Contained
			ERROR E_INVALIDPARAMS, 'You cannot dimension a contained array!'
		ENDIF
		LOCAL lnAddress
		THIS.Rows = MIN(m.lnRows, FLL_MAX_ARRAY_SIZE)
		m.lnAddress = ReAllocMem(THIS.Address, THIS.Rows * THIS.ElementSize)
		IF m.lnAddress = 0
			LOCAL laError[1]
			AERROREX('laError')
			ERROR E_USERERROR, m.laError[3]
		ELSE
			THIS.Address = m.lnAddress
			THIS.ArrayOffset = THIS.Address - THIS.ElementSize
		ENDIF
	ENDFUNC
	
	FUNCTION AddressOf(lnRow)
		RETURN THIS.ArrayOffSet + m.lnRow * THIS.ElementSize
	ENDFUNC
	
	FUNCTION MarshalArray(laSource)
		EXTERNAL ARRAY laSource
		LOCAL lnRows
		m.lnRows = ALEN(m.laSource,1)
		IF THIS.Contained
			IF m.lnRows > THIS.Rows
				ERROR E_INVALIDPARAMS, 'Array to large for marshaling into a contained C array!'
			ENDIF
			RETURN MarshalFoxArray2CArray(THIS.Address, @m.laSource, THIS.CType)
		ELSE
			IF m.lnRows > THIS.Rows
				THIS.Dimension(m.lnRows)
			ENDIF
			RETURN MarshalFoxArray2CArray(THIS.Address, @m.laSource, THIS.CType)
		ENDIF
	ENDFUNC
	
	FUNCTION UnMarshalArray(laDestination)
		EXTERNAL ARRAY laDestination
		IF THIS.Address = 0
			ERROR E_INVALIDPARAMS, 'No C array created yet!'		
		ENDIF		
		DIMENSION m.laDestination[THIS.Rows]
		RETURN MarshalCArray2FoxArray(THIS.Address, @m.laDestination, THIS.CType)
	ENDFUNC
	
	FUNCTION MarshalCursor(lcCursorAndFieldNames)
		LOCAL lnColumns, lcAlias, lnRows
		m.lcAlias = GETWORDNUM(m.lcCursorAndFieldNames, 1, '.')
		m.lnColumns = OCCURS(',', m.lcCursorAndFieldNames) + 1
		IF m.lnColumns > 1
			ERROR E_INVALIDPARAMS, 'You can only pass 1 column name!'
		ENDIF
		m.lnRows = RECCOUNT(m.lcAlias)
		IF m.lnRows > THIS.Rows
			THIS.Dimension(m.lnRows)
		ENDIF
		RETURN MarshalCursor2CArray(THIS.Address, m.lcCursorAndFieldNames, THIS.CType, m.lnRows)
	ENDFUNC
	
	FUNCTION UnMarshalCursor(lcCursorAndFieldNames)
		IF THIS.Address = 0
			ERROR E_INVALIDPARAMS, 'No C array created yet!'		
		ENDIF		
		RETURN MarshalCArray2Cursor(THIS.Address, m.lcCursorAndFieldNames, THIS.CType, THIS.Rows)	
	ENDFUNC
	
ENDDEFINE

DEFINE CLASS C2DimArray AS Custom

	Address = 0
	Elements = 0
	Rows = 0
	Columns = 0
	DIMENSION Element[1]

	PROTECTED ElementSize
	ElementSize = 0
	PROTECTED RowSize
	RowSize = 0
	PROTECTED CType
	CType = 0
	PROTECTED Contained
	Contained = .F.
	PROTECTED ColumnOffSets[1]
	DIMENSION ColumnOffSets[1] = 0

	FUNCTION Init(lnRows, lnColumns)
		DO CASE
			CASE PCOUNT() = 1
				THIS.Dimension(m.lnRows, 1)
			CASE PCOUNT() = 2
				THIS.Dimesion(m.lnRows, m.lnColumns)
		ENDCASE
	ENDFUNC

	FUNCTION Destroy
		THIS.FreeArray()
	ENDFUNC
	
	FUNCTION FreeArray
		IF !THIS.Contained AND THIS.Address != 0
			FreeMem(THIS.Address)
		ENDIF
	ENDFUNC

	FUNCTION AttachArray(lnBaseAdress, lnRows, lnColumns)
		IF PCOUNT() != 3
			ERROR E_INVALIDPARAMS, 'Not enough parameters to attach array!'
		ENDIF

		THIS.FreeArray()
		
		THIS.Address = m.lnBaseAdress
		THIS.Contained = .T.
		
		m.lnRows = MIN(m.lnRows, FLL_MAX_ARRAY_SIZE)
		m.lnColumns = MIN(m.lnColumns, FLL_MAX_ARRAY_SIZE)
		THIS.CalculateOffsets(m.lnRows, m.lnColumns )				
	ENDFUNC

	FUNCTION Dimension(lnRows, lnColumns)
		IF THIS.Contained
			ERROR E_INVALIDPARAMS, 'You cannot dimension a contained array!'
		ENDIF
		LOCAL lnAddress
		m.lnRows = MIN(m.lnRows, FLL_MAX_ARRAY_SIZE)
		m.lnColumns = MIN(m.lnColumns, FLL_MAX_ARRAY_SIZE)
		m.lnAddress = ReAllocMem(THIS.Address, m.lnRows * m.lnColumns * THIS.ElementSize)
		IF m.lnAddress = 0
			LOCAL laError[1]
			AERROREX('laError')
			ERROR E_USERERROR, m.laError[3]
		ELSE
			THIS.Address = m.lnAddress
			THIS.CalculateOffsets(m.lnRows, m.lnColumns)
		ENDIF
	ENDFUNC

	FUNCTION AddressOf(lnRow, lnColumn)
		RETURN THIS.ColumnOffSets[m.lnColumn] + m.lnRow * THIS.ElementSize
	ENDFUNC
	
	PROTECTED FUNCTION CalculateOffSets(lnRows, lnColumns)
		LOCAL xj
		THIS.Rows = m.lnRows
		THIS.Columns = m.lnColumns
		THIS.Elements = m.lnRows * m.lnColumns
		THIS.RowSize = m.lnRows * THIS.ElementSize
		DIMENSION THIS.ColumnOffSets[m.lnColumns]
		FOR xj = 0 TO m.lnColumns - 1
			THIS.ColumnOffSets[m.xj+1] = THIS.Address + m.xj * THIS.RowSize - THIS.ElementSize
		ENDFOR
	ENDFUNC

	FUNCTION MarshalArray(laArray)
		EXTERNAL ARRAY laArray
		LOCAL lnRows, lnColumns
		m.lnRows = ALEN(m.laArray, 1)
		m.lnColumns = ALEN(m.laArray, 2)
		IF m.lnRows != THIS.Rows OR m.lnColumns != THIS.Columns
			THIS.Dimension(m.lnRows, m.lnColumns)		
		ENDIF
		RETURN MarshalFoxArray2CArray(THIS.Address, @m.laArray, THIS.CType)
	ENDFUNC
	
	FUNCTION UnMarshalArray(laArray)
		EXTERNAL ARRAY laArray
		IF THIS.Address = 0
			ERROR E_INVALIDPARAMS, 'No C array created yet!'		
		ENDIF			
		DIMENSION m.laArray[THIS.Rows, THIS.Columns]
		RETURN MarshalCArray2FoxArray(THIS.Address, @m.laArray, THIS.CType)
	ENDFUNC
	
	FUNCTION MarshalCursor(lcCursorAndFieldNames)
		LOCAL lcAlias, lnRows, lnColumns
		m.lcAlias = GETWORDNUM(m.lcCursorAndFieldNames, 1, '.')
		m.lnColumns = OCCURS(',', m.lcCursorAndFieldNames) + 1
		m.lnRows = RECCOUNT(m.lcAlias)
		IF m.lnRows != THIS.Rows OR m.lnColumns != THIS.Columns
			THIS.Dimension(m.lnRows, m.lnColumns)
		ENDIF
		RETURN MarshalCursor2CArray(THIS.Address, m.lcCursorAndFieldNames, THIS.CType)
	ENDFUNC

	FUNCTION UnMarshalCursor(lcCursorAndFieldNames)
		LOCAL lnColumns, lcAlias
		IF THIS.Address = 0
			ERROR E_INVALIDPARAMS, 'No C array created yet!'		
		ENDIF			
		m.lcAlias = GETWORDNUM(m.lcCursorAndFieldNames, 1, '.')
		m.lnColumns = OCCURS(',', m.lcCursorAndFieldNames) + 1
		IF m.lnColumns != THIS.Columns
			ERROR E_INVALIDPARAMS, 'Passed number of fields is not eqaul to columncount of C array!'
		ENDIF
		RETURN MarshalCArray2Cursor(THIS.Address, m.lcCursorAndFieldNames, THIS.CType, THIS.Rows)
	ENDFUNC
	
ENDDEFINE

DEFINE CLASS CShortArray AS CArray

	CType = CTYPE_SHORT
	ElementSize = SIZEOF_SHORT

	FUNCTION Element_Access(lnRow)
		ASSERT (THIS.Address != 0) MESSAGE 'Dimension the array first!'
		ASSERT (m.lnRow <= THIS.Rows) MESSAGE 'Arrayindex out of bounds!'
		RETURN ReadShort(THIS.ArrayOffset + m.lnRow * SIZEOF_SHORT)
	ENDFUNC
	
	FUNCTION Element_Assign(lnValue, lnRow)
		ASSERT (THIS.Address != 0) MESSAGE 'Dimension the array first!'
		ASSERT (m.lnRow <= THIS.Rows) MESSAGE 'Arrayindex out of bounds!'
		WriteShort(THIS.ArrayOffset + m.lnRow * SIZEOF_SHORT, m.lnValue)
	ENDFUNC
	
ENDDEFINE

DEFINE CLASS C2DimShortArray AS C2DimArray

	CType = CTYPE_SHORT
	ElementSize = SIZEOF_SHORT

	FUNCTION Element_Access(lnRow, lnColumn)
		ASSERT (THIS.Address != 0) MESSAGE 'Dimension the array first!'
		ASSERT (m.lnRow <= THIS.Rows AND m.lnColumns <= THIS.Columns) MESSAGE 'Arrayindex out of bounds!'
		RETURN ReadShort(THIS.ColumnOffSets[m.lnColumn] + m.lnRow * SIZEOF_INT)
	ENDFUNC

	FUNCTION Element_Assign(lnValue, lnRow, lnColumn)
		ASSERT (THIS.Address != 0) MESSAGE 'Dimension the array first!'
		ASSERT (m.lnRow <= THIS.Rows AND m.lnColumn <= THIS.Columns) MESSAGE 'Arrayindex out of bounds!'
		WriteShort(THIS.ColumnOffSets[m.lnColumn] + m.lnRow * SIZEOF_INT, m.lnValue)
	ENDFUNC

ENDDEFINE

DEFINE CLASS CUShortArray AS CArray

	CType = CTYPE_USHORT
	ElementSize = SIZEOF_SHORT

	FUNCTION Element_Access(lnRow)
		ASSERT (THIS.Address != 0) MESSAGE 'Dimension the array first!'
		ASSERT (m.lnRow <= THIS.Rows) MESSAGE 'Arrayindex out of bounds!'
		RETURN ReadUShort(THIS.ArrayOffset + m.lnRow * SIZEOF_SHORT)
	ENDFUNC
	
	FUNCTION Element_Assign(lnValue, lnRow)
		ASSERT (THIS.Address != 0) MESSAGE 'Dimension the array first!'
		ASSERT (m.lnRow <= THIS.Rows) MESSAGE 'Arrayindex out of bounds!'
		WriteUShort(THIS.ArrayOffset + m.lnRow * SIZEOF_SHORT, m.lnValue)
	ENDFUNC

ENDDEFINE

DEFINE CLASS C2DimUShortArray AS C2DimArray

	CType = CTYPE_USHORT
	ElementSize = SIZEOF_SHORT

	FUNCTION Element_Access(lnRow, lnColumn)
		ASSERT (THIS.Address != 0) MESSAGE 'Dimension the array first!'
		ASSERT (m.lnRow <= THIS.Rows AND m.lnColumns <= THIS.Columns) MESSAGE 'Arrayindex out of bounds!'
		RETURN ReadUShort(THIS.ColumnOffSets[m.lnColumn] + m.lnRow * SIZEOF_SHORT)
	ENDFUNC

	FUNCTION Element_Assign(lnValue, lnRow, lnColumn)
		ASSERT (THIS.Address != 0) MESSAGE 'Dimension the array first!'
		ASSERT (m.lnRow <= THIS.Rows AND m.lnColumn <= THIS.Columns) MESSAGE 'Arrayindex out of bounds!'
		WriteUShort(THIS.ColumnOffSets[m.lnColumn] + m.lnRow * SIZEOF_SHORT, m.lnValue)
	ENDFUNC

ENDDEFINE

DEFINE CLASS CIntArray AS CArray

	CType = CTYPE_INT
	ElementSize = SIZEOF_INT

	FUNCTION Element_Access(lnRow)
		ASSERT (THIS.Address != 0) MESSAGE 'Dimension the array first!'
		ASSERT (m.lnRow <= THIS.Rows) MESSAGE 'Arrayindex out of bounds!'
		RETURN ReadInt(THIS.ArrayOffset + m.lnRow * SIZEOF_INT)
	ENDFUNC
	
	FUNCTION Element_Assign(lnValue, lnRow)
		ASSERT (THIS.Address != 0) MESSAGE 'Dimension the array first!'
		ASSERT (m.lnRow <= THIS.Rows) MESSAGE 'Arrayindex out of bounds!'
		WriteInt(THIS.ArrayOffset + m.lnRow * SIZEOF_INT, m.lnValue)
	ENDFUNC

ENDDEFINE

DEFINE CLASS C2DimIntArray AS C2DimArray

	CType = CTYPE_INT
	ElementSize = SIZEOF_INT

	FUNCTION Element_Access(lnRow, lnColumn)
		ASSERT (THIS.Address != 0) MESSAGE 'Dimension the array first!'
		ASSERT (m.lnRow <= THIS.Rows AND m.lnColumns <= THIS.Columns) MESSAGE 'Arrayindex out of bounds!'
		RETURN ReadInt(THIS.ColumnOffSets[m.lnColumn] + m.lnRow * SIZEOF_INT)
	ENDFUNC

	FUNCTION Element_Assign(lnValue, lnRow, lnColumn)
		ASSERT (THIS.Address != 0) MESSAGE 'Dimension the array first!'
		ASSERT (m.lnRow <= THIS.Rows AND m.lnColumn <= THIS.Columns) MESSAGE 'Arrayindex out of bounds!'
		WriteInt(THIS.ColumnOffSets[m.lnColumn] + m.lnRow * SIZEOF_INT, m.lnValue)
	ENDFUNC

ENDDEFINE
	
DEFINE CLASS CUIntArray AS CArray

	CType = CTYPE_UINT
	ElementSize = SIZEOF_INT

	FUNCTION Element_Access(lnRow)
		ASSERT (THIS.Address != 0) MESSAGE 'Dimension the array first!'
		ASSERT (m.lnRow <= THIS.Rows) MESSAGE 'Arrayindex out of bounds!'
		RETURN ReadUInt(THIS.ArrayOffset + m.lnRow * SIZEOF_INT)
	ENDFUNC
	
	FUNCTION Element_Assign(lnValue, lnRow)
		ASSERT (THIS.Address != 0) MESSAGE 'Dimension the array first!'
		ASSERT (m.lnRow <= THIS.Rows) MESSAGE 'Arrayindex out of bounds!'
		WriteUInt(THIS.ArrayOffset + m.lnRow * SIZEOF_INT, m.lnValue)
	ENDFUNC

ENDDEFINE

DEFINE CLASS C2DimUIntArray AS C2DimArray

	CType = CTYPE_UINT
	ElementSize = SIZEOF_INT

	FUNCTION Element_Access(lnRow, lnColumn)
		ASSERT (THIS.Address != 0) MESSAGE 'Dimension the array first!'
		ASSERT (m.lnRow <= THIS.Rows AND m.lnColumns <= THIS.Columns) MESSAGE 'Arrayindex out of bounds!'
		RETURN ReadUInt(THIS.ColumnOffSets[m.lnColumn] + m.lnRow * SIZEOF_INT)
	ENDFUNC

	FUNCTION Element_Assign(lnValue, lnRow, lnColumn)
		ASSERT (THIS.Address != 0) MESSAGE 'Dimension the array first!'
		ASSERT (m.lnRow <= THIS.Rows AND m.lnColumn <= THIS.Columns) MESSAGE 'Arrayindex out of bounds!'
		WriteUInt(THIS.ColumnOffSets[m.lnColumn] + m.lnRow * SIZEOF_INT, m.lnValue)
	ENDFUNC

ENDDEFINE

DEFINE CLASS CFloatArray AS CArray

	CType = CTYPE_FLOAT
	ElementSize = SIZEOF_FLOAT

	FUNCTION Element_Access(lnRow)
		ASSERT (THIS.Address != 0) MESSAGE 'Dimension the array first!'
		ASSERT (m.lnRow <= THIS.Rows) MESSAGE 'Arrayindex out of bounds!'
		RETURN ReadFloat(THIS.ArrayOffset + m.lnRow * SIZEOF_FLOAT)
	ENDFUNC
	
	FUNCTION Element_Assign(lnValue, lnRow)
		ASSERT (THIS.Address != 0) MESSAGE 'Dimension the array first!'
		ASSERT (m.lnRow <= THIS.Rows) MESSAGE 'Arrayindex out of bounds!'
		WriteFloat(THIS.ArrayOffset + m.lnRow * SIZEOF_FLOAT, m.lnValue)
	ENDFUNC

ENDDEFINE

DEFINE CLASS C2DimFloatArray AS C2DimArray

	CType = CTYPE_FLOAT
	ElementSize = SIZEOF_FLOAT

	FUNCTION Element_Access(lnRow, lnColumn)
		ASSERT (THIS.Address != 0) MESSAGE 'Dimension the array first!'
		ASSERT (m.lnRow <= THIS.Rows AND m.lnColumns <= THIS.Columns) MESSAGE 'Arrayindex out of bounds!'
		RETURN ReadFloat(THIS.ColumnOffSets[m.lnColumn] + m.lnRow * SIZEOF_FLOAT)
	ENDFUNC

	FUNCTION Element_Assign(lnValue, lnRow, lnColumn)
		ASSERT (THIS.Address != 0) MESSAGE 'Dimension the array first!'
		ASSERT (m.lnRow <= THIS.Rows AND m.lnColumn <= THIS.Columns) MESSAGE 'Arrayindex out of bounds!'
		WriteFloat(THIS.ColumnOffSets[m.lnColumn] + m.lnRow * SIZEOF_FLOAT, m.lnValue)
	ENDFUNC

ENDDEFINE

DEFINE CLASS CDoubleArray AS CArray

	CType = CTYPE_DOUBLE
	ElementSize = SIZEOF_DOUBLE

	FUNCTION Element_Access(lnRow)
		ASSERT (THIS.Address != 0) MESSAGE 'Dimension the array first!'
		ASSERT (m.lnRow <= THIS.Rows) MESSAGE 'Arrayindex out of bounds!'
		RETURN ReadDouble(THIS.ArrayOffset + m.lnRow * SIZEOF_DOUBLE)
	ENDFUNC
	
	FUNCTION Element_Assign(lnValue, lnRow)
		ASSERT (THIS.Address != 0) MESSAGE 'Dimension the array first!'
		ASSERT (m.lnRow <= THIS.Rows) MESSAGE 'Arrayindex out of bounds!'
		WriteDouble(THIS.ArrayOffset + m.lnRow * SIZEOF_DOUBLE, m.lnValue)
	ENDFUNC

ENDDEFINE

DEFINE CLASS C2DimDoubleArray AS C2DimArray

	CType = CTYPE_DOUBLE
	ElementSize = SIZEOF_DOUBLE

	FUNCTION Element_Access(lnRow, lnColumn)
		ASSERT (THIS.Address != 0) MESSAGE 'Dimension the array first!'
		ASSERT (m.lnRow <= THIS.Rows AND m.lnColumns <= THIS.Columns) MESSAGE 'Arrayindex out of bounds!'
		RETURN ReadDouble(THIS.ColumnOffSets[m.lnColumn] + m.lnRow * SIZEOF_DOUBLE)
	ENDFUNC

	FUNCTION Element_Assign(lnValue, lnRow, lnColumn)
		ASSERT (THIS.Address != 0) MESSAGE 'Dimension the array first!'
		ASSERT (m.lnRow <= THIS.Rows AND m.lnColumn <= THIS.Columns) MESSAGE 'Arrayindex out of bounds!'
		WriteDouble(THIS.ColumnOffSets[m.lnColumn] + m.lnRow * SIZEOF_DOUBLE, m.lnValue)
	ENDFUNC
	
ENDDEFINE

DEFINE CLASS CBoolArray AS CArray

	CType = CTYPE_BOOL
	ElementSize = SIZEOF_INT

	FUNCTION Element_Access(lnRow)
		ASSERT (THIS.Address != 0) MESSAGE 'Dimension the array first!'
		ASSERT (m.lnRow <= THIS.Rows) MESSAGE 'Arrayindex out of bounds!'
		RETURN ReadLogical(THIS.ArrayOffset + m.lnRow * SIZEOF_INT)
	ENDFUNC
	
	FUNCTION Element_Assign(lnValue, lnRow)
		ASSERT (THIS.Address != 0) MESSAGE 'Dimension the array first!'
		ASSERT (m.lnRow <= THIS.Rows) MESSAGE 'Arrayindex out of bounds!'
		WriteLogical(THIS.ArrayOffset + m.lnRow * SIZEOF_INT, m.lnValue)
	ENDFUNC

ENDDEFINE

DEFINE CLASS C2DimBoolArray AS C2DimArray

	CType = CTYPE_BOOL
	ElementSize = SIZEOF_INT

	FUNCTION Element_Access(lnRow, lnColumn)
		ASSERT (THIS.Address != 0) MESSAGE 'Dimension the array first!'
		ASSERT (m.lnRow <= THIS.Rows AND m.lnColumns <= THIS.Columns) MESSAGE 'Arrayindex out of bounds!'
		RETURN ReadLogical(THIS.ColumnOffSets[m.lnColumn] + m.lnRow * SIZEOF_INT)
	ENDFUNC

	FUNCTION Element_Assign(lnValue, lnRow, lnColumn)
		ASSERT (THIS.Address != 0) MESSAGE 'Dimension the array first!'
		ASSERT (m.lnRow <= THIS.Rows AND m.lnColumn <= THIS.Columns) MESSAGE 'Arrayindex out of bounds!'
		WriteLogical(THIS.ColumnOffSets[m.lnColumn] + m.lnRow * SIZEOF_INT, m.lnValue)
	ENDFUNC

ENDDEFINE

DEFINE CLASS CStringArray AS CArray

	CType = CTYPE_CSTRING
	ElementSize = SIZEOF_POINTER
	
	FUNCTION FreeArray
		IF THIS.Address != 0
			FreeRefArray(THIS.Address, 1, THIS.Rows)
			IF !THIS.Contained
				FreeMem(THIS.Address)			
			ENDIF
		ENDIF
		THIS.Contained = .F.
		THIS.Address = 0
	ENDFUNC

	FUNCTION Element_Access(lnRow)
		ASSERT (THIS.Address != 0) MESSAGE 'Dimension the array first!'
		ASSERT (m.lnRow <= THIS.Rows) MESSAGE 'Arrayindex out of bounds!'
		RETURN ReadPCString(THIS.ArrayOffset + m.lnRow * SIZEOF_POINTER)
	ENDFUNC
	
	FUNCTION Element_Assign(lnValue, lnRow)
		ASSERT (THIS.Address != 0) MESSAGE 'Dimension the array first!'
		ASSERT (m.lnRow <= THIS.Rows) MESSAGE 'Arrayindex out of bounds!'
		WritePCString(THIS.ArrayOffset + m.lnRow * SIZEOF_POINTER, m.lnValue)
	ENDFUNC

ENDDEFINE

DEFINE CLASS C2DimStringArray AS C2DimArray

	CType = CTYPE_CSTRING
	ElementSize = SIZEOF_POINTER
	
	FUNCTION FreeArray
		IF THIS.Address != 0
			FreeRefArray(THIS.Address, 1, THIS.Elements)
			IF !THIS.Contained
				FreeMem(THIS.Address)			
			ENDIF
		ENDIF
		THIS.Contained = .F.
		THIS.Address = 0
	ENDFUNC

	FUNCTION Element_Access(lnRow, lnColumn)
		ASSERT (THIS.Address != 0) MESSAGE 'Dimension the array first!'
		ASSERT (m.lnRow <= THIS.Rows AND m.lnColumns <= THIS.Columns) MESSAGE 'Arrayindex out of bounds!'
		RETURN ReadPCString(THIS.ColumnOffSets[m.lnColumn] + m.lnRow * SIZEOF_POINTER)
	ENDFUNC

	FUNCTION Element_Assign(lnValue, lnRow, lnColumn)
		ASSERT (THIS.Address != 0) MESSAGE 'Dimension the array first!'
		ASSERT (m.lnRow <= THIS.Rows AND m.lnColumn <= THIS.Columns) MESSAGE 'Arrayindex out of bounds!'
		WritePCString(THIS.ColumnOffSets[m.lnColumn] + m.lnRow * SIZEOF_POINTER, m.lnValue)
	ENDFUNC

ENDDEFINE

DEFINE CLASS CWStringArray AS CArray

	CType = CTYPE_WSTRING
	ElementSize = SIZEOF_POINTER
	
	FUNCTION FreeArray
		IF THIS.Address != 0
			FreeRefArray(THIS.Address, 1, THIS.Elements)
			IF !THIS.Contained
				FreeMem(THIS.Address)			
			ENDIF
		ENDIF
		THIS.Contained = .F.
		THIS.Address = 0
	ENDFUNC

	FUNCTION Element_Access(lnRow)
		ASSERT (THIS.Address != 0) MESSAGE 'Dimension the array first!'
		ASSERT (m.lnRow <= THIS.Rows) MESSAGE 'Arrayindex out of bounds!'
		RETURN ReadPWString(THIS.ArrayOffset + m.lnRow * SIZEOF_POINTER)
	ENDFUNC
	
	FUNCTION Element_Assign(lnValue, lnRow)
		ASSERT (THIS.Address != 0) MESSAGE 'Dimension the array first!'
		ASSERT (m.lnRow <= THIS.Rows) MESSAGE 'Arrayindex out of bounds!'
		WritePWString(THIS.ArrayOffset + m.lnRow * SIZEOF_POINTER, m.lnValue)
	ENDFUNC

ENDDEFINE

DEFINE CLASS C2DimWStringArray AS C2DimArray

	CType = CTYPE_WSTRING
	ElementSize = SIZEOF_POINTER
	
	FUNCTION FreeArray
		IF THIS.Address != 0
			FreeRefArray(THIS.Address, 1, THIS.Elements)
			IF !THIS.Contained
				FreeMem(THIS.Address)			
			ENDIF
		ENDIF
		THIS.Contained = .F.
		THIS.Address = 0
	ENDFUNC

	FUNCTION Element_Access(lnRow, lnColumn)
		ASSERT (THIS.Address != 0) MESSAGE 'Dimension the array first!'
		ASSERT (m.lnRow <= THIS.Rows AND m.lnColumns <= THIS.Columns) MESSAGE 'Arrayindex out of bounds!'
		RETURN ReadPWString(THIS.ColumnOffSets[m.lnColumn] + m.lnRow * SIZEOF_POINTER)
	ENDFUNC

	FUNCTION Element_Assign(lnValue, lnRow, lnColumn)
		ASSERT (THIS.Address != 0) MESSAGE 'Dimension the array first!'
		ASSERT (m.lnRow <= THIS.Rows AND m.lnColumn <= THIS.Columns) MESSAGE 'Arrayindex out of bounds!'
		WritePWString(THIS.ColumnOffSets[m.lnColumn] + m.lnRow * SIZEOF_POINTER, m.lnValue)
	ENDFUNC

ENDDEFINE

DEFINE CLASS CCharArray AS CArray

	CType = CTYPE_CHARARRAY
	ElementSize = 0

	FUNCTION AttachArray(lnBaseAddress, lnRows, lnLength)
		IF PCOUNT() != 3
			ERROR E_INVALIDPARAMS, 'Not enough parameters to attach array!'
		ENDIF

		THIS.FreeArray()
		
		THIS.Contained = .T.
		THIS.Address = m.lnBaseAddress
		THIS.Rows = m.lnRows
		THIS.ElementSize = m.lnLength
		THIS.ArrayOffset = m.lnBaseAddress - m.lnLength && eases access to the array in Element_Access (for 1 based addressing)
	ENDFUNC

	FUNCTION Dimension(lnRows, lnLength)
		IF THIS.Contained
			ERROR E_INVALIDPARAMS, 'You cannot dimension a contained array!'
		ENDIF
		LOCAL lnAddress
		THIS.Rows = MIN(m.lnRows, FLL_MAX_ARRAY_SIZE)
		THIS.ElementSize = m.lnLength
		m.lnAddress = ReAllocMem(THIS.Address, m.lnRows * m.lnLength)
		IF m.lnAddress = 0
			LOCAL laError[1]
			AERROREX('laError')
			ERROR E_USERERROR, m.laError[3]
		ENDIF
		THIS.Address = m.lnAddress
		THIS.ArrayOffset = THIS.Address - m.lnLength
	ENDFUNC

	FUNCTION Element_Access(lnRow)
		ASSERT (THIS.Address != 0) MESSAGE 'Dimension the array first!'
		ASSERT (m.lnRow <= THIS.Rows) MESSAGE 'Arrayindex out of bounds!'
		RETURN ReadCharArray(THIS.ArrayOffset + m.lnRow * THIS.ElementSize, THIS.ElementSize)
	ENDFUNC
	
	FUNCTION Element_Assign(lnValue, lnRow)
		ASSERT (THIS.Address != 0) MESSAGE 'Dimension the array first!'
		ASSERT (m.lnRow <= THIS.Rows) MESSAGE 'Arrayindex out of bounds!'
		WriteCharArray(THIS.ArrayOffset + m.lnRow * THIS.ElementSize, m.lnValue, THIS.ElementSize)
	ENDFUNC
	
	FUNCTION MarshalArray(laSource, lnLength)
		EXTERNAL ARRAY laSource
		IF THIS.Dimension(ALEN(m.laSource,1), m.lnLength)
			RETURN MarshalFoxArray2CArray(THIS.Address, @m.laSource, THIS.CType, m.lnLength)
		ELSE
			RETURN .F.
		ENDIF
	ENDFUNC
	
	FUNCTION UnMarshalArray(laDestination)
		EXTERNAL ARRAY laDestination
		IF THIS.Address = 0
			ERROR E_INVALIDPARAMS, 'No C array created yet!'		
		ENDIF		
		DIMENSION m.laDestination[THIS.Rows]
		RETURN MarshalCArray2FoxArray(THIS.Address, @m.laDestination, THIS.CType, THIS.ElementSize)
	ENDFUNC

ENDDEFINE

DEFINE CLASS C2DimCharArray AS C2DimArray

	CType = CTYPE_CHARARRAY
	ElementSize = 0

	FUNCTION AttachArray(lnBaseAdress, lnRows, lnColumns, lnLength)
		IF PCOUNT() != 4
			ERROR E_INVALIDPARAMS, 'Not enough parameters to attach array!'
		ENDIF

		THIS.FreeArray()
		
		THIS.Address = m.lnBaseAdress
		THIS.Contained = .T.
		THIS.ElementSize = m.lnLength
				
		m.lnRows = MIN(m.lnRows, FLL_MAX_ARRAY_SIZE)
		m.lnColumns = MIN(m.lnColumns, FLL_MAX_ARRAY_SIZE)
		THIS.CalculateOffsets(m.lnRows, m.lnColumns)
	ENDFUNC

	FUNCTION Dimension(lnRows, lnColumns, lnLength)
		IF THIS.Contained
			ERROR E_INVALIDPARAMS, 'You cannot dimension a contained array!'
		ENDIF
		LOCAL lnAddress
		m.lnRows = MIN(m.lnRows, FLL_MAX_ARRAY_SIZE)
		m.lnColumns = MIN(m.lnColumns, FLL_MAX_ARRAY_SIZE)
		THIS.ElementSize = m.lnLength
		m.lnAddress = ReAllocMem(THIS.Address, m.lnRows * m.lnColumns * THIS.ElementSize)
		IF m.lnAddress = 0
			LOCAL laError[1]
			AERROREX('laError')
			ERROR E_USERERROR, m.laError[3]
		ELSE
			THIS.Address = m.lnAddress
			THIS.CalculateOffsets(m.lnRows, m.lnColumns)
		ENDIF
	ENDFUNC

	FUNCTION Element_Access(lnRow, lnColumn)
		ASSERT (THIS.Address != 0) MESSAGE 'Dimension the array first!'
		ASSERT (m.lnRow <= THIS.Rows AND m.lnColumns <= THIS.Columns) MESSAGE 'Arrayindex out of bounds!'
		RETURN ReadCharArray(THIS.ColumnOffSets[m.lnColumn] + m.lnRow * THIS.ElementSize)
	ENDFUNC

	FUNCTION Element_Assign(lnValue, lnRow, lnColumn)
		ASSERT (THIS.Address != 0) MESSAGE 'Dimension the array first!'
		ASSERT (m.lnRow <= THIS.Rows AND m.lnColumn <= THIS.Columns) MESSAGE 'Arrayindex out of bounds!'
		WriteCharArray(THIS.ColumnOffSets[m.lnColumn] + m.lnRow * THIS.ElementSize, m.lnValue, THIS.ElementSize)
	ENDFUNC

	FUNCTION MarshalArray(laArray, lnLength)
		EXTERNAL ARRAY laArray
		LOCAL lnRows, lnColumns
		m.lnRows = ALEN(m.laArray, 1)
		m.lnColumns = ALEN(m.laArray, 2)
		IF m.lnRows != THIS.Rows OR m.lnColumns != THIS.Columns OR m.lnLength != THIS.ElementSize
			THIS.Dimension(m.lnRows, m.lnColumns, m.lnLength)
		ENDIF
		RETURN MarshalFoxArray2CArray(THIS.Address, @m.laArray, THIS.CType, m.lnLength)
	ENDFUNC

	FUNCTION UnMarshalArray(laArray)
		IF THIS.Address = 0
			ERROR E_INVALIDPARAMS, 'No C array created yet!'		
		ENDIF
		DIMENSION m.laArray[THIS.Rows, THIS.Columns]
		RETURN MarshalCArray2FoxArray(THIS.Address, @m.laArray, THIS.CType, THIS.ElementSize)
	ENDFUNC

ENDDEFINE

DEFINE CLASS CWCharArray AS CArray

	CType = CTYPE_WCHARARRAY
	ElementSize = 0
	CodePage = CP_ACP
	
	FUNCTION AttachArray(nBaseAdress,nElements,nLength)
		ASSERT (PCOUNT()=3) MESSAGE 'Not enough parameters to attach array'

		THIS.FreeArray()
		
		THIS.Address = nBaseAdress
		THIS.Contained = .T.
		THIS.Elements = nElements
		THIS.ElementSize = nLength * SIZEOF_WCHAR
		THIS.ArrayOffset = THIS.Address - THIS.ElementSize && eases access to the array in Element_Access (for 1 based addressing)

	ENDFUNC
	
	FUNCTION Dimension(nElements, nLength)
		LOCAL lnAddress
		THIS.Elements = nElements
		THIS.ElementSize = nLength * SIZEOFWCHAR
		lnAddress = ReAllocMem(THIS.Address,nElements*THIS.ElementSize)
		IF lnAddress != 0
			THIS.Address = lnAddress
			THIS.ArrayOffset = THIS.Address - THIS.ElementSize
			RETURN .T.
		ELSE
			RETURN .F.
		ENDIF
	ENDFUNC
	
	FUNCTION Element_Access(nElement)
		ASSERT (THIS.Address != 0) MESSAGE 'Dimension the array first!'	
		ASSERT (nElement <= THIS.Elements) MESSAGE 'Arrayindex out of bounds!'		
		IF VARTYPE(nElement) = 'N'
			RETURN ReadWCharArray(THIS.ArrayOffset+nElement*THIS.ElementSize, THIS.ElementSize)
		ELSE
			RETURN ''
		ENDIF
	ENDFUNC
	
	FUNCTION Element_Assign(nNewVal,nElement)
		ASSERT (THIS.Address != 0) MESSAGE 'Dimension the array first!'
		ASSERT (nElement <= THIS.Elements) MESSAGE 'Arrayindex out of bounds!'
		WriteWCharArray(THIS.ArrayOffset+nElement*THIS.ElementSize,nNewVal,THIS.ElementSize)
	ENDFUNC

	FUNCTION MarshalArray(laSource, lnLength)
		EXTERNAL ARRAY laSource
		IF THIS.Dimesion(ALEN(m.laSource,1), m.nLength)
			RETURN MarshalFoxArray2CArray(THIS.Address, @m.laSource, THIS.CType, m.lnLength, THIS.CodePage)
		ELSE
			RETURN .F.
		ENDIF
	ENDFUNC
	
	FUNCTION UnMarshalArray(laDestination)
		EXTERNAL ARRAY laDestination
		DIMENSION laDestination[MIN(THIS.Elements,FLL_MAX_ARRAY_SIZE)]
		RETURN MarshalCArray2FoxArray(THIS.Address, @m.laDestination, THIS.ElementSize, THIS.CodePage)
	ENDFUNC

ENDDEFINE

DEFINE CLASS C2DimWCharArray AS C2DimArray

	CType = CTYPE_WCHARARRAY
	ElementSize = 0
	CodePage = CP_ACP
	
	FUNCTION AttachArray(nBaseAdress,nElements,nLength)
		ASSERT (PCOUNT()=3) MESSAGE 'Not enough parameters to attach array'

		THIS.FreeArray()
		
		THIS.Address = nBaseAdress
		THIS.Contained = .T.
		THIS.Elements = nElements
		THIS.ElementSize = nLength * SIZEOF_WCHAR
		THIS.ArrayOffset = THIS.Address - THIS.ElementSize && eases access to the array in Element_Access (for 1 based addressing)

	ENDFUNC
	
	FUNCTION Dimension(nElements, nLength)
		LOCAL lnAddress
		THIS.Elements = nElements
		THIS.ElementSize = nLength * SIZEOFWCHAR
		lnAddress = ReAllocMem(THIS.Address,nElements*THIS.ElementSize)
		IF lnAddress != 0
			THIS.Address = lnAddress
			THIS.ArrayOffset = THIS.Address - THIS.ElementSize
			RETURN .T.
		ELSE
			RETURN .F.
		ENDIF
	ENDFUNC
	
	FUNCTION Element_Access(nElement)
		ASSERT (THIS.Address != 0) MESSAGE 'Dimension the array first!'	
		ASSERT (nElement <= THIS.Elements) MESSAGE 'Arrayindex out of bounds!'		
		IF VARTYPE(nElement) = 'N'
			RETURN ReadWCharArray(THIS.ArrayOffset+nElement*THIS.ElementSize, THIS.ElementSize)
		ELSE
			RETURN ''
		ENDIF
	ENDFUNC
	
	FUNCTION Element_Assign(nNewVal,nElement)
		ASSERT (THIS.Address != 0) MESSAGE 'Dimension the array first!'
		ASSERT (nElement <= THIS.Elements) MESSAGE 'Arrayindex out of bounds!'
		WriteWCharArray(THIS.ArrayOffset+nElement*THIS.ElementSize,nNewVal,THIS.ElementSize)
	ENDFUNC

	FUNCTION MarshalArray(laSource, lnLength)
		EXTERNAL ARRAY laSource
		IF THIS.Dimesion(ALEN(m.laSource,1), m.nLength)
			RETURN MarshalFoxArray2CArray(THIS.Address, @m.laSource, THIS.CType, m.lnLength, THIS.CodePage)
		ELSE
			RETURN .F.
		ENDIF
	ENDFUNC
	
	FUNCTION UnMarshalArray(laDestination)
		EXTERNAL ARRAY laDestination
		DIMENSION laDestination[MIN(THIS.Elements,FLL_MAX_ARRAY_SIZE)]
		RETURN MarshalCArray2FoxArray(THIS.Address, @m.laDestination, THIS.ElementSize, THIS.CodePage)
	ENDFUNC

ENDDEFINE