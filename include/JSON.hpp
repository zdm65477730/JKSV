#pragma once
#include <json-c/json.h>
#include <memory>

namespace json
{
    // Use this instead of default json_object
    using Object = std::unique_ptr<json_object, decltype(&json_object_put)>;

    // Use this instead of json_object_from_x. Pass the function and its arguments instead.
    template <typename... Args>
    static inline json::Object new_object(json_object *(*function)(Args...), Args... args)
    {
        return json::Object((*function)(args...), json_object_put);
    }
} // namespace json
