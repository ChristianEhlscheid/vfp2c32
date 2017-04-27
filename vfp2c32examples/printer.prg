#INCLUDE vfp2c.h

&& Printer related functions
&& prerequisites: InitVFP2C32 must have been called with the VFP2C_INIT_PRINT flag set

CD (FULLPATH(JUSTPATH(SYS(16))))
SET LIBRARY TO vfp2c32.fll ADDITIVE
INITVFP2C32(VFP2C_INIT_PRINT)

LOCAL lnCount, xj

&& APRINTERSEX
&& wraps EnumPrinters - http://msdn.microsoft.com/library/default.asp?url=/library/en-us/gdi/prntspol_9fjn.asp

&& Level 1
&& http://search.msdn.microsoft.com/search/default.aspx?siteId=0&tab=0&query=PRINTER_INFO_1
lnCount = APRINTERSEX('laPrinters','',PRINTER_ENUM_LOCAL+PRINTER_ENUM_CONNECTIONS,1)
IF lnCount >= 1
	FOR xj = 1 TO lnCount
		? "Flags:", laPrinters[xj,1] 	
		? "Description:", laPrinters[xj,2] 	
		? "Name:", laPrinters[xj,3] 	
		? "Comment:", laPrinters[xj,4] 	
	ENDFOR
ENDIF

&& Level 2
&& http://search.msdn.microsoft.com/search/default.aspx?siteId=0&tab=0&query=PRINTER_INFO_2
lnCount = APRINTERSEX('laPrinters','',PRINTER_ENUM_LOCAL+PRINTER_ENUM_CONNECTIONS,2)
IF lnCount >= 1
	FOR xj = 1 TO lnCount
		? "Servername:", laPrinters[xj,1]
		? "Printername:", laPrinters[xj,2]
		? "Sharename:", laPrinters[xj,3]
		? "Portname:", laPrinters[xj,4]
		? "Drivername:", laPrinters[xj,5]
		? "Comment:", laPrinters[xj,6]
		? "Location:", laPrinters[xj,7]
		? "Seperation file:", laPrinters[xj,8]
		? "Printprocessor:", laPrinters[xj,9]
		? "Datatype:", laPrinters[xj,10]
		? "Parameters:", laPrinters[xj,11]
		? "Attributes:", laPrinters[xj,12]
		? "Priority:", laPrinters[xj,13]
		? "DefaultPriority:", laPrinters[xj,14]
		? "Starttime:", laPrinters[xj,15]
		? "Untiltime:", laPrinters[xj,16]
		? "Status:", laPrinters[xj,17]
		? "Jobs:", laPrinters[xj,18]
		? "AveragePPM:", laPrinters[xj,19]
	ENDFOR
ENDIF


&& Level 4
&& http://search.msdn.microsoft.com/search/default.aspx?siteId=0&tab=0&query=PRINTER_INFO_4
lnCount = APRINTERSEX('laPrinters','',PRINTER_ENUM_LOCAL+PRINTER_ENUM_CONNECTIONS,4)
IF lnCount >= 1
	FOR xj = 1 TO lnCount
		? "Printername", laPrinters[xj,1]
		? "Servername", laPrinters[xj,2]
		? "Attributes", laPrinters[xj,3]
	ENDFOR
ENDIF

&& Level 5
&& http://search.msdn.microsoft.com/search/default.aspx?siteId=0&tab=0&query=PRINTER_INFO_5
lnCount = APRINTERSEX('laPrinters','',PRINTER_ENUM_LOCAL+PRINTER_ENUM_CONNECTIONS,5)
IF lnCount >= 1
	FOR xj = 1 TO lnCount
		? "Printername", laPrinters[xj,1]
		? "Portname", laPrinters[xj,2]
		? "Attributes", laPrinters[xj,3]
		? "DeviceNotSelectedtimeout", laPrinters[xj,4]
		? "TransmissionRetryTimeout", laPrinters[xj,5]
	ENDFOR
ENDIF


&& APrintJobs
&& wraps EnumJobs api - http://search.msdn.microsoft.com/search/default.aspx?siteId=0&tab=0&query=EnumJobs

LOCAL lcPrinter, lcPort

&& use the first available printer for this example
APRINTERS(laPrinters)
lcPrinter = laPrinters[1,1]
lcPort = laPrinters[1,2]


&& Level 1
&& http://search.msdn.microsoft.com/search/default.aspx?siteId=0&tab=0&query=JOB_INFO_1
lnCount = APRINTJOBS('laJobs',lcPrinter,1)
IF lnCount >= 1
	FOR xj = 1 TO lnCount
		? "Document:", laJobs[xj,1]
		? "Printername:", laJobs[xj,2]
		? "Username:", laJobs[xj,3]
		? "Machinename:", laJobs[xj,4]
		? "Datatype:", laJobs[xj,5]
		? "pStatus:", laJobs[xj,6]
		? "JobID:", laJobs[xj,7]
		? "Status:", laJobs[xj,8]
		? "Priority:", laJobs[xj,9]
		? "Position:", laJobs[xj,10]
		? "TotalPages:", laJobs[xj,11]
		? "PagesPrinted:", laJobs[xj,12]
		? "Submitted time: ", laJobs[xj,13]
	ENDFOR
ENDIF

&& Level 2
&& http://search.msdn.microsoft.com/search/default.aspx?siteId=0&tab=0&query=JOB_INFO_2
lnCount = APRINTJOBS('laJobs',lcPrinter,1)
IF lnCount >= 1
	FOR xj = 1 TO lnCount
		? "Document:", laJobs[xj,1]
		? "Printername:", laJobs[xj,2]
		? "Username:", laJobs[xj,3]
		? "Machinename:", laJobs[xj,4]
		? "Datatype:", laJobs[xj,5]
		? "pStatus:", laJobs[xj,6]
		? "JobID:", laJobs[xj,7]
		? "Status:", laJobs[xj,8]
		? "Priority:", laJobs[xj,9]
		? "Position:", laJobs[xj,10]
		? "TotalPages:", laJobs[xj,11]
		? "PagesPrinted:", laJobs[xj,12]
		? "Submitted time:", laJobs[xj,13]
		? "Notifyname:", laJobs[xj,14]
		? "Printprocessor:", laJobs[xj,15]
		? "Parameters:", laJobs[xj,16]
		? "Drivername:", laJobs[xj,17]
		? "Time:", laJobs[xj,18]
		? "Starttime:", laJobs[xj,19]
		? "Untiltime:", laJobs[xj,20]
		? "Size:", laJobs[xj,21]
	ENDFOR
ENDIF


&& APrinterForms
&& wraps EnumForms api - http://search.msdn.microsoft.com/search/default.aspx?siteId=0&tab=0&query=EnumForms

&& http://search.msdn.microsoft.com/search/default.aspx?siteId=0&tab=0&query=FORM_INFO_1
lnCount = APRINTERFORMS('laForms',lcPrinter)
IF lnCount >= 1
	FOR xj = 1 TO lnCount
		? "Flags:", laForms[xj,1]
		? "Name:", laForms[xj,2]
		? "Size X:", laForms[xj,3]
		? "Size Y:", laForms[xj,4]
		? "Imageable Area Bottom:", laForms[xj,5]
		? "Imageable Area Top:", laForms[xj,6]
		? "Imageable Area Left:", laForms[xj,7]
		? "Imageable Area Right:", laForms[xj,8]
	ENDFOR
ENDIF


&& APrinterTrays
&& wraps DeviceCapabilities api - http://search.msdn.microsoft.com/search/default.aspx?siteId=0&tab=0&query=DeviceCapabilities

lnCount = APRINTERTRAYS('laTrays',lcPrinter,lcPort)
IF lnCount >= 1
	FOR xj = 1 TO lnCount
		? "Trayname:", laTrays[xj,1]
		? "Tray-No.:", laTrays[xj,2]
	ENDFOR
ENDIF


&& APaperSizes
&& wraps DeviceCapabilities api - http://search.msdn.microsoft.com/search/default.aspx?siteId=0&tab=0&query=DeviceCapabilities
lnCount = APAPERSIZES('laPapers', lcPrinter, lcPort, PAPERSIZE_UNIT_MM)
IF lnCount >= 1
	FOR xj = 1 TO lnCount
		? "No.", laPapers[xj,1]
		? "Name", laPapers[xj,2]
		? "Width", laPapers[xj,3]
		? "Height", laPapers[xj,4]
	ENDFOR
ENDIF

lnCount = APAPERSIZES('laPapers', lcPrinter, lcPort, PAPERSIZE_UNIT_INCH)
IF lnCount >= 1
	FOR xj = 1 TO lnCount
		? "No.", laPapers[xj,1]
		? "Name", laPapers[xj,2]
		? "Width", laPapers[xj,3]
		? "Height", laPapers[xj,4]
	ENDFOR
ENDIF