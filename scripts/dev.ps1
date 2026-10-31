param(
    [Parameter(Position = 0)][ValidateSet('doctor','prepare','configure','build','test','check','format','verify')][string]$Command = 'doctor',
    [string]$ConfigPath,
    [hashtable]$Paths = @{},
    [switch]$RequireWarp,
    [string]$ReportPath,
    [ValidateSet('windows-debug','windows-relwithdebinfo','windows-release','windows-asan','windows-msvc')][string]$Preset = 'windows-debug',
    [string]$ArchiveDirectory,
    [switch]$Offline,
    [switch]$WithEditor,
    [switch]$EnableHooks,
    [switch]$Reproduce
)

$ErrorActionPreference = 'Stop'
. "$PSScriptRoot/environment/Doctor.ps1"
$repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
try {
    if ($Command -eq 'prepare') {
        . "$PSScriptRoot/environment/Prepare.ps1"
        Initialize-HibikiEnvironment $repository $ArchiveDirectory -Offline:$Offline -RequireWarp:$RequireWarp -WithEditor:$WithEditor -EnableHooks:$EnableHooks -ConfigPath $ConfigPath -Paths $Paths
        exit 0
    }
    if ($Command -in @('configure','build','test')) {
        $environment = Read-HibikiJson (Join-Path $repository '.local/environment.json')
        $diagnostics = Invoke-HibikiDoctor -RepositoryRoot $repository -Scope cpp
        $failed = @($diagnostics.checks | Where-Object {
            ($_.id -in @('manifest','overrides','host','cmake','cmake.provenance','ninja','ninja.provenance','clang-cl','clang-cl.provenance','lld-link','lld-link.provenance') -or $_.id -match '^(msvc\.|sdk\.)') -and $_.status -ne 'matched'
        })
        if ($failed.Count) { throw "Build prerequisite checks failed: $($failed.id -join ', ')" }
        $savedPath = $env:PATH
        $runtime = Join-Path (Split-Path (Split-Path $environment.paths.'clang-cl')) 'lib/clang/22/lib/windows'
        Push-Location $repository
        try {
            $env:PATH = "$runtime;$savedPath"
            if ($Command -eq 'configure') { & $environment.paths.cmake --preset $Preset }
            elseif ($Command -eq 'build') { & $environment.paths.cmake --build --preset $Preset }
            else { & (Join-Path (Split-Path $environment.paths.cmake) 'ctest.exe') --preset $Preset }
            $result = $LASTEXITCODE
        }
        finally { Pop-Location; $env:PATH = $savedPath }
        exit $result
    }
    if ($Command -eq 'verify') {
        $environment = Read-HibikiJson (Join-Path $repository '.local/environment.json')
        $reportDirectory = Join-Path $repository 'artifacts/environment'
        [IO.Directory]::CreateDirectory($reportDirectory) | Out-Null
        $verification = @{ status = 'running'; started_at = [DateTime]::UtcNow.ToString('o'); steps = @(); source_files = @() }
        $reportFile = Join-Path $reportDirectory 'verification.json'
        function Save-Verification {
            [IO.File]::WriteAllText($reportFile, ($verification | ConvertTo-Json -Depth 10), [Text.UTF8Encoding]::new($false))
        }
        function Invoke-VerificationStep {
            param([string]$Name, [scriptblock]$Action)
            Write-Host "Verification: $Name"
            & $Action *>&1 | Tee-Object -FilePath (Join-Path $reportDirectory "verify-$Name.log")
            if ($LASTEXITCODE -ne 0) { throw "Verification failed: $Name (exit $LASTEXITCODE)" }
            $verification.steps += @{ name = $Name; status = 'passed' }
            Save-Verification
        }
        Push-Location $repository
        try {
            $files = @(git ls-files --cached --others --exclude-standard | Sort-Object -Unique)
            if ($LASTEXITCODE -ne 0) { throw 'Source inventory failed.' }
            foreach ($file in $files) {
                $verification.source_files += @{ path = $file; sha256 = (Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash.ToLowerInvariant() }
            }
            $verification.toolchain_sha256 = (Get-FileHash -LiteralPath tools/toolchain-lock.json -Algorithm SHA256).Hash.ToLowerInvariant()
            $verification.python_lock_sha256 = (Get-FileHash -LiteralPath uv.lock -Algorithm SHA256).Hash.ToLowerInvariant()
            Save-Verification
            Invoke-VerificationStep 'cpp-matrix' { & "$PSScriptRoot/environment/VerifyCpp.ps1" }
            Invoke-VerificationStep 'quality' { & "$PSScriptRoot/environment/Check.ps1" }
            Invoke-VerificationStep 'doctor-regressions' { & "$repository/tests/environment/doctor.tests.ps1" }
            $testDirectory = Join-Path $reportDirectory ('pytest-' + [Guid]::NewGuid().ToString('N'))
            $pythonArguments = @('-m','pytest','tests/environment','--basetemp',$testDirectory,'-q',"--junitxml=$reportDirectory/python-acceptance.xml")
            if ($RequireWarp) { $pythonArguments += 'tests/toolchain/test_warp.py' }
            Invoke-VerificationStep 'python-and-environment' { & $environment.paths.python @pythonArguments }
            if ($WithEditor) { Invoke-VerificationStep 'editor' { & "$PSScriptRoot/environment/VerifyEditor.ps1" } }
            if ($Reproduce) { Invoke-VerificationStep 'reproduction' { & "$PSScriptRoot/environment/Reproduce.ps1" } }
            $diagnostics = Invoke-HibikiDoctor -RepositoryRoot $repository -RequireWarp:$RequireWarp
            [IO.File]::WriteAllText((Join-Path $reportDirectory 'doctor-final.json'), ($diagnostics | ConvertTo-Json -Depth 12), [Text.UTF8Encoding]::new($false))
            if (-not $diagnostics.ready) { throw 'Final environment identity checks failed.' }
            foreach ($entry in $verification.source_files) {
                if ((Get-FileHash -LiteralPath $entry.path -Algorithm SHA256).Hash -ne $entry.sha256) { throw "Verification changed source: $($entry.path)" }
            }
            $afterFiles = @(git ls-files --cached --others --exclude-standard | Sort-Object -Unique)
            if (@(Compare-Object $files $afterFiles).Count -gt 0) { throw 'Verification changed the source-file inventory.' }
            $verification.steps += @{ name = 'final-identities-and-source-preservation'; status = 'passed' }
            $verification.status = 'passed'
        }
        catch { $verification.status = 'failed'; $verification.error = $_.Exception.Message; throw }
        finally { $verification.completed_at = [DateTime]::UtcNow.ToString('o'); Save-Verification; Pop-Location }
        Write-Host "Verification passed: $reportFile"
        exit 0
    }
    if ($Command -in @('check','format')) {
        & "$PSScriptRoot/environment/Check.ps1" -Format:($Command -eq 'format')
        exit 0
    }
    $report = Invoke-HibikiDoctor -RepositoryRoot $repository -ConfigPath $ConfigPath -Paths $Paths -RequireWarp:$RequireWarp
    foreach ($check in $report.checks) {
        Write-Output ("[{0}] {1}: {2}" -f $check.status, $check.id, $check.detail)
        if ($check.path) { Write-Output ("  path: {0}" -f $check.path) }
        if ($check.status -ne 'matched') {
            Write-Output ("  expected: {0}" -f ($check.expected | ConvertTo-Json -Compress -Depth 4))
            Write-Output ("  actual: {0}" -f ($check.actual | ConvertTo-Json -Compress -Depth 4))
            Write-Output ("  required: {0}; next: {1}" -f $check.required, $check.next_step)
        }
    }
    Write-Output ("Ready: {0}; required failures: {1}" -f $report.ready, $report.required_failures)
    if ($ReportPath) {
        $destination = Resolve-HibikiPath $repository $ReportPath
        $reportRoot = [IO.Path]::GetFullPath((Join-Path $repository 'artifacts/environment'))
        if (-not $destination.StartsWith($reportRoot + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase) -or
            [IO.Path]::GetExtension($destination) -ne '.json') {
            throw 'ReportPath must name a JSON file inside artifacts/environment.'
        }
        # Check existing ancestors before creating the explicitly requested report.
        $ancestor = $destination
        while ($ancestor -and $ancestor.Length -ge $repository.Length) {
            if (Test-Path -LiteralPath $ancestor) {
                $item = Get-Item -LiteralPath $ancestor -Force
                if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'Report paths require ordinary directories.' }
            }
            $ancestor = [IO.Path]::GetDirectoryName($ancestor)
        }
        [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($destination)) | Out-Null
        [IO.File]::WriteAllText($destination, ($report | ConvertTo-Json -Depth 12), [Text.UTF8Encoding]::new($false))
        Write-Output "Report: $destination"
    }
    if ($report.ready) { exit 0 }
    exit 1
}
catch {
    Write-Output ("[mismatch] {0}: {1}" -f $Command, $_.Exception.Message)
    exit 2
}
