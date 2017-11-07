#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include "nifty/python/converter.hxx"

#include "nifty/graph/rag/grid_rag_stacked_2d.hxx"

#ifdef WITH_HDF5
#include "nifty/graph/rag/grid_rag_stacked_2d_hdf5.hxx"
#include "nifty/graph/rag/grid_rag_labels_hdf5.hxx"
#endif

namespace py = pybind11;


namespace nifty{
namespace graph{



    using namespace py;

    template<class CLS, class BASE>
    void removeFunctions(py::class_<CLS, BASE > & clsT){
        clsT
            .def("insertEdge", [](CLS * self,const uint64_t u,const uint64_t ){
                throw std::runtime_error("cannot insert edges into 'GridRag'");
            })
            .def("insertEdges",[](CLS * self, py::array_t<uint64_t> pyArray) {
                throw std::runtime_error("cannot insert edges into 'GridRag'");
            })
        ;
    }

    template<class LABELS>
    void exportExplicitGridRagStacked2D(
        py::module & ragModule,
        const std::string & clsName,
        const std::string & facName
    ){
        py::object baseGraphPyCls = ragModule.attr("ExplicitLabelsGridRag3D");

        typedef ExplicitLabels<3, LABELS> LabelsProxyType;
        typedef GridRag<3, LabelsProxyType >  BaseGraph;
        typedef GridRagStacked2D<LabelsProxyType >  GridRagType;

        auto clsT = py::class_<GridRagType, BaseGraph>(ragModule, clsName.c_str());
        clsT
            .def("labelsProxy",&GridRagType::labelsProxy,py::return_value_policy::reference)
            .def_property_readonly("shape",[](const GridRagType & self){return self.shape();})
            .def("minMaxLabelPerSlice",[](const GridRagType & self){
                const auto & shape = self.shape();
                nifty::marray::PyView<uint64_t, 2> out({std::size_t(shape[0]),std::size_t(2)});
                for(auto sliceIndex = 0; sliceIndex<shape[0]; ++sliceIndex){
                    auto mima = self.minMaxNode(sliceIndex);
                    out(sliceIndex, 0) = mima.first;
                    out(sliceIndex, 1) = mima.second;
                }
                return out;
            })
            .def("numberOfNodesPerSlice",[](const GridRagType & self){
                const auto & shape = self.shape();
                nifty::marray::PyView<uint64_t,  1> out({std::size_t(shape[0])});
                for(auto sliceIndex = 0; sliceIndex<shape[0]; ++sliceIndex){
                    out(sliceIndex) =  self.numberOfNodes(sliceIndex);
                }
                return out;
            })
            .def("numberOfInSliceEdges",[](const GridRagType & self){
                const auto & shape = self.shape();
                nifty::marray::PyView<uint64_t,  1> out({std::size_t(shape[0])});
                for(auto sliceIndex = 0; sliceIndex<shape[0]; ++sliceIndex){
                    out(sliceIndex) =  self.numberOfInSliceEdges(sliceIndex);
                }
                return out;
            })
            .def("numberOfInBetweenSliceEdges",[](const GridRagType & self){
                const auto & shape = self.shape();
                nifty::marray::PyView<uint64_t,  1> out({std::size_t(shape[0])});
                for(auto sliceIndex = 0; sliceIndex<shape[0]; ++sliceIndex){
                    out(sliceIndex) =  self.numberOfInBetweenSliceEdges(sliceIndex);
                }
                return out;
            })
            .def("inSliceEdgeOffset",[](const GridRagType & self){
                const auto & shape = self.shape();
                nifty::marray::PyView<uint64_t,  1> out({std::size_t(shape[0])});
                for(auto sliceIndex = 0; sliceIndex<shape[0]; ++sliceIndex){
                    out(sliceIndex) =  self.inSliceEdgeOffset(sliceIndex);
                }
                return out;
            })
            .def("betweenSliceEdgeOffset",[](const GridRagType & self){
                const auto & shape = self.shape();
                nifty::marray::PyView<uint64_t,  1> out({std::size_t(shape[0])});
                for(auto sliceIndex = 0; sliceIndex<shape[0]; ++sliceIndex){
                    out(sliceIndex) =  self.betweenSliceEdgeOffset(sliceIndex);
                }
                return out;
            })
            .def_property_readonly("totalNumberOfInSliceEdges",[](const GridRagType & self){return self.numberOfInSliceEdges();})
            .def_property_readonly("totalNumberOfInBetweenSliceEdges",[](const GridRagType & self){return self.numberOfInBetweenSliceEdges();})
            .def("serialize",[](const GridRagType & self){
                nifty::marray::PyView<uint64_t> out({self.serializationSize()});
                auto ptr = &out(0);
                self.serialize(ptr);
                return out;
            })
            .def("deserialize",[](GridRagType & self, nifty::marray::PyView<uint64_t,1> serialization) {
                    auto  startPtr = &serialization(0);
                    auto  lastElement = &serialization(serialization.size()-1);
                    auto d = lastElement - startPtr + 1;
                    NIFTY_CHECK_OP(d,==,serialization.size(), "serialization must be contiguous");
                    self.deserialize(startPtr);
            })
            .def("edgeLengths",[](GridRagType & self) {
                nifty::marray::PyView<uint64_t,1> out({self.numberOfEdges()});
                const auto & edgeLens = self.edgeLengths();
                for(int edge = 0; edge < self.numberOfEdges(); ++edge)
                    out(edge) = edgeLens[edge];
                return out;
            })
        ;

        removeFunctions<GridRagType, BaseGraph>(clsT);

        // from labels
        ragModule.def(facName.c_str(),
            [](
               nifty::marray::PyView<LABELS, 3> labels,
               const int numberOfThreads
            ){
                auto s = typename  GridRagType::SettingsType();
                s.numberOfThreads = numberOfThreads;
                ExplicitLabels<3, LABELS> explicitLabels(labels);
                auto ptr = new GridRagType(explicitLabels, s);
                return ptr;
            },
            py::return_value_policy::take_ownership,
            py::keep_alive<0, 1>(),
            py::arg("labels"),
            py::arg_t< int >("numberOfThreads", -1 )
        );

        // from labels + initialization
        ragModule.def(facName.c_str(),
            [](
                nifty::marray::PyView<LABELS, 3> labels,
                nifty::marray::PyView<uint64_t,   1, false>  serialization
            ){
                auto  startPtr = &serialization(0);
                auto  lastElement = &serialization(serialization.size()-1);
                auto d = lastElement - startPtr + 1;
                NIFTY_CHECK_OP(d,==,serialization.size(), "serialization must be contiguous");

                auto s = typename  GridRagType::SettingsType();
                s.numberOfThreads = -1;
                ExplicitLabels<3, LABELS> explicitLabels(labels);
                auto ptr = new GridRagType(explicitLabels, startPtr, s);
                return ptr;
            },
            py::return_value_policy::take_ownership,
            py::keep_alive<0, 1>(),
            py::arg("labels"),
            py::arg_t< int >("numberOfThreads", -1 )
        );

    }

    #ifdef WITH_HDF5
    template<class LABELS>
    void exportHdf5GridRagStacked2D(
        py::module & ragModule,
        const std::string & clsName,
        const std::string & facName
    ){
        py::object baseGraphPyCls = ragModule.attr("GridRagHdf5Labels3D");

        typedef Hdf5Labels<3, LABELS> LabelsProxyType;
        typedef GridRag<3, LabelsProxyType >  BaseGraph;
        typedef GridRagStacked2D<LabelsProxyType >  GridRagType;

        auto clsT = py::class_<GridRagType, BaseGraph>(ragModule, clsName.c_str());
        clsT
            .def_property_readonly("shape",[](const GridRagType & self){return self.shape();})
            .def("labelsProxy",&GridRagType::labelsProxy,py::return_value_policy::reference)
            .def("minMaxLabelPerSlice",[](const GridRagType & self){
                const auto & shape = self.shape();
                nifty::marray::PyView<uint64_t, 2> out({std::size_t(shape[0]),std::size_t(2)});
                for(auto sliceIndex = 0; sliceIndex<shape[0]; ++sliceIndex){
                    auto mima = self.minMaxNode(sliceIndex);
                    out(sliceIndex, 0) = mima.first;
                    out(sliceIndex, 1) = mima.second;
                }
                return out;
            })
            .def("numberOfNodesPerSlice",[](const GridRagType & self){
                const auto & shape = self.shape();
                nifty::marray::PyView<uint64_t,  1> out({std::size_t(shape[0])});
                for(auto sliceIndex = 0; sliceIndex<shape[0]; ++sliceIndex){
                    out(sliceIndex) =  self.numberOfNodes(sliceIndex);
                }
                return out;
            })
            .def("numberOfInSliceEdges",[](const GridRagType & self){
                const auto & shape = self.shape();
                nifty::marray::PyView<uint64_t,  1> out({std::size_t(shape[0])});
                for(auto sliceIndex = 0; sliceIndex<shape[0]; ++sliceIndex){
                    out(sliceIndex) =  self.numberOfInSliceEdges(sliceIndex);
                }
                return out;
            })
            .def("numberOfInBetweenSliceEdges",[](const GridRagType & self){
                const auto & shape = self.shape();
                nifty::marray::PyView<uint64_t,  1> out({std::size_t(shape[0])});
                for(auto sliceIndex = 0; sliceIndex<shape[0]; ++sliceIndex){
                    out(sliceIndex) =  self.numberOfInBetweenSliceEdges(sliceIndex);
                }
                return out;
            })
            .def("inSliceEdgeOffset",[](const GridRagType & self){
                const auto & shape = self.shape();
                nifty::marray::PyView<uint64_t,  1> out({std::size_t(shape[0])});
                for(auto sliceIndex = 0; sliceIndex<shape[0]; ++sliceIndex){
                    out(sliceIndex) =  self.inSliceEdgeOffset(sliceIndex);
                }
                return out;
            })
            .def("betweenSliceEdgeOffset",[](const GridRagType & self){
                const auto & shape = self.shape();
                nifty::marray::PyView<uint64_t,  1> out({std::size_t(shape[0])});
                for(auto sliceIndex = 0; sliceIndex<shape[0]; ++sliceIndex){
                    out(sliceIndex) =  self.betweenSliceEdgeOffset(sliceIndex);
                }
                return out;
            })
            .def_property_readonly("totalNumberOfInSliceEdges",[](const GridRagType & self){return self.numberOfInSliceEdges();})
            .def_property_readonly("totalNumberOfInBetweenSliceEdges",[](const GridRagType & self){return self.numberOfInBetweenSliceEdges();})
            .def("serialize",
                [](const GridRagType & self) {
                    nifty::marray::PyView<uint64_t> out({self.serializationSize()});
                    auto ptr = &out(0);
                    self.serialize(ptr);
                    return out;
                }
            )
            .def("edgeLengths",[](GridRagType & self) {
                nifty::marray::PyView<uint64_t,1> out({self.numberOfEdges()});
                const auto & edgeLens = self.edgeLengths();
                for(int edge = 0; edge < self.numberOfEdges(); ++edge)
                    out(edge) = edgeLens[edge];
                return out;
            })
        ;

        removeFunctions<GridRagType, BaseGraph>(clsT);

        // init from labels
        ragModule.def(facName.c_str(),
            [](
                const LabelsProxyType & labelsProxy,
                const int numberOfThreads
            ){
                auto s = typename  GridRagType::SettingsType();
                s.numberOfThreads = numberOfThreads;

                auto ptr = new GridRagType(labelsProxy, s);
                return ptr;
            },
            py::return_value_policy::take_ownership,
            py::keep_alive<0, 1>(),
            py::arg("labelsProxy"),
            py::arg_t< int >("numberOfThreads", -1 )
        );

        // init from labels + serialization
        ragModule.def(facName.c_str(),
            [](
                const LabelsProxyType & labelsProxy,
                nifty::marray::PyView<uint64_t,   1, false>  serialization
            ){
                auto  startPtr = &serialization(0);
                auto  lastElement = &serialization(serialization.size()-1);
                auto d = lastElement - startPtr + 1;

                NIFTY_CHECK_OP(d,==,serialization.size(), "serialization must be contiguous");

                auto s = typename  GridRagType::SettingsType();
                s.numberOfThreads = -1;
                auto ptr = new GridRagType(labelsProxy, startPtr, s);
                return ptr;
            },
            py::return_value_policy::take_ownership,
            py::keep_alive<0, 1>(),
            py::arg("labelsProxy"),
            py::arg("serialization")
        );

    }
    #endif

    void exportGridRagStacked(py::module & ragModule) {
        exportExplicitGridRagStacked2D<uint32_t>(ragModule, "GridRagStacked2DExplicit", "gridRagStacked2DExplicitImpl");
        #ifdef WITH_HDF5
        exportHdf5GridRagStacked2D<uint32_t>(ragModule, "GridRagStacked2DHdf5", "gridRagStacked2DHdf5Impl");
        #endif
    }

} // end namespace graph
} // end namespace nifty
