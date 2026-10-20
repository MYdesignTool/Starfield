/* Dependency-free publication checks. Run: node tests/website-check.cjs */
'use strict';
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const root = path.resolve(__dirname, '..');
const site = path.join(root, 'docs', 'site');
const allowed = new Set(['.html', '.css', '.js', '.svg', '.jpg', '.txt']);
const files = [];
function visit(directory) {
  for (const entry of fs.readdirSync(directory, { withFileTypes: true })) {
    const file = path.join(directory, entry.name);
    assert(!entry.isSymbolicLink(), `No symlinks in publication: ${file}`);
    if (entry.isDirectory()) visit(file);
    else {
      assert(allowed.has(path.extname(file)) || entry.name === '.nojekyll', `Unexpected published file: ${file}`);
      assert(fs.statSync(file).size < 500_000, `Review large public asset: ${file}`);
      files.push(file);
    }
  }
}
visit(site);
assert(files.includes(path.join(site, 'index.html')), 'Home page is required');
assert(files.includes(path.join(site, '.nojekyll')), 'Disable Jekyll processing');
let links = 0;
function verifyReference(from, reference) {
  if (!reference || reference === '#' || reference.startsWith('data:')) return;
  if (from.endsWith('.css') && reference.startsWith('#')) {
    const owners = files.filter(file => file.endsWith('.html') &&
      [...fs.readFileSync(file, 'utf8').matchAll(/<link\b[^>]*href="([^"]+)"/g)]
        .some(match => path.resolve(path.dirname(file), match[1].split('?')[0]) === from));
    assert(owners.length, `CSS fragment has no document owner: ${reference}`);
    for (const owner of owners) verifyReference(owner, reference);
    return;
  }
  if (/^https?:/.test(reference)) {
    assert(reference.startsWith('https://'), `Use HTTPS: ${reference}`);
    return;
  }
  assert(!/^(?:\/|[a-z]+:)/i.test(reference), `Use project-relative resources: ${reference}`);
  const [filePart, fragment] = reference.split('#');
  const target = filePart ? path.resolve(path.dirname(from), decodeURIComponent(filePart.split('?')[0])) : from;
  assert(target.startsWith(site + path.sep), `Resource outside publication: ${reference}`);
  assert(fs.existsSync(target), `Missing resource: ${reference} from ${path.basename(from)}`);
  if (fragment) {
    const markup = fs.readFileSync(target, 'utf8');
    const ids = [...markup.matchAll(/\bid="([^"]+)"/g)].map(match => match[1]);
    assert(ids.includes(decodeURIComponent(fragment)), `Missing fragment: ${reference}`);
  }
  links++;
}
for (const file of files) {
  const content = fs.readFileSync(file, 'utf8');
  assert(!/[A-Z]:\\(?:Users|Project|Software)\\/.test(content), `Local path leaked: ${file}`);
  if (file.endsWith('.html')) {
    assert(/<html\s+lang=/.test(content), 'Set page language');
    assert(/name="viewport"/.test(content), 'Set mobile viewport');
    assert.equal((content.match(/<h1\b/g) || []).length, 1, 'Exactly one page title');
    const ids = [...content.matchAll(/\bid="([^"]+)"/g)].map(match => match[1]);
    assert.equal(ids.length, new Set(ids).size, 'Unique DOM IDs');
    for (const match of content.matchAll(/\b(?:href|src)="([^"]+)"/g)) verifyReference(file, match[1]);
    for (const match of content.matchAll(/\baria-(?:controls|labelledby)="([^"]+)"/g)) {
      for (const id of match[1].split(' ')) assert(ids.includes(id), `Missing ARIA target: ${id}`);
    }
  }
  if (file.endsWith('.css')) {
    for (const match of content.matchAll(/url\(['"]?([^'"\)]+)['"]?\)/g)) verifyReference(file, match[1]);
  }
  if (file.endsWith('.js')) new vm.Script(content, { filename: file });
}
const workflow = fs.readFileSync(path.join(root, '.github', 'workflows', 'pages.yml'), 'utf8');
assert(/path: docs\/site\s/.test(workflow), 'Publish only the website directory');
assert(!/path: ['"]?\.(?:['"]?\s|\/)/.test(workflow), 'Never publish checkout root');
assert(workflow.includes("github.ref == 'refs/heads/main' && github.event_name != 'pull_request'"), 'Deploy main only, never PR code');
assert(workflow.includes('node tests/website-check.cjs'), 'Validate before publication');
const total = files.reduce((bytes, file) => bytes + fs.statSync(file).size, 0);
console.log(`Website OK: ${files.length} public files, ${links} local references, ${total.toLocaleString('en-US')} bytes. JavaScript syntax and Pages publication boundary verified.`);
