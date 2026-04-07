$ErrorActionPreference = 'Continue'
$client = New-Object System.Net.Sockets.TcpClient
try {
    $client.Connect('localhost', 4444)
    $stream = $client.GetStream()
    $writer = New-Object System.IO.StreamWriter($stream)
    $reader = New-Object System.IO.StreamReader($stream)
    $writer.AutoFlush = $true

    $commands = @(
        'halt',
        'reset halt',
        'echo Flashing M487 firmware...',
        'flash write_image erase D:/AiWorkSpace/M487_ScsiTool/firmware/composite/build_gcc/firmware.bin 0x0',
        'echo Flash complete, resetting...',
        'reset run',
        'shutdown'
    )

    foreach ($cmd in $commands) {
        Write-Host ">> $cmd"
        $writer.WriteLine($cmd)
        Start-Sleep -Milliseconds 1000
        while ($reader.Peek() -ge 0) {
            $line = $reader.ReadLine()
            if ($line) { Write-Host "   $line" }
        }
    }

    $writer.Close()
    $reader.Close()
} catch {
    Write-Host "Error: $_"
} finally {
    if ($client) { $client.Close() }
}
Write-Host "Done."