$env:Path = "C:\msys64\mingw64\bin;C:\Users\rinry\Tool\openocd-build\bin;" + $env:Path
Write-Host "Testing OpenOCD connection to M487..."
$process = Start-Process -FilePath 'C:\Users\rinry\Tool\openocd-build\bin\openocd.exe' -ArgumentList '-f', 'C:\Users\rinry\Tool\openocd\m487.cfg', '-d3' -NoNewWindow -Wait -PassThru -RedirectStandardOutput 'C:\Users\rinry\Tool\stdout.txt' -RedirectStandardError 'C:\Users\rinry\Tool\stderr.txt'
$exitCode = $process.ExitCode
Write-Host "Exit code: $exitCode"
Write-Host "=== STDOUT ==="
Get-Content 'C:\Users\rinry\Tool\stdout.txt' -ErrorAction SilentlyContinue
Write-Host "=== STDERR ==="
Get-Content 'C:\Users\rinry\Tool\stderr.txt' -ErrorAction SilentlyContinue
