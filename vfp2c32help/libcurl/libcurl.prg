********************************************************************************
*!* libcurl.prg
********************************************************************************
*!* This is the main file for the libcurl class. This gets called by
*!* libcurl class in the Init event.

#Include libcurl.h

Local ;
	m.lcThisName As String, ;
	m.lcThisPath As String

*!* Get this file name and path:
m.lcThisName = Sys(16)

*!* get this file path
m.lcThisPath = Addbs(Justpath(m.lcThisName))

*!* Add file path to VFP path
If Empty(Set("Path")) Then
	Set Path To (m.lcThisPath )
Else
	If Not m.lcThisPath $ Set("Path") Then
		Set Path To (Set("Path") + ";" + m.lcThisPath)
	Endif
Endif

Set Procedure To (m.lcThisName) Additive

*!* Add libcurl.vcx to classes
Set Classlib To (Forceext(m.lcThisName, "vcx")) Additive

*!* since libcurl.dll cannot find its supporting dlls if they are not in the Windows path,
*!* even if they are in the same folder as libcurl.dll, let's just load them here then.
LoadDll(m.lcThisPath + "libeay32.dll")
LoadDll(m.lcThisPath + "libssl32.dll")

*!* Load vfp2c32:
Do vfp2c32.prg

Return

********************************************************************************
*!* LoadDll(cDllFileName)
********************************************************************************
Procedure LoadDll
	Lparameters pcDllFileName
	Local lnHandle As Integer

	m.lnHandle = apiGetModuleHandle(m.pcDllFileName)

	If m.lnHandle  = 0 Then
		m.lnHandle = apiLoadLibrary(m.pcDllFileName)
	Endif

	Return m.lnHandle
Endproc

*!* libcurl Api functions

********************************************************************************
*!* curl_easy_cleanup - End a libcurl easy session
********************************************************************************
*!*	This function must be the last function to call for an easy session. It is the
*!*	opposite of the curl_easy_init(3) function and must be called with the same
*!*	handle as input that the curl_easy_init call returned.

*!*	This will effectively close all connections this handle has used and possibly has
*!*	kept open until now. Don't call this function if you intend to transfer more files.

*!*	Any uses of the handle after this function has been called are illegal. This
*!*	kills the handle and all memory associated with it!

*!* RETURN VALUE: None

*!* void curl_easy_cleanup(CURL * handle )
********************************************************************************

Function apiCurlEasyCleanup
	Lparameters nHandle
	Declare curl_easy_cleanup In libcurl.Dll As apiCurlEasyCleanup ;
		Integer nHandle
	Return apiCurlEasyCleanup(m.nHandle)
Endfunc

********************************************************************************
*!* curl_easy_escape - URL encodes the given string
********************************************************************************
*!*	This function converts the given input string to an URL encoded string and
*!*	returns that as a new allocated string. All input characters that are not a-z,
*!*	A-Z or 0-9 are converted to their "URL escaped" version (%NN where NN is a
*!*	two-digit hexadecimal number).

*!*	If the length argument is set to 0 (zero), curl_easy_escape(3) uses strlen() on
*!*	the input url to find out the size.

*!*	You must curl_free(3) the returned string when you're done with it.

*!* RETURN VALUE: A pointer to a zero terminated string or NULL if it failed.

*!* char *curl_easy_escape( CURL * curl , char * url , int length )
********************************************************************************

Function apiCurlEasyEscape
	Lparameters nHandle, cUrl, nLen
	Declare Integer curl_easy_escape In libcurl.Dll As apiCurlEasyEscape ;
		Integer nHandle, ;
		String  cUrl, ;
		Integer nLen
	Return apiCurlEasyEscape(m.nHandle, m.cUrl, m.nLen)
Endfunc

********************************************************************************
*!*	curl_easy_getinfo - extract information from a curl handle
********************************************************************************
*!*	Request internal information from the curl session with this function. The third
*!*	argument MUST be a pointer to a long, a pointer to a char *, a pointer to a struct
*!*	curl_slist * or a pointer to a double (as this documentation describes further down).
*!*	The data pointed-to will be filled in accordingly and can be relied upon only if
*!*	the function returns CURLE_OK. Use this function AFTER a performed transfer if
*!*	you want to get transfer- oriented data.

*!*	You should not free the memory returned by this function unless it is explicitly
*!*	mentioned below.

*!* RETURN VALUE: If the operation was successful, CURLE_OK is returned.
*!* Otherwise an appropriate error code will be returned.

*!*	CURLcode curl_easy_getinfo(CURL *curl, CURLINFO info, ... )
********************************************************************************

Function apiCurlEasyGetInfoInteger
	Lparameters nHandle, nInfo, nParam
	Declare Integer curl_easy_getinfo In libcurl.Dll As apiCurlEasyGetInfoInteger;
		Integer nHandle, ;
		Integer nInfo, ;
		Integer @nParam
	Return apiCurlEasyGetInfoInteger(m.nHandle ,m.nInfo ,@m.nParam)
Endfunc

Function apiCurlEasyGetInfoDouble
	Lparameters nHandle, nInfo, nParam
	Declare Integer curl_easy_getinfo In libcurl.Dll As apiCurlEasyGetInfoDouble;
		Integer nHandle, ;
		Integer nInfo, ;
		Double @nParam
	Return apiCurlEasyGetInfoDouble(m.nHandle ,m.nInfo ,@m.nParam)
Endfunc

Function apiCurlEasyGetInfoString
	Lparameters nHandle, nInfo, cParam
	Declare Integer curl_easy_getinfo In libcurl.Dll As apiCurlEasyGetInfoString;
		Integer nHandle, ;
		Integer nInfo, ;
		String  @cParam
	Return apiCurlEasyGetInfoString(m.nHandle ,m.nInfo ,@m.cParam)
Endfunc

********************************************************************************
*!* curl_easy_init - Start a libcurl easy session
********************************************************************************
*!*	This function must be the first function to call, and it returns a CURL easy
*!*	handle that you must use as input to other easy-functions. curl_easy_init
*!*	initializes curl and this call MUST have a corresponding call to
*!*	curl_easy_cleanup(3) when the operation is complete.

*!*	If you did not already call curl_global_init(3), curl_easy_init(3) does it
*!*	automatically. This may be lethal in multi-threaded cases, since
*!*	curl_global_init(3) is not thread-safe, and it may result in resource problems
*!*	because there is no corresponding cleanup.

*!*	You are strongly advised to not allow this automatic behaviour, by calling
*!*	curl_global_init(3) yourself properly. See the description in libcurl(3) of
*!*	global environment requirements for details of how to use this function.

*!* RETURN VALUE: If this function returns NULL, something went wrong and you
*!* cannot use the other curl functions.

*!* CURL *curl_easy_init( )
********************************************************************************

Function apiCurlEasyInit
	Declare Integer curl_easy_init In libcurl.Dll As apiCurlEasyInit
	Return apiCurlEasyInit()
Endfunc

********************************************************************************
*!*	curl_easy_perform - Perform a file transfer
********************************************************************************
*!*	This function is called after the init and all the curl_easy_setopt(3) calls are
*!*	made, and will perform the transfer as described in the options. It must be
*!*	called with the same handle as input as the curl_easy_init call returned.

*!*	You can do any amount of calls to curl_easy_perform(3) while using the same
*!*	handle. If you intend to transfer more than one file, you are even encouraged
*!*	to do so. libcurl will then attempt to re-use the same connection for the
*!*	following transfers, thus making the operations faster, less CPU intense and
*!*	using less network resources. Just note that you will have to use
*!*	curl_easy_setopt(3) between the invokes to set options for the following
*!*	curl_easy_perform.

*!*	You must never call this function simultaneously from two places using the same
*!*	handle. Let the function return first before invoking it another time. If you
*!*	want parallel transfers, you must use several curl handles.

*!* RETURN VALUE: 0 means everything was ok, non-zero means an error occurred as
*!*	<curl/curl.h> defines. If the CURLOPT_ERRORBUFFER was set with curl_easy_setopt
*!*	there will be a readable error message in the error buffer when non-zero is returned.

*!* CURLcode curl_easy_perform(CURL * handle )
********************************************************************************

Function apiCurlEasyPerform
	Lparameters nHandle
	Declare Integer curl_easy_perform In libcurl.Dll As apiCurlEasyPerform ;
		Integer nHandle
	Return apiCurlEasyPerform(m.nHandle)
Endfunc

********************************************************************************
*!* curl_easy_reset - reset all options of a libcurl session handle
********************************************************************************
*!*	Re-initializes all options previously set on a specified CURL handle to the
*!*	default values. This puts back the handle to the same state as it was in when
*!*	it was just created with curl_easy_init(3).

*!*	It does not change the following information kept in the handle: live connections,
*!*	the Session ID cache, the DNS cache, the cookies and shares.

*!* RETURN VALUE: Nothing

*!* void curl_easy_reset(CURL *handle )
********************************************************************************

Function apiCurlEasyReset
	Lparameters nHandle
	Declare curl_easy_reset In libcurl.Dll As apiCurlEasyReset ;
		Integer nHandle
	Return apiCurlEasyReset(m.nHandle)
Endfunc

********************************************************************************
*!* curl_easy_setopt - set options for a curl easy handle
********************************************************************************
*!*	curl_easy_setopt() is used to tell libcurl how to behave. By using the appropriate
*!*	options to curl_easy_setopt, you can change libcurl's behavior. All options are
*!*	set with the option followed by a parameter. That parameter can be a long, a
*!*	function pointer, an object pointer or a curl_off_t, depending on what the specific
*!*	option expects. Read this manual carefully as bad input values may cause libcurl
*!*	to behave badly! You can only set one option in each function call. A typical
*!*	application uses many curl_easy_setopt() calls in the setup phase.

*!*	Options set with this function call are valid for all forthcoming transfers
*!*	performed using this handle. The options are not in any way reset between
*!*	transfers, so if you want subsequent transfers with different options, you must
*!*	change them between the transfers. You can optionally reset all options back to
*!*	internal default with curl_easy_reset(3).

*!*	Strings passed to libcurl as 'char *' arguments, are copied by the library; thus
*!*	the string storage associated to the pointer argument may be overwritten after
*!*	curl_easy_setopt() returns. Exceptions to this rule are described in the option
*!*	details below.

*!*	NOTE: before 7.17.0 strings were not copied. Instead the user was forced keep them
*!*	available until libcurl no longer needed them.

*!*	The handle is the return code from a curl_easy_init(3) or curl_easy_duphandle(3) call.

*!* RETURN VALUE: CURLE_OK (zero) means that the option was set properly, non-zero
*!*	means an error occurred as <curl/curl.h> defines. See the libcurl-errors(3) man page
*!*	for the full list with descriptions.

*!*	If you try to set an option that libcurl doesn't know about, perhaps because the
*!*	library is too old to support it or the option was removed in a recent version,
*!*	this function will return CURLE_FAILED_INIT.

*!* CURLcode curl_easy_setopt(CURL *handle, CURLoption option, parameter)
********************************************************************************

Function apiCurlEasySetOptInteger
	Lparameters nHandle, Option, nParam
	Declare Integer curl_easy_setopt In libcurl.Dll As apiCurlEasySetOptInteger ;
		Integer nHandle, ;
		Integer Option, ;
		Integer nParam
	Return apiCurlEasySetOptInteger(m.nHandle, m.Option, m.nParam)
Endfunc

Function apiCurlEasySetOptInt64
	Lparameters nHandle, Option, nParamLo, nParamHi
	Declare Integer curl_easy_setopt In libcurl.Dll As apiCurlEasySetOptInt64 ;
		Integer nHandle, ;
		Integer Option, ;
		Integer nParamLo, ;
		Integer nParamHi
	Return apiCurlEasySetOptInt64(m.nHandle, m.Option, m.nParamLo, m.nParamHi)
Endfunc

Function apiCurlEasySetOptString
	Lparameters nHandle, Option, cParam
	Declare Integer curl_easy_setopt In libcurl.Dll As apiCurlEasySetOptString ;
		Integer nHandle, ;
		Integer Option, ;
		String  cParam
	Return apiCurlEasySetOptString(m.nHandle, m.Option, m.cParam)
Endfunc

********************************************************************************
*!*	curl_easy_strerror - return string describing error code
********************************************************************************
*!*	The curl_easy_strerror() function returns a string describing the CURLcode error
*!*	code passed in the argument errornum.

*!* RETURN VALUE: A pointer to a zero terminated string.

*!* const char *curl_easy_strerror(CURLcode  errornum )
********************************************************************************

Function apiCurlEasyStrError
	Lparameters ErrorNum
	Declare String curl_easy_strerror In libcurl.Dll As apiCurlEasyStrError ;
		Integer ErrorNum
	Return apiCurlEasyStrError(m.ErrorNum)
Endfunc

********************************************************************************
*!* curl_easy_unescape - URL decodes the given string
********************************************************************************
*!*	This function converts the given URL encoded input string to a "plain string"
*!*	and returns that in an allocated memory area. All input characters that are URL
*!*	encoded (%XX where XX is a two-digit hexadecimal number) are converted to their
*!*	binary versions.

*!*	If the length argument is set to 0 (zero), curl_easy_unescape(3) will use
*!*	strlen() on the input url string to find out the size.

*!*	If outlength is non-NULL, the function will write the length of the returned
*!*	string in the integer it points to. This allows an escaped string containing %00
*!*	to still get used properly after unescaping.

*!*	You must curl_free(3) the returned string when you're done with it.

*!* RETURN VALUE: A pointer to a zero terminated string or NULL if it failed.

*!* char *curl_easy_unescape( CURL * curl , char * url , int inlength , int * outlength )
********************************************************************************

Function apiCurlEasyUnEscape
	Lparameters nHandle, cUrl, InLen, OutLen
	Declare Integer curl_easy_unescape In libcurl.Dll As apiCurlEasyUnEscape ;
		Integer nHandle, ;
		String  cUrl, ;
		Integer InLen, ;
		Integer @OutLen
	Return apiCurlEasyUnEscape(m.nHandle, m.cUrl, m.InLen, @m.OutLen)
Endfunc

********************************************************************************
*!*	curl_free - reclaim memory that has been obtained through a libcurl call
********************************************************************************
*!*	curl_free reclaims memory that has been obtained through a libcurl call. Use
*!*	curl_free() instead of free() to avoid anomalies that can result from differences
*!*	in memory management between your application and libcurl.

*!* RETURN VALUE: None

*!*	void curl_free( char * ptr )
********************************************************************************

Function apiCurlFree
	Lparameters nPointer
	Declare curl_free In libcurl.Dll As apiCurlFree ;
		Integer nPointer
	Return apiCurlFree (m.nPointer)
Endfunc

********************************************************************************
*!* curl_getdate - Convert an date string to number of seconds since January 1, 1970
********************************************************************************
*!*	This function returns the number of seconds since January 1st 1970 in the UTC
*!*	time zone, for the date and time that the datestring parameter specifies. The
*!*	now parameter is not used, pass a NULL there.

*!*	NOTE: This function was rewritten for the 7.12.2 release and this documentation
*!*	covers the functionality of the new one. The new one is not feature-complete with
*!*	the old one, but most of the formats supported by the new one was supported by
*!*	the old too.

*!* RETURN VALUE: This function returns -1 when it fails to parse the date string.
*!*	Otherwise it returns the number of seconds as described.

*!*	If the year is larger than 2037 on systems with 32 bit time_t, this function will
*!*	return 0x7fffffff (since that is the largest possible signed 32 bit number).

*!*	Having a 64 bit time_t is not a guarantee that dates beyond 03:14:07 UTC,
*!*	January 19, 2038 will work fine. On systems with a 64 bit time_t but with a
*!*	crippled mktime(), curl_getdate will return -1 in this case.

*!* time_t curl_getdate(char * datestring , time_t *now )
********************************************************************************

Function apiCurlGetDate
	Lparameters cDateString, nNow
	Declare Integer curl_getdate In libcurl.Dll As apiCurlGetDate ;
		String  cDateString, ;
		Integer nNow
	Return apiCurlGetDate(m.cDateString, m.nNow)
Endfunc

********************************************************************************
*!* curl_global_cleanup - global libcurl cleanup
********************************************************************************
*!*	This function releases resources acquired by curl_global_init(3).

*!*	You should call curl_global_cleanup(3) once for each call you make to
*!*	curl_global_init(3), after you are done using libcurl.

*!*	This function is not thread safe. You must not call it when any other thread in
*!*	the program (i.e. a thread sharing the same memory) is running. This doesn't just
*!*	mean no other thread that is using libcurl. Because curl_global_cleanup(3) calls
*!*	functions of other libraries that are similarly thread unsafe, it could conflict
*!*	with any other thread that uses these other libraries.

*!*	See the description in libcurl(3) of global environment requirements for details
*!*	of how to use this function.

*!* RETURN VALUE: None

*!* void curl_global_cleanup(void)
********************************************************************************

Function apiCurlGlobalCleanup
	Declare curl_global_cleanup In libcurl.Dll As apiCurlGlobalCleanup
	Return apiCurlGlobalCleanup()
Endfunc

********************************************************************************
*!* curl_global_init - Global libcurl initialisation
********************************************************************************
*!*	This function sets up the program environment that libcurl needs. Think of it
*!*	as an extension of the library loader.

*!*	This function must be called at least once within a program (a program is all
*!*	the code that shares a memory space) before the program calls any other function
*!*	in libcurl. The environment it sets up is constant for the life of the program
*!*	and is the same for every program, so multiple calls have the same effect as
*!*	one call.

*!*	The flags option is a bit pattern that tells libcurl exactly what features to
*!*	init, as described below. Set the desired bits by ORing the values together.
*!*	In normal operation, you must specify CURL_GLOBAL_ALL. Don't use any other value
*!*	unless you are familiar with and mean to control internal operations of libcurl.

*!*	This function is not thread safe. You must not call it when any other thread in
*!*	the program (i.e. a thread sharing the same memory) is running. This doesn't just
*!*	mean no other thread that is using libcurl. Because curl_global_init() calls
*!*	functions of other libraries that are similarly thread unsafe, it could conflict
*!*	with any other thread that uses these other libraries.

*!*	See the description in libcurl(3) of global environment requirements for details
*!*	of how to use this function.

*!* RETURN VALUE: If this function returns non-zero, something went wrong and you
*!* cannot use the other curl functions.

*!* CURLcode curl_global_init(long flags )
********************************************************************************

Function apiCurlGlobalInit
	Lparameters nFlags
	Declare Integer curl_global_init In libcurl.Dll As apiCurlGlobalInit ;
		Integer nFlags
	Return apiCurlGlobalInit(nFlags)
Endfunc

********************************************************************************
*!* curl_multi_add_handle - add an easy handle to a multi session
********************************************************************************
*!*	Adds a standard easy handle to the multi stack. This function call will make this
*!*	multi_handle control the specified easy_handle. Furthermore, libcurl now initiates
*!*	the connection associated with the specified easy_handle.

*!*	When an easy handle has been added to a multi stack, you can not and you must not
*!*	use curl_easy_perform(3) on that handle!

*!*	If the easy handle is not set to use a shared (CURLOPT_SHARE) or global DNS cache
*!*	(CURLOPT_DNS_USE_GLOBAL_CACHE), it will be made to use the DNS cache that is shared
*!*	between all easy handles within the multi handle when curl_multi_add_handle(3)
*!*	is called.

*!*	The easy handle will remain added until you remove it again with curl_multi_remove_handle(3).
*!*	 You should remove the easy handle from the multi stack before you terminate first
*!*	 the easy handle and then the multi handle:

*!*	1 - curl_multi_remove_handle(3)
*!*	2 - curl_easy_cleanup(3)
*!*	3 - curl_multi_cleanup(3)

*!* RETURN VALUE: CURLMcode type, general libcurl multi interface error code.

*!* CURLMcode curl_multi_add_handle(CURLM *multi_handle, CURL *easy_handle)
********************************************************************************

Function apiCurlMultiAddHandle
	Lparameters MultiHandle, EasyHandle
	Declare Integer curl_multi_add_handle In libcurl.Dll As apiCurlMultiAddHandle ;
		Integer MultiHandle, ;
		Integer EasyHandle
	Return apiCurlMultiAddHandle(m.MultiHandle, m.EasyHandle)
Endfunc

********************************************************************************
*!* curl_multi_cleanup - close down a multi session
********************************************************************************
*!*	Cleans up and removes a whole multi stack. It does not free or touch any
*!*	individual easy handles in any way - they still need to be closed individually,
*!*	using the usual curl_easy_cleanup(3) way. The order of cleaning up should be:

*!*	1 - curl_multi_remove_handle(3) before any easy handles are cleaned up
*!*	2 - curl_easy_cleanup(3) can now be called independently since the easy handle
*!*	    is no longer connected to the multi handle
*!*	3 - curl_multi_cleanup(3) should be called when all easy handles are removed

*!* RETURN VALUE: CURLMcode type, general libcurl multi interface error code.

*!* CURLMcode curl_multi_cleanup( CURLM *multi_handle )
********************************************************************************

Function apiCurlMultiCleanup
	Lparameters MultiHandle
	Declare Integer curl_multi_cleanup In libcurl.Dll As apiCurlMultiCleanup ;
		Integer MultiHandle
	Return apiCurlMultiCleanup(m.MultiHandle)
Endfunc

********************************************************************************
*!* curl_multi_info_read - read multi stack informationals
********************************************************************************
*!*	Ask the multi handle if there are any messages/informationals from the individual
*!*	transfers. Messages may include informationals such as an error code from the
*!*	transfer or just the fact that a transfer is completed. More details on these
*!*	should be written down as well.

*!*	Repeated calls to this function will return a new struct each time, until a NULL
*!*	is returned as a signal that there is no more to get at this point. The integer
*!*	pointed to with msgs_in_queue will contain the number of remaining messages after
*!*	this function was called.

*!*	When you fetch a message using this function, it is removed from the internal queue
*!*	so calling this function again will not return the same message again. It will
*!*	instead return new messages at each new invoke until the queue is emptied.

*!*	WARNING: The data the returned pointer points to will not survive calling
*!*	curl_multi_cleanup(3), curl_multi_remove_handle(3) or curl_easy_cleanup(3).

*!*	The 'CURLMsg' struct is very simple and only contain very basic information.
*!*	If more involved information is wanted, the particular "easy handle" in present
*!*	in that struct and can thus be used in subsequent regular curl_easy_getinfo(3)
*!*	calls (or similar):

*!*	 struct CURLMsg {
*!*	 CURLMSG msg; /* what this message means */
*!*	 CURL *easy_handle; /* the handle it concerns */
*!*	 union {
*!*	 void *whatever; /* message-specific data */
*!*	 CURLcode result; /* return code for transfer */
*!*	 }
*!*	 data;
*!*	 };

*!*	When msg is CURLMSG_DONE, the message identifies a transfer that is done,
*!*	and then result contains the return code for the easy handle that just completed.
*!*	At This Point, there Is no Other msg types defined

*!* RETURN VALUE: A pointer to a filled-in struct, or NULL if it failed or ran
*!*	out of structs. It also writes the number of messages left in the queue
*!*	(after this read) in the integer the second argument points to.

*!* CURLMsg *curl_multi_info_read( CURLM *multi_handle,   int *msgs_in_queue)
********************************************************************************

Function apiCurlMultiInfoRead
	Lparameters MultiHandle, MsgsInQueue
	Declare Integer curl_multi_info_read In libcurl.Dll As apiCurlMultiInfoRead ;
		Integer MultiHandle, ;
		Integer @MsgsInQueue
	Return apiCurlMultiInfoRead(m.MultiHandle, @m.MsgsInQueue)
Endfunc

********************************************************************************
*!* curl_multi_init - create a multi handle
********************************************************************************
*!*	This function returns a CURLM handle to be used as input to all the other
*!*	multi-functions, sometimes referred to as a multi handle on some places in the
*!*	documentation. This init call MUST have a corresponding call to
*!*	curl_multi_cleanup(3) when the operation is complete.

*!* RETURN VALUE: If this function returns NULL, something went wrong and you
*!* Cannot use the other curl functions.

*!* CURLM *curl_multi_init( )
********************************************************************************

Function apiCurlMultiInit
	Declare Integer curl_multi_init In libcurl.Dll As apiCurlMultiInit
	Return apiCurlMultiInit()
Endfunc

********************************************************************************
*!* curl_multi_perform - reads/writes available data from each easy handle
********************************************************************************
*!*	When the app thinks there's data available for the multi_handle, it should call
*!*	this function to read/write whatever there is to read or write right now.
*!*	curl_multi_perform() returns as soon as the reads/writes are done. This function
*!*	does not require that there actually is any data available for reading or that
*!*	data can be written, it can be called just in case. It will write the number of
*!*	handles that still transfer data in the second argument's integer-pointer.

*!*	When you call curl_multi_perform() and the amount of running_handles is changed
*!*	from the previous call (or is less than the amount of easy handles you've added
*!*	to the multi handle), you know that there is one or more transfers less "running".
*!*	You can then call curl_multi_info_read(3) to get information about each individual
*!*	completed transfer, and that returned info includes CURLcode and more.

*!* RETURN VALUE:
*!*	CURLMcode type, general libcurl multi interface error code.

*!*	If you receive CURLM_CALL_MULTI_PERFORM, this basically means that you should call
*!*	curl_multi_perform again, before you select() on more actions. You don't have to do
*!*	it immediately, but the return code means that libcurl may have more data available
*!*	to return or that there may be more data to send off before it is "satisfied". Do
*!*	note that curl_multi_perform(3) will return CURLM_CALL_MULTI_PERFORM only when it
*!*	wants to be called again immediately. When things are fine and there are nothing
*!*	immediate it wants done, it'll return CURLM_OK and you need to wait for "action" and
*!*	then call this function again.

*!*	NOTE that this only returns errors etc regarding the whole multi stack. There might
*!*	still have occurred problems on individual transfers even when this function returns
*!*	CURLM_OK.

*!* CURLMcode curl_multi_perform(CURLM *multi_handle, int *running_handles)
********************************************************************************

Function apiCurlMultiPerform
	Lparameters MultiHandle, RunningHandles
	Declare Integer curl_multi_perform In libcurl.Dll As apiCurlMultiPerform ;
		Integer MultiHandle, ;
		Integer @RunningHandles
	Return apiCurlMultiPerform(m.MultiHandle, @m.RunningHandles)
Endfunc

Function apiCurlMultiRemoveHandle
	Lparameters MultiHandle, EasyHandle
	Declare Integer curl_multi_remove_handle In libcurl.Dll As apiCurlMultiRemoveHandle ;
		Integer MultiHandle, ;
		Integer EasyHandle
	Return apiCurlMultiRemoveHandle(m.MultiHandle, m.EasyHandle)
Endfunc

Function apiCurlMultiSetOptInteger
	Lparameters MultiHandle, Option, nParam
	Declare Integer curl_multi_setopt In libcurl.Dll As apiCurlMultiSetOptInteger ;
		Integer MultiHandle, ;
		Integer Option, ;
		Integer nParam
	Return apiCurlMultiSetOptInteger(m.MultiHandle, m.Option, m.nParam)
Endproc

Function apiCurlMultiStrError
	Lparameters ErrorNum
	Declare String curl_multi_strerror In libcurl.Dll As apiCurlMultiStrError ;
		Integer ErrorNum
	Return apiCurlMultiStrError(m.ErrorNum)
Endfunc

Function apiCurlMultiTimeOut
	Lparameters MultiHandle, nTimeOut
	Declare Integer curl_multi_timeout In libcurl.Dll As apiCurlMultiTimeOut ;
		Integer MultiHandle, ;
		Integer @nTimeOut
	Return apiCurlMultiTimeOut(m.MultiHandle, @m.nTimeOut)
Endfunc

********************************************************************************
*!* curl_slist_append - add a string to an slist
********************************************************************************
*!*	curl_slist_append() appends a specified string to a linked list of strings.
*!*	The existing list should be passed as the first argument while the new list is
*!*	returned from this function. The specified string has been appended when this
*!*	function returns. curl_slist_append() copies the string.

*!*	The list should be freed again (after usage) with curl_slist_free_all(3).

*!*	RETURN VALUE :A null pointer is returned if anything went wrong, otherwise the
*!*	new list pointer is returned.

*!* struct curl_slist *curl_slist_append(struct curl_slist * list, const char * string )
********************************************************************************

Function apiCurlSlistAppend
	Lparameters nList, cString
	Declare Integer curl_slist_append In libcurl.Dll As apiCurlSlistAppend ;
		Integer nList, ;
		String  cString
	Return apiCurlSlistAppend(m.nList, m.cString)
Endfunc

********************************************************************************
*!* curl_slist_free_all - free an entire curl_slist list
********************************************************************************
*!*	curl_slist_free_all() removes all traces of a previously built curl_slist linked list.

*!* RETURN VALUE: None

*!* void curl_slist_free_all(struct curl_slist * list)
********************************************************************************

Function apiCurlSlistFreeAll
	Lparameters nList
	Declare Integer curl_slist_free_all In libcurl.Dll As apiCurlSlistFreeAll ;
		Integer nList
	Return apiCurlSlistFreeAll(m.nList)
Endfunc

********************************************************************************
*!* curl_version - returns the libcurl version string
********************************************************************************
*!* Returns a human readable string with the version number of libcurl and some
*!*	of its important components (like OpenSSL version).

*!* RETURN VALUE: A pointer to a zero terminated string.

*!* char *curl_version( )
********************************************************************************

Function apiCurlVersion
	Declare String curl_version In libcurl.Dll As apiCurlVersion
	Return apiCurlVersion()
Endfunc

*!* Windows Api functions

Function apiCloseHandle
	Lparameters hObject
	Declare Integer CloseHandle In win32api As apiCloseHandle ;
		Integer hObject
	Return apiCloseHandle(m.hObject)
Endfunc

Function apiCreateFile
	Lparameters lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile
	Declare Integer CreateFile In win32api As apiCreateFile ;
		String  lpFileName, ;
		Integer dwDesiredAccess, ;
		Integer dwShareMode, ;
		Integer lpSecurityAttributes, ;
		Integer dwCreationDisposition, ;
		Integer dwFlagsAndAttributes, ;
		Integer hTemplateFile
	Return apiCreateFile(m.lpFileName, m.dwDesiredAccess, m.dwShareMode, m.lpSecurityAttributes, m.dwCreationDisposition, m.dwFlagsAndAttributes, m.hTemplateFile)
Endfunc

Function apiLoadLibrary
	Lparameters lpLibFileName
	Declare Integer LoadLibrary In win32api As apiLoadLibrary;
		String  lpLibFileName
	Return apiLoadLibrary(m.lpLibFileName)
Endfunc

Function apiGetFileSizeEx
	Lparameters hFile, lpFileSize
	Declare Integer GetFileSizeEx In win32api As apiGetFileSizeEx ;
		Integer hFile, ;
		String  @lpFileSize
	Return apiGetFileSizeEx(m.hFile, @m.lpFileSize)
Endfunc

Function apiGetModuleHandle
	Lparameters lpModule
	Declare Integer GetModuleHandle In win32api As apiGetModuleHandle ;
		String  lpModule
	Return apiGetModuleHandle(m.lpModule)
Endfunc

Function apiGetTickCount()
	Declare Integer GetTickCount In win32api As apiGetTickCount
	Return apiGetTickCount()
Endfunc

Function apiReadFile
	Lparameters hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped
	Declare Integer ReadFile In win32api As apiReadFile ;
		Integer hFile, ;
		String  @lpBuffer, ;
		Integer nNumberOfBytesToRead, ;
		Integer @lpNumberOfBytesRead, ;
		Integer lpOverlapped
	Return apiReadFile(m.hFile, @m.lpBuffer, m.nNumberOfBytesToRead, @m.lpNumberOfBytesRead, m.lpOverlapped)
Endfunc

Function apiSetFilePointerEx
	Lparameters hFile, DistanceToMoveLo, DistanceToMoveHi, NewFilePointer, MoveMethod
	Declare Integer SetFilePointerEx In win32api As apiSetFilePointerEx ;
		Integer hFile, ;
		Integer DistanceToMoveLo, ;
		Integer DistanceToMoveHi, ;
		String  @NewFilePointer, ;
		Integer dwMoveMethod
	Return apiSetFilePointerEx(m.hFile, m.DistanceToMoveLo, m.DistanceToMoveHi, @m.NewFilePointer, m.MoveMethod)
Endfunc

Function apiWriteFile
	Lparameters hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped
	Declare Integer WriteFile In win32api As apiWriteFile ;
		Integer hFile, ;
		String  lpBuffer, ;
		Integer nNumberOfBytesToWrite, ;
		Integer @lpNumberOfBytesWritten, ;
		Integer lpOverlapped
	Return apiWriteFile(m.hFile, m.lpBuffer, m.nNumberOfBytesToWrite, @m.lpNumberOfBytesWritten, m.lpOverlapped)
Endfunc

********************************************************************************
*!* apiCurlReadFile
********************************************************************************
*!*	This declare uses an Integer instead of a String for lpBuffer, since libcurl 
*!*	already provides a pointer to the string to read.
********************************************************************************
Function apiCurlReadFile
	Lparameters hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped
	Declare Integer ReadFile In win32api As apiCurlReadFile;
		Integer hFile, ;
		Integer lpBuffer, ;
		Integer nNumberOfBytesToRead, ;
		Integer @lpNumberOfBytesRead, ;
		Integer lpOverlapped
	Return apiCurlReadFile(m.hFile, m.lpBuffer, m.nNumberOfBytesToRead, @m.lpNumberOfBytesRead, m.lpOverlapped)
Endfunc

********************************************************************************
*!* apiCurlWriteFile
********************************************************************************
*!*	This declare uses an Integer instead of a String for lpBuffer, since libcurl 
*!*	already provides a pointer to the string to write.
********************************************************************************
Function apiCurlWriteFile
	Lparameters hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped
	Declare Integer WriteFile In win32api As apiCurlWriteFile;
		Integer hFile, ;
		Integer lpBuffer, ;
		Integer nNumberOfBytesToWrite, ;
		Integer @lpNumberOfBytesWritten, ;
		Integer lpOverlapped
	Return apiCurlWriteFile(m.hFile, m.lpBuffer, m.nNumberOfBytesToWrite, @m.lpNumberOfBytesWritten, m.lpOverlapped)
Endfunc
