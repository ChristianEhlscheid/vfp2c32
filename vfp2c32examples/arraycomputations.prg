&& prerequisites: NONE!

CD (FULLPATH(JUSTPATH(SYS(16))))
SET LIBRARY TO vfp2c32.fll ADDITIVE

DIMENSION laTest[20]
FOR xj = 1 TO 20
laTest[xj] = xj
ENDFOR

?ASum(@laTest)
?AMin(@laTest)
?AMax(@laTest)
?AAverage(@laTest)

FOR xj = 1 TO 20
laTest[xj] = NTOM(xj+0.01)
ENDFOR

?ASum(@laTest)
?AMin(@laTest)
?AMax(@laTest)
?AAverage(@laTest)

FOR xj = 1 TO 20
laTest[xj] = DATETIME()+xj
ENDFOR
?AMin(@laTest)
?AMax(@laTest)

FOR xj = 1 TO 20
laTest[xj] = DATE()+xj
ENDFOR
?AMin(@laTest)
?AMax(@laTest)

DIMENSION laTest[20,2]
FOR xj = 1 TO 20
laTest[xj,1] = xj
laTest[xj,2] = xj * 10
ENDFOR

&& first dimension is default dimension if a multi dimensional array is passed
?ASum(@laTest)
?AMin(@laTest)
?AMax(@laTest)
?AAverage(@laTest)

&& second optional parameter specifies dimension 
?ASum(@laTest,2)
?AMin(@laTest,2)
?AMax(@laTest,2)
?AAverage(@laTest,2)

&& if second parameter is 0 the functions operate on all elements of the array
?ASum(@laTest,0)
?AMin(@laTest,0)
?AMax(@laTest,0)
?AAverage(@laTest,0)




