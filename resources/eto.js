/**
 * @typedef {Object} Application
 * @property {function(string): void} alert - Выводит системное уведомление
 * @property {function(Object): void} initMainFrame - Инициализировать основную форму приложения 
 * @property {function(string): Object} findByName - Найти компонент по имени
 * @property {function(function): Promise} startAsync - Функция для выполнения асинхронной операции
 */


/**
 * @typedef {Object} FrameConfig
 * @property {number} id - Идентификатор контрола
 * @property {string} name - Имя контрола
 * @property {string} title - Заголовок
 * @property {any} width - Ширина
 * @property {any} height - Высота
 * @property {Array} items - Массив элементов
 * @property {string} tag - Метка контрола
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
 * @property {string} tag - Метка контрола
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
 * @property {string} tag - Метка контрола
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
 * @property {string} tag - Метка контрола
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
    setValue = function (value) { }

    /**
   * Получить значение из текстового поля
   * @returns {string} Текст из поля
   */
    getValue = function () { }
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
 * @property {string} tag - Метка контрола
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
 * @property {string} tag - Метка контрола
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
     * Получить подсказку
     * @returns {undefined} Результат
     */
    getLabel = function () { }

    /**
     * Установить подсказку
     * @param {string} label - подсказка
     * @returns {undefined} Результат
     */
    setLabel = function (label) { }

    /**
     * Получить начение
     * @returns {string} Результат
     */
    getValue = function () { }

    /**
     * Установить значение
     * @param {string} value - Установить новое активное значение
     * @returns {undefined} Результат
     */
    setValue = function (value) { }
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
 * @property {string} tag - Метка контрола
 */
class Button {
    /**
     * @param {ButtonConfig} config - Объект настроек
     */
    constructor(config) {
        Object.assign(this, config);
    }
}
    

globalThis.Text = Text;
globalThis.Button = Button;
globalThis.ComboBox = ComboBox;
globalThis.Label = Label;
globalThis.GridBagLayout = GridBagLayout;
globalThis.Frame = Frame;

