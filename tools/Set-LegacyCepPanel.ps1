# Exact-path, reversible retirement of this project's legacy CEP registration.
# Default is a read-only report. No registry, other extension or shared cache access.
param(
    [ValidateSet('Report','Disable','Restore')][string]$Action='Report',
    [string]$ExtensionPath='C:\Program Files (x86)\Common Files\Adobe\CEP\extensions\cep_panel',
    [string]$QualificationReport=''
)
$ErrorActionPreference='Stop'
Import-Module Microsoft.PowerShell.Utility
$taskRepo=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$taskSource=[IO.Path]::GetFullPath((Join-Path $taskRepo 'web_panel')).TrimEnd('\')
$taskOldSource=[IO.Path]::GetFullPath((Join-Path $taskRepo 'cep_panel')).TrimEnd('\')
$taskExtension=[IO.Path]::GetFullPath($ExtensionPath).TrimEnd('\')
# Renaming inside extensions does not disable CEP (it still scans the manifest).
# Use a sibling outside the scan root, on the same volume. Never copy a Junction.
$taskScanRoot=Split-Path -Parent $taskExtension
if((Split-Path -Leaf $taskScanRoot) -cne 'extensions' -or (Split-Path -Leaf (Split-Path -Parent $taskScanRoot)) -cne 'CEP'){
    throw 'Expected an explicit Adobe CEP/extensions registration path.'
}
$taskRetained=Join-Path (Split-Path -Parent $taskScanRoot) 'starfield-legacy-cep-disabled'
if($taskExtension -eq $taskRetained -or [IO.Path]::GetPathRoot($taskExtension) -ne [IO.Path]::GetPathRoot($taskRetained)){
    throw 'Retirement requires distinct paths on the same volume.'
}
function Assert-OwnJunction([string]$taskPath){
    $taskItem=Get-Item -LiteralPath $taskPath -Force
    if($taskItem.LinkType -ne 'Junction'){throw "Refusing a non-Junction: $taskPath"}
    $taskTarget=[IO.Path]::GetFullPath([string]$taskItem.Target).TrimEnd('\')
    if(-not $taskTarget.Equals($taskSource,[StringComparison]::OrdinalIgnoreCase) -and
       -not $taskTarget.Equals($taskOldSource,[StringComparison]::OrdinalIgnoreCase)){throw "Not this project's CEP registration: $taskPath -> $taskTarget"}
}
function Test-Registration([string]$taskPath){return [bool](Get-Item -LiteralPath $taskPath -Force -ErrorAction SilentlyContinue)}
$taskBefore=[ordered]@{action=$Action;extension=$taskExtension;retained=$taskRetained;source=$taskSource;
    extensionExists=(Test-Registration $taskExtension);retainedExists=(Test-Registration $taskRetained)}
if($taskBefore.extensionExists){Assert-OwnJunction $taskExtension}
if($taskBefore.retainedExists){Assert-OwnJunction $taskRetained}
$taskBefore | ConvertTo-Json
Write-Host "Undo: powershell -NoProfile -ExecutionPolicy Bypass -File tools/Set-LegacyCepPanel.ps1 -Action Restore -ExtensionPath '$taskExtension'"
if($Action -eq 'Report'){Write-Host 'Read-only. Actual AE qualification and exact-path host authorization are required before Disable.';exit 0}
if(Get-Process AfterFX,AfterFX_64 -ErrorAction SilentlyContinue){throw 'Close AE before changing the extension registration; this script never stops a process.'}
if($Action -eq 'Disable'){
    if(-not $taskBefore.extensionExists -or $taskBefore.retainedExists){throw 'Expected only the active legacy Junction.'}
    if(-not $QualificationReport){throw 'Actual AE qualification report is required before retirement.'}
    $taskQualification=(Resolve-Path -LiteralPath $QualificationReport).Path
    $taskArtifacts=[IO.Path]::GetFullPath((Join-Path $taskRepo 'artifacts'))+'\'
    if(-not $taskQualification.StartsWith($taskArtifacts,[StringComparison]::OrdinalIgnoreCase)){throw 'Keep the qualification report under checkout artifacts.'}
    $taskEvidence=Get-Content -LiteralPath $taskQualification -Raw -Encoding UTF8 | ConvertFrom-Json
    if($taskEvidence.host -cne 'After Effects 2023.5.0 Build 52' -or
       $taskEvidence.kind -cnotin @('actual-ae-webview2-qualification','actual-ae-webview2-owner-acceptance')){
        throw 'Offline/fake-host evidence cannot qualify CEP retirement.'
    }
    $taskHost=Join-Path $taskRepo 'dist/StarfieldHost.aex'
    if((Get-FileHash -LiteralPath $taskHost -Algorithm SHA256).Hash -cne $taskEvidence.nativeHostSha256){throw 'Qualification is for a different installed native Host.'}
    if($taskEvidence.kind -ceq 'actual-ae-webview2-owner-acceptance'){
        # A direct owner release decision is distinct from instrumented matrix
        # qualification. Preserve that distinction instead of inventing passes.
        # This evidence is not authorization to operate outside the checkout.
        if($taskEvidence.acceptedBy -cne 'project-owner' -or
           $taskEvidence.retirementApproved -isnot [bool] -or -not $taskEvidence.retirementApproved -or
           [string]::IsNullOrWhiteSpace($taskEvidence.ownerStatement)){
            throw 'Explicit current-version owner acceptance is required.'
        }
        $taskFiles=@(Get-ChildItem -LiteralPath $taskSource -File -Recurse)
        if(@($taskEvidence.panelFiles).Count -ne $taskFiles.Count){throw 'Owner acceptance panel file set is incomplete.'}
        $taskSeen=@{}
        foreach($taskFile in $taskEvidence.panelFiles){
            $taskRelative=[string]$taskFile.path
            if($taskRelative -notmatch '^[A-Za-z0-9_./-]+$' -or $taskRelative.Split('/') -contains '..' -or $taskSeen.ContainsKey($taskRelative)){
                throw 'Invalid or duplicate accepted panel path.'
            }
            $taskSeen[$taskRelative]=$true
            if((Get-FileHash -LiteralPath (Join-Path $taskSource $taskRelative) -Algorithm SHA256).Hash -cne $taskFile.sha256){
                throw "Owner acceptance is for different panel bytes: $taskRelative"
            }
        }
        foreach($taskFile in $taskFiles){
            $taskRelative=$taskFile.FullName.Substring($taskSource.Length+1).Replace('\','/')
            if(-not $taskSeen.ContainsKey($taskRelative)){throw 'Unrecorded panel file in owner acceptance.'}
        }
        Write-Host 'Owner accepted this installed version; unreported matrix cases are not marked passed.'
    }else{
        foreach($taskGate in @('dockingLifecycle','inputImeFocus','allNodeTopology','nativeAuthorAnimation','presetsAndModel','undoPersistence','failureHeadless')){
            if($taskEvidence.gates.$taskGate -isnot [bool] -or $taskEvidence.gates.$taskGate -ne $true){throw "Actual AE gate remains open: $taskGate"}
        }
        foreach($taskKind in @('Emitter','Auxiliary','Particle','Force','Transform','Motion','Turbulence','Model','Material','Output')){
            if($taskKind -cnotin @($taskEvidence.nodeKinds)){throw "Missing qualified node kind: $taskKind"}
        }
    }
    Assert-OwnJunction $taskExtension
    [IO.Directory]::Move($taskExtension,$taskRetained)
    Assert-OwnJunction $taskRetained
    if(Test-Registration $taskExtension){throw 'Legacy registration still exists after retirement.'}
}else{
    if($taskBefore.extensionExists -or -not $taskBefore.retainedExists){throw 'Expected only the retained legacy Junction.'}
    Assert-OwnJunction $taskRetained
    $taskSavedLink=Get-Item -LiteralPath $taskRetained -Force
    if(([IO.Path]::GetFullPath([string]$taskSavedLink.Target)).TrimEnd('\') -eq $taskSource){
        [IO.Directory]::Move($taskRetained,$taskExtension)
    }else{
        # Only the validated old Junction is removed, never its target. Rebind
        # an explicitly requested legacy restore to the renamed current source.
        if(-not(Test-Path -LiteralPath $taskSource)){throw 'Current web_panel source is missing.'}
        New-Item -ItemType Junction -Path $taskExtension -Target $taskSource | Out-Null
        [IO.Directory]::Delete($taskRetained,$false)
    }
    Assert-OwnJunction $taskExtension
}
$taskRecord=Join-Path $taskRepo ('artifacts/webview2/cep-registration-'+(Get-Date -Format 'yyyyMMdd-HHmmssfff')+'.json')
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $taskRecord) | Out-Null
[ordered]@{before=$taskBefore;qualificationReport=$QualificationReport;after=@{extensionExists=(Test-Registration $taskExtension);retainedExists=(Test-Registration $taskRetained)}} |
    ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $taskRecord -Encoding UTF8
Write-Host "Recorded before/after: $taskRecord. Registry and all other extensions unchanged."
