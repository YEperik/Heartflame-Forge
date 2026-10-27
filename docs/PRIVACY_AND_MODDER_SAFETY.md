# Privacy and modder safety

This guide is intentionally practical: it helps keep personal information and private work out of public project history while preserving enough technical detail to diagnose bugs.

## What the application does today

- The current source does not implement account login, telemetry, cloud sync, or automatic update checks.
- Supported asset operations read local files and write explicit output files locally.
- The Windows CI workflow downloads Qt, vcpkg dependencies, the Microsoft Visual C++ runtime, and build tooling to the build runner. Those downloads are build dependencies, not asset uploads.
- A native extension DLL is not sandboxed. It can run with the permissions of the user who launched Heartflame Forge.

These statements describe this source line; review changes to networking, update systems, crash reports, and plugin loading before changing the privacy description.

## Keep personal work private

Create a local `private/` folder for notes, test checklists, and private asset mappings. The repository `.gitignore` excludes the entire directory, and release packaging does not collect it. Before every commit, check `git status --short` and `git diff --cached --stat`.

Never place any of the following in public README files, issues, pull requests, commits, sample fixtures, or screenshots:

- Passwords, API tokens, private keys, account recovery information, cookies, or signed access URLs.
- Personal names, email addresses, phone numbers, home directories, Windows usernames, drive labels, machine identifiers, or private repository URLs.
- Full game data archives, saves, licensed assets, or extracted material you do not have permission to redistribute.
- Unredacted logs that include absolute paths, environment variables, command lines with secrets, or unrelated files.

## Safer asset workflow

1. Make a working copy of the archive or asset before inspecting or editing.
2. Record a SHA-256 for the source if reproducibility matters.
3. Save edits to a new destination; do not replace the original archive.
4. Verify the output archive and re-open the edited file with the tool.
5. Treat `PARTIAL`, `unsupported`, or `unknown` as a stop condition for editing that profile. Do not force the parser to interpret an undocumented layout.
6. Test modified assets on a separate copy and keep an untouched backup.

## Safe bug reports

Use a synthetic file or a minimal fragment you are allowed to share. Include the Heartflame Forge version, compiler/build type, exact error text, and steps to reproduce. Replace `C:\Users\YourName\...` with a neutral placeholder such as `C:\Work\sample.bin`. Do not attach original game files simply to make a report easier to understand.

## Extension precautions

Only load a plugin whose source and release origin you trust. Check the plugin's hashes and dependencies, and review the native code before running it. Removing a plugin DLL from `plugins/` prevents it from being discovered on the next start, but does not reverse actions it may have already taken.

## Blank personal checklist

Use [`MODDER_PRIVATE_CHECKLIST_TEMPLATE.md`](MODDER_PRIVATE_CHECKLIST_TEMPLATE.md) as a blank template. Copy it into a local `private/` folder and fill in only the local copy. The public template must remain blank.
