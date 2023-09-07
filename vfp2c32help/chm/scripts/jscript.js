if ( window.addEventListener ) { 
     window.addEventListener( "load", setvfp2c32version, false );
} else if ( window.attachEvent ) { 
        window.attachEvent( "onload", setvfp2c32version );
} else if ( window.onLoad ) {
		window.onload = setvfp2c32version;
}

function setvfp2c32version()
{
	var version = document.getElementById('vfp2c32version');
	version.innerText = 'VFP2C32 2.0.0.38';
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

