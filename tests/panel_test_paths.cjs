"use strict";
const path=require("node:path");
const root=path.resolve(process.env.STARFIELD_PANEL_ROOT || path.join(__dirname,"../web_panel"));
// All panel modules and file reads use the same explicitly selected candidate.
// Assertions, host mocks and protocol behavior are unchanged.
module.exports=relative=>path.join(root,relative);
