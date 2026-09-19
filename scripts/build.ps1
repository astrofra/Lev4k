param(
    [Parameter(Position = 0)]
    [ValidateSet('Editor', 'Snapshot', 'Release', 'All', 'Test')]
    [string]$Target = 'Editor',
    [ValidateRange(1, 2147483647)]
    [int]$MaxIntroBytes = 4095
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot

try {
    $cmakeCommand = Get-Command cmake.exe -ErrorAction SilentlyContinue
    $cmake = if ($cmakeCommand) { $cmakeCommand.Source } else { $null }
    if (-not $cmake) {
        $standalone = Join-Path $env:ProgramFiles 'CMake\bin\cmake.exe'
        if (Test-Path -LiteralPath $standalone) { $cmake = $standalone }
    }
    if (-not $cmake) {
        $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
        if (Test-Path -LiteralPath $vswhere) {
            $installation = & $vswhere -latest -version '[17.0,18.0)' -products '*' -requires Microsoft.Component.MSBuild -property installationPath
            if ($installation) {
                $bundled = Join-Path $installation 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
                if (Test-Path -LiteralPath $bundled) { $cmake = $bundled }
            }
        }
    }
    if (-not $cmake) { throw 'Install CMake 3.24+ or the Visual Studio C++ CMake tools.' }

    Push-Location $projectRoot
    try {
        $gpuTests = if ($Target -eq 'Test') { 'ON' } else { 'OFF' }
        & $cmake --preset windows-x86 "-DLEV4K_MAX_INTRO_BYTES=$MaxIntroBytes" "-DLEV4K_BUILD_GPU_TESTS=$gpuTests"
        if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed. Check the diagnostics above and the CMake 3.24+, Visual Studio 2022 C++, and Windows SDK prerequisites.' }
        if ($Target -eq 'Test') {
            & $cmake --build build/cmake-windows-x86 --config Release --target lev4k_feedback_probe_0 lev4k_feedback_probe_1
            if ($LASTEXITCODE -ne 0) { throw 'GPU probe build failed.' }
            $ctest = Join-Path (Split-Path -Parent $cmake) 'ctest.exe'
            & $ctest --test-dir build/cmake-windows-x86 -C Release --output-on-failure
            if ($LASTEXITCODE -ne 0) { throw 'GPU checks failed. See the CTest output.' }
        } else {
            $presets = switch ($Target) {
                'Editor' { @('editor') }
                'Snapshot' { @('snapshot') }
                'Release' { @('intro') }
                'All' { @('editor', 'snapshot', 'intro') }
            }
            foreach ($preset in $presets) {
                & $cmake --build --preset $preset
                if ($LASTEXITCODE -ne 0) { throw "Build '$preset' failed. Check the compiler, Crinkler, or size-check output above." }
            }
            Write-Output "Executables: $(Join-Path $projectRoot 'build\cmake-windows-x86\out\Release')"
            if ($Target -in @('Editor', 'All')) {
                Write-Output 'Run Editor with the repository root as its working directory, or open the generated solution and use F5.'
            }
        }
    } finally {
        Pop-Location
    }
} catch {
    Write-Error $_
    exit 1
}
