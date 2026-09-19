# Creates a new, independent working copy. Never writes into the main project's src/out.
param(
    [ValidateSet('drifting-shore', 'primordial-awakening', 'exe-gfx', 'bonzo-compute')]
    [string]$Name = 'drifting-shore',
    [string]$Destination,
    [ValidateSet('None', 'Editor', 'Snapshot', 'Release')]
    [string]$Build = 'None'
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$catalogue = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'manifest.json') -Raw | ConvertFrom-Json
$example = $catalogue.examples | Where-Object { $_.name -eq $Name }
if ($example.status -ne 'prepared-copy') {
    throw "$Name is a source reference requiring its own renderer. See examples/$Name/README.md."
}

if (-not $Destination) {
    $suffix = [guid]::NewGuid().ToString('N').Substring(0, 8)
    $Destination = Join-Path $PSScriptRoot "_work\$Name-$suffix"
}
$destinationPath = [System.IO.Path]::GetFullPath($Destination).TrimEnd('\', '/')
$rootPath = [System.IO.Path]::GetFullPath($projectRoot).TrimEnd('\', '/')
$workPrefix = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '_work')).TrimEnd('\', '/') + '\'
if ($destinationPath.Equals($rootPath, [StringComparison]::OrdinalIgnoreCase) -or
    ($destinationPath.StartsWith($rootPath + '\', [StringComparison]::OrdinalIgnoreCase) -and
     -not $destinationPath.StartsWith($workPrefix, [StringComparison]::OrdinalIgnoreCase))) {
    throw 'Choose a new directory outside the checkout, or below examples/_work/.'
}
if (Test-Path -LiteralPath $destinationPath) {
    throw "Destination already exists; nothing was overwritten: $destinationPath"
}

$upstream = Join-Path $PSScriptRoot "$Name\upstream"
foreach ($file in $example.files) {
    $source = Join-Path $upstream $file.path
    if ((Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash -ne $file.sha256) {
        throw "Upstream reference changed: $source. Edit a prepared copy instead."
    }
}

$utf8 = New-Object System.Text.UTF8Encoding($false)
function Replace-One([string]$Path, [string]$Pattern, [string]$Replacement) {
    $content = [System.IO.File]::ReadAllText($Path)
    if ([regex]::Matches($content, $Pattern).Count -ne 1) {
        throw "Expected one setting matching '$Pattern' in $Path. The host may need a revised example recipe."
    }
    [System.IO.File]::WriteAllText($Path, [regex]::Replace($content, $Pattern, $Replacement), $utf8)
}

New-Item -ItemType Directory -Path $destinationPath | Out-Null
foreach ($file in @('Lev4k.sln', 'Lev4k.vcxproj', 'Lev4k.vcxproj.filters', 'LICENSE', 'link.exe', 'crinkler-license.txt', 'crinkler-manual.txt', 'shader_minifier.exe')) {
    Copy-Item -LiteralPath (Join-Path $projectRoot $file) -Destination (Join-Path $destinationPath $file)
}
Copy-Item -LiteralPath (Join-Path $projectRoot 'src') -Destination $destinationPath -Recurse
Copy-Item -LiteralPath (Join-Path $upstream 'src\shaders\fragment.frag') -Destination (Join-Path $destinationPath 'src\shaders\fragment.frag')
Copy-Item -LiteralPath (Join-Path $upstream 'src\shaudio.h') -Destination (Join-Path $destinationPath 'src\shaudio.h')

# Drifting Shore needs mipmaps, 148 seconds of music, and its RGBA8 brightness encoding.
Replace-One (Join-Path $destinationPath 'src\main.cpp') '(?m)^#define USE_MIPMAPS[ \t]+0\r?$' '#define USE_MIPMAPS  1'
Replace-One (Join-Path $destinationPath 'src\main.cpp') '(?m)^#define RECORD_IMG_LENGTH[ \t]+145\r?$' '#define RECORD_IMG_LENGTH 148'
Replace-One (Join-Path $destinationPath 'src\feedback.h') 'GL_RGBA32F' 'GL_RGBA8'

# Compression is unattended in the prepared project; the main project is untouched.
$project = Join-Path $destinationPath 'Lev4k.vcxproj'
$xml = [System.IO.File]::ReadAllText($project).Replace('/PROGRESSGUI ', '')
[System.IO.File]::WriteAllText($project, $xml, $utf8)
$record = [ordered]@{
    example = $Name
    upstreamRepository = $catalogue.repository
    upstreamCommit = $example.commit
    hostMainSha256 = (Get-FileHash -LiteralPath (Join-Path $projectRoot 'src\main.cpp') -Algorithm SHA256).Hash
    adaptations = @('Maintained host with separate visual/audio resources', 'USE_MIPMAPS=1', 'RECORD_IMG_LENGTH=148', 'Upstream shaudio.h: 148 seconds, 1920x4000 audio texture', 'RGBA8 visual targets for the upstream brightness encoding')
}
[System.IO.File]::WriteAllText((Join-Path $destinationPath 'example-origin.json'), ($record | ConvertTo-Json -Depth 5), $utf8)
[System.IO.File]::WriteAllText((Join-Path $destinationPath 'README.md'), @"
# Drifting Shore working copy

Shaders by NuSan for Revision 2024, from $($catalogue.repository)/tree/$($example.commit).
This copy uses the maintained Lev4K host with mipmaps, the original audio settings,
and RGBA8 visual targets. See example-origin.json and LICENSE.

Open Lev4k.sln, select Editor / x86, and build/run with F5.
Edit src/shaders/fragment.frag here; Ctrl+S reloads it in Editor.
The default image size is 1920 x 1080 and the music lasts 148 seconds.
The shader is demanding; music generation can take time.

Build Snapshot or Release separately and measure out/Lev4k-release.exe.
This adapted host is not the original competition executable; a 4K result is not guaranteed.
The original project and other prepared copies are independent of this directory.
"@, $utf8)

Push-Location $destinationPath
try {
    & .\shader_minifier.exe --preserve-externals --no-renaming-list m1,m2,m3,m4 -o .\src\shaders\fragment.inl .\src\shaders\fragment.frag
    if ($LASTEXITCODE -ne 0) { throw 'Shader minification failed in the new working copy.' }
    if ($Build -ne 'None') {
        $msbuild = Get-Command MSBuild.exe -ErrorAction SilentlyContinue
        if ($msbuild) {
            $msbuildPath = $msbuild.Source
        } else {
            $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
            if (-not (Test-Path -LiteralPath $vswhere)) { throw 'MSBuild was not found. Open the prepared solution in Visual Studio.' }
            $installation = & $vswhere -latest -products '*' -requires Microsoft.Component.MSBuild -property installationPath
            if (-not $installation) { throw 'Install Visual Studio C++ tools to build the prepared solution.' }
            $msbuildPath = Join-Path $installation 'MSBuild\Current\Bin\MSBuild.exe'
        }
        & $msbuildPath .\Lev4k.sln /t:Rebuild "/p:Configuration=$Build" /p:Platform=x86 /nologo /v:minimal
        if ($LASTEXITCODE -ne 0) { throw "Build failed. The prepared copy remains available at $destinationPath." }
    }
} finally {
    Pop-Location
}
Write-Output "Prepared example: $destinationPath"
Write-Output "Open solution: $(Join-Path $destinationPath 'Lev4k.sln')"
