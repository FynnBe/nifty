#pragma once


#include "nifty/graph/rag/grid_rag.hxx"
#include "nifty/marray/marray.hxx"

#include "nifty/tools/for_each_coordinate.hxx"

namespace nifty{
namespace graph{


    template<std::size_t DIM, class LABELS_TYPE, class LABELS, class NODE_MAP>
    void gridRagAccumulateLabels(
        const ExplicitLabelsGridRag<DIM, LABELS_TYPE> & graph,
        nifty::marray::View<LABELS> data,
        NODE_MAP &  nodeMap,
        const bool ignoreBackground = false,
        const LABELS ignoreValue = 0
    ){
        typedef std::array<int64_t, DIM> Coord;

        const auto labelsProxy = graph.labelsProxy();
        const auto & shape = labelsProxy.shape();
        const auto labels = labelsProxy.labels();

        std::vector<  std::unordered_map<LABELS, std::size_t> > overlaps(graph.numberOfNodes());



        nifty::tools::forEachCoordinate(shape,[&](const Coord & coord){
            const auto node = labels(coord);
            const auto l  = data(coord);
            overlaps[node][l] += 1;
        });

        for(const auto node : graph.nodes()){
            const auto & ol = overlaps[node];
            // find max ol
            std::size_t maxOl = 0;
            LABELS maxOlLabel = 0;
            for(auto kv : ol){
                if(kv.second > maxOl){
                    if(ignoreBackground && kv.first == ignoreValue) {
                        continue;
                    }
                    maxOl = kv.second;
                    maxOlLabel = kv.first;
                }
            }
            nodeMap[node] = maxOlLabel;
        }
    }


} // end namespace graph
} // end namespace nifty


