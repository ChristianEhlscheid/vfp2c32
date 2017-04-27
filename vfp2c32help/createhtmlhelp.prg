LPARAMETERS lcFunction

#INCLUDE "commonheader.h"

LOCAL lcPath, loHtmlHelpCreator
m.lcPath = ADDBS(JUSTPATH(FULLPATH(SYS(16))))
CD (m.lcPath)
SET PROCEDURE TO commonfuncs.prg ADDITIVE

m.loHtmlHelpCreator = CREATEOBJECT('cHtmlHelpCreator', m.lcPath)
m.loHtmlHelpCreator.CreateCHM(m.lcFunction)

DEFINE CLASS cHtmlHelpCreator AS Session

	cPath = ''
	bPreview = .F.
	Datasession = 2
	cHtmlPath = ''
	
	
	FUNCTION Init
		LPARAMETERS lcPath
		THIS.cPath = ADDBS(m.lcPath)
		THIS.cHtmlPath = THIS.cPath + 'chm\pages\'
		
		IF !USED('vi')
			USE vfp2cintelli ALIAS vi IN 0 AGAIN SHARED
		ENDIF
		IF !USED('ap')
			USE apilinks ALIAS ap IN 0 AGAIN SHARED
		ENDIF
		IF !USED('se')
			USE settings ALIAS se IN 0 AGAIN SHARED
		ENDIF
		IF !USED('groups')
			USE groups ALIAS gr IN 0 AGAIN SHARED
		ENDIF
		
		CREATE CURSOR htmlhelp (funcname C(50), html M)
		
		SELECT vi.expanded AS funcname, CAST('<a href="/pages/' + LOWER(ALLTRIM(vi.expanded)) + '.html">' + ALLTRIM(expanded) + '</a>' AS C(254)) AS funclink FROM vi ;
			WHERE vi.type = 'F' AND ALLTRIM(vi.cmd) == '{vfp2c32menu}' INTO CURSOR internallinks
		
		SELECT ap.apifunc AS funcname, CAST('<a href="' + ALLTRIM(apilink) + '">' + ALLTRIM(apifunc) + '</a>' AS C(254)) AS funclink FROM ap WHERE substitute INTO CURSOR externallinks	

	ENDFUNC
	
	FUNCTION Destroy
		&& close all temporary cursors
		USE IN SELECT('htmlhelp')
		USE IN SELECT('vi')
		USE IN SELECT('ap')
		USE IN SELECT('se')
		USE IN SELECT('gr')
		USE IN SELECT('internallinks')
		USE IN SELECT('externallinks')
	ENDFUNC

	FUNCTION CreateCHM
		LPARAMETERS lcFunction
		THIS.bPreview = VARTYPE(m.lcFunction)  = 'C'

		IF !THIS.bPreview
			WAIT WINDOW 'Creating HTML help ...' NOWAIT NOCLEAR
		ENDIF
		
		SELECT vi
		IF THIS.bPreview
			LOCATE FOR vi.type = 'F' AND ALLTRIM(vi.cmd) == '{vfp2c32menu}' AND ALLTRIM(expanded) == ALLTRIM(m.lcFunction)
			THIS.CreateFunctionPage()
		ELSE
			WAIT WINDOW 'Creating Function Pages' NOWAIT NOCLEAR
			SCAN FOR vi.type = 'F' AND ALLTRIM(vi.cmd) == '{vfp2c32menu}'
				THIS.CreateFunctionPage()
			ENDSCAN
		ENDIF

		LOCAL lcSafety
		m.lcSafety = SET("Safety")
		SET SAFETY OFF

		IF THIS.bPreview
		
			SELECT htmlhelp
			LOCATE
			STRTOFILE(htmlhelp.html, THIS.cPath + 'preview.html', 0)
			
		ELSE

			SELECT htmlhelp
			SCAN
				STRTOFILE(htmlhelp.html, THIS.cHtmlPath + FORCEEXT(LOWER(ALLTRIM(htmlhelp.funcname)), 'html'), 0)	
			ENDSCAN

			WAIT WINDOW 'Creating Reference' NOWAIT NOCLEAR
			THIS.CreateFunctionsAZPage()
			THIS.CreateFunctionsByCategoryPage()
			THIS.CreateCategoryPages()

			WAIT WINDOW 'Creating TOC' NOWAIT NOCLEAR
			THIS.CreateChmToc()
			THIS.CreateChmIndex()
			THIS.UpdateTopicIds()
			THIS.CreateChmMapHeader()
			THIS.CreateChmAliasHeader()
			
			THIS.CompileCHM('chm\vfp2c32.hhp')

			WAIT WINDOW 'FINISHED!'

			IF MESSAGEBOX('Open CHM?', 4+256, 'VFP2C32 Help') = 6
				ShellExecute(0, "", "hh.exe", ["] + THIS.cPath + [chm\vfp2c32.chm"], "", 5)
			ENDIF
		ENDIF

		SET SAFETY &lcSafety
	ENDFUNC
	
	FUNCTION CreateFunctionPage
		LOCAL lcHtml, lcGroupId, lcUniqueId, lcInitFlag
		
		m.lcInitFlag = '<div class="flags">'
		IF vi.threadsafe
			m.lcInitFlag = m.lcInitFlag + '<a class="threadsafe" title="Threadsafe"></a>'
		ELSE
			m.lcInitFlag = m.lcInitFlag + '<a class="notthreadsafe" title="Not available in threadsafe version"></a>'
		ENDIF
		m.lcInitFlag = m.lcInitFlag + '</div>'

		m.lcHtml = THIS.HtmlHeader(vi.expanded, 'function', m.lcInitFlag)
		
		lcHtml = lcHtml + TextToHtml(vi.descrip, 'p', 'description') + CRLF
		lcHtml = lcHtml + '<div class="block"><pre>' + ALLTRIM(vi.expanded) + '(' + THIS.CreateParameterLinks(vi.tip) + ')</pre></div>' + CRLF

		lcHtml = lcHtml + THIS.ConvertParameters()

		lcHtml = lcHtml + '<h2>Return Value</h2>' + CRLF
		lcHtml = lcHtml + TextToHtml(vi.returnval, 'p') + CRLF

		IF !EMPTY(vi.remarks)
			lcHtml = lcHtml + '<h2>Remarks</h2>' + CRLF
			lcHtml = lcHtml + TextToHtml(vi.remarks, 'p') + CRLF
		ENDIF

		IF !EMPTY(vi.example)
			lcHtml = lcHtml + '<h2>Example</h2>' + CRLF
			lcHtml = lcHtml + TextToHtml(vi.example, 'p')
		ENDIF

		m.lcHtml = THIS.CreateLinks(m.lcHtml, .T., vi.expanded)
		m.lcHtml = THIS.CreateLinks(m.lcHtml, .F., vi.expanded)

		IF !EMPTY(vi.groupid) OR !EMPTY(vi.apifuncs)
			lcHtml = lcHtml + '<h2>See Also</h2>' + CRLF

			IF !EMPTY(vi.groupid)
				lcHtml = lcHtml + '<h4>Reference</h4>' + CRLF
				lcHtml = lcHtml + '<p>'
				lcGroupId = vi.groupid
				lcUniqueId = vi.uniqueid
				SELECT expanded FROM vi WHERE vi.groupid == m.lcGroupId AND vi.uniqueid != lcUniqueId INTO CURSOR vigroup
				SELECT vigroup
				SCAN
					lcHtml = lcHtml + '<a href="/pages/' + ALLTRIM(LOWER(vigroup.expanded)) + '.html">' + ALLTRIM(vigroup.expanded) + '</a><br />' + CRLF
				ENDSCAN
				lcHtml = lcHtml + '</p>' + CRLF
			ENDIF
			
			IF !EMPTY(vi.apifuncs)
				LOCAL lcApiLinks
				m.lcApiLinks = THIS.CreateApiLinks(vi.apifuncs)
				IF !EMPTY(m.lcApiLinks)
					lcHtml = lcHtml + '<h4>Used WinApi functions</h4>' + CRLF
					lcHtml = lcHtml + '<p>' + m.lcApiLinks + '</p>' + CRLF
				ENDIF
				RELEASE lcApiLinks
			ENDIF
			
		ENDIF

		m.lcHtml = m.lcHtml + CRLF + THIS.HtmlFooter()

		INSERT INTO htmlhelp VALUES (ALLTRIM(vi.expanded), m.lcHtml)
	ENDFUNC

	FUNCTION HtmlHeader
		LPARAMETERS lcTitle, lcHeaderClass, lcInitFlag

		LOCAL lcHeader

		TEXT TO lcHeader TEXTMERGE NOSHOW PRETEXT 2
		<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.1//EN" "http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd">
		<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en">
		<head>
		<meta http-equiv="X-UA-Compatible" content="IE=edge"/>
		<meta http-equiv="Content-Type" content="text/html; charset=windows-1252" />
		<title><<ALLTRIM(m.lcTitle)>></title>
		ENDTEXT

		IF THIS.bPreview
			
		TEXT TO lcHeader ADDITIVE TEXTMERGE NOSHOW PRETEXT 2
		<style type="text/css">
		<!--
		<<STRTRAN(FILETOSTR(THIS.cPath + 'chm\scripts\styles.css'), '/images/', './chm/images/')>>
		-->
		</style>
		<script type="text/javascript">
		<<FILETOSTR(THIS.cPath + 'chm\scripts\jscript.js')>>
		</script>
		</head>		
		ENDTEXT

		ELSE

		TEXT TO lcHeader ADDITIVE TEXTMERGE NOSHOW PRETEXT 2
		<link href="/scripts/styles.css" rel="stylesheet" type="text/css" />
		<script src="/scripts/jscript.js" type="text/javascript"></script>
		</head>
		ENDTEXT

		ENDIF

		m.lcHeaderClass = IIF(VARTYPE(m.lcHeaderClass) = 'C' AND !EMPTY(m.lcHeaderClass), [ class="] + ALLTRIM(m.lcHeaderClass) + ["], '')
		m.lcInitFlag = IIF(VARTYPE(m.lcInitFlag) = 'C', ALLTRIM(m.lcInitFlag), '')
		
		TEXT TO lcHeader ADDITIVE TEXTMERGE NOSHOW PRETEXT 2
		<body>
		<div id="header">
		<p id="vfp2c32version"></p>
		<h1<<m.lcHeaderClass>>><<ALLTRIM(m.lcTitle)>></h1>
		<<m.lcInitFlag>>
		</div>
		<div id="content">
		ENDTEXT
		
		m.lcHeader = m.lcHeader + CRLF
		RETURN m.lcHeader
	ENDFUNC
	
	FUNCTION HtmlFooter
		LOCAL lcFooter
		
		TEXT TO lcFooter TEXTMERGE NOSHOW PRETEXT 2
		</div>
		<div id="footer">
		<p>&copy; <a href="https://github.com/ChristianEhlscheid/vfp2c32">VFP2C32</a> - Shared Source License for <a href="https://vfpx.github.io/">VFPX</a></p>
		</div>
		</body>
		</html>
		ENDTEXT

		m.lcFooter = CRLF + m.lcFooter
		RETURN m.lcFooter
	ENDFUNC

	FUNCTION CompileCHM
		LPARAMETERS lcFile
		LOCAL lcPath, lcExe
		m.lcExe = 'hhc.exe'
		m.lcPath = ADDBS(ALLTRIM(se.hhcpath))
		IF !FILE(m.lcPath + m.lcExe)
			m.lcPath = ADDBS(GETDIR('', '', 'Select path to HTML Workshop'))
			IF FILE(m.lcPath + m.lcExe)
				REPLACE hhcpath WITH m.lcPath IN se
			ELSE
				RETURN
			ENDIF
		ENDIF

		DECLARE INTEGER ShellExecute IN shell32.dll INTEGER hwnd, STRING lpOperation, STRING lpFile, STRING lpParameters, STRING lpDirectory, INTEGER nShowCmd
		ShellExecute(0, "", ["] + m.lcPath + m.lcExe + ["], ["] + m.lcFile + ["], "", 5)
	ENDFUNC

	FUNCTION CreateParameterLinks
		LPARAMETERS lcParameters
		LOCAL lcRet, lcCleanParms, laParms[1], lcReplace, lnCount, xj
		m.lcRet = ''
		m.lcParameters = ALLTRIM(m.lcParameters)
		m.lcCleanParms = CHRTRAN(m.lcParameters, "[]", '')
		m.lnCount = ALINES(m.laParms, m.lcCleanParms , 1, ',')
		FOR m.xj = 1 TO m.lnCount
			m.lcReplace = [<a href="#p] + ALLTRIM(STR(m.xj)) + [">] + m.laParms[m.xj] + [</a>]
			m.lcParameters = STRTRAN(m.lcParameters, m.laParms[m.xj], m.lcReplace, 1, 1, 0)
		ENDFOR
		RETURN m.lcParameters
	ENDFUNC

	FUNCTION CreateApiLinks
		LPARAMETERS lcApis
		LOCAL lcHtml, laFuncs[1], lnCount, xj, lcFuncName, lcParm
		m.lcHtml = ''
		m.lnCount = ALINES(m.laFuncs, m.lcApis, 1+4)
		FOR m.xj = 1 TO m.lnCount
			IF GETWORDCOUNT(m.laFuncs[m.xj]) = 1
				m.lcFuncName = m.laFuncs[m.xj]
				m.lcParm = ''
			ELSE
				m.lcFuncName = GETWORDNUM(m.laFuncs[m.xj], 1)
				m.lcParm = SUBSTR(m.laFuncs[m.xj], LEN(GETWORDNUM(m.laFuncs[m.xj],1))+1)
			ENDIF
			
			SELECT ap
			LOCATE FOR ALLTRIM(ap.apifunc) == m.lcFuncName
			IF FOUND()
				m.lcHtml = m.lcHtml + IIF(!EMPTY(m.lcHtml), '<br />' + CRLF, '') + '<a href="' + ALLTRIM(ap.apilink) + '">' + m.lcFuncName + '</a>' + m.lcParm 
			ENDIF
		ENDFOR
		RETURN m.lcHtml
	ENDFUNC

	FUNCTION ConvertParameters
		LOCAL lcHtml, laParms[1], lnParmCount, laParmTmp[1], laParmTips[1], lnParmTipCount, xj
		
		lcHtml = ''
		lnParmCount = ALINES(laParms, CHRTRAN(vi.tip, '[]', ''), 1+4, ',')
		lnParmTipCount = ALINES(m.laParmTmp, vi.data, 1+4)

		IF m.lnParmTipCount > 0
			DIMENSION laParmTips[m.lnParmTipCount,2]
			FOR m.xj = 1 TO m.lnParmTipCount
				laParmTips[m.xj,1] = VAL(GETWORDNUM(m.laParmTmp[m.xj], 1, ','))
				laParmTips[m.xj,2] = ALLTRIM(GETWORDNUM(m.laParmTmp[m.xj], 2, ','))
			ENDFOR
		ELSE
			DIMENSION laParmTips[1,2]
			laParmTips[1,1] = 0
			laParmTips[1,2] = ''
		ENDIF

		IF lnParmCount > 0
			lcHtml = lcHtml + '<h3>Parameters</h3>' + CRLF

			FOR m.xj = 1 TO m.lnParmCount
			
				lnTip = ASCAN(m.laParmTips, m.xj, 1, -1, 1, 8)
				IF lnTip > 0
					
					SELECT tip FROM vi WHERE ALLTRIM(vi.expanded) == m.laParmTips[m.lnTip,2] INTO CURSOR viparm

					IF _TALLY = 1
						lcHtml = lcHtml + [<dl class="parm"><dt><a id="p] + ALLTRIM(STR(m.xj)) + [">]  + ALLTRIM(MLINE(viparm.tip, 1)) + [</a></dt>] + CRLF
						lcHtml = lcHtml + [<dd>] + THIS.ConvertParameterTip(viparm.tip) + [</dd></dl>] + CRLF
					ENDIF

				ENDIF
				
			ENDFOR
			
			USE IN SELECT('viparm')
		ENDIF
		
		RETURN lcHtml
	ENDFUNC

	FUNCTION ConvertParameterTip
		LPARAMETERS lcText
		LOCAL laLines[1], lnCount, xj, lcRet
		m.lcRet = ''
		m.lnCount = ALINES(m.laLines, ALLTRIM(lcText), 0)
		FOR m.xj = 2 TO m.lnCount
			m.lcRet = m.lcRet + m.laLines[m.xj] + CRLF
		ENDFOR
		RETURN TextToHtml(m.lcRet, 'p')
	ENDFUNC

	FUNCTION CreateLinks
		LPARAMETERS lcHtml, lbInternal, lcExcludeFunc
		
		LOCAL xj, xo, laDelim[7]
		m.laDelim[1] = ' '
		m.laDelim[2] = ','
		m.laDelim[3] = '.'
		m.laDelim[4] = CRLF
		m.laDelim[5] = '('
		m.laDelim[6] = ')'
		m.laDelim[7] = '>'

		m.lcExcludeFunc = IIF(VARTYPE(m.lcExcludeFunc) = 'C', ALLTRIM(m.lcExcludeFunc), '')

		IF m.lbInternal
			SELECT internallinks
		ELSE
			SELECT externallinks
		ENDIF
		
		SCAN FOR !(ALLTRIM(funcname) == m.lcExcludeFunc) AND ALLTRIM(funcname) $ m.lcHtml

			FOR m.xj = 1 TO ALEN(m.laDelim)
				FOR m.xo = 1 TO ALEN(m.laDelim)
					m.lcHtml = STRTRAN(m.lcHtml, m.laDelim[m.xj] + ALLTRIM(funcname) + m.laDelim[m.xo], m.laDelim[m.xj] + ALLTRIM(funclink) + m.laDelim[m.xo], 1, -1, 0)
				ENDFOR	
			ENDFOR

		ENDSCAN
		
		RETURN m.lcHtml
	ENDFUNC

	FUNCTION CreateChmToc
		
		LOCAL lcSiteMap, lcEntry, lcSiteMapGroup, lcSiteMapAZ
		&& create sitemap A-Z
		m.lcSiteMapAZ = ''
		SELECT expanded, UPPER(expanded) AS corder FROM vi WHERE vi.type = 'F' AND ALLTRIM(vi.cmd) == '{vfp2c32menu}' ORDER BY corder INTO CURSOR vs
		SELECT vs
		SCAN

TEXT TO m.lcEntry TEXTMERGE NOSHOW
		<LI> <OBJECT type="text/sitemap">
			<param name="Name" value="<<ALLTRIM(vs.expanded)>>">
			<param name="Local" value="pages\<<ALLTRIM(LOWER(vs.expanded))>>.html">
			</OBJECT>
ENDTEXT

			m.lcSiteMapAZ = m.lcSiteMapAZ + CRLF + m.lcEntry

		ENDSCAN
		m.lcSiteMapAZ = m.lcSiteMapAZ + CRLF

		&& create sitemap by group
		m.lcSiteMapGroup = ''
		SELECT groupid, groupname FROM groups ORDER BY groupname INTO CURSOR vg
		SELECT vg
		SCAN
			
TEXT TO m.lcSiteMapGroup ADDITIVE TEXTMERGE NOSHOW
	<LI> <OBJECT type="text/sitemap">
		<param name="Name" value="<<ALLTRIM(vg.groupname)>>">
		<param name="Local" value="pages\group<<LOWER(vg.groupid)>>.html">
		</OBJECT>
	<UL>
ENDTEXT
			
			SELECT expanded, UPPER(expanded) AS corder FROM vi WHERE vi.type = 'F' AND ALLTRIM(vi.cmd) == '{vfp2c32menu}' AND vi.groupid == vg.groupid ORDER BY corder INTO CURSOR vs
			SELECT vs
			SCAN

TEXT TO m.lcEntry TEXTMERGE NOSHOW
		<LI> <OBJECT type="text/sitemap">
			<param name="Name" value="<<ALLTRIM(vs.expanded)>>">
			<param name="Local" value="pages\<<ALLTRIM(LOWER(vs.expanded))>>.html">
			</OBJECT>
ENDTEXT

			m.lcSiteMapGroup = m.lcSiteMapGroup + CRLF + m.lcEntry
			
			ENDSCAN

			m.lcSiteMapGroup = m.lcSiteMapGroup + CRLF + CHR(9) + CHR(9) + '</UL>' + CRLF

		ENDSCAN
		m.lcSiteMapGroup = m.lcSiteMapGroup + CRLF

		&& merge sitemap into hhc file
		LOCAL lcSitemap

TEXT TO m.lcSitemap TEXTMERGE NOSHOW
<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML//EN">
<HTML>
<HEAD>
<meta name="GENERATOR" content="Microsoft&reg; HTML Help Workshop 4.1">
<!-- Sitemap 1.0 -->
</HEAD><BODY>
<OBJECT type="text/site properties">
	<param name="Window Styles" value="0x800025">
</OBJECT>
<UL>
	<LI> <OBJECT type="text/sitemap">
		<param name="Name" value="VFP2C32">
		<param name="Local" value="pages\static\Mainpage.html">
		</OBJECT>
		<UL>
			<LI> <OBJECT type="text/sitemap">
				<param name="Name" value="Error handling">
				<param name="Local" value="pages\static\Errorhandling.html">
				</OBJECT>
			<LI> <OBJECT type="text/sitemap">
				<param name="Name" value="Distribution">
				<param name="Local" value="pages\static\Distribution.html">
				</OBJECT>
			<LI> <OBJECT type="text/sitemap">
				<param name="Name" value="Intellisense &amp; Context Help">
				<param name="Local" value="pages\static\Intellisense.html">
				</OBJECT>
			<LI> <OBJECT type="text/sitemap">
				<param name="Name" value="Version history">
				<param name="Local" value="pages\static\VersionHistory.html">
				</OBJECT>
			<LI> <OBJECT type="text/sitemap">
				<param name="Name" value="Developers and collaborators">
				<param name="Local" value="pages\static\Developers.html">
				</OBJECT>
		</UL>
	<LI> <OBJECT type="text/sitemap">
		<param name="Name" value="VFP2C32Front">
		<param name="Local" value="pages\static\VFP2C32Front.html">
		</OBJECT>
		<UL>
			<LI> <OBJECT type="text/sitemap">
				<param name="Name" value="Translation options">
				<param name="Local" value="pages\static\VFP2C32Front_Options.html">
				</OBJECT>
			<LI> <OBJECT type="text/sitemap">
				<param name="Name" value="Common errors">
				<param name="Local" value="pages\static\VFP2C32Front_Errors.html">
				</OBJECT>
			<LI> <OBJECT type="text/sitemap">
				<param name="Name" value="Examples">
				</OBJECT>
				<UL>
					<LI> <OBJECT type="text/sitemap">
						<param name="Name" value="GlobalMemoryStatus">
						<param name="Local" value="pages\static\VFP2C32Front_GlobalMemoryStatus.html">
						</OBJECT>
					<LI> <OBJECT type="text/sitemap">
						<param name="Name" value="RegisterPowerSettingNotification">
						<param name="Local" value="pages\static\VFP2C32Front_RegisterPowerSettingNotification.html">
						</OBJECT>
				</UL>
		</UL>
	<LI> <OBJECT type="text/sitemap">
		<param name="Name" value="Reference">
		</OBJECT>
		<UL>
			<LI> <OBJECT type="text/sitemap">
				<param name="Name" value="Functions A-Z">
				<param name="Local" value="pages\FunctionsA-Z.html">
				</OBJECT>
				<UL>
				<<m.lcSiteMapAZ>>
				</UL>
			<LI> <OBJECT type="text/sitemap">
				<param name="Name" value="Functions by Category">
				<param name="Local" value="pages\FunctionsCategory.html">
				</OBJECT>
				<UL>
				<<m.lcSiteMapGroup>>		
				</UL>
		</UL>
	</UL>
</UL>
</BODY></HTML>
ENDTEXT

		STRTOFILE(m.lcSitemap, THIS.cPath + 'chm\vfp2c32toc.hhc', 0)

		USE IN SELECT('vs')
		USE IN SELECT('vg')		
	ENDFUNC

	FUNCTION CreateChmIndex

		LOCAL lcIndex, lcEntry, lcBegin, lcEnd
		m.lcIndex = ''

		SELECT expanded, UPPER(expanded) AS corder FROM vi WHERE vi.type = 'F' AND ALLTRIM(vi.cmd) == '{vfp2c32menu}' ORDER BY corder INTO CURSOR vs
		SELECT vs
		SCAN

TEXT TO m.lcEntry TEXTMERGE NOSHOW
		<LI> <OBJECT type="text/sitemap">
			<param name="Name" value="<<ALLTRIM(vs.expanded)>>">
			<param name="Local" value="pages\<<ALLTRIM(LOWER(vs.expanded))>>.html">
			</OBJECT>
ENDTEXT

			m.lcIndex = m.lcIndex + CRLF + m.lcEntry

		ENDSCAN
		m.lcIndex = m.lcIndex + CRLF

		LOCAL lcCurFile, lcBegin, lcEnd, lcMap
		m.lcCurFile = FILETOSTR(THIS.cPath + 'chm\vfp2c32index.hhk')

TEXT TO m.lcBegin TEXTMERGE NOSHOW
</HEAD><BODY>
<UL>
ENDTEXT

TEXT TO m.lcEnd NOSHOW 
</UL>
</BODY></HTML>
ENDTEXT

		m.lcMap = STREXTRACT(m.lcCurFile, m.lcBegin, m.lcEnd)
		m.lcCurFile = STRTRAN(m.lcCurFile, m.lcMap, m.lcIndex)

		STRTOFILE(m.lcCurFile, THIS.cPath + 'chm\vfp2c32index.hhk', 0)
		USE IN SELECT('vs')
	ENDFUNC

	FUNCTION UpdateTopicIds()
		LOCAL lnTopicId 	
		m.lnTopicId = 1
		SELECT vi
		SET ORDER TO I1 
		SCAN FOR vi.type = 'F' AND ALLTRIM(vi.cmd) == '{vfp2c32menu}'
			REPLACE topicid WITH m.lnTopicId
			m.lnTopicId = m.lnTopicId + 1 
		ENDSCAN
		SET ORDER TO
	ENDFUNC

	FUNCTION CreateChmMapHeader
		LOCAL lcFile, lcEntry, lcId
		m.lcFile = ''
		SELECT expanded, topicid FROM vi WHERE vi.type = 'F' AND ALLTRIM(vi.cmd) == '{vfp2c32menu}' ORDER BY topicid INTO CURSOR vs
		SELECT vs
		SCAN
			m.lcId = ALLTRIM(STR(vs.topicid))
			m.lcFile = m.lcFile + '#define ' + m.lcId + ' ' + m.lcId + ' // ' + ALLTRIM(vs.expanded) + CRLF
		ENDSCAN
		STRTOFILE(m.lcFile, THIS.cPath + 'chm\vfp2c32map.h', 0)
		USE IN SELECT('vs')
	ENDFUNC

	FUNCTION CreateChmAliasHeader
		LOCAL lcFile, lcEntry, lcId
		m.lcFile = ''
		SELECT expanded, topicid FROM vi WHERE vi.type = 'F' AND ALLTRIM(vi.cmd) == '{vfp2c32menu}' ORDER BY topicid INTO CURSOR vs
		SELECT vs
		SCAN
			m.lcId = ALLTRIM(STR(vs.topicid))
			m.lcFile = m.lcFile + m.lcId + '=pages/' + ALLTRIM(vs.expanded) + '.html' + CRLF
		ENDSCAN
		STRTOFILE(m.lcFile, THIS.cPath + 'chm\vfp2c32alias.h', 0)
		USE IN SELECT('vs')
	ENDFUNC

	FUNCTION CreateFunctionsAZPage
	
		LOCAL lcHtml, lcFunction, lcHeader, lcDescription

		m.lcHeader = '<p class="links">'
		m.lcHtml = ''

		FOR m.xj = ASC('A') TO ASC('Z')
		
			SELECT expanded AS funcname, UPPER(expanded) AS cOrder, descrip FROM vi WHERE vi.type = 'F' AND ALLTRIM(vi.cmd) == '{vfp2c32menu}' AND UPPER(LEFT(vi.expanded,1)) = CHR(m.xj) ORDER BY cOrder INTO CURSOR vs
			IF _TALLY > 0

				m.lcHeader = m.lcHeader + [<a href="#C_] + CHR(m.xj) + [">] + CHR(m.xj) + [</a>]

				m.lcHtml = m.lcHtml + [<h2>] + CHR(m.xj) + [<a id="C_] + CHR(m.xj) + ["></a></h2>]
				m.lcHtml = m.lcHtml + CRLF + '<table class="list">'
				SELECT vs
				SCAN
					m.lcFunction = ALLTRIM(vs.funcname)
					m.lcDescription = HtmlChars(vs.descrip) 
					m.lcDescription = THIS.CreateLinks(m.lcDescription, .T., m.lcFunction)
					m.lcDescription = THIS.CreateLinks(m.lcDescription, .F., m.lcFunction)
					m.lcHtml = m.lcHtml + CRLF + [<tr class="] + IIF(MOD(RECNO('vs'), 2) = 0, 'even', 'uneven') + ["><td class="w20"><a href="] + LOWER(m.lcFunction) + [.html">] + m.lcFunction + [</a></td><td>] + m.lcDescription + '</td></tr>'
				ENDSCAN	
				m.lcHtml = m.lcHtml + CRLF + '</table>' + CRLF
				
			ENDIF
				
		ENDFOR
		m.lcHeader = m.lcHeader + '</p>'
		
		m.lcHtml = THIS.HtmlHeader('Functions A-Z', '', m.lcHeader) + m.lcHtml + CRLF + THIS.HtmlFooter()
		
		STRTOFILE(m.lcHtml, THIS.cHtmlPath + 'functionsa-z.html', 0)
		USE IN SELECT('vs')
	ENDFUNC

	FUNCTION CreateFunctionsByCategoryPage

		LOCAL lcHtml, lcGroup
		m.lcHtml = THIS.HtmlHeader('Functions by Category')
		m.lcHtml = m.lcHtml + '<table class="list">' + CRLF
		
		SELECT groupid, groupname, descrip, UPPER(groupname) AS corder FROM groups ORDER BY corder INTO CURSOR vg
		SELECT vg
		SCAN
				m.lcGroup = ALLTRIM(vg.groupname)
				m.lcHtml = m.lcHtml + [<tr class="] + IIF(MOD(RECNO('vg'), 2) = 0, 'even', 'uneven') + ["><td><a href="group] + LOWER(vg.groupid) + [.html">] + HtmlChars(m.lcGroup) + [</a></td><td>] + HtmlChars(vg.descrip) + [</td></tr>] + CRLF
		ENDSCAN	
		
		m.lcHtml = m.lcHtml + '</table>' + THIS.HtmlFooter()
		STRTOFILE(m.lcHtml, THIS.cHtmlPath + 'functionscategory.html', 0)
		USE IN SELECT('vg')
	ENDFUNC

	FUNCTION CreateCategoryPages
	
		LOCAL lcHtml, lcFunction, lcDescription

		SELECT gr
		SCAN

			m.lcHtml = THIS.HtmlHeader(ALLTRIM(gr.groupname))

			m.lcHtml = m.lcHtml + '<p>' + CRLF + HtmlChars(gr.descrip) + '</p>' + CRLF
			m.lcHtml = m.lcHtml + TextToHtml(gr.descrip2, 'p') + CRLF
			m.lcHtml = m.lcHtml + CRLF + '<table class="list">'
			
			SELECT expanded, descrip, UPPER(expanded) AS corder FROM vi WHERE vi.type = 'F' AND ALLTRIM(vi.cmd) == '{vfp2c32menu}' AND vi.groupid == gr.groupid ORDER BY corder INTO CURSOR vs
			SELECT vs
			SCAN
				m.lcFunction = ALLTRIM(vs.expanded)
				m.lcDescription = TextToHtml(vs.descrip, '', '')
				m.lcDescription = THIS.CreateLinks(m.lcDescription, .T., m.lcFunction)
				m.lcDescription = THIS.CreateLinks(m.lcDescription, .F., m.lcFunction)
				m.lcHtml = m.lcHtml + CRLF + [<tr class="] + IIF(MOD(RECNO('vs'), 2) = 0, 'even', 'uneven') + ["><td><a href="] + LOWER(m.lcFunction) + [.html">] + m.lcFunction + [</a></td><td>] + m.lcDescription + '</td></tr>'
			ENDSCAN

			m.lcHtml = m.lcHtml + CRLF + '</table>' + CRLF	
			m.lcHtml = m.lcHtml + THIS.HtmlFooter()
			
			STRTOFILE(m.lcHtml, THIS.cHtmlPath + 'group' + LOWER(gr.groupid) + '.html', 0)

		ENDSCAN
	
	ENDFUNC

ENDDEFINE

