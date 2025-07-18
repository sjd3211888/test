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
// 2024-05 ~ 2025-05: idlgen optimization: typeobject fix, production efficiency improvement, etc.
// ------------------------------------------------------------------

package com.eprosima.idl.parser.tree;

import java.util.ArrayList;
import org.antlr.v4.runtime.Token;

public class ExportContainer extends TreeNode
{
    protected ExportContainer(String scopeFile, boolean isInScope, String scope, String name, Token token)
    {
        super(scopeFile, isInScope, scope, name, token);

        m_exports = new ArrayList<Export>();
    }

    protected ExportContainer(ExportContainer other)
    {
        super(other);

        m_exports = new ArrayList<Export>();
        for (Export exp_dec : other.getExports()) {
            this.m_exports.add(exp_dec.copy());
        }
    }

    public void add(Export exp)
    {
        m_exports.add(exp);
        exp.setParent(this);
    }

    public ArrayList<Export> getExports()
    {
        return m_exports;
    }

    private ArrayList<Export> m_exports;
}
