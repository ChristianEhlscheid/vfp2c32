LOCAL lcPath
m.lcPath = ADDBS(JUSTPATH(FULLPATH(SYS(16))))
CD (m.lcPath)

SET CLASSLIB TO (m.lcPath + 'libcurl\libcurl.vcx') ADDITIVE

IF !USED('ap')
	USE apilinks ALIAS ap IN 0 AGAIN SHARED
ENDIF

LOCAL laTmp[1], laFuncs[1], xj, xo, lnCount
SELECT apifuncs FROM vfp2cintelli WHERE !EMPTY(apifuncs) INTO ARRAY laTmp

FOR m.xj = 1 TO ALEN(m.laTmp)

	m.lnCount = ALINES(m.laFuncs, m.laTmp[m.xj], 1+4)
	FOR m.xo = 1 TO m.lnCount
		m.laFuncs[m.xo] = ALLTRIM(GETWORDNUM(m.laFuncs[m.xo], 1))

		SELECT ap
		LOCATE FOR ALLTRIM(ap.apifunc) == m.laFuncs[m.xo]
		IF !FOUND()
			INSERT INTO ap VALUES (m.laFuncs[m.xo], '', 1, .T.)
		ENDIF
	ENDFOR

ENDFOR

LOCAL loCurl, lcUrl, lcXml, lcLink, lcEntry, lnCount, xj, lcSearch
loCurl = NEWOBJECT('libcurl')

lcAppId = 'QbjbLg7V34EMPEqKtMEcoeHrNcHAtuHbKsDYyf8UqFmOk6ZPJzUJzSa6vzwFJOYeWTE-'

SELECT ap
SCAN FOR EMPTY(ap.apilink)
	WAIT WINDOW 'Updating API Link ' + ALLTRIM(STR(RECNO())) NOWAIT NOCLEAR

	m.lcSearch = ALLTRIM(ap.apifunc) + ICASE(ap.type = 1, ' Function', ap.type = 2, ' Structure', ap.type = 3, ' Enumeration', ap.type = 4, ' Interface', '')
	m.lcUrl = 'http://boss.yahooapis.com/ysearch/web/v1/' + m.loCurl.CurlEasyEscape(["] + m.lcSearch + ["]) + '?appid=' + m.lcAppId + '&format=xml&sites=msdn.microsoft.com&style=raw'
	m.lcXml = m.loCurl.HttpDownloadString(m.lcUrl)
	
	&& xml parsing for dummies, why throw an XML parser at it if it's that easy ....
	m.lnCount = OCCURS( '<result>', lcXml)
	FOR m.xj = 1 TO m.lnCount
	
		m.lcEntry = STREXTRACT(m.lcXml, '<result>', '</result>', m.xj)
		m.lcLink = STREXTRACT(m.lcEntry, '<url>', '</url>')
		IF LEFT(m.lcLink, LEN('http://msdn.microsoft.com/en-us/library/')) == 'http://msdn.microsoft.com/en-us/library/'
			REPLACE ap.apilink WITH m.lcLink IN ap
			EXIT
		ENDIF
		
	ENDFOR

ENDSCAN

loCurl = NULL

USE IN SELECT('ap')
USE IN SELECT('vfp2cintelli')
RELEASE CLASSLIB libcurl

WAIT WINDOW 'Updating API Links finished'