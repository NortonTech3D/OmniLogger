# Security Policy

## Supported Versions

Currently supported versions of OmniLogger:

| Version | Supported          |
| ------- | ------------------ |
| 1.0.x   | :white_check_mark: |

## Security Features

OmniLogger includes several built-in security features:

### Input Validation
- All user inputs are validated before processing
- Range checks on numeric inputs (intervals, pins, etc.)
- Length validation on string inputs
- JSON structure validation before parsing

### Path Security
- Path traversal protection in file operations
- File path sanitization (prevents `../` attacks)
- Filename validation for downloads

### Data Protection
- Bounds checking on all array accesses
- Buffer overflow prevention through safe string operations
- Null termination on all string buffers
- ADC reading validation

### Password Security
- WPA2 password requirements (8+ characters minimum)
- AP password validation
- Note: WiFi credentials stored in plaintext in ESP32 NVS (hardware limitation)

## Known Limitations

### No Encryption
- Web interface uses HTTP, not HTTPS
- Data transmitted in plaintext over local network
- WiFi credentials stored unencrypted in flash memory

### No Authentication
- Web interface does not require login
- Anyone on the same network can access the interface
- Physical access to device allows credential extraction

### Network Exposure
- Device should only be used on trusted networks
- Not suitable for direct internet exposure
- No built-in firewall or access control

## Deployment Recommendations

1. **Use Trusted Networks Only**
   - Deploy on private/home networks
   - Use network segmentation for IoT devices
   - Consider VLAN isolation

2. **Change Default Credentials**
   - Change default AP password immediately
   - Use strong WiFi passwords
   - Use unique SSID names

3. **Physical Security**
   - Protect physical access to the device
   - Secure SD card containing data
   - Consider enclosure with tamper detection

4. **Network Security**
   - Use WPA2/WPA3 encryption on WiFi
   - Implement MAC filtering if needed
   - Monitor network traffic for anomalies

5. **Data Privacy**
   - Encrypt SD card data if needed (external solution)
   - Regularly backup and clear old data
   - Sanitize SD cards before disposal

6. **Regular Updates**
   - Check for firmware updates regularly
   - Monitor security advisories
   - Update promptly when patches available

## Reporting a Vulnerability

If you discover a security vulnerability in OmniLogger, please report it by:

1. **DO NOT** open a public issue
2. Send details to the repository maintainers privately
3. Include:
   - Description of the vulnerability
   - Steps to reproduce
   - Potential impact
   - Suggested fix (if available)

We will acknowledge receipt within 48 hours and provide a detailed response within 7 days, including:
- Confirmation of the issue
- Timeline for fix
- Credit acknowledgment (if desired)

## Security Update Process

When a security vulnerability is confirmed:

1. **Assessment**: Evaluate severity and impact
2. **Fix Development**: Develop and test patch
3. **Release**: Create security release
4. **Notification**: Update users via:
   - GitHub Security Advisory
   - Release notes
   - README update
5. **Documentation**: Update this security policy

## Best Practices for Users

### Before Deployment
- [ ] Change default AP password
- [ ] Use strong WiFi credentials
- [ ] Update to latest firmware
- [ ] Review security settings
- [ ] Test on isolated network first

### During Operation
- [ ] Monitor for unusual activity
- [ ] Keep firmware updated
- [ ] Regularly backup data
- [ ] Check SD card health
- [ ] Review access logs if available

### After Decommissioning
- [ ] Erase configuration (use reset function)
- [ ] Format SD card securely
- [ ] Remove from network
- [ ] Update network credentials if needed

## Responsible Disclosure

We follow responsible disclosure practices:
- 90-day disclosure timeline for reported vulnerabilities
- Coordinated disclosure with security researchers
- Credit given to reporters (unless anonymous)
- Public advisories for confirmed vulnerabilities

## Additional Resources

- [OWASP IoT Security](https://owasp.org/www-project-internet-of-things/)
- [ESP32 Security Features](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s2/security/index.html)
- [WiFi Security Best Practices](https://www.wi-fi.org/discover-wi-fi/security)

## Contact

For security concerns: Open a private security advisory on GitHub
For general questions: Open a regular issue on GitHub

---

Last Updated: 2026-01-04
