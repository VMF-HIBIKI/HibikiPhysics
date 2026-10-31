$ErrorActionPreference = 'Stop'
$repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$parent = [IO.Path]::GetFullPath((Join-Path $repository 'artifacts/repro'))
$destination = [IO.Path]::GetFullPath((Join-Path $parent ([Guid]::NewGuid().ToString('N').Substring(0, 8) + '/source copy')))
if (-not $destination.StartsWith($parent + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase) -or (Test-Path -LiteralPath $destination)) {
    throw 'Reproduction requires a new directory inside artifacts/repro.'
}
$environment = Get-Content -LiteralPath (Join-Path $repository '.local/environment.json') -Raw | ConvertFrom-Json
$hostExecutable = (Get-Process -Id $PID).Path
$reportDirectory = Join-Path $repository 'artifacts/environment'
[IO.Directory]::CreateDirectory($reportDirectory) | Out-Null
$report = @{ source = $repository; checkout = $destination; status = 'running'; files = @(); steps = @() }
function Save-ReproductionReport {
    [IO.File]::WriteAllText((Join-Path $reportDirectory 'reproduction.json'), ($report | ConvertTo-Json -Depth 8), [Text.UTF8Encoding]::new($false))
}
function Invoke-ReproductionStep {
    param([string]$Name, [string]$Program, [string[]]$Arguments)
    $log = Join-Path $reportDirectory "reproduction-$Name.log"
    Write-Host "Reproduction: $Name"
    & $Program @Arguments 2>&1 | Out-File -LiteralPath $log -Encoding utf8
    $report.steps += @{ name = $Name; exit_code = $LASTEXITCODE; log = $log }
    Save-ReproductionReport
    if ($LASTEXITCODE -ne 0) { Get-Content -LiteralPath $log -Tail 30 | Write-Host; throw "Reproduction step failed: $Name" }
}
Push-Location $repository
try {
    $files = @(git ls-files --cached --others --exclude-standard | Sort-Object -Unique)
    if ($LASTEXITCODE -ne 0) { throw 'Source inventory failed.' }
    foreach ($file in $files) {
        $source = Join-Path $repository $file
        $target = [IO.Path]::GetFullPath((Join-Path $destination $file))
        if (-not $target.StartsWith($destination + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) { throw 'Invalid source inventory path.' }
        [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($target)) | Out-Null
        Copy-Item -LiteralPath $source -Destination $target
        $hash = (Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash.ToLowerInvariant()
        if ((Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash -ne $hash) { throw "Copied source differs: $file" }
        $report.files += @{ path = $file; sha256 = $hash }
    }
    Save-ReproductionReport
    Invoke-ReproductionStep 'git-init' 'git' @('-C',$destination,'init','-q','-b','main')
    Invoke-ReproductionStep 'offline-prepare' $hostExecutable @('-NoProfile','-File',"$destination/scripts/dev.ps1",'prepare','-Offline','-RequireWarp','-WithEditor','-EnableHooks')
    Invoke-ReproductionStep 'doctor' $hostExecutable @('-NoProfile','-File',"$destination/scripts/dev.ps1",'doctor','-RequireWarp','-ReportPath','artifacts/environment/doctor.json')
    Invoke-ReproductionStep 'cpp-matrix' $hostExecutable @('-NoProfile','-File',"$destination/scripts/environment/VerifyCpp.ps1")
    Push-Location $destination
    try {
        Invoke-ReproductionStep 'python' (Join-Path $destination '.venv/Scripts/python.exe') @('-m','pytest','tests/toolchain/test_warp.py','-q','--junitxml=artifacts/environment/warp-tests.xml')
    }
    finally { Pop-Location }
    foreach ($entry in $report.files) {
        if ((Get-FileHash -LiteralPath (Join-Path $destination $entry.path) -Algorithm SHA256).Hash -ne $entry.sha256) { throw "Reconstruction modified source: $($entry.path)" }
    }
    $report.status = 'passed'
}
catch { $report.status = 'failed'; $report.error = $_.Exception.Message; throw }
finally {
    try {
        & $environment.paths.python (Join-Path $repository 'scripts/environment/editor_settings.py')
        if ($LASTEXITCODE -ne 0) {
            $report.status = 'failed'
            $report.restore_error = 'Original editor settings restoration failed.'
        }
    }
    finally { Save-ReproductionReport; Pop-Location }
}
if ($report.status -ne 'passed') { throw 'Reproduction completed with recorded failures.' }
exit 0
