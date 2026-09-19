# Run in an x86 VS 2022 Developer PowerShell. No sound or visible window is opened.
param([string]$OutputDirectory = (Join-Path $env:TEMP 'Lev4k-feedback-probe'))
$ErrorActionPreference = 'Stop'
if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
    throw 'Run this script from an x86 VS 2022 Developer PowerShell with cl.exe on PATH.'
}
$projectRoot = Split-Path -Parent $PSScriptRoot
$probeOutput = [System.IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $probeOutput | Out-Null
Push-Location $projectRoot
try {
    foreach ($mipmaps in 0, 1) {
        $executable = Join-Path $probeOutput "feedback-$mipmaps.exe"
        $object = Join-Path $probeOutput "feedback-$mipmaps.obj"
        & cl.exe /nologo /EHsc /std:c++17 /D_CRT_SECURE_NO_WARNINGS "/DUSE_MIPMAPS=$mipmaps" "/Fe:$executable" "/Fo:$object" tests\feedback-probe.cpp user32.lib gdi32.lib opengl32.lib winmm.lib
        if ($LASTEXITCODE -ne 0) { throw "Probe compilation failed (USE_MIPMAPS=$mipmaps)." }
        & $executable
        if ($LASTEXITCODE -ne 0) { throw "GPU probe failed (USE_MIPMAPS=$mipmaps)." }
    }
} finally {
    Pop-Location
}
