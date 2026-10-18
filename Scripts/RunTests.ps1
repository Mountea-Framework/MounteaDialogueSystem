<#
.SYNOPSIS
    Runs the Mountea Dialogue System automation tests headless and exits non-zero on any failure.

.EXAMPLE
    ./Scripts/RunTests.ps1                 # runtime + editor suites
    ./Scripts/RunTests.ps1 -Suite Runtime  # Mountea.DialogueSystem.*
    ./Scripts/RunTests.ps1 -Suite Editor   # Mountea.DialogueSystemEditor.*
    ./Scripts/RunTests.ps1 -Build          # build MounteaDevEditor first

.NOTES
    Close the editor first: the tests share the project's Saved/, Config/ and Content/Localization folders.
    Import.EndToEnd creates and deletes assets under /Game; it is part of the Editor suite.
#>
param(
    [ValidateSet('All', 'Runtime', 'Editor')]
    [string]$Suite = 'All',
    [string]$Engine,
    [string]$Project,
    [switch]$Build
)

$ErrorActionPreference = 'Stop'
$pluginDir = Split-Path -Parent $PSScriptRoot

if (-not $Project) {
    $Project = Get-ChildItem -Path (Resolve-Path (Join-Path $pluginDir '..\..')) -Filter *.uproject | Select-Object -First 1 -ExpandProperty FullName
}
if (-not $Project -or -not (Test-Path $Project)) { throw "No .uproject found; pass -Project." }

if (-not $Engine) {
    $association = (Get-Content $Project -Raw | ConvertFrom-Json).EngineAssociation
    $key = "HKLM:\SOFTWARE\EpicGames\Unreal Engine\$association"
    if (Test-Path $key) { $Engine = (Get-ItemProperty $key).InstalledDirectory }
}
if (-not $Engine -or -not (Test-Path $Engine)) { throw "Engine not found; pass -Engine <UE root>." }

$editorCmd = Join-Path $Engine 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$projectName = [IO.Path]::GetFileNameWithoutExtension($Project)

if ($Build) {
    & (Join-Path $Engine 'Engine\Build\BatchFiles\Build.bat') "${projectName}Editor" Win64 Development "-Project=$Project" -WaitMutex
    if ($LASTEXITCODE -ne 0) { throw "Build failed ($LASTEXITCODE)." }
}

# The runtime prefix is followed by '.', so it does not also match Mountea.DialogueSystemEditor.*
$filter = switch ($Suite) {
    'Runtime' { 'Mountea.DialogueSystem.' }
    'Editor'  { 'Mountea.DialogueSystemEditor.' }
    default   { 'Mountea.DialogueSystem.+Mountea.DialogueSystemEditor.' }
}

$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$reportDir = Join-Path (Split-Path -Parent $Project) "Saved\Automation\MounteaDialogue-$stamp"
$log = Join-Path $reportDir 'Automation.log'
New-Item -ItemType Directory -Force -Path $reportDir | Out-Null

Write-Host "Running '$filter' ($Suite) ..."
for ($attempt = 1; $attempt -le 2; $attempt++) {
    & $editorCmd $Project "-ExecCmds=Automation RunTests $filter;Quit" -unattended -nullrhi -nosplash -nopause "-ReportExportPath=$reportDir" "-abslog=$log" | Out-Null
    # The engine sometimes crashes saving the Slate tab layout under -nullrhi before any test starts; retry once.
    $started = Select-String -Path $log -Pattern 'Test Started\.' -Quiet
    $crashed = Select-String -Path $log -Pattern 'Critical error|Fatal error' -Quiet
    if ($started -or -not $crashed) { break }
    Write-Host 'Editor crashed before any test started; retrying once ...'
}

$results = Select-String -Path $log -Pattern 'Test Completed\. Result=\{(\w+)\} Name=\{[^}]*\} Path=\{([^}]*)\}' | ForEach-Object {
    [pscustomobject]@{ Result = $_.Matches[0].Groups[1].Value; Path = $_.Matches[0].Groups[2].Value }
}
$failed = @($results | Where-Object Result -ne 'Success')
$crash = Select-String -Path $log -Pattern 'Critical error|Fatal error' -Quiet

foreach ($test in $failed) { Write-Host "FAIL  $($test.Path)" -ForegroundColor Red }
Write-Host ("{0} passed, {1} failed. Log and report: {2}" -f ($results.Count - $failed.Count), $failed.Count, $reportDir)

if ($crash) { Write-Host 'The editor crashed during the run; see the log.' -ForegroundColor Red; exit 2 }
if ($results.Count -eq 0) { Write-Host 'No tests ran.' -ForegroundColor Red; exit 3 }
exit [int]($failed.Count -gt 0)
