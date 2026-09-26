/** Базовый объект приложения */
export interface Application {
    /** Выводит системное уведомление */
    alert: (text: string) => void;
    /** Выводит системное уведомление с ошибкой */
    error: (text: string) => void;
    /** Диалоговое окно с вопросом. Ответ может быть либо ДА либо НЕТ */
    yesNo: (text: string) => boolean;
    /** Инициализировать основную форму приложения */
    initMainFrame: (cfg: Frame) => void;
    /** Найти компонент по имени */
    findByName: (name: string) => any;
    /** Список агрументов для запуска */
    getArguments: () => string[];
    /** Закрыть приложение */
    exit: () => void;
    /** Функция для выполнения асинхронной операции */
    startAsync: (fn: () => Promise<any>) => Promise<any>
    /** Объект работы с хранилищем */
    storage: {
        /** Добавить объект в хранилизе. Первый параметр имя коллекции, второй объект */
        add: (storageName: string, instance: any) => void;
        /** Удалить по Id. Первый параметр имя коллекции, второй Id */
        remove: (storageName: string, id: any) => void;
        /** Обновить объект по Id. Первый параметр имя коллекции, второй объект, в котором есть Id */
        update: (storageName: string, instance: any) => void;
        /** Получить объект по Id. Первый параметр имя коллекции, второй Id */
        getById: (storageName: string, id: any) => any;
        /** Фильтр объектов. Первый параметр это имя коллекции, второй опциональнй ( '$.age' = 25 ) */
        filter: (storageName: string, filter: string) => any[];
    },
    /** Объект работы с файлами */
    file: {
        /** Проверка файла на существование */
        exists: (fileName: string) => boolean;
        /* Записать контент в файл */
        write: (fileName: string, content: string) => void;
        /* Прочитать контент из файла */
        read: (fileName: string) => string;
        /** Расширение файла */
        extension: (fileName: string) => string;
        /** Получить имя файла, по полному пути */
        fileName: (fileName: string) => string;
        /** Получить размер файла */
        size: (fileName: string) => number;
        /** Получить папку по имени файла */
        directory: (fileName: string) => string;
    },
    /** Объект работы с каталогами */
    directory: {
        /** Проверка папку на существование */
        exists: (directoryName: string) => boolean;
        /** Создание папки */
        create: (directoryName: string) => boolean;
        /** Папка из которой запущено приложение */
        getDirectoryExecutable: () => string;
        /** Список файлов в папке */
        getListFile: () => string[];
    }
}

/** Базовый интерфейс контрола */
export interface IControl {
    /** Имя контрола */
    name?: string;
    /** Ширина в пикселях или в отношениях, но через символ *. например просто* или 2* */
    width?: string | number;
    /** Высота в пикселях или в отношениях, но через символ *. например просто* или 2* */
    height?: string | number;
    /** Столбец внутри контейнера */
    column?: number;
    /** Количество занимаемях столбцов */
    columnSpan?: number;
    /** Строка внутри контейнера */
    row?: number;
    /** Количество занимаемях строк */
    rowSpan?: number;
}

/** Свойства фрейма */
export interface IFrame extends IControl {
    /** Заголовок */
    title?: string;
    /** Массив элементов */
    items?: any[];
}

/** Свойства фрейма */
export class Frame {
    constructor(public config: IFrame) {
    }
}

/** Описание контейнера с компонентами */
export interface IGridBagLayout extends IControl {
    /** Массив элементов */
    items?: any[];
}

/** Описание контейнера с компонентами */
export class GridBagLayout {
    constructor(public config: IGridBagLayout) {
    }
}

/** Описание статического текста на форме */
export interface ILabel extends IControl {
    /** Текст в подсказке */
    label?: string;
}

/** Описание статического текста на форме */
export class Label{
    constructor(public config: IGridBagLayout) {
    }
}

/** Описание текстового поля ввода */
export interface IText extends IControl {
    /** Текст в поле ввода */
    value?: string;
    /** Текст как подсказка в пустом поле */
    placeholder?: string;
    /** Поле многострочное */
    multiline?: boolean;
    /** Событие вызывается при изменении значения */
    changed?: (fn: (input: Text) => void) => void;
}

/** Описание текстового поля ввода */
export class Text {
    constructor(public config: IText) {
    }

    /** Установить значение в текстовое поле */
    public setValue(value: string): void {

    }

    /** Получить значение из текстового поля */
    public getValue(): string {
        return '';
    }
}

export interface ICheckBox extends IControl {
    /** Событие вызывается при изменении значения */
    changed?: (control: CheckBox) => void;
}

export class CheckBox {
    constructor(public config: ICheckBox) {
    }
}

/** Описание выпадающего списка */
export interface IComboBox extends IControl {
    /** Текущее выбранное значение */
    value?: string;
    /** Список элементов */
    items?: {text: string, value: string}[];
    /** Событие вызывается при изменении значения */
    changed?: (control: ComboBox) => void;
}

/** Описание выпадающего списка */
export class ComboBox {
    constructor(public config: IComboBox) {
    }

    /** Установить список элементов. Это массив из text и value свойств */
    public setItems(items: {text: string, value: string}[]): void {

    }

    /** Получить выбранное значение */
    public getSelectedValue(): string {
        return '';
    }
    /** Установить выбранное значение */
    public setSelectedValue(value: string): void {

    }
}

/** Конфигурация кнопки */
export interface IButton extends IControl {
    /** Текст на кнопке */
    label?: string;
    /** Вызывается при клике на кнопку */
    click?: (control: Button) => void;
}

/** Конфигурация кнопки */
export class Button {
    constructor(public config: IButton) {
    }
}

/** Git репозиторий */
export class GitRepository {
    _repositoryFileName: string;
    _topLevelPath: string;
    constructor(repositoryFileName: string) {
        this._repositoryFileName = repositoryFileName;
    }

    /** Выполнить команду в git консоли */
    // @ts-ignore
    async runGitCommand(command: string): Promise<string> {
        return '';
    }

    /** Получить папку верхнего уровня репозитория */
    async getTopLevelPath() {
        this._topLevelPath = this._topLevelPath || ((await this.runGitCommand('rev-parse --show-toplevel')) || '').trim();
        return this._topLevelPath;
    }

    /** Получение списка бранчей */
    async getListBranch() {
        const listLocal = (await this.runGitCommand("branch")).split('\n').map(x => x.trim()).filter(x => !!x);
        const listRemote = (await this.runGitCommand("branch -r")).split('\n').map(x => x.trim()).filter(x => !!x);
        if (listRemote.length >= 1) {
            listRemote.splice(0, 1);
        }
        const listAllBranch = [listLocal, listRemote];
        const list = [];

        for(let i = 0 ; i < listAllBranch.length ; i++) {
            const listBranch = listAllBranch[i];
            for(let j = 0 ; j < listBranch.length ; j++) {
                const branch = listBranch[j];

                let branchName = branch.trim();
                // @ts-ignore
                let isCurrent = branchName.startsWith('*');

                list.push({
                    isCurrent: isCurrent,
                    name: branchName.replace(new RegExp("^\\*+"), "").trim(),
                    isRemote: listBranch === listRemote
                });
            }
        }

        return list;
    }
}

(globalThis as any).Text = Text;
(globalThis as any).Button = Button;
(globalThis as any).ComboBox = ComboBox;
(globalThis as any).Label = Label;
(globalThis as any).GridBagLayout = GridBagLayout;
(globalThis as any).Frame = Frame;
(globalThis as any).GitRepository = GitRepository;

// Пишем правильную заглушку require
globalThis.require = function(moduleName) {
    if (moduleName === './eto' || moduleName === 'eto') {
        return {
            Text: globalThis.Text,
            Button: globalThis.Button,
            ComboBox: globalThis.ComboBox,
            Label: globalThis.Label,
            GridBagLayout: globalThis.GridBagLayout,
            Frame: globalThis.Frame,
            GitRepository: globalThis.GitRepository,
        };
    }
    return {};
};
