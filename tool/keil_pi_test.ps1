# Find Keil PackInstaller settings and try to trigger install
$pdsc = "C:\Users\rinry\AppData\Local\Keil_v5\ARM\PACK\.Web\Nuvoton.NuMicroM4_DFP.pdsc"
if (Test-Path $pdsc) {
    Write-Host "=== NuMicroM4_DFP.pdsc ==="
    $content = Get-Content $pdsc -Raw
    if ($content -match 'vendor="([^"]+)"') { Write-Host "Vendor: $($Matches[1])" }
    if ($content -match 'url="([^"]+)"') { Write-Host "URL: $($Matches[1])" }
    if ($content -match 'version="([^"]+)"') { Write-Host "Version: $($Matches[1])" }
    if ($content -match '<release([^>]+)>') { Write-Host "Release: $($Matches[1])" }
}

# Try PackInstaller's built-in update/check
Write-Host ""
Write-Host "=== Trying PackInstaller CLI ==="
$pi = "C:\Users\rinry\AppData\Local\Keil_v5\UV4\PackInstaller.exe"
$cmdLine = @(
    "-r $pdsc",
    "--pack $pdsc"
)
foreach ($arg in $cmdLine) {
    Write-Host "Trying: $pi $arg"
    $out = "$env:TEMP\pi_test.txt"
    $err = "$env:TEMP\pi_err.txt"
    $proc = Start-Process $pi -ArgumentList $arg -Wait -PassThru -NoNewWindow -RedirectStandardOutput $out -RedirectStandardError $err
    Write-Host "Exit code: $($proc.ExitCode)"
    if (Test-Path $out) { Get-Content $out | Select-Object -First 10 }
    if (Test-Path $err) { Get-Content $err | Select-Object -First 10 }
}
