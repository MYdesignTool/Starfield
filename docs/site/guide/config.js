/* Docsify owns Markdown rendering, routing, sidebar and search. */
window.$docsify={
 pageStatus:{"/":"foundation","/start/welcome":"foundation","/start/requirements":"foundation","/start/first-system":"foundation","/start/learning-path":"foundation","/interface/overview":"foundation","/interface/graph-panel":"foundation","/interface/effect-controls":"foundation","/interface/preset-panel":"foundation","/canvas/add-select":"foundation","/canvas/connections":"foundation","/canvas/insert":"foundation","/canvas/navigation":"foundation","/canvas/undo-save":"foundation","/nodes/emitter":"foundation","/nodes/particle":"foundation","/nodes/force":"foundation","/nodes/transform":"foundation","/nodes/motion":"foundation","/nodes/turbulence":"foundation","/nodes/model":"foundation","/nodes/material":"foundation","/nodes/output":"foundation","/looks/appearance":"foundation","/looks/animation":"foundation","/looks/textures-models":"foundation","/presets/apply":"foundation","/presets/save":"foundation","/presets/import-export":"foundation","/help/faq":"foundation","/help/troubleshooting":"foundation","/help/glossary":"foundation","/help/feedback":"foundation","/start/installation":"draft","/nodes/auxiliary":"draft"},
 name:'Starfield 使用文档',nameLink:'#/',loadSidebar:true,
 alias:{'/.*/_sidebar.md':'/_sidebar.md'},subMaxLevel:2,auto2top:true,topMargin:130,
 notFoundPage:'_404.md',routerMode:'hash',relativePath:false,executeScript:false,
 externalLinkTarget:'_self',search:{paths:["/","/start/welcome","/start/requirements","/start/installation","/start/first-system","/start/learning-path","/interface/overview","/interface/graph-panel","/interface/effect-controls","/interface/preset-panel","/canvas/add-select","/canvas/connections","/canvas/insert","/canvas/navigation","/canvas/undo-save","/nodes/emitter","/nodes/auxiliary","/nodes/particle","/nodes/force","/nodes/transform","/nodes/motion","/nodes/turbulence","/nodes/model","/nodes/material","/nodes/output","/looks/appearance","/looks/textures-models","/looks/animation","/presets/apply","/presets/save","/presets/import-export","/help/faq","/help/troubleshooting","/help/glossary","/help/feedback"],
 placeholder:'搜索章节与主题',noData:'没有找到相关章节',depth:2,maxAge:3600000,namespace:'starfield-guide-foundations-20261023'},
 plugins:[function(hook,vm){hook.afterEach(function(html){var label=window.$docsify.pageStatus[vm.route.path]==='foundation'?'基础说明 · 操作细节持续更新':'章节完善中';return '<div class="doc-eyebrow">STARFIELD / USER GUIDE <span>'+label+'</span></div>'+html;});hook.mounted(function(){
   document.querySelector('.docs-skip').addEventListener('click',function(event){event.preventDefault();var main=document.querySelector('.markdown-section');main.setAttribute('tabindex','-1');main.focus();});
   document.addEventListener('keydown',function(event){if(event.key==='Escape'&&window.matchMedia('(max-width:768px)').matches){document.body.classList.remove('close');document.querySelector('.sidebar-toggle').focus();}});
 });hook.doneEach(function(){
   if(window.matchMedia('(max-width:768px)').matches)document.body.classList.remove('close');
   var heading=document.querySelector('.markdown-section h1');
   document.title=(heading?heading.textContent:'使用文档')+' · Starfield';
   var toggle=document.querySelector('.sidebar-toggle');if(toggle)toggle.setAttribute('aria-label','切换文档目录');
   var search=document.querySelector('.search input');if(search)search.setAttribute('aria-label','搜索使用文档');
 });}]
};
