/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    itkStatisticsImageFilter.txx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 2002 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef _itkStatisticsImageFilter_txx
#define _itkStatisticsImageFilter_txx
#include "itkStatisticsImageFilter.h"

#include "itkImageRegionIterator.h"
#include "itkNumericTraits.h"

namespace itk {

template<class TInputImage>
void
StatisticsImageFilter<TInputImage>
::GenerateInputRequestedRegion()
{
  Superclass::GenerateInputRequestedRegion();
  if ( this->GetInput() )
    {
    this->GetInput()->SetRequestedRegionToLargestPossibleRegion();
    }
}


template<class TInputImage>
void
StatisticsImageFilter<TInputImage>
::AllocateOutputs()
{
  // Pass the input through as the output
  this->GraftOutput(this->GetInput());
}

template<class TInputImage>
void
StatisticsImageFilter<TInputImage>
::BeforeThreadedGenerateData()
{
  int numberOfThreads = this->GetNumberOfThreads();

  // Resize the thread temporaries
  m_Count.resize(numberOfThreads);
  m_SumOfSquares.resize(numberOfThreads);
  m_Sum.resize(numberOfThreads);
  m_ThreadMin.resize(numberOfThreads);
  m_ThreadMax.resize(numberOfThreads);
  
  // Initialize the temporaries
  m_Count.Fill(NumericTraits<long>::Zero);
  m_Sum.Fill(NumericTraits<RealType>::Zero);
  m_SumOfSquares.Fill(NumericTraits<RealType>::Zero);
  m_ThreadMin.Fill(NumericTraits<RealType>::max());
  m_ThreadMax.Fill(NumericTraits<RealType>::NonpositiveMin());
}

template<class TInputImage>
void
StatisticsImageFilter<TInputImage>
::AfterThreadedGenerateData()
{
  unsigned int i;
  long count;
  RealType sum, sumOfSquares;
    
  int numberOfThreads = this->GetNumberOfThreads();

  sum = sumOfSquares = NumericTraits<RealType>::Zero;
  count = 0;

  // Find the min/max over all threads and accumulate count, sum and sum of squares
  m_Minimum = NumericTraits<RealType>::max();
  m_Maximum = NumericTraits<RealType>::NonpositiveMin();
  for( i = 0; i < numberOfThreads; i++)
    {
    count += m_Count[i];
    sum += m_Sum[i];
    sumOfSquares += m_SumOfSquares[i];

    if (m_ThreadMin[i] < m_Minimum)
      {
      m_Minimum = m_ThreadMin[i];
      }
    if (m_ThreadMax[i] > m_Maximum)
      {
      m_Maximum = m_ThreadMax[i];
      }
    }
  // compute statistics
  m_Mean = sum / static_cast<RealType>(count);

  // unbiased estimate
  m_Variance = (sumOfSquares - (sum*sum / static_cast<RealType>(count)))
    / (static_cast<RealType>(count) - 1);
  m_Sigma = sqrt(m_Variance);

}

template<class TInputImage>
void
StatisticsImageFilter<TInputImage>
::ThreadedGenerateData(const RegionType& regionForThread,
                       int threadId) 
{
  RealType value;
  ImageRegionIterator<TInputImage> it (this->GetInput(), regionForThread);
  
  // support progress methods/callbacks
  unsigned long updateVisits = 0, i=0;
  if ( threadId == 0 )
    {
    updateVisits = regionForThread.GetNumberOfPixels()/10;
    if ( updateVisits < 1 ) updateVisits = 1;
    }

  // do the work
  while (!it.IsAtEnd())
    {
    value = static_cast<RealType>(it.Get());
    if (value < m_ThreadMin[threadId])
      {
      m_ThreadMin[threadId] = value;
      }
    if (value > m_ThreadMax[threadId])
      {
      m_ThreadMax[threadId] = value;
      }
    
    m_Sum[threadId] += value;
    m_SumOfSquares[threadId] += (value * value);
    m_Count[threadId]++;
    ++it;

    if ( threadId == 0 && !(i % updateVisits ) )
      {
      this->UpdateProgress( static_cast<float>(i) / 
                            static_cast<float>(updateVisits * 10.0) );
      }
    ++i;
    }
}

template <class TImage>
void 
StatisticsImageFilter<TImage>
::PrintSelf(std::ostream& os, Indent indent) const
{
  Superclass::PrintSelf(os,indent);

  os << indent << "Minimum: "  << m_Minimum << std::endl;
  os << indent << "Maximum: "  << m_Maximum << std::endl;
  os << indent << "Mean: "     << m_Mean << std::endl;
  os << indent << "Sigma: "    << m_Sigma << std::endl;
  os << indent << "Variance: " << m_Variance << std::endl;
}


}// end namespace itk
#endif
