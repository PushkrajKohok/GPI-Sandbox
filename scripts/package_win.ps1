param([string]$Preset="release")
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$repo = Resolve-Path "$root\.."
$out  = Join-Path $repo "out\$Preset"
$dist = Join-Path $repo "dist\win"
New-Item -Force -ItemType Directory $dist | Out-Null

$bundle = Join-Path $dist "gpi-sandbox-$Preset-win64"
New-Item -Force -ItemType Directory $bundle | Out-Null

Copy-Item "$out\gpi_host.exe" $bundle
Copy-Item "$out\gpi_child.exe" $bundle
Copy-Item "$out\plugins\bin\*" (Join-Path $bundle "plugins\bin") -Recurse -Force
Copy-Item "$repo\config\*.toml" (Join-Path $bundle "config") -Force
Copy-Item "$repo\scripts\run_demo.ps1" $bundle -Force
Copy-Item "$repo\README.md" $bundle -Force

Compress-Archive -Path "$bundle\*" -DestinationPath "$bundle.zip" -Force
Write-Host "Packaged: $bundle.zip"
