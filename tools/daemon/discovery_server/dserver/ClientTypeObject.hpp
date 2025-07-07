/*
 * Copyright (c) 2025 Li Auto Inc. and its affiliates
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _VBS_DDS_GENERATED_Client_TYPE_OBJECT_H_
#define _VBS_DDS_GENERATED_Client_TYPE_OBJECT_H_

#include <ertps/types/TypeObject.h>
#include <map>

#if defined(_WIN32)
#if defined(VBS_USER_DLL_EXPORT)
#define vbs_user_DllExport __declspec(dllexport)
#else
#define vbs_user_DllExport
#endif
#else
#define vbs_user_DllExport
#endif

#if defined(_WIN32)
#if defined(VBS_USER_DLL_EXPORT)
#if defined(Client_SOURCE)
#define C_DllAPI __declspec(dllexport)
#else
#define Client_DllAPI __declspec(dllimport)
#endif  // Client_SOURCE
#else
#define Client_DllAPI
#endif
#else
#define Client_DllAPI
#endif  // _WIN32

using namespace evbs::ertps::types;

vbs_user_DllExport void registerClientTypes();

vbs_user_DllExport const TypeIdentifier* GetClientIdentifier();
vbs_user_DllExport const TypeObject* GetClientObject();
vbs_user_DllExport const TypeObject* GetCompleteClientObject();

#endif  // _VBS_DDS_GENERATED_Client_TYPE_OBJECT_H_
