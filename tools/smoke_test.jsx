/*
 * Smoke test: proves that After Effects actually runs scripts from this folder.
 *
 * Run it exactly like the dump tool:
 *   After Effects > File > Scripts > Run Script File... > tools/smoke_test.jsx
 *
 * Expected: a dialog that says "Scripting works" and shows the AE version, the open
 * project name and the active composition.
 *
 * If nothing at all appears, the file was not run by AE (double-clicking a .jsx opens
 * whatever Windows has associated with it, which is not After Effects).
 */

(function () {
    var project = app.project ? app.project.name : "<no project>";
    var comp = (app.project && app.project.activeItem) ? app.project.activeItem.name : "<no active item>";
    var selected = "<none>";
    if (app.project && app.project.activeItem && app.project.activeItem instanceof CompItem) {
        selected = app.project.activeItem.selectedLayers.length + " layer(s) selected";
    }
    alert("Scripting works.\n\nAfter Effects: " + app.version + "\nProject: " + project +
          "\nActive item: " + comp + "\n" + selected);
})();
