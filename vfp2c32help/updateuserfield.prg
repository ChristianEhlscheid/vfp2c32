#INCLUDE "commonheader.h"

CD (JUSTPATH(FULLPATH(SYS(16))))

IF USED('vfp2cintelli')
	USE IN SELECT('vfp2cintelli')
ENDIF

TRY
	USE vfp2cintelli IN 0 ALIAS vi EXCLUSIVE
CATCH
	USE vfp2cintelli IN 0 ALIAS vi SHARED
ENDTRY	

WAIT WINDOW 'Updating "USER" field' NOWAIT NOCLEAR

LOCAL lcUser, laFields[4]
laFields[1] = 'expanded'
laFields[2] = 'returnval'
laFields[3] = 'descrip'
laFields[4] = 'remarks'

SELECT vi
SCAN
	m.lcUser = '<vfp2c32>' + CHR(13)
	
	DO CASE
		&& function
		CASE type = 'F' AND ALLTRIM(cmd) == '{vfp2c32menu}'

			FOR xj = 1 TO ALEN(m.laFields)
				IF !EMPTY(EVALUATE('vi.' + m.laFields[xj]))
					m.lcUser = m.lcUser + '<' + m.laFields[xj] + '>' + StripTags(EVALUATE('vi.' + m.laFields[xj])) + '</' +  m.laFields[xj] + '>' + CHR(13)
				ENDIF
			ENDFOR
			
			IF vi.topicid != 0
				m.lcUser = m.lcUser + '<topicid>' + ALLTRIM(STR(vi.topicid)) + '</topicid>' + CHR(13)
			ENDIF

		&& function tip
		CASE type = 'F' AND EMPTY(abbrev)
		
			IF ('<table class="constlist">' $ vi.tip) OR ('<table class="stringlist">' $ vi.tip)
				m.lcUser = m.lcUser + '<tip>' + HtmlTableToXml(vi.tip) + '</tip>'
			ELSE
				m.lcUser = m.lcUser + '<tip>' + StripTags(vi.tip) + '</tip>'			
			ENDIF
		
	ENDCASE
	
	m.lcUser = m.lcUser + '</vfp2c32>'
	REPLACE user WITH m.lcUser IN vi
ENDSCAN

IF ISEXCLUSIVE('vi')
	SELECT vi
	PACK MEMO
ENDIF

USE IN SELECT('vi')

WAIT WINDOW 'FINISHED!'

FUNCTION StripTags
	LPARAMETERS lcTip
	LOCAL laLines[1], xj, lnCount, lcRet, xo, lcTmp
	m.lcRet = ''

	m.lcTip = STRTRAN(m.lcTip, '<b>', '')
	m.lcTip = STRTRAN(m.lcTip, '</b>', '')
	m.lcTip = STRTRAN(m.lcTip, '<i>', '')
	m.lcTip = STRTRAN(m.lcTip, '</i>', '')
	m.lnCount = ALINES(m.laLines, m.lcTip, 1)
	
	FOR m.xj = 1 TO m.lnCount

		DO CASE
			CASE m.laLines[m.xj] = '<code>'
				
			CASE m.laLines[m.xj] = '<tip>'
				m.lcRet = m.lcRet + IIF(EMPTY(m.lcRet), '', CHR(13)) + 'Tip: '
							
			CASE m.laLines[m.xj] = '<note>'
				m.lcRet = m.lcRet + IIF(EMPTY(m.lcRet), '', CHR(13)) + 'Note: '
			
			CASE m.laLines[m.xj] = '<table>'
				m.xj = m.xj + 1 
				m.lcTmp = ''
				DO WHILE m.xj <= m.lnCount AND m.laLines[m.xj] != '</table>'
					m.lcTmp = m.lcTmp + m.laLines[m.xj] + CRLF
					m.xj = m.xj + 1 
				ENDDO
				m.lcRet = m.lcRet + IIF(EMPTY(m.lcRet), '', CHR(13)) + HtmlTableToText(m.lcTmp)
				
			CASE m.laLines[m.xj] = '</code>'
				m.lcRet = m.lcRet + CHR(13)
				
			CASE m.laLines[m.xj] = '</tip>'
				m.lcRet = m.lcRet + CHR(13)
				
			CASE m.laLines[m.xj] = '</note>'
				m.lcRet = m.lcRet + CHR(13)
			
			OTHERWISE
				m.lcRet = m.lcRet + IIF(EMPTY(m.lcRet), '', CHR(13)) + m.laLines[m.xj]
			
		ENDCASE
		
	ENDFOR
	
	RETURN m.lcRet
ENDFUNC

FUNCTION HtmlTableToText
	LPARAMETERS lcTable

	LOCAL lnRows, lnCount, laWords[1], laRows[1], xj, xo, lcRet, lcWord, lcLine, lbInTag
	m.lcRet = ''
	
	m.lnRows = ALINES(m.laRows, m.lcTable, 1+4, '<tr>', '</tr>')
	FOR m.xj = 1 TO m.lnRows	

		IF EMPTY(m.laRows[m.xj])
			LOOP
		ENDIF

		m.lcLine = ''	
		m.lnCount = ALINES(m.laWords, m.laRows[m.xj], 16, '<th', '<td', '</', '>')
		FOR m.xo = 1 TO m.lnCount

			m.lcWord = m.laWords[m.xo]
			DO CASE
				CASE m.lcWord == '<th' OR m.lcWord == '<td' OR m.lcWord == '</'
					m.lbInTag = .T.
			
				CASE m.lcWord == '<tr'
					m.lbInTag = .T.
					
				CASE RIGHT(m.lcWord, 1) == '>'
					m.lbInTag = .F.
					m.lcWord = ''
						
				CASE RIGHT(m.lcWord, 2) == '</'
					m.lcWord = SUBSTR(m.lcWord, 1, LEN(m.lcWord) - 2)
					
				OTHERWISE
					m.lcWord = ''
			ENDCASE

			IF !m.lbInTag AND !EMPTY(m.lcWord)
				m.lcLine = m.lcLine + IIF(EMPTY(m.lcLine), '', ' - ') + m.lcWord
			ENDIF
					
		ENDFOR
	
		m.lcRet = m.lcRet + IIF(EMPTY(m.lcRet), '', CHR(13)) + m.lcLine
	ENDFOR
	
	RETURN m.lcRet
ENDFUNC

FUNCTION HtmlTableToXml
	LPARAMETERS lcTip
	LOCAL lcTable, lnRows, laRows[1], xj, lcRet, lcValue, lcDesc, lcScript, lcFirstLine

	IF '<table class="constlist">' $ m.lcTip
		m.lcTable = STREXTRACT(m.lcTip, '<table class="constlist">', '</table>', 1, 1)	
		m.lcScript = 'vfp2c32constmenuitem'
		m.lcDesc = SUBSTR(m.lcTip, 1, AT('<table class="constlist">', m.lcTip) - 1)
	ELSE
		m.lcTable = STREXTRACT(m.lcTip, '<table class="stringlist">', '</table>', 1, 1)	
		m.lcScript = 'vfp2c32stringmenuitem'
		m.lcDesc = SUBSTR(m.lcTip, 1, AT('<table class="stringlist">', m.lcTip) - 1)
	ENDIF
	
	m.lcDesc = RTRIM(m.lcDesc, 0, CHR(10), CHR(13), CHR(9), CHR(32))
	m.lcFirstLine = GETWORDNUM(m.lcDesc, 1, CHR(13))
	m.lcDesc = SUBSTR(m.lcDesc, LEN(m.lcFirstLine)+1)
	m.lcDesc = LTRIM(m.lcDesc, 0, CHR(10), CHR(13), CHR(9), CHR(32))
	
	m.lcRet = '<script>' + m.lcScript + '</script>'
	m.lcRet = m.lcRet + '<desc>' + m.lcDesc + '</desc>'
	m.lcRet = m.lcRet + '<itemlist>'
	m.lnRows = ALINES(m.laRows, m.lcTable, 1+4, '<tr>', '</tr>')
	FOR m.xj = 1 TO m.lnRows	

		IF EMPTY(m.laRows[m.xj])
			LOOP
		ENDIF

		m.lcValue = GETWORDNUM(STREXTRACT(m.laRows[m.xj], '<td>', '</td>', 1, 1), 1, ' ')
		m.lcDesc = STREXTRACT(m.laRows[m.xj], '<td>', '</td>', 2, 1)

		IF !EMPTY(m.lcValue)
			m.lcRet = m.lcRet + '<e><v>' + ALLTRIM(m.lcValue) + '</v><d>' + ALLTRIM(m.lcDesc) + '</d></e>'
		ENDIF

	ENDFOR	
	m.lcRet = m.lcRet + '</itemlist>'
	RETURN m.lcRet
ENDFUNC