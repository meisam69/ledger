#ifndef SERVICE_ABSTRACT_CALLABLE_HPP
#define SERVICE_ABSTRACT_CALLABLE_HPP
#include "service/types.hpp"
#include"logger.hpp"
namespace fetch {
namespace service {

namespace details {
/* Argument packing routines for callables.
 * @T is the type of the argument that will be packed next.
 * @arguments are the arguments that remains to be pack
 *
 * This struct contains packer structs that takes function arguments and
 * serializes them into a referenced byte array. The content of this
 * struct belongs to the implementational details and are hence not
 * exposed directly to the developer.
 */
template <typename T, typename... arguments>  
struct Packer {
  /* Implementation of the serialization.
   *
   * @serializer is a reference to the serializer.
   *  @next is the next argument that will be fed into the serializer.
   */
  static void SerializeArguments(serializer_type &serializer,
                                 T &next, arguments... args) {
    serializer << next;
    Packer<arguments... >::SerializeArguments(serializer, args...);
  }
};

/* Termination of the argument packing routines.
 *
 * This specialisation is invoked when only one argument is left.
 */  
template < typename T >  
struct Packer<T> {
  /* Implementation of the serialization.
   *
   * @serializer is a reference to the serializer.
   * @last is the last argument that will be fed into the serializer.
   */  
  static void SerializeArguments(serializer_type &serializer,
                                 T &last) {
    serializer << last;
    serializer.Seek(0);
  }
};  
};


/* This function packs a function call into byte array.
 * @arguments are the argument types of args.
 * @serializer is the serializer to which the arguments will be packed.
 * @protocol is the protocol the call belongs to.
 * @function is the function that is being called.
 * @args are the arguments to the function.
 *
 * For this function to work, it is a requirement that there exists a
 * serilization implementation for all argument types in the argument
 * list. 
 *
 * The serializer is is always left at position 0.
 */
template <typename... arguments>
void PackCall(serializer_type &serializer,
              protocol_handler_type const& protocol,
              function_handler_type const& function, arguments... args) {
  LOG_STACK_TRACE_POINT;
  
  serializer << protocol;
  serializer << function;
  details::Packer<arguments... >::SerializeArguments(serializer, args...);
}


/* This function is the no-argument packer.
 * @serializer is the serializer to which the arguments will be packed.
 * @protocol is the protocol the call belongs to.
 * @function is the function that is being called.
 *
 * This function covers the case where no arguments are given. The
 * serializer is is always left at position 0.
 */  
void PackCall(serializer_type &serializer,
              protocol_handler_type const& protocol,
              function_handler_type const& function) {
  LOG_STACK_TRACE_POINT;
  
  serializer << protocol;
  serializer << function;
  serializer.Seek(0);
}

/* Function that packs arguments to serializer. 
 * @serializer is the serializer to which the arguments will be packed.
 * @args are the arguments to the function.
 *
 * Serializers for all arguments in the argument list are requried.
 */    
template <typename... arguments>
void PackArgs(serializer_type &serializer, arguments... args) {
  details::Packer<arguments... >::SerializeArguments(serializer, args...);
}

/* This is the no-argument packer.
 * @serializer is the serializer to which the arguments will be packed.
 *
 * This function covers the case where no arguments are given. The
 * serializer is is always left at position 0.
 */    
void PackArgs(serializer_type &serializer) {
  LOG_STACK_TRACE_POINT;
  
  serializer.Seek(0);
}

enum Callable {
  CLIENT_ID_ARG = 1
};
  
/* Abstract class for callables.
 *
 * This class defines but a single virtual operator.   
 */  
class AbstractCallable {
 public:
  AbstractCallable(uint64_t meta_data = 0) : meta_data_(meta_data) 
  {

  }
  
  virtual ~AbstractCallable(){};
  
  /* Call operator that impolements deserialization and invocation of function.
   * @result is a serializer used to serialize the result.
   * @params is a serializer that is used to deserialize the arguments.
   */
  virtual void operator()(serializer_type &result, serializer_type &params) = 0;

  uint64_t const & meta_data() const 
  {
    return meta_data_;
  }
  
private:
  uint64_t meta_data_ = 0;  
};
};
};

#endif