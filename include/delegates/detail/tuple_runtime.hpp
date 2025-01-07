
#ifndef DELEGATES_TUPLE_RUNTIME_HEADER
#define DELEGATES_TUPLE_RUNTIME_HEADER

#include <tuple>
#include <type_traits>
#include <utility>
#include <stdexcept>

// std::tuple runtime tools
namespace delegates {
namespace tuple_runtime {
namespace detail {

template<
  typename Tuple,
  typename Indices=std::make_index_sequence<std::tuple_size<Tuple>::value> >
struct runtime_param_func_table;

// set value in tuple by element index from void* pointer to value
// type hash is used for check that element has the same type as user provided
template<size_t Idx, typename Tuple>
constexpr bool tuple_set_value_ptr_fn(Tuple& t, const void* pv, size_t type_hash)  {
  using elem_orig_type = typename std::tuple_element<Idx,Tuple>::type;
  using elem_type_noconst = typename std::decay< typename std::tuple_element<Idx,Tuple>::type >::type;
  using elem_type_noref = typename std::remove_reference< typename std::tuple_element<Idx,Tuple>::type >::type;

  static elem_type_noref default_val{}; // default value will be set on clear

  if (type_hash && pv
      && typeid(elem_orig_type).hash_code() != type_hash
      && typeid(elem_type_noref).hash_code() != type_hash
      && typeid(elem_type_noconst).hash_code() != type_hash) {
    return false;
  }

  const elem_type_noconst* p_input = reinterpret_cast<const elem_type_noconst*>(pv);
  elem_type_noconst& v = const_cast<elem_type_noconst&>( std::get<Idx>(t) );
  v = p_input ? *p_input : default_val;
  return true;
};

// get tuple element type hash by index. Type of element checked 'as is'
template<size_t Idx, typename Tuple>
constexpr size_t tuple_get_item_type_hash_fn(const Tuple& t)  {
  using elem_type=typename std::tuple_element<Idx,Tuple>::type;
  return typeid(elem_type).hash_code();
};

// get void* pointer tuple element by index
template<size_t Idx, typename Tuple>
constexpr void* tuple_get_item_value_ptr_fn(Tuple& t)  {
  using elem_type = typename std::tuple_element<Idx,Tuple>::type;
  auto& v = std::get<Idx>(t);
  return (void*)(&v);
};

template<typename Tuple,size_t ... Indices>
struct runtime_param_func_table<Tuple,std::index_sequence<Indices...> >{
  using set_ptr_func_ptr = bool(*)(Tuple&,const void*,size_t);
  using get_type_func_ptr = size_t(*)(const Tuple&);
  using get_ptr_func_ptr = void*(*)(Tuple&);

  static constexpr set_ptr_func_ptr set_table[std::tuple_size<Tuple>::value]={ &tuple_set_value_ptr_fn<Indices>... };
  static constexpr get_type_func_ptr get_type_table[std::tuple_size<Tuple>::value]={ &tuple_get_item_type_hash_fn<Indices>... };
  static constexpr get_ptr_func_ptr get_ptr_table[std::tuple_size<Tuple>::value]={ &tuple_get_item_value_ptr_fn<Indices>... };
};

template<typename Tuple,size_t ... Indices>
constexpr typename
  runtime_param_func_table<Tuple,std::index_sequence<Indices...>>::set_ptr_func_ptr
    runtime_param_func_table<Tuple,std::index_sequence<Indices...>>::set_table[std::tuple_size<Tuple>::value];

template<typename Tuple,size_t ... Indices>
constexpr typename
  runtime_param_func_table<Tuple,std::index_sequence<Indices...>>::get_type_func_ptr
    runtime_param_func_table<Tuple,std::index_sequence<Indices...>>::get_type_table[std::tuple_size<Tuple>::value];

template<typename Tuple,size_t ... Indices>
constexpr typename
  runtime_param_func_table<Tuple,std::index_sequence<Indices...>>::get_ptr_func_ptr
    runtime_param_func_table<Tuple,std::index_sequence<Indices...>>::get_ptr_table[std::tuple_size<Tuple>::value];

}//namespace detail

template<typename Tuple>
constexpr bool runtime_tuple_set_value_ptr(Tuple&& t,size_t index, const void* pv, size_t type_hash) {
  using tuple_type=typename std::remove_reference<Tuple>::type;
  if (index>=std::tuple_size<tuple_type>::value)
    throw std::runtime_error("Out of range");
  return detail::runtime_param_func_table<tuple_type>::set_table[index](t,pv,type_hash);
}

template<typename Tuple>
constexpr size_t runtime_tuple_get_element_type_hash(Tuple&& t,size_t index){
  using tuple_type=typename std::remove_reference<Tuple>::type;
  if (index>=std::tuple_size<tuple_type>::value)
    throw std::runtime_error("Out of range");
  return detail::runtime_param_func_table<tuple_type>::get_type_table[index](t);
}

template<typename Tuple>
constexpr void* runtime_tuple_get_value_ptr(Tuple&& t,size_t index){
  using tuple_type=typename std::remove_reference<Tuple>::type;
  if (index>=std::tuple_size<tuple_type>::value)
    throw std::runtime_error("Out of range");
  return detail::runtime_param_func_table<tuple_type>::get_ptr_table[index](t);
}

}//namespace tuple_runtime
}//namespace delegates

#endif //DELEGATES_TUPLE_RUNTIME_HEADER
