[CmdletBinding()]
param(
    [ValidateSet("all", "x86", "x64")]
    [string]$Architecture = "all",
    [string]$W64DevkitX86 = "D:\tools\w64devkit",
    [string]$W64DevkitX64 = "D:\tools\w64devkit-x64",
    [int]$Jobs = 6
)

$ErrorActionPreference = "Stop"
$projectRoot = Split-Path -Parent $PSScriptRoot

function Invoke-Checked([string]$program, [string[]]$arguments) {
    & $program @arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$program failed with exit code $LASTEXITCODE"
    }
}

function Build-WindowsTarget(
    [string]$preset,
    [string]$toolchainRoot,
    [string]$destination
) {
    $cmake = Join-Path $toolchainRoot "bin\cmake.exe"
    if (-not (Test-Path -LiteralPath $cmake)) {
        $cmake = "cmake"
    }
    Invoke-Checked $cmake @("--preset", $preset,
        "-DW64DEVKIT_ROOT=$($toolchainRoot -replace '\\','/')", "-DwxBUILD_SHARED=OFF")
    Invoke-Checked $cmake @("--build", "--preset", $preset, "--parallel", $Jobs)

    $buildDirectory = Join-Path $projectRoot "build\$preset"
    $outputDirectory = Join-Path $projectRoot "dist\$destination"
    New-Item -ItemType Directory -Force -Path $outputDirectory | Out-Null
    Copy-Item -LiteralPath (Join-Path $buildDirectory "EyesRelax.exe") -Destination $outputDirectory -Force
    Copy-Item -LiteralPath (Join-Path $projectRoot "locales") -Destination $outputDirectory -Recurse -Force
    $iconOutput = Join-Path $outputDirectory "icons"
    New-Item -ItemType Directory -Force -Path $iconOutput | Out-Null
    Get-ChildItem -LiteralPath (Join-Path $projectRoot "assets\icons\runtime") -File |
        Copy-Item -Destination $iconOutput -Force
}

Push-Location $projectRoot
try {
    if ($Architecture -in @("all", "x86")) {
        Build-WindowsTarget "windows-xp-x86" $W64DevkitX86 "windows-xp-x86"
    }
    if ($Architecture -in @("all", "x64")) {
        Build-WindowsTarget "windows-x64" $W64DevkitX64 "windows-x64"
    }
} finally {
    Pop-Location
}
