# Read-only preflight shared by the launcher and offline checks. No process calls.
function Get-StarfieldAcceptanceBuild([string]$Directory,[string]$GatewayPath) {
    $taskFixture=Get-Content -LiteralPath (Join-Path $Directory 'fixtures.json') -Raw -Encoding UTF8 | ConvertFrom-Json
    if($taskFixture.version -ne 1 -or $taskFixture.gatewayBuild -isnot [string] -or
        $taskFixture.gatewayBuild -notmatch '^native-presets-[1-9][0-9]*$') {throw 'Invalid prepared acceptance generation.'}
    $taskExpected=$taskFixture.gatewayBuild
    $taskInstalled=[IO.File]::ReadAllText($GatewayPath)
    if(-not $taskInstalled.Contains('var GATEWAY_BUILD = "'+$taskExpected+'"')) {
        throw ('Install the checked '+$taskExpected+' pair before running acceptance.')
    }
    return $taskExpected
}
