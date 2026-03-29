# Install Nuvoton M4 DFP via Keil PackInstaller
$uv4Path = "C:\Users\rinry\AppData\Local\Keil_v5\UV4\UV4.exe"
$packInstallerPath = "C:\Users\rinry\AppData\Local\Keil_v5\UV4\PackInstaller.exe"
$nuvotonM4Pack = "C:\Users\rinry\AppData\Local\Keil_v5\ARM\PACK\.Web\Nuvoton\Nuvoton.NuMicroM4_DFP.pdsc"
$nuvotonPack = "C:\Users\rinry\AppData\Local\Keil_v5\ARM\PACK\.Web\Nuvoton\Nuvoton.NuMicro_DFP.pdsc"

Write-Host "=== Keil MDK Environment ==="
if (Test-Path $uv4Path) { Write-Host "UV4.exe: OK" } else { Write-Host "UV4.exe: NOT FOUND" }
if (Test-Path $packInstallerPath) { Write-Host "PackInstaller.exe: OK" } else { Write-Host "PackInstaller.exe: NOT FOUND" }
if (Test-Path $nuvotonM4Pack) { Write-Host "Nuvoton.NuMicroM4_DFP.pdsc: OK" } else { Write-Host "Nuvoton.NuMicroM4_DFP.pdsc: NOT FOUND" }
if (Test-Path $nuvotonPack) { Write-Host "Nuvoton.NuMicro_DFP.pdsc: OK" } else { Write-Host "Nuvoton.NuMicro_DFP.pdsc: NOT FOUND" }

# Check Nu-Link USB driver
Write-Host ""
Write-Host "=== Nu-Link USB Driver ==="
$driverFiles = Get-ChildItem "C:\Users\rinry\AppData\Local\Keil_v5\ARM\NULink\Nu_Link_Driver.ini" -ErrorAction SilentlyContinue
if ($driverFiles) { Write-Host "Nu_Link_Driver.ini: OK" } else { Write-Host "Nu_Link_Driver.ini: NOT FOUND" }

# Check if Nu-Link USB driver installer exists
$driverInstaller = "C:\Users\rinry\AppData\Local\Keil_v5\ARM\NULink\Nu-Link_USB_Driver 1.11.exe"
if (Test-Path $driverInstaller) {
    Write-Host "Nu-Link USB Driver installer: EXISTS"
    Write-Host "  Run as admin to install Keil Nu-Link driver"
} else {
    Write-Host "Nu-Link USB Driver installer: NOT FOUND"
}

# Show the nu_link driver INI content (chip names supported)
Write-Host ""
Write-Host "=== Nu-Link Driver INI - supported chips ==="
$iniContent = Get-Content "C:\Users\rinry\AppData\Local\Keil_v5\ARM\NULink\Nu_Link_Driver.ini" -Raw
$chipSections = [regex]::Matches($iniContent, '^\[([^\]]+)\]', [System.Text.RegularExpressions.RegexOptions]::Multiline) | ForEach-Object { $_.Groups[1].Value }
Write-Host "Supported chip families:"
foreach ($chip in $chipSections) {
    Write-Host "  - $chip"
}
