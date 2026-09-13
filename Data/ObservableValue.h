#pragma once
#include <map>
#include <cstdint>
#include <functional>
#include <atomic>

template<typename T>
class ObservableValue {
private:
    inline static std::atomic<int64_t> counterListeners{0};
    std::map<int64_t, std::function<void()>> listeners;
    std::vector<T> values;

public:
    ObservableValue() {
    }

    int64_t subscribe(std::function<void()> cb) {
        auto id = counterListeners++;
        listeners[id] = cb;
        return id;
    }

    void unsubscribe(int64_t id) {
        listeners.erase(id);
    }

    /** Изменение значения и уведомление всех */
    void set(T value) {
        this->values.push_back(value);
        for (const auto &[id, func]: listeners) {
            if (func) {
                func();
            }
        }
    }

    void clear() {
        this->values.clear();
        for (const auto &[id, func]: listeners) {
            if (func) {
                func();
            }
        }
    }

    /** Получение текущего значения в любой момент */
    T get(int index) {
        return values.at(index);
    }

    int count() {
        return values.size();
    }
};