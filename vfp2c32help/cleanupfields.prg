CD (JUSTPATH(FULLPATH(SYS(16))))

UPDATE vfp2cintelli SET tip = RemoveWhiteSpaceFromEnd(tip), ;
				data = RemoveWhiteSpaceFromEnd(data), ;
				returnval = RemoveWhiteSpaceFromEnd(returnval), ;
				descrip = RemoveWhiteSpaceFromEnd(descrip), ;
				example = RemoveWhiteSpaceFromEnd(example), ;
				apifuncs = RemoveWhiteSpaceFromEnd(apifuncs) ;
	WHERE type = 'F'

FUNCTION RemoveWhiteSpaceFromEnd
	LPARAMETERS lcValue
	RETURN RTRIM(m.lcValue, 0, CHR(13), CHR(10), CHR(32), CHR(9))
ENDFUNC