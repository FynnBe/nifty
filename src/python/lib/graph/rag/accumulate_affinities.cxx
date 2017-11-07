#include <cstddef>
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include "nifty/python/converter.hxx"


#include "nifty/graph/rag/grid_rag.hxx"
#include "nifty/graph/rag/feature_accumulation/grid_rag_affinity_features.hxx"


namespace py = pybind11;


namespace nifty{
namespace graph{

    using namespace py;

    // TODO first version....
    template<class RAG>
    void exportAccumulateAffinityFeaturesT(
        py::module & ragModule
    ){
        ragModule.def("computeCostsAndNhFromAffinities",
        [](
            const RAG & rag,
            nifty::marray::PyView<float> affinities,
            const std::vector<int> & ranges,
            const std::vector<int> & axes,
            const int numberOfThreads
        ){

            // TODO
            //{
            //    py::gil_scoped_release allowThreads;
            //}
            // FIXME this doesn't lift the gil
            LiftedNh<RAG> lnh(
                rag, ranges.begin(), ranges.end(), axes.begin(), numberOfThreads
            );

            uint64_t nLocal  = rag.edgeIdUpperBound() + 1;
            uint64_t nLifted = lnh.edgeIdUpperBound() + 1;
            marray::PyView<float> outLocal({nLocal, uint64_t(9)});
            marray::PyView<float> outLifted({nLifted, uint64_t(9)});
            {
                py::gil_scoped_release allowThreads;
                accumulateLongRangeAffinities(rag, lnh, affinities, 0., 1.,  outLocal, outLifted, numberOfThreads);
            }
            marray::PyView<uint32_t> lnhOut({nLifted, uint64_t(2)});
            {
                py::gil_scoped_release allowThreads;
                for(std::size_t e = 0; e < nLifted; ++e) {
                    lnhOut(e, 0) = lnh.u(e);
                    lnhOut(e, 1) = lnh.v(e);
                }
            }
            return std::make_tuple(lnhOut, outLocal, outLifted);
        },
        py::arg("rag"),
        py::arg("affinities"),
        py::arg("ranges"),
        py::arg("axes"),
        py::arg("numberOfThreads")= -1
        );

        ragModule.def("featuresFromLocalAffinities",
        [](
            const RAG & rag,
            nifty::marray::PyView<float> affinities,
            const int numberOfThreads
        ){

            uint64_t nEdges  = rag.edgeIdUpperBound() + 1;
            marray::PyView<float> out({nEdges, uint64_t(9)});
            {
                py::gil_scoped_release allowThreads;
                accumulateAffinities(rag, affinities, 0., 1.,  out, numberOfThreads);
            }
            return out;
        },
        py::arg("rag"),
        py::arg("affinities"),
        py::arg("numberOfThreads")= -1
        );
    }

    void exportAccumulateAffinityFeatures(py::module & ragModule) {
        typedef ExplicitLabelsGridRag<3, uint32_t> Rag3d;
        exportAccumulateAffinityFeaturesT<Rag3d>(ragModule);
    }

}
}
