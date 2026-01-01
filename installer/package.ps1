param(
    # Folder that contains ValveWorkbench.exe produced by your Qt Creator build.
    # NOTE: This can be a Debug build output, but for a real release you should
    # point this at your MSVC Release build output folder.
    [Parameter(Mandatory = $false)]
    [string]$BuildDir = "c:\\Users\\lizar\\Documents\\ValveWorkbench\\build\\Desktop_Qt_6_9_3_MSVC2022_64bit-Debug\\release",

    # Root folder of the ValveWorkbench repo (where models/, circuits/, analyser.json live).
    # If omitted, inferred from the script location.
    [Parameter(Mandatory = $false)]
    [string]$RepoRoot = "",

    # Optional: explicitly specify the Arduino sketch folder to bundle.
    # This should be the directory that contains the .ino file (and any headers).
    # If omitted, the script will try a few common folder names and then auto-discover
    # a .ino within the repo.
    [Parameter(Mandatory = $false)]
    [string]$ArduinoDir = "",

    # Path to windeployqt.exe (Qt deployment tool).
    [Parameter(Mandatory = $false)]
    [string]$WinDeployQt = "",

    # If set, does not delete an existing dist folder.
    [Parameter(Mandatory = $false)]
    [switch]$NoClean
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Resolve-Or-Throw([string]$Path, [string]$ErrorMessage) {
    $resolved = Resolve-Path -Path $Path -ErrorAction SilentlyContinue
    if (-not $resolved) {
        throw $ErrorMessage
    }
    return $resolved.Path
}

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    $RepoRoot = (Resolve-Path -Path (Join-Path $PSScriptRoot "..\")).Path
} else {
    $RepoRoot = (Resolve-Path -Path $RepoRoot).Path
}

$buildDirResolved = Resolve-Or-Throw $BuildDir "BuildDir not found: $BuildDir"

$exePath = Join-Path $buildDirResolved "ValveWorkbench.exe"
if (-not (Test-Path -Path $exePath -PathType Leaf)) {
    throw "ValveWorkbench.exe not found in BuildDir: $buildDirResolved"
}

# Try to auto-locate windeployqt if not provided.
if ([string]::IsNullOrWhiteSpace($WinDeployQt)) {
    $candidates = @(
        "C:\\Qt\\6.9.3\\msvc2022_64\\bin\\windeployqt.exe",
        "C:\\Qt\\6.9.2\\msvc2022_64\\bin\\windeployqt.exe",
        "C:\\Qt\\6.9.1\\msvc2022_64\\bin\\windeployqt.exe",
        "C:\\Qt\\6.9.0\\msvc2022_64\\bin\\windeployqt.exe",
        "C:\\Qt\\6.8.0\\msvc2022_64\\bin\\windeployqt.exe"
    )

    foreach ($c in $candidates) {
        if (Test-Path -Path $c -PathType Leaf) {
            $WinDeployQt = $c
            break
        }
    }

    if ([string]::IsNullOrWhiteSpace($WinDeployQt)) {
        throw "windeployqt.exe not found. Re-run with -WinDeployQt <full path to windeployqt.exe>."
    }
}

$WinDeployQt = Resolve-Or-Throw $WinDeployQt "windeployqt.exe not found: $WinDeployQt"

$distRoot = Join-Path $RepoRoot "dist"
$payloadDir = Join-Path $distRoot "ValveWorkbench"

if (-not $NoClean) {
    if (Test-Path -Path $payloadDir) {
        Remove-Item -Path $payloadDir -Recurse -Force
    }
}

New-Item -ItemType Directory -Path $payloadDir -Force | Out-Null

Write-Host "Repo root:     $RepoRoot"
Write-Host "BuildDir:      $buildDirResolved"
Write-Host "Payload dir:   $payloadDir"
Write-Host "windeployqt:   $WinDeployQt"

# Copy ValveWorkbench.exe first
Copy-Item -Path $exePath -Destination (Join-Path $payloadDir "ValveWorkbench.exe") -Force

# Copy required runtime assets from repo
$repoModels = Join-Path $RepoRoot "models"
if (-not (Test-Path -Path $repoModels -PathType Container)) {
    throw "models folder not found at repo root: $repoModels"
}
Copy-Item -Path $repoModels -Destination (Join-Path $payloadDir "models") -Recurse -Force

$repoAnalyserJson = Join-Path $RepoRoot "analyser.json"
if (Test-Path -Path $repoAnalyserJson -PathType Leaf) {
    Copy-Item -Path $repoAnalyserJson -Destination (Join-Path $payloadDir "analyser.json") -Force
}

$repoCircuits = Join-Path $RepoRoot "circuits"
if (Test-Path -Path $repoCircuits -PathType Container) {
    Copy-Item -Path $repoCircuits -Destination (Join-Path $payloadDir "circuits") -Recurse -Force
}

$repoArduino = ""

if (-not [string]::IsNullOrWhiteSpace($ArduinoDir)) {
    $resolvedArduino = (Resolve-Path -Path $ArduinoDir).Path

    if (Test-Path -Path $resolvedArduino -PathType Leaf) {
        # If user passed a direct .ino path, use its containing folder.
        $repoArduino = Split-Path -Path $resolvedArduino -Parent
    } else {
        # User passed a directory. If it contains (or contains beneath it) a .ino,
        # use the directory containing the first discovered .ino.
        $ino = Get-ChildItem -Path $resolvedArduino -Recurse -File -Filter "*.ino" -ErrorAction SilentlyContinue |
            Select-Object -First 1

        if ($ino) {
            $repoArduino = $ino.DirectoryName
        } else {
            $repoArduino = $resolvedArduino
        }
    }
} else {
    $candidates = @(
        (Join-Path $RepoRoot "Analyser_Valveworkbench_v1_0_copy_20251013103620"),
        (Join-Path $RepoRoot "arduino"),
        (Join-Path $RepoRoot "Arduino"),
        (Join-Path $RepoRoot "firmware"),
        (Join-Path $RepoRoot "Firmware")
    )

    foreach ($c in $candidates) {
        if (Test-Path -Path $c -PathType Container) {
            $repoArduino = $c
            break
        }
    }

    if ([string]::IsNullOrWhiteSpace($repoArduino)) {
        # Fallback: auto-discover a sketch folder by looking for a .ino file in the repo.
        $ino = Get-ChildItem -Path $RepoRoot -Recurse -File -Filter "*.ino" -ErrorAction SilentlyContinue |
            Where-Object {
                $_.FullName -notmatch "\\\\build\\\\" -and
                $_.FullName -notmatch "\\\\dist\\\\" -and
                $_.FullName -notmatch "\\\\docs\\\\" -and
                $_.FullName -notmatch "\\\\ngspice\\\\" -and
                $_.FullName -notmatch "\\\\Spice64\\\\" -and
                $_.FullName -notmatch "\\\\vcpkg_installed\\\\"
            } |
            Select-Object -First 1

        if ($ino) {
            $repoArduino = $ino.DirectoryName
        }
    }
}

if (-not [string]::IsNullOrWhiteSpace($repoArduino) -and (Test-Path -Path $repoArduino -PathType Container)) {
    Write-Host "Arduino source: $repoArduino"
    $arduinoDest = Join-Path $payloadDir "arduino"
    New-Item -ItemType Directory -Path $arduinoDest -Force | Out-Null
    Copy-Item -Path $repoArduino -Destination (Join-Path $arduinoDest (Split-Path $repoArduino -Leaf)) -Recurse -Force
} else {
    throw "Arduino folder not found. Provide it explicitly via -ArduinoDir <path-to-folder-containing-.ino>."
}

$arduinoPayload = Join-Path $payloadDir "arduino"
if (-not (Test-Path -Path $arduinoPayload -PathType Container)) {
    throw "Arduino copy failed: payload folder missing: $arduinoPayload"
}

function Find-RuntimeDll([string]$DllName) {
    $searchRoots = @(
        $buildDirResolved,
        (Join-Path $RepoRoot "release"),
        (Join-Path $RepoRoot "debug"),
        (Join-Path $RepoRoot "vcpkg_installed\\x64-windows\\bin"),
        (Join-Path $RepoRoot "vcpkg_installed\\x64-windows\\debug\\bin")
    )

    foreach ($root in $searchRoots) {
        if (-not (Test-Path -Path $root -PathType Container)) {
            continue
        }

        $candidate = Join-Path $root $DllName
        if (Test-Path -Path $candidate -PathType Leaf) {
            return $candidate
        }
    }

    return $null
}

# Copy non-Qt runtime DLLs that ValveWorkbench depends on.
# These may live in different places depending on how you built (qmake post-link, vcpkg, etc).
$runtimeDlls = @(
    @{ Name = "glog.dll"; Required = $true },
    @{ Name = "gflags.dll"; Required = $true },
    @{ Name = "ceres.dll"; Required = $false }
)

foreach ($entry in $runtimeDlls) {
    $dll = $entry.Name
    $required = [bool]$entry.Required
    $src = Find-RuntimeDll $dll

    if ($src) {
        Copy-Item -Path $src -Destination (Join-Path $payloadDir $dll) -Force
        Write-Host "Copied $dll from: $src"
    } elseif ($required) {
        throw "Required runtime DLL not found: $dll. Checked build output, repo release/debug, and vcpkg bins."
    } else {
        Write-Host "WARNING: Optional runtime DLL not found: $dll" -ForegroundColor Yellow
    }
}

Write-Host "Running windeployqt..."

# Deploy Qt runtime into payload. We include the MSVC compiler runtime DLLs as well.
& $WinDeployQt `
    (Join-Path $payloadDir "ValveWorkbench.exe") `
    --compiler-runtime `
    --no-translations `
    --no-opengl-sw

Write-Host ""
Write-Host "DONE. Payload created:" -ForegroundColor Green
Write-Host "  $payloadDir"
Write-Host ""
Write-Host "Next step: build the installer with Inno Setup using installer\\ValveWorkbench.iss"
