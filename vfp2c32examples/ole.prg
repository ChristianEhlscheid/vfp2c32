&& OLE/COM functions
&& prerequisites: none

CD (FULLPATH(JUSTPATH(SYS(16))))
SET LIBRARY TO vfp2c32.fll ADDITIVE

LOCAL lcClsID, lcReadableClsId
SET STEP ON
lcClsID = ClsIDFromProgID('VisualFoxPro.Application')
&& lcClsID now contains binary CLSID

&& binary ClsID to readable ClsID conversion and vice versa
lcReadableClsId = StringFromClsID(lcClsID)
? lcReadableClsId
? ClsIDFromString(lcReadableClsID) = lcClsID

&& the reverse, can take both binary or readable ClsID's as a parameter
?ProgIDFromClsID(lcClsID)
?ProgIDFromClsID(lcReadableClsId)

&& compare 2 GUID's, also can take mixed (binary or readable) Guid's in both parameters
?IsEqualGuid(lcClsID,lcReadableClsID)

&& create a new unique GUID
LOCAL lcGuid
lcGuid = CreateGuid() && or CreateGuid(0)
?lcGuid, 'Ansi'
lcGuid = CreateGuid(1)
?lcGuid, 'Unicode'
lcGuid = CreateGuid(2)
?lcGuid, 'Binary'


