$bytes = [System.IO.File]::ReadAllBytes('D:\AiWorkSpace\M487_ScsiTool\firmware\composite\build_gcc\firmware.bin')
$header = $bytes[0..31]
$hex = ($header | ForEach-Object { $_.ToString('X2') }) -join ' '
Write-Host "Firmware header (first 32 bytes):"
Write-Host $hex
Write-Host ""
Write-Host ("Size: {0} bytes ({1} KB)" -f $bytes.Length, ($bytes.Length / 1024))
Write-Host ""
Write-Host "Vector table:"
Write-Host ("  Reset:     0x{0:X8}" -f [BitConverter]::ToUInt32($bytes, 0))
Write-Host ("  NMI:       0x{0:X8}" -f [BitConverter]::ToUInt32($bytes, 4))
Write-Host ("  HardFault: 0x{0:X8}" -f [BitConverter]::ToUInt32($bytes, 8))