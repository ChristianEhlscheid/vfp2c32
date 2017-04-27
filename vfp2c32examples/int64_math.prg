CD (FULLPATH(JUSTPATH(SYS(16))))
SET LIBRARY TO vfp2c32.fll ADDITIVE

LOCAL lcVar, lcMax, lcMin
lcMax = "9223372036854775807"
lcMin  = "-9223372036854775808"

lnDiv = INT64_DIV(m.lcMax, 2)
? INT64_ADD(INT64_MUL(m.lnDiv, 2), 1, 2) == m.lcMax

m.lcBinary = INT64_MUL(m.lnDiv, 2, 3)
?STR2INT64(m.lcBinary, 2)

m.lnDiv = INT64_DIV(m.lcMin, 2)
m.lnMul = INT64_MUL(m.lnDiv, 2)
? INT642STR(m.lnMul, 2) == m.lcMin

TRY
	INT64_DIV(m.lcMin, -1)
CATCH TO loError
	? 'Overflow exptected!', m.loError.Message
ENDTRY

TRY
	INT64_ADD(m.lcMax, 1)
CATCH TO loError
	? 'Overflow exptected!', m.loError.Message
ENDTRY

TRY
	INT64_SUB(m.lcMin, 1)
CATCH TO loError
	? 'Overflow exptected!', m.loError.Message
ENDTRY

TRY
	m.lcTmp = INT64_SUB(m.lnDiv, 2)
	INT64_MUL(m.lcTmp, 2)
CATCH TO loError
	? 'Overflow exptected!', m.loError.Message
ENDTRY

