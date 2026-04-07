# OpenOCD telnet - write firmware to APROM flash
$client = New-Object System.Net.Sockets.TcpClient
$client.Connect("localhost", 4444)
$stream = $client.GetStream()
$writer = New-Object System.IO.StreamWriter($stream)
$reader = New-Object System.IO.StreamReader($stream)
$writer.AutoFlush = $true
$stream.ReadTimeout = 10000

# Skip banner
for ($i = 0; $i -lt 5; $i++) { $reader.ReadLine() | Out-Null }

# Halt target
$writer.WriteLine("halt")
Start-Sleep -Milliseconds 500
$reader.ReadLine() | Out-Null

# Check current pc
$writer.WriteLine("reg pc")
for ($i = 0; $i -lt 3; $i++) { Write-Host $reader.ReadLine() }

# Write firmware to flash at 0x00000000
Write-Host "Writing firmware to flash..."
$writer.WriteLine("flash write_image erase D:/AiWorkSpace/M487_ScsiTool/firmware/composite/build_gcc/firmware.bin 0x00000000 bin")
$resp = $reader.ReadToEnd()
Write-Host $resp

$writer.WriteLine("exit")
$client.Close()
