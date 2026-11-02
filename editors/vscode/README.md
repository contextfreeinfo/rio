# Rio for VS Code and Codium

Colors for `.rio` files, and through `rio lsp` (the language server in the `rio` program): errors as
you type (the first one, underlined), completion (Ctrl+Space; after a dot, by itself), and formatting
(Format Document: `rio fmt`'s layout).

A platform's package (from a release: `rio-vscode-<version>-<platform>.vsix`) has `rio` in it. Otherwise
it uses `rio` on the PATH, or set `rio.path` to it.

Building the extension: `npm ci`, then `npx @vscode/vsce package` (with `rio` in `bin/` and `--target`,
such as `win32-x64`, for a platform's package: CI's release job does that), which makes `rio-0.1.0.vsix`;
install it with `code --install-extension rio-0.1.0.vsix` (or `codium`).
