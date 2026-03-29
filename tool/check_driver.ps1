$devices = Get-PnpDevice | Where-Object { $_.DeviceID -like '*0416*511C*' }
foreach ($dev in $devices) {
    Write-Host "=== Device: $($dev.FriendlyName) ==="
    Write-Host "InstanceId: $($dev.InstanceId)"
    Write-Host "Status: $($dev.Status)"
    Write-Host ""

    # Get driver details
    $devInst = $dev.InstanceId
    $driver = Get-WmiObject -Class Win32_PnPSignedDriver | Where-Object { $_.DeviceID -eq $devInst } | Select-Object -First 1
    if ($driver) {
        Write-Host "Driver: $($driver.DeviceName)"
        Write-Host "DriverVersion: $($driver.DriverVersion)"
        Write-Host "InfName: $($driver.InfName)"
        Write-Host "Manufacturer: $($driver.Manufacturer)"
    }
    Write-Host ""
}
