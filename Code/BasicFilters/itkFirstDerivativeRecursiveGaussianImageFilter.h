/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    itkFirstDerivativeRecursiveGaussianImageFilter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 2002 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef __itkFirstDerivativeRecursiveGaussianImageFilter_h
#define __itkFirstDerivativeRecursiveGaussianImageFilter_h

#include "itkRecursiveGaussianImageFilter.h"

namespace itk
{
  
/** \class FirstDerivativeRecursiveGaussianImageFilter
 * \brief Convolve an image with a kernel approximating the
 *        first derivative of a Gaussian.
 *
 * itkFirstDerivativeRecursiveGaussianImageFilter convolves an image with a kernel
 * approximating the first derivative of a Gaussian.  This class implements
 * the recursive filtering method proposed by R.Deriche in IEEE-PAMI Vol.12,
 * No.1, January 1990, pp 78-87.  
 *
 * \ingroup ImageEnhancement 
 * \ingroup ImageFeatureExtraction   Singlethreaded
 *
 */
template <class TInputImage, class TOutputImage, class TComputation>
class ITK_EXPORT FirstDerivativeRecursiveGaussianImageFilter:
    public RecursiveGaussianImageFilter<TInputImage,TOutputImage,TComputation>
{

public:
  /** Standard class typedefs. */
  typedef FirstDerivativeRecursiveGaussianImageFilter  Self;
  typedef RecursiveGaussianImageFilter<TInputImage,TOutputImage,TComputation> 
          Superclass;
  typedef SmartPointer<Self>                  Pointer;
  typedef SmartPointer<const Self>  ConstPointer;
  
  /** Method for creation through the object factory. */
  itkNewMacro(Self);  
  
protected:
  FirstDerivativeRecursiveGaussianImageFilter() {};
  virtual ~FirstDerivativeRecursiveGaussianImageFilter() {};  

  /** Set up the coefficients of the filter to approximate a specific kernel.
   * typically it can be used to approximate a Gaussian or one of its
   * derivatives. */
  virtual void SetUp(void);

private:
  FirstDerivativeRecursiveGaussianImageFilter(const Self&); //purposely not implemented
  void operator=(const Self&); //purposely not implemented

};
  
} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkFirstDerivativeRecursiveGaussianImageFilter.txx"
#endif

#endif




