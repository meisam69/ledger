#ifndef LIBFETCHCORE_SERVICE_SERVER_HPP
#define LIBFETCHCORE_SERVICE_SERVER_HPP
#include "service/server.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace fetch
{
namespace service
{

template< typename T >
void BuildServiceServer(std::string const &custom_name, pybind11::module &module) {

  namespace py = pybind11;
  py::class_<ServiceServer< T >, T, fetch::service::ServiceServerInterface>(module, custom_name )
    .def(py::init< uint16_t, typename fetch::service::ServiceServer<T>::thread_manager_ptr_type >())
    .def("ServiceInterfaceOf", &ServiceServer< T >::ServiceInterfaceOf);

}
};
};

#endif