#INCLUDE cparserh.h

DEFINE CLASS oCParser AS Relation

	cDir = ""

	DIMENSION laTypes[1] && array of completly parsed & processed types

	nPointerSize = 4 && size of a pointer (hold this here cause this changes to 8 bytes on Win64 systems)
	nPragmaPack = 8 && Pragma pack option as in C(++)
	nPlatForm = 1 && 0 = all, 1 = Win32 , 2 = Win64
	nCharSet = 1 && 0 = all, 1 = Ansi, 2 = Unicode

	nErrorCount = 0 && count of parseerrors detected
	nWalkErrors = 0 && number of errors detected during tree walking
	
	
	PROCEDURE Init()
		THIS.cDir = FULLPATH(CURDIR())
		THIS.nPlatform = THIS.nPlatform
		THIS.nCharset = THIS.nCharset
		THIS.DeclareParser()
		THIS.InitTables()
		THIS.InitTypeList()
	ENDPROC

	PROCEDURE Destroy()	
		clear_errors()
		clear_parsedtypes()
		clear_knowntypes()
		CLEAR DLLS 'start_parse', 'remove_comments', 'add_knowntype', 'find_knowntype', 'clear_knowntypes', ;
				'clear_parsedtypes', 'get_parsedtypehead', 'debug_entry', 'clear_errors', 'get_parseerrorhead', ;
				'get_errorcount'
		USE IN SELECT('cbasetypes')
		USE IN SELECT('ctypedefs')
		USE IN SELECT('cerrors')
		USE IN SELECT('keywords')
	ENDPROC

	PROCEDURE nPlatForm_Assign(nPlatform)
		THIS.nPlatform = nPlatform
		IF nPlatform = 1
			THIS.nPointerSize = 4
		ELSE
			THIS.nPointerSize = 8
		ENDIF
	ENDPROC
	
	PROCEDURE nCharset_Assign(nCharSet)
		THIS.nCharset = nCharset
	ENDPROC
	
	PROCEDURE nErrorCount_Access()
		RETURN get_errorcount()
	ENDPROC
	
	PROCEDURE InitTables()
		IF !USED('cbasetypes')
			USE cbasetypes IN 0 ORDER TAG ibasetype1
		ENDIF
		IF !USED('ctypedefs')
			USE ctypedefs IN 0 ORDER TAG itypedef1
			CURSORSETPROP("Buffering",3,'ctypedefs')
		ENDIF
		IF !USED('keywords')
			USE keywords IN 0
		ENDIF
		GO TOP IN cbasetypes
		GO TOP IN ctypedefs
		CREATE CURSOR cErrors (nOrder I,cMessage M, nLineNo I NULL)
		SELECT cErrors
		INDEX ON nOrder TAG nOrder DESCENDING
		SET ORDER TO TAG nOrder DESCENDING
	ENDPROC
	
	PROCEDURE DeclareParser()
		DECLARE INTEGER start_parse IN cparser.dll STRING cInputFile
		DECLARE INTEGER remove_comments IN cparser.dll STRING cInputFile, STRING cOutputFile
		DECLARE INTEGER add_knowntype IN cparser.dll STRING cType
		DECLARE remove_knowntype IN cparser.dll STRING cType
		DECLARE clear_knowntypes IN cparser.dll
		DECLARE clear_parsedtypes IN cparser.dll
		DECLARE INTEGER get_parsedtypehead IN cparser.dll
		DECLARE debug_entry IN cparser.dll
		DECLARE INTEGER get_parseerrorhead IN cparser.dll
		DECLARE INTEGER get_errorcount IN cparser.dll
		DECLARE clear_errors IN cparser.dll
	ENDPROC

	PROCEDURE InitTypeList()
		LOCAL lnRecno
		lnRecno = RECNO('ctypedefs')
		SELECT ctypedefs
		SCAN FOR !hidetype
			add_knowntype(ALLTRIM(typename))
		ENDSCAN
		GO (lnRecno) IN ctypedefs
	ENDPROC
	
	FUNCTION AddType(lcTypename)
		add_knowntype(ALLTRIM(lcTypename))	
	ENDFUNC
	
	FUNCTION RemoveType(lcTypename)
		remove_knowntype(ALLTRIM(lcTypename))	
	ENDFUNC

	FUNCTION ReplaceType(lcOldType,lcNewType)
		remove_knowntype(ALLTRIM(lcOldType))
		add_knowntype(ALLTRIM(lcNewType))
	ENDFUNC
	
	PROCEDURE RemoveComments(lcInput,lcOutput)
		remove_comments(THIS.cDir + lcInput,THIS.cDir + lcOutput)	
	ENDPROC

	PROCEDURE ParseFile(lcFile)
		RETURN start_parse(THIS.cDir + lcFile)
	ENDPROC
	
	PROCEDURE WalkParsedTypes
		LOCAL loParsedTypes
		loParsedTypes = CREATEOBJECT('PARSEDTYPES',get_parsedtypehead())
		DO WHILE loParsedTypes.Address != 0
			THIS.ProcessDeclaration(loParsedTypes.ParseNode)
			loParsedTypes.Address = loParsedTypes.NextNode
		ENDDO
	ENDPROC

	&& main entry point to process a T_DECLARATION node
	&& Parameters:
	&& lnAddress = address of T_DECLARATION node to process
	&& it calls the appropriate entry function to process the declaration and then 
	&& add's the generated TYPENODE to the array THIS.laTypes
	PROCEDURE ProcessDeclaration(lnAddress)
		LOCAL loDeclaration, loTypeSpec, loDeclKind, loType

		loDeclaration = CREATEOBJECT('PARSENODE',lnAddress)
		loTypeSpec = CREATEOBJECT('PARSENODE',loDeclaration.link1)
		loTypeSpec.Address = loTypeSpec.link1
		
		DO CASE
			CASE loTypeSpec.nType = T_STRUCT
				loType = THIS.BuildStructure(loTypeSpec.Address)
				THIS.ProcessDeclarators(loDeclaration.link2,loType)
				THIS.FixStructMemberNames(loType)
			CASE loTypeSpec.nType = T_ENUM
				loType = THIS.BuildEnum(loTypeSpec.Address)
				THIS.ProcessDeclarators(loDeclaration.link2,loType)
			OTHERWISE
				loType = THIS.BuildBasic(loTypeSpec.Address)
		ENDCASE
		
		THIS.AddParsedType(loType)
	ENDPROC	

	&& entry function to build a structure/union TYPENODE
	&& Parameters:
	&& lnAddress = adress of T_STRUCT node
	&&
	&& e.g.
	&& link1 -> T_STRUCT
	&&	link1 -> T_STRUCT
	&&	link2 -> T_IDENTIFIER = _RECT
	&&	link3 -> T_STRUCTDECLARATION
	&&		link2 -> T_TYPESPECIFIER
	&&			link1 -> T_TYPENAME = LONG
	&&		link3 -> T_STRUCTDECLARATOR
	&&			link2 -> T_IDENTIFIER = left
	&&		link1 -> T_STRUCTDECLARATION
	&&			link2 -> T_TYPESPECIFIER ......
	&&
	&& returns the the processed structure/union in a TYPENODE object
	PROCEDURE BuildStructure(lnAddress)
		
		LOCAL loStructHead, loStructType, loStructTag, loDecls, loTypeNode, loTypeSpec

		loStructHead = CREATEOBJECT('PARSENODE',lnAddress)
		loStructType = CREATEOBJECT('PARSENODE',loStructHead.link1) && pointer to node with type T_STRUCT or T_UNION
		loStructTag = CREATEOBJECT('PARSENODE',loStructHead.link2) && pointer to tag of structure or 0
		loDecls = CREATEOBJECT('PARSENODE',loStructHead.link3) && pointer to linked list of T_STRUCTDECLARATION nodes
		
		loTypeNode = CREATEOBJECT('TYPENODE') && the new structure
		loTypeNode.nType = IIF(loStructType.nType = T_STRUCT,TT_STRUCTHEADER,TT_UNIONHEADER)
		loTypeNode.Typemask = TM_STRUCT
		
		IF loStructTag.Address != 0
			loTypeNode.cName = loStructTag.String
		ENDIF
			
		&& iterate over linked list of T_STRUCTDECLARATION nodes
		DO WHILE loDecls.Address != 0
			
			&& process the type specifiers of the declaration
			loTypeSpec = THIS.ProcessTypeSpecs(loDecls.link2)

			&& process the declarators of the struct declaration
			THIS.ProcessStructDeclarators(loDecls.link3,loTypeSpec,loTypeNode)

			&& skip to next T_STRUCTDECLARATION node
			loDecls.Address = loDecls.link1
			
		ENDDO
		
		RETURN loTypeNode
		
	ENDPROC
	
	
	PROCEDURE BuildEnum(lnAddress)	
		
		
		LOCAL loEnumHead, loEnumList, loConstant, loEnumTag, loTypeNode, loEnumerator, lnEnumValue

		loEnumHead = CREATEOBJECT('PARSENODE',lnAddress)
		loEnumTag = CREATEOBJECT('PARSENODE',loEnumHead.link1)
		loEnumList = CREATEOBJECT('PARSENODE',loEnumHead.link2)
		
		loEnumName = CREATEOBJECT('PARSENODE')
		loConstant = CREATEOBJECT('PARSENODE')
		
		loTypeNode = CREATEOBJECT('TYPENODE') && the new typenode
		loTypeNode.nType = TT_ENUMHEADER
		loTypeNode.Typemask = TM_ENUM
		
		IF loEnumTag.Address != 0
			loTypeNode.cName = loEnumTag.String
		ENDIF
		
		lnEnumValue = -1

		DO WHILE loEnumList.Address != 0
		
			loEnumerator = CREATEOBJECT('TYPENODE')
			
			loEnumName.Address = loEnumList.link2
			loConstant.Address = loEnumList.link3
			
			IF loEnumName.Address != 0
				loEnumerator.cName = loEnumName.String
			ENDIF
			
			IF loConstant.Address != 0
				lnEnumValue = loConstant.Constant
			ELSE
				lnEnumValue = lnEnumValue + 1 
			ENDIF

			loEnumerator.ValueOf = lnEnumValue
			
			loTypeNode.AddMember(loEnumerator)
			
			loEnumList.Address = loEnumList.link1		
		
		ENDDO
		
		RETURN loTypeNode
	
	ENDPROC
	
	PROCEDURE BuildBasic
	
	
	ENDPROC
	
	&& this function processes a linked list of PARSENODE's of type T_TYPESPECIFIER
	&& Parameters:
	&& lnAddress = address of first T_TYPESPECIFIER node
	&&
	&&	T_TYPESPECIFIER
	&& 		link1 -> T_INT
	&&			link2 -> T_TYPESPECIFIER
	&& 				link1 -> T_SHORT
	&&				link2 -> T_TYPESPECIFIER
	&&					link1 -> T_UNSIGNED
	&&					link2 -> 0
	&&
	&& it returns an object of type TYPENODE in which the typeinformation is stored,
	&& it call's recursivly into BuildStructure/BuildEnum if it detects such a type specifier
	&& and in this case it returns the completly processed structure 
	PROCEDURE ProcessTypeSpecs(lnAddress)

	LOCAL loType, loTSpec, loSpecifier, lnTypeKind, lnMask

	lnMask = 0 && mask to store various specifiers into one value (see TM_ #DEFINE's)
	loType = CREATEOBJECT('TYPENODE')
	loType.nType = TT_BASICTYPE
	loTSpec = CREATEOBJECT('PARSENODE',lnAddress) && T_TYPESPECIFIER node
	loSpecifier = CREATEOBJECT('PARSENODE') && the actual specifier linked to T_TYPESPECIFIER

	DO WHILE loTSpec.Address != 0

		loSpecifier.Address = loTSpec.link1 && get the address of the actual specifier linked to T_TYPESPECIFIER

		lnTypeKind = loSpecifier.nType 

		IF loType.nType = TT_BASICTYPE

			DO CASE

				CASE lnTypeKind = T_TYPENAME
					IF BITAND(lnMask,TM_VOID+TM_CHAR+TM_WCHAR+TM_INT8+TM_SHORT+TM_INT+TM_LONG+TM_FLOAT+ ;
							TM_DOUBLE+TM_INT64+TM_SIGNED+TM_UNSIGNED) > 0
						THIS.AddError(INVALIDTYPESPEC,TM2String(lnMask+TM_TYPENAME),loSpecifier.nLineNo)
					ELSE
						loType.nType = TT_TYPEDEF
						loType.TypeName = loSpecifier.String
					ENDIF

				CASE lnTypeKind = T_VOID
					IF BITAND(lnMask,TM_VOID+TM_CHAR+TM_WCHAR+TM_INT8+TM_SHORT+TM_INT+TM_LONG+TM_FLOAT+ ;
							TM_DOUBLE+TM_INT64+TM_SIGNED+TM_UNSIGNED) > 0
						THIS.AddError(INVALIDTYPESPEC,TM2String(lnMask+TM_VOID),loSpecifier.nLineNo)
					ELSE
						lnMask = BITOR(lnMask,TM_VOID)
					ENDIF

				CASE lnTypeKind = T_CHAR
					IF BITAND(lnMask,TM_VOID+TM_CHAR+TM_WCHAR+TM_INT8+TM_SHORT+TM_INT+TM_LONG+TM_FLOAT+ ;
							TM_DOUBLE+TM_INT64) > 0
						THIS.AddError(INVALIDTYPESPEC,TM2String(lnMask+TM_CHAR),loSpecifier.nLineNo)
					ELSE
						lnMask = BITOR(lnMask,TM_CHAR)
					ENDIF

				CASE lnTypeKind = T_WCHAR
					IF BITAND(lnMask,TM_VOID+TM_CHAR+TM_WCHAR+TM_INT8+TM_SHORT+TM_INT+TM_LONG+TM_FLOAT+ ;
							TM_DOUBLE+TM_INT64) > 0
						THIS.AddError(INVALIDTYPESPEC,TM2String(lnMask+TM_WCHAR),loSpecifier.nLineNo)
					ELSE
						lnMask = BITOR(lnMask,TM_WCHAR)
					ENDIF

				CASE lnTypeKind = T_INT8
					IF BITAND(lnMask,TM_VOID+TM_CHAR+TM_WCHAR+TM_SHORT+TM_INT+TM_LONG+TM_FLOAT+ ;
							TM_DOUBLE+TM_INT64) > 0
						THIS.AddError(INVALIDTYPESPEC,TM2String(lnMask+TM_INT8),loSpecifier.nLineNo)
					ELSE
						lnMask = BITOR(lnMask,TM_INT8)
					ENDIF

				CASE lnTypeKind = T_SHORT
					IF BITAND(lnMask,TM_VOID+TM_CHAR+TM_WCHAR+TM_INT8+TM_LONG+TM_FLOAT+TM_DOUBLE+ ;
							TM_INT64) > 0
						THIS.AddError(INVALIDTYPESPEC,TM2String(lnMask+TM_SHORT),loSpecifier.nLineNo)
					ELSE
						IF BITAND(lnMask,TM_INT) > 0 && int is already there
							lnMask = BITAND(lnMask,TM_SIGNED+TM_UNSIGNED) && remove anything but sign
							lnMask = BITOR(lnMask,TM_SHORT) && and make it a normal short
						ELSE
							lnMask = BITOR(lnMask,TM_SHORT)
						ENDIF
					ENDIF

				CASE lnTypeKind = T_INT
					IF BITAND(lnMask,TM_VOID+TM_CHAR+TM_WCHAR+TM_INT8+TM_INT+TM_FLOAT+TM_DOUBLE+ ;
							TM_INT64) > 0
						THIS.AddError(INVALIDTYPESPEC,TM2String(lnMask+TM_INT),loSpecifier.nLineNo)
					ELSE
						IF BITAND(lnMask,TM_SHORT+TM_LONG) = 0 && if "short" or "long" is already there .. just ignore "int"
							lnMask = BITOR(lnMask,TM_INT)
						ENDIF
					ENDIF

				CASE lnTypeKind = T_LONG
					IF BITAND(lnMask,TM_VOID+TM_CHAR+TM_WCHAR+TM_INT8+TM_SHORT+TM_FLOAT+TM_INT64) > 0
						THIS.AddError(INVALIDTYPESPEC,TM2String(lnMask+TM_LONG),loSpecifier.nLineNo)
					ELSE
						DO CASE
							CASE BITAND(lnMask,TM_LONG) > 0 && long long special case, remap to __int64
								lnMask = BITAND(lnMask,TM_SIGNED+TM_UNSIGNED) && remove anything except sign
								lnMask = BITOR(lnMask,TM_INT64) 
							CASE BITAND(lnMask,TM_INT) > 0 && if "int" is already there just ignore "long"
							OTHERWISE
								lnMask = BITOR(lnMask,TM_LONG)
						ENDCASE
					ENDIF

				CASE lnTypeKind = T_FLOAT
					IF BITAND(lnMask,TM_VOID+TM_CHAR+TM_WCHAR+TM_INT8+TM_SHORT+TM_INT+TM_LONG+TM_FLOAT+ ;
						TM_DOUBLE+TM_INT64+TM_UNSIGNED+TM_SIGNED) > 0
						THIS.AddError(INVALIDTYPESPEC,TM2String(lnMask+TM_FLOAT),loSpecifier.nLineNo)
					ELSE
						lnMask = BITOR(lnMask,TM_FLOAT)
					ENDIF

				CASE lnTypeKind = T_DOUBLE
					IF BITAND(lnMask,TM_VOID+TM_CHAR+TM_WCHAR+TM_INT8+TM_SHORT+TM_INT+TM_FLOAT+ ;
						TM_DOUBLE+TM_INT64+TM_UNSIGNED+TM_SIGNED) > 0
						THIS.AddError(INVALIDTYPESPEC,TM2String(lnMask+TM_DOUBLE),loSpecifier.nLineNo)
					ELSE
						lnMask = BITOR(lnMask,TM_DOUBLE)
					ENDIF
					
				CASE lnTypeKind = T_INT64
					IF BITAND(lnMask,TM_VOID+TM_CHAR+TM_WCHAR+TM_INT8+TM_SHORT+TM_INT+TM_FLOAT+TM_DOUBLE+TM_INT64) > 0
						THIS.AddError(INVALIDTYPESPEC,TM2String(lnMask+TM_INT64),loSpecifier.nLineNo)
					ELSE
						lnMask = BITOR(lnMask,TM_INT64)
					ENDIF

				CASE lnTypeKind = T_SIGNED
					IF BITAND(lnMask,TM_VOID+TM_WCHAR+TM_FLOAT+TM_DOUBLE+TM_UNSIGNED+TM_SIGNED) > 0
						THIS.AddError(INVALIDTYPESPEC,TM2String(lnMask+TM_SIGNED),loSpecifier.nLineNo)
					ELSE
						lnMask = BITOR(lnMask,TM_SIGNED)
					ENDIF

				CASE lnTypeKind = T_UNSIGNED
					IF BITAND(lnMask,TM_VOID+TM_WCHAR+TM_FLOAT+TM_DOUBLE+TM_UNSIGNED+TM_SIGNED) > 0
						THIS.AddError(INVALIDTYPESPEC,TM2String(lnMask+TM_UNSIGNED),loSpecifier.nLineNo)
					ELSE
						lnMask = BITOR(lnMask,TM_UNSIGNED)
					ENDIF
					
				CASE lnTypeKind = T_STRUCT
					IF lnMask > 0
						THIS.AddError(INVALIDTYPESPEC,TM2String(lnMask+TM_STRUCT),loSpecifier.nLineNo)
					ELSE
						loType = THIS.BuildStructure(loTSpec.link1)
						loType.nLineNo = loTSpec.nLineNo
					ENDIF

				CASE lnTypeKind = T_ENUM
					IF lnMask > 0
						THIS.AddError(INVALIDTYPESPEC,TM2String(lnMask+TM_ENUM),loSpecifier.nLineNo)
					ELSE
						loType = THIS.BuildEnum(loTSpec.link1)
						loType.nLineNo = loTSpec.nLineNo
					ENDIF
					
			
			ENDCASE
		
		ELSE
			THIS.AddError(INVALIDTYPESPEC,"additional type specifier after struct/union/enum or typedef",loSpecifier.nLineNo)
		ENDIF
			
		&& skip to next T_TYPESPECIFIER
		loTSpec.Address = loTSpec.link2
		
	ENDDO
	
	IF loType.nType = TT_BASICTYPE
		
		DO CASE
			CASE lnMask = TM_SIGNED && special case "signed" only - remap to int
				lnMask = TM_INT
			CASE lnMask = TM_UNSIGNED && special case "unsigned" only - remap to unsigned int
				lnMask = TM_INT+TM_UNSIGNED
			CASE BITAND(lnMask,TM_SIGNED) > 0 && remove "signed" cause it is the default anyway ..
				lnMask = BITAND(lnMask,BITNOT(TM_SIGNED))
		ENDCASE
		
		loType.Typemask = lnMask
	ENDIF
	
	RETURN loType
	
	ENDPROC
	
	&& this functions processes as linked list of T_STRUCTDECLARATOR's 
	&& Parameters:
	&& lnAddress = address of first T_STRUCTDECLARATOR node
	&&
	&& loTypeSpec = object of type TYPENODE which holds the information
	&& of the corresponding type specifier('s) for the declaration (T_TYPESPECIFIER's)
	&& which is returned from THIS.ProcessTypeSpecs()
	&& 
	&& loStruct =  object of type TYPENODE (with nType = TT_STRUCTHEADER|T_UNIONHEADER) to which
	&& new members are added
	&&
	&&	T_STRUCTDECLARATION
	&&		link2 -> T_TYPESPECIFIER
	&&			link1 -> T_TYPENAME = LONG
	&&		link3 -> T_STRUCTDECLARATOR
	&&			link2 -> T_IDENTIFIER = left
	&&			link1 -> T_STRUCTDECLARATOR
	&&				link2 -> T_IDENTIFER = top
	&&				link1 -> T_STRUCTDECLARATOR
	&&			
	PROCEDURE ProcessStructDeclarators(lnAddress,loTypeSpec,loStruct)

		LOCAL loStructDeclar, loNewDeclaration, loNode, loConstant

		loStructDeclar = CREATEOBJECT('PARSENODE',lnAddress) && PARSENODE of type T_STRUCTDECLARATOR
		loDecal = CREATEOBJECT('PARSENODE') && either T_IDENTIFIER, T_SUBSCRIPT or T_POINTER
		loConstant = CREATEOBJECT('PARSENODE') && to read T_CONSTANT's linked to T_SUBSCRIPT or T_STRUCTDECLARTOR
		
		IF loStructDeclar.Address = 0 && is an anonymous struct or union (enforced by parser)
			loStruct.AddMember(loTypeSpec)
			RETURN
		ENDIF
		
		DO WHILE loStructDeclar.Address != 0

			loNewDeclaration = loTypeSpec.Clone() && duplicate the typeinformation node

			IF loStructDeclar.link3 != 0 && is it a bitfield?
				loConstant.Address = loStructDeclar.link3
				loNewDeclaration.nBits = loConstant.Constant
			ENDIF
			
			loDecal.Address = loStructDeclar.link2

			DO WHILE .T.
	
				DO CASE
					CASE loDecal.nType = T_IDENTIFIER
						loNewDeclaration.cName = THIS.FixMemberName(loDecal.String)
						loNewDeclaration.nLineNo = loDecal.nLineNo
						EXIT
					CASE loDecal.nType = T_SUBSCRIPT
						loConstant.Address = loDecal.link2
						IF loConstant.Address != 0
							loNewDeclaration.AddSubscript(loConstant.Constant)
						ELSE
							loNewDeclaration.AddSubscript(0)
						ENDIF
					CASE loDecal.nType = T_POINTER
						loNewDeclaration.Indirect = loNewDeclaration.Indirect + 1
				ENDCASE
				
				&& skip to next declaration modifier/identifier
				loDecal.Address = loDecal.link1
				
			ENDDO
			
			loStruct.AddMember(loNewDeclaration)
			
			&& skip to next T_STRUCTDECLARATOR node
			loStructDeclar.Address = loStructDeclar.link1
			
		ENDDO
	
	ENDPROC
	
	PROCEDURE ProcessDeclarators(lnAddress,loType)
	
		LOCAL loDeclNode, loNode, loConstant, loDeclar
		loDeclNode = CREATEOBJECT('PARSENODE',lnAddress)
		loNode = CREATEOBJECT('PARSENODE')
		loConstant = CREATEOBJECT('PARSENODE')

		DO WHILE loDeclNode.Address != 0
			
			loNode.Address = loDeclNode.link2

			loDeclar = CREATEOBJECT('TYPENODE')
			loDeclar.Typemask = loType.Typemask
			
			DO WHILE .T.
				
				DO CASE
					CASE loNode.nType = T_IDENTIFIER
						loDeclar.cName = loNode.String
						EXIT
					CASE loNode.nType = T_SUBSCRIPT
						loConstant.Address = loNode.link2
						IF loConstant.Address != 0
							loDeclar.AddSubscript(loConstant.Constant)
						ELSE
							loDeclar.AddSubscript(0) && e.g. someStructField[] (is possible as the last member in a struct)
						ENDIF
					CASE loNode.nType = T_POINTER
						loDeclar.Indirect = loDeclar.Indirect + 1
				ENDCASE
				
				loNode.Address = loNode.link1
				
			ENDDO
		
			loType.AddDeclarator(loDeclar)
		
			loDeclNode.Address = loDeclNode.link1
		
		ENDDO
		
		IF !EMPTY(loType.cName)
			loDeclar = CREATEOBJECT('TYPENODE')
			loDeclar.cName = loType.cName
			loType.AddDeclarator(loDeclar)
		ENDIF
	
	ENDPROC
	
	&& function to resolve name clashes between structure member names and intrinsic VFP properties and keywords
	PROCEDURE FixMemberName(lcMember)
		SELECT keywords
		LOCATE FOR ALLTRIM(propname) == UPPER(lcMember)
		IF FOUND()
			RETURN "m" + lcMember
		ELSE
			RETURN lcMember
		ENDIF
	ENDPROC
	
	&& function to resolve name clashes between fields inside embedded structs/unions
	PROCEDURE FixStructMemberNames(loType)
		LOCAL xj
		FOR xj = 1 TO ALEN(loType.laMembers)
			IF INLIST(loType.laMembers[xj].nType,TT_STRUCTHEADER,TT_UNIONHEADER) AND VARTYPE(loType.laMembers[1]) = 'O'
				THIS.FixStructMemberNamesEx(loType.laMembers[xj])			
			ENDIF
		ENDFOR
	ENDPROC
	
	PROCEDURE FixStructMemberNamesEx(loType)
		LOCAL xj
		FOR xj = 1 TO ALEN(loType.laMembers)
			IF !EMPTY(loType.cName)
				loType.laMembers[xj].cName = LEFT(loType.cName + "_" + loType.laMembers[xj].cName,128) && 128 is the maximum 
			ENDIF
		ENDFOR

		FOR xj = 1 TO ALEN(loType.laMembers)
			IF INLIST(loType.laMembers[xj].nType,TT_STRUCTHEADER,TT_UNIONHEADER) AND VARTYPE(loType.laMembers[xj].laMembers[1]) = 'O'
				THIS.FixStructMemberNamesEx(loType.laMembers[xj])
			ENDIF
		ENDFOR
	ENDPROC
	
	PROCEDURE PrepareTypeForCG(lnIndex)
		LOCAL loType, lcFilter
		loType = THIS.laTypes[lnIndex]

		IF loType.nType == TT_ENUMHEADER
			RETURN loType
		ELSE
			loType = loType.Clone()
		ENDIF
		
		SELECT ctypedefs
		lcFilter = "INLIST(charset,0," + ALLTRIM(STR(THIS.nCharset)) + ;
					") AND INLIST(platform,0," + ALLTRIM(STR(THIS.nPlatform)) + ")"
		SET FILTER TO &lcFilter

		IF !THIS.ResolveTypeInfo(loType)
			RETURN .F.
		ENDIF
		
		SELECT ctypedefs
		SET FILTER TO
		
		THIS.ComputeOffsets(loType)
		
		RETURN loType
	ENDPROC
		
	PROCEDURE ResolveTypeInfo(loType)

		&&SET STEP ON
		DO CASE
			CASE loType.nType = TT_BASICTYPE

				IF loType.bArray AND BITAND(loType.Typemask,TM_CHAR+TM_WCHAR+TM_BYTE) > 0
					loType.Typemask = BITOR(loType.Typemask,TM_ARRAYOF)
				ENDIF

				SELECT cbasetypes
				LOCATE FOR typemask = loType.Typemask AND indirect = loType.Indirect
				IF !FOUND()
					THIS.AddError(TYPENOTFOUND,TM2String(loType.Typemask),loType.nLineNo)
					RETURN .F.
				ENDIF

				IF loType.Indirect = 0
					loType.SizeOf = cbasetypes.sizeof
					loType.AlignOf = cbasetypes.alignof
				ELSE
					loType.SizeOf = THIS.nPointerSize
					loType.AlignOf = THIS.nPointerSize
					loType.BaseSize = cbasetypes.sizeof				
				ENDIF			
	
				loType.Assertion = ALLTRIM(cbasetypes.assertion)
				loType.ReadFunc = ALLTRIM(cbasetypes.readfunc)
				loType.WriteFunc = ALLTRIM(cbasetypes.writefunc)
				loType.CSharpType = ALLTRIM(cbasetypes.csharptype)
				loType.MarshalGet = ALLTRIM(cbasetypes.marshalget)
				loType.MarshalSet = ALLTRIM(cbasetypes.marshalset)

			CASE loType.nType = TT_TYPEDEF

				LOCAL lcTypeName, lnRecno, lnIndirect
				lcTypeName = loType.typename
				lnRecno = 0
				lnIndirect = 0
				SELECT ctypedefs
				LOCATE FOR ALLTRIM(typename) == lcTypename

				IF !FOUND()
					THIS.AddError(TYPENOTFOUND,loType.Typename,loType.nLineNo)
					RETURN .F.
				ENDIF

				DO WHILE .T.
					lnIndirect = lnIndirect + ctypedefs.indirect
					IF cTypedefs.Typemask != 0
						EXIT
					ENDIF
					lcTypeName = ALLTRIM(ctypedefs.basetype)
					LOCATE FOR ALLTRIM(typename) == lcTypeName
					IF !FOUND()
						THIS.AddError(TYPENOTFOUND,lcTypeName,loType.nLineNo)
						RETURN .F.
					ENDIF
				ENDDO

				lnIndirect = lnIndirect + loType.Indirect && needed for later code generation
				loType.Indirect = lnIndirect
				loType.Typemask = ctypedefs.typemask
				
				IF loType.typemask = TM_STRUCT
				
					DO CASE
						CASE lnIndirect = 0
							loType.SizeOf = ctypedefs.sizeof
							loType.AlignOf = ctypedefs.alignof
							loType.VfpClass = ALLTRIM(ctypedefs.vfpclass)
							loType.CSharpType = ALLTRIM(ctypedefs.vfpclass)
							
						CASE lnIndirect = 1
							loType.SizeOf = THIS.nPointerSize
							loType.AlignOf = THIS.nPointerSize
							loType.ReadFunc = "ReadPointer"
							loType.WriteFunc = "WritePointer"
							loType.CSharpType = "IntPtr"
							loType.MarshalGet = "ReadIntPtr"
							loType.MarshalSet = "WriteIntPtr"
						
						OTHERWISE
							loType.SizeOf = THIS.nPointerSize
							loType.AlignOf = THIS.nPointerSize
							loType.ReadFunc = "NoCodeGenerationAvailable"
							loType.WriteFunc = "NoCodeGenerationAvailable"
							loType.CSharpType = "??"
							loType.MarshalGet = "NoCodeGenerationAvailable"
							loType.MarshalSet = "NoCodeGenerationAvailable"
					ENDCASE		
					
				ELSE
				
					IF loType.bArray AND BITAND(loType.TypeMask,TM_CHAR+TM_WCHAR+TM_BYTE) > 0 && handle W/CharArray special case
						loType.Typemask = BITOR(loType.Typemask,TM_ARRAYOF)
					ENDIF

					SELECT cbasetypes
					LOCATE FOR typemask = loType.typemask AND indirect = lnIndirect
					IF !FOUND()
						THIS.AddError(TYPENOTFOUND,loType.Typename,loType.nLineNo)
						RETURN .F.
					ENDIF

					IF lnIndirect = 0
						loType.SizeOf = cbasetypes.sizeof
						loType.AlignOf = cbasetypes.alignof
					ELSE
						loType.SizeOf = THIS.nPointerSize
						loType.AlignOf = THIS.nPointerSize
						loType.BaseSize = cbasetypes.sizeof
					ENDIF
					loType.Assertion = ALLTRIM(cbasetypes.assertion)
					loType.ReadFunc = ALLTRIM(cbasetypes.readfunc)
					loType.WriteFunc = ALLTRIM(cbasetypes.writefunc)
					loType.CSharpType = ALLTRIM(cbasetypes.csharptype)
					loType.MarshalGet = ALLTRIM(cbasetypes.marshalget)
					loType.MarshalSet = ALLTRIM(cbasetypes.marshalset)
					
				ENDIF
				
			CASE loType.nType = TT_ENUMHEADER

				SELECT cbasetypes
				LOCATE FOR typemask = TM_ENUM AND indirect = loType.indirect
				loType.Typemask = TM_ENUM

				DO CASE
					CASE loType.Indirect = 0
						loType.SizeOf = cbasetypes.sizeof
						loType.AlignOf = cbasetypes.alignof
						loType.Assertion = ALLTRIM(cbasetypes.assertion)
						loType.ReadFunc = ALLTRIM(cbasetypes.readfunc)
						loType.WriteFunc = ALLTRIM(cbasetypes.writefunc)
						loType.CSharpType = ALLTRIM(cbasetypes.csharptype)
						loType.MarshalGet = ALLTRIM(cbasetypes.marshalget)
						loType.MarshalSet = ALLTRIM(cbasetypes.marshalset)

					CASE loType.Indirect = 1
						loType.SizeOf = THIS.nPointerSize
						loType.AlignOf = THIS.nPointerSize
						loType.BaseSize = cbasetypes.sizeof
						loType.Assertion = ALLTRIM(cbasetypes.assertion)
						loType.ReadFunc = ALLTRIM(cbasetypes.readfunc)
						loType.WriteFunc = ALLTRIM(cbasetypes.writefunc)
						loType.CSharpType = ALLTRIM(cbasetypes.csharptype)
						loType.MarshalGet = ALLTRIM(cbasetypes.marshalget)
						loType.MarshalSet = ALLTRIM(cbasetypes.marshalset)
					
					OTHERWISE
						loType.SizeOf = THIS.nPointerSize
						loType.AlignOf = THIS.nPointerSize
						loType.ReadFunc = 'NoCodeGenerationAvailable'
						loType.WriteFunc = 'NoCodeGenerationAvailable'
						loType.CSharpType = "??"
						loType.MarshalGet = "NoCodeGenerationAvailable"
						loType.MarshalSet = "NoCodeGenerationAvailable"
						
				ENDCASE
			
			CASE INLIST(loType.nType,TT_STRUCTHEADER,TT_UNIONHEADER) AND VARTYPE(loType.laMembers[1]) = 'L'

				loType.nType = TT_STRUCTREF
				DO CASE
					CASE loType.Indirect = 0
						SELECT ctypedefs
						LOCATE FOR ALLTRIM(typename) == loType.cName AND indirect = 0
						IF !FOUND()
							THIS.AddError(TYPENOTFOUND,loType.Typename,loType.nLineNo)
							RETURN .F.
						ENDIF
						loType.SizeOf = ctypedefs.sizeof
						loType.AlignOf = ctypedefs.alignof
						loType.VfpClass = ALLTRIM(ctypedefs.vfpclass)
						loType.CSharpType = ALLTRIM(ctypedefs.vfpclass)
						
					CASE loType.Indirect = 1
						SELECT cbasetypes
						LOCATE FOR typemask = TM_STRUCT AND indirect = 1
						loType.SizeOf = THIS.nPointerSize
						loType.AlignOf = THIS.nPointerSize
						loType.Assertion = ALLTRIM(cbasetypes.assertion)
						loType.ReadFunc = ALLTRIM(cbasetypes.readfunc)
						loType.WriteFunc = ALLTRIM(cbasetypes.writefunc)
						loType.CSharpType = ALLTRIM(cbasetypes.csharptype)
						loType.MarshalGet = ALLTRIM(cbasetypes.marshalget)
						loType.MarshalSet = ALLTRIM(cbasetypes.marshalset)
												
					OTHERWISE
						loType.SizeOf = THIS.nPointerSize
						loType.AlignOf = THIS.nPointerSize
						loType.ReadFunc = "NoCodeGenerationAvailable"
						loType.WriteFunc = "NoCodeGenerationAvailable" 
						loType.CSharpType = "??"
						loType.MarshalGet = "NoCodeGenerationAvailable"
						loType.MarshalSet = "NoCodeGenerationAvailable"
						
				ENDCASE

			CASE INLIST(loType.nType,TT_STRUCTHEADER,TT_UNIONHEADER)

				LOCAL xj
				FOR xj = 1 TO ALEN(loType.laMembers)
					IF !THIS.ResolveTypeInfo(loType.laMembers[xj])
						RETURN .F.
					ENDIF
				ENDFOR

		ENDCASE
		
		RETURN .T.
	
	ENDPROC
	
	&& this functions is the entry point to compute the offsets of the struct/union members
	&& it first checks to see if the type passed has any substructs/unions and if so it calls
	&& itself .. so the struct/union is computed from the inside out 
	&& it then calls ComputeAggregate which does the actual computations
	PROCEDURE ComputeOffsets(loType)

		LOCAL xj

		FOR xj = 1 TO ALEN(loType.laMembers)
			IF INLIST(loType.laMembers[xj].nType,TT_STRUCTHEADER,TT_UNIONHEADER)
				THIS.ComputeOffsets(loType.laMembers[xj])
			ENDIF
		ENDFOR
		
		THIS.ComputeAggregate(loType)
		
		IF VARTYPE(loType.laDeclar[1]) = 'O'
			FOR xj = 1 TO ALEN(loType.laDeclar)
				loType.laDeclar[xj].SizeOf = loType.SizeOf			
				IF loType.laDeclar[xj].Indirect = 0
					loType.laDeclar[xj].AlignOf = loType.AlignOf
				ELSE
					loType.laDeclar[xj].AlignOf = THIS.nPointerSize
				ENDIF
			ENDFOR
		ENDIF
		
	ENDPROC

	&& this function computes the actual offsets of struct/union members
	PROCEDURE ComputeAggregate(loType)

		LOCAL lnOffset, lnMaxAlign, lnMaxSizeOf, lnNextAddress, lnNextPragmaAddress, lnAddress, xj

		lnOffset = 0
		lnMaxAlign = 0
		lnMaxSizeOf = 0
		
		FOR xj = 1 TO ALEN(loType.laMembers)
			&& store biggest alignment and size
			lnMaxAlign = MAX(lnMaxAlign,loType.laMembers[xj].AlignOf)
			lnMaxSizeOf = MAX(lnMaxSizeOf,loType.laMembers[xj].SizeOf)

			&& compute next address both for pragma option and alignment requirement of the type
			IF xj > 1
				IF MOD(lnOffset,loType.laMembers[xj].AlignOf) = 0 OR MOD(lnOffset,THIS.nPragmaPack) = 0
					lnNextAddress = lnOffset
					lnNextPragmaAddress = lnOffset
				ELSE
					lnNextAddress = lnOffset + loType.laMembers[xj].AlignOf - MOD(lnOffset,loType.laMembers[xj].AlignOf)
					lnNextPragmaAddress = lnOffset + THIS.nPragmaPack - MOD(lnOffset,THIS.nPragmaPack)
				ENDIF
			ELSE
				lnNextAddress = 0
				lnNextPragmaAddress = 0
			ENDIF
			&& the smaller one wins
			lnAddress = MIN(lnNextAddress,lnNextPragmaAddress) 
			
			loType.laMembers[xj].OffsetOf = lnAddress

			IF loType.nType = TT_STRUCTHEADER
				lnOffset = lnAddress + loType.laMembers[xj].SizeOf
			ENDIF
			
		ENDFOR

		loType.AlignOf = MIN(lnMaxAlign,THIS.nPragmaPack)
		
		IF loType.nType = TT_STRUCTHEADER
			loType.SizeOf = lnOffset + MOD(lnOffset,loType.AlignOf)
		ELSE
			loType.SizeOf = lnMaxSizeOf
		ENDIF
			
	ENDPROC
	
	PROCEDURE AddParsedType(loType)
		IF VARTYPE(THIS.laTypes[1]) = 'L'
			THIS.laTypes[1] = loType
		ELSE
			DIMENSION THIS.laTypes[ALEN(THIS.laTypes)+1]
			AINS(THIS.laTypes,1)
			THIS.laTypes[1] = loType
		ENDIF
	ENDPROC
	
	PROCEDURE ClearParsedTypes()
		DIMENSION THIS.laTypes[1]
		THIS.laTypes[1] = .F.
		clear_parsedtypes()
	ENDPROC	
	
	PROCEDURE ClearErrors
		THIS.nWalkErrors = 0
		clear_errors()
		SELECT cerrors
		ZAP
	ENDPROC

	PROCEDURE DisplayErrors()
		LOCAL loErrorNode, lnError
		loErrorNode = CREATEOBJECT('PARSEERRORS',get_parseerrorhead())
		lnError = 0
		DO WHILE loErrorNode.Address != 0
			lnError = lnError + 1 
			INSERT INTO cErrors VALUES (lnError,loErrorNode.pErrorMes,loErrorNode.nLineNo)
			loErrorNode.Address = loErrorNode.NextError		
		ENDDO
	ENDPROC
	
	PROCEDURE AddError(lcMessage,lcValue,nLineNumber)
		THIS.nWalkErrors = THIS.nWalkErrors + 1 
		LOCAL lnMax
		CALCULATE MAX(cErrors.nOrder) TO lnMax IN cErrors
		INSERT INTO cErrors VALUES (lnMax+1,lcMessage + ": " + CRLF + "'" + lcValue + "'",nLineNumber)
	ENDPROC
	
ENDDEFINE

