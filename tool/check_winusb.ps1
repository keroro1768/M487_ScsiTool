# Check if WinUSB is loaded for the Nu-Link USB interfaces
$devs = Get-PnpDevice | Where-Object { $_.DeviceID -like '*0416*511C*' }
foreach ($d in $devs) {
    Write-Host "Instance: $($d.InstanceId)"
    Write-Host "  Status: $($d.Status)"

    # Try to get the device interface info
    $devProps = Get-WmiObject -Namespace root\WMI -Class MSSerial_PortName 2>$null
}

# More direct: check the oem inf files to see what driver they install
Write-Host ""
Write-Host "=== Checking oem133.inf and oem134.inf ==="
$oemFiles = @(
    "$env:SystemRoot\inf\oem133.inf",
    "$env:SystemRoot\inf\oem134.inf"
)
foreach ($f in $oemFiles) {
    if (Test-Path $f) {
        Write-Host "Found: $f"
        $content = Get-Content $f | Select-Object -First 30
        foreach ($line in $content) { Write-Host "  $line" }
    } else {
        Write-Host "NOT found: $f"
    }
}

# List all oem*.inf files in inf directory
Write-Host ""
Write-Host "=== All oem*.inf files ==="
Get-ChildItem "$env:SystemRoot\inf\oem*.inf" | Where-Object { $_.LastWriteTime -gt (Get-Date).AddDays(-7) } | ForEach-Object {
    Write-Host "$($_.Name) - $($_.LastWriteTime)"
}
