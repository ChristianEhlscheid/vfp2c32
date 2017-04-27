#include <windows.h>
#include <math.h>

#include "pro_ext.h"
#include "vfp2c32.h"
#include "vfpmacros.h"
#include "vfp2cutil.h"
#include "vfp2cprint.h"
#include "vfp2ccppapi.h"
#include "vfp2chelpers.h"
#include "vfpmacros.h"

void _fastcall APrintersEx(ParamBlk *parm)
{
try
{
 	FoxArray pArray(p1);
   	FoxString pName(parm,2);
	FoxString pData(1024);
	FoxObject pObject;

	CBuffer pBuffer(1024 * 16); // 16KB should be sufficient space 

	LPPRINTER_INFO_1 pInfo1;
	LPPRINTER_INFO_2 pInfo2;
	LPPRINTER_INFO_4 pInfo4;
	LPPRINTER_INFO_5 pInfo5;

	DWORD dwFlags = PCount() >= 3 && p3.ev_long ? p3.ev_long : PRINTER_ENUM_LOCAL;
	DWORD dwLevel = PCount() >= 4 && p4.ev_long ? p4.ev_long : 2;
	DWORD dwDest = PCount() >= 5 && p5.ev_long ? p5.ev_long : APRINT_DEST_ARRAY;
	DWORD dwBytes, dwCount;

	if (Vartype(p2) != '0' && Vartype(p2) != 'C')
		throw E_INVALIDPARAMS;

	if (dwLevel != 1 && dwLevel != 2 && dwLevel != 4 && dwLevel != 5)
		throw E_INVALIDPARAMS;

	// If Level is 4, you can only use the PRINTER_ENUM_CONNECTIONS and PRINTER_ENUM_LOCAL constants.
	if (dwLevel == 4 && (dwFlags & ~(PRINTER_ENUM_CONNECTIONS|PRINTER_ENUM_LOCAL)) > 0)
		throw E_INVALIDPARAMS;

	if (dwDest != APRINT_DEST_ARRAY && dwDest != APRINT_DEST_OBJECTARRAY)
		throw E_INVALIDPARAMS;

	if (!EnumPrinters(dwFlags,pName,dwLevel,pBuffer,pBuffer.Size(),&dwBytes,&dwCount))
	{
		DWORD nLastError = GetLastError();
		if (nLastError == ERROR_INSUFFICIENT_BUFFER)
		{
			pBuffer.Size(dwBytes);
			if (!EnumPrinters(dwFlags,pName,dwLevel,pBuffer,pBuffer.Size(),&dwBytes,&dwCount))
			{
				SaveWin32Error("EnumPrinters", nLastError);
				throw E_APIERROR;
			}
		}
		else
		{
			SaveWin32Error("EnumPrinters", nLastError);
			throw E_APIERROR;
		}
	}

	if (dwCount == 0)
	{
		Return(0);
		return;
	}

	switch(dwLevel)
	{
		case 1:
			pInfo1 = reinterpret_cast<LPPRINTER_INFO_1>(pBuffer.Address());
			if (dwDest == APRINT_DEST_ARRAY)
			{
				pArray.Dimension(dwCount,4);
				for (unsigned int xj = 1; xj <= dwCount; xj++)
				{
					pArray(xj,1) = pInfo1->Flags;
					pArray(xj,2) = pData = pInfo1->pDescription;
					pArray(xj,3) = pData = pInfo1->pName;
					pArray(xj,4) = pData = pInfo1->pComment;
					pInfo1++;
				}
			}
			else
			{
				pArray.Dimension(dwCount,1);
				for (unsigned int xj = 1; xj <= dwCount; xj++)
				{
					pObject.EmptyObject();
					pObject("Flags") << pInfo1->Flags;
					pObject("Description") << (pData = pInfo1->pDescription);
					pObject("Name") << (pData = pInfo1->pName);
					pObject("Comment") << (pData = pInfo1->pComment);
					pArray(xj) = pObject;
					pInfo1++;
				}
			}
			break;

		case 2:
			pInfo2 = reinterpret_cast<LPPRINTER_INFO_2>(pBuffer.Address());
			if (dwDest == APRINT_DEST_ARRAY)
			{
				pArray.Dimension(dwCount,19);
				for (unsigned int xj = 1; xj <= dwCount; xj++)
				{
					pArray(xj,1) = pData = pInfo2->pServerName;
					pArray(xj,2) = pData = pInfo2->pPrinterName;
					pArray(xj,3) = pData = pInfo2->pShareName;
					pArray(xj,4) = pData = pInfo2->pPortName;
					pArray(xj,5) = pData = pInfo2->pDriverName;
					pArray(xj,6) = pData = pInfo2->pComment;
					pArray(xj,7) = pData = pInfo2->pLocation;
					pArray(xj,8) = pData = pInfo2->pSepFile;
					pArray(xj,9) = pData = pInfo2->pPrintProcessor;
					pArray(xj,10) = pData = pInfo2->pDatatype;
					pArray(xj,11) = pData = pInfo2->pParameters;
					pArray(xj,12) = pInfo2->Attributes;
					pArray(xj,13) = pInfo2->Priority;
					pArray(xj,14) = pInfo2->DefaultPriority;
					pArray(xj,15) = pInfo2->StartTime;
					pArray(xj,16) = pInfo2->UntilTime;
					pArray(xj,17) = pInfo2->Status;
					pArray(xj,18) = pInfo2->cJobs;
					pArray(xj,19) = pInfo2->AveragePPM;
					pInfo2++;
				}
			}
			else
			{
				pArray.Dimension(dwCount,1);
				for (unsigned int xj = 1; xj <= dwCount; xj++)
				{
					pObject.EmptyObject();
					pObject("ServerName") << (pData = pInfo2->pServerName);
					pObject("PrinterName") << (pData = pInfo2->pPrinterName);
					pObject("ShareName") << (pData = pInfo2->pShareName);
					pObject("PortName") << (pData = pInfo2->pPortName);
					pObject("DriverName") << (pData = pInfo2->pDriverName);
					pObject("Comment") << (pData = pInfo2->pComment);
					pObject("Location") << (pData = pInfo2->pLocation);
					pObject("SepFile") << (pData = pInfo2->pSepFile);
					pObject("PrintProcessor") << (pData = pInfo2->pPrintProcessor);
					pObject("Datatype") << (pData = pInfo2->pDatatype);
					pObject("Parameters") << (pData = pInfo2->pParameters);
					pObject("Attributes") << pInfo2->Attributes;
					pObject("Priority") << pInfo2->Priority;
					pObject("DefaultPriority") << pInfo2->DefaultPriority;
					pObject("StartTime") << pInfo2->StartTime;
					pObject("UntilTime") << pInfo2->UntilTime;
					pObject("Status") << pInfo2->Status;
					pObject("Jobs") << pInfo2->cJobs;
					pObject("AveragePPM") << pInfo2->AveragePPM;
					pArray(xj) = pObject;
					pInfo2++;
				}
			}
			break;

		case 4:
			pInfo4 = reinterpret_cast<LPPRINTER_INFO_4>(pBuffer.Address());
			if (dwDest == APRINT_DEST_ARRAY)
			{
				pArray.Dimension(dwCount,3);
				for (unsigned int xj = 1; xj <= dwCount; xj++)
				{
					pArray(xj,1) = pData = pInfo4->pPrinterName;
					pArray(xj,2) = pData = pInfo4->pServerName;
					pArray(xj,3) = pInfo4->Attributes;
					pInfo4++;
				}
			}
			else
			{
				pArray.Dimension(dwCount,1);
				for (unsigned int xj = 1; xj <= dwCount; xj++)
				{
					pObject.EmptyObject();
					pObject("PrinterName") << (pData = pInfo4->pPrinterName);
					pObject("ServerName") << (pData = pInfo4->pServerName);
					pObject("Attributes") << pInfo4->Attributes;
					pArray(xj) = pObject;
					pInfo4++;
				}
			}
			break;

		case 5:
			pInfo5 = reinterpret_cast<LPPRINTER_INFO_5>(pBuffer.Address());
			if (dwDest == APRINT_DEST_ARRAY)
			{
				pArray.Dimension(dwCount,5);
				for (unsigned int xj = 1; xj <= dwCount; xj++)
				{
					pArray(xj,1) = pData = pInfo5->pPrinterName;
					pArray(xj,2) = pData = pInfo5->pPortName;
					pArray(xj,3) = pInfo5->Attributes;
					pArray(xj,4) = pInfo5->DeviceNotSelectedTimeout;
					pArray(xj,5) = pInfo5->TransmissionRetryTimeout;
					pInfo5++;
				}
			}
			else
			{
				pArray.Dimension(dwCount,1);
				for (unsigned int xj = 1; xj <= dwCount; xj++)
				{
					pObject.EmptyObject();
					pObject("PrinterName") << (pData = pInfo5->pPrinterName);
					pObject("PortName") << (pData = pInfo5->pPortName);
					pObject("Attributes") << pInfo5->Attributes;
					pObject("DeviceNotSelectedTimeout") << pInfo5->DeviceNotSelectedTimeout;
					pObject("TransmissionRetryTimeout") << pInfo5->TransmissionRetryTimeout;
					pArray(xj) = pObject;
					pInfo5++;
				}
			}
	}

	pArray.ReturnRows();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall APrintJobs(ParamBlk *parm)
{
try
{
	FoxArray pArray(p1);
	FoxString pPrinter(p2);
	FoxString pJob(PRINT_ENUM_BUFFER);
	FoxDateTime pDateTime;
	FoxObject pObject;

	PrinterHandle hPrinter;
	DWORD dwJobs, dwLevel, dwBytes, dwDest;
	LPJOB_INFO_1 pJobInfo;
	LPJOB_INFO_2 pJobInfo2;

	dwLevel = PCount() >= 3 ? p3.ev_long : 1;
	dwDest = PCount() >= 4 ? p4.ev_long : APRINT_DEST_ARRAY;

	if (dwLevel != 1 && dwLevel != 2)
		throw E_INVALIDPARAMS;

	if (dwDest != APRINT_DEST_ARRAY && dwDest != APRINT_DEST_OBJECTARRAY)
		throw E_INVALIDPARAMS;

	if(!OpenPrinter(pPrinter, hPrinter, NULL))
	{
		SaveWin32Error("OpenPrinter", GetLastError());
		throw E_APIERROR;
	}

	// call EnumJobs() to find out how much memory is needed
	if(!EnumJobs(hPrinter, 0, 0xFFFFFFFF, dwLevel, 0, 0, &dwBytes, &dwJobs))
	{
		DWORD nLastError = GetLastError();
		if (nLastError != ERROR_INSUFFICIENT_BUFFER)
		{
			SaveWin32Error("EnumJobs", nLastError);
			throw E_APIERROR;
		}
	}

	if (!dwBytes)
	{
		Return(0);
		return;
	}

	CBuffer pBuffer(dwBytes);

	if(!EnumJobs(hPrinter, 0, 0xFFFFFFFF, dwLevel, pBuffer, dwBytes, &dwBytes, &dwJobs))
	{
		SaveWin32Error("EnumJobs", GetLastError());
		throw E_APIERROR;
	}

	if (dwLevel == 1)
	{
		pJobInfo = reinterpret_cast<LPJOB_INFO_1>(pBuffer.Address());
		if (dwDest == APRINT_DEST_ARRAY)
		{
			pArray.Dimension(dwJobs,13);
			for (unsigned int xj = 1; xj <= dwJobs; xj++)
			{
				pArray(xj,1) = pJob = pJobInfo->pDocument;
				pArray(xj,2) = pJob = pJobInfo->pPrinterName;
				pArray(xj,3) = pJob = pJobInfo->pUserName;
				pArray(xj,4) = pJob = pJobInfo->pMachineName;
				pArray(xj,5) = pJob = pJobInfo->pDatatype;
				pArray(xj,6) = pJob = pJobInfo->pStatus;
				pArray(xj,7) = pJobInfo->JobId;
				pArray(xj,8) = pJobInfo->Status;
				pArray(xj,9) = pJobInfo->Priority;
				pArray(xj,10) = pJobInfo->Position;
				pArray(xj,11) = pJobInfo->TotalPages;
				pArray(xj,12) = pJobInfo->PagesPrinted;
				pArray(xj,13) = pDateTime = pJobInfo->Submitted;
				pJobInfo++;
			}
		}
		else 
		{
			pArray.Dimension(dwJobs,1);
			for (unsigned int xj = 1; xj <= dwJobs; xj++)
			{
				pObject.EmptyObject();
				pObject("Document") << (pJob = pJobInfo->pDocument);
				pObject("PrinterName") << (pJob = pJobInfo->pPrinterName);
				pObject("UserName") << (pJob = pJobInfo->pUserName);
				pObject("MachineName") << (pJob = pJobInfo->pMachineName);
				pObject("Datatype") << (pJob = pJobInfo->pDatatype);
				pObject("pStatus") << (pJob = pJobInfo->pStatus);
				pObject("JobId") << pJobInfo->JobId;
				pObject("Status") << pJobInfo->Status;
				pObject("Priority") << pJobInfo->Priority;
				pObject("Position") << pJobInfo->Position;
				pObject("TotalPages") << pJobInfo->TotalPages;
				pObject("PagesPrinted") << pJobInfo->PagesPrinted;
				pObject("Submitted") << (pDateTime = pJobInfo->Submitted);
				pArray(xj) = pObject;
				pJobInfo++;
			}
		}
	}
	else
	{
		pJobInfo2 = reinterpret_cast<LPJOB_INFO_2>(pBuffer.Address());
		if (dwDest == APRINT_DEST_ARRAY)
		{
			pArray.Dimension(dwJobs,21);
			for (unsigned int xj = 1; xj <= dwJobs; xj++)
			{
				pArray(xj,1) = pJob = pJobInfo2->pDocument;
				pArray(xj,2) = pJob = pJobInfo2->pPrinterName;
				pArray(xj,3) = pJob = pJobInfo2->pUserName;
				pArray(xj,4) = pJob = pJobInfo2->pMachineName;
				pArray(xj,5) = pJob = pJobInfo2->pDatatype;
				pArray(xj,6) = pJob = pJobInfo2->pStatus;
				pArray(xj,7) = pJobInfo2->JobId;
				pArray(xj,8) = pJobInfo2->Status;
				pArray(xj,9) = pJobInfo2->Priority;
				pArray(xj,10) = pJobInfo2->Position;
				pArray(xj,11) = pJobInfo2->TotalPages;
				pArray(xj,12) = pJobInfo2->PagesPrinted;
				pArray(xj,13) = pDateTime = pJobInfo2->Submitted;
				pArray(xj,14) = pJob = pJobInfo2->pNotifyName;
				pArray(xj,15) = pJob = pJobInfo2->pPrintProcessor;
				pArray(xj,16) = pJob = pJobInfo2->pParameters;
				pArray(xj,17) = pJob = pJobInfo2->pDriverName;
				pArray(xj,18) = pJobInfo2->Time;
				pArray(xj,19) = pJobInfo2->StartTime;
				pArray(xj,20) = pJobInfo2->UntilTime;
				pArray(xj,21) = pJobInfo2->Size;
				pJobInfo2++;
			}
		}
		else
		{
			pArray.Dimension(dwJobs,1);
			for (unsigned int xj = 1; xj <= dwJobs; xj++)
			{
				pObject.EmptyObject();
				pObject("Document") << (pJob = pJobInfo2->pDocument);
				pObject("PrinterName") << (pJob = pJobInfo2->pPrinterName);
				pObject("UserName") << (pJob = pJobInfo2->pUserName);
				pObject("MachineName") << (pJob = pJobInfo2->pMachineName);
				pObject("Datatype") << (pJob = pJobInfo2->pDatatype);
				pObject("pStatus") << (pJob = pJobInfo2->pStatus);
				pObject("JobId") << pJobInfo2->JobId;
				pObject("Status") << pJobInfo2->Status;
				pObject("Priority") << pJobInfo2->Priority;
				pObject("Position") << pJobInfo2->Position;
				pObject("TotalPages") << pJobInfo2->TotalPages;
				pObject("PagesPrinted") << pJobInfo2->PagesPrinted;
				pObject("Submitted") << (pDateTime = pJobInfo2->Submitted);
				pObject("Notifyname") << (pJob = pJobInfo2->pNotifyName);
				pObject("PrintProcessor") << (pJob = pJobInfo2->pPrintProcessor);
				pObject("Parameters") << (pJob = pJobInfo2->pParameters);
				pObject("DriverName") << (pJob = pJobInfo2->pDriverName);
				pObject("Time") << pJobInfo2->Time;
				pObject("StartTime") << pJobInfo2->StartTime;
				pObject("UntilTime") << pJobInfo2->UntilTime;
				pObject("Size") << pJobInfo2->Size;
				pArray(xj) = pObject;
				pJobInfo2++;
			}
		}
	}
	pArray.ReturnRows();
}
catch (int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall APrinterForms(ParamBlk *parm)
{
try
{
	FoxArray pArray(p1);
	FoxString pPrinter(parm,2);
	FoxString pForm(PRINT_ENUM_BUFFER);
	PrinterHandle hPrinter;
	DWORD dwBytes, dwForms;
	PFORM_INFO_1 pFormInfo;

	if(!OpenPrinter(pPrinter, hPrinter, 0))
	{
		SaveWin32Error("OpenPrinter", GetLastError());
		throw E_APIERROR;
	}

	// First call EnumForms() to find out how much memory we need
	if (!EnumForms(hPrinter, 1, 0, 0, &dwBytes, &dwForms))
	{
		DWORD nLastError = GetLastError();
		if (nLastError != ERROR_INSUFFICIENT_BUFFER)
		{
			SaveWin32Error("EnumForms", nLastError);
			throw E_APIERROR;
		}
	}
	else
	{
		Return(0);
		return;
	}

	CBuffer pBuffer(dwBytes);
	if (!EnumForms(hPrinter, 1, pBuffer, dwBytes, &dwBytes, &dwForms))
	{
		SaveWin32Error("EnumForms", GetLastError());
		throw E_APIERROR;
	}

	if (dwForms == 0)
	{
		Return(0);
		return;
	}

	pArray.Dimension(dwForms,8);
	pFormInfo = reinterpret_cast<PFORM_INFO_1>(pBuffer.Address());

	for (unsigned int xj = 1; xj <= dwForms; xj++)
	{
		pArray(xj,1) = pFormInfo->Flags;
		pArray(xj,2) = pForm = pFormInfo->pName;
		pArray(xj,3) = pFormInfo->Size.cx;
		pArray(xj,4) = pFormInfo->Size.cy;
		pArray(xj,5) = pFormInfo->ImageableArea.bottom;
		pArray(xj,6) = pFormInfo->ImageableArea.top;
		pArray(xj,7) = pFormInfo->ImageableArea.left;
		pArray(xj,8) = pFormInfo->ImageableArea.right;
		pFormInfo++;
	}

	pArray.ReturnRows();
}
catch (int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall APaperSizes(ParamBlk *parm)
{
try
{
	FoxArray pArray(p1);
	FoxString pPrinter(p2);
	FoxString pPort(p3);
	FoxDouble pDouble(4);
	int nUnit = PCount() == 4 ? p4.ev_long : PAPERSIZE_UNIT_MM;

	if (nUnit < PAPERSIZE_UNIT_MM || nUnit > PAPERSIZE_UNIT_POINT)
		throw E_INVALIDPARAMS;

	FoxString pPapernameTmp(65);
	DWORD dwCount;

	dwCount = DeviceCapabilities(pPrinter, pPort, DC_PAPERS, 0, 0);
	if (dwCount == 0)
	{
		Return(0);
		return;
	}

	CBuffer pPapersBuffer(dwCount * sizeof(WORD));
	CBuffer pPapernamesBuffer(dwCount * 64);
	CBuffer pPapersizeBuffer(dwCount * sizeof(POINT));
	
	dwCount = DeviceCapabilities(pPrinter, pPort, DC_PAPERS, pPapersBuffer, 0);
	dwCount = DeviceCapabilities(pPrinter, pPort, DC_PAPERNAMES, pPapernamesBuffer, 0);
	dwCount = DeviceCapabilities(pPrinter, pPort, DC_PAPERSIZE, pPapersizeBuffer, 0);
   
	pArray.Dimension(dwCount, 4);

	WORD *pPapers = reinterpret_cast<WORD*>(pPapersBuffer.Address());
	char *pPapernames = reinterpret_cast<char*>(pPapernamesBuffer.Address());
	POINT *pPapersize = reinterpret_cast<POINT*>(pPapersizeBuffer.Address());

	for (unsigned int xj = 1; xj <= dwCount; xj++)
	{
		pArray(xj,1) = *pPapers;
		pArray(xj,2) = pPapernameTmp.StrnCpy(pPapernames, 64);

		switch(nUnit)
		{
			case PAPERSIZE_UNIT_MM:
				pArray(xj,3) = pPapersize->x;
				pArray(xj,4) = pPapersize->y;
				break;
			case PAPERSIZE_UNIT_INCH:
				pArray(xj,3) = pDouble = (pPapersize->x * INCH_PER_MM / 10);
				pArray(xj,4) = pDouble = (pPapersize->y * INCH_PER_MM / 10);
				break;
			case PAPERSIZE_UNIT_POINT:
				pArray(xj,3) = pDouble = (floor(pPapersize->x * POINTS_PER_MM + 0.49) / 10);
				pArray(xj,4) = pDouble = (floor(pPapersize->y * POINTS_PER_MM + 0.49) / 10);
				break;
		}

		pPapers++;
		pPapernames += 64;
		pPapersize++;
	}

	pArray.ReturnRows();
}
catch (int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall APrinterTrays(ParamBlk *parm)
{
try
{
	FoxArray pArray(p1);
	FoxString pPrinter(p2);
	FoxString pPort(p3);
	FoxString pBinName(PRINT_TRAY_BUFFER);
	DWORD nBins;
	LPWORD pBinPtr;
	char *pBinNamesPtr;
	
	// get the number of bins
	nBins = DeviceCapabilities(pPrinter,pPort,DC_BINS,0,0);
	if (nBins == -1)
	{
		SaveWin32Error("DeviceCapabilities", GetLastError());
		throw E_APIERROR;
	}
	else if (nBins == 0)
	{
		Return(0);
		return;
	}

	CBuffer pBins(nBins * sizeof(WORD));
	CBuffer pNames(nBins * PRINT_TRAY_BUFFER);

	if (DeviceCapabilities(pPrinter,pPort,DC_BINS,pBins,0) == -1 ||
		DeviceCapabilities(pPrinter,pPort,DC_BINNAMES,pNames,0) == -1)
	{
		SaveWin32Error("DeviceCapabilities", GetLastError());
		throw E_APIERROR;
	}

	pArray.Dimension(nBins,2);

	pBinPtr = reinterpret_cast<LPWORD>(pBins.Address());
	pBinNamesPtr = pNames;

	for (unsigned int xj = 1; xj <= nBins; xj++)
	{
		pArray(xj,1) = pBinName = pBinNamesPtr;
		pArray(xj,2) = *pBinPtr;
		
		pBinNamesPtr += PRINT_TRAY_BUFFER;
		pBinPtr++;
	}

	pArray.ReturnRows();
}
catch (int nErrorNo)
{
	RaiseError(nErrorNo);
}
}
