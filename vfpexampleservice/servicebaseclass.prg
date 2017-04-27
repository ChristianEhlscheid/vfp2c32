#INCLUDE "foxpro.h"

&& base class for all services, just implements a common errorhandler that handles Win32Exceptions thrown by the classes in win32classes.prg, VFP2C32 errors and normal VFP errors
DEFINE CLASS ServiceBaseclass AS Session OLEPUBLIC

	ThrowError_COMATTRIB = COMATTRIB_RESTRICTED && don't expose error event
	FUNCTION ThrowError(loExc)
		LOCAL lcError, lcMessage

		TRY

			DO CASE
				CASE VARTYPE(m.loExc.UserValue) = 'O' AND m.loExc.UserValue.Name = 'Win32Exception'
					m.lcError = 'Win32Exception' + CHR(10) + ;
						'Error No : ' + TRANSFORM(m.loExc.UserValue.ErrorNo) + CHR(10) + ;
					    'Function : ' + m.loExc.UserValue.Details + CHR(10) + ;
					    'Program : ' + m.loExc.UserValue.Procedure + CHR(10) + ;
					    'LineNo: ' + TRANSFORM(m.loExc.UserValue.LineNo) + CHR(10) + ;
					    'StackTrace: ' + m.loExc.UserValue.StackTrace
					m.lcMessage = m.loExc.UserValue.Message
					
				CASE m.loExc.ErrorNo = 1098
					LOCAL laError[1]
					AERROREX('laError')		
					m.lcError = 'Error No : ' + TRANSFORM(m.laError[1]) + CHR(10) + ;
							    'Function : ' + m.laError[2] + CHR(10) + ;
							    'LineNo: ' + + TRANSFORM(m.loExc.LineNo)
					m.lcMessage = m.laError[3]
					
				OTHERWISE
					m.lcError = 'Error No : ' + TRANSFORM(m.loExc.ErrorNo) + CHR(10) + ;
						    'LineContents : ' + m.loExc.LineContents + CHR(10) + ;
	   					    'LineNo: ' + + TRANSFORM(m.loExc.LineNo) + CHR(10) + ;				    
	   					    'Program : ' + m.loExc.Procedure + CHR(10)
	   				m.lcMessage = m.loExc.Message
	   				
	   			ENDCASE

		CATCH TO loExc2

			m.lcError = 'Error No : ' + TRANSFORM(m.loExc2.ErrorNo) + CHR(10) + ;
				    'LineContents : ' + m.loExc2.LineContents + CHR(10) + ;
				    'LineNo: ' + + TRANSFORM(m.loExc2.LineNo) + CHR(10) + ;				    
				    'Program : ' + m.loExc2.Procedure + CHR(10)
			m.lcMessage = m.loExc2.Message
	
		ENDTRY

		STRTOFILE(STRCONV(STRCONV(m.lcError + CHR(13) + m.lcMessage + CHR(13),1), 5), 'vfpsrvhost.log', 1)
		COMRETURNERROR(m.lcError, m.lcMessage)
	ENDFUNC

ENDDEFINE