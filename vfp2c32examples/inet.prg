#INCLUDE vfp2c.h

&& UrlDownloadToFile 
&& prerequisites: InitVFP2C32 must have been called with the VFP2C_INIT_URLMON & VFP2C_INIT_CALLBACK flags set

CD (FULLPATH(JUSTPATH(SYS(16))))
SET LIBRARY TO vfp2c32.fll ADDITIVE
INITVFP2C32(VFP2C_INIT_ALL)

LOCAL laUrl1, lcUrl2, lcUrl3, lcUrl4, lcFile, lcFile2, lcFile3, lcFile4
lcUrl = "http:\\www.vfp2c.com\bilder\testfile.zip"
lcUrl2 = "http:\\www.vfp2c.com\bilder\TT.mp4"
lcUrl3 = "http:\\www.vfp2c.com\bilder\VIDEO_TS3.VOB"
lcUrl4 = "http:\\www.vfp2c.com\bilder\VIDEO_TS4.VOB"
lcFile = "C:\myvideo.vob"
lcFile2 = "C:\myvideo2.vob"
lcFile3 = "C:\myvideo3.vob"
lcFile4 = "C:\myvideo4.vob"

&& !!! UrlDownloadToFile (the underlying function) needs the protocol (http:\\) prefix !!!
&& downloading the file without progress
&&? UrlDownloadToFileEx(lcUrl,lcFile)

PUBLIC loForm, loForm2, loForm3, loForm4
DO FORM frmdownloadprogress NAME loForm
DO FORM frmdownloadprogress NAME loForm2
DO FORM frmdownloadprogress NAME loForm3
DO FORM frmdownloadprogress NAME loForm4

loForm.Top = 0
loForm2.Top = loForm.Top + loForm.Height + 30
loForm3.Top = loForm2.Top + loForm2.Height + 30
loForm4.Top = loForm3.Top + loForm3.Height + 30

loForm.StartDownload(lcUrl,lcFile,"loForm.Progress",.T.)
loForm2.StartDownload(lcUrl2,lcFile2,"loForm2.Progress",.T.)
loForm3.StartDownload(lcUrl3,lcFile3,"loForm3.Progress",.T.)
loForm4.StartDownload(lcUrl4,lcFile4,"loForm4.Progress",.T.)

RETURN

&& return value is the HRESULT of the UrlDownloadToFile function
&& if it's not S_OK (0) you can call AErrorEx for error information

&& list IP's of the current machine
? AIPADDRESSES('laIPS')
DISPLAY MEMORY LIKE laIPS

&& get IP of a host/domain
? ResolveHostToIP('www.google.com')

&& the above call returns the first IP the winsock library reports
&& if you want all IP's of a host you can do
? ResolveHostToIP('www.google.com','laIPS')
DISPLAY MEMORY LIKE laIPS


&& IcmpPing
&& prerequisites: InitVFP2C32 must have been called with the VFP2C_INIT_IPHELPER & VFP2C_INIT_WINSOCK flags set

&& Ping a remote host
LOCAL lnPings
lnPings = ICMPPING('laPings','www.google.com')
IF lnPings > 0
	? "Return address", laPings[1]
	? "Roundtrip time", laPings[2]
	? "Status", laPings[3]
	? "Ping data valid", laPings[4]
ENDIF

? ICMPPING('laPings','www.google.com',0,0,0,0,.F.,4)
DISPLAY MEMORY LIKE laPings

&& Ip2MacAddress
&& prerequisites: InitVFP2C32 must have been called with the VFP2C_INIT_IPHELPER flag set
&& uses SendArp API - http://search.msdn.microsoft.com/search/default.aspx?siteId=0&tab=0&query=SendArp
&& !!! Works only for IP addresses in your local network !!!!
? IP2MACADDRESS('192.168.0.1')
