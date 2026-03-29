# List USB devices using WMI
$usbDevices = Get-WmiObject -Class Win32_USBControllerDevice | ForEach-Object { 
    $device = [wmi]$_.Dependent 
    $device | Select-Object DeviceID, Caption, Manufacturer
} | Where-Object { $_.DeviceID -ne $null } | Select-Object -First 20

foreach ($dev in $usbDevices) {
    Write-Host "DeviceID: $($dev.DeviceID)"
    Write-Host "Caption: $($dev.Caption)"
    Write-Host "Manufacturer: $($dev.Manufacturer)"
    Write-Host "---"
}
