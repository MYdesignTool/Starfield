/*
 * Read-only probe for the public ExtendScript view of Starfield's hidden graph
 * parameter. It does not change a project, write files, or read/write preferences.
 * Run from AE 2023 with exactly one layer carrying Starfield Particle selected,
 * then return the dialog text so ADR 0009 can choose a graph-edit transport.
 */
(function () {
    var MATCH_NAME = "org.starfieldfx.particle";
    var PARAMETER_NAME = "Node Graph Data";
    var lines = ["Starfield graph property probe (read-only)", "AE " + app.version];

    function describeValue(property) {
        try {
            var value = property.value;
            var kind = typeof value;
            if (value === null) return "null";
            if (kind === "string") return "string, length " + value.length;
            if (value instanceof Array) return "array, length " + value.length;
            if (kind === "undefined") return "undefined";
            if (kind === "object") return "object";
            return kind + ", scalar " + String(value).substring(0, 80);
        } catch (error) {
            return "read threw: " + error.toString();
        }
    }

    try {
        if (!app.project) { alert(lines.join("\n") + "\nNo project is open."); return; }
        var comp = app.project.activeItem;
        if (!comp || !(comp instanceof CompItem)) {
            alert(lines.join("\n") + "\nOpen a composition and select one effect layer.");
            return;
        }
        var selected = comp.selectedLayers;
        if (!selected || selected.length !== 1) {
            alert(lines.join("\n") + "\nSelect exactly one layer carrying Starfield Particle.");
            return;
        }
        var effects = selected[0].property("ADBE Effect Parade");
        var match = null;
        if (effects) {
            for (var i = 1; i <= effects.numProperties; i++) {
                var effect = effects.property(i);
                if (effect && effect.matchName === MATCH_NAME) {
                    if (match) { alert(lines.join("\n") + "\nAmbiguous: multiple Starfield effects on the layer."); return; }
                    match = effect;
                }
            }
        }
        if (!match) { alert(lines.join("\n") + "\nSelected layer has no Starfield Particle effect."); return; }
        var graph = match.property(PARAMETER_NAME);
        if (!graph) { alert(lines.join("\n") + "\nParameter not found: " + PARAMETER_NAME); return; }
        lines.push("Effect: " + match.name + " | property index " + graph.propertyIndex);
        try { lines.push("propertyValueType: " + String(graph.propertyValueType)); }
        catch (error) { lines.push("propertyValueType: unavailable (" + error.toString() + ")"); }
        lines.push("value: " + describeValue(graph));
        try { lines.push("canSetExpression: " + String(graph.canSetExpression)); }
        catch (error) { lines.push("canSetExpression: unavailable"); }
        try { lines.push("isTimeVarying: " + String(graph.isTimeVarying) + " | keyframes: " + graph.numKeys); }
        catch (error) { lines.push("keyframe info: unavailable"); }
        lines.push("setValue method: " + (typeof graph.setValue));
        alert(lines.join("\n"));
    } catch (error) {
        lines.push("Probe error: " + error.toString());
        alert(lines.join("\n"));
    }
})();
