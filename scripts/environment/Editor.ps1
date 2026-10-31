function Initialize-HibikiEditor {
    param([string]$RepositoryRoot, [string]$ArchiveDirectory, $Roots)
    $ErrorActionPreference = 'Stop'
    $manifest = Get-Content -LiteralPath (Join-Path $RepositoryRoot 'tools/toolchain-lock.json') -Raw | ConvertFrom-Json
    $editorRoot = Join-Path $env:LOCALAPPDATA 'HibikiPhysics/vscode'
    $extensions = Join-Path $editorRoot 'extensions'
    $userData = Join-Path $editorRoot 'user-data'
    $cli = Join-Path $Roots.vscode 'bin/code.cmd'
    $profileCheck = & $cli --user-data-dir $userData --extensions-dir $extensions --profile HibikiPhysics --list-extensions 2>&1
    if ($LASTEXITCODE -ne 0) {
        if (($profileCheck | Out-String) -notmatch "Profile 'HibikiPhysics' not found") { throw ($profileCheck | Out-String) }
        & $cli --user-data-dir $userData --extensions-dir $extensions --profile HibikiPhysics --new-window $RepositoryRoot
        $created = $false
        for ($attempt = 0; $attempt -lt 20; $attempt++) {
            & $cli --user-data-dir $userData --extensions-dir $extensions --profile HibikiPhysics --list-extensions 2>&1 | Out-Null
            if ($LASTEXITCODE -eq 0) { $created = $true; break }
            Start-Sleep -Milliseconds 500
        }
        if (-not $created) { throw 'Dedicated editor profile creation remains pending.' }
    }
    $configPath = Join-Path $RepositoryRoot '.local/environment.json'
    $config = Get-Content -LiteralPath $configPath -Raw | ConvertFrom-Json
    $config.paths | Add-Member -NotePropertyName extensions -NotePropertyValue $extensions -Force
    $orderedIds = @('ms-python.python','ms-python.debugpy','ms-vscode.cpptools',
        'llvm-vs-code-extensions.vscode-clangd','detachhead.basedpyright','charliermarsh.ruff')
    foreach ($id in $orderedIds) {
        $artifact = $manifest.artifacts | Where-Object id -eq $id
        $archive = Join-Path $ArchiveDirectory $artifact.filename
        if ((Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash -ne $artifact.sha256) { throw "VSIX hash mismatch: $id" }
        $installed = @(& $cli --user-data-dir $userData --extensions-dir $extensions --profile HibikiPhysics --list-extensions --show-versions)
        if ($LASTEXITCODE -ne 0) { throw 'Editor profile extension inventory failed.' }
        if ("$id@$($artifact.version)" -notin $installed) {
            & $cli --user-data-dir $userData --extensions-dir $extensions --profile HibikiPhysics --install-extension $archive --force --do-not-include-pack-dependencies
            if ($LASTEXITCODE -ne 0) { throw "Extension installation failed: $id" }
        }
        else { Write-Host "Reusing extension $id $($artifact.version)" }
        $folders = @(Get-ChildItem -LiteralPath $extensions -Directory | Where-Object { $_.Name -like "$id-$($artifact.version)*" })
        if ($folders.Count -ne 1) { throw "Exact extension directory required: $id" }
        $config.paths | Add-Member -NotePropertyName $id -NotePropertyValue $folders[0].FullName -Force
        & (Join-Path $Roots.python 'python.exe') (Join-Path $RepositoryRoot 'scripts/environment/archive_inventory.py') --artifact $id --archive $archive --root $folders[0].FullName --output (Join-Path $RepositoryRoot ".local/receipts/$id.json")
        if ($LASTEXITCODE -ne 0) { throw "Installed extension verification failed: $id" }
    }
    $config | Add-Member -NotePropertyName editor -NotePropertyValue @{ user_data = $userData; extensions = $extensions; profile = 'HibikiPhysics' } -Force
    [IO.File]::WriteAllText($configPath, ($config | ConvertTo-Json -Depth 10), [Text.UTF8Encoding]::new($false))
    & (Join-Path $Roots.python 'python.exe') (Join-Path $RepositoryRoot 'scripts/environment/editor_settings.py')
    if ($LASTEXITCODE -ne 0) { throw 'Editor template rendering failed.' }
}
