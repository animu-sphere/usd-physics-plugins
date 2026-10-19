# Security Policy

Please do not report suspected vulnerabilities in a public issue or pull
request. Unsafe USD input handling, resource lifetime errors, memory corruption,
path handling, package integrity and unsafe build or CI behavior are in scope.

## Report privately

Use GitHub's private [Report a vulnerability](https://github.com/animu-sphere/usd-physics-plugins/security/advisories/new)
form. Include:

- affected component, commit or release;
- observed impact and minimal reproduction or generated fixture;
- OS, compiler, build options and relevant OpenUSD/Jolt versions;
- logs needed to reproduce, with private data removed.

Share assets only with permission. If private vulnerability reporting is
unavailable, open a minimal issue requesting a private contact without
technical details, exploit code or private data.

Maintainers will acknowledge reports when able, investigate and coordinate
a fix and disclosure as appropriate. Credit is given on request when safe.
Reports affecting an external SDK may also need coordination with its maintainers.

## Maintenance scope

Security fixes target the current development line and the latest published
release, when one exists. Older releases have no promised backport coverage.
Include your exact version so maintainers can determine the affected scope.

## Community concerns

For sensitive conduct concerns, request a private maintainer contact without
publishing personal information or incident details. The fallback issue should
only request contact; send the report through the private channel provided.
