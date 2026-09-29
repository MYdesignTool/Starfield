/*
 * Disposable probe for direct ExtendScript writes to the CUSTOM_VALUE graph
 * parameter. It duplicates the selected effect layer, attempts to scan a
 * deliberately invalid short graph string on the duplicate, and removes the
 * duplicate in the same undo group. It never changes the original effect and
 * never saves the project.
 *
 * Run only in a throwaway project copy. The malformed input is too short to
 * pass GraphParameter.cpp's bounded SFLDGRAPH1 scanner. The result is only a
 * clue about how Property.setValue handles malformed CUSTOM_VALUE input; it
 * does not prove that a valid graph write, undo, or persistence will work.
 */
(function () {
    var MATCH_NAME = "org.starfieldfx.particle";
    var PARAMETER_NAME = "Node Graph Data";
    var INVALID_PROBE_TEXT = "SFLDGRAPH1:00";
    var lines = ["Starfield graph setValue probe", "AE " + app.version];

    function findEffect(layer) {
        var parade = layer ? layer.property("ADBE Effect Parade") : null;
        var found = null;
        if (!parade) return null;
        for (var i = 1; i <= parade.numProperties; i++) {
            var candidate = parade.property(i);
            if (candidate && candidate.matchName === MATCH_NAME) {
                if (found) return null;
                found = candidate;
            }
        }
        return found;
    }

    if (!app.project) {
        alert(lines.join("\n") + "\nNo project is open.");
        return;
    }
    var comp = app.project.activeItem;
    if (!comp || !(comp instanceof CompItem)) {
        alert(lines.join("\n") + "\nOpen a composition first.");
        return;
    }
    var selected = comp.selectedLayers;
    if (!selected || selected.length !== 1) {
        alert(lines.join("\n") + "\nSelect exactly one layer carrying one Starfield Particle effect.");
        return;
    }
    var originalLayer = selected[0];
    var originalEffect = findEffect(originalLayer);
    if (!originalEffect) {
        alert(lines.join("\n") + "\nThe selected layer must carry exactly one Starfield Particle effect.");
        return;
    }
    var originalProperty = originalEffect.property(PARAMETER_NAME);
    if (!originalProperty || typeof originalProperty.setValue !== "function") {
        alert(lines.join("\n") + "\nThe graph property or its setValue method is unavailable.");
        return;
    }

    lines.push("Target: " + comp.name + " / " + originalLayer.name);
    lines.push("The test duplicates the layer, calls setValue with invalid two-byte text, and removes the duplicate.");
    lines.push("The result is only a clue about malformed CUSTOM_VALUE writes; it does not prove a valid graph write.");
    lines.push("Run this only in a throwaway project copy; the project may be marked modified, but this script does not save it.");
    if (!confirm(lines.join("\n") + "\n\nContinue?")) return;

    var duplicate = null;
    var groupOpen = false;
    var outcome = "No write attempt completed.";
    var cleanupError = null;
    try {
        app.beginUndoGroup("Starfield graph setValue probe");
        groupOpen = true;
        duplicate = originalLayer.duplicate();
        if (!duplicate) throw new Error("AE did not duplicate the selected layer.");
        var duplicateEffect = findEffect(duplicate);
        if (!duplicateEffect) throw new Error("The duplicate does not contain exactly one Starfield Particle effect.");
        var duplicateProperty = duplicateEffect.property(PARAMETER_NAME);
        if (!duplicateProperty || typeof duplicateProperty.setValue !== "function") {
            throw new Error("The duplicate graph property or its setValue method is unavailable.");
        }
        try {
            duplicateProperty.setValue(INVALID_PROBE_TEXT);
            outcome = "setValue returned without throwing for intentionally invalid text.";
        } catch (error) {
            outcome = "setValue threw: " + error.toString();
        }
    } catch (error) {
        outcome = "Probe setup failed: " + error.toString();
    } finally {
        if (duplicate) {
            try { duplicate.remove(); }
            catch (error) { cleanupError = error.toString(); }
        }
        if (groupOpen) {
            try { app.endUndoGroup(); }
            catch (error) { cleanupError = cleanupError || error.toString(); }
        }
        try { originalLayer.selected = true; } catch (ignored) { /* selection restoration is best effort */ }
    }

    lines.push(outcome);
    lines.push(duplicate && cleanupError ? "Cleanup needs attention: " + cleanupError :
               cleanupError ? "Undo-group cleanup error: " + cleanupError :
               "Temporary duplicate removed; original effect was not written; project was not saved.");
    alert(lines.join("\n"));
})();
