function Get-HibikiPayloadEvidence {
    param([string]$RepositoryRoot, $Artifact, [string]$Path, [switch]$Directory)
    $ErrorActionPreference = 'Stop'
    $receiptPath = Join-Path $RepositoryRoot ".local/receipts/$($Artifact.id).json"
    if (-not (Test-Path -LiteralPath $receiptPath -PathType Leaf)) {
        return [pscustomobject]@{ status = 'pending'; detail = 'Verified archive payload receipt required.'; sha256 = $null }
    }
    try {
        $receipt = Get-Content -LiteralPath $receiptPath -Raw -Encoding UTF8 | ConvertFrom-Json
        if ($receipt.schema_version -ne 1 -or $receipt.artifact -ne $Artifact.id -or
            $receipt.version -ne $Artifact.version -or $receipt.archive_sha256 -ne $Artifact.sha256) {
            throw 'Installed payload receipt differs from the selected archive.'
        }
        $rootPath = [IO.Path]::GetFullPath($receipt.root).TrimEnd('\','/')
        $target = [IO.Path]::GetFullPath($Path)
        $properties = @()
        if ($Directory) {
            if ($target.TrimEnd('\','/') -ne $rootPath) { throw 'Installation directory differs from the verified payload root.' }
            $properties = @($receipt.files.PSObject.Properties)
        }
        else {
            $relative = $null
            if ($target.StartsWith($rootPath + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
                $relative = $target.Substring($rootPath.Length + 1).Replace('\','/')
            }
            elseif ($Artifact.id -eq 'python' -and $target -eq (Join-Path $RepositoryRoot '.venv/Scripts/python.exe')) {
                $venv = Get-Content -LiteralPath (Join-Path $RepositoryRoot '.venv/pyvenv.cfg') -Raw
                $homePath = [regex]::Match($venv, '(?m)^home\s*=\s*(.+)$').Groups[1].Value.Trim()
                if ([IO.Path]::GetFullPath($homePath).TrimEnd('\','/') -ne $rootPath) { throw 'Virtual-environment base interpreter differs.' }
                $relative = 'Lib/venv/scripts/nt/python.exe'
            }
            if (-not $relative) { throw 'Selected executable has no matching payload path in the receipt.' }
            $properties = @($receipt.files.PSObject.Properties | Where-Object Name -eq $relative)
        }
        if ($properties.Count -eq 0) { throw 'Payload file identities required.' }
        $lastHash = $null
        foreach ($property in $properties) {
            $file = $target
            if ($Directory) {
                $file = [IO.Path]::GetFullPath((Join-Path $rootPath $property.Name))
                if (-not $file.StartsWith($rootPath + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
                    throw 'Payload file path escapes its installation root.'
                }
            }
            if ($property.Value.sha256 -notmatch '^[0-9a-f]{64}$') { throw 'Payload SHA-256 required.' }
            $lastHash = (Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash.ToLowerInvariant()
            if ($lastHash -ne $property.Value.sha256 -or (Get-Item -LiteralPath $file).Length -ne $property.Value.size) {
                throw "Installed payload content differs: $file"
            }
        }
        return [pscustomobject]@{ status = 'matched'; detail = "Verified $($properties.Count) payload file identities against the archive receipt."; sha256 = $lastHash }
    }
    catch { return [pscustomobject]@{ status = 'mismatch'; detail = $_.Exception.Message; sha256 = $null } }
}
