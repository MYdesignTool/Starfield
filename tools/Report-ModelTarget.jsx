// Read-only identity report. Run manually in AE after selecting the Starfield
// layer. No project changes, files, preferences, registry or commands are written.
(function () {
    var project = app.project;
    var comp = project ? project.activeItem : null;
    var layers = comp instanceof CompItem ? comp.selectedLayers : [];
    if (!project || !project.rootFolder || !(comp instanceof CompItem) || layers.length !== 1) {
        alert("Select the composition and one layer carrying Starfield, then run this report again.");
        return;
    }
    var host = typeof $ !== "undefined" && $.global ? $.global : this;
    var session = host.__SFLD_modelGraphTransactionV1;
    var status = session ? String(session.state) : "none";
    var remaining = session && typeof session.expires === "number" ?
        Math.ceil((session.expires - (new Date()).getTime()) / 1000) : "none";
    var command = app.findMenuCommandId("Starfield Apply Model Graph Transaction");
    alert("Starfield Model target (read-only)\n" +
        "token=p" + Number(project.rootFolder.id) + "-c" + Number(comp.id) + "-l" + Number(layers[0].id) +
        "\nAE=" + app.version + "\nlayer=" + layers[0].name +
        "\nengine=" + (typeof $ !== "undefined" ? String($.engineName) : "unknown") +
        "\nModel state=" + status + "; remaining seconds=" + remaining +
        "\nHost command=" + command + "; Host request entry=" +
        typeof host.SFLD_modelTransactionHostRequest);
}());
