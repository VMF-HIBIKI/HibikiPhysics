$ErrorActionPreference = 'Stop'
$repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
. (Join-Path $repository 'scripts/environment/Doctor.ps1')
$fixture = Join-Path $repository ('artifacts/environment-tests/' + [Guid]::NewGuid().ToString('N') + '/workspace with spaces')
[IO.Directory]::CreateDirectory($fixture) | Out-Null
foreach ($relative in @('tools/toolchain-lock.json', 'tools/provenance/windows-system-components.json',
        '.python-version', 'pyproject.toml', 'scripts/dev.ps1', 'scripts/environment/Doctor.ps1', 'scripts/environment/Inventory.ps1')) {
    $destination = Join-Path $fixture $relative
    [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($destination)) | Out-Null
    Copy-Item -LiteralPath (Join-Path $repository $relative) -Destination $destination
}
$script:passed = 0
function Assert-Doctor {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw "FAILED: $Message" }
    $script:passed++
    Write-Output "PASS: $Message"
}
function Find-Check {
    param($Report, [string]$Id)
    return ($Report.checks | Where-Object id -eq $Id | Select-Object -First 1)
}
function Get-FixtureSnapshot {
    return (@(Get-ChildItem -LiteralPath $fixture -Recurse -Force -File | Sort-Object FullName | ForEach-Object {
        "$($_.FullName)|$($_.LastWriteTimeUtc.Ticks)|$((Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash)"
    }) -join "`n")
}

$checks = [Collections.Generic.List[object]]::new()
Add-HibikiCheck $checks 'required' 'matched' '' 'identity' 'identity' 'Fixture identity.'
Add-HibikiCheck $checks 'optional' 'missing' '' 'optional identity' $null 'Optional fixture.' $false
$aggregate = New-HibikiReport $checks @{} $fixture
Assert-Doctor ($aggregate.ready -and $aggregate.required_failures -eq 0) 'Matched required checks yield success while optional checks remain visible'
Add-HibikiCheck $checks 'required-pending' 'pending' '' 'identity' $null 'Pending fixture.'
$aggregate = New-HibikiReport $checks @{} $fixture
Assert-Doctor (-not $aggregate.ready -and $aggregate.required_failures -eq 1) 'Pending required evidence blocks readiness'

$before = Get-FixtureSnapshot
$environmentBefore = @(Get-ChildItem Env: | Sort-Object Name | ForEach-Object { "$($_.Name)=$($_.Value)" }) -join "`n"
$report = Invoke-HibikiDoctor -RepositoryRoot $fixture -Paths @{ 'clang-cl' = 'missing directory/clang-cl.exe' }
Assert-Doctor ((Find-Check $report 'clang-cl').status -eq 'missing') 'An explicit missing executable produces missing status'
Assert-Doctor ($report.selected_paths['clang-cl'] -eq (Join-Path $fixture 'missing directory/clang-cl.exe')) 'Space-containing relative overrides resolve against the repository'
Assert-Doctor (-not $report.ready -and $report.required_failures -gt 0) 'Required failures produce a failed readiness result'
Assert-Doctor ((Get-FixtureSnapshot) -eq $before) 'Default diagnostics preserve fixture file contents and timestamps'
$environmentAfter = @(Get-ChildItem Env: | Sort-Object Name | ForEach-Object { "$($_.Name)=$($_.Value)" }) -join "`n"
Assert-Doctor ($environmentBefore -eq $environmentAfter) 'Diagnostics preserve process environment variables'

$configPath = Join-Path $fixture 'local paths.json'
@{ schema_version = 1; paths = @{ cmake = 'configured cmake/cmake.exe' } } | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $configPath -Encoding UTF8
$report = Invoke-HibikiDoctor -RepositoryRoot $fixture -ConfigPath $configPath -Paths @{ cmake = 'override cmake/cmake.exe' }
Assert-Doctor ($report.selected_paths.cmake -eq (Join-Path $fixture 'override cmake/cmake.exe')) 'Command-line paths take precedence over local configuration'
$report = Invoke-HibikiDoctor -RepositoryRoot $fixture -ConfigPath 'absent.json'
Assert-Doctor ((Find-Check $report 'overrides').status -eq 'mismatch') 'Explicit missing configuration is reported'
$report = Invoke-HibikiDoctor -RepositoryRoot $fixture -Paths @{ cmaek = 'typo.exe' }
Assert-Doctor ((Find-Check $report 'overrides').status -eq 'mismatch') 'Unknown override keys are rejected'

$binary = Join-Path $fixture 'inert file.exe'
$bytes = [byte[]]::new(128)
$bytes[0] = 0x4d; $bytes[1] = 0x5a; $bytes[60] = 64
$bytes[64] = 0x50; $bytes[65] = 0x45; $bytes[68] = 0x64; $bytes[69] = 0x86
[IO.File]::WriteAllBytes($binary, $bytes)
Assert-Doctor ((Get-HibikiArchitecture $binary) -eq 'x64') 'PE architecture is read from bytes'
$bytes[68] = 0x4c; $bytes[69] = 0x01
[IO.File]::WriteAllBytes($binary, $bytes)
Assert-Doctor ((Get-HibikiArchitecture $binary) -eq 'x86') 'PE x86 architecture is distinguished'
$originalIdentity = (Get-Command Get-HibikiBinaryIdentity).ScriptBlock
try {
    function Get-HibikiBinaryIdentity { return [pscustomobject]@{ version = '0.0.1'; architecture = 'x64'; sha256 = ('0' * 64) } }
    $checks = [Collections.Generic.List[object]]::new()
    Test-HibikiBinary $checks 'test' $binary '22.1.6'
    Assert-Doctor ($checks[0].status -eq 'mismatch') 'A wrong version produces mismatch status'
    function Get-HibikiBinaryIdentity { return [pscustomobject]@{ version = ''; architecture = 'x64'; sha256 = ('0' * 64) } }
    $checks = [Collections.Generic.List[object]]::new()
    Test-HibikiBinary $checks 'test' $binary '22.1.6'
    Assert-Doctor ($checks[0].status -eq 'pending') 'Missing version-resource evidence produces pending status'
    function Get-HibikiBinaryIdentity { return [pscustomobject]@{ version = '22.1.6'; architecture = 'x64'; sha256 = ('0' * 64) } }
    $checks = [Collections.Generic.List[object]]::new()
    Test-HibikiBinary $checks 'test' $binary '22.1.6'
    Assert-Doctor ($checks[0].status -eq 'matched' -and $checks[1].status -eq 'pending') 'Version match and payload provenance have separate states'
}
finally { Set-Item Function:Get-HibikiBinaryIdentity $originalIdentity }

Set-Content -LiteralPath (Join-Path $fixture '.python-version') -Value '3.12.0' -Encoding UTF8
$report = Invoke-HibikiDoctor -RepositoryRoot $fixture -RequireWarp
Assert-Doctor ((Find-Check $report 'python.declarations').status -eq 'mismatch') 'Conflicting Python declarations are detected'
Assert-Doctor ((Find-Check $report 'warp-lang.installed').required) 'Warp becomes required through the explicit switch'
$lockPath = Join-Path $fixture 'tools/toolchain-lock.json'
$lock = Read-HibikiJson $lockPath
$lock.artifacts += $lock.artifacts[0]
$lock | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $lockPath -Encoding UTF8
$report = Invoke-HibikiDoctor -RepositoryRoot $fixture
Assert-Doctor ((Find-Check $report 'manifest').status -eq 'mismatch') 'Duplicate manifest IDs fail before path discovery'
Copy-Item -LiteralPath (Join-Path $repository 'tools/toolchain-lock.json') -Destination $lockPath

$systemPath = Join-Path $fixture 'tools/provenance/windows-system-components.json'
Add-Content -LiteralPath $systemPath -Value ' '
$report = Invoke-HibikiDoctor -RepositoryRoot $fixture
Assert-Doctor ((Find-Check $report 'manifest').status -eq 'mismatch') 'Companion manifest tampering is detected'
$system = Read-HibikiJson $systemPath
$testHeader = Join-Path $fixture 'mock msvc/include/test.h'
[IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($testHeader)) | Out-Null
[IO.File]::WriteAllText($testHeader, 'reference header')
$system.files = @([pscustomobject]@{ root = 'msvc'; path = 'include/test.h';
    sha256 = (Get-FileHash -LiteralPath $testHeader -Algorithm SHA256).Hash.ToLowerInvariant(); size = (Get-Item -LiteralPath $testHeader).Length })
$system | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath $systemPath -Encoding UTF8
$lock = Read-HibikiJson $lockPath
$lock.system_components.sha256 = (Get-FileHash -LiteralPath $systemPath -Algorithm SHA256).Hash.ToLowerInvariant()
$lock | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $lockPath -Encoding UTF8
$report = Invoke-HibikiDoctor -RepositoryRoot $fixture -Paths @{ msvc = 'mock msvc' }
Assert-Doctor ((Find-Check $report 'msvc.include/test.h').status -eq 'matched') 'Selected system files are verified by content hash'
[IO.File]::WriteAllText($testHeader, 'changed header')
$report = Invoke-HibikiDoctor -RepositoryRoot $fixture -Paths @{ msvc = 'mock msvc' }
Assert-Doctor ((Find-Check $report 'msvc.include/test.h').status -eq 'mismatch') 'Changed system-file contents produce mismatch status'

$extension = Join-Path $fixture 'extension with spaces'
[IO.Directory]::CreateDirectory($extension) | Out-Null
$package = @{ publisher = 'llvm-vs-code-extensions'; name = 'vscode-clangd'; version = '0.6.0'; __metadata = @{ targetPlatform = 'undefined' } }
$package | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $extension 'package.json') -Encoding UTF8
$report = Invoke-HibikiDoctor -RepositoryRoot $fixture -Paths @{ 'llvm-vs-code-extensions.vscode-clangd' = $extension }
Assert-Doctor ((Find-Check $report 'llvm-vs-code-extensions.vscode-clangd').status -eq 'matched') 'Universal extension metadata is recognized at an explicit path'
$package.version = '0.0.1'
$package | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $extension 'package.json') -Encoding UTF8
$report = Invoke-HibikiDoctor -RepositoryRoot $fixture -Paths @{ 'llvm-vs-code-extensions.vscode-clangd' = $extension }
Assert-Doctor ((Find-Check $report 'llvm-vs-code-extensions.vscode-clangd').status -eq 'mismatch') 'Wrong extension versions are reported'

$hostExe = (Get-Process -Id $PID).Path
$entry = Join-Path $fixture 'scripts/dev.ps1'
$before = Get-FixtureSnapshot
& $hostExe -NoProfile -File $entry doctor | Out-Null
Assert-Doctor ($LASTEXITCODE -eq 1) 'The entry point returns exit code 1 for required diagnostic failures'
Assert-Doctor ((Get-FixtureSnapshot) -eq $before) 'The default entry point preserves the fixture tree'
& $hostExe -NoProfile -File $entry doctor -ReportPath 'artifacts/environment/doctor.json' | Out-Null
Assert-Doctor ($LASTEXITCODE -eq 1) 'Explicit report writing preserves the diagnostic exit code'
$saved = Read-HibikiJson (Join-Path $fixture 'artifacts/environment/doctor.json')
Assert-Doctor ($saved.schema_version -eq 1 -and -not $saved.ready -and $saved.checks.Count -gt 0) 'The explicit JSON report contains structured diagnostics'
& $hostExe -NoProfile -File $entry doctor -ReportPath 'outside.json' | Out-Null
Assert-Doctor ($LASTEXITCODE -eq 2 -and -not (Test-Path -LiteralPath (Join-Path $fixture 'outside.json'))) 'Report output stays within the designated directory'
Write-Output "$script:passed doctor checks passed. Fixture: $fixture"
exit 0
