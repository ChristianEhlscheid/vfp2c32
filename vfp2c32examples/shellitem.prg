#INCLUDE vfp2c.h

LOCAL lcPath
m.lcPath = FULLPATH(JUSTPATH(SYS(16)))
CD (m.lcPath)

IF TYPE('_WIN64') = 'L' AND _WIN64
SET LIBRARY TO vfp2c64.fll ADDITIVE
ELSE
SET LIBRARY TO vfp2c32.fll ADDITIVE
ENDIF

LOCAL loItem, loDescList, loDesc, lcProperty, lPropValue, loStore

m.loItem = SHGetShellItem(ADDBS(m.lcPath) + 'test2.jpg')
m.loStore = m.loItem.GetPropertyStore() && IPropertyStore - when accessing more than one property using the store is more efficient, otherwise the file is opened and closed for each property access
m.loDescList = m.loItem.GetPropertyDescriptionList() && IPropertyDescriptionList
FOR EACH m.loDesc IN m.loDescList
	m.lcProperty = m.loDesc.GetCanonicalName() && m.loDesc -> IPropertyDescription
	m.lPropValue = m.loStore.GetValue(m.lcProperty)
	? m.lcProperty, TYPE('m.lPropValue'), m.lPropValue
ENDFOR

LOCAL laLatitude, laLongitude
m.laLatitude = m.loStore.GetValue('System.GPS.Latitude')
?m.laLatitude[1]
?m.laLatitude[2]
?m.laLatitude[3]
m.laLongitude= m.loStore.GetValue('System.GPS.Longitude')
?m.laLongitude[1]
?m.laLongitude[2]
?m.laLongitude[3]
? m.loStore.GetValue('System.GPS.Altitude')
*!*	lPropValue = m.loItem.GetProperty('System.GPS.DestLatitude')
*!*	? lPropValue
*!*	lPropValue = m.loItem.GetProperty('System.GPS.DestLatitudeDenominator')
*!*	? lPropValue
*!*	lPropValue = m.loItem.GetProperty('System.GPS.DestLatitudeNumerator')
*!*	? lPropValue
*!*	lPropValue = m.loItem.GetProperty('System.GPS.DestLatitudeRef')
*!*	? lPropValue
*!*	lPropValue = m.loItem.GetProperty('System.GPS.DestLongitude')
*!*	? lPropValue
*!*	lPropValue = m.loItem.GetProperty('System.GPS.DestLongitudeDenominator')
*!*	? lPropValue
*!*	lPropValue = m.loItem.GetProperty('System.GPS.DestLongitudeNumerator')
*!*	? lPropValue
*!*	lPropValue = m.loItem.GetProperty('System.GPS.DestLongitudeRef')
*!*	? lPropValue