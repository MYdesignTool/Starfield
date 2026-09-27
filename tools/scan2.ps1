# Focused minidump scan: exception record, our module, and any thread stack that
# contains a return address inside the plug-in. Diagnostic only; the dump it reads stays
# under artifacts/crash/ while the script is tracked, so it holds no paths or user data.
param([Parameter(Mandatory = $true)][string]$Path)

$ErrorActionPreference = 'Stop'
$bytes = [System.IO.File]::ReadAllBytes($Path)
function U32([long]$o) { return [BitConverter]::ToUInt32($bytes, [int]$o) }
function U64([long]$o) { return [BitConverter]::ToUInt64($bytes, [int]$o) }

Write-Host ("file size = {0}" -f $bytes.Length)
if ((U32 0) -ne 0x504D444D) { throw 'not a minidump' }
$streamCount = U32 8
$dirRva = U32 12
$streams = @{}
for ($i = 0; $i -lt $streamCount; $i++) {
    $o = $dirRva + $i * 12
    $t = [int](U32 $o)
    $streams[$t] = @{ size = (U32 ($o + 4)); rva = (U32 ($o + 8)) }
}
Write-Host ('streams: ' + (($streams.Keys | Sort-Object) -join ','))

function ReadMdString([long]$rva) {
    $len = U32 $rva
    if ($len -le 0 -or $len -gt 8192 -or ($rva + 4 + $len) -gt $bytes.Length) { return '<bad>' }
    return [System.Text.Encoding]::Unicode.GetString($bytes, [int]($rva + 4), [int]$len)
}

# Modules
$modules = New-Object System.Collections.ArrayList
if ($streams.ContainsKey(4)) {
    $rva = $streams[4].rva
    $count = U32 $rva
    $start = $rva + 8
    if ((U64 $start) -lt 0x10000) { $start = $rva + 4 }
    for ($i = 0; $i -lt $count; $i++) {
        $o = $start + $i * 108
        $base = U64 $o
        $size = U32 ($o + 8)
        if ($size -eq 0) { continue }
        $null = $modules.Add([pscustomobject]@{ Base = $base; End = $base + $size; Size = $size
                Name = (ReadMdString (U32 ($o + 20))); TDS = (U32 ($o + 16)) })
    }
}

function Describe([uint64]$address) {
    foreach ($m in $modules) {
        if ($address -ge $m.Base -and $address -lt $m.End) { return ("{0}+0x{1:x}" -f (Split-Path -Leaf $m.Name), ($address - $m.Base)) }
    }
    return $null
}

$plugin = $modules | Where-Object { (Split-Path -Leaf $_.Name) -match 'StarfieldParticle' } | Select-Object -First 1
if ($plugin) {
    Write-Host ("plugin: {0}" -f $plugin.Name)
    Write-Host ("  base=0x{0:x16} size=0x{1:x} timestamp={2} ({3})" -f $plugin.Base, $plugin.Size, $plugin.TDS, [DateTimeOffset]::FromUnixTimeSeconds($plugin.TDS).ToLocalTime())
} else {
    Write-Host 'plugin module NOT loaded in this dump'
}
$ae = $modules | Where-Object { (Split-Path -Leaf $_.Name) -match 'AfterFX' } | Select-Object -First 3
foreach ($m in $ae) { Write-Host ("host: {0} ts={1} ({2})" -f (Split-Path -Leaf $m.Name), $m.TDS, [DateTimeOffset]::FromUnixTimeSeconds($m.TDS).ToLocalTime()) }

# Exception
$exceptionThread = 0
if ($streams.ContainsKey(6)) {
    $ex = $streams[6].rva
    $exceptionThread = U32 $ex
    Write-Host ("exception code=0x{0:x8} address=0x{1:x16}" -f (U32 ($ex + 8)), (U64 ($ex + 16)))
    $ctxRva = U32 ($ex + 164)
    $ctxSize = U32 ($ex + 160)
    if ($ctxSize -ge 0x120) {
        Write-Host ("  ctx rip=0x{0:x16} -> {1}" -f (U64 ($ctxRva + 0xF8)), (Describe (U64 ($ctxRva + 0xF8))))
        Write-Host ("  ctx rsp=0x{0:x16}" -f (U64 ($ctxRva + 0x98)))
    }
}

# Every thread stack: look for our module
if ($streams.ContainsKey(3) -and $plugin) {
    $rva = $streams[3].rva
    $count = U32 $rva
    $found = 0
    for ($i = 0; $i -lt $count; $i++) {
        $o = $rva + 8 + $i * 48
        if (($o + 48) -gt $bytes.Length) { break }
        $tid = U32 $o
        $sStart = U64 ($o + 24)
        $sSize = U32 ($o + 32)
        $sRva = U32 ($o + 36)
        if ($sSize -le 0 -or ($sRva + $sSize) -gt $bytes.Length) { continue }
        # Scan the whole captured stack; collect distinct plug-in RVAs.
        $hits = @{}
        $k = 0
        while ($k + 8 -le $sSize) {
            $v = U64 ($sRva + $k)
            if ($v -ge $plugin.Base -and $v -lt $plugin.End) { $hits[$v - $plugin.Base] = $k }
            $k += 8
        }
        if ($hits.Count -gt 0) {
            $found++
            Write-Host ("thread {0}: {1} distinct plug-in addresses" -f $tid, $hits.Count)
            $hits.Keys | Sort-Object | Select-Object -First 40 | ForEach-Object { Write-Host ("    aex+0x{0:x} (stack+0x{1:x})" -f $_, $hits[$_]) }
        }
    }
    Write-Host ("threads containing plug-in return addresses: {0}" -f $found)
}

# Crashpad annotations (stream 24): keys/values as UTF-16 strings
if ($streams.ContainsKey(24) -or $streams.ContainsKey(1129316353)) {
    $key = if ($streams.ContainsKey(24)) { 24 } else { 1129316353 }
    $rva = $streams[$key].rva
    $size = [Math]::Min($streams[$key].size, 65536)
    $text = [System.Text.Encoding]::Unicode.GetString($bytes, [int]$rva, [int]$size)
    Write-Host 'annotations (utf-16 strings):'
    $shown = 0
    foreach ($m in [regex]::Matches($text, '[\x20-\x7e]{3,120}')) {
        Write-Host ('    ' + $m.Value)
        $shown++
        if ($shown -ge 60) { break }
    }
}
