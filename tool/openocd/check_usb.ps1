Get-PnpDevice -InstanceId 'USB*' | ForEach-Object {
    if ($_.FriendlyName -like '*M487*' -or $_.FriendlyName -like '*Nuvoton*') {
        $_ | Format-List InstanceId, Status, FriendlyName
    }
}