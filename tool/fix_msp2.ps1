$f = "D:\AiWorkSpace\KM\M487-Examples\Projects\HSUSBD_Mass_Storage_ShortPacket\KEIL\HSUSBD_Mass_Storage_ShortPacket.uvprojx"
$lib = "D:\AiWorkSpace\KM\M487-Examples\Projects\HSUSBD_Mass_Storage_ShortPacket\Library"
$content = Get-Content $f -Raw
Write-Host "Before: $(($content -replace 'Library').Length)"
$content = $content -replace '\.\./\.\./\.\./\.\./Library', $lib
$content = $content -replace '\.\.\\\.\.\\\.\.\\\.\.\\Library', $lib
Write-Host "After: $(($content -replace 'Library').Length)"
Set-Content -Path $f -Value $content -NoNewline -Encoding UTF8
Write-Host "Saved"
