# OpenOCD telnet test - read flash
$client = New-Object System.Net.Sockets.TcpClient
$client.Connect("localhost", 4444)
$stream = $client.GetStream()
$writer = New-Object System.IO.StreamWriter($stream)
$reader = New-Object System.IO.StreamReader($stream)
$writer.AutoFlush = $true
$stream.ReadTimeout = 3000

# Skip banner lines
for ($i = 0; $i -lt 5; $i++) { $reader.ReadLine() | Out-Null }

# Read 8 words from flash 0x00000000
$writer.WriteLine("mdw 0x00000000 8")
Start-Sleep -Milliseconds 500
$lines = @()
try {
    while ($true) {
        $line = $reader.ReadLine()
        if ($line -eq $null) { break }
        $lines += $line
        if ($line -match ">" -or $line -match "timeout") { break }
    }
} catch {}
$lines | ForEach-Object { Write-Host $_ }

$writer.WriteLine("exit")
$client.Close()
