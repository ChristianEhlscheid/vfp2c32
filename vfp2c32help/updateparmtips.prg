LPARAMETERS lcFunction

#INCLUDE "commonheader.h"

IF VARTYPE(m.lcFunction) != 'C'
	CD (JUSTPATH(FULLPATH(SYS(16))))
ENDIF

m.lcFunction = IIF(VARTYPE(m.lcFunction) = 'C', ALLTRIM(m.lcFunction), '')

LOCAL lbUsed
m.lbUsed = USED('vfp2cintelli')
IF !m.lbUsed
	USE 'vfp2cintelli.dbf' IN 0 AGAIN SHARED
ENDIF

LOCAL laParms[1], lcParms, lnPCount, xj, laTips[1], lnTCount, xo
LOCAL lcTipName, lcTipType, lcData, lcOldTip, lnRecNo, lcNewData
LOCAL loRow

SELECT vfp2cintelli
SCAN FOR UPPER(type) = 'F' AND ALLTRIM(cmd) == '{vfp2c32menu}' AND (EMPTY(m.lcFunction) OR ALLTRIM(expanded) == m.lcFunction)

	m.lcParms = CHRTRAN(vfp2cintelli.tip,'@[] ','')
	m.lnPCount = ALINES(m.laParms, m.lcParms, 1+4, ',')
	m.lnTCount = ALINES(m.laTips, vfp2cintelli.data, 1+4)
	m.lnRecNo = RECNO('vfp2cintelli')
	
	m.lcData = ''
	FOR m.xj = 1 TO m.lnPCount

		m.lcTipName = SUBSTR(ALLTRIM(vfp2cintelli.expanded), 1, 22) + '_T' + PADL(ALLTRIM(STR(m.xj)), 2, '0')
		
		LOCATE FOR ALLTRIM(expanded) == m.lcTipName
		IF FOUND()
			m.lcTipType = IIF('<tip><script>' $ vfp2cintelli.user, 'F', 'T')
		ELSE
			m.lcTipType = 'T'
		ENDIF
		
		m.lcData = m.lcData + IIF(EMPTY(m.lcData), '', CHR(13)) + ALLTRIM(STR(m.xj)) + ',' + m.lcTipName + ',' + m.lcTipType
		
		m.lcOldTip = ''
		FOR m.xo = 1 TO m.lnTCount
			IF VAL(GETWORDNUM(m.laTips[m.xo], 1, ',')) = m.xj
				m.lcOldTip = ALLTRIM(GETWORDNUM(m.laTips[m.xo], 2, ','))
				EXIT
			ENDIF
		ENDFOR

		DO CASE
			CASE EMPTY(m.lcOldTip)
				APPEND BLANK IN vfp2cintelli
				REPLACE type WITH 'F', expanded WITH m.lcTipName, case WITH 'U', save WITH .F., ;
					uniqueid WITH SYS(2015), timestamp WITH VFP2C_INTELLI_TIMESTAMP  IN vfp2cintelli
				
			CASE m.lcOldTip == m.lcTipName
				LOCATE FOR ALLTRIM(expanded) == m.lcTipName
				IF !FOUND()
					APPEND BLANK IN vfp2cintelli
					REPLACE type WITH 'F', expanded WITH m.lcTipName, case WITH 'U', save WITH .F., ;
						uniqueid WITH SYS(2015), timestamp WITH VFP2C_INTELLI_TIMESTAMP IN vfp2cintelli
				ENDIF
			
			CASE UPPER(m.lcOldTip) == UPPER(m.lcTipName)
				UPDATE vfp2cintelli SET expanded = m.lcTipName WHERE ALLTRIM(expanded) == m.lcOldTip
			
			OTHERWISE

					SELECT COUNT(*) as nCount FROM vfp2cintelli WHERE TipExists(m.lcOldTip, vfp2cintelli.data) INTO CURSOR vitipcount
					&& if this tip is only used by one function, just update the tipname
					IF vitipcount.nCount = 1
						UPDATE vfp2cintelli SET expanded = m.lcTipName WHERE ALLTRIM(expanded) == m.lcOldTip				
					ELSE
						&& create a copy of the old tip with the new name
						SELECT vfp2cintelli
						LOCATE FOR ALLTRIM(expanded) == m.lcOldTip
						IF FOUND()
							SCATTER MEMO NAME loRow
							APPEND BLANK IN vfp2cintelli
							GATHER NAME loRow MEMO
							REPLACE uniqueid WITH SYS(2015), expanded WITH m.lcTipName IN vfp2cintelli
						ELSE
							APPEND BLANK IN vfp2cintelli
							REPLACE type WITH 'F', expanded WITH m.lcTipName, case WITH 'U', save WITH .F., ;
								uniqueid WITH SYS(2015), timestamp WITH VFP2C_INTELLI_TIMESTAMP IN vfp2cintelli
						ENDIF

					ENDIF
								
		ENDCASE
	
		GO (m.lnRecNo) IN vfp2cintelli
	
	ENDFOR
	
	REPLACE data WITH m.lcData IN vfp2cintelli
		
ENDSCAN

IF !m.lbUsed
	USE IN SELECT('vfp2cintelli')
ENDIF
USE IN SELECT('vitipcount')

FUNCTION TipExists
	LPARAMETERS lcTip, lcData
	LOCAL laTips[1], lnCount, xj
	m.lnCount = ALINES(m.laTips, m.lcData, 1+4)
	FOR m.xj = 1 TO m.lnCount
		IF ALLTRIM(GETWORDNUM(m.laTips[m.xj], 2, ',')) == m.lcTip
			RETURN .T.
		ENDIF
	ENDFOR
	RETURN .F.
ENDFUNC