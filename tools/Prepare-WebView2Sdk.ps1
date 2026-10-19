param([switch]$Download)
$ErrorActionPreference='Stop'
$taskRepo=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$taskVersion='1.0.4258.31'
$taskSha='56F7F4B8BF9AEE4B8EFEFBBDD4F67D5F74EBD1B100ED0806DA71BF76AF481AA9'
$taskDirectory=Join-Path $taskRepo 'artifacts/webview2/dependencies'
$taskPackage=Join-Path $taskDirectory "microsoft.web.webview2.$taskVersion.nupkg"
$taskSdk=Join-Path $taskDirectory "sdk-$taskVersion"
if(-not $Download){
    Write-Host "Report only: pinned Microsoft WebView2 SDK $taskVersion under $taskDirectory."
    Write-Host 'Add -Download to fetch and hash-check the official NuGet package. No runtime installation, registry, AE or user-profile writes.'
    if(Test-Path -LiteralPath $taskPackage){Write-Host "Package SHA256: $((Get-FileHash -LiteralPath $taskPackage -Algorithm SHA256).Hash)"}
    exit 0
}
New-Item -ItemType Directory -Force -Path $taskDirectory | Out-Null
if(-not (Test-Path -LiteralPath $taskPackage)){
    Invoke-WebRequest -UseBasicParsing -Uri "https://api.nuget.org/v3-flatcontainer/microsoft.web.webview2/$taskVersion/microsoft.web.webview2.$taskVersion.nupkg" -OutFile $taskPackage
}
if((Get-FileHash -LiteralPath $taskPackage -Algorithm SHA256).Hash -cne $taskSha){throw 'WebView2 SDK package SHA256 mismatch; refusing extraction.'}
if(-not (Test-Path -LiteralPath $taskSdk)){
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $taskZip=[IO.Compression.ZipFile]::OpenRead($taskPackage)
    try{
        foreach($taskEntry in $taskZip.Entries){
            $taskTarget=[IO.Path]::GetFullPath((Join-Path $taskSdk $taskEntry.FullName))
            if(-not $taskTarget.StartsWith($taskSdk+'\',[StringComparison]::OrdinalIgnoreCase)){throw 'SDK archive path escapes its destination.'}
        }
    }finally{$taskZip.Dispose()}
    [IO.Compression.ZipFile]::ExtractToDirectory($taskPackage,$taskSdk)
}
foreach($taskFile in @('build/native/include/WebView2.h','build/native/x64/WebView2LoaderStatic.lib')){
    if(-not (Test-Path -LiteralPath (Join-Path $taskSdk $taskFile))){throw "Incomplete WebView2 SDK: $taskFile"}
}
Write-Host "Verified WebView2 SDK $taskVersion at $taskSdk. SHA256 $taskSha. Runtime unchanged."
