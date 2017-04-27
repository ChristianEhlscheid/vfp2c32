#INCLUDE cparserh.h

DEFINE CLASS PARSEDTYPES AS Exception

	Address = 0
	ParseNode = .F.
	NextNode = .F.
	
	FUNCTION Init(lnAddress)
		THIS.Address = lnAddress
	ENDFUNC

	FUNCTION ParseNode_Access
		RETURN ReadPointer(THIS.Address)
	ENDFUNC
	
	FUNCTION NextNode_Access
		RETURN ReadPointer(THIS.Address+4)
	ENDFUNC

ENDDEFINE

DEFINE CLASS PARSEERRORS AS Exception

	Address = 0
	pErrorMes = .F.
	nLineNo = .F.
	NextError = .F.
	
	FUNCTION Init(lnAddress)	
		THIS.Address = lnAddress
	ENDFUNC
	
	FUNCTION pErrorMes_Access
		RETURN ReadPCString(THIS.Address)
	ENDFUNC
	
	FUNCTION nLineNo_Access
		RETURN ReadInt(THIS.Address+4)
	ENDFUNC
	
	FUNCTION NextError_Access
		RETURN ReadPointer(THIS.Address+8)
	ENDFUNC

ENDDEFINE

DEFINE CLASS PARSENODE AS Exception

*!*	typedef struct _PARSENODE {
*!*		NODETYPE nType;	
*!*		union {	
*!*		struct _PARSENODE *link[3];	
*!*		char *String;	
*!*		int Constant;
*!*		} un;
*!*		int LineNo;
*!*	} PARSENODE, *LPPARSENODE;
	
	Address = 0
	nType = .F.
	link1 = .F.
	link2 = .F.
	link3 = .F.
	String = .F.
	Constant = .F.
	nLineNo = .F.
	
	FUNCTION Init(lnAddress)
		THIS.Address = lnAddress
	ENDFUNC
	
	FUNCTION nType_Access
		RETURN ReadInt(THIS.Address)
	ENDFUNC
	
	FUNCTION link1_Access
		RETURN ReadPointer(THIS.Address+4)
	ENDFUNC

	FUNCTION link2_Access
		RETURN ReadPointer(THIS.Address+8)
	ENDFUNC

	FUNCTION link3_Access
		RETURN ReadPointer(THIS.Address+12)
	ENDFUNC

	FUNCTION String_Access
		RETURN ReadPCString(THIS.Address+4)
	ENDFUNC
	
	FUNCTION Constant_Access
		RETURN ReadInt(THIS.Address+4)
	ENDFUNC
	
	FUNCTION nLineNo_Access
		RETURN ReadInt(THIS.Address+16)
	ENDFUNC

ENDDEFINE

DEFINE CLASS TYPENODE AS Exception
	
	nType = 0 && one of TT_ defines
	TypeMask = 0 && bit-mask of TM_VOID, TM_CHAR ... when nType = TT_BASICTYPE
	TypeName = "" && typename when nType = TT_TYPEDEF
	Indirect = 0 && level of indirection (*)
	nBits = 0 && Bits of a Bitfield
	cName = "" && name of structure member or structure tag
	bArray = .F. && is Array of some type
	bBitField = .F.
	BaseSize = 0 && size of type alone
	SizeOf = 0 && overall size of type
	AlignOf = 0 && alignment requirement of type
	OffsetOf = 0 && offset of member in structure in bytes
	Assertion = "" && assertion command for type
	ReadFunc = "" && function to write the type to memory
	WriteFunc = "" && function to read the type from memory
	VfpClass = "" && vfp class thats implements this type (for embedded substructures)
	nLineNo = 0	&& linenumber on which type is defined
	ValueOf = 0 && value of enumerator
	CSharpType = "" && type in .NET
	MarshalGet = "" && Marshal read routine in .NET
	MarshalSet = "" && Marshal write routine in .NET
	DIMENSION laMembers[1] && structure member TYPENODE'S array when nType = TT_STRUCTHEADER | TT_UNIONHEADER
	DIMENSION laSubscripts[1] && subscript information when an array
	DIMENSION laDeclar[1] && declarator's for whole structure/union

	FUNCTION nBits_Assign(nBits)
		THIS.nBits = nBits
		THIS.bBitField = .T.
	ENDFUNC
	
	FUNCTION SizeOf_Assign(nSize)
		THIS.BaseSize = nSize
		IF VARTYPE(THIS.laSubscripts[1]) = 'L'
			THIS.SizeOf = nSize
		ELSE
			LOCAL lnElements, xj
			lnElements = 1
			FOR xj = 1 TO ALEN(THIS.laSubscripts)
				lnElements = lnElements * THIS.laSubscripts[xj]
			ENDFOR
			THIS.SizeOf = lnElements * nSize
		ENDIF
	ENDFUNC
	
	FUNCTION OffsetOf_Assign(nOffset)
		IF INLIST(THIS.nType,TT_STRUCTHEADER,TT_UNIONHEADER)
			LOCAL xj
			FOR xj = 1 TO ALEN(THIS.laMembers)
				THIS.laMembers[xj].OffsetOf = nOffset
			ENDFOR
		ELSE
			THIS.OffsetOf = THIS.OffsetOf + nOffset	
		ENDIF
	ENDFUNC
	
	&& add a TYPENODE to this TYPENODE (when THIS.nType = TT_STRUCTHEADER or TT_UNIONHEADER)
	FUNCTION AddMember(loObj)
		IF VARTYPE(THIS.laMembers[1]) = 'L'
			THIS.laMembers[1] = loObj
		ELSE
			LOCAL lnAlen
			lnAlen = ALEN(THIS.laMembers) + 1
			DIMENSION THIS.laMembers[lnAlen]
			THIS.laMembers[lnAlen] = loObj
		ENDIF
	ENDFUNC
	
	FUNCTION AddDeclarator(loObj)
		IF VARTYPE(THIS.laDeclar[1]) = 'L'
			THIS.laDeclar[1] = loObj
		ELSE
			LOCAL lnAlen
			lnAlen = ALEN(THIS.laDeclar) + 1
			DIMENSION THIS.laDeclar[lnAlen]
			THIS.laDeclar[lnAlen] = loObj
		ENDIF
	ENDFUNC
	
	&& make a full duplicate of this TYPENODE
	FUNCTION Clone
		LOCAL loNewObj, xj
		loNewObj = CREATEOBJECT('TYPENODE')
		loNewObj.nType = THIS.nType
		loNewObj.Typemask = THIS.TypeMask
		loNewObj.Typename = THIS.Typename
		loNewObj.Indirect = THIS.Indirect
		loNewObj.cName = THIS.cName
		loNewObj.bArray = THIS.bArray
		IF THIS.nBits != 0
			loNewObj.nBits = THIS.nBits
		ENDIF
		IF VARTYPE(THIS.laMembers[1]) = 'O'
			DIMENSION loNewObj.laMembers[ALEN(THIS.laMembers)]
			FOR xj = 1 TO ALEN(THIS.laMembers)
				loNewObj.laMembers[xj] = THIS.laMembers[xj].Clone()
			ENDFOR
		ENDIF
		IF VARTYPE(THIS.laSubscripts[1]) = 'N'
			DIMENSION loNewObj.laSubscripts[ALEN(THIS.laSubscripts)]
			FOR xj = 1 TO ALEN(THIS.laSubscripts)
				loNewObj.laSubscripts[xj] = THIS.laSubscripts[xj]
			ENDFOR
		ENDIF
		IF VARTYPE(THIS.laDeclar[1]) = 'O'
			DIMENSION loNewObj.laDeclar[ALEN(THIS.laDeclar)]
			FOR xj = 1 TO ALEN(THIS.laDeclar)
				loNewObj.laDeclar[xj] = THIS.laDeclar[xj].Clone()
			ENDFOR
		ENDIF
		RETURN loNewObj
	ENDFUNC
	
	&& add a subscript
	&& the subscripts are persisted in reverse order in the parsetree
	&& thats why each new subscripts is added to the beginning of the laSubscripts array
	FUNCTION AddSubscript(lnValue)
		IF VARTYPE(THIS.laSubscripts[1]) = 'L'
			THIS.laSubscripts[1] = lnValue
		ELSE
			DIMENSION THIS.laSubscripts[ALEN(THIS.laSubscripts)+1]
			AINS(THIS.laSubscripts,1) && shift all elements 1 up
			THIS.laSubscripts[1] = lnValue
		ENDIF
		THIS.bArray = .T.
	ENDFUNC
	
ENDDEFINE

&& converts a Typemask to a string
PROCEDURE TM2String(lnMask)

	LOCAL lcString
	lcString = ""

	IF BITAND(lnMask,TM_STRUCT) > 0 
		lcString = lcString + " struct"
	ENDIF
	
	IF BITAND(lnMask,TM_TYPENAME) > 0
		lcString = lcString + " typedef"
	ENDIF
	
	IF BITAND(lnMask,TM_ENUM) > 0
		lcString = lcString + " enum"
	ENDIF
	
	IF BITAND(lnMask,TM_SIGNED) > 0
		lcString = lcString + " signed"
	ENDIF

	IF BITAND(lnMask,TM_UNSIGNED) > 0 
		lcString = lcString + " unsigned"
	ENDIF
			
	IF BITAND(lnMask,TM_SHORT) > 0
		lcString = lcString + " short"
	ENDIF

	IF BITAND(lnMask,TM_LONG) > 0
		lcString = lcString + " long"
	ENDIF

	IF BITAND(lnMask,TM_CHAR) > 0
		lcString = lcString + " char"
	ENDIF

	IF BITAND(lnMask,TM_WCHAR) > 0
		lcString = lcString + " wchar_t"
	ENDIF

	IF BITAND(lnMask,TM_INT8) > 0
		lcString = lcString + " __int8"
	ENDIF
	
	IF BITAND(lnMask,TM_INT) > 0
		lcString = lcString + " int"
	ENDIF
	
	IF BITAND(lnMask,TM_INT64) > 0
		lcString = lcString + " __int64"
	ENDIF

	IF BITAND(lnMask,TM_FLOAT) > 0
		lcString = lcString + " float"
	ENDIF

	IF BITAND(lnMask,TM_DOUBLE) > 0
		lcString = lcString + " double"
	ENDIF

	IF BITAND(lnMask,TM_VOID) > 0
		lcString = lcString + " void"
	ENDIF
	
	RETURN ALLTRIM(lcString)

ENDPROC

&& processes a string of type specifiers 
&& and returns the corresponding typemask
&& e.g. "unsigned int"
FUNCTION String2TM(lcType)
	LOCAL lnMask
	lnMask = 0

	IF "void" $ lcType
		lnMask = BITOR(lnMask,TM_VOID)
		lcType = STRTRAN(lcType,"void")
	ENDIF

	IF "wchar_t" $ lcType
		lnMask = BITOR(lnMask,TM_WCHAR)
		lcType = STRTRAN(lcType,"wchar_t")
	ENDIF

	IF "char" $ lcType
		lnMask = BITOR(lnMask,TM_CHAR)
		lcType = STRTRAN(lcType,"char")
	ENDIF
	
	IF "__int8" $ lcType
		lnMask = BITOR(lnMask,TM_INT8)
		lcType = STRTRAN(lcType,"__int8")
	ENDIF

	IF "__int64" $ lcType
		lnMask = BITOR(lnMask,TM_INT64)
		lcType = STRTRAN(lcType,"__int64")
	ENDIF
		
	IF "int" $ lcType
		lnMask = BITOR(lnMask,TM_INT)
		lcType = STRTRAN(lcType,"int")
	ENDIF
	
	IF "short" $ lcType
		lnMask = BITOR(lnMask,TM_SHORT)
		lcType = STRTRAN(lcType,"short")
	ENDIF
	
	IF "long" $ lcType
		lnMask = BITOR(lnMask,TM_LONG)
		lcType = STRTRAN(lcType,"long")
	ENDIF
	
	IF "float" $ lcType
		lnMask = BITOR(lnMask,TM_FLOAT)
		lcType = STRTRAN(lcType,"float")
	ENDIF

	IF "double" $ lcType
		lnMask = BITOR(lnMask,TM_DOUBLE)
		lcType = STRTRAN(lcType,"double")
	ENDIF

	IF "unsigned" $ lcType
		lnMask = BITOR(lnMask,TM_UNSIGNED)
		lcType = STRTRAN(lcType,"unsigned")
	ENDIF
	&& "signed" is ignored
	
	IF "bool" $ lcType
		lnMask = BITOR(lnMask,TM_BOOL)
	ENDIF
	
	RETURN lnMask

ENDFUNC