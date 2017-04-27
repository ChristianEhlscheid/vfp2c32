CD (JUSTPATH(FULLPATH(SYS(16))))

IF !FILE('vfp2cintelli_merge.dbf')
	SELECT vfp2cintelli  FROM vi WHERE 1=2 INTO TABLE vfp2cintelli_merge.dbf
	USE IN SELECT('vfp2cintelli')
ELSE
	IF !USED('vfp2cintelli_merge') OR !ISEXCLUSIVE('vfp2cintelli_merge')
		USE IN SELECT('vfp2cintelli_merge')
		USE vfp2cintelli_merge IN 0 EXCLUSIVE
	ENDIF
	ZAP IN vfp2cintelli_merge
ENDIF

XMLTOCURSOR(GETFILE(), 'vfp2cintelli_merge', 4+512+8192)

LOCAL lnCount, xj, lcField, laFields[1]
lnCount = AFIELDS(m.laFields, 'vfp2cintelli_merge')
FOR m.xj = 1 TO m.lnCount
	IF m.laFields[m.xj, 2] = 'M'
		m.lcField = m.laFields[m.xj, 1]
		UPDATE vfp2cintelli_merge SET &lcField = STRTRAN(&lcField, CHR(13)+CHR(10), CHR(10))
		UPDATE vfp2cintelli_merge SET &lcField = STRTRAN(&lcField, CHR(10), CHR(13) + CHR(10))
	ENDIF
ENDFOR

SELECT vfp2cintelli_merge
PACK MEMO
USE IN SELECT('vfp2cintelli_merge')