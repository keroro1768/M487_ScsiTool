import subprocess
result = subprocess.run(['powershell', '-Command', 'Get-PnpDevice -Class USB -Status OK'], capture_output=True, text=True, timeout=10)
print('STDOUT:', result.stdout[:2000])
print('STDERR:', result.stderr[:500])
print('Return code:', result.returncode)
