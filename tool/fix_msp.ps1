$f = "D:\AiWorkSpace\KM\M487-Examples\Projects\HSUSBD_Mass_Storage_ShortPacket\KEIL\HSUSBD_Mass_Storage_ShortPacket.uvprojx"
$lib = "D:\AiWorkSpace\KM\M487-Examples\Projects\HSUSBD_Mass_Storage_ShortPacket\Library"
$content = Get-Content $f -Raw
# Replace relative Library paths (both forward slash and backslash)
$content = $content -replace '\.\./\.\./\.\./\.\./Library', $lib
$content = $content -replace '\.\.\\\.\.\\\.\.\\\.\.\\Library', $lib
# Fix the CMSIS pack path (replace absolute with our local copy)
$content = $content -replace 'C:/Users/rinry/AppData/Local/Arm/Packs/ARM/CMSIS/6.3.0/CMSIS/Core/Include', $lib
Set-Content -Path $f -Value $content -NoNewline
Write-Host "Done"
