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
// 2024-02 ~ 2024-12: Feature enhancement of idlgen and idl2xml: xcdr support, idl type validation, and improvement of union and typeobject features, etc.
// ------------------------------------------------------------------

package com.eprosima.idl.parser.typecode;

import java.util.List;
import java.util.ArrayList;

public class UnionMember extends Member
{
    public UnionMember(TypeCode typecode, String name, List<String> labels, boolean isDefault)
    {
        super(typecode, name);
        m_internallabels = labels;
        m_default = isDefault;
    }

    public UnionMember(UnionMember other)
    {
        super(other);
        m_internallabels = other.getInternalLabels();
        m_default = other.isDefault();
        this.m_internallabels = new ArrayList<String>();
        for (String internallabel : other.m_internallabels) {
            this.m_internallabels.add(new String(internallabel));
        }
        this.m_labels = new ArrayList<String>();
        for (String label : other.m_labels) {
            this.m_labels.add(new String(label));
        }
        this.m_javalabels = new ArrayList<String>();
        for (String javalabel : other.m_javalabels) {
            this.m_javalabels.add(new String(javalabel));
        }
        this.m_values = new ArrayList<String>();
        for (String value : other.m_values) {
            this.m_values.add(new String(value));
        }
    }

    public UnionMember copy() {
        return new UnionMember(this);
    }

    public List<String> getInternalLabels()
    {
        return m_internallabels;
    }

    public List<String> getLabels()
    {
        return m_labels;
    }

    public void setLabels(List<String> labels)
    {
        m_labels = labels;
    }

    public List<String> getValues()
    {
        return m_values;
    }

    public void setValues(List<String> values)
    {
        m_values = values;
    }

    public List<String> getJavaLabels()
    {
        return m_javalabels;
    }

    public void setJavaLabels(List<String> labels)
    {
        m_javalabels = labels;
    }

    public boolean isDefault()
    {
        return m_default;
    }

    public boolean isPrintable()
    {
        return m_default || (null != m_labels && 0 < m_labels.size());
    }

    private List<String> m_internallabels = null;
    private List<String> m_labels = null;
    private List<String> m_javalabels = null;
    private List<String> m_values = null;

    private boolean m_default = false;
}
