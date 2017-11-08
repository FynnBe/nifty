#pragma once

#include <algorithm>

#include "nifty/graph/rag/grid_rag_stacked_2d.hxx"

#ifdef WITH_HDF5
#include "nifty/hdf5/hdf5_array.hxx"
#include "nifty/graph/rag/grid_rag_stacked_2d_hdf5.hxx"
#endif

#include "nifty/marray/marray.hxx"
#include "nifty/tools/for_each_coordinate.hxx"
#include "nifty/array/arithmetic_array.hxx"


namespace nifty{
namespace graph{

template<
    class LABELS_PROXY,
    class PIXEL_ARRAY,
    class NODE_MAP
>
void projectScalarNodeDataToPixels(
    const GridRagStacked2D<LABELS_PROXY> & graph,
    NODE_MAP & nodeData, // why is this not const?
    PIXEL_ARRAY & pixelData,
    const int numberOfThreads = -1
){
    typedef array::StaticArray<int64_t, 3> Coord;
    typedef array::StaticArray<int64_t, 2> Coord2;

    const auto & labelsProxy = graph.labelsProxy();
    const auto & shape = labelsProxy.shape();

    typedef LABELS_PROXY LabelsProxyType;
    typedef typename LABELS_PROXY::LabelType LabelType;
    typedef typename LabelsProxyType::BlockStorageType LabelsBlockStorage;
    typedef typename tools::BlockStorageSelector<PIXEL_ARRAY>::type DataBlockStorage;

    nifty::parallel::ParallelOptions pOpts(numberOfThreads);
    nifty::parallel::ThreadPool threadpool(pOpts);
    const auto nThreads = pOpts.getActualNumThreads();

    uint64_t numberOfSlices = shape[0];
    Coord2 sliceShape2({shape[1], shape[2]});
    Coord sliceShape3({int64_t(1),shape[1], shape[2]});

    LabelsBlockStorage sliceLabelsStorage(threadpool, sliceShape3, nThreads);
    DataBlockStorage   sliceDataStorage(threadpool, sliceShape3, nThreads);

    parallel::parallel_foreach(threadpool, numberOfSlices, [&](const int tid, const int64_t sliceIndex){

        // fetch the data for the slice
        auto sliceLabelsFlat3DView = sliceLabelsStorage.getView(tid);
        auto sliceDataFlat3DView   = sliceDataStorage.getView(tid);

        const Coord blockBegin({sliceIndex,int64_t(0),int64_t(0)});
        const Coord blockEnd({sliceIndex+1, sliceShape2[0], sliceShape2[1]});

        tools::readSubarray(labelsProxy, blockBegin, blockEnd, sliceLabelsFlat3DView);

        auto sliceLabels = sliceLabelsFlat3DView.squeezedView();
        auto sliceData = sliceDataFlat3DView.squeezedView();

        nifty::tools::forEachCoordinate(sliceShape2,[&](const Coord2 & coord){
            const auto node = sliceLabels( coord.asStdArray() );
            sliceData(coord.asStdArray()) = nodeData[node];
        });

        tools::writeSubarray(pixelData, blockBegin, blockEnd, sliceDataFlat3DView);

    });
}


template<
    class LABELS_PROXY,
    class PIXEL_ARRAY,
    class NODE_MAP,
    class COORD
>
void projectScalarNodeDataInSubBlock(
    const GridRagStacked2D<LABELS_PROXY> & graph,
    NODE_MAP & nodeData, // why is this not const?
    PIXEL_ARRAY & pixelData,
    const COORD & blockBegin,
    const COORD & blockEnd,
    const int numberOfThreads = -1
){
    typedef array::StaticArray<int64_t, 3> Coord;
    typedef array::StaticArray<int64_t, 2> Coord2;

    const auto & labelsProxy = graph.labelsProxy();
    const auto & shape = labelsProxy.shape();

    typedef LABELS_PROXY LabelsProxyType;
    typedef typename LABELS_PROXY::LabelType LabelType;
    typedef typename LabelsProxyType::BlockStorageType LabelsBlockStorage;
    typedef typename tools::BlockStorageSelector<PIXEL_ARRAY>::type DataBlockStorage;

    nifty::parallel::ParallelOptions pOpts(numberOfThreads);
    nifty::parallel::ThreadPool threadpool(pOpts);
    const auto nThreads = pOpts.getActualNumThreads();

    uint64_t numberOfSlices = blockEnd[0] - blockBegin[0];
    Coord2 sliceShape2({blockEnd[1] - blockBegin[1], blockEnd[2] - blockBegin[2]});
    Coord  sliceShape3({int64_t(1), blockEnd[1] - blockBegin[1], blockEnd[2] - blockBegin[2]});

    LabelsBlockStorage sliceLabelsStorage(threadpool, sliceShape3, nThreads);
    DataBlockStorage   sliceDataStorage(threadpool, sliceShape3, nThreads);

    parallel::parallel_foreach(threadpool, numberOfSlices, [&](const int tid, const int64_t sliceIndex){

        auto globalSliceIndex = sliceIndex + blockBegin[0];
        // fetch the data for the slice
        auto sliceLabelsFlat3DView = sliceLabelsStorage.getView(tid);
        auto sliceDataFlat3DView   = sliceDataStorage.getView(tid);

        const Coord globalBegin({globalSliceIndex, blockBegin[1], blockBegin[2]});
        const Coord globalEnd(  {globalSliceIndex+1, blockEnd[1], blockEnd[2]});

        tools::readSubarray(labelsProxy, globalBegin, globalEnd, sliceLabelsFlat3DView);

        auto sliceLabels = sliceLabelsFlat3DView.squeezedView();
        auto sliceData = sliceDataFlat3DView.squeezedView();

        nifty::tools::forEachCoordinate(sliceShape2,[&](const Coord2 & coord){
            const auto node = sliceLabels(coord.asStdArray());
            sliceData(coord.asStdArray()) = nodeData[node];
        });

        const Coord localBegin({sliceIndex, int64_t(0), int64_t(0)});
        const Coord localEnd({sliceIndex+1, sliceShape3[1], sliceShape3[2]});

        tools::writeSubarray(pixelData, localBegin, localEnd, sliceDataFlat3DView);

    });

}

} // namespace graph
} // namespace nifty
