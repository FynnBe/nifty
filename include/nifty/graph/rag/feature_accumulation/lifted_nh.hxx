#pragma once

#include "nifty/parallel/threadpool.hxx"
#include "nifty/tools/array_tools.hxx"
#include "nifty/graph/undirected_list_graph.hxx"

namespace nifty {
namespace graph {

template<class RAG>
class LiftedNh : public UndirectedGraph<>{

// TODO
// - make ready for 2d stacked
// - out of core
public:
    typedef RAG Rag;
    typedef UndirectedGraph<> BaseType;

    typedef array::StaticArray<int64_t, 4> Coord4;
    typedef array::StaticArray<int64_t, 3> Coord3;
    typedef array::StaticArray<int64_t, 2> Coord2;

    // compute lifted nh from affinities
    template<typename ITER>
    LiftedNh(
        const RAG & rag,
        ITER rangeIterBegin,
        ITER rangeIterEnd,
        ITER axesIterBegin,
        const int numberOfThreads=-1
    ) : ranges_(rangeIterBegin, rangeIterEnd),
        axes_(axesIterBegin, axesIterBegin + std::distance(rangeIterBegin, rangeIterEnd))
    {
        initLiftedNh(rag, numberOfThreads);
    }

    const std::vector<int> & ranges() const {return ranges_;}
    const std::vector<int> & axes()  const {return axes_;}
    const std::vector<int> & lrIndices() const {return lrIndices_;}

private:
    void initLiftedNh(
        const Rag & labels, const int numberOfThreads);

    std::vector<int> ranges_;
    std::vector<int> axes_;
    std::vector<std::size_t> lrIndices_;
};


// TODO use block storage mechanism to make out of core
template<class RAG>
void LiftedNh<RAG>::initLiftedNh(
    const RAG & rag, const int numberOfThreads
) {

    //typedef tools::BlockStorage<LabelType> LabelStorage;
    const auto & labels = rag.labelsProxy().labels();

    // set the number of nodes in the graph == number of labels
    BaseType::assign(rag.labelsProxy().numberOfLabels());
    Coord3 shape;
    for(std::size_t d = 0; d < 3; ++d) {
        shape[d] = labels.shape(d);
    }

    // TODO parallelize properly
    // threadpool and actual number of threads
    //nifty::parallel::ThreadPool threadpool(numberOfThreads);
    //const std::size_t nThreads = threadpool.nThreads();

    //
    // get the lr edges (== ranges with abs value bigger than 1)
    //

    for(std::size_t ii = 0; ii < ranges_.size(); ++ii) {
        if(std::abs(ranges_[ii]) > 1) {
            lrIndices_.push_back(ii);
        }
    }

    // number of links = number of lr channels * number of pixels
    std::size_t nLinks = lrIndices_.size() * labels.size();

    // FIXME super dirty hack to get the index to offsets translator from marray
    Coord4 affShape;
    affShape[0] = lrIndices_.size();
    for(std::size_t d = 0; d < 3; ++d) {
        affShape[d+1] = shape[d];
    }

    // FIXME skip init
    //marray::Marray<int8_t> fakeAffinities(marray::InitializationSkipping, affShape.begin(), affShape.end());
    marray::Marray<int8_t> fakeAffinities(affShape.begin(), affShape.end());

    //
    // iterate over the links and insert the corresponding uv pairs into the NH
    //
    Coord4 affCoord;
    Coord3 cU, cV;
    int axis, range, lrId;
    for(std::size_t linkId = 0; linkId < nLinks; ++linkId) {
        fakeAffinities.indexToCoordinates(linkId, affCoord.begin());
        lrId = lrIndices_[affCoord[0]];
        axis  = axes_[lrId];
        range = ranges_[lrId];

        for(std::size_t d = 0; d < 3; ++d) {
            cU[d] = affCoord[d+1];
            cV[d] = affCoord[d+1];
        }
        cV[axis] += range;
        // range check
        if(cV[axis] >= shape[axis] || cV[axis] < 0) {
            continue;
        }
        auto u = labels(cU.asStdArray());
        auto v = labels(cV.asStdArray());

        // only do stuff if the labels are different
        if(u != v) {

            // only add an edge to the lifted nh if it is not
            // in the local one
            if(rag.findEdge(u, v) != -1) {
                continue;
            }

            BaseType::insertEdge(
                std::min(u, v), std::max(u, v)
            );
        }
    }
}


}
}
