$ErrorActionPreference = "Stop"

$Here = Split-Path -Parent $MyInvocation.MyCommand.Path
$Work = Join-Path $env:TEMP "packit-led-pattern-build"
$Zip = Join-Path $Work "ffgl.zip"
$Sdk = Join-Path $Work "ffgl-master"

Write-Host "[1/6] Checking Visual Studio Build Tools..." -ForegroundColor Cyan
$VsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $VsWhere)) {
    throw "Visual Studio 2022 or Build Tools 2022 was not found. Install the Desktop development with C++ workload first."
}
$MsBuild = & $VsWhere -latest -products * -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe | Select-Object -First 1
if (-not $MsBuild) {
    throw "MSBuild was not found. Add the Desktop development with C++ workload to Visual Studio 2022."
}

Write-Host "[2/6] Downloading official Resolume FFGL SDK..." -ForegroundColor Cyan
Remove-Item $Work -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Path $Work | Out-Null
Invoke-WebRequest "https://github.com/resolume/ffgl/archive/refs/heads/master.zip" -OutFile $Zip
Expand-Archive $Zip -DestinationPath $Work -Force

Write-Host "[3/6] Preparing PACK.IT LED Pattern project..." -ForegroundColor Cyan
python (Join-Path $Here "prepare_project.py") $Sdk

Write-Host "[4/6] Building 64-bit Release DLL..." -ForegroundColor Cyan
$Project = Join-Path $Sdk "build\windows\PackItLEDPattern.vcxproj"
& $MsBuild $Project /m /p:Configuration=Release /p:Platform=x64
if ($LASTEXITCODE -ne 0) { throw "Compilation failed with exit code $LASTEXITCODE." }

$Dll = Join-Path $Sdk "binaries\x64\Release\PackItLEDPattern.dll"
if (-not (Test-Path $Dll)) { throw "Build finished, but the DLL was not found at $Dll" }

Write-Host "[5/6] Installing into Resolume Extra Effects..." -ForegroundColor Cyan
$InstallDir = Join-Path ([Environment]::GetFolderPath("MyDocuments")) "Resolume\Extra Effects"
New-Item -ItemType Directory -Path $InstallDir -Force | Out-Null
Copy-Item $Dll (Join-Path $InstallDir "PackItLEDPattern.dll") -Force

Write-Host "[6/6] Done." -ForegroundColor Green
Write-Host "Installed: $(Join-Path $InstallDir 'PackItLEDPattern.dll')"
Write-Host "Open Resolume Arena, go to Sources and search for: PK LED Pattern"
