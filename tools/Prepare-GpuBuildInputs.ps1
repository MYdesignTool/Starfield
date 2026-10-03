param([switch]$Download)
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$folder = Join-Path $repo 'artifacts\gpu-build\nvrtc-12.4.127'
$wheel = Join-Path $folder 'nvrtc.whl'
$url = 'https://files.pythonhosted.org/packages/7c/30/8c844bfb770f045bcd8b2c83455c5afb45983e1a8abf0c4e5297b481b6a5/nvidia_cuda_nvrtc_cu12-12.4.127-py3-none-win_amd64.whl'
$sha256 = 'A961B2F1D5F17B14867C619CEB99EF6FCEC12E46612711BCEC78EB05068A60EC'
if (-not $Download) {
    [pscustomobject]@{ Action='read-only'; Destination=$folder; Url=$url; SHA256=$sha256; Exists=(Test-Path -LiteralPath $wheel) }
    return
}
# Build-only, NVIDIA-owned PyPI package. No pip installation, registry, PATH,
# user-profile, system CUDA runtime, or driver modification. Retain its license.
New-Item -ItemType Directory -Force -Path $folder | Out-Null
if (-not (Test-Path -LiteralPath $wheel)) { Invoke-WebRequest -Uri $url -OutFile $wheel }
if ((Get-FileHash -LiteralPath $wheel -Algorithm SHA256).Hash -ne $sha256) { throw 'NVRTC wheel checksum mismatch.' }
Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip = [IO.Compression.ZipFile]::OpenRead($wheel)
try {
    $root = [IO.Path]::GetFullPath($folder).TrimEnd('\') + '\'
    foreach ($entry in $zip.Entries) {
        $target = [IO.Path]::GetFullPath((Join-Path $folder $entry.FullName))
        if (-not $target.StartsWith($root,[StringComparison]::OrdinalIgnoreCase)) { throw 'Archive path leaves build-input directory.' }
        if ($entry.FullName.EndsWith('/')) { continue }
        New-Item -ItemType Directory -Force -Path ([IO.Path]::GetDirectoryName($target)) | Out-Null
        [IO.Compression.ZipFileExtensions]::ExtractToFile($entry,$target,$true)
    }
} finally { $zip.Dispose() }
Write-Host "Verified workspace NVRTC build input: $folder"
