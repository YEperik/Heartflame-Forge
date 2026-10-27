# Security policy

Heartflame Forge processes user-supplied binary files and can load native extension libraries. Please treat parser bugs and plugin-loading issues as security-relevant when they could cause crashes, memory corruption, path traversal, unsafe overwrites, or unexpected code execution.

## Supported release

The current development line is `1.3.x`. Only releases published by a repository owner should be treated as official. This source snapshot does not contain a project code-signing certificate; Windows artifacts are expected to carry SHA-256 checksums but are not publisher-signed.

## Reporting a security issue

- Do **not** publish exploit details, private game assets, saves, credentials, personal paths, or unredacted crash dumps in a public issue.
- If GitHub private vulnerability reporting is enabled for the hosting repository, use the repository's **Security → Report a vulnerability** flow.
- If private reporting is not enabled, contact a repository maintainer through a private channel before publishing technical details. No maintainer email or private reporting URL is embedded in this source package; repository owners should configure one before announcing a public release.
- For a non-sensitive bug, open a public issue with the smallest synthetic or redistributable fixture that reproduces it.

## Extension trust model

Qt plugins are native executable code. They are not sandboxed. Do not load an extension from an unknown source just because its file has the expected name or appears in the `plugins/` directory. Review source and provenance before loading it.

## Safe reproduction

Use synthetic fixtures where possible. If a game asset is required, share only a minimal excerpt you have permission to redistribute; redact local usernames, absolute file paths, machine details, and unrelated content. Keep the original file and checksum so a report can be reproduced without modifying the source data.
