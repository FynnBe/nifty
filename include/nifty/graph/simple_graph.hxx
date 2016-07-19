#pragma once
#ifndef NIFTY_GRAPH_SIMPLE_GRAPH_HXX
#define NIFTY_GRAPH_SIMPLE_GRAPH_HXX

#include <vector>
#include <boost/version.hpp>

// for strange reason travis does not find the boost flat set
#ifdef WITHIN_TRAVIS
#include <set>
#define __setimpl std::set
#else
#include <boost/container/flat_set.hpp>
#define __setimpl boost::container::flat_set
#endif


#include <boost/iterator/counting_iterator.hpp>

#include "nifty/container/flat_set.hxx"
#include "nifty/tools/runtime_check.hxx"
#include "nifty/graph/undirected_graph_base.hxx"
#include "nifty/graph/detail/adjacency.hxx"
#include "nifty/graph/graph_tags.hxx"

namespace nifty{
namespace graph{




namespace detail_graph{
    template<class EDGE_INTERNAL_TYPE, class NODE_INTERNAL_TYPE >
    struct UndirectedGraphTypeHelper{
        typedef EDGE_INTERNAL_TYPE EdgeInternalType;
        typedef NODE_INTERNAL_TYPE NodeInternalType;
        typedef detail_graph::UndirectedAdjacency<int64_t,int64_t,NodeInternalType,EdgeInternalType> NodeAdjacency;
        //typedef std::set<NodeAdjacency > NodeStorage;
        typedef nifty::container::FlatSet <NodeAdjacency> NodeStorage;

        typedef std::pair<NodeInternalType,NodeInternalType> EdgeStorage;
        typedef boost::counting_iterator<int64_t> NodeIter;
        typedef boost::counting_iterator<int64_t> EdgeIter;
        typedef typename NodeStorage::const_iterator AdjacencyIter;
    };



    class SimpleGraphNodeIter : public boost::counting_iterator<int64_t>{
        using boost::counting_iterator<int64_t>::counting_iterator;
        using boost::counting_iterator<int64_t>::operator=;
    };

    class SimpleGraphEdgeIter : public boost::counting_iterator<int64_t>{
        using boost::counting_iterator<int64_t>::counting_iterator;
        using boost::counting_iterator<int64_t>::operator=;
    };
};


template<class EDGE_INTERNAL_TYPE = int64_t, 
         class NODE_INTERNAL_TYPE = int64_t>
class UndirectedGraph : public
    UndirectedGraphBase<
        UndirectedGraph<EDGE_INTERNAL_TYPE,NODE_INTERNAL_TYPE>,
        detail_graph::SimpleGraphNodeIter,
        detail_graph::SimpleGraphEdgeIter,
        typename detail_graph::UndirectedGraphTypeHelper<EDGE_INTERNAL_TYPE,NODE_INTERNAL_TYPE>::AdjacencyIter
    >
{
protected:
    typedef EDGE_INTERNAL_TYPE EdgeInternalType;
    typedef NODE_INTERNAL_TYPE NodeInternalType;
    typedef detail_graph::UndirectedAdjacency<int64_t,int64_t,NodeInternalType,EdgeInternalType> NodeAdjacency;
    typedef nifty::container::FlatSet<NodeAdjacency> NodeStorage;
    typedef std::pair<EdgeInternalType,EdgeInternalType> EdgeStorage;
public:
    typedef detail_graph::SimpleGraphNodeIter NodeIter;
    typedef boost::counting_iterator<int64_t> EdgeIter;
    typedef typename NodeStorage::const_iterator AdjacencyIter;


    typedef ContiguousTag EdgeIdTag;
    typedef ContiguousTag NodeIdTag;

    typedef SortedTag EdgeIdOrderTag;
    typedef SortedTag NodeIdOrderTag;


    // constructors
    UndirectedGraph(const uint64_t numberOfNodes = 0, const uint64_t reserveNumberOfEdges = 0)
    :   nodes_(numberOfNodes),
        edges_()
    {
        edges_.reserve(reserveNumberOfEdges);
    }

    void assign(const uint64_t numberOfNodes = 0, const uint64_t reserveNumberOfEdges = 0){
        nodes_.clear();
        edges_.clear();
        nodes_.resize(numberOfNodes);
        edges_.reserve(reserveNumberOfEdges);
    }

    int64_t insertEdge(const int64_t u, const int64_t v){
   
        const auto fres =  nodes_[u].find(NodeAdjacency(v));
        if(fres != nodes_[u].end())
            return fres->edge();
        else{
            const auto uu = std::min(u,v);
            const auto vv = std::max(u,v);
            auto e = EdgeStorage(uu, vv);
            auto ei = edges_.size();
            edges_.push_back(e);
            nodes_[u].insert(NodeAdjacency(v,ei));
            nodes_[v].insert(NodeAdjacency(u,ei));
            return ei;
        }
    }



    // MUST IMPL INTERFACE
    int64_t u(const int64_t e)const{
        NIFTY_ASSERT_OP(e,<,numberOfEdges());
        return edges_[e].first;
    }
    int64_t v(const int64_t e)const{
        NIFTY_ASSERT_OP(e,<,numberOfEdges());
        return edges_[e].second;
    }

    int64_t findEdge(const int64_t u, const int64_t v)const{
        NIFTY_ASSERT_OP(u,<,numberOfNodes());
        NIFTY_ASSERT_OP(v,<,numberOfNodes());
        const auto fres =  nodes_[u].find(NodeAdjacency(v));
        if(fres != nodes_[u].end())
            return fres->edge();
        else
            return -1;
    }
    uint64_t nodeIdUpperBound() const{return numberOfNodes() == 0 ? -1 : numberOfNodes()-1;}
    uint64_t edgeIdUpperBound() const{return numberOfEdges() == 0 ? -1 : numberOfEdges()-1;}
    uint64_t numberOfEdges() const {return edges_.size();}
    uint64_t numberOfNodes() const{return nodes_.size();}

    NodeIter nodesBegin()const{return NodeIter(0);}
    NodeIter nodesEnd()const{return NodeIter(this->numberOfNodes());}
    EdgeIter edgesBegin()const{return EdgeIter(0);}
    EdgeIter edgesEnd()const{return EdgeIter(this->numberOfEdges());}

    AdjacencyIter adjacencyBegin(const int64_t node)const{
        NIFTY_ASSERT_OP(node,<,numberOfNodes());
        return nodes_[node].begin();
    }
    AdjacencyIter adjacencyEnd(const int64_t node)const{
        NIFTY_ASSERT_OP(node,<,numberOfNodes());
        return nodes_[node].end();
    }


    AdjacencyIter adjacencyOutBegin(const int64_t node)const{
        return adjacencyBegin(node);
    }

    // optional (with default impl in base)
    std::pair<int64_t,int64_t> uv(const int64_t e)const{
        const auto _uv = edges_[e];
        return std::pair<int64_t,int64_t>(_uv.first, _uv.second);
    }
    template<class F>
    void forEachEdge(F && f)const{
        for(uint64_t edge=0; edge< numberOfEdges(); ++edge){
            f(edge);
        }
    }


protected:


    template<class MUTEX>
    int64_t inserEdgeWithMutex(
        const int64_t u, 
        const int64_t v,
        MUTEX & edgeMutex,
        MUTEX * nodeMutexArray,
        const size_t nMutex
    ){
        const auto mu = u%nMutex;
        const auto mv = v%nMutex;
        auto & uMtx =  nodeMutexArray[mu];
        auto & vMtx =  nodeMutexArray[mv];   
        uMtx.lock();
        const auto fres =  nodes_[u].find(NodeAdjacency(v));
        const auto foundThisEdge = (fres != nodes_[u].end());
        uMtx.unlock();
        if(foundThisEdge){
            return fres->edge();
        }
        else{
            const auto uu = std::min(u,v);
            const auto vv = std::max(u,v);
            auto e = EdgeStorage(uu, vv);

            edgeMutex.lock();
            auto ei = edges_.size();
            edges_.push_back(e);
            edgeMutex.unlock();

            if(mu!=mv){
                uMtx.lock();
                nodes_[u].insert(NodeAdjacency(v,ei));
                uMtx.unlock();

                vMtx.lock();
                nodes_[v].insert(NodeAdjacency(u,ei));
                vMtx.unlock();
            }
            else{
                uMtx.lock();
                nodes_[u].insert(NodeAdjacency(v,ei));
                nodes_[v].insert(NodeAdjacency(u,ei));
                uMtx.unlock();
            }
            return ei;
        }
    }


    std::vector<NodeStorage> nodes_;
    std::vector<EdgeStorage> edges_;
};

} // namespace nifty::graph
} // namespace nifty

#endif  // NIFTY_GRAPH_SIMPLE_GRAPH_HXX
