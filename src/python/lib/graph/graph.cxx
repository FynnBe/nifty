#include <pybind11/pybind11.h>
#include <iostream>

namespace py = pybind11;


namespace nifty{
namespace graph{

    void initSubmoduleGraph(py::module &niftyModule) {
        auto graphModule = niftyModule.def_submodule("graph","graph submodule");
    }

}
}