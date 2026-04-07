$bytes = [System.IO.File]::ReadAllBytes('D:\AiWorkSpace\KM\M480BSP\SampleCode\StdDriver\HSUSBD_Mass_Storage_ShortPacket\KEIL\obj\HSUSBD_Mass_Storage_ShortPacket.bin')
$header = $bytes[0..31]
$hex = ($header | ForEach-Object { $_.ToString('X2') }) -join ' '
Write-Host "Keil Binary Header: $hex"
Write-Host ("Size: {0} bytes ({1} KB)" -f $bytes.Length, ($bytes.Length / 1024))