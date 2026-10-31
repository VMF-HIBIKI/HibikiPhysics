param(
    [ValidateSet('environment','ruff-fix','ruff-format','clang-format','docs')][string]$Mode,
    [Parameter(ValueFromRemainingArguments = $true)][string[]]$Files
)
$ErrorActionPreference = 'Stop'
$repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
try {
    $configuration = Get-Content -LiteralPath (Join-Path $repository '.local/environment.json') -Raw | ConvertFrom-Json
    $manifest = Get-Content -LiteralPath (Join-Path $repository 'tools/toolchain-lock.json') -Raw | ConvertFrom-Json
    if ($Mode -eq 'environment') {
        $version = & (Join-Path $repository '.venv/Scripts/prek.exe') --version
        $expected = ($manifest.artifacts | Where-Object id -eq 'prek-wheel').version
        if ($version -notmatch ('^prek ' + [regex]::Escape($expected) + '(\s|$)')) { throw 'prek version differs; run prepare.' }
        $receipt = Get-Content -LiteralPath (Join-Path $repository '.local/python-environment.json') -Raw | ConvertFrom-Json
        if ((Get-FileHash -LiteralPath (Join-Path $repository 'uv.lock') -Algorithm SHA256).Hash -ne $receipt.lock_sha256) { throw 'Python environment lock changed; run prepare.' }
        exit 0
    }
    $paths = @($Files | ForEach-Object { [IO.Path]::GetFullPath($_) })
    if ($paths.Count -eq 0) { exit 0 }
    if ($Mode -eq 'clang-format') {
        $version = & $configuration.paths.'clang-format' --version
        $expected = ($manifest.artifacts | Where-Object id -eq 'llvm').version
        if ($version -notmatch ('version ' + [regex]::Escape($expected) + '(\s|$)')) { throw 'clang-format version differs; run prepare.' }
        & $configuration.paths.'clang-format' "--style=file:$repository/.clang-format" -i -- @paths
        exit $LASTEXITCODE
    }
    if ($Mode -eq 'docs') {
        & $configuration.paths.python (Join-Path $PSScriptRoot 'check_documents.py') @paths
        exit $LASTEXITCODE
    }
    $uv = $configuration.paths.uv
    $version = & $uv --version
    $expected = ($manifest.artifacts | Where-Object id -eq 'uv').version
    if ($version -notmatch ('^uv ' + [regex]::Escape($expected) + '\s')) { throw 'uv version differs; run prepare.' }
    $ruff = Join-Path $repository '.venv/Scripts/ruff.exe'
    $version = & $ruff --version
    $expected = ($manifest.artifacts | Where-Object id -eq 'ruff-wheel').version
    if ($version -ne "ruff $expected") { throw 'Ruff version differs; run prepare.' }
    $arguments = @('run','--project',$repository,'--locked','--offline','--no-sync','ruff')
    if ($Mode -eq 'ruff-fix') { $arguments += @('check','--fix') }
    else { $arguments += 'format' }
    $arguments += @('--config',(Join-Path $repository 'pyproject.toml'),'--') + $paths
    & $uv @arguments
    exit $LASTEXITCODE
}
catch { Write-Output $_.Exception.Message; exit 1 }
