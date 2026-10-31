$ErrorActionPreference = 'Stop'
$mutex = [Threading.Mutex]::new($false, 'Local\HibikiPhysicsEditorAcceptance')
$held = $false
try {
try { $held = $mutex.WaitOne(0) } catch [Threading.AbandonedMutexException] { $held = $true }
if (-not $held) { throw 'An editor acceptance run is already active.' }
$repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$environment = Get-Content -LiteralPath (Join-Path $repository '.local/environment.json') -Raw | ConvertFrom-Json
$reportPath = Join-Path $repository 'artifacts/environment/editor-acceptance.json'
$workspacePath = Join-Path $repository '.local/editor-acceptance.code-workspace'
[IO.File]::WriteAllText($workspacePath, (@{ folders = @(@{ path = $repository }) } | ConvertTo-Json -Depth 4), [Text.UTF8Encoding]::new($false))
$started = [DateTime]::UtcNow
& (Join-Path $environment.roots.vscode 'bin/code.cmd') --user-data-dir $environment.editor.user_data --extensions-dir $environment.editor.extensions --profile HibikiPhysics --extensionDevelopmentPath (Join-Path $repository 'tools/vscode/acceptance') $workspacePath
if ($LASTEXITCODE -ne 0) { throw 'Editor acceptance host launch failed.' }
$deadline = $started.AddMinutes(5)
$previous = ''
while ([DateTime]::UtcNow -lt $deadline) {
    if (Test-Path -LiteralPath $reportPath) {
        try { $report = Get-Content -LiteralPath $reportPath -Raw | ConvertFrom-Json }
        catch { Start-Sleep -Milliseconds 500; continue }
        $reportStart = if ($report.started_at -is [DateTime]) { $report.started_at.ToUniversalTime() } else { [DateTimeOffset]::Parse($report.started_at).UtcDateTime }
        if ($reportStart -ge $started) {
            $state = "$($report.status): $(@($report.checks.PSObject.Properties).Count) completed checks"
            if ($state -ne $previous) { Write-Host $state; $previous = $state }
            if ($report.status -eq 'failed') { throw $report.error }
            if ($report.status -eq 'passed') {
                Write-Host "Editor acceptance passed: $reportPath"
                exit 0
            }
        }
    }
    Start-Sleep -Milliseconds 500
}
throw "Editor acceptance timed out; inspect the live host and $reportPath"
}
finally { if ($held) { $mutex.ReleaseMutex() }; $mutex.Dispose() }
