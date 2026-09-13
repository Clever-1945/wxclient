/** @typedef {import('./eto.js')} SourceTypes */
/** @type {import('./eto.js').Application} */
var app;

app.initMainFrame(new Frame({
    title: 'Приложение',
    width: 400,
    height: 300,
    items: [
        new Button({
            row: 1,
            column: 1,
            width: '*',
            text: 'text 3',
            click: () => {
                
            }
        }),
    ]
}));



