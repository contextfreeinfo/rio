// Rio for VS Code (and Codium): colors from the grammar, and the rest from rio lsp, the language server
// in the rio program: errors as you type, completion, formatting. Which rio: the setting rio.path, else
// the one bundled with the extension (bin/: a platform's package has one), else rio on the PATH.
const fs = require('fs');
const path = require('path');
const vscode = require('vscode');
const { LanguageClient } = require('vscode-languageclient/node');

let client;

exports.activate = function (context) {
  const bundled = path.join(context.extensionPath, 'bin', process.platform === 'win32' ? 'rio.exe' : 'rio');
  let command = vscode.workspace.getConfiguration('rio').get('path');
  if (!command && fs.existsSync(bundled)) {
    command = bundled;
    try {
      fs.chmodSync(bundled, 0o755); // (a package's files may come without their executable bit)
    } catch (e) {}
  }
  command = command || 'rio';
  client = new LanguageClient('rio', 'Rio', { command, args: ['lsp'] }, { documentSelector: [{ scheme: 'file', language: 'rio' }] });
  client.start().catch((e) => vscode.window.showWarningMessage(`Rio: couldn't start "${command} lsp" (the setting rio.path says where rio is): ${e.message}`));
  context.subscriptions.push({ dispose: () => client && client.stop() });
};

exports.deactivate = function () {
  return client ? client.stop() : undefined;
};
