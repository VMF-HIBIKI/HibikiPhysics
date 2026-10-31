function Resolve-HibikiPath {
    param([string]$Root, [string]$Path)
    if ([string]::IsNullOrWhiteSpace($Path)) { throw 'A path must contain a value.' }
    if ([IO.Path]::IsPathRooted($Path)) { return [IO.Path]::GetFullPath($Path) }
    return [IO.Path]::GetFullPath((Join-Path $Root $Path))
}

function Read-HibikiJson {
    param([string]$Path)
    return (Get-Content -LiteralPath $Path -Raw -Encoding UTF8 | ConvertFrom-Json)
}

function Get-HibikiArchitecture {
    param([string]$Path)
    $stream = [IO.File]::OpenRead($Path)
    $reader = [IO.BinaryReader]::new($stream)
    try {
        if ($reader.ReadUInt16() -ne 0x5a4d) { return 'unknown' }
        $stream.Position = 0x3c
        $offset = $reader.ReadInt32()
        if ($offset -lt 0 -or $offset -gt $stream.Length - 6) { return 'unknown' }
        $stream.Position = $offset
        if ($reader.ReadUInt32() -ne 0x4550) { return 'unknown' }
        switch ($reader.ReadUInt16()) {
            0x8664 { return 'x64' }
            0x014c { return 'x86' }
            0xaa64 { return 'arm64' }
            default { return 'unknown' }
        }
    }
    finally { $reader.Dispose() }
}

function Get-HibikiBinaryIdentity {
    param([string]$Path)
    $file = Get-Item -LiteralPath $Path
    $info = [Diagnostics.FileVersionInfo]::GetVersionInfo($file.FullName)
    $version = $info.ProductVersion
    if (-not $version) { $version = $info.FileVersion }
    $parsed = [regex]::Match([string]$version, '^\s*(\d+\.\d+\.\d+)(?:\.0)?(?:\s|$)')
    return [pscustomobject]@{
        version = $parsed.Groups[1].Value
        raw_version = $version
        architecture = Get-HibikiArchitecture $file.FullName
        sha256 = (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
    }
}

function Add-HibikiCheck {
    param($Checks, [string]$Id, [string]$Status, [string]$Path, $Expected, $Actual,
        [string]$Detail, [bool]$Required = $true,
        [string]$Next = 'Review the selected path and identity; prepare the pinned component during ENV-004.')
    $Checks.Add([pscustomobject]@{
        id = $Id; status = $Status; required = $Required; path = $Path
        expected = $Expected; actual = $Actual; detail = $Detail
        next_step = $(if ($Status -eq 'matched') { '' } else { $Next })
    }) | Out-Null
}

function Test-HibikiBinary {
    param($Checks, [string]$Id, [string]$Path, [string]$Version)
    if (-not $Path -or -not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        Add-HibikiCheck $Checks $Id 'missing' $Path $Version $null 'Executable file required.'
        return
    }
    try {
        $actual = Get-HibikiBinaryIdentity $Path
        $status = 'matched'
        if ($actual.architecture -ne 'x64') { $status = 'mismatch' }
        elseif (-not $actual.version) { $status = 'pending' }
        elseif ($actual.version -ne $Version) { $status = 'mismatch' }
        Add-HibikiCheck $Checks $Id $status $Path @{ version = $Version; architecture = 'x64' } $actual 'PE architecture and version-resource inspection.'
        Add-HibikiCheck $Checks "$Id.provenance" 'pending' $Path 'Verified installed payload inventory' $actual.sha256 'Observed file hash; archive-to-installed-file identity awaits ENV-004.'
    }
    catch { Add-HibikiCheck $Checks $Id 'pending' $Path $Version $null $_.Exception.Message }
}

function Select-HibikiPath {
    param([string]$Root, [hashtable]$Paths, [string]$Key, [string[]]$Candidates, [string]$Command)
    if ($Paths.ContainsKey($Key)) { return (Resolve-HibikiPath $Root $Paths[$Key]) }
    foreach ($candidate in $Candidates) {
        if ($candidate -and (Test-Path -LiteralPath $candidate)) { return [IO.Path]::GetFullPath($candidate) }
    }
    if ($Command) {
        $application = Get-Command $Command -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($application) { return $application.Source }
    }
    if ($Candidates.Count -gt 0) { return $Candidates[0] }
    return $null
}

function Invoke-HibikiDoctor {
    param([string]$RepositoryRoot, [string]$ConfigPath, [hashtable]$Paths = @{}, [switch]$RequireWarp,
        [ValidateSet('full','cpp')][string]$Scope = 'full')
    $ErrorActionPreference = 'Stop'
    . (Join-Path $PSScriptRoot 'Inventory.ps1')
    $checks = [Collections.Generic.List[object]]::new()
    $root = [IO.Path]::GetFullPath($RepositoryRoot)
    $selected = @{}
    $manifestPath = Join-Path $root 'tools/toolchain-lock.json'
    try {
        $manifest = Read-HibikiJson $manifestPath
        if ($manifest.schema_version -ne 1 -or $manifest.target -ne 'windows-x86_64') { throw 'Supported manifest: schema 1, windows-x86_64.' }
        $artifacts = @{}
        foreach ($artifact in $manifest.artifacts) {
            if ($artifacts.ContainsKey($artifact.id)) { throw "Duplicate artifact: $($artifact.id)" }
            if ($artifact.sha256 -notmatch '^[0-9a-f]{64}$' -or $artifact.size -le 0 -or
                $artifact.verification -notin @('publisher_hash_matched', 'downloaded_and_hashed')) { throw "Incomplete artifact identity: $($artifact.id)" }
            if ($artifact.publisher_sha256 -and $artifact.publisher_sha256 -ne $artifact.sha256) { throw "Publisher hash conflict: $($artifact.id)" }
            $artifacts[$artifact.id] = $artifact
        }
        $requiredIds = @('llvm','cmake','ninja','uv','python','vscode','eigen','googletest','spdlog','benchmark',
            'pytest-wheel','ruff-wheel','basedpyright-wheel','debugpy-wheel','prek-wheel','warp-lang-wheel',
            'ms-vscode.cpptools','llvm-vs-code-extensions.vscode-clangd','ms-python.python','ms-python.debugpy',
            'detachhead.basedpyright','charliermarsh.ruff')
        foreach ($id in $requiredIds) { if (-not $artifacts.ContainsKey($id)) { throw "Required artifact: $id" } }
        $systemPath = Resolve-HibikiPath $root $manifest.system_components.path
        if ((Get-FileHash -LiteralPath $systemPath -Algorithm SHA256).Hash -ne $manifest.system_components.sha256) { throw 'Windows component manifest hash mismatch.' }
        $system = Read-HibikiJson $systemPath
        if (@($system.files).Count -eq 0) { throw 'Windows file identities required.' }
        Add-HibikiCheck $checks 'manifest' 'matched' $manifestPath 1 1 'Manifest fields, required IDs and companion hash checked.'
    }
    catch {
        Add-HibikiCheck $checks 'manifest' 'mismatch' $manifestPath 'Complete identity records' $null $_.Exception.Message
        return (New-HibikiReport $checks $selected $root)
    }

    $effective = @{}
    $allowed = @('toolsRoot','visualStudio','msvc','sdk','extensions','python','uv','cmake','ninja','vscode',
        'clang-cl','clangd','clang-format','clang-tidy','lld-link') + @($manifest.artifacts | Where-Object kind -eq 'vscode_extension' | ForEach-Object id)
    try {
        $explicitConfig = [bool]$ConfigPath
        if (-not $ConfigPath) { $ConfigPath = '.local/environment.json' }
        $ConfigPath = Resolve-HibikiPath $root $ConfigPath
        if (Test-Path -LiteralPath $ConfigPath -PathType Leaf) {
            $config = Read-HibikiJson $ConfigPath
            if ($config.schema_version -ne 1 -or -not $config.paths) { throw 'Local configuration requires schema_version 1 and a paths object.' }
            foreach ($property in $config.paths.PSObject.Properties) { $effective[$property.Name] = $property.Value }
        }
        elseif ($explicitConfig) { throw "Explicit configuration file missing: $ConfigPath" }
        foreach ($key in $Paths.Keys) { $effective[$key] = $Paths[$key] }
        foreach ($key in @($effective.Keys)) {
            if ($key -notin $allowed -or $effective[$key] -isnot [string] -or [string]::IsNullOrWhiteSpace($effective[$key])) { throw "Invalid path override: $key" }
            $effective[$key] = Resolve-HibikiPath $root $effective[$key]
        }
        Add-HibikiCheck $checks 'overrides' 'matched' $ConfigPath 'Recognized path keys' $effective 'Command-line overrides take precedence over local configuration.'
    }
    catch {
        Add-HibikiCheck $checks 'overrides' 'mismatch' $ConfigPath 'Valid explicit paths' $null $_.Exception.Message
        return (New-HibikiReport $checks $selected $root)
    }

    $hostArchitecture = $env:PROCESSOR_ARCHITEW6432
    if (-not $hostArchitecture) { $hostArchitecture = $env:PROCESSOR_ARCHITECTURE }
    $hostStatus = 'matched'
    if ($env:OS -ne 'Windows_NT' -or $hostArchitecture -ne 'AMD64') { $hostStatus = 'mismatch' }
    Add-HibikiCheck $checks 'host' $hostStatus '' 'Windows AMD64' $hostArchitecture 'Host operating-system architecture.'
    $toolsRoot = Join-Path $env:LOCALAPPDATA 'HibikiPhysics/tools'
    if ($effective.ContainsKey('toolsRoot')) { $toolsRoot = $effective.toolsRoot }
    $selected.toolsRoot = $toolsRoot
    $tools = @('clang-cl','clangd','clang-format','clang-tidy','lld-link','cmake','ninja')
    if ($Scope -eq 'full') { $tools += @('uv','python','vscode') }
    foreach ($tool in $tools) {
        $artifactId = $tool
        if ($tool -in @('clang-cl','clangd','clang-format','clang-tidy','lld-link')) { $artifactId = 'llvm' }
        $version = $artifacts[$artifactId].version
        $directory = Join-Path $toolsRoot "$artifactId/$version"
        $executable = "$tool.exe"
        if ($tool -eq 'vscode') { $executable = 'Code.exe' }
        $candidates = @((Join-Path $directory "bin/$executable"), (Join-Path $directory $executable))
        if ($tool -eq 'python') { $candidates = @((Join-Path $root '.venv/Scripts/python.exe')) }
        if ($artifactId -eq 'llvm') { $candidates += Join-Path $env:ProgramFiles "LLVM/bin/$executable" }
        if ($tool -eq 'vscode') { $candidates += Join-Path $env:LOCALAPPDATA 'Programs/Microsoft VS Code/Code.exe' }
        $path = Select-HibikiPath $root $effective $tool $candidates $executable
        $selected[$tool] = $path
        Test-HibikiBinary $checks $tool $path $version
        if ($path -and (Test-Path -LiteralPath $path -PathType Leaf)) {
            $evidence = Get-HibikiPayloadEvidence $root $artifacts[$artifactId] $path
            $provenance = $checks | Where-Object id -eq "$tool.provenance" | Select-Object -First 1
            if ($provenance) {
                $provenance.status = $evidence.status
                $provenance.detail = $evidence.detail
                $provenance.expected = $artifacts[$artifactId].sha256
                if ($evidence.status -eq 'matched') {
                    $provenance.next_step = ''
                    $binary = $checks | Where-Object id -eq $tool | Select-Object -First 1
                    if ($binary.status -eq 'pending' -and $binary.actual.architecture -eq 'x64') {
                        $binary.status = 'matched'
                        $binary.detail = 'Version identified by the pinned archive payload; PE architecture matches.'
                        $binary.next_step = ''
                    }
                }
            }
        }
    }

    $vsCandidates = @()
    $vsRecords = @{}
    $preferredVs = @()
    $instances = Join-Path $env:ProgramData 'Microsoft/VisualStudio/Packages/_Instances'
    if (Test-Path -LiteralPath $instances) {
        foreach ($instance in @(Get-ChildItem -LiteralPath $instances -Directory | Sort-Object Name)) {
            try {
                $state = Read-HibikiJson (Join-Path $instance.FullName 'state.json')
                if ($state.installationPath) {
                    $vsCandidates += $state.installationPath
                    $vsRecords[$state.installationPath] = @{ state = $state; directory = $instance.FullName }
                    if ($state.installationVersion -eq $system.visual_studio_installation_version) { $preferredVs += $state.installationPath }
                }
            }
            catch { Add-HibikiCheck $checks "vs.catalog.$($instance.Name)" 'pending' $instance.FullName 'Readable installation state' $null $_.Exception.Message }
        }
    }
    $matchingVs = @($vsCandidates | Where-Object { Test-Path -LiteralPath (Join-Path $_ "VC/Tools/MSVC/$($system.msvc_directory_version)") })
    $selected.visualStudio = Select-HibikiPath $root $effective 'visualStudio' ($preferredVs + $matchingVs + $vsCandidates) ''
    $vsStatus = 'missing'
    if ($selected.visualStudio -and (Test-Path -LiteralPath $selected.visualStudio -PathType Container)) { $vsStatus = 'matched' }
    Add-HibikiCheck $checks 'visualStudio' $vsStatus $selected.visualStudio 'Visual Studio installation directory' $selected.visualStudio 'Installation discovery from local Visual Studio instance records.'
    $record = $null
    if ($selected.visualStudio) { $record = $vsRecords[$selected.visualStudio] }
    if ($record) {
        $versionStatus = 'matched'
        if ($record.state.installationVersion -ne $system.visual_studio_installation_version) { $versionStatus = 'mismatch' }
        Add-HibikiCheck $checks 'visualStudio.version' $versionStatus $selected.visualStudio $system.visual_studio_installation_version $record.state.installationVersion 'Visual Studio installation revision.'
        try {
            $installedPackages = (Read-HibikiJson (Join-Path $record.directory 'state.packages.json')).packages
            $toolsetPrefix = (($system.msvc_directory_version -split '\.')[0..1] -join '.')
            $sdkPrefix = (($system.sdk_directory_version -split '\.')[0..2] -join '.')
            $corePackages = @("Microsoft.VC.$toolsetPrefix.Tools.HostX64.TargetX64.base", "Microsoft.VC.$toolsetPrefix.CRT.Headers.base",
                "Microsoft.VC.$toolsetPrefix.CRT.x64.Desktop.base", "Microsoft.VC.$toolsetPrefix.Props.x64", "Win11SDK_$sdkPrefix")
            foreach ($id in $corePackages) {
                $expected = @($system.packages | Where-Object id -eq $id | Select-Object -ExpandProperty version -Unique)
                $actual = @($installedPackages | Where-Object id -eq $id | Select-Object -ExpandProperty version -Unique)
                $status = 'mismatch'
                if ($actual.Count -eq 0) { $status = 'missing' }
                elseif ($expected.Count -eq 1 -and $actual.Count -eq 1 -and $actual[0] -eq $expected[0]) { $status = 'matched' }
                Add-HibikiCheck $checks "package.$id" $status $record.directory $expected $actual 'Installed compiler, CRT or SDK catalog package revision.'
            }
        }
        catch { Add-HibikiCheck $checks 'visualStudio.packages' 'pending' $record.directory 'Readable installed package inventory' $null $_.Exception.Message }
    }
    else { Add-HibikiCheck $checks 'visualStudio.inventory' 'pending' $selected.visualStudio 'Matching installation state record' $null 'Selected installation metadata requires confirmation.' }
    $msvcCandidates = @()
    if ($selected.visualStudio) { $msvcCandidates += Join-Path $selected.visualStudio "VC/Tools/MSVC/$($system.msvc_directory_version)" }
    $selected.msvc = Select-HibikiPath $root $effective 'msvc' $msvcCandidates ''
    $selected.sdk = Select-HibikiPath $root $effective 'sdk' @((Join-Path ${env:ProgramFiles(x86)} 'Windows Kits/10')) ''
    try {
        $uninstall = 'HKLM:\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall'
        $displayPrefix = (($system.sdk_display_version -split '\.')[0..2] -join '.') + '.*'
        $sdkVersions = @(Get-ChildItem -LiteralPath $uninstall | Get-ItemProperty | Where-Object {
            $_.DisplayName -like 'Windows Software Development Kit*' -and $_.DisplayVersion -like $displayPrefix
        } | Select-Object -ExpandProperty DisplayVersion -Unique)
        $status = 'mismatch'
        if ($sdkVersions.Count -eq 0) { $status = 'missing' }
        elseif ($system.sdk_display_version -in $sdkVersions) { $status = 'matched' }
        Add-HibikiCheck $checks 'sdk.servicing' $status $uninstall $system.sdk_display_version $sdkVersions 'Host SDK servicing registration; selected SDK file hashes are checked separately.'
    }
    catch { Add-HibikiCheck $checks 'sdk.servicing' 'pending' '' $system.sdk_display_version $null $_.Exception.Message }
    foreach ($file in $system.files) {
        $path = $null
        if ($selected[$file.root]) { $path = Join-Path $selected[$file.root] $file.path }
        try {
            if (-not $path -or -not (Test-Path -LiteralPath $path -PathType Leaf)) {
                Add-HibikiCheck $checks "$($file.root).$($file.path)" 'missing' $path $file.sha256 $null 'Required toolset or SDK file.'
                continue
            }
            $hash = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()
            $status = 'matched'
            if ($hash -ne $file.sha256 -or (Get-Item -LiteralPath $path).Length -ne $file.size) { $status = 'mismatch' }
            Add-HibikiCheck $checks "$($file.root).$($file.path)" $status $path $file.sha256 $hash 'Selected system-file content identity.'
        }
        catch { Add-HibikiCheck $checks "$($file.root).$($file.path)" 'pending' $path $file.sha256 $null $_.Exception.Message }
    }

    if ($Scope -eq 'cpp') { return (New-HibikiReport $checks $selected $root) }
    $declaredPython = (Get-Content -LiteralPath (Join-Path $root '.python-version') -Raw).Trim()
    $pythonStatus = 'matched'
    if ($declaredPython -ne $manifest.python.version -or $declaredPython -ne $artifacts.python.version) { $pythonStatus = 'mismatch' }
    Add-HibikiCheck $checks 'python.declarations' $pythonStatus (Join-Path $root '.python-version') $artifacts.python.version $declaredPython 'Repeated interpreter version fields.'
    $venvConfig = Join-Path $root '.venv/pyvenv.cfg'
    $venvStatus = 'missing'
    $venvVersion = $null
    if (Test-Path -LiteralPath $venvConfig -PathType Leaf) {
        $venvText = Get-Content -LiteralPath $venvConfig -Raw
        $venvVersion = [regex]::Match($venvText, '(?m)^version(?:_info)?\s*=\s*(\d+\.\d+\.\d+)').Groups[1].Value
        $venvStatus = 'mismatch'
        $projectInterpreter = Join-Path $root '.venv/Scripts/python.exe'
        if ($venvVersion -eq $declaredPython -and $selected.python -eq $projectInterpreter) { $venvStatus = 'matched' }
        elseif (-not $venvVersion) { $venvStatus = 'pending' }
    }
    Add-HibikiCheck $checks 'python.environment' $venvStatus $venvConfig $declaredPython $venvVersion 'Project virtual-environment metadata and selected interpreter path.'
    $projectText = Get-Content -LiteralPath (Join-Path $root 'pyproject.toml') -Raw -Encoding UTF8
    $dependencyBlock = [regex]::Match($projectText, '(?ms)^\[dependency-groups\]\s*(.*?)(?=^\[|\z)').Groups[1].Value
    foreach ($wheel in @($manifest.artifacts | Where-Object kind -eq 'python_wheel')) {
        $name = $wheel.id -replace '-wheel$', ''
        $required = $name -ne 'warp-lang' -or $RequireWarp
        $group = 'dev'
        if ($name -eq 'warp-lang') { $group = 'warp' }
        $groupText = [regex]::Match($dependencyBlock, '(?ms)^\s*' + $group + '\s*=\s*\[(.*?)\]').Groups[1].Value
        $groupText = ($groupText -split '\r?\n' | Where-Object { $_ -notmatch '^\s*#' }) -join "`n"
        $pattern = '"' + [regex]::Escape($name) + '==([^"\r\n]+)"'
        $declaration = [regex]::Matches($groupText, $pattern)
        $status = 'matched'
        if ($declaration.Count -ne 1 -or $declaration[0].Groups[1].Value -ne $wheel.version) { $status = 'mismatch' }
        Add-HibikiCheck $checks "$name.declaration" $status (Join-Path $root 'pyproject.toml') $wheel.version @($declaration | ForEach-Object { $_.Groups[1].Value }) 'Exact direct dependency declaration.'
        $site = Join-Path $root '.venv/Lib/site-packages'
        $metadataFiles = @()
        if (Test-Path -LiteralPath $site) {
            $normalized = $name -replace '-', '_'
            $metadataFiles = @(Get-ChildItem -LiteralPath $site -Directory | Where-Object { $_.Name -like "$normalized-*.dist-info" } | ForEach-Object { Join-Path $_.FullName 'METADATA' })
        }
        $status = 'missing'; $actual = @()
        foreach ($metadataPath in $metadataFiles) {
            if (Test-Path -LiteralPath $metadataPath -PathType Leaf) {
                $content = Get-Content -LiteralPath $metadataPath -Raw
                $actual += [regex]::Match($content, '(?m)^Version: (.+)\r?$').Groups[1].Value.Trim()
            }
        }
        if ($actual.Count -gt 0) {
            $status = 'mismatch'
            if ($actual.Count -eq 1 -and $actual[0] -eq $wheel.version) { $status = 'matched' }
        }
        Add-HibikiCheck $checks "$name.installed" $status $site $wheel.version $actual 'Project environment distribution metadata.' $required
    }
    $lockPath = Join-Path $root 'uv.lock'
    $pythonReceipt = Join-Path $root '.local/python-environment.json'
    $lockStatus = 'pending'
    $lockDetail = 'Reviewed lock and verified installed-environment receipt required.'
    $lockHash = $null
    try {
        if ((Test-Path -LiteralPath $lockPath) -and (Test-Path -LiteralPath $pythonReceipt)) {
            $receipt = Read-HibikiJson $pythonReceipt
            $lockHash = (Get-FileHash -LiteralPath $lockPath -Algorithm SHA256).Hash.ToLowerInvariant()
            if ($manifest.python.transitive_lock -ne 'locked' -or $receipt.schema_version -ne 1 -or
                $receipt.lock_sha256 -ne $lockHash -or $receipt.python_version -ne $declaredPython -or
                'dev' -notin $receipt.groups -or ($RequireWarp -and 'warp' -notin $receipt.groups)) { throw 'Python lock receipt identity or selected groups differ.' }
            $venvRoot = [IO.Path]::GetFullPath((Join-Path $root '.venv'))
            $fileEntries = @($receipt.files.PSObject.Properties)
            if ($fileEntries.Count -eq 0) { throw 'Python installed file inventory required.' }
            foreach ($entry in $fileEntries) {
                $file = [IO.Path]::GetFullPath((Join-Path $venvRoot $entry.Name))
                if (-not $file.StartsWith($venvRoot + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) { throw 'Python receipt path escapes the environment.' }
                if ((Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash -ne $entry.Value.sha256 -or
                    (Get-Item -LiteralPath $file).Length -ne $entry.Value.size) { throw "Python installed file differs: $file" }
            }
            $lockStatus = 'matched'
            $lockDetail = "Locked environment receipt and $($fileEntries.Count) installed file hashes match."
        }
    }
    catch { $lockStatus = 'mismatch'; $lockDetail = $_.Exception.Message }
    Add-HibikiCheck $checks 'python.lock' $lockStatus $lockPath 'Reviewed lock and verified installed payloads' $lockHash $lockDetail

    $extensionRoot = Select-HibikiPath $root $effective 'extensions' @((Join-Path $env:USERPROFILE '.vscode/extensions')) ''
    $selected.extensions = $extensionRoot
    foreach ($extension in @($manifest.artifacts | Where-Object kind -eq 'vscode_extension')) {
        $candidates = @()
        if (Test-Path -LiteralPath $extensionRoot -PathType Container) {
            $candidates = @(Get-ChildItem -LiteralPath $extensionRoot -Directory | Where-Object { $_.Name -like "$($extension.id)-$($extension.version)*" } | Sort-Object Name | ForEach-Object FullName)
            $candidates += @(Get-ChildItem -LiteralPath $extensionRoot -Directory | Where-Object { $_.Name -like "$($extension.id)-*" } | Sort-Object Name -Descending | ForEach-Object FullName)
        }
        $path = Select-HibikiPath $root $effective $extension.id $candidates ''
        $selected[$extension.id] = $path
        try {
            if (-not $path -or -not (Test-Path -LiteralPath (Join-Path $path 'package.json') -PathType Leaf)) {
                Add-HibikiCheck $checks $extension.id 'missing' $path $extension.version $null 'Selected extension package directory required.'
                continue
            }
            $package = Read-HibikiJson (Join-Path $path 'package.json')
            $status = 'matched'
            $platform = $package.__metadata.targetPlatform
            if (-not $platform -or $platform -eq 'undefined') { $platform = 'universal' }
            if ("$($package.publisher).$($package.name)" -ne $extension.id -or $package.version -ne $extension.version -or $platform -ne $extension.platform) { $status = 'mismatch' }
            Add-HibikiCheck $checks $extension.id $status $path @{ version = $extension.version; platform = $extension.platform } @{ version = $package.version; platform = $platform } 'Extension package identity and target platform.'
            $evidence = Get-HibikiPayloadEvidence $root $extension $path -Directory
            if ($evidence.status -eq 'matched' -and $platform -eq 'universal' -and
                "$($package.publisher).$($package.name)" -eq $extension.id -and $package.version -eq $extension.version) {
                $identityCheck = $checks | Where-Object id -eq $extension.id | Select-Object -First 1
                $identityCheck.status = 'matched'
                $identityCheck.actual.platform = $extension.platform
                $identityCheck.detail = 'Extension version matches; platform identified by the verified VSIX archive payload.'
                $identityCheck.next_step = ''
            }
            Add-HibikiCheck $checks "$($extension.id).provenance" $evidence.status $path $extension.sha256 $null $evidence.detail
        }
        catch { Add-HibikiCheck $checks $extension.id 'pending' $path $extension.version $null $_.Exception.Message }
    }
    return (New-HibikiReport $checks $selected $root)
}

function New-HibikiReport {
    param($Checks, $Selected, [string]$Root)
    $failed = @($Checks | Where-Object { $_.required -and $_.status -ne 'matched' }).Count
    return [pscustomobject]@{
        schema_version = 1; scope = 'windows-identity-diagnostics'; repository = $Root
        recorded_at = [DateTime]::UtcNow.ToString('o'); ready = ($failed -eq 0)
        required_failures = $failed; selected_paths = $Selected; checks = @($Checks.ToArray())
    }
}
