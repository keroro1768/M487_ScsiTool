$env:Path = "C:\msys64\mingw64\bin;C:\Users\rinry\Tool\openocd-build\bin;" + $env:Path
Write-Host "PATH set"
$process = Start-Process -FilePath 'C:\Users\rinry\Tool\openocd-build\bin\openocd.exe' -ArgumentList '--version' -NoNewWindow -Wait -PassThru -RedirectStandardOutput 'C:\Users\rinry\Tool\stdout.txt' -RedirectStandardError 'C:\Users\rinry\Tool\stderr.txt'
$exitCode = $process.ExitCode
Write-Host "Exit code: $exitCode"
Get-Content 'C:\Users\rinry\Tool\stdout.txt','C:\Users\rinry\Tool\stderr.txt' -ErrorAction SilentlyContinue
