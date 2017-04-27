#INCLUDE "vfp2c.h"

IF _VFP.StartMode = 0
	CD (_VFP.ActiveProject.HomeDir)
ENDIF

_SCREEN.FontName = 'Tahoma'
_SCREEN.FontSize = 9

SET CONFIRM ON
SET DELETED ON
SET NOTIFY OFF
SET NOTIFY CURSOR OFF
SET NULLDISPLAY TO ''
SET MULTILOCKS ON
SET OPTIMIZE ON
SET SAFETY OFF
SET TALK OFF
SET UDFPARMS TO VALUE

SET PROCEDURE TO cparser, codegen, cparsetypes ADDITIVE
SET LIBRARY TO vfp2c32.fll ADDITIVE

ON ERROR ErrorHandler(ERROR(),MESSAGE(),LINENO(),PROGRAM())

DO FORM frmvfp2c

IF _VFP.StartMode != 0
	READ EVENTS
	CLEAR ALL
ENDIF

FUNCTION ErrorHandler(nErrorNo,cMessage,nLineNo,cProgram)
	LOCAL lcMessage, lnRetVal
	lcMessage = 'An Error occurred: ' + CRLF + ;
				'Error No.: ' + ALLTRIM(STR(nErrorNo)) + CRLF + ;
				'Message: "' + cMessage + '"' + CRLF + ;
				'Procedure: "' + cProgram + '"' + CRLF + ;
				'LineNo: ' + ALLTRIM(STR(nLineNo)) + CRLF
				
	lnRetVal = MESSAGEBOX(lcMessage,48+2,'Error')

	DO CASE
		CASE lnRetVal = 3 && abort
			RETURN TO main
		CASE lnRetVal = 4 && retry
			RETRY
		CASE lnRetVal = 5 && ignore
	ENDCASE

ENDFUNC