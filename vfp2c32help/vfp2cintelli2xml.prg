LOCAL lcPath, lbUsed
m.lcPath = ADDBS(JUSTPATH(FULLPATH(SYS(16))))
CD (m.lcPath)

m.lbUsed = USED('vfp2cintelli')
IF !m.lbUsed
	USE vfp2cintelli IN 0 SHARED
ENDIF

LOCAL lcXml
CURSORTOXML('vfp2cintelli', m.lcPath + 'vfp2cintelli.xml', 1, 0+8+16+512, 0, "1")

IF !m.lbUsed
	USE IN SELECT('vfp2cintelli')
ENDIF