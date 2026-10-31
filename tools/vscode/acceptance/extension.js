const vscode = require('vscode');
const fs = require('fs');
const path = require('path');
const childProcess = require('child_process');

const delay = ms => new Promise(resolve => setTimeout(resolve, ms));
async function until(operation, description, milliseconds = 90000) {
    const end = Date.now() + milliseconds;
    while (Date.now() < end) {
        const value = await operation();
        if (value) return value;
        await delay(500);
    }
    throw new Error(`Timed out: ${description}`);
}
function positionOf(document, token) {
    const offset = document.getText().indexOf(token);
    if (offset < 0) throw new Error(`Source token required: ${token}`);
    return document.positionAt(offset);
}

async function languageCheck(file, token, expectedName) {
    const document = await vscode.workspace.openTextDocument(file);
    await vscode.window.showTextDocument(document);
    const position = positionOf(document, token);
    const definitions = await until(async () => {
        const result = await vscode.commands.executeCommand('vscode.executeDefinitionProvider', document.uri, position);
        return result && result.length ? result : null;
    }, `definition for ${token}`);
    const locations = definitions.map(item => (item.uri || item.targetUri).fsPath);
    if (!locations.some(location => [expectedName].flat().some(name => location.endsWith(name)))) {
        throw new Error(`Definition target differs: ${JSON.stringify(locations)}`);
    }
    const completion = await until(async () => {
        const result = await vscode.commands.executeCommand('vscode.executeCompletionItemProvider', document.uri, position);
        return result && result.items.length ? result : null;
    }, `completion for ${token}`);
    const wanted = token.split('(')[0];
    const labels = completion.items.map(item => typeof item.label === 'string' ? item.label : item.label.label);
    if (!labels.some(label => label.includes(wanted))) throw new Error(`Expected completion missing: ${wanted}`);
    return {file, definitions: locations, completion_count: completion.items.length, matching_completions: labels.filter(label => label.includes(wanted))};
}

async function debugCheck(folder, configuration, source, lineToken, expressions) {
    const document = await vscode.workspace.openTextDocument(source);
    await vscode.window.showTextDocument(document);
    const position = positionOf(document, lineToken);
    const breakpoint = new vscode.SourceBreakpoint(new vscode.Location(document.uri, position));
    vscode.debug.addBreakpoints([breakpoint]);
    let stopped;
    let ended = false;
    let active;
    const tracker = vscode.debug.registerDebugAdapterTrackerFactory(configuration.type, {
        createDebugAdapterTracker(session) {
            if (session.name !== configuration.name) return;
            active = session;
            return {
                onDidSendMessage(message) {
                    if (message.type === 'event' && message.event === 'stopped') stopped = message.body;
                    if (message.type === 'event' && message.event === 'terminated') ended = true;
                }
            };
        }
    });
    try {
        if (!await vscode.debug.startDebugging(folder, configuration)) throw new Error('Debug launch rejected');
        await until(() => {
            if (ended) throw new Error('Debuggee terminated before the breakpoint');
            return stopped;
        }, `${configuration.name} breakpoint`);
        let stack = await active.customRequest('stackTrace', {threadId: stopped.threadId});
        let frame = stack.stackFrames.find(item => item.source && path.resolve(item.source.path) === path.resolve(source));
        if (!frame) throw new Error(`Expected source frame missing: ${JSON.stringify(stack)}`);
        if (frame.line !== position.line + 1) throw new Error(`Breakpoint line differs: ${frame.line}`);
        if (stopped.reason !== 'breakpoint') throw new Error(`Expected breakpoint stop: ${stopped.reason}`);
        const breakpointLine = frame.line;
        const threadId = stopped.threadId;
        stopped = undefined;
        await active.customRequest('next', {threadId});
        await until(() => stopped, `${configuration.name} source step`);
        stack = await active.customRequest('stackTrace', {threadId: stopped.threadId});
        frame = stack.stackFrames.find(item => item.source && path.resolve(item.source.path) === path.resolve(source));
        if (!frame || frame.line !== breakpointLine + 1) throw new Error('Source stepping did not reach the next statement');
        const scopes = await active.customRequest('scopes', {frameId: frame.id});
        const variables = [];
        for (const scope of scopes.scopes.filter(item => !item.expensive)) {
            const response = await active.customRequest('variables', {variablesReference: scope.variablesReference});
            variables.push(...response.variables);
        }
        const evaluations = {};
        for (const [expression, expected] of Object.entries(expressions)) {
            const response = await active.customRequest('evaluate', {expression, frameId: frame.id, context: 'watch'});
            if (!response.result.includes(expected)) throw new Error(`Unexpected ${expression}: ${response.result}`);
            evaluations[expression] = response.result;
        }
        return {reason: 'breakpoint', breakpoint_line: breakpointLine, step_reason: stopped.reason, frame: {name: frame.name, line: frame.line, path: frame.source.path},
            variables: variables.map(item => ({name: item.name, value: item.value})), evaluations};
    } finally {
        if (active) await vscode.debug.stopDebugging(active);
        vscode.debug.removeBreakpoints([breakpoint]);
        tracker.dispose();
    }
}

async function diagnosticCheck(root, language, content, expected) {
    const file = path.join(root, '.local', `acceptance-lsp-probe.${language === 'cpp' ? 'cpp' : 'py'}`);
    fs.writeFileSync(file, content);
    const document = await vscode.workspace.openTextDocument(file);
    await vscode.window.showTextDocument(document);
    const diagnostics = await until(() => {
        const result = vscode.languages.getDiagnostics(document.uri).filter(item => item.severity === vscode.DiagnosticSeverity.Error && item.message.includes(expected));
        return result.length ? result : null;
    }, `${language} diagnostics`);
    const result = diagnostics.map(item => ({source: item.source, message: item.message, line: item.range.start.line + 1}));
    await vscode.commands.executeCommand('workbench.action.closeActiveEditor');
    return result;
}

async function formattingCheck() {
    const document = await vscode.workspace.openTextDocument({language: 'python', content: 'value=1\n'});
    await vscode.window.showTextDocument(document);
    try {
    const edits = await until(async () => {
        const result = await vscode.commands.executeCommand('vscode.executeFormatDocumentProvider', document.uri, {insertSpaces: true, tabSize: 4});
        return result && result.length ? result : null;
    }, 'Python formatter');
    let result = document.getText();
    for (const edit of [...edits].sort((a, b) => document.offsetAt(b.range.start) - document.offsetAt(a.range.start))) {
        result = result.slice(0, document.offsetAt(edit.range.start)) + edit.newText + result.slice(document.offsetAt(edit.range.end));
    }
    if (result !== 'value = 1\n') throw new Error(`Unexpected formatter result: ${result}`);
    return {input: document.getText(), formatted: result};
    } finally {
        if (document.getText() === 'value=1\n') {
            await vscode.window.showTextDocument(document);
            await vscode.commands.executeCommand('workbench.action.revertAndCloseActiveEditor');
        }
    }
}

async function saveActionsCheck(root) {
    const file = path.join(root, 'tests/toolchain/python_debug_probe.py');
    const document = await vscode.workspace.openTextDocument(file);
    if (document.isDirty) throw new Error('The debugger probe must be saved before editor acceptance');
    const original = document.getText();
    const replace = async text => {
        const edit = new vscode.WorkspaceEdit();
        edit.replace(document.uri, new vscode.Range(document.positionAt(0), document.positionAt(document.getText().length)), text);
        if (!await vscode.workspace.applyEdit(edit) || !await document.save()) throw new Error('Editor save failed');
    };
    try {
        await vscode.window.showTextDocument(document);
        await replace('import sys\nimport pathlib\nimport os\n' + original + '\n_acceptance_probe = (os.name, sys.version)\n');
        const formatted = document.getText();
        if (!formatted.startsWith('import os\nimport sys\n') || formatted.includes('import pathlib')) throw new Error('Ruff save actions did not sort imports and remove the unused import');
        return {safe_fix: true, imports_sorted: true};
    } finally {
        await replace(original);
        if (fs.readFileSync(file, 'utf8') !== original) throw new Error('Editor probe source restoration failed');
    }
}

async function run() {
    const folder = vscode.workspace.workspaceFolders && vscode.workspace.workspaceFolders[0];
    const root = path.resolve(__dirname, '../../..');
    const reportPath = path.join(root, 'artifacts/environment/editor-acceptance.json');
    const report = {started_at: new Date().toISOString(), status: 'running', checks: {}};
    const save = () => {fs.mkdirSync(path.dirname(reportPath), {recursive: true}); fs.writeFileSync(reportPath, JSON.stringify(report, null, 2));};
    save();
    let python;
    try {
        if (!folder) {
            report.status = 'opening-workspace';
            report.workspace_trusted = vscode.workspace.isTrusted;
            save();
            report.workspace_update_requested = vscode.workspace.updateWorkspaceFolders(0, 0, {uri: vscode.Uri.file(root), name: 'HibikiPhysics'});
            save();
            return;
        }
        if (!vscode.workspace.isTrusted) throw new Error('Workspace trust is required for debugger acceptance');
        const environment = JSON.parse(fs.readFileSync(path.join(root, '.local/environment.json'), 'utf8'));
        const launch = JSON.parse(fs.readFileSync(path.join(root, '.vscode/launch.json'), 'utf8')).configurations;
        report.configuration = {
            folders: vscode.workspace.workspaceFolders.map(item => item.uri.fsPath),
            workspace_file: vscode.workspace.workspaceFile?.fsPath,
            clangd: vscode.workspace.getConfiguration('clangd').get('path'),
            arguments: vscode.workspace.getConfiguration('clangd').get('arguments'),
            executable_exists: fs.existsSync(environment.paths.clangd),
            interpreter: vscode.workspace.getConfiguration('python').get('defaultInterpreterPath')
        };
        save();
        for (const id of ['llvm-vs-code-extensions.vscode-clangd', 'detachhead.basedpyright', 'ms-vscode.cpptools', 'ms-python.debugpy']) {
            const extension = vscode.extensions.getExtension(id);
            if (!extension) throw new Error(`Extension unavailable: ${id}`);
            await extension.activate();
        }
        report.checks.cpp_language = await languageCheck(path.join(root, 'tests/toolchain/DebugProbe.cpp'), 'GetVersion', ['BuildInfo.h', 'BuildInfo.cpp']);
        save();
        report.checks.python_language = await languageCheck(path.join(root, 'tests/toolchain/python_debug_probe.py'), 'calculate_total(values)', 'python_debug_probe.py');
        save();
        report.checks.cpp_diagnostics = await diagnosticCheck(root, 'cpp', 'int main() { return missing_name; }\n', 'missing_name');
        report.checks.python_diagnostics = await diagnosticCheck(root, 'python', 'value: int = "invalid"\n', 'int');
        report.checks.python_formatting = await formattingCheck();
        report.checks.python_save_actions = await saveActionsCheck(root);
        save();
        report.checks.cpp_debug = await debugCheck(folder,
            {...launch.find(item => item.type === 'cppvsdbg'), name: 'Hibiki C++ acceptance'},
            path.join(root, 'tests/toolchain/DebugProbe.cpp'), 'const int', {sum: '10', version: '0.1.0'});
        save();
        const probe = path.join(root, 'tests/toolchain/python_debug_probe.py');
        const log = fs.openSync(path.join(root, 'artifacts/environment/python-debug-server.log'), 'w');
        python = childProcess.spawn(environment.paths.python, ['-m', 'debugpy', '--listen', '127.0.0.1:5678', '--wait-for-client', probe],
            {cwd: root, windowsHide: true, stdio: ['ignore', log, log]});
        fs.closeSync(log);
        await delay(2000);
        report.checks.python_debug = await debugCheck(folder,
            {...launch.find(item => item.type === 'debugpy'), name: 'Hibiki Python acceptance'},
            probe, 'identity =', {total: '10', identity: '10'});
        report.status = 'passed';
    } catch (error) {
        report.status = 'failed';
        report.error = String(error.stack || error);
    } finally {
        if (python && python.exitCode === null) python.kill();
        report.completed_at = new Date().toISOString();
        save();
    }
}
exports.activate = () => {run();};
exports.deactivate = () => {};
