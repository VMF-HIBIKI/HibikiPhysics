function Initialize-HibikiArchives {
    param([string]$RepositoryRoot, [string]$ArchiveDirectory,
        [string]$ToolsRoot = (Join-Path $env:LOCALAPPDATA 'HibikiPhysics/tools'))
    $ErrorActionPreference = 'Stop'
    . (Join-Path $PSScriptRoot 'Inventory.ps1')
    $manifest = Get-Content -LiteralPath (Join-Path $RepositoryRoot 'tools/toolchain-lock.json') -Raw | ConvertFrom-Json
    $toolsRootPath = [IO.Path]::GetFullPath($ToolsRoot)
    [IO.Directory]::CreateDirectory($toolsRootPath) | Out-Null
    $result = @{}
    foreach ($artifact in @($manifest.artifacts | Where-Object { $_.id -in @('llvm','cmake','ninja','uv','python','vscode') })) {
        $archive = Join-Path $ArchiveDirectory $artifact.filename
        if (-not (Test-Path -LiteralPath $archive -PathType Leaf)) { throw "Archive required: $archive" }
        if ((Get-Item -LiteralPath $archive).Length -ne $artifact.size -or
            (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash -ne $artifact.sha256) { throw "Archive integrity mismatch: $archive" }
        $destination = [IO.Path]::GetFullPath((Join-Path $toolsRootPath "$($artifact.id)/$($artifact.version)"))
        $marker = Join-Path $destination '.hibiki-archive-sha256'
        if (Test-Path -LiteralPath $marker) {
            if ((Get-Content -LiteralPath $marker -Raw).Trim() -eq $artifact.sha256) {
                $localReceipt = Join-Path $RepositoryRoot ".local/receipts/$($artifact.id).json"
                $sharedReceipt = Join-Path $destination '.hibiki-payloads.json'
                if (-not (Test-Path -LiteralPath $localReceipt) -and (Test-Path -LiteralPath $sharedReceipt)) {
                    [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($localReceipt)) | Out-Null
                    Copy-Item -LiteralPath $sharedReceipt -Destination $localReceipt
                }
                $evidence = Get-HibikiPayloadEvidence $RepositoryRoot $artifact $destination -Directory
                if ($evidence.status -ne 'matched') { throw "Existing tool payload requires review: $($artifact.id): $($evidence.detail)" }
                $result[$artifact.id] = $destination
                Write-Host "Reusing $($artifact.id) $($artifact.version)"
                continue
            }
        }
        if (Test-Path -LiteralPath $destination) { throw "Existing unverified destination requires inspection: $destination" }
        $staging = Join-Path $toolsRootPath ('.staging-' + [Guid]::NewGuid().ToString('N'))
        [IO.Directory]::CreateDirectory($staging) | Out-Null
        Write-Host "Extracting $($artifact.id) $($artifact.version)"
        if ($archive.EndsWith('.zip')) {
            Add-Type -AssemblyName System.IO.Compression.FileSystem
            [IO.Compression.ZipFile]::ExtractToDirectory($archive, $staging)
        }
        else {
            $tar = Join-Path $env:SystemRoot 'System32/tar.exe'
            $entries = & $tar -tf $archive
            if ($LASTEXITCODE -ne 0) { throw "Archive listing failed: $archive" }
            foreach ($entry in $entries) {
                if ($entry -match '(^[/\\]|^[A-Za-z]:|(^|[/\\])\.\.([/\\]|$))') { throw "Unsafe archive member: $entry" }
            }
            & $tar -xf $archive -C $staging
            if ($LASTEXITCODE -ne 0) { throw "Archive extraction failed: $archive" }
        }
        $children = @(Get-ChildItem -LiteralPath $staging -Force)
        $source = $staging
        if ($children.Count -eq 1 -and $children[0].PSIsContainer) { $source = $children[0].FullName }
        foreach ($candidate in @($source, $destination)) {
            if (-not [IO.Path]::GetFullPath($candidate).StartsWith($toolsRootPath + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
                throw "Tool move must stay inside $toolsRootPath"
            }
        }
        [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($destination)) | Out-Null
        Move-Item -LiteralPath $source -Destination $destination
        [IO.File]::WriteAllText($marker, $artifact.sha256 + "`n", [Text.UTF8Encoding]::new($false))
        $result[$artifact.id] = $destination
    }
    return $result
}

function Get-HibikiArchives {
    param([string]$RepositoryRoot, [string]$ImportDirectory, [string]$CacheRoot, [switch]$Offline)
    $ErrorActionPreference = 'Stop'
    $manifest = Get-Content -LiteralPath (Join-Path $RepositoryRoot 'tools/toolchain-lock.json') -Raw | ConvertFrom-Json
    $flat = Join-Path $RepositoryRoot '.local/archives'
    [IO.Directory]::CreateDirectory($flat) | Out-Null
    foreach ($artifact in $manifest.artifacts) {
        if ($artifact.sha256 -notmatch '^[0-9a-f]{64}$' -or [IO.Path]::GetFileName($artifact.filename) -ne $artifact.filename) {
            throw "Invalid archive identity: $($artifact.id)"
        }
        $directory = Join-Path $CacheRoot $artifact.sha256
        $cached = Join-Path $directory $artifact.filename
        $local = Join-Path $flat $artifact.filename
        if (-not (Test-Path -LiteralPath $cached)) {
            $import = $null
            if ($ImportDirectory) { $import = Join-Path $ImportDirectory $artifact.filename }
            if ($import -and (Test-Path -LiteralPath $import -PathType Leaf)) {
                if ((Get-FileHash -LiteralPath $import -Algorithm SHA256).Hash -ne $artifact.sha256) { throw "Imported archive hash mismatch: $import" }
                [IO.Directory]::CreateDirectory($directory) | Out-Null
                Copy-Item -LiteralPath $import -Destination $cached
            }
            else {
                if ($Offline) { throw "Offline archive missing: $($artifact.id) $($artifact.sha256)" }
                [IO.Directory]::CreateDirectory($directory) | Out-Null
                $partial = "$cached.part"
                $curl = Join-Path $env:SystemRoot 'System32/curl.exe'
                & $curl --fail --location --retry 3 --continue-at - --output $partial $artifact.url
                if ($LASTEXITCODE -ne 0) { throw "Download incomplete; partial retained: $partial" }
                if ((Get-FileHash -LiteralPath $partial -Algorithm SHA256).Hash -ne $artifact.sha256) { throw "Downloaded archive hash mismatch; retained: $partial" }
                Move-Item -LiteralPath $partial -Destination $cached
            }
        }
        if ((Get-Item -LiteralPath $cached).Length -ne $artifact.size -or
            (Get-FileHash -LiteralPath $cached -Algorithm SHA256).Hash -ne $artifact.sha256) { throw "Cached archive identity mismatch: $cached" }
        if (-not (Test-Path -LiteralPath $local)) {
            # Same-volume hard links keep the checkout view small; copying supports other volumes.
            try { New-Item -ItemType HardLink -Path $local -Target $cached -ErrorAction Stop | Out-Null }
            catch { Copy-Item -LiteralPath $cached -Destination $local }
        }
        if ((Get-FileHash -LiteralPath $local -Algorithm SHA256).Hash -ne $artifact.sha256) { throw "Checkout archive view differs: $local" }
    }
    return $flat
}

function Initialize-HibikiEnvironment {
    param([string]$RepositoryRoot, [string]$ArchiveDirectory, [switch]$Offline,
        [switch]$RequireWarp, [switch]$WithEditor, [switch]$EnableHooks,
        [string]$ConfigPath, [hashtable]$Paths = @{})
    $ErrorActionPreference = 'Stop'
    $state = Invoke-HibikiDoctor -RepositoryRoot $RepositoryRoot -ConfigPath $ConfigPath -Paths $Paths -Scope cpp
    $systemFailures = @($state.checks | Where-Object {
        ($_.id -in @('manifest','overrides','host','visualStudio','visualStudio.version') -or $_.id -match '^(msvc\.|sdk\.|package\.)') -and $_.status -ne 'matched'
    })
    if ($systemFailures.Count -gt 0) { throw "Selected system prerequisites require review: $($systemFailures.id -join ', ')" }
    $cache = Join-Path $env:LOCALAPPDATA 'HibikiPhysics/cache'
    $archives = Get-HibikiArchives $RepositoryRoot $ArchiveDirectory (Join-Path $cache 'downloads') -Offline:$Offline
    $roots = Initialize-HibikiArchives $RepositoryRoot $archives $state.selected_paths.toolsRoot
    $python = Join-Path $roots.python 'python.exe'
    $manifest = Get-Content -LiteralPath (Join-Path $RepositoryRoot 'tools/toolchain-lock.json') -Raw | ConvertFrom-Json
    foreach ($id in @('python','llvm','cmake','ninja','uv','vscode')) {
        $artifact = $manifest.artifacts | Where-Object id -eq $id
        & $python (Join-Path $RepositoryRoot 'scripts/environment/archive_inventory.py') --artifact $id --archive (Join-Path $archives $artifact.filename) --root $roots[$id] --output (Join-Path $RepositoryRoot ".local/receipts/$id.json")
        if ($LASTEXITCODE -ne 0) { throw "Installed payload verification failed: $id" }
        Copy-Item -LiteralPath (Join-Path $RepositoryRoot ".local/receipts/$id.json") -Destination (Join-Path $roots[$id] '.hibiki-payloads.json')
    }
    $paths = @{
        toolsRoot = $state.selected_paths.toolsRoot
        msvc = $state.selected_paths.msvc; sdk = $state.selected_paths.sdk; visualStudio = $state.selected_paths.visualStudio
        cmake = (Join-Path $roots.cmake 'bin/cmake.exe'); ninja = (Join-Path $roots.ninja 'ninja.exe')
        uv = (Join-Path $roots.uv 'uv.exe'); python = (Join-Path $RepositoryRoot '.venv/Scripts/python.exe')
        vscode = (Join-Path $roots.vscode 'Code.exe')
    }
    foreach ($tool in @('clang-cl','clangd','clang-format','clang-tidy','lld-link')) { $paths[$tool] = Join-Path $roots.llvm "bin/$tool.exe" }
    $configuration = @{ schema_version = 1; paths = $paths; roots = $roots; archives = $archives; base_python = $python }
    $configPath = Join-Path $RepositoryRoot '.local/environment.json'
    if (Test-Path -LiteralPath $configPath) {
        $previous = Get-Content -LiteralPath $configPath -Raw | ConvertFrom-Json
        if ($previous.editor) {
            $configuration.editor = $previous.editor
            foreach ($property in $previous.paths.PSObject.Properties) {
                if ($property.Name -eq 'extensions' -or $property.Name -in @($manifest.artifacts | Where-Object kind -eq 'vscode_extension' | ForEach-Object id)) {
                    $paths[$property.Name] = $property.Value
                }
            }
        }
    }
    [IO.File]::WriteAllText($configPath, ($configuration | ConvertTo-Json -Depth 10), [Text.UTF8Encoding]::new($false))
    $userPresets = @{ version = 6; configurePresets = @(); buildPresets = @(); testPresets = @() }
    foreach ($preset in @('windows-debug','windows-relwithdebinfo','windows-release','windows-asan','windows-msvc')) {
        $localName = "local-$preset"
        $userPresets.configurePresets += @{ name = $localName; inherits = $preset; cacheVariables = @{ HIBIKI_SOURCE_ARCHIVES = $archives } }
        $userPresets.buildPresets += @{ name = $localName; configurePreset = $localName; jobs = 8 }
        $userPresets.testPresets += @{ name = $localName; configurePreset = $localName; output = @{ outputOnFailure = $true } }
    }
    [IO.File]::WriteAllText((Join-Path $RepositoryRoot 'CMakeUserPresets.json'), ($userPresets | ConvertTo-Json -Depth 10), [Text.UTF8Encoding]::new($false))
    if (-not (Test-Path -LiteralPath (Join-Path $RepositoryRoot 'uv.lock'))) { throw 'The reviewed uv.lock file is required for preparation.' }
    $savedCache = $env:UV_CACHE_DIR; $savedDownloads = $env:UV_PYTHON_DOWNLOADS
    Push-Location $RepositoryRoot
    try {
        $env:UV_CACHE_DIR = Join-Path $cache 'uv'
        $env:UV_PYTHON_DOWNLOADS = 'never'
        $arguments = @('sync','--locked','--python',$python)
        if ($RequireWarp) { $arguments += @('--group','warp') }
        if ($Offline) { $arguments += '--offline' }
        & $paths.uv @arguments
        if ($LASTEXITCODE -ne 0) { throw 'Locked Python synchronization failed.' }
        $inventoryArguments = @((Join-Path $RepositoryRoot 'scripts/environment/python_inventory.py'))
        if ($RequireWarp) { $inventoryArguments += '--warp' }
        & $paths.python @inventoryArguments
        if ($LASTEXITCODE -ne 0) { throw 'Locked Python installed-file verification failed.' }
        if ($WithEditor) {
            . (Join-Path $PSScriptRoot 'Editor.ps1')
            Initialize-HibikiEditor $RepositoryRoot $archives ([pscustomobject]$roots)
        }
        if ($EnableHooks) {
            & (Join-Path $RepositoryRoot '.venv/Scripts/prek.exe') install
            if ($LASTEXITCODE -ne 0) { throw 'Commit hook installation failed.' }
        }
    }
    finally { Pop-Location; $env:UV_CACHE_DIR = $savedCache; $env:UV_PYTHON_DOWNLOADS = $savedDownloads }
}
