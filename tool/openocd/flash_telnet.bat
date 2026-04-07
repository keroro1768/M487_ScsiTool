# M487 Flash via OpenOCD telnet
# Start OpenOCD in background, use telnet to send commands

# First, kill any existing openocd processes on these ports
taskkill /F /IM openocd.exe 2>$null

# Start OpenOCD in background
start /b cmd /c "D:\AiWorkSpace\M487_ScsiTool\tool\openocd\openocd.bat -s D:\AiWorkSpace\M487_ScsiTool\tool\OpenOCD\scripts -f D:\AiWorkSpace\M487_ScsiTool\tool\openocd\nulink_m487_ice.cfg"

# Wait for server to start
timeout /t 3 /nobreak >nul

# Use PowerShell to send commands via TCP
powershell -Command "
$client = New-Object System.Net.Sockets.TcpClient
$client.Connect('localhost', 4444)
$stream = $client.GetStream()
$writer = New-Object System.IO.StreamWriter($stream)
$reader = New-Object System.IO.StreamReader($stream)

# Send flash erase and program commands
$commands = @(
    'init',
    'halt',
    'reset halt',
    'flash write_image erase D:/AiWorkSpace/M487_ScsiTool/firmware/composite/build_gcc/firmware.bin 0x0',
    'reset run',
    'shutdown'
)

foreach ($cmd in $commands) {
    Write-Host \"Sending: $cmd\"
    $writer.WriteLine($cmd)
    $writer.Flush()
    Start-Sleep -Milliseconds 500
    if ($reader.Peek() -ge 0) {
        Write-Host $reader.ReadLine()
    }
}

$writer.Close()
$reader.Close()
$client.Close()
"

echo Done