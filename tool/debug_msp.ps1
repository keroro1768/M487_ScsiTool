$f = "D:\AiWorkSpace\KM\M487-Examples\Projects\HSUSBD_Mass_Storage_ShortPacket\KEIL\HSUSBD_Mass_Storage_ShortPacket.uvprojx"
$lib = "D:\AiWorkSpace\KM\M487-Examples\Projects\HSUSBD_Mass_Storage_ShortPacket\Library"
$content = Get-Content $f -Raw
# Find a sample line with Library path
$lines = $content -split "`n"
foreach ($line in $lines) {
    if ($line -match 'Library\\Device') {
        Write-Host "Found: $line"
        break
    }
}
# Try different patterns
$test = '..\..\..\..\Library\Device'
Write-Host "Test match: $([bool]($content -match [regex]::Escape($test)))"
$test2 = '..\..\..\..\Library'
Write-Host "Test2 match: $([bool]($content -match [regex]::Escape($test2)))"
