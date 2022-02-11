/**
 * @file SerializerRegistry.hpp
 */

#ifndef RUBBERDAQ_INCLUDE_SERIALIZERREGISTRY_HPP_
#define RUBBERDAQ_INCLUDE_SERIALIZERREGISTRY_HPP_

#include "rubberdaq/GenericCallback.hpp"

#include <map>
#include <functional>
#include <typeinfo>
#include <typeindex>

namespace dunedaq {
namespace rubberdaq {

class SerializerRegistry {
public: 

  SerializerRegistry(){}
  SerializerRegistry(const SerializerRegistry&) = delete;            ///< SerializerRegistry is not copy-constructible
  SerializerRegistry& operator=(const SerializerRegistry&) = delete; ///< SerializerRegistry is not copy-assignable
  SerializerRegistry(SerializerRegistry&&) = delete;                 ///< SerializerRegistry is not move-constructible
  SerializerRegistry& operator=(SerializerRegistry&&) = delete;      ///< SerializerRegistry is not move-assignable

  template<typename Datatype, typename Function>
  void register_serializer(Function&& f) {
    m_serializers[std::type_index(typeid(Datatype))] = f;
  }

  template<typename Datatype, typename Function>
  void register_deserializer(Function&& f) {
    m_deserializers[std::type_index(typeid(Datatype))] = f;
  }

  template<typename Datatype>
  GenericCallback& get_serializer(){
    auto tidx = std::type_index(typeid(Datatype));
    if (!m_serializers.count(tidx)) {
      // throw error
    }
    return m_serializers[tidx];
  }

  template<typename Datatype>
  GenericCallback& get_deserializer(){
    auto tidx = std::type_index(typeid(Datatype));
    if (!m_deserializers.count(tidx)) {
      // throw error
    }
    return m_deserializers[tidx];
  }

private:
  std::map<std::type_index, GenericCallback> m_serializers;
  std::map<std::type_index, GenericCallback> m_deserializers;

};

} // namespace rubberdaq
} // namespace dunedaq

#endif // RUBBERDAQ_INCLUDE_SERIALIZERREGISTRY_HPP_
