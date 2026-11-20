param([switch]$Format)
$ErrorActionPreference = 'Stop'
$repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$configuration = Get-Content -LiteralPath (Join-Path $repository '.local/environment.json') -Raw | ConvertFrom-Json
Push-Location $repository
try {
    $files = @(git ls-files --cached --others --exclude-standard | Sort-Object -Unique | Where-Object {
        $_ -notmatch '^(\.local/|artifacts/|build/|install/|third_party/|\.venv/)' -and (Test-Path -LiteralPath $_ -PathType Leaf)
    })
    if ($LASTEXITCODE -ne 0) { throw 'Git source inventory failed.' }
    $cpp = @($files | Where-Object { $_ -match '\.(cpp|hpp|h)$' })
    $pythonFiles = @($files | Where-Object { $_ -match '\.py$' })
    $ruff = Join-Path $repository '.venv/Scripts/ruff.exe'
    if ($Format) {
        & $ruff check --fix -- @pythonFiles
        if ($LASTEXITCODE -ne 0) { throw 'Python lint requires review.' }
        & $ruff format -- @pythonFiles
        if ($LASTEXITCODE -ne 0) { throw 'Python formatting failed.' }
        & $configuration.paths.'clang-format' -i -- @cpp
        if ($LASTEXITCODE -ne 0) { throw 'C++ formatting failed.' }
        return
    }
    & $ruff check --no-cache -- @pythonFiles
    if ($LASTEXITCODE -ne 0) { throw 'Python lint failed.' }
    & $ruff format --check -- @pythonFiles
    if ($LASTEXITCODE -ne 0) { throw 'Python format check failed.' }
    & $configuration.paths.'clang-format' --dry-run --Werror -- @cpp
    if ($LASTEXITCODE -ne 0) { throw 'C++ format check failed.' }
    & (Join-Path $repository '.venv/Scripts/basedpyright.exe')
    if ($LASTEXITCODE -ne 0) { throw 'Python type checking failed.' }
    & $configuration.paths.python (Join-Path $PSScriptRoot 'check_text.py') @files
    if ($LASTEXITCODE -ne 0) { throw 'Text conventions failed.' }
    & (Join-Path $repository '.venv/Scripts/prek.exe') run check-json check-yaml check-toml check-case-conflict check-merge-conflict hibiki-bilingual-docs hibiki-example-types --files @files
    if ($LASTEXITCODE -ne 0) { throw 'File validation failed.' }
    $sources = @($cpp | Where-Object { $_ -match '\.cpp$' -and $_ -ne 'tests/toolchain/AsanProbe.cpp' })
    foreach ($source in $sources) {
        if ($source -match '^(tests|examples)/install-consumer/') {
            $database = 'build/installed-tests-windows-debug'
            if (Test-Path (Join-Path $database 'compile_commands.json')) { & $configuration.paths.'clang-tidy' -p $database $source }
            else { & $configuration.paths.'clang-tidy' $source -- -x c++ -std=c++20 "-I$repository/include" }
        }
        else { & $configuration.paths.'clang-tidy' -p build/windows-debug $source }
        if ($LASTEXITCODE -ne 0) { throw "C++ static checks failed: $source" }
    }
}
finally { Pop-Location }
