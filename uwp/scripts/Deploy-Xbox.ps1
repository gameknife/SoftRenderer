[CmdletBinding()]
param(
    [string]$BuildDirectory = "",
    [string]$XboxAddress = "",
    [string]$CertificateThumbprint = "",
    [switch]$SkipSigning
)

$ErrorActionPreference = "Stop"

$repositoryRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot "..\..\")).Path
if ([string]::IsNullOrWhiteSpace($BuildDirectory)) {
    $BuildDirectory = Join-Path $repositoryRoot "out\build\xbox-release"
}
if ([string]::IsNullOrWhiteSpace($XboxAddress)) {
    $XboxAddress = Read-Host "Xbox Device Portal address (for example 192.168.1.20)"
}

$BuildDirectory = [System.IO.Path]::GetFullPath($BuildDirectory)
$packageFile = Get-ChildItem -LiteralPath $BuildDirectory -Recurse -Filter "SoftRendererUwp_*.msix" |
    Where-Object { $_.FullName -notmatch "\\Dependencies\\" } |
    Sort-Object LastWriteTime -Descending |
    Select-Object -First 1
if (-not $packageFile) {
    throw "No MSIX package found below $BuildDirectory. Run: cmake --build --preset xbox-release"
}

function Get-SignTool {
    $command = Get-Command signtool.exe -ErrorAction SilentlyContinue
    if ($command) { return $command.Source }
    $kitsRoot = Join-Path ${env:ProgramFiles(x86)} "Windows Kits\10\bin"
    $candidate = Get-ChildItem -LiteralPath $kitsRoot -Filter signtool.exe -Recurse |
        Sort-Object FullName -Descending | Select-Object -First 1
    if (-not $candidate) { throw "signtool.exe was not found." }
    return $candidate.FullName
}

if (-not $SkipSigning) {
    if ([string]::IsNullOrWhiteSpace($CertificateThumbprint)) {
        $CertificateThumbprint = Read-Host "Signing certificate SHA-1 thumbprint"
    }
    $certificate = Get-ChildItem -LiteralPath "Cert:\CurrentUser\My\$CertificateThumbprint" -ErrorAction SilentlyContinue
    if (-not $certificate -or -not $certificate.HasPrivateKey) {
        throw "Signing certificate $CertificateThumbprint with a private key was not found."
    }
    & (Get-SignTool) sign /fd SHA256 /sha1 $CertificateThumbprint $packageFile.FullName
    if ($LASTEXITCODE -ne 0) { throw "signtool.exe failed with exit code $LASTEXITCODE." }
}

$baseUri = "https://${XboxAddress}:11443"
$responseFile = Join-Path $env:TEMP "softrenderer-xbox-response.json"
$headers = @(curl.exe -k -sS -D - -o NUL "$baseUri/api/app/packagemanager/packages")
$cookieLine = $headers | Where-Object { $_ -match '^Set-Cookie:\s*CSRF-Token=' } | Select-Object -First 1
$token = [regex]::Match($cookieLine, 'CSRF-Token=([^;\r\n]+)').Groups[1].Value
if ([string]::IsNullOrWhiteSpace($token)) { throw "Xbox Device Portal did not return a CSRF token." }

$packageName = $packageFile.Name
& curl.exe -k -sS -X POST -H "X-CSRF-Token: $token" -H "Cookie: CSRF-Token=$token" `
    -F "$packageName=@$($packageFile.FullName)" `
    "$baseUri/api/app/packagemanager/package?package=$([uri]::EscapeDataString($packageName))" `
    -o $responseFile
if ($LASTEXITCODE -ne 0) { throw "Xbox package upload failed." }

Write-Host "Uploaded $packageName to $XboxAddress. Use Dev Home or Device Portal to launch SoftRendererUwp."
