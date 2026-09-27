/*
 * Dumps the parameter tree of the effects on the selected layers.
 *
 * Uses only the public After Effects scripting DOM. Nothing is decompiled, no third-party
 * binary is touched, and no file of another plug-in is read: AE itself exposes every effect
 * parameter, including hidden ones, to scripting.
 *
 * Why this exists: aligning our control names, order, defaults and ranges with the reference
 * product needs its parameter list, and reading that from screenshots is slow and incomplete.
 * A script dump is faster and more accurate than judging a binary, because it reports the
 * behavior of the installed version, not of whatever build was analysed earlier.
 *
 * How to run (After Effects 2023):
 *   1. Open a composition.
 *   2. Either select the layers whose effects you want, or select nothing to walk every layer
 *      of the composition.
 *   3. File > Scripts > Run Script File... and pick this file.
 *   4. A message shows the written path; the file is effect_parameters.txt on your Desktop.
 *
 * Output, one tab-indented line per property:
 *   name | matchName | idx N | value | timeVarying <bool> | keys N
 * Values that are vectors print as [a, b, c, d]. Popup choice strings are not exposed to
 * scripting, so a popup reports its 1-based value; its label comes from the Effect Controls UI.
 */

(function () {
    var lines = [];

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
        lines.push("=== LAYER: " + layer.name + " (" + layer.matchName + ") ===");
        var parade = null;
        try { parade = layer.property("ADBE Effect Parade"); } catch (error) { parade = null; }
        if (!parade) return;
        for (var e = 1; e <= parade.numProperties; e++) {
            var effect = parade.property(e);
            lines.push("");
            lines.push("EFFECT " + e + ": " + effect.name + " | matchName=" + effect.matchName);
            for (var p = 1; p <= effect.numProperties; p++) dumpProperty(effect.property(p), 1);
        }
    }

    if (!app.project) { alert("Open a project first."); return; }
    var comp = app.project.activeItem;
    if (!comp || !(comp instanceof CompItem)) { alert("Open a composition first."); return; }

    var layers = comp.selectedLayers;
    if (!layers || layers.length === 0) {
        layers = [];
        for (var i = 1; i <= comp.numLayers; i++) layers.push(comp.layer(i));
    }

    for (var l = 0; l < layers.length; l++) dumpLayer(layers[l]);

    var file = new File(Folder.desktop.fsName + "/effect_parameters.txt");
    file.encoding = "UTF-8";
    if (!file.open("w")) { alert("Could not write " + file.fsName); return; }
    file.write(lines.join("\r\n"));
    file.close();
    alert("Dumped " + layers.length + " layer(s), " + lines.length + " lines to:\n" + file.fsName);
})();
