# M487Filter Driver Signing Guide

> **Purpose:** Sign M487Filter.sys for installation on Windows 10/11
> **Driver Type:** KMDF HID Upper Filter Driver (Kernel-Mode)
> **Date:** 2026-03-28

---

## Overview

Windows requires all kernel-mode drivers to be digitally signed to load. There are three main paths:

| Method | Use Case | Cost | Requires Reboot |
|--------|----------|------|-----------------|
| **Test Signing** | Development/Debugging | Free | Yes (first time) |
| **EV Code Signing** | Production Deployment | $300-500/yr | No |
| **Microsoft Portal** | Long-term production | Varies | Varies |

---

## Method 1: Test Signing (Development Only)

Test signing allows self-signed certificates for local development.

### Step 1: Enable Test Signing Mode

```powershell
# Run as Administrator
bcdedit /set testsigning on
```

**Requires reboot.** You should see "Test Mode" watermark in desktop corner after reboot.

### Step 2: Verify Test Signing Enabled

```powershell
bcdedit /enum testsigning
```

Output should show `Yes` under "Test Signing".

### Step 3: Create Self-Signed Certificate

```powershell
# Create a test certificate
New-SelfSignedCertificate -Type Custom `
    -Subject "CN=M487Filter Test" `
    -KeyUsage DigitalSignature `
    -FriendlyName "M487 Test Certificate" `
    -CertStoreLocation "Cert:\CurrentUser\My" `
    -TextExtension @(
        "2.5.29.37={text}1.3.6.1.5.5.7.3.3",
        "2.5.29.19={text}"
    )
```

### Step 4: Export Certificate to PFX

```powershell
# Get the certificate thumbprint
$thumbprint = (Get-ChildItem Cert:\CurrentUser\My | Where-Object {
    $_.Subject -like "*M487Filter Test*"
}).Thumbprint

# Export to PFX
$password = ConvertTo-SecureString -String "M487TestOnly" -Force -AsPlainText
Export-PfxCertificate -Cert "Cert:\CurrentUser\My\$thumbprint" `
    -FilePath "C:\certs\M487Test.pfx" `
    -Password $password
```

### Step 5: Generate Catalog File (inf2cat)

Find inf2cat in your WDK/SDK installation:

```powershell
$inf2cat = "C:\Program Files (x86)\Windows Kits\10\bin\10.0.22621.0\x64\inf2cat.exe"

# Generate catalog for Windows 10 x64
& $inf2cat /v2 /driver:"D:\AiWorkSpace\M487_ScsiTool\tool\M487Filter\driver\" /os:10_x64
```

**Note:** The `/os:` parameter specifies target OS versions:
- `10_x64` = Windows 10 x64
- `10_x86` = Windows 10 x86  
- `Server6.3` = Windows Server 2012 R2
- `Server10` = Windows Server 2016+
- Multiple can be specified: `/os:10_x64,10_x86,Server10`

### Step 6: Sign the Catalog

```powershell
$signtool = "C:\Program Files (x86)\Windows Kits\10\bin\10.0.22621.0\x64\signtool.exe"

& $signtool sign /fd SHA256 /a /f "C:\certs\M487Test.pfx" /p "M487TestOnly" `
    "D:\AiWorkSpace\M487_ScsiTool\tool\M487Filter\driver\M487Filter.cat"
```

### Verify Signature

```powershell
# Check catalog signature
Get-AuthenticodeSignature "D:\AiWorkSpace\M487_ScsiTool\tool\M487Filter\driver\M487Filter.cat"

# Should show: Valid, HashAlgorithm: SHA256, SignerCertificate: M487Filter Test
```

---

## Method 2: EV Code Signing Certificate (Production)

EV (Extended Validation) certificates work without enabling Test Mode and pass Driver Signature Enforcement.

### Step 1: Obtain EV Certificate

**Certificate Authorities offering EV Code Signing:**

| Provider | Price (approx) | Token Required |
|----------|-----------------|-----------------|
| DigiCert | $424/year | Yes (USB) |
| GlobalSign | $399/year | Yes (USB) |
| SSL.com | $299/year | Yes (USB) |
| Sectigo (Comodo) | $239/year | Yes (USB) |

**Process:**
1. Purchase EV Code Signing certificate
2. Complete organization validation (CA calls/verifies)
3. Receive USB token with certificate installed
4. Install token driver on build machine

### Step 2: Install Token Driver

```powershell
# Install SafeNet token client (required for most USB tokens)
# Download from: https://knowledge.digicert.com/general-information/ev-code-signing-token.html

# After installation, verify token
certutil -scinfo
```

### Step 3: Sign the Driver

```powershell
# With EV certificate on USB token
$signtool = "C:\Program Files (x86)\Windows Kits\10\bin\10.0.22621.0\x64\signtool.exe"

# Find the certificate in the token
& $signtool sign /fd SHA256 /sha1 "<thumbprint>" `
    "D:\AiWorkSpace\M487_ScsiTool\tool\M487Filter\driver\M487Filter.cat"

# Or use the certificate store if token driver maps it there
& $signtool sign /fd SHA256 /n "Your Company Name" `
    "D:\AiWorkSpace\M487_ScsiTool\tool\M487Filter\driver\M487Filter.cat"
```

### Step 4: Timestamp the Signature (Required!)

```powershell
# Add RFC 3161 timestamp for long-term validity
& $signtool sign /td SHA256 /tr http://timestamp.digicert.com /sha1 "<thumbprint>" `
    "D:\AiWorkSpace\M487_ScsiTool\tool\M487Filter\driver\M487Filter.cat"
```

**Why timestamps matter:**
- Driver signature remains valid after certificate expires
- SHA1 timestamps work until 2030
- SHA256 timestamps work indefinitely

### Alternative: PFX with EV Certificate

If your CA allows PFX export (some don't for EV):

```powershell
# Sign using PFX file
& $signtool sign /fd SHA256 /f "C:\certs\ev-certificate.pfx" /p "<password>" `
    /td SHA256 /tr http://timestamp.digicert.com `
    "D:\AiWorkSpace\M487_ScsiTool\tool\M487Filter\driver\M487Filter.cat"
```

---

## Method 3: Microsoft Hardware Dev Center (Production + HLK)

For distribution via Windows Update or enterprise deployment, submit through Microsoft.

### Portal: https://partner.microsoft.com/dashboard/hardware

**Process:**
1. Join Windows Hardware Dev Center
2. Submit driver package (INF + SYS + CAT)
3. Run HLK tests (Hardware Lab Kit)
4. Sign with Microsoft attestation signature
5. Publish to Windows Update (optional)

**Benefits:**
- Works without any local certificate
- Automatic Windows Update distribution
- Enterprise managed deployment

---

## Troubleshooting Signing Issues

### Error: "The signing certificate is not valid for driver signing"

**Cause:** Using a code signing cert instead of EV, or certificate not trusted.

**Fix:** 
- Use EV certificate, or
- Add certificate to Trusted Root: `certutil -addstore Root cert.cer`

### Error: "The file is not signed"

**Cause:** inf2cat didn't run, or signtool failed silently.

**Fix:**
```powershell
# Verify catalog exists
dir M487Filter.cat

# Verify catalog is signed
signtool verify /pa /v M487Filter.cat
```

### Error: "Publisher information is not available"

**Cause:** INF doesn't have proper CatalogFile entry.

**Fix:** Ensure INF has `CatalogFile=M487Filter.cat` in [Version] section.

### Error 0x800B0109: "Certificate chain ends at untrusted root"

**Cause:** Self-signed cert not in Trusted Roots.

**Fix:**
```powershell
# Add to Trusted Publishers store
Import-Certificate -FilePath "M487Test.cer" -Cert Cert:\LocalMachine\TrustedPublisher

# Also add to Trusted Root (for test signing)
Import-Certificate -FilePath "M487Test.cer" -Cert Cert:\LocalMachine\Root
```

---

## Quick Reference: Signtool Commands

```powershell
# Find signtool
dir "C:\Program Files (x86)\Windows Kits\10\bin\*\x64\signtool.exe" /s

# Verify signature
signtool verify /pa /v M487Filter.cat

# Show certificate details
signtool verify /pa /dv /v M487Filter.cat

# Sign with SHA256
signtool sign /fd SHA256 /sha1 <thumbprint> /tr http://timestamp.digicert.com /td SHA256 M487Filter.cat

# Sign with SHA1 fallback
signtool sign /fd SHA1 /sha1 <thumbprint> /t http://timestamp.digicert.com M487Filter.cat
```

---

## Driver Files Summary

```
driver/
├── M487Filter.sys      # Driver binary (kernel-mode)
├── M487Filter.inf      # Installation manifest
├── M487Filter.cat      # Digitally signed catalog (THIS gets signed)
├── M487Filter.mof      # WMI Managed Object Format
└── M487Filter.rc       # Version resource
```

**Note:** The .sys file itself is typically not signed directly. Windows validates the .cat (catalog) file, which references the .sys file by hash.

---

## References

- [Driver Signing (Microsoft Docs)](https://docs.microsoft.com/en-us/windows-hardware/drivers/install/driver-signing)
- [Device Driver Signing (WDK Docs)](https://docs.microsoft.com/en-us/windows-hardware/drivers/devtest/)
- [SignTool (Microsoft Docs)](https://docs.microsoft.com/en-us/windows/win32/seccrypto/signtool)
- [Inf2Cat (Microsoft Docs)](https://docs.microsoft.com/en-us/windows-hardware/drivers/devtest/inf2cat)
- [EV Code Signing Requirements](https://docs.microsoft.com/en-us/windows-hardware/drivers/install/ev-code-signing-certificate-requirements)
