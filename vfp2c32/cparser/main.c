#include "standard.h"

extern int yydebug;
extern int yyparse();

extern int cclex();
extern FILE *ccin, *ccout;
extern FILE *pTrace;	

extern int delex();
extern FILE *dein, *deout;

int _stdcall DllMain(int hModule, unsigned long nReason, void* cSomeThing)
{
	return 1;
}

int _stdcall start_parse(char *pInput)
{
	int nRetVal;

	yyin = fopen(pInput, "r");
	
	yyrestart(yyin); // needs to be here for reentrency support after an parsing error (resets the lexer)
	gnTypeFlag = 1; // just for safety ..

#ifdef _DEBUG	
	pTrace = fopen("c:\\tracefile.txt","w");
#endif

	if (!yyin)
		return -1;
	
#ifdef _DEBUG
	yydebug = 1;
#endif
	
	gnLineNo = 1;
	
	nRetVal = yyparse();

	fclose(yyin);

#ifdef _DEBUG
	fclose(pTrace);
#endif

	return nRetVal;
}

int _stdcall remove_comments(char *pInput, char* pOutput)
{
	int nRetVal;
	
	ccin = fopen(pInput, "r");
		
	if (!ccin)
		return -1;
	
	ccout = fopen(pOutput,"w");

	if (!ccout)
	{
		fclose(ccin);
		return -1;	
	}	

	nRetVal = cclex();

	fclose(ccin);
	fclose(ccout);

	return nRetVal;
}

int _stdcall extract_defines(char *pInput, char* pOutput)
{
	int nRetVal;
	
	dein = fopen(pInput, "r");
		
	if (!dein)
		return -1;
	
	deout = fopen(pOutput,"w");

	if (!deout)
	{
		fclose(dein);
		return -1;	
	}	

	nRetVal = delex();

	fclose(dein);
	fclose(deout);

	return nRetVal;
}

void _stdcall debug_entry()
{
	char pBuffer[256];
	int nDummy = 11;
	sprintf(pBuffer,"hello C %d",nDummy);
}