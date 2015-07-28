module.exports = {
    entry: "./entry.js",
    output: {
        path: __dirname,
        filename: "bundle.js"
    },
    module: {
        loaders: [
            { test: /\.css$/, loader: "style!css" }
        ]
    },
    externals: {
        // require("d3") is external and available
        //  on the global var d3
        "io": "io"
    }
};