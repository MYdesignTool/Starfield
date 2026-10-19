param(
    [Parameter(Mandatory = $true)][string]$SdkRoot,
    [Parameter(Mandatory = $true)][string]$IntermediateDir,
    [Parameter(Mandatory = $true)][string]$ResourceSource,
    [string]$FlagHeaderPath = '',
    [string]$VersionHeaderPath = '',
    [string]$OutFlagsName = 'STARFIELD_OUT_FLAGS',
    [string]$OutFlags2Name = 'STARFIELD_OUT_FLAGS2',
    [switch]$GeneralPlugin
)

$ErrorActionPreference = 'Continue'
if (-not $FlagHeaderPath) { $FlagHeaderPath = Join-Path $PSScriptRoot 'PluginFlags.h' }
if (-not $VersionHeaderPath) { $VersionHeaderPath = Join-Path $PSScriptRoot 'PluginVersion.h' }
$null = New-Item -ItemType Directory -Force -Path $IntermediateDir
$headerDir = Join-Path $SdkRoot 'Examples\Headers'
$toolPath = Join-Path $SdkRoot 'Examples\Resources\PiPLtool.exe'
# The generated batch file is deliberately ASCII. MSBuild's custom action can
# run from a non-ASCII checkout path, but embedding that absolute path in an
# ASCII .cmd silently replaces the characters with '?'. Use paths relative to
# the intermediate directory so the native tools receive only ASCII arguments.
function Get-RelativeBuildPath([string]$BasePath,[string]$TargetPath) {
    $base = [IO.Path]::GetFullPath($BasePath).TrimEnd('\') + '\'
    $target = [IO.Path]::GetFullPath($TargetPath)
    $baseUri = [Uri]$base
    $targetUri = [Uri]$target
    return [Uri]::UnescapeDataString($baseUri.MakeRelativeUri($targetUri).ToString()).Replace('/', '\')
}
function Get-PiPLStringData([string]$Value,[bool]$Pascal) {
    $bytes = [Text.Encoding]::ASCII.GetBytes($Value)
    if ([Text.Encoding]::ASCII.GetString($bytes) -cne $Value -or $Value.Contains('"') -or $Value.Contains('\')) {
        throw 'PiPL cache identity contains unsupported characters.'
    }
    if ($Pascal) {
        if ($bytes.Length -gt 255) { throw 'PiPL Pascal identity is too long.' }
        $length = $bytes.Length + 1
        $paddedLength = [int]([Math]::Ceiling($length / 4.0) * 4)
        $encoded = ('\x{0:X2}' -f $bytes.Length) + $Value + ('\0' * ($paddedLength - $length))
        return @{ Length = $paddedLength; Encoded = $encoded }
    }
    $paddedLength = [int]([Math]::Ceiling($bytes.Length / 4.0) * 4)
    return @{ Length = $paddedLength; Encoded = $Value + ('\0' * ($paddedLength - $bytes.Length)) }
}
function Set-PiPLRrcStringProperty([string]$Text,[string]$PropertyCode,[string]$Value,[bool]$Pascal) {
    $code = [regex]::Escape($PropertyCode)
    $pattern = '(?m)^(?<lead>[ \t]*"' + $code + '"[ \t]*,[ \t]*/\*[ \t]*''name''[ \t]*PINameProperty[ \t]*\*/[ \t]*\r?\n[ \t]*RSCS32\(0\),[ \t]*\r?\n[ \t]*)RSCS32\([ \t]*\d+[ \t]*\),[ \t]*\r?\n(?<indent>[ \t]*)"[^"\r\n]*",[ \t]*\r?$'
    $propertyMatches = [regex]::Matches($Text, $pattern)
    if ($propertyMatches.Count -ne 1) { throw "PiPL cache identity property $PropertyCode was not unique." }
    $data = Get-PiPLStringData $Value $Pascal
    return [regex]::Replace($Text, $pattern, [System.Text.RegularExpressions.MatchEvaluator]{
        param($m)
        $m.Groups['lead'].Value + 'RSCS32(' + $data.Length + '),' + [Environment]::NewLine +
            $m.Groups['indent'].Value + '"' + $data.Encoded + '",'
    }, 1)
}
$resourceIdPattern = '(?m)^(?<lead>[ \t]*resource[ \t]+''PiPL''[ \t]*\()(?<value>\d+)(?<tail>\)[ \t]*\{[ \t]*)\r?$'
$namePattern = '(?m)^(?<lead>[ \t]*Name[ \t]*\{[ \t]*)"(?<value>[^"]*)"(?<tail>[ \t]*\},[ \t]*)\r?$'
$matchNamePattern = '(?m)^(?<lead>[ \t]*AE_Effect_Match_Name[ \t]*\{[ \t]*)"(?<value>[^"]*)"(?<tail>[ \t]*\},[ \t]*)\r?$'
$resourceArgument = Get-RelativeBuildPath $IntermediateDir $ResourceSource
$headersArgument = Get-RelativeBuildPath $IntermediateDir $headerDir
$toolArgument = Get-RelativeBuildPath $IntermediateDir $toolPath
Push-Location $IntermediateDir
try {
    $batchPath = Join-Path $IntermediateDir 'BuildStarfieldPiPL.cmd'
    $batch = @"
@echo off
cl /nologo /Tc"$resourceArgument" /I "$headersArgument" /EP > "StarfieldPiPL.rr"
if errorlevel 1 exit /b %errorlevel%
"$toolArgument" "StarfieldPiPL.rr" "StarfieldPiPL.rrc"
if errorlevel 1 exit /b %errorlevel%
cl /nologo /TcStarfieldPiPL.rrc /D MSWindows /EP > "StarfieldPiPL.rc"
exit /b %errorlevel%
"@
    [System.IO.File]::WriteAllText($batchPath, $batch, [System.Text.Encoding]::ASCII)
    & $env:ComSpec /d /c 'BuildStarfieldPiPL.cmd'
    $toolExitCode = $LASTEXITCODE
}
finally {
    Pop-Location
}
if ($toolExitCode -ne 0) {
    # Some Windows images cannot start the legacy 32-bit Adobe PiPLtool.exe.
    # Reuse a previous conversion only when its preprocessed PiPL input is
    # byte-for-byte identical. The normal RC generation and version/flags
    # checks below still run on the reused output. A changed resource has no
    # fallback and continues to fail closed.
    $resourceRr = Join-Path $IntermediateDir 'StarfieldPiPL.rr'
    $resourceRrc = Join-Path $IntermediateDir 'StarfieldPiPL.rrc'
    $artifactRoot = Join-Path (Split-Path -Parent $PSScriptRoot) 'artifacts\plugin'
    $expectedHash = (Get-FileHash -LiteralPath $resourceRr -Algorithm SHA256).Hash
    $reused = $false
    if (Test-Path -LiteralPath $artifactRoot) {
        foreach ($cachedRr in Get-ChildItem -LiteralPath $artifactRoot -Filter 'StarfieldPiPL.rr' -File -Recurse -ErrorAction SilentlyContinue) {
            if ([IO.Path]::GetFullPath($cachedRr.FullName).Equals(
                [IO.Path]::GetFullPath($resourceRr),[StringComparison]::OrdinalIgnoreCase)) { continue }
            if ((Get-FileHash -LiteralPath $cachedRr.FullName -Algorithm SHA256).Hash -ne $expectedHash) { continue }
            $cachedRrc = Join-Path $cachedRr.DirectoryName 'StarfieldPiPL.rrc'
            if (-not (Test-Path -LiteralPath $cachedRrc) -or (Get-Item -LiteralPath $cachedRrc).Length -eq 0) { continue }
            Copy-Item -LiteralPath $cachedRrc -Destination $resourceRrc -Force
            Push-Location $IntermediateDir
            try {
                & $env:ComSpec /d /c 'cl /nologo /TcStarfieldPiPL.rrc /D MSWindows /EP > StarfieldPiPL.rc'
                $cacheRcExitCode = $LASTEXITCODE
            }
            finally { Pop-Location }
            if ($cacheRcExitCode -ne 0) { throw "Cached PiPL RC preprocessing failed with exit code $cacheRcExitCode." }
            Write-Warning "Adobe PiPLtool exited $toolExitCode; reused its cached conversion for identical PiPL input $expectedHash."
            $reused = $true
            break
        }
    }
    if (-not $reused) {
        # A packed-version, flags-2, or node-effect identity edit can be converted
        # without the legacy PiPLtool only when the current preprocessed resource
        # is byte-identical to a cached input after normalizing those exact fields.
        # Update only the matching eVER/srev and eGL2 values in the converted
        # resource; any other .rr/.rrc difference remains a hard failure.
        $currentText = [IO.File]::ReadAllText($resourceRr)
        $flags2Pattern = '(?m)^(?<lead>\s*AE_Effect_Global_OutFlags_2\s*\{\s*)(?<value>0x[0-9A-Fa-f]+L?)(?<tail>\s*\},\s*)$'
        $versionPattern = '(?m)^(?<lead>\s*(?:AE_Effect_Version|Version)\s*\{\s*)(?<value>\d+)(?<tail>\s*\},?\s*)$'
        $resourceIdPattern = '(?m)^(?<lead>[ \t]*resource[ \t]+''PiPL''[ \t]*\()(?<value>\d+)(?<tail>\)[ \t]*\{[ \t]*)\r?$'
        $namePattern = '(?m)^(?<lead>[ \t]*Name[ \t]*\{[ \t]*)"(?<value>[^"]*)"(?<tail>[ \t]*\},[ \t]*)\r?$'
        $matchNamePattern = '(?m)^(?<lead>[ \t]*AE_Effect_Match_Name[ \t]*\{[ \t]*)"(?<value>[^"]*)"(?<tail>[ \t]*\},[ \t]*)\r?$'
        $currentFlagsMatch = [regex]::Matches($currentText, $flags2Pattern)
        $currentVersionMatch = [regex]::Matches($currentText, $versionPattern)
        $currentResourceIdMatch = [regex]::Matches($currentText, $resourceIdPattern)
        $currentNameMatch = [regex]::Matches($currentText, $namePattern)
        $currentMatchNameMatch = [regex]::Matches($currentText, $matchNamePattern)
        $currentIdentityValid = $currentResourceIdMatch.Count -eq 1 -and $currentNameMatch.Count -eq 1
        if ($GeneralPlugin) { $currentIdentityValid = $currentIdentityValid -and $currentMatchNameMatch.Count -eq 0 }
        else { $currentIdentityValid = $currentIdentityValid -and $currentMatchNameMatch.Count -eq 1 }
        if ($currentFlagsMatch.Count -le 1 -and $currentVersionMatch.Count -eq 1 -and
            $currentIdentityValid) {
            $currentFlagsToken = if ($currentFlagsMatch.Count -eq 1) { $currentFlagsMatch[0].Groups['value'].Value } else { $null }
            $currentFlagsValue = if ($currentFlagsToken) { [Convert]::ToUInt32($currentFlagsToken.TrimEnd('L','l').Substring(2), 16) } else { 0 }
            $currentVersionToken = $currentVersionMatch[0].Groups['value'].Value
            $currentVersionValue = [Convert]::ToUInt32($currentVersionToken)
            foreach ($cachedRr in Get-ChildItem -LiteralPath $artifactRoot -Filter 'StarfieldPiPL.rr' -File -Recurse -ErrorAction SilentlyContinue) {
                if ([IO.Path]::GetFullPath($cachedRr.FullName).Equals(
                    [IO.Path]::GetFullPath($resourceRr),[StringComparison]::OrdinalIgnoreCase)) { continue }
                $cachedRrc = Join-Path $cachedRr.DirectoryName 'StarfieldPiPL.rrc'
                if (-not (Test-Path -LiteralPath $cachedRrc) -or (Get-Item -LiteralPath $cachedRrc).Length -eq 0) { continue }
                $cachedText = [IO.File]::ReadAllText($cachedRr.FullName)
                $cachedFlagsMatch = [regex]::Matches($cachedText, $flags2Pattern)
                $cachedVersionMatch = [regex]::Matches($cachedText, $versionPattern)
                $cachedResourceIdMatch = [regex]::Matches($cachedText, $resourceIdPattern)
                $cachedNameMatch = [regex]::Matches($cachedText, $namePattern)
                $cachedMatchNameMatch = [regex]::Matches($cachedText, $matchNamePattern)
                $cachedIdentityValid = $cachedResourceIdMatch.Count -eq 1 -and $cachedNameMatch.Count -eq 1
                if ($GeneralPlugin) { $cachedIdentityValid = $cachedIdentityValid -and $cachedMatchNameMatch.Count -eq 0 }
                else { $cachedIdentityValid = $cachedIdentityValid -and $cachedMatchNameMatch.Count -eq 1 }
                if ($cachedFlagsMatch.Count -ne $currentFlagsMatch.Count -or $cachedVersionMatch.Count -ne 1 -or
                    -not $cachedIdentityValid) { continue }
                $cachedFlagsToken = if ($cachedFlagsMatch.Count -eq 1) { $cachedFlagsMatch[0].Groups['value'].Value } else { $null }
                $cachedVersionToken = $cachedVersionMatch[0].Groups['value'].Value
                $normalizedCurrent = $currentText
                if ($currentFlagsMatch.Count -eq 1) {
                    $normalizedCurrent = [regex]::Replace($normalizedCurrent, $flags2Pattern,
                        [System.Text.RegularExpressions.MatchEvaluator]{ param($m) $m.Groups['lead'].Value + $cachedFlagsToken + $m.Groups['tail'].Value }, 1)
                }
                $normalizedCurrent = [regex]::Replace($normalizedCurrent, $versionPattern,
                    [System.Text.RegularExpressions.MatchEvaluator]{ param($m) $m.Groups['lead'].Value + $cachedVersionToken + $m.Groups['tail'].Value }, 1)
                if (-not $GeneralPlugin) {
                    $cachedResourceIdToken = $cachedResourceIdMatch[0].Groups['value'].Value
                    $cachedNameToken = $cachedNameMatch[0].Groups['value'].Value
                    $cachedMatchNameToken = $cachedMatchNameMatch[0].Groups['value'].Value
                    $normalizedCurrent = [regex]::Replace($normalizedCurrent, $resourceIdPattern,
                        [System.Text.RegularExpressions.MatchEvaluator]{ param($m) $m.Groups['lead'].Value + $cachedResourceIdToken + $m.Groups['tail'].Value }, 1)
                    $normalizedCurrent = [regex]::Replace($normalizedCurrent, $namePattern,
                        [System.Text.RegularExpressions.MatchEvaluator]{ param($m) $m.Groups['lead'].Value + '"' + $cachedNameToken + '"' + $m.Groups['tail'].Value }, 1)
                    $normalizedCurrent = [regex]::Replace($normalizedCurrent, $matchNamePattern,
                        [System.Text.RegularExpressions.MatchEvaluator]{ param($m) $m.Groups['lead'].Value + '"' + $cachedMatchNameToken + '"' + $m.Groups['tail'].Value }, 1)
                }
                $normalizedCurrent = [string]::Join("`n", @($normalizedCurrent -split '\r?\n' | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }))
                $normalizedCached = [string]::Join("`n", @($cachedText -split '\r?\n' | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }))
                if (-not $normalizedCurrent.Equals($normalizedCached, [StringComparison]::Ordinal)) { continue }

                $resourceTextUpdated = [IO.File]::ReadAllText($cachedRrc)
                if ($currentVersionToken -ne $cachedVersionToken) {
                    if ($GeneralPlugin) {
                        $srevPattern = '(?s)("srev"\s*,\s*/\*\s*''vers''\s*PIVersionProperty\s*\*/\s*\r?\n\s*RSCS32\(0\),\s*\r?\n\s*RSCS32\(4\),\s*)(?<value>\d+)(?<tail>\s*,\s*)(?<high>\d+)'
                        $srevMatches = [regex]::Matches($resourceTextUpdated, $srevPattern)
                        if ($srevMatches.Count -ne 1 -or [uint32]$srevMatches[0].Groups['value'].Value -ne [uint32]$cachedVersionToken) { continue }
                        $low = $currentVersionValue -band 65535
                        $high = $currentVersionValue -shr 16
                        $resourceTextUpdated = [regex]::Replace($resourceTextUpdated, $srevPattern,
                            [System.Text.RegularExpressions.MatchEvaluator]{ param($m) $m.Groups[1].Value + $low + $m.Groups['tail'].Value + $high }, 1)
                    } else {
                        $eVERPattern = '(?s)(0x65564552L\s*,\s*/\*\s*''eVER''\s*\*/\s*\r?\n\s*0L,\s*\r?\n\s*4L\s*\r?\n\s*)(?<value>\d+)L(?=\s*,)'
                        $eVERMatches = [regex]::Matches($resourceTextUpdated, $eVERPattern)
                        if ($eVERMatches.Count -ne 1 -or [uint32]$eVERMatches[0].Groups['value'].Value -ne [uint32]$cachedVersionToken) { continue }
                        $resourceTextUpdated = [regex]::Replace($resourceTextUpdated, $eVERPattern,
                            [System.Text.RegularExpressions.MatchEvaluator]{ param($m) $m.Groups[1].Value + $currentVersionValue + 'L' }, 1)
                    }
                }
                if ($currentFlagsMatch.Count -eq 1 -and $currentFlagsToken -ne $cachedFlagsToken) {
                    $cachedFlagsValue = [Convert]::ToUInt32($cachedFlagsToken.TrimEnd('L','l').Substring(2), 16)
                    $oldDecimal = [string]$cachedFlagsValue
                    $newDecimal = [string]$currentFlagsValue
                    $eGL2Pattern = '(?s)("2LGe"\s*,\s*/\*\s*''eGL2''\s*\*/\s*\r?\n\s*0L,\s*\r?\n\s*4L,\s*\r?\n\s*)' + [regex]::Escape($oldDecimal) + 'L(?=\s*,)'
                    $eGL2Matches = [regex]::Matches($resourceTextUpdated, $eGL2Pattern)
                    if ($eGL2Matches.Count -ne 1) { continue }
                    $resourceTextUpdated = [regex]::Replace($resourceTextUpdated, $eGL2Pattern,
                        [System.Text.RegularExpressions.MatchEvaluator]{ param($m) $m.Groups[1].Value + $newDecimal + 'L' }, 1)
                }
                if (-not $GeneralPlugin) {
                    $currentResourceId = [uint32]$currentResourceIdMatch[0].Groups['value'].Value
                    $cachedResourceId = [uint32]$cachedResourceIdMatch[0].Groups['value'].Value
                    $resourceHeaderPattern = '(?m)^(?<lead>[ \t]*)' + [regex]::Escape([string]$cachedResourceId) + '(?<tail>[ \t]+PiPL[ \t]+DISCARDABLE[ \t]*)\r?$'
                    $resourceHeaderMatches = [regex]::Matches($resourceTextUpdated, $resourceHeaderPattern)
                    if ($resourceHeaderMatches.Count -ne 1) { continue }
                    $resourceTextUpdated = [regex]::Replace($resourceTextUpdated, $resourceHeaderPattern,
                        [System.Text.RegularExpressions.MatchEvaluator]{ param($m) $m.Groups['lead'].Value + $currentResourceId + $m.Groups['tail'].Value }, 1)
                    $resourceTextUpdated = Set-PiPLRrcStringProperty $resourceTextUpdated 'eman' $currentNameMatch[0].Groups['value'].Value $true
                    $resourceTextUpdated = Set-PiPLRrcStringProperty $resourceTextUpdated 'ANMe' $currentMatchNameMatch[0].Groups['value'].Value $true
                }
                [IO.File]::WriteAllText($resourceRrc, $resourceTextUpdated)
                Push-Location $IntermediateDir
                try {
                    & $env:ComSpec /d /c 'cl /nologo /TcStarfieldPiPL.rrc /D MSWindows /EP > StarfieldPiPL.rc'
                    $cacheRcExitCode = $LASTEXITCODE
                }
                finally { Pop-Location }
                if ($cacheRcExitCode -ne 0) { throw "Normalized PiPL RC preprocessing failed with exit code $cacheRcExitCode." }
                Write-Warning "Adobe PiPLtool exited $toolExitCode; reused a cached conversion after proving that only packed version, global out-flags 2, and node effect identity fields changed."
                $reused = $true
                break
            }
        }
    }
    if (-not $reused) { throw "Adobe PiPL resource pipeline failed with exit code $toolExitCode and no exact-input cached conversion." }
}

# Guard against a stale PiPL resource. The custom build step tracks the .r file, not
# the headers it includes, so a flags/version edit can silently leave an old PiPL in
# the .aex; AE then rejects the plug-in with "global outflags mismatch". Compare the
# generated resource against the single-source headers and fail loudly instead.
$generatedRc = Join-Path $IntermediateDir 'StarfieldPiPL.rc'
if (-not (Test-Path -LiteralPath $generatedRc)) { throw "PiPL pipeline did not produce $generatedRc" }

$flagText = [System.IO.File]::ReadAllText($FlagHeaderPath)
$versionText = [System.IO.File]::ReadAllText($VersionHeaderPath)
$resourceText = [System.IO.File]::ReadAllText($generatedRc)

if (-not $GeneralPlugin) {
    $identityRr = [IO.File]::ReadAllText((Join-Path $IntermediateDir 'StarfieldPiPL.rr'))
    $identityResourceId = [regex]::Matches($identityRr, $resourceIdPattern)
    $identityName = [regex]::Matches($identityRr, $namePattern)
    $identityMatchName = [regex]::Matches($identityRr, $matchNamePattern)
    if ($identityResourceId.Count -ne 1 -or $identityName.Count -ne 1 -or $identityMatchName.Count -ne 1) {
        throw 'Generated node PiPL input has invalid or duplicate identity fields.'
    }
    $expectedResourceHeader = [string]$identityResourceId[0].Groups['value'].Value + '  PiPL  DISCARDABLE'
    $expectedNameData = Get-PiPLStringData $identityName[0].Groups['value'].Value $true
    $expectedMatchData = Get-PiPLStringData $identityMatchName[0].Groups['value'].Value $true
    if (-not $resourceText.Contains($expectedResourceHeader) -or
        -not $resourceText.Contains('"' + $expectedNameData.Encoded + '"') -or
        -not $resourceText.Contains('"' + $expectedMatchData.Encoded + '"')) {
        throw 'Generated node PiPL resource identity differs from its preprocessed source.'
    }
}

$entries = @(
        @{ Name = $OutFlagsName; Text = $flagText; Source = $FlagHeaderPath },
        @{ Name = $OutFlags2Name; Text = $flagText; Source = $FlagHeaderPath },
        @{ Name = 'STARFIELD_VERSION_PACKED'; Text = $versionText; Source = $VersionHeaderPath })
if ($GeneralPlugin) {
    $preprocessed = [IO.File]::ReadAllText((Join-Path $IntermediateDir 'StarfieldPiPL.rr'))
    if ($preprocessed -notmatch 'Kind\s*\{\s*AEGP\s*\}' -or
        $preprocessed -notmatch 'CodeWin64X86\s*\{\s*"StarfieldHostEntry"\s*\}') {
        throw 'General PiPL must declare AEGP and the independent StarfieldHostEntry export.'
    }
    $entries = @($entries | Where-Object { $_.Name -eq 'STARFIELD_VERSION_PACKED' })
}
foreach ($entry in $entries) {
    $match = [regex]::Match($entry.Text, "#define\s+$($entry.Name)\s+(0x[0-9A-Fa-f]+|\d+)")
    if (-not $match.Success) { throw "Could not read $($entry.Name) from $($entry.Source)" }

    $literal = $match.Groups[1].Value
    $value = if ($literal.StartsWith('0x')) {
        [System.Convert]::ToUInt32($literal, 16)
    }
    else {
        [System.Convert]::ToUInt32($literal)
    }

    $valuePattern = "(?<![\d])$value\s*L"
    if ($GeneralPlugin) {
        $low = $value -band 65535
        $high = $value -shr 16
        $valuePattern = '"srev"[\s\S]*?4,\s*0x0,\s*' + $low + ',\s*' + $high + ','
        if ($resourceText -notmatch '"xgEA"' -or $resourceText -notmatch '"StarfieldHostEntry\\0') {
            throw 'Generated General PiPL has an incorrect kind or entry point.'
        }
    }
    if ($resourceText -notmatch $valuePattern) {
        throw "Generated PiPL does not declare $($entry.Name) = $value ($literal). The PiPL resource is stale: delete the intermediate directory and rebuild."
    }
}
