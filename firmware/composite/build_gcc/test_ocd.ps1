# OpenOCD telnet test script
$client = New-Object System.Net.Sockets.TcpClient
$client.Connect("localhost", 4444)
$stream = $client.GetStream()
$writer = New-Object System.IO.StreamWriter($stream)
$reader = New-Object System.IO.StreamReader($stream)
$writer.AutoFlush = $true
$stream.ReadTimeout = 5000

# Send halt command
$writer.WriteLine("halt")
$resp = $reader.ReadLine()
Write-Host "halt: $resp"

# Read 16 words from flash 0x00000000
$writer.WriteLine("mdw 0x00000000 16")
$resp = $reader.ReadToEnd()
Write-Host $resp

$writer.WriteLine("exit")
$client.Close()
