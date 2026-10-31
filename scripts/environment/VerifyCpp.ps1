param([string[]]$Presets = @('windows-debug','windows-relwithdebinfo','windows-release','windows-asan','windows-msvc'))
$ErrorActionPreference = 'Stop'
$repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$config = Get-Content -LiteralPath (Join-Path $repository '.local/environment.json') -Raw | ConvertFrom-Json
$cmake = $config.paths.cmake
$ctest = Join-Path ([IO.Path]::GetDirectoryName($cmake)) 'ctest.exe'
$llvmBin = [IO.Path]::GetDirectoryName($config.paths.'clang-cl')
$runtime = Join-Path ([IO.Path]::GetDirectoryName($llvmBin)) 'lib/clang/22/lib/windows'
$savedPath = $env:PATH
$savedLanguage = $env:VSLANG
$env:VSLANG = '1033'
$env:PATH = "$runtime;$llvmBin;$savedPath"
$reportRoot = Join-Path $repository 'artifacts/environment'
[IO.Directory]::CreateDirectory($reportRoot) | Out-Null
function Invoke-Checked {
    param([string]$Program, [string[]]$Arguments, [string]$Log)
    Write-Host "$Program $($Arguments -join ' ')"
    & $Program @Arguments 2>&1 | Out-File -LiteralPath $Log -Encoding utf8
    if ($LASTEXITCODE -ne 0) {
        Get-Content -LiteralPath $Log -Tail 35 | Write-Host
        throw "Command failed ($LASTEXITCODE): $Program $($Arguments -join ' ')"
    }
    Write-Host "Passed; log: $Log"
}
Push-Location $repository
try {
    foreach ($preset in $Presets) {
        if ($preset -notin @('windows-debug','windows-relwithdebinfo','windows-release','windows-asan','windows-msvc')) { throw "Unknown preset: $preset" }
        Invoke-Checked $cmake @('--preset', $preset) (Join-Path $reportRoot "$preset-configure.log")
        Invoke-Checked $cmake @('--build','--preset',$preset) (Join-Path $reportRoot "$preset-build.log")
        Invoke-Checked $ctest @('--preset',$preset,'--output-junit',"$reportRoot/$preset-tests.xml") (Join-Path $reportRoot "$preset-tests.log")
        $marker = Join-Path $repository "build/$preset/tests/toolchain/hibiki-controlled-failure.marker"
        [IO.File]::WriteAllText($marker, 'Controlled CTest failure propagation')
        try {
            $failureOutput = & $ctest --preset $preset -R '^Build.LinkedIdentity$' --output-on-failure 2>&1 | Out-String
            $failureCode = $LASTEXITCODE
            [IO.File]::WriteAllText((Join-Path $reportRoot "$preset-expected-failure.log"), $failureOutput)
            if ($failureCode -eq 0 -or $failureOutput -notmatch 'Hibiki controlled failure') { throw 'CTest must propagate the controlled assertion failure.' }
        }
        finally { [IO.File]::Delete($marker) }
        Invoke-Checked $cmake @('--install',"build/$preset") (Join-Path $reportRoot "$preset-install.log")
        $configuration = 'Debug'
        if ($preset -eq 'windows-release') { $configuration = 'Release' }
        if ($preset -eq 'windows-relwithdebinfo') { $configuration = 'RelWithDebInfo' }
        $arguments = @('-S','examples/install-consumer','-B',"build/consumer-$preset",'-G','Ninja',
            "-DCMAKE_TOOLCHAIN_FILE=$repository/cmake/toolchains/windows.cmake", "-DCMAKE_BUILD_TYPE=$configuration",
            "-DCMAKE_PREFIX_PATH=$repository/install/$preset")
        if ($preset -eq 'windows-msvc') { $arguments += '-DHIBIKI_COMPILER=msvc' }
        if ($preset -eq 'windows-asan') {
            $arguments += @('-DHIBIKI_ASAN=ON','-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL','-DCMAKE_CXX_FLAGS=/fsanitize=address')
        }
        Invoke-Checked $cmake $arguments (Join-Path $reportRoot "$preset-consumer-configure.log")
        Invoke-Checked $cmake @('--build',"build/consumer-$preset") (Join-Path $reportRoot "$preset-consumer-build.log")
        Invoke-Checked $ctest @('--test-dir',"build/consumer-$preset",'--output-on-failure') (Join-Path $reportRoot "$preset-consumer-test.log")
        if ($preset -eq 'windows-asan') {
            $probe = Join-Path $repository 'build/windows-asan/tests/toolchain/hibiki_asan_probe.exe'
            $output = & $probe 2>&1 | Out-String
            $code = $LASTEXITCODE
            [IO.File]::WriteAllText((Join-Path $reportRoot 'asan-controlled-error.log'), $output)
            if ($code -eq 0 -or $output -notmatch 'AddressSanitizer: heap-buffer-overflow' -or $output -notmatch 'AsanProbe.cpp') {
                throw 'ASan must report a symbolized heap-buffer-overflow and fail the child process.'
            }
        }
    }
}
finally { Pop-Location; $env:PATH = $savedPath; $env:VSLANG = $savedLanguage }
exit 0
