// Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ------------------------------------------------------------------
// Modification history:
// feature: Typeobject adaptation in type discovery requirements
// ------------------------------------------------------------------

#ifndef VBS_TYPEOBJECT_HPP_
#define VBS_TYPEOBJECT_HPP_

#include "deps/type/TypeObject.h"
#include "deps/type/TypeObjectFactory.h"
#include "deps/type/TypeNamesGenerator.h"
#include "deps/type/AnnotationParameterValue.h"

namespace vbs {

using TypeIdentifier = vbs::common::types::TypeIdentifier;
using TypeObject = vbs::common::types::TypeObject;
using TypeObjectFactory = vbs::common::types::TypeObjectFactory;

const unsigned char EK_COMPLETE = vbs::common::types::EK_COMPLETE;  // 0x1111 0010

using TypeNamesGenerator = vbs::common::types::TypeNamesGenerator;
using CompleteStructMember = vbs::common::types::CompleteStructMember;
using CompleteStructType = vbs::common::types::CompleteStructType;
using CompleteBitflag = vbs::common::types::CompleteBitflag;
using CompleteBitsetType = vbs::common::types::CompleteBitsetType;
using CompleteBitfield = vbs::common::types::CompleteBitfield;
using CompleteAnnotationType = vbs::common::types::CompleteAnnotationType;
using CompleteAnnotationParameter = vbs::common::types::CompleteAnnotationParameter;
using CompleteAliasType = vbs::common::types::CompleteAliasType;
using CompleteEnumeratedType = vbs::common::types::CompleteEnumeratedType;
using CompleteEnumeratedLiteral = vbs::common::types::CompleteEnumeratedLiteral;
using CompleteUnionMember = vbs::common::types::CompleteUnionMember;
using CompleteUnionType = vbs::common::types::CompleteUnionType;
using CompleteBitmaskType = vbs::common::types::CompleteBitmaskType;
using AnnotationParameterValue = vbs::common::types::AnnotationParameterValue;
using AppliedAnnotation = vbs::common::types::AppliedAnnotation;
using AppliedAnnotationParameter = vbs::common::types::AppliedAnnotationParameter;

}  // namespace vbs

#endif  // VBS_TYPEOBJECT_HPP_
