# OpenOCD telnet - try program command
$client = New-Object System.Net.Sockets.TcpClient
$client.Connect("localhost", 4444)
$stream = $client.GetStream()
$writer = New-Object System.IO.StreamWriter($stream)
$reader = New-Object System.IO.StreamReader($stream)
$writer.AutoFlush = $true
$stream.ReadTimeout = 8000

# Skip banner
for ($i = 0; $i -lt 3; $i++) { $reader.ReadLine() | Out-Null }

# Try program command to write flash
Write-Host "=== Trying program command ==="
$writer.WriteLine("program D:/AiWorkSpace/M487_ScsiTool/firmware/composite/build_gcc/firmware.bin 0x10000000 bin")
$resp = @()
try {
    $sb = [text.stringbuilder]::new()
    for ($i = 0; $i -lt 20; $i++) {
        $line = $reader.ReadLine()
        if ($line -eq $null) { break }
        $sb.AppendLine($line) | Out-Null
        if ($line -match ">" -or $line -match "error" -or $line -match "failed") { break }
    }
    Write-Host $sb.ToString()
} catch { Write-Host "Error: $_" }

$writer.WriteLine("exit")
$client.Close()
