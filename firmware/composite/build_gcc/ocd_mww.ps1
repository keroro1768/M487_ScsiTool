# OpenOCD telnet - write and read FMC
$client = New-Object System.Net.Sockets.TcpClient
$client.Connect("localhost", 4444)
$stream = $client.GetStream()
$writer = New-Object System.IO.StreamWriter($stream)
$reader = New-Object System.IO.StreamReader($stream)
$writer.AutoFlush = $true
$stream.ReadTimeout = 5000

# Skip banner
for ($i = 0; $i -lt 3; $i++) { $reader.ReadLine() | Out-Null }

# Try mww (memory write word) to enable FMC ISP
Write-Host "=== Write 0x01 to FMC ISPCTL ==="
$writer.WriteLine("mww 0x400C0000 0x01")
Start-Sleep -Milliseconds 300
$lines = @()
try {
    for ($i = 0; $i -lt 5; $i++) {
        $line = $reader.ReadLine()
        if ($line -eq $null) { break }
        $lines += $line
    }
} catch {}
$lines | ForEach-Object { Write-Host $_ }

# Read back
Write-Host "`n=== Read FMC ISPCTL ==="
$writer.WriteLine("mdw 0x400C0000 2")
Start-Sleep -Milliseconds 300
$lines = @()
try {
    for ($i = 0; $i -lt 5; $i++) {
        $line = $reader.ReadLine()
        if ($line -eq $null) { break }
        $lines += $line
    }
} catch {}
$lines | ForEach-Object { Write-Host $_ }

$writer.WriteLine("exit")
$client.Close()
