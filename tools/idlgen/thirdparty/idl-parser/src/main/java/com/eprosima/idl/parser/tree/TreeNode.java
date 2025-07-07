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
// 2023-07 ~ 2025-05: idlgen optimization: java typedef, typeobject improvement, cpp generation fix, type validation, etc.
// ------------------------------------------------------------------

package com.eprosima.idl.parser.tree;

import com.eprosima.idl.context.Context;

import java.util.Map;
import java.util.HashMap;
import java.util.List;
import java.util.ArrayList;
import java.util.Arrays;

import org.antlr.v4.runtime.Token;

public class TreeNode implements Notebook
{
    public TreeNode(String scopeFile, boolean isInScope, String scope, String name, Token tk)
    {
        int index = name.lastIndexOf("::");

        if (index != -1) {
            m_scope = (scope.equals("") ? "" : scope + "::") + name.substring(0, index);
            m_name = name.substring(index+2);
        } else {
            m_scope = scope;
            m_name = name;
        }
        m_name_origin = name;
        m_scopeFile = scopeFile;
        m_isinscope = isInScope;
        m_annotations = new HashMap<String, Annotation>();
        tk_ = tk;
    }

    public TreeNode(TreeNode other) {
        this.m_scopeFile = other.m_scopeFile;
        this.m_isinscope = other.m_isinscope;
        this.m_name = other.m_name;
        this.m_name_origin = other.m_name_origin;
        this.m_scope = other.m_scope;
        this.m_annotations = new HashMap<String, Annotation>();
        for (Map.Entry<String, Annotation> entry : other.m_annotations.entrySet()) {
            this.m_annotations.put(entry.getKey(), new Annotation(entry.getValue()));
        }
        this.tk_ = other.tk_;
    }

    public String getScopeFile()
    {
        return m_scopeFile;
    }

    public boolean isInScope()
    {
        return m_isinscope;
    }

    public boolean isStream()
    {
        return m_name.contains("_stream");
    }

    public String getName()
    {
        return m_name;
    }

    public String getNameOrigin()
    {
        return m_name_origin;
    }

    public ArrayList<String> getScopednameList()
    {
        if (m_scope == null || m_scope.isEmpty())
            return null;
        String[] namespaces = m_scope.split("::");

        ArrayList<String> namespaceList = new ArrayList<>(Arrays.asList(namespaces));
        return namespaceList;
    }

    public String getCTypeName()
    {
        return m_name.replace("_", "_1");
    }

    public String getUnderlineName()
    {
        return m_name.replace("_", "_1");
    }

    public String getScopedname()
    {
        if(m_scope == null || m_scope.isEmpty())
            return m_name;

        return m_scope + "::" + m_name;
    }

    public String getROS2Scopedname()
    {
        if(m_scope == null || m_scope.isEmpty())
            return m_name;

        return m_scope + "::dds_::" + m_name + "_";
    }

    public String getCScopedname()
    {
        if(m_scope.isEmpty())
            return m_name;
        return m_scope.replace("::", "_") + "_" + m_name;
    }

    public String getJavaScopedname()
    {
        if(m_scope.isEmpty())
            return m_name;

        return m_scope.replace("::", ".") + "." + m_name;
    }

    public String getRustScopedname()
    {
        // if(m_scope.isEmpty())
        //     return "crate::" + m_name;
        // return "crate::" + m_scope + "::" + m_name;
        return m_name;
    }

    public String getScope()
    {
        return m_scope;
    }

    public String getUnderlineScope()
    {
        return m_scope.replace("_", "_1");
    }
    /*
     * @brief This function returns the scoped name of the interface but
     * changing "::" by "_".
     */
    public String getFormatedScopedname()
    {
        String ret = null;

        if(m_scope == null || m_scope.isEmpty())
            ret = m_name;
        else
            ret = m_scope + "::" + m_name;

        return ret.replaceAll("::", "_");
    }

    public boolean getHasScope()
    {
        return !(m_scope == null || m_scope.isEmpty());
    }

    @Override
    public void addAnnotation(Context ctx, Annotation annotation)
    {
        if(annotation != null)
            m_annotations.put(annotation.getName(), annotation);
    }

    @Override
    public Map<String, Annotation> getAnnotations()
    {
        return m_annotations;
    }

    public boolean isHasMethod() {
        Annotation ann = m_annotations.get("method");
        if (ann != null)
        {
            return ann.getValue().toUpperCase().equals("TRUE");
        }
        return false;
    }

    public Token getToken()
    {
        return tk_;
    }

    private String m_scopeFile = null;
    private boolean m_isinscope = false;
    private String m_name = null;
    private String m_name_origin = null;
    private String m_scope = null;
    //! Map that stores the annotations of the interface.
    private HashMap<String, Annotation> m_annotations = null;
    //! IDL Parser token
    Token tk_ = null;
}
