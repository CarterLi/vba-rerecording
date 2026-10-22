<#
    Package.ps1 - collect one architecture into a release archive.

    The project builds two cores, the V8 core and the V7 core, and each of
    them goes into its own output directory.  The post build step has already
    put lua51.dll and 7z.dll next to each executable, and wgpu_native.dll when
    the WebGPU renderer is part of the build.  This script takes the two of
    them, renames the executables to names a user can tell apart, writes a
    short readme next to them and zips the lot.

    It is what the CI workflow (.github/workflows/build.yml) uses, and it can
    be run by hand after a build:

        pwsh -File win32\defaultconfig\Package.ps1 -Platform x64 -OutputDir package

    The result is package\vba-rerecording-<arch>.zip, holding one directory
    with both cores and the libraries they load at run time.
#>

[CmdletBinding()]
param(
    # Which architecture to package.
    [Parameter(Mandatory = $true)]
    [ValidateSet('Win32', 'x64', 'ARM64')]
    [string]$Platform,

    # Where the archive is written.  Created when it does not exist.
    [Parameter(Mandatory = $true)]
    [string]$OutputDir,

    # Where the project lives.  The default is the repository root, two
    # levels above this script.
    [string]$Root = (Split-Path -Parent (Split-Path -Parent $PSScriptRoot)),

    # The address of the project, written into the readme of the package.
    [string]$Repository = 'https://github.com/CarterLi/vba-rerecording'
)

$ErrorActionPreference = 'Stop'

# ---------------------------------------------------------------------------
# The output directories of the two cores, per architecture.
#
# These are the OutDir of the project file: $(SolutionDir) is the win32
# directory, and every architecture has its own directory under it.
# ---------------------------------------------------------------------------
$win32 = Join-Path $Root 'win32'

switch ($Platform) {
    'Win32' {
        $arch = 'win32'
        $dirs = [ordered]@{
            'V8' = Join-Path $win32 'x86\V82XR'
            'V7' = Join-Path $win32 'x86\V72XR'
        }
    }
    'x64' {
        $arch = 'x64'
        $dirs = [ordered]@{
            'V8' = Join-Path $win32 'x64\V82XR'
            'V7' = Join-Path $win32 'x64\V72XR'
        }
    }
    'ARM64' {
        $arch = 'arm64'
        $dirs = [ordered]@{
            'V8' = Join-Path $win32 'arm64\V82XR'
            'V7' = Join-Path $win32 'arm64\V72XR'
        }
    }
}

# The name each core gets inside the package.
$exeNames = [ordered]@{
    'V8' = 'VBA-RR-v8.exe'
    'V7' = 'VBA-RR-v7.exe'
}

# The libraries the emulator loads at run time.  The WebGPU one is only
# there for the architectures that have it.
$runtimeLibraries = @('lua51.dll', '7z.dll', 'wgpu_native.dll')

# ---------------------------------------------------------------------------
# The staging directory, and the archive that is made out of it.
# ---------------------------------------------------------------------------
if (-not (Test-Path -LiteralPath $OutputDir)) {
    New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null
}
$OutputDir = (Resolve-Path -LiteralPath $OutputDir).Path

$stage = Join-Path $OutputDir "vba-rerecording-$arch"
if (Test-Path -LiteralPath $stage) {
    Remove-Item -LiteralPath $stage -Recurse -Force
}
New-Item -ItemType Directory -Path $stage -Force | Out-Null

# ---------------------------------------------------------------------------
# The two cores and the libraries.
# ---------------------------------------------------------------------------
foreach ($core in $dirs.Keys) {
    $dir = $dirs[$core]

    if (-not (Test-Path -LiteralPath $dir)) {
        throw "The output directory '$dir' does not exist.  Was the $core core built?"
    }

    $exe = Get-ChildItem -LiteralPath $dir -Filter 'VBA_VS_Latest_*.exe' -File |
           Select-Object -First 1

    if (-not $exe) {
        throw "No executable was found in '$dir'.  Was the $core core built?"
    }

    Copy-Item -LiteralPath $exe.FullName -Destination (Join-Path $stage $exeNames[$core])

    foreach ($library in $runtimeLibraries) {
        $source = Join-Path $dir $library
        if (Test-Path -LiteralPath $source) {
            Copy-Item -LiteralPath $source -Destination $stage
        }
    }
}

# ---------------------------------------------------------------------------
# The readme of the package.
# ---------------------------------------------------------------------------
$readme = @"
VBA-rr - the re-recording branch of VisualBoyAdvance - Modernized Edition
$arch build

  VBA-RR-v7.exe   the V7 core (reports itself as v23.7).  This is the original
                  VBA core, and the one the vast majority of the existing VBM
                  recordings were made with.  Use this one unless you have a
                  reason not to.

  VBA-RR-v8.exe   the V8 core (reports itself as v24.1).  This core came from
                  VBA-M.  It fixes a number of emulation bugs - a good part of
                  them in the GB core, and a few in the GBA core, such as the
                  well known crash of the tree boss in the Blue Moon edition
                  of Rockman EXE 4 (Mega Man) - but it also brings some lag
                  with it.

If you are recording a GBA game, use the V7 core: the V8 core runs the old,
excessively laggy GBA timing, so the game lags more than it should and a
movie recorded with it does not match one recorded with the V7 core.

Both write their own trace next to the executable, vba-trace-v8.log and
vba-trace-v7.log.

The Play Movie dialog shows the core the selected movie was recorded with, so
you can tell before you start which of the two you need.  A movie from the
other core can still be played; it is just not guaranteed to stay in sync.

Requirements
------------
Windows 7 or later is what we recommend, and Windows 10 or 11 if you want the
dark mode and the per-monitor DPI handling.  Every optional library
(d3d11.dll, d3dcompiler_47.dll, XAUDIO2_9.dll, wgpu_native.dll) is loaded at
run time and is not needed: the emulator falls back to the next renderer when
one of them is missing.  The exceptions are the Windows Imaging Component the
screenshots go through, which needs the codecs Windows 7 and later ship with,
and the bundled lua51.dll, which is needed for the Lua scripting.

The release notes of this build are in
documentation/VBArr-modernized-changelog.txt in the repository, and the
project itself is at
$Repository
"@

Set-Content -LiteralPath (Join-Path $stage 'readme.txt') -Value $readme -Encoding utf8

# ---------------------------------------------------------------------------
# The archive.
# ---------------------------------------------------------------------------
$archive = Join-Path $OutputDir "vba-rerecording-$arch.zip"
if (Test-Path -LiteralPath $archive) {
    Remove-Item -LiteralPath $archive -Force
}

Compress-Archive -Path $stage -DestinationPath $archive -CompressionLevel Optimal

Write-Host "Packaged $arch into $archive"
Get-ChildItem -LiteralPath $stage | ForEach-Object { Write-Host "  $($_.Name)" }
