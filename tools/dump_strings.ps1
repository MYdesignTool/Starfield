# Extracts crashpad annotation streams and printable strings from a minidump.
# Diagnostic tool for a dump kept under artifacts/crash/ (Git-ignored); the script itself
# is tracked, so it must stay free of hard-coded paths and of user-specific data.
param([Parameter(Mandatory = $true)][string]$Path, [int]$MaxStrings = 400)

$ErrorActionPreference = 'Stop'
$bytes = [System.IO.File]::ReadAllBytes($Path)
function U32([long]$o) { return [BitConverter]::ToUInt32($bytes, [int]$o) }

$streamCount = U32 8
$dirRva = U32 12
Write-Host '=== streams ==='
for ($i = 0; $i -lt $streamCount; $i++) {
    $o = $dirRva + $i * 12
    $type = U32 $o; $size = U32 ($o + 4); $rva = U32 ($o + 8)
    Write-Host ("type={0} size={1} rva={2}" -f $type, $size, $rva)
    # Crashpad annotations (24) and Adobe's custom stream: dump their strings.
    if ($type -eq 24 -or $type -eq 1129316353 -or $type -eq 16) {
        $text = [System.Text.Encoding]::GetEncoding(28591).GetString($bytes, [int]$rva, [int][Math]::Min($size, 65536))
        Write-Host ("--- strings in stream {0}:" -f $type)
        $matches = [regex]::Matches($text, '[\x20-\x7e]{5,}')
        $shown = 0
        foreach ($m in $matches) {
            Write-Host ("    " + $m.Value)
            $shown++
            if ($shown -ge 60) { break }
        }
    }
}

Write-Host '=== interesting strings anywhere in dump (error-ish) ==='
$pattern = 'Starfield|After Effects error|global outflags|parameter|caused a crash|Fatal|abort|exception|plug-in'
$full = [System.Text.Encoding]::GetEncoding(28591).GetString($bytes)
$found = [regex]::Matches($full, "[\x20-\x7e]{6,120}")
$count = 0
$seen = @{}
foreach ($m in $found) {
    if ($m.Value -notmatch $pattern) { continue }
    if ($seen.ContainsKey($m.Value)) { continue }
    $seen[$m.Value] = $true
    Write-Host ("    " + $m.Value)
    $count++
    if ($count -ge $MaxStrings) { break }
}
