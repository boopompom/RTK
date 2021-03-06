/*=========================================================================
 *
 *  Copyright RTK Consortium
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

#ifndef rtkProjectGeometricPhantomImageFilter_hxx
#define rtkProjectGeometricPhantomImageFilter_hxx

#include "rtkProjectGeometricPhantomImageFilter.h"
#include "rtkGeometricPhantomFileReader.h"
#include "rtkForbildPhantomFileReader.h"
#include "rtkRayConvexIntersectionImageFilter.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>

namespace rtk
{
template <class TInputImage, class TOutputImage>
ProjectGeometricPhantomImageFilter<TInputImage, TOutputImage>
::ProjectGeometricPhantomImageFilter():
m_PhantomScale(1.),
m_OriginOffset(0.),
m_IsForbildConfigFile(false)
{
  m_RotationMatrix.SetIdentity();
}

template< class TInputImage, class TOutputImage >
void ProjectGeometricPhantomImageFilter< TInputImage, TOutputImage >::GenerateData()
{
  //Reading figure config file
  if(! m_ConfigFile.empty() )
    {
    if(m_IsForbildConfigFile)
      {
      typedef rtk::ForbildPhantomFileReader ReaderType;
      ReaderType::Pointer reader = ReaderType::New();
      reader->SetFilename(m_ConfigFile);
      reader->GenerateOutputInformation();
      this->m_GeometricPhantom = reader->GetGeometricPhantom();
      }
    else
      {
      typedef rtk::GeometricPhantomFileReader ReaderType;
      ReaderType::Pointer reader = ReaderType::New();
      reader->SetFilename(m_ConfigFile);
      reader->GenerateOutputInformation();
      this->m_GeometricPhantom = reader->GetGeometricPhantom();
      }
    }

  //Check that it's not empty
  const GeometricPhantom::ConvexShapeVector &cov = m_GeometricPhantom->GetConvexShapes();
  if( cov.size() == 0 )
    itkExceptionMacro(<< "Empty phantom");

  // Create one add filter per convex object
  std::vector< typename itk::ImageSource<TOutputImage>::Pointer > projectors;
  for(size_t i=0; i<cov.size(); i++)
    {
    ConvexShape::Pointer co = cov[i]->Clone();
    co->Rotate( m_RotationMatrix );
    co->Translate( m_OriginOffset );
    co->Rescale( m_PhantomScale );

    if( projectors.size() )
      {
      typedef RayConvexIntersectionImageFilter<TOutputImage, TOutputImage>  RCOIType;
      typename RCOIType::Pointer rcoi = RCOIType::New();
      rcoi->SetInput(projectors.back()->GetOutput());
      rcoi->SetGeometry(this->GetGeometry());
      rcoi->SetConvexShape(co);
      projectors.push_back( rcoi.GetPointer() );
      }
    else
      {
      typedef RayConvexIntersectionImageFilter<TInputImage, TOutputImage>  RCOIType;
      typename RCOIType::Pointer rcoi = RCOIType::New();
      rcoi->SetInput(this->GetInput());
      rcoi->SetGeometry(this->GetGeometry());
      rcoi->SetConvexShape(co);
      projectors.push_back( rcoi.GetPointer() );
      }
    }

  projectors.back()->GetOutput()->SetRequestedRegion( this->GetOutput()->GetRequestedRegion() );
  projectors.back()->Update();
  this->GraftOutput( projectors.back()->GetOutput() );
}

} // end namespace rtk

#endif
