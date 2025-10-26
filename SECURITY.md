# Security Policy

## Supported Versions

We take the security of HyperRecall seriously. The following versions are currently being supported with security updates:

| Version | Supported          |
| ------- | ------------------ |
| 0.9.x   | :white_check_mark: |
| < 0.9   | :x:                |

## Reporting a Vulnerability

If you discover a security vulnerability in HyperRecall, please follow these steps:

### 1. Do Not Publicly Disclose

Please do not create a public GitHub issue for security vulnerabilities. Public disclosure before a fix is available can put all users at risk.

### 2. Report Privately

Report security vulnerabilities by:

- Creating a private security advisory on GitHub (preferred)
- Emailing the project maintainers (see contact information in the repository)

### 3. Provide Details

When reporting a vulnerability, please include:

- Description of the vulnerability
- Steps to reproduce the issue
- Potential impact
- Any suggested fixes (if applicable)
- Your contact information for follow-up

### 4. Response Timeline

- **Initial Response**: We aim to acknowledge receipt within 48 hours
- **Assessment**: We will assess the severity and impact within 5 business days
- **Fix Development**: Critical vulnerabilities will be prioritized for immediate fixing
- **Disclosure**: Once a fix is available, we will coordinate disclosure timing with you

## Security Best Practices

### For Users

1. **Keep Updated**: Always use the latest version of HyperRecall
2. **Database Security**: Protect your `~/.local/share/HyperRecall/` directory (Linux) or `%APPDATA%\HyperRecall\` (Windows)
3. **Backup Regularly**: Use the export functionality to backup your data
4. **File Permissions**: Ensure your data directory has appropriate permissions

### For Developers

1. **Input Validation**: Always validate user input
2. **SQL Injection**: Use prepared statements (already implemented)
3. **Memory Safety**: Check buffer bounds and avoid overflows
4. **Error Handling**: Never expose sensitive information in error messages
5. **Dependencies**: Keep raylib and SQLite3 updated to their latest stable versions

## Security Features

HyperRecall implements the following security measures:

### Database Security

- **Prepared Statements**: All SQL queries use prepared statements to prevent SQL injection
- **WAL Mode**: Write-Ahead Logging for database integrity
- **Foreign Keys**: Enabled for referential integrity
- **ACID Compliance**: Transactions ensure data consistency

### Memory Safety

- **C17 Standard**: Modern C with safety features
- **Bounds Checking**: Buffer overflow protection
- **Error Checking**: All allocations and operations are checked
- **No Unsafe Functions**: Avoiding deprecated unsafe functions

### Platform Security

- **Standard Paths**: Uses platform-specific secure directories for data storage
- **File Permissions**: Respects operating system file permissions
- **No Network**: No network access by default (offline-first design)

## Known Limitations

### Current Version (0.9.3)

- **Local Storage Only**: All data is stored locally; no cloud sync yet
- **No Encryption**: Database is not encrypted (planned for future versions)
- **No Authentication**: No user authentication (single-user application)

### Planned Security Enhancements (v1.1+)

- Database encryption at rest
- Optional password protection
- Secure cloud sync with end-to-end encryption
- Two-factor authentication for cloud features

## Vulnerability Disclosure Policy

### Coordinated Disclosure

We follow coordinated vulnerability disclosure:

1. **Confidential Report**: Security researchers report vulnerabilities privately
2. **Verification**: We verify and assess the vulnerability
3. **Fix Development**: We develop and test a fix
4. **Coordinated Release**: We coordinate disclosure with the reporter
5. **Public Disclosure**: After fix is released, we publicly disclose the vulnerability

### Credit

We will publicly thank security researchers who responsibly disclose vulnerabilities (unless they prefer to remain anonymous).

## Security Updates

Security updates will be:

- Released as quickly as possible after verification
- Clearly marked in release notes
- Announced through GitHub releases
- Included in CHANGELOG.md with `[SECURITY]` tag

## Scope

### In Scope

- SQL injection vulnerabilities
- Buffer overflow vulnerabilities
- Authentication bypasses (when authentication is implemented)
- Data corruption vulnerabilities
- Path traversal vulnerabilities
- Memory safety issues
- Privilege escalation

### Out of Scope

- Social engineering
- Physical access attacks
- Denial of service from local user actions
- Issues in third-party dependencies (report to their maintainers)
- Issues requiring physical access to the machine

## Contact

For security concerns, please use:

- GitHub Security Advisories (preferred)
- Project issue tracker with `[SECURITY]` tag (for non-critical issues)
- Direct contact with project maintainers (see repository)

## Acknowledgments

We would like to thank the security researchers and users who help keep HyperRecall secure by responsibly disclosing vulnerabilities.

---

**Note**: This security policy will be updated as the project evolves. Please check back regularly for updates.

Last Updated: October 26, 2025
