#pragma once
#ifndef NIFTY_GRAPH_GALA_CLASSIFIER_RF_HXX
#define NIFTY_GRAPH_GALA_CLASSIFIER_RF_HXX

#include <iostream>
#include "vigra/multi_array.hxx"
#include "vigra/random_forest.hxx"


namespace nifty{
namespace graph{

    template<class T>
    class RfClassifier{
    public:
        RfClassifier()
        :   classifier_(nullptr){

        }
        void initialize(const size_t numberOfFeatures){
            numberOfFeatures_ = numberOfFeatures;
            classifier_ = nullptr;
        }
        void addTrainingExample(const T * features, const uint8_t label){
            newRfLabels_.push_back(label);
            for(size_t fi=0; fi<numberOfFeatures_; ++fi){
                newRfFeatruresFlat_.push_back(features[fi]);
            }
        }
        void train(){
            makeTrainingSet();
            if(classifier_ != nullptr){
                delete classifier_;
            }
            auto rfOpts = vigra::RandomForestOptions();  
            rfOpts.tree_count(100);
            rfOpts.predict_weighted();
            classifier_ = new  vigra::RandomForest<uint8_t>(rfOpts);

            // construct visitor to calculate out-of-bag error
            vigra::rf::visitors::OOB_Error oob_v;
            // perform training
            classifier_->learn(rfFeatures_, rfLabels_, vigra::rf::visitors::create_visitor(oob_v));
            std::cout << "the out-of-bag error is: " << oob_v.oob_breiman << "\n"; 
        }

        double predictProbability(const T * features){
            // copy stuff atm 
            vigra::MultiArray<2, T>       f(vigra::Shape2(1,numberOfFeatures_));     
            vigra::MultiArray<2, double>  p(vigra::Shape2(1,2));
            for(size_t i=0; i<numberOfFeatures_; ++i){
                f[i] = features[i];
            }
            classifier_->predictProbabilities(f,p);
            return p(0,1);
        }
    private:
        void makeTrainingSet(){

            const auto nOld = rfFeatures_.shape(0);
            const auto nAdded = newRfLabels_.size();
            const auto nTotal = nOld + nAdded;

            std::cout<<"new added examples "<<nAdded<<" total "<<nTotal<<"\n";
            vigra::MultiArray<2, T> fNew(vigra::Shape2( nTotal ,numberOfFeatures_));
            vigra::MultiArray<2, uint8_t> lNew(vigra::Shape2( nTotal ,1));

            for(auto i=0; i<nOld; ++i){
                lNew[i] = rfLabels_[i];
                for(auto fi=0; fi<numberOfFeatures_; ++fi)
                    fNew(i, fi) = rfFeatures_(i,fi);
            }
            auto c = 0;
            for(auto i=nOld; i<nTotal; ++i){
                lNew[i] = newRfLabels_[i-nOld];
                for(auto fi=0; fi<numberOfFeatures_; ++fi){
                    fNew(i, fi) = newRfFeatruresFlat_[c];
                    ++c;
                }
            }
            rfFeatures_ = fNew;
            rfLabels_ = lNew;
            newRfLabels_.resize(0);
            newRfFeatruresFlat_.resize(0);
        }


        vigra::RandomForest<uint8_t> *  classifier_;
        vigra::MultiArray<2, T>         rfFeatures_;     
        vigra::MultiArray<2, uint8_t>   rfLabels_;

        std::vector< T >        newRfFeatruresFlat_;
        std::vector<uint8_t>    newRfLabels_;
        uint64_t numberOfFeatures_;
    };


} // namespace nifty::graph
} // namespace nifty

#endif  // NIFTY_GRAPH_GALA_CLASSIFIER_RF_HXX