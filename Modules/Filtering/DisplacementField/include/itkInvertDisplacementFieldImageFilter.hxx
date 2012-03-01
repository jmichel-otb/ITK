/*=========================================================================
 *
 *  Copyright Insight Software Consortium
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
#ifndef __itkInvertDisplacementFieldImageFilter_hxx
#define __itkInvertDisplacementFieldImageFilter_hxx

#include "itkInvertDisplacementFieldImageFilter.h"

#include "itkComposeDisplacementFieldsImageFilter.h"
#include "itkImageRegionIterator.h"
#include "itkVectorLinearInterpolateImageFunction.h"

namespace itk
{

/*
 * InvertDisplacementFieldImageFilter class definitions
 */
template<class TInputImage, class TOutputImage>
InvertDisplacementFieldImageFilter<TInputImage, TOutputImage>
::InvertDisplacementFieldImageFilter() :
 m_MaximumNumberOfIterations( 20 ),
 m_MaxErrorToleranceThreshold( 0.1 ),
 m_MeanErrorToleranceThreshold( 0.001 )
{
  this->SetNumberOfRequiredInputs( 1 );

  typedef VectorLinearInterpolateImageFunction <InputFieldType, RealType> DefaultInterpolatorType;
  typename DefaultInterpolatorType::Pointer interpolator = DefaultInterpolatorType::New();
  this->m_Interpolator = interpolator;

  this->m_ComposedField = DisplacementFieldType::New();
  this->m_ScaledNormImage = RealImageType::New();
  this->m_EnforceBoundaryCondition = true;

}

template<class TInputImage, class TOutputImage>
InvertDisplacementFieldImageFilter<TInputImage, TOutputImage>
::~InvertDisplacementFieldImageFilter()
{
}

template<class TInputImage, class TOutputImage>
void
InvertDisplacementFieldImageFilter<TInputImage, TOutputImage>
::SetInterpolator( InterpolatorType *interpolator )
{
  itkDebugMacro( "setting Interpolator to " << interpolator );
  if ( this->m_Interpolator != interpolator )
    {
    this->m_Interpolator = interpolator;
    this->Modified();
    if( !this->GetDisplacementField() )
      {
      this->m_Interpolator->SetInputImage( this->GetInput( 0 ) );
      }
    }
}

template<class TInputImage, class TOutputImage>
void
InvertDisplacementFieldImageFilter<TInputImage, TOutputImage>
::GenerateData()
{
  this->AllocateOutputs();

  VectorType zeroVector( 0.0 );

  typename DisplacementFieldType::ConstPointer displacementField = this->GetInput();
  typename InverseDisplacementFieldType::Pointer inverseDisplacementField = this->GetOutput();
  inverseDisplacementField->FillBuffer( zeroVector );

  for( unsigned int d = 0; d < ImageDimension; d++ )
    {
    this->m_DisplacementFieldSpacing[d] = displacementField->GetSpacing()[d];
    }

  this->m_ScaledNormImage->CopyInformation( displacementField );
  this->m_ScaledNormImage->SetRegions( displacementField->GetRequestedRegion() );
  this->m_ScaledNormImage->Allocate();
  this->m_ScaledNormImage->FillBuffer( 0.0 );

  SizeValueType numberOfPixelsInRegion = ( displacementField->GetRequestedRegion() ).GetNumberOfPixels();
  this->m_MaxErrorNorm = NumericTraits<RealType>::max();
  this->m_MeanErrorNorm = NumericTraits<RealType>::max();
  unsigned int iteration = 0;

  while( iteration++ < this->m_MaximumNumberOfIterations &&
    this->m_MaxErrorNorm > this->m_MaxErrorToleranceThreshold &&
    this->m_MeanErrorNorm > this->m_MeanErrorToleranceThreshold )
    {
    itkDebugMacro( "Iteration " << iteration << ": mean error norm = " << this->m_MeanErrorNorm
      << ", max error norm = " << this->m_MaxErrorNorm );

    typedef ComposeDisplacementFieldsImageFilter<DisplacementFieldType> ComposerType;
    typename ComposerType::Pointer composer = ComposerType::New();
    composer->SetDisplacementField( displacementField );
    composer->SetWarpingField( inverseDisplacementField );

    this->m_ComposedField = composer->GetOutput();
    this->m_ComposedField->Update();
    this->m_ComposedField->DisconnectPipeline();

    /**
     * Multithread processing to multiply each element of the composed field by 1 / spacing
     */
    this->m_MeanErrorNorm = NumericTraits<RealType>::Zero;
    this->m_MaxErrorNorm = NumericTraits<RealType>::Zero;

    this->m_DoThreadedEstimateInverse = false;
    typename ImageSource<TOutputImage>::ThreadStruct str0;
    str0.Filter = this;
    this->GetMultiThreader()->SetNumberOfThreads( this->GetNumberOfThreads() );
    this->GetMultiThreader()->SetSingleMethod( this->ThreaderCallback, &str0 );
    this->GetMultiThreader()->SingleMethodExecute();

    this->m_MeanErrorNorm /= static_cast<RealType>( numberOfPixelsInRegion );

    this->m_Epsilon = 0.5;
    if( iteration == 1 )
      {
      this->m_Epsilon = 0.75;
      }

    /**
     * Multithread processing to estimate inverse field
     */
    this->m_DoThreadedEstimateInverse = true;
    typename ImageSource<TOutputImage>::ThreadStruct str1;
    str1.Filter = this;
    this->GetMultiThreader()->SetNumberOfThreads( this->GetNumberOfThreads() );
    this->GetMultiThreader()->SetSingleMethod( this->ThreaderCallback, &str1 );
    this->GetMultiThreader()->SingleMethodExecute();
    }
}

template<class TInputImage, class TOutputImage>
void
InvertDisplacementFieldImageFilter<TInputImage, TOutputImage>
::ThreadedGenerateData( const RegionType & region, ThreadIdType itkNotUsed( threadId ) )
{
  const typename DisplacementFieldType::RegionType fullregion = this->m_ComposedField->GetRequestedRegion();
  const typename DisplacementFieldType::SizeType size = fullregion.GetSize();
  const typename DisplacementFieldType::IndexType startIndex = fullregion.GetIndex();
  const typename DisplacementFieldType::PixelType zeroVector( 0.0 );
  ImageRegionIterator<DisplacementFieldType> ItE( this->m_ComposedField, region );
  ImageRegionIterator<RealImageType> ItS( this->m_ScaledNormImage, region );

  if( this->m_DoThreadedEstimateInverse )
    {
    ImageRegionIterator<DisplacementFieldType> ItI( this->GetOutput(), region );

    for( ItI.GoToBegin(), ItE.GoToBegin(), ItS.GoToBegin(); !ItI.IsAtEnd(); ++ItI, ++ItE, ++ItS )
      {
      VectorType update = ItE.Get();
      RealType scaledNorm = ItS.Get();

      if( scaledNorm > this->m_Epsilon * this->m_MaxErrorNorm  )
        {
        update *= ( this->m_Epsilon * this->m_MaxErrorNorm / scaledNorm );
        }
      update = ItI.Get() + update * this->m_Epsilon;
      ItI.Set( update );
      typename DisplacementFieldType::IndexType index = ItI.GetIndex();
      if ( this->m_EnforceBoundaryCondition )
        {
        for ( unsigned int dimension = 0; dimension < ImageDimension; ++dimension )
          {
          if ( index[dimension] == startIndex[dimension] || index[dimension] == static_cast<IndexValueType>( size[dimension] ) - startIndex[dimension] - 1 )
            {
            ItI.Set( zeroVector );
            break;
            }
          }
        } // enforce boundary condition
      }
    }
  else
    {
    for( ItE.GoToBegin(), ItS.GoToBegin(); !ItE.IsAtEnd(); ++ItE, ++ItS )
      {
      VectorType displacement = ItE.Get();
      RealType scaledNorm = 0.0;
      for( unsigned int d = 0; d < ImageDimension; d++ )
        {
        scaledNorm += vnl_math_sqr( displacement[d] / this->m_DisplacementFieldSpacing[d] );
        }
      scaledNorm = vcl_sqrt( scaledNorm );

      this->m_MeanErrorNorm += scaledNorm;
      if( this->m_MaxErrorNorm < scaledNorm )
        {
        this->m_MaxErrorNorm = scaledNorm;
        }

      ItS.Set( scaledNorm );
      ItE.Set( -displacement );
      }
    }
}

template<class TInputImage, class TOutputImage>
void
InvertDisplacementFieldImageFilter<TInputImage, TOutputImage>
::PrintSelf( std::ostream& os, Indent indent ) const
{
  Superclass::PrintSelf( os, indent );

  os << "Interpolator:" << std::endl;
  this->m_Interpolator->Print( os, indent );

  os << "Maximum number of iterations: " << this->m_MaximumNumberOfIterations << std::endl;
  os << "Max error tolerance threshold: " << this->m_MaxErrorToleranceThreshold << std::endl;
  os << "Mean error tolerance threshold: " << this->m_MeanErrorToleranceThreshold << std::endl;
}

}  //end namespace itk

#endif