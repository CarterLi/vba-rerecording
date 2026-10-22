<#
    FindMSBuild.ps1 - print the path of the MSBuild.exe to use.

    GitHub's Windows runners have Visual Studio installed but MSBuild is not
    on PATH, and where the installer and the build tools live depends on the
    host: on an x64 host the installer is under "Program Files (x86)", on an
    ARM64 host it is under "Program Files".  This looks in both, and falls
    back to the usual directory layout.

    The host native build of MSBuild is preferred, because it is the one that
    can load the native toolset for that host.

    Usage:
        pwsh -File win32\defaultconfig\FindMSBuild.ps1

    The path is printed on stdout and nothing else, so it can be used as:
        $msbuild = & win32\defaultconfig\FindMSBuild.ps1
#>

$ErrorActionPreference = 'Stop'

$candidates = New-Object System.Collections.Generic.List[string]

# The roots the Visual Studio installer and the IDE itself live under.
$roots = @()
foreach ($root in @(${env:ProgramFiles(x86)}, $env:ProgramFiles)) {
    if ($root -and (Test-Path -LiteralPath $root) -and ($roots -notcontains $root)) {
        $roots += $root
    }
}

# ---------------------------------------------------------------------------
# vswhere, when it is there, knows which installation is the right one.
# ---------------------------------------------------------------------------
foreach ($root in $roots) {
    $vswhere = Join-Path $root 'Microsoft Visual Studio\Installer\vswhere.exe'

    if (-not (Test-Path -LiteralPath $vswhere)) { continue }

    $found = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild `
                      -find 'MSBuild\**\Bin\MSBuild.exe' 2>$null

    foreach ($path in @($found)) {
        if ($path -and -not $candidates.Contains($path)) {
            $candidates.Add($path)
        }
    }
}

# ---------------------------------------------------------------------------
# Without vswhere, the directory layout is the same anyway.
# ---------------------------------------------------------------------------
foreach ($root in $roots) {
    $ide = Join-Path $root 'Microsoft Visual Studio'

    if (-not (Test-Path -LiteralPath $ide)) { continue }

    $found = Get-ChildItem -LiteralPath $ide -Filter 'MSBuild.exe' -Recurse -File `
                          -ErrorAction SilentlyContinue |
             Where-Object { $_.Directory.Name -in @('amd64', 'ARM64', 'x86') }

    foreach ($item in $found) {
        if (-not $candidates.Contains($item.FullName)) {
            $candidates.Add($item.FullName)
        }
    }
}

if ($candidates.Count -eq 0) {
    throw 'No MSBuild.exe was found.  Is Visual Studio installed on this machine?'
}

# ---------------------------------------------------------------------------
# The host native build comes first.
# ---------------------------------------------------------------------------
$hostToolset = if ($env:PROCESSOR_ARCHITECTURE -eq 'ARM64') { 'ARM64' }
               elseif ($env:PROCESSOR_ARCHITECTURE -eq 'AMD64') { 'amd64' }
               else { 'x86' }

$ordered = @($candidates | Sort-Object -Property @{ Expression = {
    if ($_ -match "\\$hostToolset\\") { 0 } else { 1 } } })

Write-Output $ordered[0]
