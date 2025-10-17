$here = Split-Path -Parent $MyInvocation.MyCommand.Path
cd $here
.\gpi_host.exe --demo
