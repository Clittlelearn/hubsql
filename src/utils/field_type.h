//
// Created by admin on 2025/10/31.
//

#ifndef JSONSER_FIELD_TYPE_H
#define JSONSER_FIELD_TYPE_H

#include <cstdint>
#include <exception>
#include <string>
#include <nlohmann/json.hpp>
template <typename T>
   class field_type {
public:
   template<typename JST>
   static void to_json(JST & json,const T & t) {
         json=t;
   }
   template<typename JST>
    static bool to_obj(const JST & json, T & obj,std::string & error) {
      try {
         obj= json.template get<T>();
         return true;
      }catch (std::exception &e) {
         error=e.what();
         return false;
      }
   }
};



//template <>
//class field_type<nlohmann::json> 
//{
//public:
//    static void to_json(nlohmann::json &json, const nlohmann::json &t) 
//   {
//      json = t;
//   }
//    static bool to_obj(const nlohmann::json &json, nlohmann::json &obj, std::string &error)
//   {
//       obj = json;
//       return true;
//   }
//};

//template<>
//class field_type<std::string>{
//public:
//    static void to_json(nlohmann::json &json, const std::string &t) 
//   {
//      json = t;
//   }
//    static bool to_obj(const nlohmann::json &json, std::string&obj, std::string &error)
//   {
//       obj = mm::hex::remove0xPrefix(json.get<std::string>());
//       return true;
//   }
//};
#endif //JSONSER_FIELD_TYPE_H