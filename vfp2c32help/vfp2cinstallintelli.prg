LOCAL loData, lbUsed

WAIT WINDOW 'Installing intellisense data for VFP2C32 ...' NOWAIT NOCLEAR

TRY
	CD (FULLPATH(JUSTPATH(SYS(16))))
	USE (_FOXCODE) IN 0 SHARED ALIAS vfpintellicursor

	m.lbUsed = USED('vfp2cintelli')
	IF !m.lbUsed
		USE vfp2cintelli IN 0 SHARED
	ENDIF

	SELECT vfp2cintelli
	SCAN
		SELECT vfpintellicursor
		LOCATE FOR ALLTRIM(vfpintellicursor.uniqueid) == ALLTRIM(vfp2cintelli.uniqueid)

		IF !FOUND()
			APPEND BLANK IN vfpintellicursor
		ENDIF

		SELECT vfp2cintelli
		SCATTER MEMO NAME loData

		&& if tip
		IF m.loData.type = 'F' AND EMPTY(m.loData.abbrev)
			m.loData.tip = STREXTRACT(m.loData.user, '<tip>', '</tip>')
			m.loData.user = '<vfp2c32></vfp2c32>'
		ENDIF
		
		SELECT vfpintellicursor
		GATHER NAME loData MEMO
	ENDSCAN

	WAIT WINDOW 'Intellisense table was updated successful!' TIMEOUT 2
	
CATCH TO loExc

	LOCAL lcMessage
	m.lcMessage = 'An error occured while updating the Intellisense table' + CHR(13) + ;
		'Error-No: ' + TRANSFORM(m.loExc.ErrorNo) + CHR(13) + ;
		'Message: ' + m.loExc.Message + CHR(13) + ;
		'LineNo: ' + TRANSFORM(m.loExc.LineNo)
	MESSAGEBOX(m.lcMessage, 48, 'VFP2C32')
	
FINALLY
	USE IN SELECT('vfpintellicursor')
	IF !m.lbUsed
		USE IN SELECT('vfp2cintelli')
	ENDIF
ENDTRY