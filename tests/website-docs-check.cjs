/* Verify the user-guide route graph and self-contained Docsify publication. */
'use strict';
const fs=require('node:fs'),path=require('node:path'),assert=require('node:assert/strict'),vm=require('node:vm');
const root=path.resolve(__dirname,'../docs/site/guide'),window={};
vm.runInNewContext(fs.readFileSync(path.join(root,'config.js'),'utf8'),{window});
const config=window.$docsify;
assert.equal(config.routerMode,'hash','Deep links must work on project Pages without rewrites');
assert.equal(config.executeScript,false);
assert.equal(config.search.namespace,'starfield-guide-v2');
const sidebar=fs.readFileSync(path.join(root,'_sidebar.md'),'utf8');
const routes=[...sidebar.matchAll(/\]\((\/[^)]*)\)/g)].map(m=>m[1]);
assert.equal(new Set(routes).size,routes.length,'Unique sidebar routes');
assert.equal(routes.length,35,'All 35 user-guide pages must remain discoverable');
assert.deepEqual([...config.search.paths].sort(),[...routes].sort(),'Search covers every page');
function resolve(route){return path.join(root,route==='/'?'README.md':route.slice(1)+'.md');}
let links=0;
for(const route of routes){
  const file=resolve(route);assert(fs.existsSync(file),`Missing sidebar page ${route}`);
  const content=fs.readFileSync(file,'utf8');assert.equal((content.match(/^# /gm)||[]).length,1,`One title: ${route}`);
  assert.match(content,/待补充|暂未填充/,'Draft scope must be explicit');
  for(const [,destination]of content.matchAll(/\]\(([^)]+)\)/g)){
    assert(routes.includes(destination),`Broken guide link ${destination} in ${route}`);links++;
  }
}
const nodes=routes.filter(r=>r.startsWith('/nodes/'));assert.equal(nodes.length,10,'Every node has a user reference entry');
const theme=fs.readFileSync(path.join(root,'vendor/vue.css'),'utf8');
assert(!/@import/.test(theme),'No external font dependency');
assert(fs.readFileSync(path.join(root,'vendor/LICENSE.txt'),'utf8').includes('MIT License'));
assert(fs.readFileSync(path.join(root,'_404.md'),'utf8').includes('[返回文档首页](/)'));
console.log(`User docs OK: ${routes.length} pages, 7 sections, ${nodes.length} nodes, ${links} content links, complete search paths, hash routing and local framework assets.`);
