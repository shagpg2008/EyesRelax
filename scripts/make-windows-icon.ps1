[CmdletBinding()]
param(
    [string]$Source = "assets\icons\runtime\eye-open.png",
    [string]$Destination = "resources\windows\EyesRelax.ico"
)

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing

$projectRoot = Split-Path -Parent $PSScriptRoot
$sourcePath = Join-Path $projectRoot $Source
$destinationPath = Join-Path $projectRoot $Destination
$sizes = @(16, 24, 32, 48, 64)
$images = [System.Collections.Generic.List[byte[]]]::new()
$sourceImage = [System.Drawing.Image]::FromFile($sourcePath)

try {
    foreach ($size in $sizes) {
        $bitmap = [System.Drawing.Bitmap]::new(
            $size, $size, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
        try {
            $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
            try {
                $graphics.Clear([System.Drawing.Color]::Transparent)
                $graphics.CompositingMode = [System.Drawing.Drawing2D.CompositingMode]::SourceCopy
                $graphics.CompositingQuality = [System.Drawing.Drawing2D.CompositingQuality]::HighQuality
                $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
                $graphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
                $graphics.DrawImage($sourceImage, 0, 0, $size, $size)
            } finally {
                $graphics.Dispose()
            }

            $andStride = [int]([Math]::Ceiling($size / 32.0) * 4)
            $memory = [System.IO.MemoryStream]::new()
            $writer = [System.IO.BinaryWriter]::new($memory)
            try {
                # BITMAPINFOHEADER. ICO stores XOR and AND masks in one doubled-height DIB.
                $writer.Write([uint32]40)
                $writer.Write([int32]$size)
                $writer.Write([int32]($size * 2))
                $writer.Write([uint16]1)
                $writer.Write([uint16]32)
                $writer.Write([uint32]0)
                $writer.Write([uint32]($size * $size * 4))
                $writer.Write([int32]0)
                $writer.Write([int32]0)
                $writer.Write([uint32]0)
                $writer.Write([uint32]0)

                # BGRA pixels, bottom row first. This is supported by Windows XP.
                for ($y = $size - 1; $y -ge 0; --$y) {
                    for ($x = 0; $x -lt $size; ++$x) {
                        $pixel = $bitmap.GetPixel($x, $y)
                        $writer.Write([byte]$pixel.B)
                        $writer.Write([byte]$pixel.G)
                        $writer.Write([byte]$pixel.R)
                        $writer.Write([byte]$pixel.A)
                    }
                }

                # 1-bit transparency mask, also bottom-up and DWORD aligned.
                for ($y = $size - 1; $y -ge 0; --$y) {
                    $row = [byte[]]::new($andStride)
                    for ($x = 0; $x -lt $size; ++$x) {
                        if ($bitmap.GetPixel($x, $y).A -eq 0) {
                            $byteIndex = [int][Math]::Floor($x / 8.0)
                            $row[$byteIndex] = $row[$byteIndex] -bor (0x80 -shr ($x % 8))
                        }
                    }
                    $writer.Write($row)
                }
                $writer.Flush()
                $images.Add($memory.ToArray())
            } finally {
                $writer.Dispose()
                $memory.Dispose()
            }
        } finally {
            $bitmap.Dispose()
        }
    }
} finally {
    $sourceImage.Dispose()
}

$stream = [System.IO.File]::Create($destinationPath)
$output = [System.IO.BinaryWriter]::new($stream)
try {
    $output.Write([uint16]0)
    $output.Write([uint16]1)
    $output.Write([uint16]$sizes.Count)
    $offset = 6 + 16 * $sizes.Count
    for ($i = 0; $i -lt $sizes.Count; ++$i) {
        $size = $sizes[$i]
        $output.Write([byte]$size)
        $output.Write([byte]$size)
        $output.Write([byte]0)
        $output.Write([byte]0)
        $output.Write([uint16]1)
        $output.Write([uint16]32)
        $output.Write([uint32]$images[$i].Length)
        $output.Write([uint32]$offset)
        $offset += $images[$i].Length
    }
    foreach ($image in $images) {
        $output.Write($image)
    }
} finally {
    $output.Dispose()
    $stream.Dispose()
}

Write-Host "Created $destinationPath with sizes: $($sizes -join ', ')"
