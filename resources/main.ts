import {Application, Button, Frame} from "./eto";

var app: Application;


app.initMainFrame(new Frame({
    title: 'Приложение',
    width: 400,
    height: 300,
    items: [
        new Button({
            row: 1,
            column: 1,
            width: '*',
            label: 'text 3',
            click: () => {

            }
        })
    ]
}));


