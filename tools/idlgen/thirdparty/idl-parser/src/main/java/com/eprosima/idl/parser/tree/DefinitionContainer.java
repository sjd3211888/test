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

import java.util.ArrayList;
import java.util.List;
import java.util.Iterator;
import org.antlr.v4.runtime.Token;

public class DefinitionContainer extends TreeNode {
    protected DefinitionContainer(String scopeFile, boolean isInScope, String scope, String name, Token token) {
        super(scopeFile, isInScope, scope, name, token);

        m_definitions = new ArrayList<Definition>();
    }

    public DefinitionContainer(DefinitionContainer other) {
        super(other);

        this.m_definitions = new ArrayList<>();
        for (Definition def : other.m_definitions) {
            this.m_definitions.add(def.copy());
        }
    }

    public void add(Definition def) {
        m_definitions.add(def);
        def.setParent(this);
    }

    public void addAll(List<Definition> defs) {
        for (Definition def : defs) {
            m_definitions.add(def);
            def.setParent(this);
        }
    }

    public void delete(Definition def) {
        Iterator<Definition> it = m_definitions.iterator();
        while (it.hasNext()) {
            Definition currentDef = it.next();
            if (currentDef.isIsModule()) {
                Module item = (Module) currentDef;
                String scopedName = item.getScopedname();
                if (scopedName == null) {
                    return;
                }
                item.delete(def);
            } else if (currentDef.isIsTypeDeclaration()) {
                TypeDeclaration item = (TypeDeclaration) currentDef;
                if (item.getScopedname().equals(((TypeDeclaration) def).getScopedname())) {
                    it.remove();
                }
            }
        }
    }

    public ArrayList<Definition> getDefinitions() {
        return m_definitions;
    }

    public ArrayList<TypeDeclaration> getDefinitionsScopeNames() {
        ArrayList<TypeDeclaration> qualifiedNames = new ArrayList<>();
        for (Definition def : m_definitions) {
            if (def.isIsTypeDeclaration()) {
                TypeDeclaration typedecl = (TypeDeclaration) def;
                qualifiedNames.add(typedecl);

            } else if (def.isIsModule()) {
                DefinitionContainer container = (DefinitionContainer) def;
                qualifiedNames.addAll(container.getDefinitionsScopeNames());
            }
        }
        return qualifiedNames;
    }

    private ArrayList<Definition> m_definitions;
}
