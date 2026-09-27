# Minimal minidump reader: exception record, module list, and a raw stack scan.
# Diagnostic tool for a dump kept under artifacts/crash/ (Git-ignored); the script itself
# is tracked, so it must stay free of hard-coded paths and of user-specific data.
# Usage: powershell -ExecutionPolicy Bypass -File tools/scan_dump.ps1 <dump.dmp>
param([Parameter(Mandatory = $true)][string]$Path)

$ErrorActionPreference = 'Stop'
$bytes = [System.IO.File]::ReadAllBytes($Path)

function U32([long]$o) { return [BitConverter]::ToUInt32($bytes, [int]$o) }
function U64([long]$o) { return [BitConverter]::ToUInt64($bytes, [int]$o) }

if ((U32 0) -ne 0x504D444D) { throw 'Not a minidump (missing MDMP signature).' }
$streamCount = U32 8
$dirRva = U32 12
Write-Host ("streams={0} dirRva={1}" -f $streamCount, $dirRva)

$streams = @{}
Write-Host ('stream types: ' + (0..($streamCount - 1) | ForEach-Object { (U32 ($dirRva + $_ * 12)) }) -join ',')
for ($i = 0; $i -lt $streamCount; $i++) {
    $o = $dirRva + $i * 12
    $streams[[int](U32 $o)] = @{ size = (U32 ($o + 4)); rva = (U32 ($o + 8)) }
}

function ReadMdString([long]$rva) {
    $len = U32 $rva
    if ($len -le 0 -or $len -gt 8192 -or ($rva + 4 + $len) -gt $bytes.Length) { return '<bad>' }
    return [System.Text.Encoding]::Unicode.GetString($bytes, [int]($rva + 4), [int]$len)
}

# --- Module list (stream 4) ---
$modules = New-Object System.Collections.ArrayList
if ($streams.ContainsKey(4)) {
    $rva = $streams[4].rva
    $count = U32 $rva
    foreach ($pad in @(8, 4)) {
        $start = $rva + $pad
        if (($start + $count * 108) -gt $bytes.Length) { continue }
        $base0 = U64 $start
        if ($base0 -ge 0x10000 -and $base0 -lt 0x00007FFFFFFFFFFF) { break }
    }
    for ($i = 0; $i -lt $count; $i++) {
        $o = $start + $i * 108
        $base = U64 $o
        $sizeImg = U32 ($o + 8)
        if ($sizeImg -eq 0) { continue }
        $null = $modules.Add([pscustomobject]@{
                Base = $base; End = $base + $sizeImg; Size = $sizeImg
                Name = (ReadMdString (U32 ($o + 20))); TimeStamp = (U32 ($o + 16))
            })
    }
}
Write-Host ("modules={0}" -f $modules.Count)

function Describe([uint64]$address) {
    foreach ($m in $modules) {
        if ($address -ge $m.Base -and $address -lt $m.End) {
            $leaf = Split-Path -Leaf $m.Name
            return ("{0}+0x{1:x}" -f $leaf, ($address - $m.Base))
        }
    }
    return $null
}

# --- Exception (stream 6) ---
$exceptionThread = 0
$stackStart = 0; $stackSize = 0; $stackRva = 0
if ($streams.ContainsKey(6)) {
    $ex = $streams[6].rva
    $exceptionThread = U32 $ex
    $code = U32 ($ex + 8)
    $flags = U32 ($ex + 12)
    $addr = U64 ($ex + 16)
    $nparams = U32 ($ex + 24)
    Write-Host ("exception code=0x{0:x8} flags=0x{1:x} address=0x{2:x16} params={3}" -f $code, $flags, $addr, $nparams)
    Write-Host ("exception address -> {0}" -f (Describe $addr))
    if ($nparams -gt 0 -and $nparams -le 15) {
        for ($p = 0; $p -lt $nparams; $p++) {
            $v = U64 ($ex + 32 + $p * 8)
            $d = Describe $v
            Write-Host ("  info[{0}] = 0x{1:x16} {2}" -f $p, $v, ($(if ($d) { "-> $d" } else { '' })))
        }
    }
    $ctxSize = U32 ($ex + 160)
    $ctxRva = U32 ($ex + 164)
    if ($ctxSize -ge 0x120 -and ($ctxRva + 0x100) -lt $bytes.Length) {
        $rip = U64 ($ctxRva + 0xF8)
        $rsp = U64 ($ctxRva + 0x98)
        $rbp = U64 ($ctxRva + 0xA0)
        $rcx = U64 ($ctxRva + 0x80)
        $rdx = U64 ($ctxRva + 0x88)
        Write-Host ("context rip=0x{0:x16} -> {1}" -f $rip, (Describe $rip))
        Write-Host ("context rsp=0x{0:x16} rbp=0x{1:x16} rcx=0x{2:x16} rdx=0x{3:x16}" -f $rsp, $rbp, $rcx, $rdx)
    }
    Write-Host ("exception thread id = {0}" -f $exceptionThread)
}

# --- Thread list (stream 3): find the exception thread's stack ---
if ($streams.ContainsKey(3)) {
    $rva = $streams[3].rva
    $count = U32 $rva
    Write-Host ("threads={0}" -f $count)
    for ($i = 0; $i -lt $count; $i++) {
        $o = $rva + 8 + $i * 48
        if (($o + 48) -gt $bytes.Length) { break }
        $tid = U32 $o
        $sStart = U64 ($o + 24)
        $sSize = U32 ($o + 32)
        $sRva = U32 ($o + 36)
        if ($tid -eq $exceptionThread -and $sSize -gt 0) {
            $stackStart = $sStart; $stackSize = $sSize; $stackRva = $sRva
        }
    }
}

# --- Stack scan ---
function ScanRange([long]$start, [long]$size, [long]$rva, [int]$limit) {
    if ($size -le 0 -or ($rva + $size) -gt $bytes.Length) { return }
    $hits = 0
    $o = 0
    while ($o + 8 -le $size -and $hits -lt $limit) {
        $value = U64 ($rva + $o)
        if ($value -gt 0x10000) {
            $d = Describe $value
            if ($d) {
                Write-Host ("  stack 0x{0:x16} (+0x{1:x}) -> {2}" -f $value, $o, $d)
                $hits++
            }
        }
        $o += 8
    }
}

if ($stackSize -gt 0) {
    Write-Host ("stack scan: start=0x{0:x16} size={1}" -f $stackStart, $stackSize)
    ScanRange $stackStart $stackSize $stackRva 60
} elseif ($streams.ContainsKey(5)) {
    $rva = $streams[5].rva
    $count = U32 $rva
    Write-Host ("memory ranges={0}" -f $count)
    for ($i = 0; $i -lt $count; $i++) {
        $o = $rva + 4 + $i * 16
        $mStart = U64 $o
        $mSize = U32 ($o + 8)
        $mRva = U32 ($o + 12)
        Write-Host ("  range 0x{0:x16} size={1}" -f $mStart, $mSize)
        if ($i -lt 4) { ScanRange $mStart $mSize $mRva 30 }
    }
}

Write-Host '=== loaded modules of interest ==='
foreach ($m in $modules) {
    $leaf = Split-Path -Leaf $m.Name
    if ($leaf -match 'AfterFX|Starfield|StarfieldParticle|adobe|CRClient') {
        Write-Host ("0x{0:x16} size=0x{1:x} {2}" -f $m.Base, $m.Size, $leaf)
    }
}

# --- Thread stack descriptor stats + scan of the large memory ranges (real stacks) ---
if ($streams.ContainsKey(3)) {
    $rva = $streams[3].rva
    $count = U32 $rva
    $withStack = 0
    for ($i = 0; $i -lt $count; $i++) {
        $o = $rva + 8 + $i * 48
        if (($o + 48) -gt $bytes.Length) { break }
        $sSize = U32 ($o + 32)
        $sRva = U32 ($o + 36)
        if ($sSize -gt 0 -and ($sRva + $sSize) -le $bytes.Length) {
            $withStack++
            if ($withStack -le 5) { Write-Host ("thread {0}: stack start=0x{1:x16} size={2} rva={3}" -f (U32 $o), (U64 ($o + 24)), $sSize, $sRva) }
        }
    }
    Write-Host ("threads with captured stack: {0} of {1}" -f $withStack, $count)
}

if ($streams.ContainsKey(5)) {
    $rva = $streams[5].rva
    $count = U32 $rva
    $scanned = 0
    for ($i = 0; $i -lt $count -and $scanned -lt 8; $i++) {
        $o = $rva + 4 + $i * 16
        $mStart = U64 $o
        $mSize = U32 ($o + 8)
        $mRva = U32 ($o + 12)
        if ($mSize -lt 0x40000 -or ($mRva + $mSize) -gt $bytes.Length) { continue }
        $scanned++
        Write-Host ("=== memory range 0x{0:x16} size={1}:" -f $mStart, $mSize)
        $hits = 0
        $k = 0
        while ($k + 8 -le $mSize -and $hits -lt 60) {
            $value = U64 ($mRva + $k)
            if ($value -gt 0x10000) {
                $d = Describe $value
                if ($d) {
                    Write-Host ("    +0x{0:x} 0x{1:x16} -> {2}" -f $k, $value, $d)
                    $hits++
                }
            }
            $k += 8
        }
    }
}

# --- Scan every thread stack for frames inside our plug-in ---
$plugin = $modules | Where-Object { (Split-Path -Leaf $_.Name) -match 'StarfieldParticle' } | Select-Object -First 1
if (-not $plugin) { Write-Host 'StarfieldParticle.aex not found in module list.'; return }
Write-Host ("plugin base=0x{0:x16} size=0x{1:x}" -f $plugin.Base, $plugin.Size)

if ($streams.ContainsKey(3)) {
    $rva = $streams[3].rva
    $count = U32 $rva
    $withFrames = New-Object System.Collections.ArrayList
    for ($i = 0; $i -lt $count; $i++) {
        $o = $rva + 8 + $i * 48
        if (($o + 48) -gt $bytes.Length) { break }
        $tid = U32 $o
        $sStart = U64 ($o + 24)
        $sSize = U32 ($o + 32)
        $sRva = U32 ($o + 36)
        if ($sSize -le 0 -or ($sRva + $sSize) -gt $bytes.Length) { continue }
        $hits = New-Object System.Collections.ArrayList
        $k = 0
        while ($k + 8 -le $sSize) {
            $value = U64 ($sRva + $k)
            if ($value -ge $plugin.Base -and $value -lt $plugin.End) {
                $null = $hits.Add(@{ off = $k; addr = $value; rva = $value - $plugin.Base })
            }
            $k += 8
        }
        if ($hits.Count -gt 0) {
            $null = $withFrames.Add(@{ tid = $tid; owner = (($tid -eq $exceptionThread)); hits = $hits; start = $sStart; size = $sSize })
        }
    }
    Write-Host ("threads with plug-in frames: {0}" -f $withFrames.Count)
    foreach ($t in $withFrames) {
        Write-Host ("--- thread {0} {1} frames={2}" -f $t.tid, $(if ($t.owner) { '(exception thread)' } else { '' }), $t.hits.Count)
        foreach ($h in $t.hits) { Write-Host ("    aex+0x{0:x} (stack+0x{1:x})" -f $h.rva, $h.off) }
    }

    # Full frame list for threads that were inside the host, the CRT, or our plug-in:
    # this is where an abort()/fatal-exit caller shows up.
    Write-Host '=== host/CRT/plug-in frames per thread ==='
    $key = @('AfterFX', 'AfterFXLib', 'BEE', 'ucrtbase', 'vcruntime', 'msvcp', 'PICA', 'StarfieldParticle', 'sentry', 'crashpad')
    for ($i = 0; $i -lt $count; $i++) {
        $o = $rva + 8 + $i * 48
        if (($o + 48) -gt $bytes.Length) { break }
        $tid = U32 $o
        $sSize = U32 ($o + 32)
        $sRva = U32 ($o + 36)
        if ($sSize -le 0 -or ($sRva + $sSize) -gt $bytes.Length) { continue }
        $frames = New-Object System.Collections.ArrayList
        $k = 0
        while ($k + 8 -le $sSize) {
            $value = U64 ($sRva + $k)
            if ($value -gt 0x10000) {
                $d = Describe $value
                if ($d) {
                    $leaf = ($d -split '\+')[0]
                    $hit = $false
                    foreach ($n in $key) { if ($leaf -like "$n*") { $hit = $true; break } }
                    if ($hit) { $null = $frames.Add($d) }
                }
            }
            $k += 8
        }
        if ($frames.Count -gt 0) {
            Write-Host ("--- thread {0} ({1} frames)" -f $tid, $frames.Count)
            $seen = 0
            foreach ($f in $frames) { Write-Host ("    $f"); $seen++; if ($seen -ge 18) { break } }
        }
    }
}
