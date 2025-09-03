
#ifndef DELEGATES_TUPLE_RUNTIME_HEADER
#define DELEGATES_TUPLE_RUNTIME_HEADER

#include "../delegates_conf.h"

#include <tuple>
#include <type_traits>
#include <utility>
#include <stdexcept>

DELEGATES_BASE_NAMESPACE_BEGIN

// std::tuple runtime tools
namespace delegates {
namespace tuple_runtime {
namespace detail {

template<
  typename Tuple,
  typename Indices=std::make_index_sequence<std::tuple_size<Tuple>::value> >
struct runtime_param_func_table;

template<
  typename Tuple,
  typename TDefaultValTuple,
  typename Indices = std::make_index_sequence<std::tuple_size<Tuple>::value> >
struct runtime_param_set_func_table;

// set value in tuple by element index from void* pointer to value. Or default value from another tuple
// type hash is used for check that element has the same type as user provided
template<size_t Idx, typename Tuple, typename TDefaultValTuple>
constexpr bool tuple_set_value_or_default_ptr_fn(Tuple& tup, TDefaultValTuple& tup_def_values, const void* pv, size_t type_hash)  {
  using elem_orig_type = typename std::tuple_element<Idx,Tuple>::type;
  using elem_type_noconst = typename std::decay< typename std::tuple_element<Idx,Tuple>::type >::type;
  using elem_type_noref = typename std::remove_reference< typename std::tuple_element<Idx,Tuple>::type >::type;

  if (type_hash && pv
      && typeid(elem_orig_type).hash_code() != type_hash
      && typeid(elem_type_noref).hash_code() != type_hash
      && typeid(elem_type_noconst).hash_code() != type_hash) {
    return false;
  }

  const elem_type_noconst* p_input = reinterpret_cast<const elem_type_noconst*>(pv);
  elem_type_noconst& v = const_cast<elem_type_noconst&>( std::get<Idx>(tup) );
  elem_type_noconst& default_v = std::get<Idx>(tup_def_values);
  v = p_input ? *p_input : default_v;
  return true;
};

// get tuple element type hash by index. Type of element checked 'as is'
template<size_t Idx, typename Tuple>
constexpr size_t tuple_get_item_type_hash_fn(const Tuple& tup)  {
  (void)tup;
  using elem_type=typename std::tuple_element<Idx,Tuple>::type;
  return typeid(elem_type).hash_code();
};

// get void* pointer tuple element by index
template<size_t Idx, typename Tuple>
constexpr void* tuple_get_item_value_ptr_fn(Tuple& tup)  {
  using elem_type = typename std::tuple_element<Idx,Tuple>::type;
  auto& v = std::get<Idx>(tup);
  return (void*)(&v);
};

template<typename Tuple, typename TDefaultValTuple, size_t ... Indices>
struct runtime_param_set_func_table<Tuple, TDefaultValTuple, std::index_sequence<Indices...> > {
  using set_ptr_func_ptr = bool(*)(Tuple&, TDefaultValTuple&, const void*, size_t);
  static constexpr set_ptr_func_ptr set_table[std::tuple_size<Tuple>::value] = { &tuple_set_value_or_default_ptr_fn<Indices>... };
};

template<typename Tuple, size_t ... Indices>
struct runtime_param_func_table<Tuple,std::index_sequence<Indices...> >{
  using get_type_func_ptr = size_t(*)(const Tuple&);
  using get_ptr_func_ptr = void*(*)(Tuple&);

  static constexpr get_type_func_ptr get_type_table[std::tuple_size<Tuple>::value]={ &tuple_get_item_type_hash_fn<Indices>... };
  static constexpr get_ptr_func_ptr get_ptr_table[std::tuple_size<Tuple>::value]={ &tuple_get_item_value_ptr_fn<Indices>... };
};

template<typename Tuple, typename TDefaultValTuple, size_t ... Indices>
constexpr typename
  runtime_param_set_func_table<Tuple, TDefaultValTuple, std::index_sequence<Indices...> >::set_ptr_func_ptr
    runtime_param_set_func_table<Tuple, TDefaultValTuple, std::index_sequence<Indices...> >::set_table[std::tuple_size<Tuple>::value];

template<typename Tuple,size_t ... Indices>
constexpr typename
  runtime_param_func_table<Tuple,std::index_sequence<Indices...>>::get_type_func_ptr
    runtime_param_func_table<Tuple,std::index_sequence<Indices...>>::get_type_table[std::tuple_size<Tuple>::value];

template<typename Tuple,size_t ... Indices>
constexpr typename
  runtime_param_func_table<Tuple,std::index_sequence<Indices...>>::get_ptr_func_ptr
    runtime_param_func_table<Tuple,std::index_sequence<Indices...>>::get_ptr_table[std::tuple_size<Tuple>::value];

}//namespace detail

template<typename Tuple, typename TDefaultValTuple>
constexpr bool runtime_tuple_set_value_ptr(Tuple&& tup, TDefaultValTuple&& tdefault, size_t index, const void* pv, size_t type_hash) {
  using tuple_type=typename std::remove_reference<Tuple>::type;
  using default_val_tuple_type = typename std::remove_reference<TDefaultValTuple>::type;

  if (index>=std::tuple_size<tuple_type>::value)
    throw std::runtime_error("Out of range");
  return detail::runtime_param_set_func_table<tuple_type, default_val_tuple_type>::set_table[index](tup,tdefault,pv,type_hash);
}

template<typename Tuple>
constexpr size_t runtime_tuple_get_element_type_hash(Tuple&& tup,size_t index){
  using tuple_type=typename std::remove_reference<Tuple>::type;
  if (index>=std::tuple_size<tuple_type>::value)
    throw std::runtime_error("Out of range");
  return detail::runtime_param_func_table<tuple_type>::get_type_table[index](tup);
}

template<typename Tuple>
constexpr void* runtime_tuple_get_value_ptr(Tuple&& tup,size_t index){
  using tuple_type=typename std::remove_reference<Tuple>::type;
  if (index>=std::tuple_size<tuple_type>::value)
    throw std::runtime_error("Out of range");
  return detail::runtime_param_func_table<tuple_type>::get_ptr_table[index](tup);
}


template <std::size_t... Is, typename Tuple>
auto ref_tuple_impl(std::index_sequence<Is...>, Tuple& tup)
-> std::tuple<std::reference_wrapper<typename std::tuple_element<Is, Tuple>::type>...> {
  return std::make_tuple(std::ref(std::get<Is>(tup))...);
}

// create tuple of references and refs existing tuple values
// tuple<int,float>   ->   tuple<int&,float&>, pointed to first one
template <typename Tuple>
auto ref_tuple(Tuple& tup)
-> decltype(ref_tuple_impl(std::make_index_sequence<std::tuple_size<Tuple>::value >{}, tup)) {
  return ref_tuple_impl(typename std::make_index_sequence<std::tuple_size<Tuple>::value>{}, tup);
}

}//namespace tuple_runtime
}//namespace delegates

DELEGATES_BASE_NAMESPACE_END

#endif //DELEGATES_TUPLE_RUNTIME_HEADER
