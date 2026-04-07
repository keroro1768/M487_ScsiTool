# Check for MSC drive letters
Write-Host "Checking MSC drive letters..."
Get-WmiObject -Class Win32_LogicalDisk | Where-Object { $_.DriveType -eq 3 } | ForEach-Object {
    Write-Host "Drive: $($_.DeviceID) - $($_.VolumeName)"
}

# Check for our specific VID/PID
Write-Host "`nChecking USB devices with VID 0x04F3..."
Get-PnpDevice -InstanceId 'USB*' | ForEach-Object {
    if ($_.InstanceId -match 'VID_04F3') {
        $_ | Format-List InstanceId, Status, FriendlyName
    }
}