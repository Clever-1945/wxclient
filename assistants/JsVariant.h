#include <variant>
#include <string>
#include <vector>
#include <map>

struct JsVariant;

// 2. Определяем чистые типы C++ для массивов и объектов
using JsArray = std::vector<JsVariant>;
using JsObject = std::map<std::string, JsVariant>;

// 3. Финальный универсальный стандартный тип
struct JsVariant : std::variant<
    std::monostate, // null / undefined
    bool,
    int64_t,
    double,
    std::string,
    JsArray,
    JsObject>
{
    // Наследуем все конструкторы от std::variant для удобства
    using variant::variant;

    std::optional<int64_t> to_int_64() const
    {
        if (std::holds_alternative<int64_t>(*this))
        {
            return std::get<int64_t>(*this);
        }
        return std::nullopt;
    }

    std::optional<int> to_int() const
    {
        if (std::holds_alternative<int64_t>(*this))
        {
            return static_cast<int>(std::get<int64_t>(*this));
        }
        return std::nullopt;
    }

    std::optional<bool> to_bool() const
    {
        if (std::holds_alternative<bool>(*this))
        {
            return std::get<bool>(*this);
        }
        return std::nullopt;
    }

    std::optional<double> to_double() const
    {
        if (std::holds_alternative<double>(*this))
        {
            return std::get<double>(*this);
        }
        return std::nullopt;
    }

    std::optional<JsArray> to_array() const
    {
        if (std::holds_alternative<JsArray>(*this))
        {
            return std::get<JsArray>(*this);
        }
        return std::nullopt;
    }

    std::optional<JsObject> to_object() const
    {
        if (std::holds_alternative<JsObject>(*this))
        {
            return std::get<JsObject>(*this);
        }
        return std::nullopt;
    }

    std::optional<std::string> to_string() const
    {
        if (std::holds_alternative<std::string>(*this))
        {
            return std::get<std::string>(*this); // static_cast здесь не нужен
        }
        return std::nullopt;
    }
};
