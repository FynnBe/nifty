#pragma once
#ifndef NIFTY_PYTHON_CONVERTER_HXX
#define NIFTY_PYTHON_CONVERTER_HXX

#include <type_traits>
#include <initializer_list>
//#include <pybind11/stl.h>



#include <pybind11/pybind11.h>
#include <pybind11/cast.h>
#include <pybind11/numpy.h>

#include <nifty/marray/marray.hxx>

namespace py = pybind11;





namespace pybind11
{
    namespace detail
    {
        template <typename Type, int DIM> 
        struct pymarray_caster;
    }
}

namespace nifty
{
namespace marray
{

    template <typename Type, int DIM = 0> 
    class PyView : public View<Type, false>
    {
        friend struct pybind11::detail::pymarray_caster<Type,DIM>;

      private:
        pybind11::array_t<Type> py_array;

      public:
        template <class ShapeIterator>
        PyView(pybind11::array_t<Type> array, Type *data, ShapeIterator begin, ShapeIterator end)
            : View<Type, false>(begin, end, data, FirstMajorOrder, FirstMajorOrder), py_array(array)
        {
            auto info = py_array.request();
            Type *ptr = (Type *)info.ptr;

            std::vector<std::size_t> strides(info.strides.begin(),info.strides.end());
            for(std::size_t i=0; i<strides.size(); ++i){
                strides[i] /= sizeof(Type);
            }
            this->assign( info.shape.begin(), info.shape.end(), strides.begin(), ptr, FirstMajorOrder);

        }

        PyView()
        {
        }
        const Type & operator[](const uint64_t index)const{
            return this->operator()(index);
        }
        Type & operator[](const uint64_t index){
            return this->operator()(index);
        }
        template <class ShapeIterator> PyView(ShapeIterator begin, ShapeIterator end)
        {
            std::vector<std::size_t> shape, strides;

            for (auto i = begin; i != end; ++i)
                shape.push_back(*i);

            for (std::size_t i = 0; i < shape.size(); ++i) {
                std::size_t stride = sizeof(Type);
                for (std::size_t j = i + 1; j < shape.size(); ++j)
                    stride *= shape[j];
                strides.push_back(stride);
            }

            py_array = pybind11::array(pybind11::buffer_info(
                nullptr, sizeof(Type), pybind11::format_descriptor<Type>::value, shape.size(), shape, strides));
            pybind11::buffer_info info = py_array.request();
            Type *ptr = (Type *)info.ptr;

            for (std::size_t i = 0; i < shape.size(); ++i) {
                strides[i] /= sizeof(Type);
            }
            this->assign(begin, end, strides.begin(), ptr, LastMajorOrder);
        }

    #ifdef HAVE_CPP11_INITIALIZER_LISTS
        template<class T_INIT>
        PyView(std::initializer_list<T_INIT> shape) : PyView(shape.begin(), shape.end())
        {
        }
    #endif
    };
}
}

namespace pybind11
{

    namespace detail
    {

        template <typename Type, int DIM> 
        struct pymarray_caster {
            typedef typename nifty::marray::PyView<Type, DIM> ViewType;
            typedef type_caster<typename intrinsic_type<Type>::type> value_conv;

            typedef typename pybind11::array_t<Type> pyarray_type;
            typedef type_caster<pyarray_type> pyarray_conv;

            bool load(handle src, bool convert)
            {
                // convert numpy array to py::array_t
                pyarray_conv conv;
                if (!conv.load(src, convert)){
                    return false;
                }
                auto pyarray = (pyarray_type)conv;

                // convert py::array_t to nifty::marray::PyView
                auto info = pyarray.request();
                if(DIM != 0 && DIM != info.shape.size()){
                    std::cout<<"not matching\n";
                    return false;
                }
                Type *ptr = (Type *)info.ptr;

                ViewType result(pyarray, ptr, info.shape.begin(), info.shape.end());
                value = result;
                return true;
            }

            static handle cast(ViewType src, return_value_policy policy, handle parent)
            {
                pyarray_conv conv;
                return conv.cast(src.py_array, policy, parent);
            }

            PYBIND11_TYPE_CASTER(ViewType, _("array<") + value_conv::name() + _(">"));
        };

        template <typename Type, int DIM> 
        struct type_caster<nifty::marray::PyView<Type, DIM> > 
            : pymarray_caster<Type,DIM> {
        };

        //template <typename Type, int DIM> 
        //struct marray_caster {
        //    static_assert(std::is_same<Type, void>::value,
        //                  "Please use nifty::marray::PyView instead of nifty::marray::View for arguments and return values.");
        //};
        //template <typename Type> 
        //struct type_caster<andres::View<Type> > 
        //: marray_caster<Type> {
        //};
    }
}














/*

namespace nifty{

    template<class T>
    std::vector<std::size_t> toSizeT(std::initializer_list<T> l){
        return std::vector<std::size_t>(l.begin(),l.end());
    }



    template<class T>
    class NumpyArray : public marray::View<T>{
    public:

        template<class SHAPE_T,class STRIDE_T>
        NumpyArray(
            std::initializer_list<SHAPE_T> shape,
            std::initializer_list<STRIDE_T> strides
        )
        : array_(){


            auto svec = toSizeT(strides);
            for(auto i=0; i<svec.size(); ++i){
                svec[i] *= sizeof(T);
            }

            array_  = py::array(
                py::buffer_info(NULL, sizeof(T),
                py::format_descriptor<T>::value,
                shape.size(), toSizeT(shape),svec)
            );

            py::buffer_info info = array_.request();
            T * ptr = static_cast<T*>(info.ptr);

            this->assign(shape.begin(),shape.end(),strides.begin(),ptr,
                marray::FirstMajorOrder
            );
        }


        NumpyArray(py::array_t<T> & array)
        :   array_(array) 
        {
            py::buffer_info info = array.request();
            T * ptr = static_cast<T*>(info.ptr);
            const auto & shape = info.shape;
            auto  strides=  info.strides;
            for(auto & s : strides)
                s/=sizeof(T);
            this->assign(shape.begin(),shape.end(),strides.begin(),ptr,
                marray::FirstMajorOrder
            );
        }
        

        py::array_t<T>  pyArray(){
            return array_;
        }        

    private:
        py::array_t<T> array_;
    };

} // namespace nifty
*/

#endif  // NIFTY_PYTHON_CONVERTER_HXX
