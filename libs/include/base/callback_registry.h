#pragma once

#include <any>
#include <format>
#include <functional>
#include <map>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

template <typename T>
class callback_registry
{
    using erased_function = std::function<std::any(std::vector<std::any>)>;

    // Внутрішня реалізація invoke
    template <typename R, typename... Args>
    R invoke_impl(T key, Args&&... args)
    {
        if (auto it = registry_.find(key); it != registry_.end())
        {
            std::vector<std::any> any_args;
            (any_args.push_back(std::any(std::forward<Args>(args))), ...);

            try
            {
                std::any result = it->second(std::move(any_args));

                if constexpr (std::is_void_v<R>)
                {
                    // Void - нічого не повертаємо
                    return;
                }
                else
                {
                    // Повертаємо результат
                    return std::any_cast<R>(result);
                }
            }
            catch (const std::bad_any_cast&)
            {
                throw std::logic_error(std::format("Type mismatch when invoking callback for key {}", static_cast<int>(key)));
            }
        }
        throw std::logic_error(std::format("key {} not found", static_cast<int>(key)));
    }

    // Перевірка чи це std::function
    template <typename>
    struct is_std_function : std::false_type
    {
    };

    template <typename R, typename... Args>
    struct is_std_function<std::function<R(Args...)>> : std::true_type
    {
    };

    template <typename Ty>
    static constexpr bool is_std_function_v = is_std_function<Ty>::value;

    // Function traits для визначення сигнатури
    template <typename>
    struct function_traits;

    // Звичайні функції
    template <typename R, typename... Args>
    struct function_traits<R (*)(Args...)>
    {
        using args_tuple = std::tuple<std::decay_t<Args>...>;
        using return_type = R;
    };

    // std::function
    template <typename R, typename... Args>
    struct function_traits<std::function<R(Args...)>>
    {
        using args_tuple = std::tuple<std::decay_t<Args>...>;
        using return_type = R;
    };

    // Lambda та function objects з простим operator()
    template <typename F>
    struct function_traits
    {
      private:
        // Спробуємо визначити через operator()
        template <typename U>
        static auto test_call_operator(int) -> function_traits<decltype(&U::operator())>;

        template <typename>
        static auto test_call_operator(...) -> function_traits<void>;

      public:
        using args_tuple = typename decltype(test_call_operator<F>(0))::args_tuple;
        using return_type = typename decltype(test_call_operator<F>(0))::return_type;
    };

    // Спеціалізація для void (не вдалося визначити)
    template <>
    struct function_traits<void>
    {
        using args_tuple = void;
        using return_type = void;
    };

    // Member functions (const)
    template <typename C, typename R, typename... Args>
    struct function_traits<R (C::*)(Args...) const>
    {
        using args_tuple = std::tuple<std::decay_t<Args>...>;
        using return_type = R;
    };

    // Member functions (non-const)
    template <typename C, typename R, typename... Args>
    struct function_traits<R (C::*)(Args...)>
    {
        using args_tuple = std::tuple<std::decay_t<Args>...>;
        using return_type = R;
    };

    // Обгортає функцію в type-erased wrapper
    template <typename F, typename... Args>
    static erased_function wrap_function(F&& f, std::tuple<Args...>)
    {
        using traits = function_traits<std::decay_t<F>>;
        using return_type = typename traits::return_type;

        return [func = std::forward<F>(f)](std::vector<std::any> args) mutable -> std::any
        {
            if (args.size() != sizeof...(Args))
            {
                throw std::logic_error(std::format("Wrong number of arguments: expected {}, got {}", sizeof...(Args), args.size()));
            }

            auto invoke_helper = [&]<size_t... Is>(std::index_sequence<Is...>) -> std::any
            {
                if constexpr (std::is_void_v<return_type>)
                {
                    // Void функція
                    func(extract_arg<Args>(args[Is])...);
                    return std::any();
                }
                else
                {
                    // Функція з результатом
                    return std::any(func(extract_arg<Args>(args[Is])...));
                }
            };

            return invoke_helper(std::index_sequence_for<Args...>{});
        };
    }

    // Розумне витягування аргументу з any з конверсією типів
    template <typename TargetType>
    static TargetType extract_arg(std::any& source)
    {
        // Прямий cast
        if (source.type() == typeid(TargetType))
        {
            return std::any_cast<TargetType>(source);
        }

        // const char* -> std::string
        if constexpr (std::is_constructible_v<TargetType, const char*>)
        {
            if (source.type() == typeid(const char*))
            {
                return TargetType(std::any_cast<const char*>(source));
            }
        }

        // char* -> std::string
        if constexpr (std::is_constructible_v<TargetType, char*>)
        {
            if (source.type() == typeid(char*))
            {
                return TargetType(std::any_cast<char*>(source));
            }
        }

        // Числові конверсії
        if constexpr (std::is_arithmetic_v<TargetType>)
        {
            if (source.type() == typeid(int))
            {
                return static_cast<TargetType>(std::any_cast<int>(source));
            }
            if (source.type() == typeid(long))
            {
                return static_cast<TargetType>(std::any_cast<long>(source));
            }
            if (source.type() == typeid(unsigned int))
            {
                return static_cast<TargetType>(std::any_cast<unsigned int>(source));
            }
            if (source.type() == typeid(long long))
            {
                return static_cast<TargetType>(std::any_cast<long long>(source));
            }
        }

        throw std::bad_any_cast();
    }

  public:
    // Перевантаження для std::function - типи явно вказані
    template <typename R, typename... Args>
    void register_callback(T key, std::function<R(Args...)> f)
    {
        registry_[key] = wrap_function(std::move(f), std::tuple<std::decay_t<Args>...>{});
    }

    // Generic версія для звичайних функцій та lambda
    template <typename F, typename = std::enable_if_t<!is_std_function_v<std::decay_t<F>>>>
    void register_callback(T key, F&& f)
    {
        using traits = function_traits<std::decay_t<F>>;

        // Перевірка чи вдалося визначити сигнатуру
        static_assert(!std::is_void_v<typename traits::args_tuple>, "Cannot deduce function signature. Wrap in std::function<R(Args...)> or use lambda.");

        registry_[key] = wrap_function(std::forward<F>(f), typename traits::args_tuple{});
    }

    // Викликає callback БЕЗ результату (void)
    template <typename... Args>
    void invoke(T key, Args&&... args)
    {
        invoke_impl<void>(key, std::forward<Args>(args)...);
    }

    // Викликає callback З результатом
    template <typename R, typename... Args>
    R invoke(T key, Args&&... args)
    {
        return invoke_impl<R>(key, std::forward<Args>(args)...);
    }

  private:
    std::map<T, erased_function> registry_;
};
