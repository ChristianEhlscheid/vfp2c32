#include "standard.h"

#define PRIME 65537

// hash table for known typedefs
LPTYPEDEF gTypeHashTab[PRIME] = {0};
LPPARSEDTYPES gParsedTypes = 0;
LPPARSEERRORS gParseErrors = 0;
int gErrorCount = 0;
int nHandleError = 1;

// computes hash value of a string
static int hash_of(char *s)
{ 
	const char *p;
	unsigned int hash = 0, g;
	for (p=s; *p != '\0'; p++)
	{
	    hash = (hash << 4) + (*p);
		g = hash & 0xF0000000;
	    if (g != 0)
		{  
	      hash = hash ^ (g >> 24);
		  hash = hash ^ g;
		}
	}
	return hash % PRIME;
}

LPTYPEDEF _stdcall add_knowntype(char *pTypename)
{
	LPTYPEDEF curr;
	curr = find_knowntype(pTypename);
    if (curr == NULL)
	{  // Not found, so add it (at the head)
		int hash = hash_of(pTypename);
        curr = (LPTYPEDEF)malloc(sizeof(STYPEDEF));
		curr->pTypename = strdup(pTypename);
		curr->next = gTypeHashTab[hash];	
		gTypeHashTab[hash] = curr;
		return curr;
	}
	return NULL;
}

void _stdcall remove_knowntype(char *pTypename)
{
  LPTYPEDEF curr, prev=NULL;
  int hash = hash_of(pTypename);

  for (curr=gTypeHashTab[hash]; curr!=NULL; curr=curr->next)
  {
    if (strcmp(pTypename,curr->pTypename) == 0)
	{
		if (prev != NULL)
			prev->next = curr->next;
		else if (curr == gTypeHashTab[hash])
			gTypeHashTab[hash] = curr->next;

		if (curr->pTypename)
			free(curr->pTypename);
        free(curr);
		break;
    }
    prev = curr;
  }
}

void _stdcall clear_knowntypes()
{
	LPTYPEDEF curr, next;
	int xj;

	for (xj=0; xj<PRIME; xj++)
	{
	    for (curr=gTypeHashTab[xj]; curr!=NULL; curr=next)
		{
			next = curr->next;
			if (curr->pTypename)
				free(curr->pTypename);
			free(curr);
		}
	}
}

// lookup if a typedef is in the hashtable
LPTYPEDEF _stdcall find_knowntype(char *pTypename)
{
  LPTYPEDEF curr;
  int hash = hash_of(pTypename);
  for (curr=gTypeHashTab[hash]; curr!=NULL ; curr=curr->next)
  {
  	  if (strcmp(pTypename,curr->pTypename) == 0) // found
		return curr;
  }
  // Not found
  return NULL;
}

void _stdcall add_parsedtype(LPPARSENODE pN)
{
	LPPARSEDTYPES pT = malloc(sizeof(PARSEDTYPES));
	pT->node = pN;
	pT->next = gParsedTypes;
	gParsedTypes = pT;
}

void _stdcall clear_parsedtypes()
{
	LPPARSEDTYPES pT1, pT2;
	if (gParsedTypes)
	{
		pT1 = gParsedTypes;
		while (pT2 = pT1->next)
		{
			free_pnode(pT1->node);
			pT1 = pT2;
		}
		free_pnode(pT1->node);
	}
	gParsedTypes = NULL;
}

void _stdcall free_pnode(LPPARSENODE pN)
{
	switch (pN->nType)
	{
		case T_IDENTIFIER:
		case T_TYPENAME:
			free(pN->un.String);
			break;
		case T_CONSTANT:
			break;
		default:
			if (pN->link1)
				free_pnode(pN->link1);
			if (pN->link2)
				free_pnode(pN->link2);
			if (pN->link3)
				free_pnode(pN->link3);
	}
	free(pN);
}

LPPARSEDTYPES _stdcall get_parsedtypehead()
{
	return gParsedTypes;
}

LPPARSENODE _stdcall create_pnode(NODETYPE nType, LPPARSENODE p1, LPPARSENODE p2, LPPARSENODE p3)
{
	LPPARSENODE node = (LPPARSENODE)malloc(sizeof(PARSENODE));
    node->nType = nType;
	node->link1 = p1;
	node->link2 = p2;
	node->link3 = p3;
	node->nLineNo = gnLineNo;
	return node;
}

LPPARSENODE _stdcall create_string(NODETYPE nType, char *pString)
{
	LPPARSENODE node = (LPPARSENODE)malloc(sizeof(PARSENODE));
    node->nType = nType;
	node->un.String = strdup(pString);
	node->nLineNo = gnLineNo;
	return node;
}

LPPARSENODE _stdcall create_const(int nConst)
{
	LPPARSENODE node = (LPPARSENODE)malloc(sizeof(PARSENODE));
    node->nType = T_CONSTANT;
	node->un.Constant = nConst;
	node->nLineNo = gnLineNo;
	return node;
}

LPPARSEERRORS _stdcall get_parseerrorhead()
{
	return gParseErrors;
}

int _stdcall get_errorcount()
{
	return gErrorCount;
}

void _stdcall add_error(char *pMask, char *pValue, int nLineNo)
{
	int nChars;
	LPPARSEERRORS pE = malloc(sizeof(PARSEERRORS));
	nChars = _scprintf(pMask,pValue);
	pE->pErrorMes = malloc(nChars+1);
	sprintf(pE->pErrorMes,pMask,pValue);
	pE->nLineNo = nLineNo;
	pE->next = gParseErrors;
	gParseErrors = pE;
	gErrorCount++;
}

void _stdcall clear_errors()
{
	LPPARSEERRORS pE1, pE2;
	if (gParseErrors)
	{
		pE1 = gParseErrors;
		while (pE2 = pE1->next)
		{
			free(pE1->pErrorMes);
			free(pE1);
			pE1 = pE2;
		}
		free(pE1->pErrorMes);
		free(pE1);
	}
	gParseErrors = NULL;
	gErrorCount = 0;
}

