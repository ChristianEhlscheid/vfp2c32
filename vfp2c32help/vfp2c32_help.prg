LPARAMETERS lcInstall

LOCAL loHelp
m.loHelp = CREATEOBJECT('VFP2C32_HtmlHelp', JUSTPATH(SYS(16)))
IF VARTYPE(m.lcInstall) = 'C' AND ALLTRIM(LOWER(m.lcInstall)) == 'install'
	m.loHelp.Install()
ELSE
	m.loHelp.StartHelp()
ENDIF

DEFINE CLASS VFP2C32_HtmlHelp AS Session
	
	DataSession = 2
	cPath = ''
		
	FUNCTION Init
		LPARAMETERS lcPath
		LOCAL lcfxtoollib
		THIS.cPath =  ADDBS(m.lcPath)
		&& ensure FOXTOOLS.FLL is loaded
		IF !('FOXTOOLS.FLL' $ SET('Library'))
			lcfxtoollib = HOME() + "FOXTOOLS.FLL"
			SET LIBRARY TO (m.lcfxtoollib) ADDITIVE
		ENDIF
	ENDFUNC
	
	FUNCTION StartHelp
		LOCAL lcWord, lnTopicId
		m.lcWord = THIS.GetSelectedWord()
		m.lnTopicId = THIS.GetTopicId(m.lcWord)
		IF m.lnTopicId != 0
			THIS.OpenVFP2C32Help(m.lnTopicId)
		ELSE
			THIS.OpenVFPHelp()
		ENDIF
	ENDFUNC

	FUNCTION GetSelectedWord
		LOCAL lnWinHdl, laEnv[25], lnStartPos, lnEndPos
		m.lnWinHdl = _WONTOP()
		_EDGETENV(m.lnWinHdl, @laEnv)
		m.lnStartPos = m.laEnv[17]
		m.lnEndPos = m.laEnv[18]
		IF m.lnStartPos != m.lnEndPos
			RETURN _EDGETSTR(m.lnWinHdl, m.lnStartPos, m.lnEndPos - 1)
		ELSE
			RETURN ''
		ENDIF
	ENDFUNC

	FUNCTION GetTopicId
		LPARAMETERS lcFunction
		LOCAL lnTopicId
		m.lnTopicId = 0
		IF !EMPTY(m.lcFunction)
			USE (_FOXCODE) ALIAS vfp2c_help SHARED
			LOCATE FOR ALLTRIM(UPPER(vfp2c_help.expanded)) == ALLTRIM(UPPER(LEFT(m.lcFunction,26))) AND ALLTRIM(vfp2c_help.cmd) == '{vfp2c32menu}' 
			IF FOUND()
				m.lnTopicId = INT(VAL(STREXTRACT(vfp2c_help.user, '<topicid>', '</topicid>')))
			ENDIF
			USE IN SELECT('vfp2c_help')
		ENDIF
		RETURN m.lnTopicId
	ENDFUNC

	FUNCTION OpenVFP2C32Help
		LPARAMETERS m.lnTopicId
		LOCAL lcState, lcFile, lcSystem
		&& save current help settings
		m.lcState = SET("Help")
		m.lcFile = SET("Help", 1)
		m.lcSystem = SET("Help", 3)

		SET HELP ON
		SET HELP TO (THIS.cPath + 'vfp2c32.chm')
		HELP ID m.lnTopicId

		&& restore help settings
		IF !EMPTY(m.lcFile)
			SET HELP TO (m.lcFile)
		ELSE
			SET HELP TO
		ENDIF
		IF !EMPTY(m.lcSystem)
			SET HELP SYSTEM
		ENDIF	
		IF m.lcState == 'OFF'
			SET HELP OFF
		ENDIF
	ENDFUNC

	FUNCTION OpenVFPHelp
		ON KEY LABEL F1
		KEYBOARD '{F1}' PLAIN CLEAR
		THIS.Install()
	ENDFUNC

	FUNCTION Install
		LOCAL lcPath
		m.lcPath = ["] + THIS.cPath + 'vfp2c32_help.prg' + ["]
		ON KEY LABEL F1 DO &lcPath
	ENDFUNC

ENDDEFINE