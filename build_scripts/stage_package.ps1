[CmdletBinding()]
param(
    [ValidateSet('Release', 'RelWithDebInfo')]
    [string]$Configuration = 'Release'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$installerRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$buildName = if ($Configuration -eq 'Release') { 'msvc-release' } else { 'msvc-dev' }
$buildRoot = Join-Path $repoRoot "build\$buildName"
$stageRoot = Join-Path $installerRoot 'dist\package'
$compiler = Join-Path $buildRoot 'bin\nirqoba.exe'
$runtime = Join-Path $buildRoot 'lib\nrqoba_runtime.lib'

foreach ($required in @(
    $compiler,
    $runtime,
    (Join-Path $repoRoot 'toolchain\toolchain.json'),
    (Join-Path $repoRoot 'toolchain\versions.lock.json')
)) {
    if (-not (Test-Path -LiteralPath $required -PathType Leaf)) {
        throw "Required package input is missing: $required. Build the compiler first."
    }
}

New-Item -ItemType Directory -Force -Path `
    (Join-Path $stageRoot 'bin'), `
    (Join-Path $stageRoot 'lib'), `
    (Join-Path $stageRoot 'toolchain'), `
    (Join-Path $stageRoot 'examples'), `
    (Join-Path $stageRoot 'docs') | Out-Null

Copy-Item -LiteralPath $compiler -Destination (Join-Path $stageRoot 'bin\nirqoba.exe') -Force
Copy-Item -LiteralPath $runtime -Destination (Join-Path $stageRoot 'lib\nrqoba_runtime.lib') -Force
Copy-Item -LiteralPath (Join-Path $repoRoot 'toolchain\toolchain.json') `
    -Destination (Join-Path $stageRoot 'toolchain\toolchain.json') -Force
Copy-Item -LiteralPath (Join-Path $repoRoot 'toolchain\versions.lock.json') `
    -Destination (Join-Path $stageRoot 'toolchain\versions.lock.json') -Force
Copy-Item -Path (Join-Path $repoRoot 'examples\*') `
    -Destination (Join-Path $stageRoot 'examples') -Recurse -Force
Copy-Item -Path (Join-Path $repoRoot 'docs\*') `
    -Destination (Join-Path $stageRoot 'docs') -Recurse -Force
Copy-Item -LiteralPath (Join-Path $repoRoot 'README.md') `
    -Destination (Join-Path $stageRoot 'README.md') -Force

Write-Host "Staged compiler and runtime package: $stageRoot"
Write-Host 'The included toolchain configuration refers to the build host toolchain; this is a developer-package foundation.'
