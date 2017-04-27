document.write('<script id="ieScriptLoad" defer src="//:"><\/script>');
document.getElementById('ieScriptLoad').onreadystatechange = function() {
	if (this.readyState == 'complete') {
		domReady();
	}
}

function domReady()
{
	var version = document.getElementById('vfpsrvhostversion');
	if (version)
		version.innerText = 'VFPSrvHost 1.0.0';
}			
			
function CopyCode(cElement)
{
	var oElement = document.getElementById(cElement);
	window.clipboardData.setData("Text", oElement.innerText);
}

function ChangeClass(oElement, cClass)
{
	oElement.className = cClass;
}

