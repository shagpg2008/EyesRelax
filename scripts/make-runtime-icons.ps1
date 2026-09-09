[CmdletBinding()]
param(
    [double]$EyeHeightRatio = 0.88
)

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing
$projectRoot = Split-Path -Parent $PSScriptRoot
$targetSize = 64

function Get-AlphaBounds([System.Drawing.Bitmap]$bitmap) {
    $rect = [System.Drawing.Rectangle]::new(0, 0, $bitmap.Width, $bitmap.Height)
    $data = $bitmap.LockBits($rect, [System.Drawing.Imaging.ImageLockMode]::ReadOnly,
                            [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    try {
        $stride = [Math]::Abs($data.Stride)
        $pixels = [byte[]]::new($stride * $bitmap.Height)
        [Runtime.InteropServices.Marshal]::Copy($data.Scan0, $pixels, 0, $pixels.Length)
        $minX = $bitmap.Width
        $minY = $bitmap.Height
        $maxX = -1
        $maxY = -1
        for ($y = 0; $y -lt $bitmap.Height; ++$y) {
            for ($x = 0; $x -lt $bitmap.Width; ++$x) {
                if ($pixels[$y * $stride + $x * 4 + 3] -gt 8) {
                    if ($x -lt $minX) { $minX = $x }
                    if ($x -gt $maxX) { $maxX = $x }
                    if ($y -lt $minY) { $minY = $y }
                    if ($y -gt $maxY) { $maxY = $y }
                }
            }
        }
        if ($maxX -lt 0) { throw "The source image has no visible pixels." }
        return [System.Drawing.Rectangle]::FromLTRB($minX, $minY, $maxX + 1, $maxY + 1)
    } finally {
        $bitmap.UnlockBits($data)
    }
}

function Convert-EyeIcon([string]$sourceRelative, [string]$destinationRelative) {
    $sourcePath = Join-Path $projectRoot $sourceRelative
    $destinationPath = Join-Path $projectRoot $destinationRelative
    $source = [System.Drawing.Bitmap]::FromFile($sourcePath)
    try {
        $bounds = Get-AlphaBounds $source
        $scale = ($targetSize * $EyeHeightRatio) / $bounds.Height
        $centreX = $bounds.Left + $bounds.Width / 2.0
        $centreY = $bounds.Top + $bounds.Height / 2.0
        $destination = [System.Drawing.Bitmap]::new(
            $targetSize, $targetSize, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
        try {
            $graphics = [System.Drawing.Graphics]::FromImage($destination)
            try {
                $graphics.Clear([System.Drawing.Color]::Transparent)
                $graphics.CompositingMode = [System.Drawing.Drawing2D.CompositingMode]::SourceCopy
                $graphics.CompositingQuality = [System.Drawing.Drawing2D.CompositingQuality]::HighQuality
                $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
                $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
                $drawX = $targetSize / 2.0 - $centreX * $scale
                $drawY = $targetSize / 2.0 - $centreY * $scale
                $graphics.DrawImage($source, [float]$drawX, [float]$drawY,
                                    [float]($source.Width * $scale),
                                    [float]($source.Height * $scale))
            } finally {
                $graphics.Dispose()
            }
            $destination.Save($destinationPath, [System.Drawing.Imaging.ImageFormat]::Png)
        } finally {
            $destination.Dispose()
        }
    } finally {
        $source.Dispose()
    }
}

Convert-EyeIcon "assets\icons\eye-open-photo-inspired-v2.png" `
                "assets\icons\runtime\eye-open.png"
Convert-EyeIcon "assets\icons\eye-closed-photo-inspired-v4.png" `
                "assets\icons\runtime\eye-closed.png"
Write-Host "Created enlarged 64x64 runtime icons (eye height ratio: $EyeHeightRatio)."
