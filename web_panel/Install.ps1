# Legacy CEP installation is retired. Never re-register it by default.
param([switch]$Uninstall)
$ErrorActionPreference='Stop'
Write-Host 'Starfield now uses AEGP Panel + WebView2 and the web_panel source directory.'
Write-Host 'Report only: no CEP registration, registry, cache or host file was changed.'
Write-Host 'Use tools/Deploy-TestBuild.ps1 for the native pairing.'
Write-Host 'Explicit legacy cleanup/restore: tools/Set-LegacyCepPanel.ps1; follow ADR0011.'
if($Uninstall){throw 'Use the exact-path legacy retirement tool; this obsolete installer performs no host action.'}
