/**
 * @typedef {Object} ApplicationStorage
 * @property {function(string): void} add - Добавить объект в хранилизе. Первый параметр имя коллекции, второй объект
 * @property {function(string): void} filter - Фильтр объектов. Первый параметр это имя коллекции, второй опциональнй ( '$.age' = 25 )
 * @property {function(string): void} remove - Удалить по Id. Первый параметр имя коллекции, второй Id
 * @property {function(string): void} update - Обновить объект по Id. Первый параметр имя коллекции, второй объект, в котором есть Id
 * @property {function(string): void} getById - Получить объект по Id. Первый параметр имя коллекции, второй Id
 */

/**
 * @typedef {Object} ApplicationFile
 * @property {function(string): boolean} exists - Проверка файла на существование
 * @property {function(string, string): void} write - Записать контент в файл
 * @property {function(string): string} read - Прочитать контент из файла
 * @property {function(string): string} extension - Расширение файла
 * @property {function(string): string} fileName - Имя файла в папке
 * @property {function(string): string} size - Размер файла
 * @property {function(string): string} directory - Получить папку где есть файл
 * */

/**
 * @typedef {Object} ApplicationDirectory
 * @property {function(string): boolean} exists - Проверка папку на существование
 * @property {function(string): boolean} create - Создание папки
 * @property {function(string): string} getDirectoryExecutable - Папка из которой запущено приложение
 * @property {function(string): Array} getListFile - Список файлов в папке
 * */

/**
 * @typedef {Object} Application
 * @property {function(string): void} alert - Выводит системное уведомление
 * @property {function(Object): void} initMainFrame - Инициализировать основную форму приложения 
 * @property {function(string): Object} findByName - Найти компонент по имени
 * @property {function(function): Promise} startAsync - Функция для выполнения асинхронной операции
 * @property {function(): string} getArguments - Список агрументов для запуска
 * @property {function(): void} exit - Закрыть приложение
 * @property {ApplicationStorage} storage - Работа с хранилищем
 * @property {ApplicationFile} file - Работа с файлами
 * @property {ApplicationDirectory} directory - Работа с папками
 */

/**
 * @typedef {Object} FrameConfig
 * @property {number} id - Идентификатор контрола
 * @property {string} name - Имя контрола
 * @property {string} title - Заголовок
 * @property {any} width - Ширина
 * @property {any} height - Высота
 * @property {Array} items - Массив элементов
 * @property {function} run - Функция, что вызывается, когда фрейм инициализировался
 */
class Frame {
    /**
     * @param {FrameConfig} config - Объект настроек
     */
    constructor(config) {
        Object.assign(this, config);
    }
}


/**
 * @typedef {Object} GridBagConfig
 * @property {string} name - Имя контрола
 * @property {number} id - Идентификатор контрола
 * @property {any} width - Ширина
 * @property {any} height - Высота
 * @property {number} column - Столбец внутри контейнера
 * @property {number} columnSpan - Количество занимаемях столбцов
 * @property {number} row - Строка внутри контейнера
 * @property {number} rowSpan - Количество занимаемях строк
 * @property {Array} items - Массив элементов
 */
class GridBagLayout {
    /**
     * @param {GridBagConfig} config - Объект настроек
     */
    constructor(config) {
        Object.assign(this, config);
    }
}


/**
 * @typedef {Object} LabelConfig
 * @property {number} id - Идентификатор контрола
 * @property {string} name - Имя контрола
 * @property {any} width - Ширина
 * @property {any} height - Высота
 * @property {number} column - Столбец внутри контейнера
 * @property {number} columnSpan - Количество занимаемях столбцов
 * @property {number} row - Строка внутри контейнера
 * @property {number} rowSpan - Количество занимаемях строк
 * @property {string} label - Текст в подсказке
 */
class Label {
    /**
     * @param {LabelConfig} config - Объект настроек
     */
    constructor(config) {
        Object.assign(this, config);
    }
}


/**
 * @typedef {Object} TextConfig
 * @property {number} id - Идентификатор контрола
 * @property {string} name - Имя контрола
 * @property {any} width - Ширина
 * @property {any} height - Высота
 * @property {number} column - Столбец внутри контейнера
 * @property {number} columnSpan - Количество занимаемях столбцов
 * @property {number} row - Строка внутри контейнера
 * @property {number} rowSpan - Количество занимаемях строк
 * @property {string} value - Текст в подсказке
 * @property {string} placeholder - Текст как подсказка в пустом поле
 * @property {boolean} multiline - Поле многострочное
 * @property {function} changed - Событие вызывается при изменении значения
 */
class Text {
    /**
     * @param {TextConfig} config - Объект настроек
     */
    constructor(config) {
        Object.assign(this, config);
    }

    /**
   * Установить значение в текстовое поле
   * @param {string} value - текст для поля
   * @returns {undefined} Результат
   */
    setValue (value) { }

    /**
   * Получить значение из текстового поля
   * @returns {string} Текст из поля
   */
    getValue () { }
}

/**
 * @typedef {Object} CheckBoxConfig
 * @property {number} id - Идентификатор контрола
 * @property {string} name - Имя контрола
 * @property {any} width - Ширина
 * @property {any} height - Высота
 * @property {number} column - Столбец внутри контейнера
 * @property {number} columnSpan - Количество занимаемях столбцов
 * @property {number} row - Строка внутри контейнера
 * @property {number} rowSpan - Количество занимаемях строк
 * @property {number} selectedIndex - Номер выбранного элемента
 * @property {Array} items - Элементы, с ключами text и value
 * @property {function} changed - Событие вызывается при изменении значения
 */
class CheckBox {

    /**
     * @param {CheckBoxConfig} config - Объект настроек
     */
    constructor(config) {
        Object.assign(this, config);
    }
}

/**
 * @typedef {Object} ComboBoxConfig
 * @property {number} id - Идентификатор контрола
 * @property {string} name - Имя контрола
 * @property {any} width - Ширина
 * @property {any} height - Высота
 * @property {number} column - Столбец внутри контейнера
 * @property {number} columnSpan - Количество занимаемях столбцов
 * @property {number} row - Строка внутри контейнера
 * @property {number} rowSpan - Количество занимаемях строк
 * @property {boolean} value - Текущее значение
 * @property {string} label - Подсказка
 * @property {function} changed - Событие вызывается при изменении значения
 */
class ComboBox {
    /**
     * @param {ComboBoxConfig} config - Объект настроек
     */
    constructor(config) {
        Object.assign(this, config);
    }

    /**
     * Установить список элементов. Это массив из text и value свойств
     */
    setItems (items) { }

    /**
     * Установить подсказку
     * @param {string} label - подсказка
     * @returns {undefined} Результат
     */
    setLabel (label) { }

    /**
     * Получить начение
     * @returns {string} Результат
     */
    getValue () { }

    /**
     * Установить значение
     * @param {string} value - Установить новое активное значение
     * @returns {undefined} Результат
     */
    setValue (value) { }
}

/**
 * @typedef {Object} ButtonConfig
 * @property {number} id - Идентификатор контрола
 * @property {string} name - Имя контрола
 * @property {any} width - Ширина
 * @property {any} height - Высота
 * @property {number} column - Столбец внутри контейнера
 * @property {number} columnSpan - Количество занимаемях столбцов
 * @property {number} row - Строка внутри контейнера
 * @property {number} rowSpan - Количество занимаемях строк
 * @property {string} label - Текст в подсказке
 * @property {function} click - Функция при клике
 */
class Button {
    /**
     * @param {ButtonConfig} config - Объект настроек
     */
    constructor(config) {
        Object.assign(this, config);
    }
}

/**
 * Git репозиторий
 * @property {string} - Полный путь до любого файла в репозитории
 * */
class GitRepository {
    _repositoryFileName;
    _topLevelPath;
    constructor(repositoryFileName) {
        this._repositoryFileName = repositoryFileName;
    }

    /**
     * Выполнить команду в git консоли
     * @param {string} - подсказка
     * @returns {Promise} Результат
     */
    async runGitCommand(command) {}

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

globalThis.Text = Text;
globalThis.Button = Button;
globalThis.ComboBox = ComboBox;
globalThis.Label = Label;
globalThis.GridBagLayout = GridBagLayout;
globalThis.Frame = Frame;
globalThis.GitRepository = GitRepository;

