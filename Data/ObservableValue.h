#pragma once
#include <map>
#include <cstdint>
#include <functional>
#include <atomic>
#include <mutex>
#include <iostream>

template<typename T>
class ObservableValue {
private:
    inline static std::atomic<int64_t> counterListeners{0};
    std::map<int64_t, std::function<void()>> listeners;
    std::vector<T> values;
    bool multiValue = false;
    std::mutex mtx;

public:
    ObservableValue(bool multiValue = true) {
        this->multiValue = multiValue;
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
        std::unique_lock<std::mutex> lock(mtx, std::try_to_lock);

        if (this->multiValue) {
            this->values.push_back(value);
        } else {
            if (this->values.size() < 1) {
                this->values.push_back(value);
            } else {
                this->values[0] = value;
            }
        }

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