[CmdletBinding()]
param(
    [string]$BuildDir = "build",

    [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
    [string]$BuildConfiguration = "Release",

    [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
    [string]$TestConfiguration = "Debug",

    [string]$BoostIncludeDir,
    [string]$Generator,
    [switch]$SkipTests
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

function Resolve-KeyrecordPath {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path,

        [Parameter(Mandatory = $true)]
        [string]$BasePath
    )

    if ([System.IO.Path]::IsPathRooted($Path)) {
        return [System.IO.Path]::GetFullPath($Path)
    }

    return [System.IO.Path]::GetFullPath((Join-Path $BasePath $Path))
}

function Invoke-Step {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Name,

        [Parameter(Mandatory = $true)]
        [string]$FilePath,

        [Parameter(Mandatory = $true)]
        [string[]]$ArgumentList
    )

    Write-Host "==> $Name"
    & $FilePath @ArgumentList
    if ($LASTEXITCODE -ne 0) {
        throw "步骤失败：$Name（退出码 $LASTEXITCODE）"
    }
}

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$buildDirPath = Resolve-KeyrecordPath -Path $BuildDir -BasePath $scriptRoot
$cmakePath = (Get-Command cmake -ErrorAction Stop).Source
$ctestPath = $null

if (-not $SkipTests) {
    $ctestPath = (Get-Command ctest -ErrorAction Stop).Source
}

Push-Location $scriptRoot
try {
    $configureArgs = @(
        "-S", $scriptRoot,
        "-B", $buildDirPath
    )

    if ($Generator) {
        $configureArgs += @("-G", $Generator)
    }

    if ($BoostIncludeDir) {
        $boostIncludePath = Resolve-KeyrecordPath -Path $BoostIncludeDir -BasePath $scriptRoot
        $configureArgs += "-DKEYRECORD_BOOST_INCLUDE_DIR=$boostIncludePath"
    }

    Invoke-Step -Name "配置 CMake 工程" -FilePath $cmakePath -ArgumentList $configureArgs

    $buildArgs = @(
        "--build", $buildDirPath,
        "--config", $BuildConfiguration
    )
    Invoke-Step -Name "编译应用目标（$BuildConfiguration）" -FilePath $cmakePath -ArgumentList $buildArgs

    if ($SkipTests) {
        Write-Host "==> 已跳过测试"
        return
    }

    $testTargets = @(
        "keyrecord_tests",
        "config_path_tests",
        "embedded_resources_tests",
        "api_queries_tests",
        "http_router_tests",
        "visualization_service_tests",
        "server_bootstrap_tests",
        "server_startup_tests",
        "readonly_database_tests"
    )

    $buildTestArgs = @(
        "--build", $buildDirPath,
        "--config", $TestConfiguration,
        "--target"
    ) + $testTargets
    Invoke-Step -Name "编译测试目标（$TestConfiguration）" -FilePath $cmakePath -ArgumentList $buildTestArgs

    $ctestArgs = @(
        "--test-dir", $buildDirPath,
        "-C", $TestConfiguration,
        "--output-on-failure"
    )
    Invoke-Step -Name "执行测试（$TestConfiguration）" -FilePath $ctestPath -ArgumentList $ctestArgs
}
finally {
    Pop-Location
}
