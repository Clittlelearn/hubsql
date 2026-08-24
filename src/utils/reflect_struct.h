//
// Created by admin on 2025/7/11.
//

#ifndef REFLECT_STRUCT_H
#define REFLECT_STRUCT_H

#include "macro.h"
#include <cstdint>
#include <sstream>
#include <type_traits>
#include <optional> // 添加 optional 头文件
#include <nlohmann/json.hpp>
#include <iostream>
#include <format>
#include "field_type.h"
#include <optional>
namespace XPJSON
{

    enum json_error_type
    {
        OK,
        NULL_VALUE,
        TYPE_ERROR,
        FEILD_PASE_ERROR
    };

    static thread_local std::string JSON_PASER_ERROR;
    static thread_local int JSON_PASER_ERROR_VALUE;
    template <typename T>
    inline void make_feild_error(const std::string &key, const std::string &e)
    {
        JSON_PASER_ERROR = std::format("[key {},type {}] pase error:{}", key, std::string(typeid(T).name()), e);
        JSON_PASER_ERROR_VALUE = FEILD_PASE_ERROR;
    }
    template <typename T>
    inline void make_result(json_error_type t, const std::string &type, const std::string &key = "")
    {
        JSON_PASER_ERROR_VALUE = t;
        switch (t)
        {
        case NULL_VALUE:
        {
            JSON_PASER_ERROR = std::format("Not found the key \'{}\' value", key);
        }
        break;
        case TYPE_ERROR:
        {
            JSON_PASER_ERROR = std::format("The json type is {} , but the provided key {} type is {}!", type, key, typeid(T).name());
        }
        break;

        default:
            JSON_PASER_ERROR = "is OK!";
            break;
        }
    }
}

// 添加 std::optional 类型检查
template <typename T>
struct is_optional : std::false_type
{
};

template <typename T>
struct is_optional<std::optional<T>> : std::true_type
{
};

template <typename T>
inline constexpr bool is_optional_v = is_optional<T>::value;

template <typename T>
struct reflect_trait
{
    template <typename Func>
    static bool for_each_members(T &self, Func &&func)
    {
        return self.for_each_members(func);
    }
};

#define REFLECT_TYPE_BEGIN(Type, ...)                        \
    template <>                                              \
    struct reflect_trait<Type>                               \
    {                                                        \
        using reflect_type = bool;                           \
        template <typename Func>                             \
        static bool for_each_members(Type &stu, Func &&func) \
        {

#define REFLECT_TYPE_PER_MEMBER(name) \
    if (!func(#name, stu.name))       \
    {                                 \
        return false;                 \
    }

#define REFLECT_TYPE_END() \
    return true;           \
    }                      \
    }

#define REFLECT_TYPE(Type, ...)                              \
    REFLECT_TYPE_BEGIN(Type)                                 \
    REFLECT_PP_FOREACH(REFLECT_TYPE_PER_MEMBER, __VA_ARGS__) \
    REFLECT_TYPE_END()

#define REFLECT_TYPE_TEMPLATE_BEGIN(Type, ...)                                           \
    template <__VA_ARGS__>                                                               \
    struct reflect_trait<REFLECT_CALL(REFLECT_CALL Type)>                                \
    {                                                                                    \
        template <typename Func>                                                         \
        static bool for_each_members(REFLECT_CALL(REFLECT_CALL Type) & stu, Func &&func) \
        {

#define REFLECT_TYPE_TEMPLATE(Type, ...)                     \
    REFLECT_CALL(REFLECT_TYPE_TEMPLATE_BEGIN Type)           \
    REFLECT_PP_FOREACH(REFLECT_TYPE_PER_MEMBER, __VA_ARGS__) \
    REFLECT_TYPE_END()

#define REFLECT_PER_MEMBER(x) \
    if (!func(#x, x))         \
    {                         \
        return false;         \
    }

#define REFLECT(...)                                        \
    template <typename Func>                                \
    bool for_each_members(Func &&func)                      \
    {                                                       \
        REFLECT_PP_FOREACH(REFLECT_PER_MEMBER, __VA_ARGS__) \
                                                            \
        return true;                                        \
    }                                                       \
    using reflect_type = bool;

template <typename T>
constexpr bool is_reflect_struct_v(...)
{
    return false;
}

template <typename T>
constexpr bool is_reflect_struct_v(typename T::reflect_type p)
{
    return true;
}

template <typename T>
constexpr bool is_reflect_struct_v(typename reflect_trait<T>::reflect_type p)
{
    return true;
};

namespace reflect
{

    template <typename K, typename V>
    bool deserialize(std::map<K, V> &map, const nlohmann::json &root, const std::string &key = "");

    // 为 std::optional 添加特化
    template <typename T>
    bool deserialize(std::optional<T> &opt, const nlohmann::json &root, const std::string &key = "");

    template <typename T, std::enable_if_t<!is_reflect_struct_v<T>(false) && !is_optional_v<T>, int> = 0>
    bool deserialize(T &object, const nlohmann::json &root, const std::string &key = "");

    template <typename T>
    bool deserialize(std::vector<T> &vec, const nlohmann::json &root, const std::string &key = "");

    template <typename T, std::enable_if_t<is_reflect_struct_v<T>(false), int> = 0>
    bool deserialize(T &object, const nlohmann::json &root, const std::string &keys = "");

    template <typename T, std::enable_if_t<!is_reflect_struct_v<T>(false) && !is_optional_v<T>, int> = 0>
    nlohmann::json serialize(T &object);

    // 为 std::optional 添加序列化特化
    template <typename T>
    nlohmann::json serialize(std::optional<T> &opt);

    template <typename T>
    nlohmann::json serialize(std::vector<T> &vec);

    template <typename K, typename V, std::enable_if_t<(std::is_same_v<K, int> || std::is_same_v<K, uint64_t>), int> = 0>
    nlohmann::json serialize(std::map<K, V> &map);

    template <typename K, typename V, std::enable_if_t<!(std::is_same_v<K, int> || std::is_same_v<K, uint64_t>), int> = 0>
    nlohmann::json serialize(std::map<K, V> &map);

    template <typename T, std::enable_if_t<is_reflect_struct_v<T>(false), int> = 0>
    nlohmann::json serialize(T &object);

    // ============ 实现部分 ============

    // std::optional 反序列化
    template <typename T>
    bool deserialize(std::optional<T> &opt, const nlohmann::json &root, const std::string &key)
    {
        if (root.is_null())
        {
            opt = std::nullopt;
            return true;
        }

        T value;
        if (!deserialize(value, root, key))
        {
            return false;
        }
        opt = std::move(value);
        return true;
    }

    template <typename T, std::enable_if_t<!is_reflect_struct_v<T>(false) && !is_optional_v<T>, int>>
    nlohmann::json serialize(T &object)
    {
        nlohmann::json root;
        field_type<T>::to_json(root, object);
        return root;
    }

    // std::optional 序列化
    template <typename T>
    nlohmann::json serialize(std::optional<T> &opt)
    {
        if (opt.has_value())
        {
            return serialize(opt.value());
        }
        else
        {
            return nullptr; // JSON null
        }
    }

    template <typename T>
    nlohmann::json serialize(std::vector<T> &vec)
    {
        nlohmann::json root = nlohmann::json::array();
        for (auto &item : vec)
        {
            root.push_back(serialize(item));
        }
        return root;
    }

    template <typename K, typename V, std::enable_if_t<(std::is_same_v<K, int> || std::is_same_v<K, uint64_t>), int>>
    nlohmann::json serialize(std::map<K, V> &map)
    {
        // K not be number
        K v = "K type not shoud be number";
        return nlohmann::json();
    }

    template <typename K, typename V, std::enable_if_t<!(std::is_same_v<K, int> || std::is_same_v<K, uint64_t>), int>>
    nlohmann::json serialize(std::map<K, V> &map)
    {
        nlohmann::json root;
        for (auto &pair : map)
        {
            auto value = serialize(pair.second);
            root[pair.first] = value;
        }
        return root;
    }

    template <typename T, std::enable_if_t<is_reflect_struct_v<T>(false), int>>
    nlohmann::json serialize(T &object)
    {
        nlohmann::json root;
        reflect_trait<T>::for_each_members(object, [&](const char *key, auto &value) -> bool
                                           { 
                                               root[key] = serialize(value);
                                               return true; });
        return root;
    }

    template <typename T>
    bool deserialize(std::vector<T> &vec, const nlohmann::json &root, const std::string &key)
    {
        bool ret = true;
        if (!root.is_array())
        {
            XPJSON::make_result<std::vector<T>>(XPJSON::TYPE_ERROR, root.type_name(), key);
            return false;
        }
        for (const auto &item : root)
        {
            T value;
            ret = deserialize(value, item);
            if (!ret)
            {
                return ret;
            }
            vec.push_back(value);
        }
        return true;
    }

    template <typename T, std::enable_if_t<!is_reflect_struct_v<T>(false) && !is_optional_v<T>, int>>
    bool deserialize(T &object, const nlohmann::json &root, const std::string &key)
    {
        if (root.is_structured())
        {
            XPJSON::make_result<T>(XPJSON::TYPE_ERROR, root.type_name(), key);
            return false;
        }
        std::string error;
        if (const bool ret = field_type<T>::to_obj(root, object, error); !ret)
        {
            XPJSON::make_feild_error<T>(key, error);
            return false;
        }
        return true;
    }

    template <typename T, std::enable_if_t<is_reflect_struct_v<T>(false), int>>
    bool deserialize(T &object, const nlohmann::json &root, const std::string &keys)
    {
        if (!root.is_object())
        {
            XPJSON::make_result<T>(XPJSON::TYPE_ERROR, root.type_name(), keys);
            return false;
        }

        return reflect_trait<T>::for_each_members(object, [&](const char *key, auto &value) -> bool
                                                  {
            if(!root.contains(key)){
                // 如果是 optional 类型，设置为 nullopt
                using value_type = std::decay_t<decltype(value)>;
                if constexpr (is_optional_v<value_type>) {
                    value = std::nullopt;
                    return true;
                } else {
                    XPJSON::make_result<value_type>(XPJSON::NULL_VALUE, "", key);
                    return false;
                }
            }
            
            bool ret = deserialize(value, root[key], key);
            if(!ret){
                return ret;
            }
            return true; });
    }

    template <typename K, typename V>
    bool deserialize(std::map<K, V> &map, const nlohmann::json &root, const std::string &key)
    {
        if (!root.is_object())
        {
            XPJSON::make_result<std::map<K, V>>(XPJSON::TYPE_ERROR, root.type_name(), key);
            return false;
        }
        for (auto it = root.begin(); it != root.end(); ++it)
        {
            const auto &key = it.key();
            const auto &value = it.value();
            auto &re = map[key];
            bool ret = deserialize(re, value);
            if (!ret)
            {
                return ret;
            }
        }
        return true;
    }

    struct reflect_status
    {
        std::optional<std::string> error;
        bool ok = true;
    };

    template <typename T>
    reflect_status Deserialize(const nlohmann::json &json, T &object)
    {
        reflect_status ret;
        try
        {
            ret.ok = deserialize(object, json);
            if (!ret.ok)
            {
                ret.error = XPJSON::JSON_PASER_ERROR;
                return ret;
            }
        }
        catch (std::exception &e)
        {
            ret.ok = false;
            ret.error = e.what();
        }
        return ret;
    }

    template <typename T>
    reflect_status Serialize(const T &object, nlohmann::json &json)
    {
        reflect_status ret;
        try
        {
            json = serialize(const_cast<T &>(object));
        }
        catch (std::exception &e)
        {
            ret.ok = false;
            ret.error = e.what();
        }
        return ret;
    }

    // 工具函数：安全获取可选字段
    template <typename T>
    std::optional<T> get_optional(const nlohmann::json &json, const std::string &key)
    {
        if (!json.contains(key) || json[key].is_null())
        {
            return std::nullopt;
        }

        try
        {
            T value;
            reflect_status status = Deserialize(json[key], value);
            if (status.ok)
            {
                return value;
            }
            return std::nullopt;
        }
        catch (...)
        {
            return std::nullopt;
        }
    }

    // 工具函数：设置可选字段
    template <typename T>
    void set_optional(nlohmann::json &json, const std::string &key,
                      const std::optional<T> &value)
    {
        if (value.has_value())
        {
            nlohmann::json val_json;
            Serialize(value.value(), val_json);
            json[key] = val_json;
        }
        else
        {
            json[key] = nullptr;
        }
    }
};

#endif // REFLECT_STRUCT_H