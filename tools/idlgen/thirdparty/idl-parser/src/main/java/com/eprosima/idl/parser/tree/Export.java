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
// 2024-02 ~ 2025-05: idlgen optimization: typeobject fix, production efficiency improvement, etc.
// ------------------------------------------------------------------

package com.eprosima.idl.parser.tree;

import com.eprosima.idl.context.Context;

public interface Export
{
    void setParent(Object obj);

    Object getParent();

    public boolean isIsOperation();

    public boolean isIsException();

    public boolean isIsTypeDeclaration();

    public boolean isIsConstDeclaration();

    // TODO Capturar el error en la gramatica y saltarlo.
    public boolean resolve(Context ctx);

    Export copy();
}
