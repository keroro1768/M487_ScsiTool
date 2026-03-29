# Try to invoke Keil UV4 pack install via command line
$uv4 = "C:\Users\rinry\AppData\Local\Keil_v5\UV4\UV4.exe"
$pi = "C:\Users\rinry\AppData\Local\Keil_v5\UV4\PackInstaller.exe"

# Check UV4 command line options
$proc = Start-Process $uv4 -ArgumentList "-h" -Wait -PassThru -NoNewWindow -RedirectStandardOutput "$env:TEMP\uv4_out.txt" -RedirectStandardError "$env:TEMP\uv4_err.txt"
Start-Sleep 2
Get-Content "$env:TEMP\uv4_out.txt" -ErrorAction SilentlyContinue | Select-Object -First 30
Get-Content "$env:TEMP\uv4_err.txt" -ErrorAction SilentlyContinue | Select-Object -First 30

# Check PackInstaller options
$proc2 = Start-Process $pi -ArgumentList "--help" -Wait -PassThru -NoNewWindow -RedirectStandardOutput "$env:TEMP\pi_out.txt" -RedirectStandardError "$env:TEMP\pi_err.txt"
Start-Sleep 2
Get-Content "$env:TEMP\pi_out.txt" -ErrorAction SilentlyContinue | Select-Object -First 30
Get-Content "$env:TEMP\pi_err.txt" -ErrorAction SilentlyContinue | Select-Object -First 30
