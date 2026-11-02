// Rio for VS Code (and Codium): colors from the grammar, and the rest from rio lsp, the language server
// in the rio program: errors as you type, completion, formatting. Which rio: the setting rio.path, else
// the one bundled with the extension (bin/: a platform's package has one), else rio on the PATH. A rio
// from a file runs as a copy, in the extension's storage, so the server never locks the one being
// rebuilt (a newer one is copied again as the server starts).
const fs = require('fs');
const path = require('path');
const vscode = require('vscode');
const { LanguageClient } = require('vscode-languageclient/node');

let client;

function copyToRun(context, from) {
  try {
    const dir = context.globalStorageUri.fsPath;
    const to = path.join(dir, 'rio-lsp' + (process.platform === 'win32' ? '.exe' : ''));
    const a = fs.statSync(from);
    let b;
    try {
      b = fs.statSync(to);
    } catch (e) {}
    if (!b || b.size !== a.size || b.mtimeMs < a.mtimeMs) {
      fs.mkdirSync(dir, { recursive: true });
      fs.copyFileSync(from, to);
      fs.chmodSync(to, 0o755); // (a package's files may come without their executable bit)
    }
    return to;
  } catch (e) {
    return from; // (can't copy it, its copy's in use: run it as it is)
  }
}

exports.activate = function (context) {
  const bundled = path.join(context.extensionPath, 'bin', process.platform === 'win32' ? 'rio.exe' : 'rio');
  let command = vscode.workspace.getConfiguration('rio').get('path');
  if (!command && fs.existsSync(bundled)) command = bundled;
  if (command && fs.existsSync(command)) command = copyToRun(context, command);
  command = command || 'rio';
  client = new LanguageClient('rio', 'Rio', { command, args: ['lsp'] }, { documentSelector: [{ scheme: 'file', language: 'rio' }] });
  client.start().catch((e) => vscode.window.showWarningMessage(`Rio: couldn't start "${command} lsp" (the setting rio.path says where rio is): ${e.message}`));
  context.subscriptions.push({ dispose: () => client && client.stop() });
};

exports.deactivate = function () {
  return client ? client.stop() : undefined;
};
