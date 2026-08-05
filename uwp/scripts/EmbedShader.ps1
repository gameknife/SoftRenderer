[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [string]$VertexShader,

    [Parameter(Mandatory)]
    [string]$PixelShader,

    [Parameter(Mandatory)]
    [string]$OutputHeader
)

$ErrorActionPreference = "Stop"

function ConvertTo-CppArray {
    param(
        [Parameter(Mandatory)]
        [string]$Name,

        [Parameter(Mandatory)]
        [byte[]]$Bytes
    )

    $lines = [System.Collections.Generic.List[string]]::new()
    [void]$lines.Add("inline constexpr unsigned char $Name[] = {")
    for ($offset = 0; $offset -lt $Bytes.Length; $offset += 16) {
        $end = [Math]::Min($offset + 16, $Bytes.Length)
        $values = for ($index = $offset; $index -lt $end; $index++) {
            "0x{0:x2}" -f $Bytes[$index]
        }
        [void]$lines.Add("    " + ($values -join ", ") + ",")
    }
    [void]$lines.Add("};")
    [void]$lines.Add("inline constexpr unsigned int ${Name}Size = sizeof($Name);")
    return $lines
}

$vertexBytes = [System.IO.File]::ReadAllBytes((Resolve-Path -LiteralPath $VertexShader))
$pixelBytes = [System.IO.File]::ReadAllBytes((Resolve-Path -LiteralPath $PixelShader))

$header = [System.Collections.Generic.List[string]]::new()
[void]$header.Add("#pragma once")
[void]$header.Add("")
[void]$header.Add("namespace SoftRendererUwp::shaders")
[void]$header.Add("{")
foreach ($line in (ConvertTo-CppArray -Name "SoftwareFrameVertexShader" -Bytes $vertexBytes)) {
    [void]$header.Add("    $line")
}
[void]$header.Add("")
foreach ($line in (ConvertTo-CppArray -Name "SoftwareFramePixelShader" -Bytes $pixelBytes)) {
    [void]$header.Add("    $line")
}
[void]$header.Add("}")
[void]$header.Add("")

$outputPath = [System.IO.Path]::GetFullPath($OutputHeader)
$parent = Split-Path -Parent $outputPath
New-Item -ItemType Directory -Force -Path $parent | Out-Null
[System.IO.File]::WriteAllLines(
    $outputPath,
    $header,
    [System.Text.UTF8Encoding]::new($false))
