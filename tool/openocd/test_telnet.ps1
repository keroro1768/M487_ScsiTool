$client = New-Object System.Net.Sockets.TcpClient
$client.ReceiveTimeout = 3000
$client.Connect("127.0.0.1", 4444)
$stream = $client.GetStream()
$sw = New-Object System.IO.StreamWriter($stream)
$sw.AutoFlush = $true
$sw.WriteLine("mdw 0x10000000 32")
Start-Sleep -Milliseconds 500
$bytes = @()
$tmp = New-Object byte[] 4096
while ($true) {
    $n = $stream.Read($tmp, 0, $tmp.Length)
    if ($n -le 0) { break }
    $bytes += $tmp[0..($n-1)]
}
$client.Dispose()
[System.Text.Encoding]::ASCII.GetString($bytes)
