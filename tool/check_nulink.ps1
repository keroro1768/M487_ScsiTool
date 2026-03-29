Get-PnpDevice | Where-Object { $_.DeviceID -like '*0416*511C*' } | Format-Table InstanceId, FriendlyName, Status
