/*=========================================================================
*
*  Copyright NumFOCUS
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*         http://www.apache.org/licenses/LICENSE-2.0.txt
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
*
*=========================================================================*/
#ifndef sitkPimpleTransform_hxx
#define sitkPimpleTransform_hxx

#include "sitkTransform.h"

#include "itkTransformBase.h"


#include "itkIdentityTransform.h"
#include "itkTranslationTransform.h"
#include "itkScaleTransform.h"
#include "itkScaleLogarithmicTransform.h"
#include "itkSimilarity2DTransform.h"
#include "itkSimilarity3DTransform.h"
#include "itkEuler2DTransform.h"
#include "itkEuler3DTransform.h"
#include "itkQuaternionRigidTransform.h"
#include "itkVersorTransform.h"
#include "itkVersorRigid3DTransform.h"
#include "itkAffineTransform.h"
#include "itkCompositeTransform.h"

#include "itkDisplacementFieldTransform.h"
#include "itkBSplineSmoothingOnUpdateDisplacementFieldTransform.h"
#include "itkGaussianSmoothingOnUpdateDisplacementFieldTransform.h"
#include "itkBSplineTransform.h"

#include <type_traits>

namespace itk
{
namespace simple
{

// This is a base class of the private implementatino of the transform
// class.
//
// The interface provide virutal method and other generic methods to
// the concrete ITK transform type, there by provide encapsulation and
// a uniform interface
class SITKCommon_HIDDEN PimpleTransformBase
{
public:

  virtual ~PimpleTransformBase( ) = default;;

  // Get Access to the internal ITK transform class
  virtual TransformBase * GetTransformBase( ) = 0;
  virtual const TransformBase * GetTransformBase( ) const = 0;

  // general methods to get information about the internal class
  virtual unsigned int GetInputDimension( ) const = 0;
  virtual unsigned int GetOutputDimension( ) const = 0;

  // Set the fixed parameter for the transform, converting from the
  // simpleITK std::vector to the ITK's array.
  void SetFixedParameters( const std::vector< double > &inParams )
    {
      size_t numberOfFixedParameters = this->GetTransformBase()->GetFixedParameters().Size();

      if ( numberOfFixedParameters == 0 )
        {
        return;
        }
      else if (numberOfFixedParameters > inParams.size())
        {
        sitkExceptionMacro("Transform expected " << numberOfFixedParameters << " fixed parameters but only " << inParams.size() << " are provided!");
        }

      // let the itk::Array class hold a reference to the input vector
      itk::TransformBase::ParametersType p;
      p.SetData( const_cast<double*>(&inParams[0]), numberOfFixedParameters, false );
      this->GetTransformBase()->SetFixedParameters( p );
    }

  // Get the fixed parameters form the transform
  std::vector< double >  GetFixedParameters( ) const
    {
      const itk::TransformBase::ParametersType &p = this->GetTransformBase()->GetFixedParameters();
      return std::vector< double >( p.begin(), p.end() );
    }

  unsigned int GetNumberOfFixedParameters( ) const
  {
    return this->GetTransformBase()->GetFixedParameters().GetSize();
  }

  unsigned int GetNumberOfParameters( ) const
  {
    return this->GetTransformBase()->GetNumberOfParameters();
  }


  void SetParameters( const std::vector< double > &inParams )
    {
      unsigned int numberOfParameters = this->GetTransformBase()->GetNumberOfParameters();

      if ( numberOfParameters == 0 )
        {
        return;
        }
      else if ( numberOfParameters > inParams.size())
        {
        sitkExceptionMacro("Transform expected " << numberOfParameters << " parameters but only " << inParams.size() << " are provided!");
        }

      // let the itk::Array class hold a reference to the input vector
      itk::TransformBase::ParametersType p;
      p.SetData( const_cast<double*>(&inParams[0]), numberOfParameters, false );
      this->GetTransformBase()->SetParameters( p );
    }
  std::vector< double > GetParameters( ) const
    {
      const itk::TransformBase::ParametersType &p = this->GetTransformBase()->GetParameters();
      return std::vector< double >( p.begin(), p.end() );
    }


  virtual PimpleTransformBase *ShallowCopy( ) const = 0;
  virtual PimpleTransformBase *DeepCopy( ) const = 0;

  virtual int GetReferenceCount( ) const = 0;

  virtual void SetIdentity() = 0;

  virtual void FlattenTransform() = 0;

  // Tries to construct an inverse of the transform, if true is returned
  // the inverse was successful, and outputTransform is modified to
  // the new class and ownership it passed to the caller.  Otherwise
  // outputTranform is not changed.
  virtual bool GetInverse( PimpleTransformBase * &outputTransform ) const = 0;

  virtual bool IsLinear() const
    {
      using TransformCategoryEnum = itk::TransformBaseTemplate<double>::TransformCategoryEnum;
      return (this->GetTransformBase()->GetTransformCategory() == TransformCategoryEnum::Linear);
    }


  std::string ToString( ) const
    {
      std::ostringstream out;
      this->GetTransformBase()->Print ( out, 1 );
      return out.str();
    }

  // note: the returned pointer needs to be externally managed and
  // deleted
  // Also the return pointer could be this
  virtual PimpleTransformBase* AddTransform( Transform &t ) = 0;


  virtual std::vector< double > TransformPoint( const std::vector< double > &p ) const = 0;
  virtual std::vector< double > TransformVector( const std::vector< double > &v,
                                                 const std::vector< double > &p) const = 0;

protected:

};

template< typename TTransformType >
class SITKCommon_HIDDEN PimpleTransform
  : public PimpleTransformBase
{
public:
  using Self = PimpleTransform;
  using TransformType = TTransformType;
  using TransformPointer = typename TransformType::Pointer;

  using CompositeTransformType = itk::CompositeTransform<double, TransformType::InputSpaceDimension>;

  static const unsigned int InputDimension = TTransformType::InputSpaceDimension;
  static const unsigned int OutputDimension = TTransformType::OutputSpaceDimension;

  template<typename TScalar>
  PimpleTransform( itk::Image<itk::Vector<TScalar, InputDimension>, InputDimension> *displacement )
    {
      this->m_Transform = TransformType::New();
      this->m_Transform->SetDisplacementField( displacement );
    }


  PimpleTransform( TransformType * p)
    {
      this->m_Transform = p;
    }

  PimpleTransform( )
    {
      this->m_Transform = TransformType::New();
    }

  PimpleTransform( Self &s )
    : m_Transform( s.m_Transform )
    {}

  PimpleTransform &operator=( const PimpleTransform &s )
    {
      m_Transform = s.m_Transform;
    }

  TransformBase * GetTransformBase( ) override { return this->m_Transform.GetPointer(); }
  const TransformBase * GetTransformBase( ) const override { return this->m_Transform.GetPointer(); }

  unsigned int GetInputDimension( ) const override { return InputDimension; }
  unsigned int GetOutputDimension( ) const override { return OutputDimension; }


  PimpleTransformBase *ShallowCopy( ) const override
    {
      return new Self( this->m_Transform.GetPointer() );
    }

  PimpleTransformBase *DeepCopy( ) const override
    {
      PimpleTransformBase *copy( new Self( this->m_Transform->Clone() ) );
      return copy;
    }

  int GetReferenceCount( ) const override
    {
      return this->m_Transform->GetReferenceCount();
    }

  void SetIdentity() override
    {
      this->SetIdentity(this->m_Transform.GetPointer());
    }

  template <typename UTransform>
  void SetIdentity( UTransform *self)
    {
      self->SetIdentity();
    }

  template <typename UScalar, unsigned int UDimension>
  void SetIdentity( itk::CompositeTransform<UScalar, UDimension> *self)
    {
      sitkExceptionMacro( "SetIdentity does is not implemented for transforms of type " << self->GetNameOfClass() );
    }

#if ( ( SITK_ITK_VERSION_MAJOR == 4 ) && ( SITK_ITK_VERSION_MINOR < 7 ) )
  template <typename UScalar, unsigned int UDimension>
  void SetIdentity( itk::DisplacementFieldTransform<UScalar, UDimension> *self)
      {
        using DFTType = itk::DisplacementFieldTransform<UScalar, UDimension>;
        typename DFTType::DisplacementFieldType *displacementField;

        displacementField = self->GetModifiableDisplacementField();
        if (displacementField)
          {
          displacementField->FillBuffer(typename DFTType::OutputVectorType(0.0));
          }
        displacementField = self->GetModifiableInverseDisplacementField();
        if (displacementField)
          {
          displacementField->FillBuffer(typename DFTType::OutputVectorType(0.0));
          }
      }

  template <typename UScalar, unsigned int UDimension>
  void SetIdentity( itk::BSplineSmoothingOnUpdateDisplacementFieldTransform<UScalar, UDimension> *self)
      {
        using DFTType = itk::DisplacementFieldTransform<UScalar, UDimension>;
        typename DFTType::DisplacementFieldType *displacementField;

        displacementField = self->GetModifiableDisplacementField();
        if (displacementField)
          {
          displacementField->FillBuffer(typename DFTType::OutputVectorType(0.0));
          }
        displacementField = self->GetModifiableInverseDisplacementField();
        if (displacementField)
          {
          displacementField->FillBuffer(typename DFTType::OutputVectorType(0.0));
          }
      }

  template <typename UScalar, unsigned int UDimension>
  void SetIdentity( itk::GaussianSmoothingOnUpdateDisplacementFieldTransform<UScalar, UDimension> *self)
      {
        using DFTType = itk::DisplacementFieldTransform<UScalar, UDimension>;
        typename DFTType::DisplacementFieldType *displacementField;

        displacementField = self->GetModifiableDisplacementField();
        if (displacementField)
          {
          displacementField->FillBuffer(typename DFTType::OutputVectorType(0.0));
          }
        displacementField = self->GetModifiableInverseDisplacementField();
        if (displacementField)
          {
          displacementField->FillBuffer(typename DFTType::OutputVectorType(0.0));
          }
      }


#endif


  void FlattenTransform() override
    {
      this->FlattenTransform(this->m_Transform.GetPointer());
    }

  template <typename UTransform>
  void FlattenTransform( UTransform *self)
    {
      Unused(self);
      // nothing to do
    }

  template <typename UScalar, unsigned int UDimension>
  void FlattenTransform( itk::CompositeTransform<UScalar, UDimension> *self)
    {
      self->FlattenTransformQueue();
      // TODO: If there is only one transform remove it from the
      // composite transform.
    }

  bool GetInverse(PimpleTransformBase * &outputTransform) const override
    {
      typename itk::LightObject::Pointer light = this->m_Transform->CreateAnother();
      typename TransformType::Pointer another = dynamic_cast<TransformType*>(light.GetPointer());

      if (another.IsNull())
        {
        sitkExceptionMacro("Unexpected error creating another " << this->m_Transform->GetNameOfClass() << ".")
        }
      if (!this->m_Transform->GetInverse(another))
        {
        return false;
        }
      outputTransform = new Self( another.GetPointer() );
      return true;
    }


  PimpleTransformBase* AddTransform( Transform &t ) override
    {
      if ( t.GetDimension() != TransformType::InputSpaceDimension )
        {
        sitkExceptionMacro( "Transform argument has dimension " << t.GetDimension()
                            << " does not match this dimension of " << TransformType::InputSpaceDimension );
        }

      typename CompositeTransformType::TransformType* base =
        dynamic_cast< typename CompositeTransformType::TransformType*>( t.GetITKBase() );

      return this->AddTransform( base, typename std::is_same<TTransformType, CompositeTransformType>::type() );
    }

  PimpleTransformBase* AddTransform( typename CompositeTransformType::TransformType* t, std::true_type isCompositeTransform )
    {
      Unused( isCompositeTransform );
      assert( t->GetInputSpaceDimension() == TransformType::InputSpaceDimension );

      m_Transform->AddTransform( t );
      m_Transform->SetAllTransformsToOptimizeOff();
      m_Transform->SetOnlyMostRecentTransformToOptimizeOn();

      return this;
    }

  PimpleTransformBase* AddTransform( typename CompositeTransformType::TransformType* t, std::false_type isNotCompositeTransform )
    {
      Unused( isNotCompositeTransform );

      typename CompositeTransformType::Pointer composite = CompositeTransformType::New();
      composite->AddTransform( this->m_Transform );
      composite->AddTransform( t );
      composite->SetAllTransformsToOptimizeOff();
      composite->SetOnlyMostRecentTransformToOptimizeOn();

      return new PimpleTransform<CompositeTransformType>( composite );
    }


  std::vector< double > TransformPoint( const std::vector< double > &pt ) const override
    {
      if (pt.size() != this->GetInputDimension() )
        {
        sitkExceptionMacro("point dimension mismatch");
        }

      typename TransformType::OutputPointType opt =
        this->m_Transform->TransformPoint( sitkSTLVectorToITK< typename TransformType::InputPointType >(pt));

      return sitkITKVectorToSTL<double>( opt );
    }


  std::vector< double > TransformVector( const std::vector< double > &vec, const std::vector< double > &pt ) const override
    {
      if (vec.size() != this->GetInputDimension() )
        {
        sitkExceptionMacro("vector dimension mismatch");
        }

      const typename TransformType::OutputVectorType & itk_vec = sitkSTLVectorToITK< typename TransformType::InputVectorType >(vec);

      if (pt.size() != this->GetInputDimension() )
        {
        sitkExceptionMacro("point dimension mismatch");
        }

      const typename TransformType::OutputPointType &itk_pt = sitkSTLVectorToITK< typename TransformType::InputPointType >(pt);

      return sitkITKVectorToSTL<double>( this->m_Transform->TransformVector( itk_vec, itk_pt ) );
    }

private:

  TransformPointer m_Transform;
};

}
}


#endif //  sitkPimpleTransform_hxx
