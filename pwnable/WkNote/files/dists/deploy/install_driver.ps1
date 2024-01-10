if ((Get-WmiObject Win32_PNPEntity).PNPClass.contains("SECCON Devices")){
	exit
}

if (!([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole("Administrators")){
	Start-Process powershell.exe -ArgumentList "-ExecutionPolicy Bypass -File `"$PSCommandPath`"" -Verb RunAs
	exit
}

# $distPath = Split-Path -Parent $MyInvocation.MyCommand.Path
$distPath = $PSScriptRoot

# Start-Process devgen.exe -ArgumentList "/add /bus ROOT /hardwareid Root\WkNote" -WorkingDirectory $distPath -wait
# Start-Process pnputil.exe -ArgumentList "/add-driver ..\driver\WkNote.inf /install" -WorkingDirectory $distPath

# I don't know why the first installation fails
Start-Process devcon.exe -ArgumentList "install ..\driver\WkNote.inf Root\WkNote" -WorkingDirectory $distPath -wait
Start-Process pnputil.exe -ArgumentList "/remove-device ROOT\SECCON_DEVICES\0000" -wait
Start-Process devcon.exe -ArgumentList "install ..\driver\WkNote.inf Root\WkNote" -WorkingDirectory $distPath -wait

Restart-Computer
