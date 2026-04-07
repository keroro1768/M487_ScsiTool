# OpenOCD direct telnet - reset and read chip
$client = New-Object System.Net.Sockets.TcpClient
$client.Connect("localhost", 4444)
$stream = $client.GetStream()
$writer = New-Object System.IO.StreamWriter($stream)
$reader = New-Object System.IO.StreamReader($stream)
$writer.AutoFlush = $true
$stream.ReadTimeout = 5000

# Skip banner
for ($i = 0; $i -lt 3; $i++) { $reader.ReadLine() | Out-Null }

# Issue reset
Write-Host "=== Sending reset halt ==="
$writer.WriteLine("reset halt")
Start-Sleep -Milliseconds 1000
$lines = @()
try {
    $sb = [text.stringbuilder]::new()
    for ($i = 0; $i -lt 10; $i++) {
        $line = $reader.ReadLine()
        if ($line -eq $null) { break }
        $sb.AppendLine($line) | Out-Null
        if ($line -match " halted|error|failed|>$") { break }
    }
    Write-Host $sb.ToString()
} catch { Write-Host "Timeout/error: $_" }

$writer.WriteLine("exit")
$client.Close()
