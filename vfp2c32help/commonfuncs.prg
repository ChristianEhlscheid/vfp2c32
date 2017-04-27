#INCLUDE "commonheader.h"

FUNCTION TextToHtml
	LPARAMETERS lcText, lcTag, lcClass
	LOCAL laLines[1], lnCount, xj, lcRet, lcCloseTag, lnOpenTag, lcDivId, lcTmp, lcLine, lnCodeId
	m.lcRet = ''
	m.lcTag = IIF(VARTYPE(m.lcTag) = 'C', ALLTRIM(m.lcTag), '')
	m.lcClass = IIF(VARTYPE(m.lcClass) = 'C', [ class="] + ALLTRIM(m.lcClass) + ["], '')
	m.lcCloseTag = IIF(EMPTY(m.lcTag), '', '</' + m.lcTag + '>')
	m.lcTag = IIF(EMPTY(m.lcTag), '', '<' + m.lcTag + m.lcClass + '>')
	m.lnOpenTag = 0
	m.lnCodeId = 0
	&& remove whitespace from beginning and end
	m.lcText = ALLTRIM(m.lcText, 0, CHR(13), CHR(10), CHR(32), CHR(9))
	m.lnCount = ALINES(m.laLines, m.lcText, 0)

	FOR m.xj = 1 TO m.lnCount
		
		m.lcLine = ALLTRIM(m.laLines[m.xj])
		
		IF m.lcLine == '<code>' OR m.lcLine == '<tip>' OR m.lcLine == '<note>' OR LEFT(m.lcLine,6)  ==  '<table'

			IF m.lnOpenTag > 0
				IF !EMPTY(m.lcTag)
					m.lcRet = m.lcRet + m.lcCloseTag + CRLF
				ENDIF
				m.lnOpenTag = 0
			ENDIF

			DO CASE
				
				CASE m.lcLine == '<code>'
					
					m.lnCodeId = m.lnCodeId + 1 
					m.lcDivId = 'code_' + ALLTRIM(STR(m.lnCodeId))
					m.lcRet = m.lcRet + [<div class="codeheader"><a onclick="CopyCode('] + m.lcDivId + [')" onmouseover="ChangeClass(this, 'onhover')" onmouseout="ChangeClass(this, '')">Copy code</a></div>] + CRLF + ;
										[<div class="block"><pre id="] + m.lcDivId + [">]
					m.xj = m.xj + 1 
					DO WHILE m.xj <= m.lnCount AND !(ALLTRIM(m.laLines[m.xj]) == '</code>')
						m.lcRet = m.lcRet + HtmlChars(m.laLines[m.xj]) + CRLF
						m.xj = m.xj + 1
					ENDDO	
					m.lcRet = m.lcRet + '</pre></div>' + CRLF
					
				CASE m.lcLine == '<tip>'

					m.lcRet = m.lcRet +[<div class="blockheader"><p class="note">Tip</p></div>] + CRLF + '<div class="block"><p>'
					m.xj = m.xj + 1 
					m.lcTmp = ''
					DO WHILE m.xj <= m.lnCount AND !(ALLTRIM(m.laLines[m.xj]) == '</tip>')
						m.lcTmp = m.lcTmp + IIF(EMPTY(m.lcTmp), '', '<br />' + CRLF) + HtmlChars(m.laLines[m.xj])
						m.xj = m.xj + 1
					ENDDO	
					m.lcRet = m.lcRet + m.lcTmp + CRLF + '</p></div>' + CRLF
				
				CASE m.lcLine == '<note>'

					m.lcRet = m.lcRet + [<div class="blockheader"><p class="note">Note</p></div>] + CRLF + '<div class="block"><p>'
					m.xj = m.xj + 1 
					m.lcTmp = ''
					DO WHILE m.xj <= m.lnCount AND !(ALLTRIM(m.laLines[m.xj]) == '</note>')
						m.lcTmp = m.lcTmp + IIF(EMPTY(m.lcTmp), '', '<br />' + CRLF) + HtmlChars(m.laLines[m.xj])
						m.xj = m.xj + 1
					ENDDO	
					m.lcRet = m.lcRet + m.lcTmp + CRLF + '</p></div>' + CRLF
						
				CASE LEFT(m.lcLine,6) == '<table'

					m.lcTmp = m.lcLine + CRLF
					m.xj = m.xj + 1 
					DO WHILE m.xj <= m.lnCount AND !(ALLTRIM(m.laLines[m.xj]) == '</table>')
						m.lcTmp = m.lcTmp + m.laLines[m.xj] + CRLF
						m.xj = m.xj + 1
					ENDDO	
					m.lcTmp = m.lcTmp + '</table>'
					
					m.lcTmp = HtmlCharsInTable(m.lcTmp)
					
					m.lcRet = m.lcRet + m.lcTmp
					
			ENDCASE
		
		ELSE

			IF m.lnOpenTag = 0
				IF !EMPTY(m.lcTag)
					m.lcRet = m.lcRet + m.lcTag
				ENDIF
				m.lnOpenTag = 1
			ENDIF		

			m.lcRet = m.lcRet + IIF(m.lnOpenTag > 1, '<br />' + CRLF, '') + HtmlChars(m.laLines[m.xj])
			m.lnOpenTag = m.lnOpenTag + 1
		
		ENDIF
		
	ENDFOR

	IF m.lnOpenTag > 0 AND !EMPTY(m.lcTag)
		m.lcRet = m.lcRet + m.lcCloseTag + CRLF
	ENDIF

	m.lcRet = STRTRAN(m.lcRet, '&lt;b&gt;', '<strong>')
	m.lcRet = STRTRAN(m.lcRet, '&lt;/b&gt;', '</strong>')
	m.lcRet = STRTRAN(m.lcRet, '&lt;i&gt;', '<em>')
	m.lcRet = STRTRAN(m.lcRet, '&lt;/i&gt;', '</em>')
	m.lcRet = STRTRAN(m.lcRet, '&amp;nbsp;', '&nbsp;')
	
	RETURN m.lcRet
ENDFUNC	

FUNCTION HtmlChars
	LPARAMETERS lcText
	m.lcText = STRTRAN(m.lcText, '&', '&amp;')
	m.lcText = STRTRAN(m.lcText, '€', '&euro;')
	m.lcText = STRTRAN(m.lcText, '<', '&lt;')
	m.lcText = STRTRAN(m.lcText, '>', '&gt;')
	&& there are definitly some escapes sequences missing .. but these suffice for the moment
	RETURN m.lcText
ENDFUNC

FUNCTION HtmlCharsInTable
	LPARAMETERS lcTable

	LOCAL lnCount, laWords[1], xj, lcRet, lcWord
	m.lcRet = ''
	m.lnCount = ALINES(m.laWords, m.lcTable, 16, '<table', '<tr', '<th', '<td', '</', '>')
	FOR m.xj = 1 TO m.lnCount
	
		m.lcWord = m.laWords[m.xj]
		DO CASE
			CASE m.lcWord == '<table' OR m.lcWord == '<tr' OR m.lcWord == '<th' OR ;
					m.lcWord == '<td' OR m.lcWord == '</' OR m.lcWord == '>'
				m.lcRet = m.lcRet + m.lcWord
					
			CASE m.lcWord == CRLF + '</'
				m.lcRet = m.lcRet + m.lcWord
			
			CASE RIGHT(m.lcWord, 2) == '</'
				m.lcWord = HtmlChars(SUBSTR(m.lcWord, 1, LEN(m.lcWord) - 2))
				m.lcWord = STRTRAN(m.lcWord, CRLF, '<br />' + CRLF)
				m.lcRet = m.lcRet + m.lcWord + '</'
				
				
			OTHERWISE
				m.lcRet = m.lcRet + m.lcWord
				
		ENDCASE
		
	ENDFOR
	RETURN m.lcRet
	
ENDFUNC