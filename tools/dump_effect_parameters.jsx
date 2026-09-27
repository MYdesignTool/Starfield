/*
 * Dumps the parameter tree of the effects on the selected layers (or of every layer in the
 * active composition). Uses only the public After Effects scripting DOM: nothing is
 * decompiled, no third-party binary is read, and AE itself exposes every effect parameter
 * - hidden and non-animated ones included - to scripting.
 *
 * WHY THIS VERSION LOOKS DEFENSIVE
 *   The first run produced "nothing happened": no dialog and no file. That almost always
 *   means the script never executed inside AE (double-clicking a .jsx hands it to whatever
 *   Windows has associated with it) or that file writing is disabled in preferences. This
 *   version therefore announces itself before doing any work, catches every error and shows
 *   it, tries several output folders, and prints its findings in a dialog if it cannot write
 *   any file at all - a run can no longer fail silently.
 *
 * HOW TO RUN (After Effects 2023)
 *   1. Edit > Preferences > Scripting & Expressions > enable
 *      "Allow Scripts to Write Files and Access Network" (needed to save the dump).
 *   2. Open a composition. Select the layers whose effects you want, or select nothing to
 *      walk every layer (put one instance of each module on one layer to capture them all).
 *   3. File > Scripts > Run Script File... and pick this file. Do NOT double-click the file.
 *   4. You get a dialog first (it proves the script is running) and a second one with the
 *      result path: effect_parameters.txt on the Desktop, or in the temp folder.
 *
 * OUTPUT (one tab-indented line per property)
 *   name | matchName | idx N | value | timeVarying <bool> | keys N
 * Vectors print as [a, b, c, d]. Popup choices are not exposed to scripting, so a popup
 * reports its 1-based value; the label strings come from the Effect Controls UI.
 */

(function () {
    // Set to true to only verify that the script runs, without walking anything.
    var SMOKE_ONLY = false;

    var lines = [];

    function report(message) {
        try { alert(message); } catch (error) { /* alert is unavailable: the file is the report */ }
    }

    function pad(depth) {
        var text = "";
        for (var i = 0; i < depth; i++) text += "\t";
        return text;
    }

    function valueOf(prop) {
        try {
            var value = prop.value;
            if (value instanceof Array) return "[" + value.join(", ") + "]";
            return String(value);
        } catch (error) {
            return "<no scalar value>";
        }
    }

    function dumpProperty(prop, depth) {
        var line = pad(depth) + prop.name + " | " + prop.matchName + " | idx " + prop.propertyIndex +
                   " | " + valueOf(prop);
        try {
            line += " | timeVarying " + prop.isTimeVarying + " | keys " + prop.numKeys;
        } catch (error) {
            /* not every property exposes keyframe state */
        }
        lines.push(line);
        var children = 0;
        try { children = prop.numProperties; } catch (error) { children = 0; }
        for (var i = 1; i <= children; i++) dumpProperty(prop.property(i), depth + 1);
    }

    function dumpLayer(layer) {
        var layerName = "<unnamed>";
        try { layerName = layer.name; } catch (error) { /* keep the placeholder */ }
        lines.push("=== LAYER: " + layerName + " ===");
        var parade = null;
        try { parade = layer.property("ADBE Effect Parade"); } catch (error) { parade = null; }
        if (!parade) {
            lines.push("    <no effect parade>");
            return;
        }
        for (var e = 1; e <= parade.numProperties; e++) {
            var effect = parade.property(e);
            lines.push("");
            lines.push("EFFECT " + e + ": " + effect.name + " | matchName=" + effect.matchName);
            for (var p = 1; p <= effect.numProperties; p++) dumpProperty(effect.property(p), 1);
        }
    }

    function writeReport(text) {
        var folders = [];
        try { folders.push(Folder.desktop); } catch (error) { /* ignore */ }
        try { folders.push(Folder.temp); } catch (error) { /* ignore */ }
        try { folders.push(new File($.fileName).parent); } catch (error) { /* ignore */ }
        for (var i = 0; i < folders.length; i++) {
            var file = new File(folders[i].fsName + "/effect_parameters.txt");
            file.encoding = "UTF-8";
            if (!file.open("w")) continue;
            file.write(text);
            file.close();
            return file.fsName;
        }
        return null;
    }

    // Tell the user the script is alive before anything can go wrong.
    report("Starfield parameter dump\nAfter Effects " + app.version + "\n" +
           (SMOKE_ONLY ? "Smoke test only: nothing will be walked." : "Reading the active composition..."));
    if (SMOKE_ONLY) return;

    try {
        var fileWriteAllowed = true;
        try {
            var security = app.preferences.getPrefAsLong("Main Pref Section", "Pref_SCRIPTING_FILE_NETWORK_SECURITY");
            fileWriteAllowed = (security === 1);
        } catch (error) { fileWriteAllowed = true; }

        lines.push("AE " + app.version + " | file write allowed: " + fileWriteAllowed);

        if (!app.project) { report("Open a project first."); return; }
        var comp = app.project.activeItem;
        if (!comp || !(comp instanceof CompItem)) { report("Open a composition first."); return; }

        var layers = comp.selectedLayers;
        if (!layers || layers.length === 0) {
            layers = [];
            for (var i = 1; i <= comp.numLayers; i++) layers.push(comp.layer(i));
        }
        lines.push("Comp: " + comp.name + " | layers walked: " + layers.length);

        for (var l = 0; l < layers.length; l++) dumpLayer(layers[l]);

        var text = lines.join("\r\n");
        var written = writeReport(text);
        if (written) {
            report("Dumped " + layers.length + " layer(s), " + lines.length + " lines to:\n" + written);
        } else {
            report("Could not write any file.\nEnable Edit > Preferences > Scripting & Expressions >\n" +
                   "\"Allow Scripts to Write Files and Access Network\", or copy the first lines from the next dialog.\n\n" +
                   text.substring(0, 1500));
        }
    } catch (error) {
        var where = "";
        try { if (error.line) where = " (line " + error.line + ")"; } catch (inner) { /* ignore */ }
        report("Script error: " + error.toString() + where);
    }
})();
