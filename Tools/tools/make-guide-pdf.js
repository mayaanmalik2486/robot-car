'use strict';
// Makes Docs\Robot_Car_Guide.pdf from Docs\CAR_AND_APP_GUIDE.md: turns the markdown into a
// plain web page, then lets Edge (or Chrome) print it to a PDF.
// Run: node Tools\tools\make-guide-pdf.js
const fs = require('fs');
const path = require('path');
const { execFileSync } = require('child_process');

const docs = path.join(__dirname, '..', '..', 'Docs');
// usage: node make-guide-pdf.js [page.md] [file.pdf]   (default: the full guide)
const source = process.argv[2] || 'CAR_AND_APP_GUIDE.md', target = process.argv[3] || 'Robot_Car_Guide.pdf';
const md = fs.readFileSync(path.join(docs, source), 'utf8').replace(/\r\n/g, '\n');

const esc = (s) => s.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
function inline(s) {
  s = esc(s).replace(/\\\|/g, '|');
  s = s.replace(/!\[([^\]]*)\]\(([^)]+)\)/g, '<img alt="$1" src="$2">');
  s = s.replace(/\[([^\]]+)\]\(([^)]+)\)/g, '<a href="$2">$1</a>');
  s = s.replace(/`([^`]+)`/g, '<code>$1</code>');
  s = s.replace(/\*\*([^*]+)\*\*/g, '<b>$1</b>');
  return s.replace(/(^|[\s(])\*([^*\n]+)\*/g, '$1<i>$2</i>');
}
const cells = (line) => line.trim().replace(/^\||\|$/g, '').split(/(?<!\\)\|/).map((c) => c.trim());

const out = [];
const lines = md.split('\n');
for (let i = 0; i < lines.length; i++) {
  const l = lines[i];
  let m;
  if ((m = /^(#{1,3}) (.*)$/.exec(l))) { out.push(`<h${m[1].length}>${inline(m[2])}</h${m[1].length}>`); continue; }
  if (/^---+$/.test(l)) { out.push('<hr>'); continue; }
  if (l.startsWith('|') && /^\|[\s:|-]+\|$/.test(lines[i + 1] || '')) {
    out.push('<table><thead><tr>' + cells(l).map((c) => `<th>${inline(c)}</th>`).join('') + '</tr></thead><tbody>');
    for (i += 2; i < lines.length && lines[i].startsWith('|'); i++) out.push('<tr>' + cells(lines[i]).map((c) => `<td>${inline(c)}</td>`).join('') + '</tr>');
    i--;
    out.push('</tbody></table>');
    continue;
  }
  if (/^(- |\d+\. )/.test(l)) {
    const ordered = /^\d/.test(l);
    out.push(ordered ? '<ol>' : '<ul>');
    for (; i < lines.length && /^(- |\d+\. )/.test(lines[i]); i++) out.push('<li>' + inline(lines[i].replace(/^(- |\d+\. )/, '')) + '</li>');
    i--;
    out.push(ordered ? '</ol>' : '</ul>');
    continue;
  }
  if (l.trim()) out.push('<p>' + inline(l) + '</p>');
}

const html = `<!doctype html><meta charset="utf-8"><title>Robot Car</title><style>
@page { size: A4; margin: 16mm 14mm; }
body { font: 10.5pt/1.5 "Segoe UI", Arial, sans-serif; color: #16181b; }
h1 { font-size: 26pt; margin: 0 0 4pt; } h2 { font-size: 15pt; margin: 20pt 0 6pt; border-bottom: 2px solid #ff7a1a; padding-bottom: 3pt; break-after: avoid; }
h3 { font-size: 11.5pt; margin: 14pt 0 4pt; break-after: avoid; }
table { border-collapse: collapse; width: 100%; margin: 6pt 0 10pt; font-size: 9.5pt; } tr { break-inside: avoid; }
th { background: #16181b; color: #fff; text-align: left; } th, td { border: 1px solid #c5c8c2; padding: 4pt 6pt; vertical-align: top; }
tr:nth-child(even) td { background: #f4f4f1; } td:first-child { font-weight: 600; }
code { background: #eee; padding: 0 3pt; border-radius: 3pt; font-size: 9.5pt; } img { max-width: 100%; } a { color: #a63a06; } hr { border: 0; border-top: 1px solid #c5c8c2; }
ul, ol { margin: 4pt 0 8pt; padding-left: 18pt; } p { margin: 4pt 0 8pt; }
</style>` + out.join('\n');
const page = path.join(docs, '_guide.html');
fs.writeFileSync(page, html);

const browsers = ['C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe', 'C:/Program Files/Google/Chrome/Application/chrome.exe'];
const browser = browsers.find((b) => fs.existsSync(b));
if (!browser) throw new Error('No Edge or Chrome found to print the PDF');
const pdf = path.join(docs, target);
execFileSync(browser, ['--headless', '--disable-gpu', '--no-pdf-header-footer', '--print-to-pdf=' + pdf, 'file:///' + page.replace(/\\/g, '/')], { stdio: 'ignore' });
fs.unlinkSync(page);
console.log('wrote ' + pdf + ' (' + Math.round(fs.statSync(pdf).size / 1024) + ' KB)');
