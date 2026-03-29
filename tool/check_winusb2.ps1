$dev = Get-PnpDevice | Where-Object { $_.DeviceID -like '*0416*511c*' -and $_.DeviceID -like '*MI_00*' -and $_.DeviceID -notlike '*HID*' }
$dev | ForEach-Object {
    Write-Host "Device: $($_.FriendlyName)"
    Write-Host "Instance: $($_.InstanceId)"
    $props = Get-WmiObject -Namespace root\CIMV2 -Class Win32_DeviceGuard | Out-Null
}

# Try to get USB device properties
$devId = "USB\VID_0416&PID_511C&MI_00\8&D8FCDFA&0&0000"
$inf = (Get-WmiObject -Class Win32_PnPSignedDriver | Where-Object { $_.DeviceID -eq $devId })
Write-Host "Driver inf: $($inf.InfName)"
Write-Host "Driver version: $($inf.DriverVersion)"
Write-Host "Manufacturer: $($inf.Manufacturer)"

# Check if WinUSB is loaded
$winusb = Get-WindowsDriver -Online -DriverCinclude 'winusb' 2>$null
if ($winusb) { Write-Host "WinUSB drivers found" }
